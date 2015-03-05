# Utilities functions shared by all cleanup scripts

VERIFICATION=1
ITERATIONS=4
export PS_WORK_PER_THREAD=1000

generate_attribute_graph()
{
    local datasetpath=$1
    local attribute=$2

    mkdir -p $datasetpath/$attribute

    echo "Sorting user IDs lexicographically ($attribute) ..."
    gunzip -c $datasetpath/$attribute/attrs.gz | LC_ALL=C sort -k2 | \
        awk '{print $1}' | gzip -c > $datasetpath/$attribute/ids-by-attr.gz

    echo "Generating universe for $attribute ..."
    gunzip -c $datasetpath/$attribute/ids-by-attr.gz | wc -l > $datasetpath/$attribute/universe

    echo "Reshaping graph according to $attribute lexicographic order ..."
    gunzip -c $datasetpath/graph.tsv.gz | \
        cpp/build/reassign_ids -i $datasetpath/$attribute/ids-by-attr.gz --translation UserIdToSortId | \
        LC_ALL=C sort -k1n -k2n -S 10G --parallel=8 --compress-program=gzip | gzip -c > $datasetpath/$attribute/graph.tsv.gz

    echo "Creating $attribute dictionary ..."
    gunzip -c $datasetpath/$attribute/attrs.gz | awk -F'\t' '{print $2}' | \
        LC_ALL=C sort | gzip -c > $datasetpath/$attribute/dict.gz

    for dictionary in "permuterm" "strarray"; do
        echo "Creating $dictionary for $attribute ..."
        gunzip -c $datasetpath/$attribute/dict.gz | \
            cpp/build/create_dictionary --type $dictionary -o $datasetpath/$attribute/dict.$dictionary
    done

    echo "Reshaping ranking ..."
    gunzip -c $datasetpath/ranking.tsv.gz | \
        cpp/build/reassign_ids -i $datasetpath/$attribute/ids-by-attr.gz --translation UserIdToSortId -c 0 | \
        LC_ALL=C sort -k1n | gzip -c > $datasetpath/$attribute/ranking.tsv.gz

    echo "Creating WAND auxiliary data ..."
    gunzip -c $datasetpath/$attribute/graph.tsv.gz | \
        cpp/build/reassign_ids -i $datasetpath/$attribute/ranking.tsv.gz --translation UserIdToString -c 1 | \
        LC_ALL=C sort -k1n -k2nr -S 10G --parallel=8 --compress-program=gzip | awk '!seen[$1]++' | \
        gzip -c > $datasetpath/$attribute/wand.tsv.gz
}

ensure_paths()
{
    local datasetpath=$1
    declare -a attributes=("${!2}")

    for attr in "${attributes[@]}"; do
        echo "Creating directory $datasetpath/$attr"
        mkdir -p $datasetpath/$attr
    done
}

collect_statistics()
{
    local datasetpath=$1

    echo "Generating universe ..."
    gunzip -c $datasetpath/graph.tsv.gz | \
        awk 'BEGIN {max = 0} {if ($1 > max) max = $1; if ($2 > max) max = $2} END {print max + 1}' > $datasetpath/universe

    echo "Gathering degree statistic ..."
    gunzip -c $datasetpath/graph.tsv.gz | \
        awk '{print $1}' | uniq -c | awk '{print $2 "\t" $1}' | \
        gzip -c > $datasetpath/degree.stats.gz

    gunzip -c $datasetpath/degree.stats.gz | \
        awk '{print $2}' | LC_ALL=C sort -k1n | uniq -c | awk '{print $2 "\t" $1}' | \
        gzip -c > $datasetpath/degree-cdf.stats.gz

    echo "Generating random ranking ..."
    gunzip -c $datasetpath/graph.tsv.gz | awk '{print $1 "\n" $2}' | LC_ALL=C sort -k1n -u | \
        LC_ALL=C sort --random-sort | awk '{print $1 "\t" NR-1}' | \
        LC_ALL=C sort -k1n | gzip -c > $datasetpath/ranking.tsv.gz
}

sample_queries()
{
    local datasetpath=$1
    declare -a attributes=("${!2}")
    local numusers=${3:-1000}
    local numbuckets=${4:-10}
    local verification=${5:-$VERIFICATION}
    local encoding="block_varint_simple"

    echo "Sampling $numusers users for the queries ..."
    gunzip -c $datasetpath/degree.stats.gz | sort -k2n -k1n | \
        python scripts/sample_users.py -k $numusers -b $numbuckets --sample=bucket | \
        sort -k1n -k3n -k2n > $datasetpath/query-users.txt

    tmpdir=$(mktemp -d 2>/dev/null || mktemp -d -t 'mytmpdir')
    echo "Using $tmpdir as temporary directory ..."

    for attribute in "${attributes[@]}"; do
        echo "Generating temporary index N1 using $encoding encoding ..."
        unpigz -c $datasetpath/$attribute/graph.tsv.gz | \
            cpp/build/create_index --universe $(cat $datasetpath/$attribute/universe) -t $encoding -o $tmpdir/n1_${encoding}_${attribute}

        let stop="$numbuckets-1"

        for bucket in $(seq 0 $stop); do
            echo "Sampling queries for $attribute attribute for bucket $bucket (FoF) ..."
            cat $datasetpath/query-users.txt | grep "^$bucket\s" | awk '{print $2}' | \
                cpp/build/sample_queries -n 2 -t ${encoding} -i $tmpdir/n1_${encoding}_${attribute} \
                    --id-mapping $datasetpath/$attribute/ids-by-attr.gz \
                    --str-mapping $datasetpath/$attribute/attrs.gz \
                    --verification $verification \
                    > $datasetpath/$attribute/queries-FoF-$bucket.txt

            echo "Sampling queries for $attribute attribute for bucket $bucket (Friend) ..."
            cat $datasetpath/query-users.txt | grep "^$bucket\s" | awk '{print $2}' | \
                cpp/build/sample_queries -n 1 -t ${encoding} -i $tmpdir/n1_${encoding}_${attribute} \
                    --id-mapping $datasetpath/$attribute/ids-by-attr.gz \
                    --str-mapping $datasetpath/$attribute/attrs.gz \
                    --verification $verification \
                    > $datasetpath/$attribute/queries-Friend-$bucket.txt
        done
    done

    rm -rf $tmpdir
}

compact_graph()
{
    local datasetpath=$1
    local attribute="_id"
    declare -a attributes=("${!2}")

    mkdir -p $datasetpath/$attribute

    echo "Compacting IDs of the original graph ..."
    zcat $datasetpath/graph.tsv.gz | awk '{print $1 "\n" $2}' | sort -k1n -u | gzip -c > $datasetpath/$attribute/ids-by-attr.gz

    echo "Generating universe for $attribute ..."
    gunzip -c $datasetpath/$attribute/ids-by-attr.gz | wc -l > $datasetpath/$attribute/universe

    # Sorting is not needed since they are already sorted
    echo "Reshaping graph according to $attribute lexicographic order ..."
    gunzip -c $datasetpath/graph.tsv.gz | \
        cpp/build/reassign_ids -i $datasetpath/$attribute/ids-by-attr.gz --translation UserIdToSortId | \
        gzip -c > $datasetpath/$attribute/graph.tsv.gz

    for attr in "${attributes[@]}"; do
        echo "Generate dict-remapping for $attr ..."
        gunzip -c $datasetpath/$attr/ids-by-attr.gz | \
            cpp/build/reassign_ids -i $datasetpath/$attribute/ids-by-attr.gz --translation UserIdToSortId -c 0 | \
            gzip -c > $datasetpath/$attribute/dict-remapping.$attr
        gunzip -c $datasetpath/$attribute/ids-by-attr.gz | \
            cpp/build/reassign_ids -i $datasetpath/$attr/ids-by-attr.gz --translation UserIdToSortId -c 0| \
            gzip -c > $datasetpath/$attribute/fast-dict-remapping.$attr
    done

    echo "Reshaping ranking ..."
    gunzip -c $datasetpath/ranking.tsv.gz | \
        cpp/build/reassign_ids -i $datasetpath/$attribute/ids-by-attr.gz --translation UserIdToSortId -c 0 | \
        LC_ALL=C sort -k1n | gzip -c > $datasetpath/$attribute/ranking.tsv.gz

    echo "Creating WAND auxiliary data ..."
    gunzip -c $datasetpath/$attribute/graph.tsv.gz | \
        cpp/build/reassign_ids -i $datasetpath/$attribute/ranking.tsv.gz --translation UserIdToString -c 1 | \
        LC_ALL=C sort -k1n -k2nr -S 10G --parallel=8 --compress-program=gzip | awk '!seen[$1]++' | \
        gzip -c > $datasetpath/$attribute/wand.tsv.gz
}

create_n1_index()
{
    local datasetpath=$1
    local outputpath=$2
    local encoding=$3
    local attribute=$4

    echo "Creating N1 index ..."
    unpigz -c $datasetpath/$attribute/graph.tsv.gz | \
        cpp/build/create_index \
            --universe $(cat $datasetpath/$attribute/universe) -t "${encoding}_simple" \
            -o $outputpath/$attribute/n1_${encoding}_simple
}

create_n2_index()
{
    local datasetpath=$1
    local outputpath=$2
    local encoding=$3
    local attribute=$4

    echo "Creating N2 index (piping friends_at_k) ..."
    unpigz -c $datasetpath/$attribute/graph.tsv.gz | \
        cpp/build/friends_at_k -t "${encoding}_simple" -i $outputpath/$attribute/n1_${encoding}_simple | \
        cpp/build/create_index \
            --universe $(cat $datasetpath/$attribute/universe) -t "${encoding}_simple" \
            -o $outputpath/$attribute/n2_${encoding}_simple
}

create_baseline_n1_index()
{
    local datasetpath=$1
    local outputpath=$2
    local encoding=$3
    local attribute=$4

    echo "Creating baseline N1 index ..."
    unpigz -c $datasetpath/_id/graph.tsv.gz | \
        cpp/build/create_index \
            --universe $(cat $datasetpath/_id/universe) -t "${encoding}_simple" \
            -o $outputpath/$attribute/baseline_n1_${encoding}_simple
}

create_baseline_n2_index()
{
    local datasetpath=$1
    local outputpath=$2
    local encoding=$3
    local attribute=$4

    echo "Creating baseline N2 index (piping friends_at_k) ..."
    unpigz -c $datasetpath/_id/graph.tsv.gz | \
        cpp/build/friends_at_k -t "${encoding}_simple" -i $outputpath/$attribute/baseline_n1_${encoding}_simple | \
        cpp/build/create_index \
            --universe $(cat $datasetpath/_id/universe) -t "${encoding}_simple" \
            -o $outputpath/$attribute/baseline_n2_${encoding}_simple
}

create_topk_n1_index()
{
    local datasetpath=$1
    local outputpath=$2
    local encoding=$3
    local attribute=$4

    echo "Creating topk N1 index ..."
    unpigz -c $datasetpath/$attribute/graph.tsv.gz | \
        cpp/build/create_index \
            --universe $(cat $datasetpath/$attribute/universe) -t "${encoding}_topk" \
            --ranking $datasetpath/$attribute/ranking.tsv.gz \
            -o $outputpath/$attribute/topk_n1_${encoding}_topk
}

execute_queries()
{
    local filename=$1
    local datasetpath=$2
    local indexpath=$3
    local resultspath=$4
    local bucket=$5
    local attribute=$6
    local encoding=$7
    local index=$8
    local scheme=$9
    local dictionary=${10}
    local friends=${11}
    local dictremapping=${12}

    if [ -n "$dictremapping" ]; then
        idmapping=$datasetpath/_id/ids-by-attr.gz
    else
        idmapping=$datasetpath/$attribute/ids-by-attr.gz
    fi

    cpp/build/simple_scheme \
        --dict-type $dictionary --dictionary $datasetpath/$attribute/dict.$dictionary \
        -t $encoding -i $indexpath/$attribute/$index \
        --id-mapping $idmapping \
        --dict-remapping "$dictremapping" \
        --query-file $datasetpath/$attribute/queries-$friends-$bucket.txt \
        --iterations $ITERATIONS \
        --scheme $scheme > $resultspath/$attribute/$bucket-$dictionary-$encoding-$filename.perf
}

topk_queries()
{
    local topk=$1
    local filename=$2
    local datasetpath=$3
    local indexpath=$4
    local resultspath=$5
    local bucket=$6
    local attribute=$7
    local encoding=$8
    local index=$9
    local scheme=${10}
    local dictionary=${11}
    local friends=${12}
    local dictremapping=${13}

    if [ -n "$dictremapping" ]; then
        idmapping=$datasetpath/_id/ids-by-attr.gz
    else
        idmapping=$datasetpath/$attribute/ids-by-attr.gz
    fi

    if [[ $scheme == *-wand ]]; then
        wand=$datasetpath/$attribute/wand.tsv.gz
    fi

    cpp/build/simple_scheme \
        --dict-type $dictionary --dictionary $datasetpath/$attribute/dict.$dictionary \
        -t $encoding -i $indexpath/$attribute/$index \
        --id-mapping $idmapping \
        --dict-remapping "$dictremapping" \
        --query-file $datasetpath/$attribute/queries-$friends-$bucket.txt \
        --iterations $ITERATIONS \
        --topk $topk \
        --ranking $datasetpath/$attribute/ranking.tsv.gz \
        --wand "$wand" \
        --scheme $scheme > $resultspath/$attribute/$bucket-$dictionary-$encoding-$filename.perf
}

statistics_for()
{
    local datasetpath=$1
    local dataset=$2
    local attribute=$3
    local output=$4

    local vn1=$(cat $DATASETDIR/$dataset/_id/universe)
    let vn1=$vn1-1

    local en1=$(gunzip -c $DATASETDIR/$dataset/_id/graph.tsv.gz | wc -l)
    local dn1=$(echo "scale=3; $en1 / $vn1" | bc)

    local ds=$(gunzip -c $DATASETDIR/$dataset/$attribute/attrs.gz | awk -F'\t' '{print $2}' | wc -m)
    let ds=$ds-$vn1

    local en1=$(gunzip -c $DATASETDIR/$dataset/_id/graph.tsv.gz | wc -l)
    local dn1=$(echo "scale=3; $en1 / $vn1" | bc)


    tmpdir=$(mktemp -d 2>/dev/null || mktemp -d -t 'mytmpdir')

    local encoding="block_varint"
    mkdir $tmpdir/$attribute
    create_baseline_n1_index $datasetpath/$dataset $tmpdir $encoding $attribute
    local en2=$(unpigz -c $datasetpath/$dataset/_id/graph.tsv.gz | \
                cpp/build/friends_at_k --histogram 1 -t "block_varint_simple" -i $tmpdir/$attribute/baseline_n1_${encoding}_simple | \
                awk '{sum += $2} END {print sum}')
    rm -rf $tmpdir

    local dn2=$(echo "scale=3; $en2 / $vn1" | bc)

    echo -e "$dataset\t$vn1\t$en1\t$dn1\t$en2\t$dn2\t$ds" >> $output
}
