#!/usr/bin/env bash

USAGE="${0} <top of the tree>"
TOP=${1:?${USAGE}}

date=$(date "+%Y/%m/%d")
features=$(git -C "${TOP}" grep 'FEATURE' -- src/* | wc -l)
todos=$(git -C "${TOP}" grep 'TODO' -- src/* | wc -l)
srclines=$(wc -l "${TOP}"/src/*.* | awk '/[0-9]+ total/ { print $1 }')

printf "%s Balances\n    Assets:FeatureCount  %d FEATURE\n    Liabilities:Code  %d Line\n" "${date}" "${features}" "${srclines}"
printf "    Liabilities:Todos  %d Todo\n" "${todos}"
