#!/bin/sh
# A SessionStart hook: prints what the nearest docs tree is missing. It never fails a session, so it always exits 0.

line_limit=20

is_docs_folder_of_a_repo() {
    [ "${1##*/}" = docs ] && [ ! -e "$1/.git" ] && [ -f "${1%/*}/docs/VERSION" ]
}

is_tree_root() {
    [ -f "$1/docs/VERSION" ] && return 0
    is_docs_folder_of_a_repo "$1" && return 1
    [ -f "$1/VERSION" ] && grep -q '^tarn [0-9]' "$1/VERSION"
}

find_tree_root() {
    directory=$PWD
    while [ "$directory" != / ]; do
        if is_tree_root "$directory"; then
            printf '%s\n' "$directory"
            return 0
        fi
        directory=${directory%/*}
        [ -z "$directory" ] && directory=/
    done
    return 1
}

find_checker() {
    if [ -n "$TARN_CHECK" ]; then
        [ -x "$TARN_CHECK" ] && printf '%s\n' "$TARN_CHECK"
        return
    fi
    command -v tarn-check 2>/dev/null
}

print_findings() {
    findings=$("$1" --quick "$2" 2>&1 | grep -v ' snapshot-absent ')
    [ -z "$findings" ] && return
    printf '%s\n' "$findings" | head -n "$line_limit"
    total=$(printf '%s\n' "$findings" | wc -l | tr -d ' ')
    if [ "$total" -gt "$line_limit" ]; then
        printf 'tarn: %s more lines, run tarn-check %s for all of them\n' "$((total - line_limit))" "$2"
    fi
}

tree_root=$(find_tree_root) || exit 0
checker=$(find_checker)
if [ -z "$checker" ]; then
    echo "tarn: this tree follows the docs standard but tarn-check was not found; build it from intisy/tarn (cmake -S . -B build && cmake --build build) and put it on PATH or set TARN_CHECK"
    exit 0
fi
print_findings "$checker" "$tree_root"
exit 0
