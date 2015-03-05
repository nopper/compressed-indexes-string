#!/bin/sh

CURRDIR="$(dirname "$0")"
source $CURRDIR/../utils.sh

DATASETDIR=${1:-datasets/clean}
INDEXDIR=${2:-datasets/db/fof-new}
RESULTSDIR=${3:-datasets/results/fof-new}
BUCKETS=$(seq 0 9)
QUERYLENGTHS=$(seq 1 5)
DICTIONARIES=("permuterm")

build_and_query()
{
    local dataset=$1
    local attribute=$2

    mkdir -p $INDEXDIR/$dataset/$attribute
    mkdir -p $RESULTSDIR/$dataset/$attribute

    create_baseline_n1_index $DATASETDIR/$dataset $INDEXDIR/$dataset "block_varint" $attribute

    for bucket in ${BUCKETS[@]}; do
        for dictionary in ${DICTIONARIES[@]}; do
            execute_queries fbn1 $DATASETDIR/$dataset $INDEXDIR/$dataset $RESULTSDIR/$dataset $bucket $attribute block_varint_simple baseline_n1_block_varint_simple fast-baseline-hopping $dictionary "FoF" $DATASETDIR/$dataset/_id/fast-dict-remapping.$attribute
        done
    done

    create_n1_index $DATASETDIR/$dataset $INDEXDIR/$dataset "ef" $attribute

    for bucket in ${BUCKETS[@]}; do
        for dictionary in ${DICTIONARIES[@]}; do
            execute_queries n1 $DATASETDIR/$dataset $INDEXDIR/$dataset $RESULTSDIR/$dataset $bucket $attribute ef_simple n1_ef_simple hopping $dictionary "FoF"
        done
    done
}

report_query_time()
{
    local dataset=$1
    local attribute=$2

    tmpdir=$(mktemp -d 2>/dev/null || mktemp -d -t 'mytmpdir')

    for querylen in ${QUERYLENGTHS[@]}; do
        local command="python scripts/renderer.py --skip=1 --format=tsv --query-len=$querylen"

        command+=" $dataset-n1="
        for bucket in ${BUCKETS[@]}; do
            command+="$RESULTSDIR/$dataset/$attribute/$bucket-permuterm-ef_simple-n1.perf,"
        done

        command+=" $dataset-fbn1="
        for bucket in ${BUCKETS[@]}; do
            command+="$RESULTSDIR/$dataset/$attribute/$bucket-permuterm-block_varint_simple-fbn1.perf,"
        done

        command+=" | awk '{print \$1 \"\\t\" \$4}' > $tmpdir/$querylen"

        # echo $command
        eval $command
    done

    local covered=0
    local command="join -t $'\\t' "

    for querylen in ${QUERYLENGTHS[@]}; do
        if [ $covered -ge 2 ]; then
            command+="| join -t $'\\t' - $tmpdir/$querylen"
        else
            command+="$tmpdir/$querylen "
        fi
        let covered+=1
    done

    # echo $command
    eval $command

    rm -rf $tmpdir
}

plot_speed()
{
    local dataset=$1
    local attribute=$2
    local querylen=$3
    local xticks=$4

    command="python scripts/renderer.py --format=pdf --xticks=$xticks --output=$RESULTSDIR/$dataset/$attribute/FoF-$querylen-permuterm.pdf --query-len=$querylen --skip=1 "
    command+="--xlabel \"Degree\" --ylabel 'Time (in \$\\mu\$s)' "

    for bucket in ${BUCKETS[@]}; do
        command+="\\\\textsf{Scan}=$RESULTSDIR/$dataset/$attribute/$bucket-permuterm-block_varint_simple-fbn1.perf "
        command+="\\\\textsf{Range}=$RESULTSDIR/$dataset/$attribute/$bucket-permuterm-ef_simple-n1.perf "
    done

    eval $command
    # echo $command
}

build_and_query "dblp" "fname"
# build_and_query "livejournal" "name"
# build_and_query "twitter" "screen"

report_query_time "dblp" "fname"
# report_query_time "livejournal" "name"
# report_query_time "twitter" "screen"

# plot_speed "twitter" "screen" 1 "[1-20],[20-34],[34-89],[89-215],[215-416],[416-775],[775-1274],[1274-1962],[1962-4055],[4055-770155]"
# plot_speed "twitter" "screen" 2 "[1-20],[20-34],[34-89],[89-215],[215-416],[416-775],[775-1274],[1274-1962],[1962-4055],[4055-770155]"
# plot_speed "twitter" "screen" 3 "[1-20],[20-34],[34-89],[89-215],[215-416],[416-775],[775-1274],[1274-1962],[1962-4055],[4055-770155]"
# plot_speed "twitter" "screen" 4 "[1-20],[20-34],[34-89],[89-215],[215-416],[416-775],[775-1274],[1274-1962],[1962-4055],[4055-770155]"
# plot_speed "twitter" "screen" 5 "[1-20],[20-34],[34-89],[89-215],[215-416],[416-775],[775-1274],[1274-1962],[1962-4055],[4055-770155]"
