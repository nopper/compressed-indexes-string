#!/bin/sh

CURRDIR="$(dirname "$0")"
source $CURRDIR/../utils.sh

RAWDATASETSDIR=${1:-datasets/raw}
DATASETDIR=${2:-datasets/clean/dblp}
ATTRIBUTES=("fname")

ensure_paths $DATASETDIR ATTRIBUTES[@]

echo "Sorting graph ..."
cat $RAWDATASETSDIR/DBLPnew/dblp-17-Oct-2014.edges | \
    sort -k1n -k2n | gzip -c > $DATASETDIR/graph.tsv.gz

echo "Extracting first names for each node ..."
cat $RAWDATASETSDIR/DBLPnew/dblp-17-Oct-2014.nodes | \
    awk -F'\t' '{print $2 "\t" $1}' | \
    tr '[:upper:]' '[:lower:]' | \
    python scripts/filter.py --graph=$DATASETDIR/graph.tsv.gz | \
    gzip -c > $DATASETDIR/fname/attrs.gz

collect_statistics $DATASETDIR

generate_attribute_graph $DATASETDIR "fname"
compact_graph $DATASETDIR ATTRIBUTES[@]

sample_queries $DATASETDIR ATTRIBUTES[@]
