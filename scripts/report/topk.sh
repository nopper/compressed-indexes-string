#!/bin/sh

CURRDIR="$(dirname "$0")"
source $CURRDIR/../utils.sh

DATASETDIR=${1:-datasets/clean}
INDEXDIR=${2:-datasets/db/topk}
RESULTSDIR=${3:-datasets/results/topk}
BUCKETS=$(seq 0 9)
DICTIONARIES=("permuterm")

build_and_query()
{
    local dataset=$1
    local attribute=$2

    mkdir -p $INDEXDIR/$dataset/$attribute
    mkdir -p $RESULTSDIR/$dataset/$attribute

    create_n1_index $DATASETDIR/$dataset $INDEXDIR/$dataset "ef" $attribute
    create_topk_n1_index $DATASETDIR/$dataset $INDEXDIR/$dataset "ef" $attribute

    for bucket in ${BUCKETS[@]}; do
        for dictionary in ${DICTIONARIES[@]}; do
            topk_queries 10 10-rmq $DATASETDIR/$dataset $INDEXDIR/$dataset $RESULTSDIR/$dataset $bucket $attribute ef_topk topk_n1_ef_topk topk-hopping-rmq $dictionary "FoF"
            topk_queries 10 10-base $DATASETDIR/$dataset $INDEXDIR/$dataset $RESULTSDIR/$dataset $bucket $attribute ef_simple n1_ef_simple topk-hopping $dictionary "FoF"
            topk_queries 10 10-wand $DATASETDIR/$dataset $INDEXDIR/$dataset $RESULTSDIR/$dataset $bucket $attribute ef_simple n1_ef_simple topk-hopping-wand $dictionary "FoF"
            topk_queries 10 10-rmqwand $DATASETDIR/$dataset $INDEXDIR/$dataset $RESULTSDIR/$dataset $bucket $attribute ef_topk topk_n1_ef_topk topk-hopping-rmq-wand $dictionary "FoF"

            topk_queries 5 5-rmq $DATASETDIR/$dataset $INDEXDIR/$dataset $RESULTSDIR/$dataset $bucket $attribute ef_topk topk_n1_ef_topk topk-hopping-rmq $dictionary "FoF"
            topk_queries 5 5-base $DATASETDIR/$dataset $INDEXDIR/$dataset $RESULTSDIR/$dataset $bucket $attribute ef_simple n1_ef_simple topk-hopping $dictionary "FoF"
            topk_queries 5 5-wand $DATASETDIR/$dataset $INDEXDIR/$dataset $RESULTSDIR/$dataset $bucket $attribute ef_simple n1_ef_simple topk-hopping-wand $dictionary "FoF"
            topk_queries 5 5-rmqwand $DATASETDIR/$dataset $INDEXDIR/$dataset $RESULTSDIR/$dataset $bucket $attribute ef_topk topk_n1_ef_topk topk-hopping-rmq-wand $dictionary "FoF"
        done
    done
}

plot_speed()
{
    local dataset=$1
    local attribute=$2
    local querylen=$3
    local topk=$4
    local xticks=$5

    command="python scripts/renderer.py --format=pdf --colors='rbgmcy' --markers='odDv^>' --xticks=$xticks --output=$RESULTSDIR/$dataset/$attribute/FoF-topk-$topk-$querylen-permuterm.pdf --query-len=$querylen --skip=1 "
    # command+="--xlabel \"Degree\" --ylabel 'Time (in \$\\mu\$s)' --offsets='0;15,-20;-20,-15;15,-20;-20' "
    command+="--xlabel \"Degree\" --ylabel 'Time (in \$\\mu\$s)' --offsets='0;5,-10;-10,-5;5,-10;-10' "

    for bucket in ${BUCKETS[@]}; do
        command+="'\\textsf{~Score}=$RESULTSDIR/$dataset/$attribute/$bucket-permuterm-ef_simple-$topk-base.perf' "
        command+="'\\textsf{~WAND}=$RESULTSDIR/$dataset/$attribute/$bucket-permuterm-ef_simple-$topk-wand.perf' "
        command+="'\\textsf{~~RMQ}=$RESULTSDIR/$dataset/$attribute/$bucket-permuterm-ef_topk-$topk-rmq.perf' "
    done

    eval $command
    # echo $command
}

build_and_query "twitter" "screen"
plot_speed "twitter" "screen" 1 5 "[1-20],[20-34],[34-89],[89-215],[215-416],[416-775],[775-1274],[1274-1962],[1962-4055],[4055-770155]"
plot_speed "twitter" "screen" 1 10 "[1-20],[20-34],[34-89],[89-215],[215-416],[416-775],[775-1274],[1274-1962],[1962-4055],[4055-770155]"
plot_speed "twitter" "screen" 2 5 "[1-20],[20-34],[34-89],[89-215],[215-416],[416-775],[775-1274],[1274-1962],[1962-4055],[4055-770155]"
plot_speed "twitter" "screen" 2 10 "[1-20],[20-34],[34-89],[89-215],[215-416],[416-775],[775-1274],[1274-1962],[1962-4055],[4055-770155]"
plot_speed "twitter" "screen" 3 5 "[1-20],[20-34],[34-89],[89-215],[215-416],[416-775],[775-1274],[1274-1962],[1962-4055],[4055-770155]"
plot_speed "twitter" "screen" 3 10 "[1-20],[20-34],[34-89],[89-215],[215-416],[416-775],[775-1274],[1274-1962],[1962-4055],[4055-770155]"
plot_speed "twitter" "screen" 4 5 "[1-20],[20-34],[34-89],[89-215],[215-416],[416-775],[775-1274],[1274-1962],[1962-4055],[4055-770155]"
plot_speed "twitter" "screen" 4 10 "[1-20],[20-34],[34-89],[89-215],[215-416],[416-775],[775-1274],[1274-1962],[1962-4055],[4055-770155]"
plot_speed "twitter" "screen" 5 5 "[1-20],[20-34],[34-89],[89-215],[215-416],[416-775],[775-1274],[1274-1962],[1962-4055],[4055-770155]"
plot_speed "twitter" "screen" 5 10 "[1-20],[20-34],[34-89],[89-215],[215-416],[416-775],[775-1274],[1274-1962],[1962-4055],[4055-770155]"