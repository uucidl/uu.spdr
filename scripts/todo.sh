#!/usr/bin/env bash

USAGE="${0} <top of the tree>"
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
            printf -- "\t--only-matches: print only the todo text\n"
            exit 1
            shift
            ;;
        *)
            break
            ;;
    esac
done

TOP=${1:?${USAGE}}
ATOP=$(cd ${TOP} && pwd)
HERE="$(dirname "${0}")"

source "${HERE}"/lib/pipelines.sh

git --no-pager -C "${TOP}" grep --full-name -n -A 3 'TODO' -- src/* | \
    prepend_top_grep_pipeline "${TOP}" | remove_leading_spaces_grep_pipeline | onlymatches_pipeline "${ONLY_MATCHES}" "TODO(.*"
