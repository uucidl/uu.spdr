#!/usr/bin/env bash

set -u

RELH="$(dirname "${0}")"
HERE="$(cd "${RELH}" && pwd)"

OUTPUT="${HERE}"/output
CC=${CC:-$(which "cc")}
CFLAGS=${CFLAGS:-}

cflags=(${CFLAGS} -Wall -Wextra -O3)
cflags=(${cflags[@]})

[ -d "${OUTPUT}" ] || mkdir -p "${OUTPUT}"
printf "INFO building into %s\n" "${OUTPUT}"

# just to print the timing information about builds
origin="$(date "+%s")"
printf "%s\n" "${origin}"
PS4='T $(($(date "+%s") - ${origin}))\011'
set -x

## <BUILD EXAMPLES..
set -o errexit

TESTS="${HERE}"/tests

D="${OUTPUT}"/spdr_basic_tests
"${CC}" -std=c99 "${cflags[@]}" -DTRACING_ENABLED=1 "${TESTS}"/spdr_basic_tests.c -lm \
        -o "${D}"
printf "PROGRAM\t%s\n" "${D}"

## ..BUILD TESTS>

set +x
