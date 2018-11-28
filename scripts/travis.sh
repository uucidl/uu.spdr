#!/usr/bin/env bash

USAGE="${0} --top <top of the tree> --os-name <os-name>"

while [ "$#" -gt 0 ]; do
    case $1 in
        --top)
            TOP=${2:?"os name required"}
            shift
            shift
            ;;
        --os-name)
            OS_NAME=${2:?"os name required"}
            shift
            shift
            ;;
        *)
            printf -- "Unknown option %s\n" "$1"
            printf -- "Usage: %s\n" "${USAGE}"
            exit 1
            ;;
    esac
done

TOP=${TOP:?${USAGE}}
OS_NAME=${OS_NAME:?${USAGE}}

set -o errexit
case "${OS_NAME}" in
    linux) bash "${TOP}"/build_examples_posix.sh ;;
    osx) bash "${TOP}"/build_examples_osx.sh ;;
    *)
        printf -- "Unknown os name %s\n" "${OS_NAME}"
        exit 1
        ;;
esac

bash "${TOP}"/build_tests.sh

# just to print the timing information about builds
origin="$(date "+%s")"
printf "%s\n" "${origin}"
PS4='T $(($(date "+%s") - ${origin}))\011'
set -x

## <RUN EXAMPLES..

export LSAN_OPTIONS=verbosity=1:log_threads=1

find "${TOP}"/output/programs -type f | while read program
do
    printf "Running %s:\n" "${program}"
    "${program}"
done

## ..RUN EXAMPES>

set +x
