#!/bin/sh

echo "Plotting degree distribution for Facebook ..."
python scripts/plot.py \
    --xlabel "Node degree" --ylabel "Frequency" \
    -i datasets/clean/fb/degree-cdf.stats.gz \
    -o results/fb-degree.pdf

echo "Plotting in-degree distribution for Twitter ..."
python scripts/plot.py \
    --xlabel "Node degree" --ylabel "Frequency" \
    -i datasets/clean/twitter/in-degree-cdf.stats.gz \
    -o results/twitter-in-degree.pdf

echo "Plotting out-degree distribution for Twitter ..."
python scripts/plot.py \
    --xlabel "Node degree" --ylabel "Frequency" \
    -i datasets/clean/twitter/out-degree-cdf.stats.gz \
    -o results/twitter-out-degree.pdf

