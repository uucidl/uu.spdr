#!/usr/bin/env sh
HERE="$(dirname ${0})"

# run this script before every commit

"${HERE}"/scripts/format-sources.sh "${HERE}"
