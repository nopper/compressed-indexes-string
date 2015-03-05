#!/bin/sh

MYTMPDIR=$(mktemp -d 2>/dev/null || mktemp -d -t 'mytmpdir')
ENCODINGS=(
    "block_varint_simple" "block_optpfor_simple" "block_interpolative_simple" "ef_simple" "single_simple" "uniform_simple" "opt_simple" "fixed_simple"
)
ENCODING=${ENCODINGS[$RANDOM % ${#ENCODINGS[@]} ]}

trap "rm -rf $MYTMPDIR" EXIT

echo "[+] Randomly selected $ENCODING as encoding scheme ..."

echo "[+] Generating ID mapping ..."
gunzip -c ./test_data/simple-names.tsv.gz | \
    sort -f -k2 | awk '{print $1}' | \
    gzip -c > "$MYTMPDIR"/id-mapping.tsv.gz

echo "[+] Creating the lexicographic version of graph ..."
gunzip -c ./test_data/simple-graph.tsv.gz | \
    ../reassign_ids --translation UserIdToSortId --input-ids "$MYTMPDIR"/id-mapping.tsv.gz | \
    sort -k1n -k2n | \
    gzip -c > "$MYTMPDIR"/graph-by-name.tsv.gz

echo "[+] Creating index k=1 ..."
gunzip -c "$MYTMPDIR"/graph-by-name.tsv.gz | \
    ../create_index -u 5 -t $ENCODING -o "$MYTMPDIR"/db1

echo "[+] Verifying index ..."
gunzip -c "$MYTMPDIR"/graph-by-name.tsv.gz | \
    ../verify_index -i "$MYTMPDIR"/db1 -t $ENCODING || { echo 'Verification failed' ; exit 1; }

echo "[+] Generating graph at k=2 ..."
gunzip -c "$MYTMPDIR"/graph-by-name.tsv.gz | \
    ../friends_at_k -i "$MYTMPDIR"/db1 -t $ENCODING | \
    gzip -c > "$MYTMPDIR"/graph-at-2.tsv.gz

echo "[+] Creating index k=2 ..."
gunzip -c "$MYTMPDIR"/graph-at-2.tsv.gz | \
    ../create_index -u 5 -t $ENCODING -o "$MYTMPDIR"/db2

echo "[+] Verifying index ..."
gunzip -c "$MYTMPDIR"/graph-at-2.tsv.gz | \
    ../verify_index -i "$MYTMPDIR"/db2 -t $ENCODING || { echo 'Verification failed' ; exit 1; }

echo "[+] Creating dictionary ..."
gunzip -c ./test_data/simple-names.tsv.gz | \
    sort -f -k2 | awk '{print $2}' | \
    ../create_dictionary --type permuterm -o "$MYTMPDIR"/d.permuterm

echo "[+] Generating queries ..."
echo "1" | \
    ../sample_queries -t $ENCODING -i "$MYTMPDIR"/db1 \
                    --str-mapping ./test_data/simple-names.tsv.gz \
                    --id-mapping "$MYTMPDIR"/id-mapping.tsv.gz > "$MYTMPDIR"/queries.txt

../index_stats -t $ENCODING -i "$MYTMPDIR"/db1
../index_stats -t $ENCODING -i "$MYTMPDIR"/db2

../simple_scheme --query-file "$MYTMPDIR"/queries.txt \
                --dict-type permuterm --dictionary "$MYTMPDIR"/d.permuterm \
                -t $ENCODING -i "$MYTMPDIR"/db1 \
                --id-mapping "$MYTMPDIR"/id-mapping.tsv.gz -n 3 \
                --scheme hopping || { echo 'Querying failed' ; exit 1; }
