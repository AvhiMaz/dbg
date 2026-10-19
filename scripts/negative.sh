#!/usr/bin/env bash

set -u

ROOT=$(cd "$(dirname "$0")/.." && pwd)
DBG="$ROOT/dbg"
BASE="$ROOT/elf/vault.so"

export UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1

TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

TEXT_OFF=$(readelf -S -W "$BASE" | sed -nE 's/^  \[ *[0-9]+\] \.text +[A-Z]+ +[0-9a-f]+ ([0-9a-f]+) .*/\1/p')
TEXT_OFF=$((16#$TEXT_OFF))
E_FLAGS_OFF=48

failed=0

patch() {
    printf "$3" | dd of="$1" bs=1 seek="$2" conv=notrunc 2>/dev/null
}

expect() {
    local name=$1 want_rc=$2 want_text=$3 out rc
    out=$(timeout 10 "$DBG" -p "$TMP/bad.so" 2>&1)
    rc=$?
    if [ "$rc" -eq "$want_rc" ] && grep -qF -- "$want_text" <<< "$out"; then
        echo "ok   $name"
    else
        echo "FAIL $name  (exit $rc, wanted $want_rc and \"$want_text\")"
        grep -m3 -iE "error|unknown" <<< "$out" | sed 's/^/       /'
        failed=1
    fi
}

cp "$BASE" "$TMP/bad.so"
patch "$TMP/bad.so" $E_FLAGS_OFF '\x02\x00\x00\x00'
expect "e_flags = 2 (sBPF v2) is rejected" 1 "sBPF v2 is not supported yet"

cp "$BASE" "$TMP/bad.so"
patch "$TMP/bad.so" $E_FLAGS_OFF '\x20\x00\x00\x00'
expect "e_flags = 0x20 (old experimental v2) is reported as unknown" 1 "unknown sBPF version (e_flags = 0x20)"

cp "$BASE" "$TMP/bad.so"
patch "$TMP/bad.so" $E_FLAGS_OFF '\x01\x00\x00\x00'
expect "e_flags = 1 (sBPF v1) is accepted" 0 "jlt     r3, 3, +0x1a"

cp "$BASE" "$TMP/bad.so"
patch "$TMP/bad.so" $TEXT_OFF '\x36'
expect "invalid opcode 0x36 prints its byte" 0 "unknown opcode=0x36"

exit $failed
