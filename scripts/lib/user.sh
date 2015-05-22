function die() {
    msg=$1
    printf "%s\n" "$msg" >&2
    exit 1
}
