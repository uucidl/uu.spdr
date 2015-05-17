#!/usr/bin/env bash

USAGE="${0} [--help|--only-matches] <top of the tree>    { prints implemented features }"

ONLY_MATCHES=0

while [ "$#" -gt 0 ]; do
    case $1 in
        --only-matches)
            ONLY_MATCHES=1
            shift
            ;;
        --help|-h)
            printf -- "Usage: %s\n" "${USAGE}"
            printf -- "\t--help: this help\n"
            printf -- "\t--only-matches: print only the feature text\n"
            exit 1
            shift
            ;;
        *)
            break
            ;;
    esac
done

HERE="$(dirname "${0}")"
TOP=${1:?${USAGE}}
ATOP=$(cd ${TOP} && pwd)

source "${HERE}"/lib/pipelines.sh

git --no-pager -C "${TOP}" grep --full-name -n 'FEATURE' -- src/* | \
    prepend_top_grep_pipeline "${TOP}" | remove_leading_spaces_grep_pipeline | onlymatches_pipeline "${ONLY_MATCHES}" "FEATURE(.*"
