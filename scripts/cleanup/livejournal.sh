#!/bin/sh

CURRDIR="$(dirname "$0")"
source $CURRDIR/../utils.sh

RAWDATASETSDIR=${1:-datasets/raw}
DATASETDIR=${2:-datasets/clean/livejournal}
ATTRIBUTES=("name")

ensure_paths $DATASETDIR ATTRIBUTES[@]

echo "Sorting graph ..."
tail -n +3 $RAWDATASETSDIR/soc-LiveJournal1/out.soc-LiveJournal1 | \
    awk '{print $1 "\t" $2}' | \
    sort -k1n -k2n | gzip -c > $DATASETDIR/graph.tsv.gz

echo "Extracting name for each node ..."
sed -e 's/|/\t/g' $RAWDATASETSDIR/soc-LiveJournal1/name2id | \
    tr '[:upper:]' '[:lower:]' | \
    gzip -c > $DATASETDIR/name/attrs.gz

echo "Filling missing attributes ..."
unpigz -c $DATASETDIR/graph.tsv.gz | python scripts/fill_missing.py --attribute-file $DATASETDIR/name/attrs.gz | gzip -c > $DATASETDIR/name/attrs-new.gz
mv $DATASETDIR/name/attrs-new.gz $DATASETDIR/name/attrs.gz

collect_statistics $DATASETDIR

generate_attribute_graph $DATASETDIR "name"
compact_graph $DATASETDIR ATTRIBUTES[@]

sample_queries $DATASETDIR ATTRIBUTES[@]
