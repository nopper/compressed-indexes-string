#!/bin/sh

CURRDIR="$(dirname "$0")"
source $CURRDIR/../utils.sh

DATASETDIR=${1:-datasets/clean}
INDEXDIR=${2:-datasets/db/encodings}
RESULTSDIR=${3:-datasets/results/encodings}
BUCKETS=$(seq 0 9)
QUERYLENGTHS=$(seq 1 5)
DICTIONARIES=("permuterm")
ENCODINGS=("block_varint" "block_interpolative" "block_optpfor" "ef")

build_and_query()
{
    local dataset=$1
    local encoding=$2
    local attribute=$3

    mkdir -p $INDEXDIR/$dataset/$attribute
    mkdir -p $RESULTSDIR/$dataset/$attribute

    create_n1_index $DATASETDIR/$dataset $INDEXDIR/$dataset $encoding $attribute
    create_baseline_n1_index $DATASETDIR/$dataset $INDEXDIR/$dataset $encoding $attribute

    for bucket in ${BUCKETS[@]}; do
        for dictionary in ${DICTIONARIES[@]}; do
            execute_queries n1 $DATASETDIR/$dataset $INDEXDIR/$dataset $RESULTSDIR/$dataset $bucket $attribute ${encoding}_simple n1_${encoding}_simple asindex $dictionary "Friend"
            execute_queries bn1 $DATASETDIR/$dataset $INDEXDIR/$dataset $RESULTSDIR/$dataset $bucket $attribute ${encoding}_simple baseline_n1_${encoding}_simple baseline-asindex $dictionary "Friend" $DATASETDIR/$dataset/_id/dict-remapping.$attribute
            execute_queries fbn1 $DATASETDIR/$dataset $INDEXDIR/$dataset $RESULTSDIR/$dataset $bucket $attribute ${encoding}_simple baseline_n1_${encoding}_simple fast-baseline-asindex $dictionary "Friend" $DATASETDIR/$dataset/_id/fast-dict-remapping.$attribute
        done
    done
}

report_query_time()
{
    local dataset=$1
    local encoding=$2
    local attribute=$3

    tmpdir=$(mktemp -d 2>/dev/null || mktemp -d -t 'mytmpdir')

    for querylen in ${QUERYLENGTHS[@]}; do
        local command="python scripts/renderer.py --skip=1 --format=tsv --query-len=$querylen"

        command+=" $dataset-$encoding-n1="
        for dictionary in ${DICTIONARIES[@]}; do
            for bucket in ${BUCKETS[@]}; do
                command+="$RESULTSDIR/$dataset/$attribute/$bucket-$dictionary-${encoding}_simple-n1.perf,"
            done
        done

        command+=" $dataset-$encoding-bn1="
        for dictionary in ${DICTIONARIES[@]}; do
            for bucket in ${BUCKETS[@]}; do
                command+="$RESULTSDIR/$dataset/$attribute/$bucket-$dictionary-${encoding}_simple-bn1.perf,"
            done
        done

        command+=" $dataset-$encoding-fbn1="
        for dictionary in ${DICTIONARIES[@]}; do
            for bucket in ${BUCKETS[@]}; do
                command+="$RESULTSDIR/$dataset/$attribute/$bucket-$dictionary-${encoding}_simple-fbn1.perf,"
            done
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

    eval $command

    rm -rf $tmpdir
}

report_prefix_search_time()
{
    local dataset=$1
    local encoding=$2
    local attribute=$3
    local dictionary="permuterm"

    tmpdir=$(mktemp -d 2>/dev/null || mktemp -d -t 'mytmpdir')

    for querylen in ${QUERYLENGTHS[@]}; do
        local command="python scripts/renderer.py --skip=1 --format=tsv --query-len=$querylen"

        command+=" $dataset-$encoding-n1="
        for bucket in ${BUCKETS[@]}; do
            command+="$RESULTSDIR/$dataset/$attribute/$bucket-$dictionary-${encoding}_simple-n1.perf,"
        done

        command+=" $dataset-$encoding-bn1="
        for bucket in ${BUCKETS[@]}; do
            command+="$RESULTSDIR/$dataset/$attribute/$bucket-$dictionary-${encoding}_simple-bn1.perf,"
        done

        command+=" $dataset-$encoding-fbn1="
        for bucket in ${BUCKETS[@]}; do
            command+="$RESULTSDIR/$dataset/$attribute/$bucket-$dictionary-${encoding}_simple-fbn1.perf,"
        done

        command+=" | awk '{print \$1 \"\\t\" \$2}' > $tmpdir/$querylen"

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

    eval $command

    rm -rf $tmpdir
}

echo "Building indices ..."
for encoding in ${ENCODINGS[@]}; do
    build_and_query "dblp" $encoding "fname"
    # build_and_query "livejournal" $encoding "name"
    # build_and_query "twitter" $encoding "screen"
done

echo "Reporting query performance ..."
for encoding in ${ENCODINGS[@]}; do
    report_query_time "dblp" $encoding "fname"
    # report_query_time "livejournal" $encoding "name"
    # report_query_time "twitter" $encoding "screen"
done

echo "Reporting prefix search performance ..."
for encoding in ${ENCODINGS[@]}; do
    report_prefix_search_time "dblp" $encoding "fname"
    # report_prefix_search_time "livejournal" $encoding "name"
    # report_prefix_search_time "twitter" $encoding "screen"
done