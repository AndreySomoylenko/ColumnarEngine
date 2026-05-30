#!/usr/bin/env bash
set -euo pipefail

source /bench/env.sh

git clone --branch "$BRANCH" "$REPO_URL" repo
cd repo

./script/setup.sh
./script/build.sh
# ./script/convert.sh "$INPUT_CSV" "$COLUMNAR"

mkdir -p "$RESULTS"
echo "query,cold_ms,hot1_ms,hot2_ms" > "$RESULTS/result.csv"

for q in $(seq 0 42); do
    query_tmp="$(mktemp -d)"

    sync
    echo 3 > /proc/sys/vm/drop_caches

    ./script/run_query.sh "$q" "$COLUMNAR" \
        "$query_tmp/cold/query${q}.csv" \
        "$query_tmp/cold/query${q}.log"
    ./script/run_query.sh "$q" "$COLUMNAR" \
        "$query_tmp/hot1/query${q}.csv" \
        "$query_tmp/hot1/query${q}.log"
    ./script/run_query.sh "$q" "$COLUMNAR" \
        "$query_tmp/hot2/query${q}.csv" \
        "$query_tmp/hot2/query${q}.log"

    cold_ms="$(awk -F= '/^elapsed_ms=/{print $2}' "$query_tmp/cold/query${q}.log")"
    hot1_ms="$(awk -F= '/^elapsed_ms=/{print $2}' "$query_tmp/hot1/query${q}.log")"
    hot2_ms="$(awk -F= '/^elapsed_ms=/{print $2}' "$query_tmp/hot2/query${q}.log")"

    cp "$query_tmp/cold/query${q}.csv" "$RESULTS/query${q}.csv"
    {
        echo "--- cold ---"
        cat "$query_tmp/cold/query${q}.log"
        echo "--- hot1 ---"
        cat "$query_tmp/hot1/query${q}.log"
        echo "--- hot2 ---"
        cat "$query_tmp/hot2/query${q}.log"
    } > "$RESULTS/query${q}.log"

    echo "$q,$cold_ms,$hot1_ms,$hot2_ms" >> "$RESULTS/result.csv"
    rm -rf "$query_tmp"
done
