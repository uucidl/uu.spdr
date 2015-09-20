#!/usr/bin/env bash

##
# NOTE(nicolas): SPDR does not need to be built outside of your
# project.
#
# You only need two things to use SPDR:
# - import the `./include` directory
# - add one of its `_unit.c` source files to your project
# - include "spdr/spdr.h" (C) or "spdr/spdr.hh" (C++)

RELH="$(dirname "${0}")"
HERE="$(cd "${RELH}" && pwd)"

OUTPUT="${HERE}"/output
CXX=${CXX:-$(which "c++")}
CC=${CC:-$(which "cc")}
CFLAGS=${CFLAGS:-}

cflags=(${CFLAGS} -Wall -Wextra -ansi)
cflags=(${cflags[@]})

[ -d "${OUTPUT}" ] || mkdir -p "${OUTPUT}"
printf "INFO building into %s\n" "${OUTPUT}"

# just to print the timing information about builds
origin="$(date "+%s")"
printf "%s\n" "${origin}"
PS4="T $(($(date "+%s") - ${origin}))\011"
set -x

## <BUILD EXAMPLES..

EXAMPLES="${HERE}"/examples

# here is all you need:
SPDR=(-I"${HERE}"/include "${HERE}"/src/spdr_posix_unit.c -lrt)

"${CC}" "${cflags[@]}" "${EXAMPLES}"/test.c -lm "${SPDR[@]}" -o "${OUTPUT}"/test

exit 0

"${CC}" "${cflags[@]}" "${EXAMPLES}"/test-scope.c "${HERE}"/src/spdr_posix_unit.c -o "${OUTPUT}"/test-scope

"${CC}" "${cflags[@]}" "${EXAMPLES}"/test-mt.c "${HERE}"/src/spdr_posix_unit.c -o "${OUTPUT}"/test-mt

"${CC}" "${cflags[@]}" "${EXAMPLES}"/test-full.c "${HERE}"/src/spdr_posix_unit.c -o "${OUTPUT}"/test-full

"${CXX}" "${cflags[@]}" "${EXAMPLES}"/test-cxx.cc "${HERE}"/src/spdr_posix_unit.c -o "${OUTPUT}"/test-cxx

## ..BUILD EXAMPLES>

printf "\a"
set +x
