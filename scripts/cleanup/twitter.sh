#!/bin/sh

CURRDIR="$(dirname "$0")"
source $CURRDIR/../utils.sh

RAWDATASETSDIR=${1:-datasets/raw}
DATASETDIR=${2:-datasets/clean/twitter}
ATTRIBUTES=("screen")

ensure_paths $DATASETDIR ATTRIBUTES[@]

echo "Inverting graph ..."
cat $RAWDATASETSDIR/Twitter/twitter_rv.net | \
    awk '{print $2 "\t" $1}' | \
    sort -k1n -k2n -S 20G --parallel=8 --compress-program=gzip | \
    gzip -c > $DATASETDIR/graph.tsv.gz

echo "Extracting screen names for each node ..."
cat $RAWDATASETSDIR/Twitter/numeric2screen | \
    grep -v -P "[\x80-\xFF]" | \
    awk '{print $1 "\t" $2}' | \
    tr '[:upper:]' '[:lower:]' | \
    gzip -c > $DATASETDIR/screen/attrs.gz

echo "Filling missing attributes ..."
unpigz -c $DATASETDIR/graph.tsv.gz | python scripts/fill_missing.py --attribute-file $DATASETDIR/screen/attrs.gz | gzip -c > $DATASETDIR/screen/attrs-new.gz
mv $DATASETDIR/screen/attrs-new.gz $DATASETDIR/screen/attrs.gz

collect_statistics $DATASETDIR

generate_attribute_graph $DATASETDIR "screen"
compact_graph $DATASETDIR ATTRIBUTES[@]

sample_queries $DATASETDIR ATTRIBUTES[@]
