#!/usr/bin/env bash

##
# NOTE(nicolas): SPDR does not need to be built outside of your
# project.
#
# You only need two things to use SPDR:
# - import the `./include` directory
# - add one of its `_unit.c` source files to your project
# - include "spdr/spdr.h" (C) or "spdr/spdr.hh" (C++)
set -u

RELH="$(dirname "${0}")"
HERE="$(cd "${RELH}" && pwd)"

OUTPUT="${HERE}"/output
CXX=${CXX:-$(which "c++")}
CC=${CC:-$(which "cc")}
CFLAGS=${CFLAGS:-}

cflags=(${CFLAGS} -Wall -Wextra)
cflags=(${cflags[@]})

[ -d "${OUTPUT}" ] || mkdir -p "${OUTPUT}"
printf "INFO building into %s\n" "${OUTPUT}"

# just to print the timing information about builds
origin="$(date "+%s")"
printf "%s\n" "${origin}"
PS4="T $(($(date "+%s") - ${origin}))\011"
set -x

## <BUILD EXAMPLES..
set -o errexit

EXAMPLES="${HERE}"/examples

# here is all you need:
SPDR=(-I"${HERE}"/include "${HERE}"/src/spdr_posix_unit.c -lrt)

"${CC}" "${cflags[@]}" -ansi "${EXAMPLES}"/test.c -lm -o "${OUTPUT}"/test "${SPDR[@]}"

"${CC}" "${cflags[@]}" "${EXAMPLES}"/test-scope.c -o "${OUTPUT}"/test-scope  "${SPDR[@]}"

"${CC}" "${cflags[@]}" -ansi "${EXAMPLES}"/test-mt.c -o "${OUTPUT}"/test-mt "${SPDR[@]}"

"${CC}" "${cflags[@]}" -ansi "${EXAMPLES}"/test-full.c -o "${OUTPUT}"/test-full "${SPDR[@]}"

"${CXX}" "${cflags[@]}" "${EXAMPLES}"/test-cxx.cc -o "${OUTPUT}"/test-cxx "${SPDR[@]}"

## ..BUILD EXAMPLES>

printf "\a"
set +x
