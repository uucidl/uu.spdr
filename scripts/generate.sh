#!/usr/bin/env bash
HERE="$(dirname ${0})"
source "${HERE}"/lib/user.sh

# generates source code

USAGE="${0} <top of the tree>"
TOP=${1:?${USAGE}}

RAGEL=${RAGEL:-$(which ragel)}
[ -x "${RAGEL}" ] || die "missing ragel or RAGEL env variable"

set -e
"${RAGEL}" "${TOP}"/src/mailbox_parser.rl.c -T1 -o "${TOP}"/src-generated/mailbox_parser.c
