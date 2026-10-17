#!/usr/bin/env bash

set -u

ROOT=$(cd "$(dirname "$0")/.." && pwd)
DBG="$ROOT/dbg"
AWK="gawk -M"

PRELOADED_SO_FILES=(
    "../elf/amm.so"
    "../elf/vault.so"
    "../elf/multisig.so"
)

CUSTOM_SO_FILES=(
    ""
    ""
    ""
)

VERBOSE=0
FILES=()
for arg in "$@"; do
    if [ "$arg" = "-v" ]; then
        VERBOSE=1
    else
        FILES+=("$arg")
    fi
done
if [ ${#FILES[@]} -eq 0 ]; then
    SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
    for so in "${PRELOADED_SO_FILES[@]}" "${CUSTOM_SO_FILES[@]}"; do
        case "$so" in
            "")    ;;
            /*)    FILES+=("$so") ;;
            "~/"*) FILES+=("$HOME/${so#"~/"}") ;;
            *)     FILES+=("$SCRIPT_DIR/$so") ;;
        esac
    done
fi

if [ ! -x "$DBG" ]; then
    echo "error: $DBG not found; run 'make build' first" >&2
    exit 2
fi

OBJDUMP=${OBJDUMP:-$(ls ~/.cache/solana/*/platform-tools/llvm/bin/llvm-objdump 2>/dev/null | sort | tail -1)}
if [ -z "$OBJDUMP" ] || [ ! -x "$OBJDUMP" ]; then
    echo "error: Solana's llvm-objdump not found; set OBJDUMP=/path" >&2
    exit 2
fi

TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

header_dbg() {
    $AWK '/^[a-z]+: +[0-9a-fx]+$/ && $1 != "symbols:" {
        sub(":", "", $1); print $1, strtonum($2)
    }' "$1" | sort
}

header_readelf() {
    readelf -h "$1" | $AWK -F': +' '
        $1 ~ /Type/                        { split($2, t, " "); print "type", (t[1]=="DYN" ? 3 : t[1]=="EXEC" ? 2 : t[1]=="REL" ? 1 : -1) }
        $1 ~ /Machine/                     { if (match($0, /0x[0-9a-f]+/)) print "machine", strtonum(substr($0, RSTART, RLENGTH)); else print "machine", ($0 ~ /Solana/ ? 263 : -1) }
        $1 ~ /^  Version/ && $2 ~ /^0x/    { print "version", strtonum($2) }
        $1 ~ /Entry point/                 { print "entry", strtonum($2) }
        $1 ~ /Start of program headers/    { print "phoff", strtonum($2) }
        $1 ~ /Start of section headers/    { print "shoff", strtonum($2) }
        $1 ~ /Flags/                       { print "flags", strtonum($2) }
        $1 ~ /Size of this header/         { print "ehsize", strtonum($2) }
        $1 ~ /Size of program headers/     { print "phentsize", strtonum($2) }
        $1 ~ /Number of program headers/   { print "phnum", strtonum($2) }
        $1 ~ /Size of section headers/     { print "shentsize", strtonum($2) }
        $1 ~ /Number of section headers/   { print "shnum", strtonum($2) }
        $1 ~ /string table index/          { print "shstrndx", strtonum($2) }
    ' | sort
}

sections_dbg() {
    $AWK 'match($0, /^\[ *([0-9]+)\] +([^ ]*) +type=([0-9]+) offset=([^ ]+) size=([^ ]+)$/, m) {
        print m[1], (m[2] == "" ? "-" : m[2]), m[3], strtonum(m[4]), strtonum(m[5])
    }' "$1"
}

sections_readelf() {
    readelf -S -W "$1" | $AWK '
        BEGIN { t["NULL"]=0; t["PROGBITS"]=1; t["SYMTAB"]=2; t["STRTAB"]=3; t["RELA"]=4;
                t["HASH"]=5; t["DYNAMIC"]=6; t["NOTE"]=7; t["NOBITS"]=8; t["REL"]=9; t["DYNSYM"]=11 }
        match($0, /^  \[ *([0-9]+)\] (.*)$/, m) {
            n = split(m[2], f, " ")
            if (f[1] == "NULL") { print m[1], "-", 0, 0, 0; next }
            print m[1], f[1], (f[2] in t ? t[f[2]] : -1), strtonum("0x" f[4]), strtonum("0x" f[5])
        }'
}

symcount_dbg()     { $AWK '/^symbols: / { print $2 }' "$1"; }
symcount_readelf() { readelf -s -W "$1" | $AWK '/^Symbol table .\.symtab. contains/ { print $5 }'; }

funcs_dbg() {
    $AWK '/^0x[0-9a-f]+ +[0-9]+ [^ ]+$/ { print strtonum($1), $2, $3 }' "$1" | sort -u
}

funcs_readelf() {
    readelf -s -W "$1" | $AWK '
        /^Symbol table/ { in_symtab = ($0 ~ /\.symtab/); next }
        in_symtab && $4 == "FUNC" { print strtonum("0x" $2), $3, $8 }' | sort -u
}

NORM_AWK='
function norm(s,    out, tok, v) {
    gsub(/<[^>]*>/, "", s); gsub(/ /, "", s)
    out = ""
    while (match(s, /[+-]?(0x[0-9a-f]+|[0-9]+)/)) {
        tok = substr(s, RSTART, RLENGTH)
        # only a number if not glued to a register name like "r10"
        if (RSTART > 1 && substr(s, RSTART - 1, 1) ~ /[a-z0-9_]/) {
            out = out substr(s, 1, RSTART + RLENGTH - 1)
        } else {
            v = strtonum(gensub(/^[+-]/, "", 1, tok))
            out = out substr(s, 1, RSTART - 1) (tok ~ /^-/ ? "-" : tok ~ /^\+/ ? "+" : "") v
        }
        s = substr(s, RSTART + RLENGTH)
    }
    return out s
}'

insns_dbg() {
    $AWK "$NORM_AWK"'
        match($0, /^ *([0-9a-f]+): ((([0-9a-f]{2}) ){8}) ([^ ]+) *(.*)$/, m) {
            split(m[2], b, " ")
            printf "%d %d 1 %s %s\t%s %s\n", strtonum("0x" m[1]), strtonum("0x" b[1]), m[5], norm(m[6]), m[5], m[6]
        }' "$1" | sort -n
}

insns_objdump() {
    "$OBJDUMP" -d "$1" | $AWK "$NORM_AWK"'
        match($0, /^ +([0-9a-f]+):[ \t]+((([0-9a-f]{2}) ){8,16})[ \t]*\t([^ \t]+)[ \t]*(.*)$/, m) {
            n = split(m[2], b, " ")
            text = m[6]; gsub(/ *<[^>]*>/, "", text)
            printf "%d %d %d %s %s\t%s %s\n", strtonum("0x" m[1]), strtonum("0x" b[1]), n / 8, m[5], norm(m[6]), m[5], text
        }' | sort -n
}

check_file() {
    local so=$1 out=$TMP/dbg.out problems=$TMP/problems
    : > "$problems"

    if ! timeout 30 "$DBG" "$so" > "$out" 2> "$TMP/dbg.err"; then
        echo "FAIL ${so#"$ROOT"/}  (dbg: $(head -1 "$TMP/dbg.err"))"
        return 1
    fi

    if ! diff <(header_dbg "$out") <(header_readelf "$so") > "$TMP/d"; then
        sed 's/^</  header dbg:    /; s/^>/  header readelf:/; /^[0-9]/d; /^---/d' "$TMP/d" >> "$problems"
    fi

    if ! diff <(sections_dbg "$out") <(sections_readelf "$so") > "$TMP/d"; then
        sed 's/^</  section dbg:    /; s/^>/  section readelf:/; /^[0-9]/d; /^---/d' "$TMP/d" >> "$problems"
    fi

    local ours theirs
    ours=$(symcount_dbg "$out")
    theirs=$(symcount_readelf "$so")
    if [ -n "$theirs" ] && [ "$ours" != "$theirs" ]; then
        echo "  symbol count: dbg=$ours readelf=$theirs" >> "$problems"
    fi

    if ! diff <(funcs_dbg "$out") <(funcs_readelf "$so") > "$TMP/d"; then
        sed 's/^</  function only in dbg:    /; s/^>/  function only in readelf:/; /^[0-9]/d; /^---/d' "$TMP/d" >> "$problems"
    fi

    insns_dbg "$out" > "$TMP/ours"
    insns_objdump "$so" > "$TMP/theirs"

    $AWK -v verbose="$VERBOSE" '
        function kind(opc) {
            if (opc == 5) return "ja";      if (opc == 24) return "lddw"
            if (opc == 133) return "call";  if (opc == 141) return "callx"
            if (opc == 149) return "exit"
            if (opc == 132 || opc == 135) return "neg"
            if (opc == 212) return "le";    if (opc == 220) return "be"
            split("LD LDX ST STX ALU32 JMP JMP32 ALU64", c, " ")
            return c[opc % 8 + 1]
        }
        BEGIN { FS = "\t"; PROCINFO["sorted_in"] = "@ind_num_asc" }
        FNR == NR { split($1, f, " "); ours[f[1]] = $1; text[f[1]] = $2; next }   # first file: ours
        {
            split($1, t, " ")
            addr = t[1]; k = kind(t[2]); seen[addr] = 1
            if (t[3] == 2) second[addr + 8] = 1   # lddw covers the next slot
            if (!(addr in ours)) { bad[k]++; ex(k, sprintf("%x: missing in dbg | objdump: %s", addr, $2)); next }
            split(ours[addr], o, " ")
            on = o[4]; oo = (5 in o) ? o[5] : ""
            tn = t[4]; to = (5 in t) ? t[5] : ""
            if ((on == tn || (on == "call" && tn == "syscall")) && oo == to) good[k]++
            else { bad[k]++; ex(k, sprintf("%x: dbg \"%s\" | objdump \"%s\"", addr, text[addr], $2)) }
        }
        function ex(k, msg) { if (verbose || shown[k]++ < 2) examples[k] = examples[k] "      " msg "\n" }
        END {
            for (a in ours) if (!(a in seen)) {
                k = (a in second) ? "lddw 2nd slot printed" : "extra line"
                bad[k]++; ex(k, sprintf("%x: dbg prints \"%s\" as its own line", a, text[a]))
            }
            for (k in good) total += good[k]
            print total > "/dev/stderr"
            n = asorti(bad, keys)
            for (i = 1; i <= n; i++) {
                k = keys[i]
                printf "  disasm %s: %d wrong, %d ok\n%s", k, bad[k], good[k] + 0, examples[k]
            }
        }' "$TMP/ours" "$TMP/theirs" >> "$problems" 2> "$TMP/total"

    local name=${so#"$ROOT"/}
    local total
    total=$(cat "$TMP/total")
    if [ -s "$problems" ]; then
        echo "FAIL $name  ($total instructions match)"
        cat "$problems"
        return 1
    fi
    echo "ok   $name  (header, sections, symbols, $total instructions)"
}

failed=0
for so in "${FILES[@]}"; do
    if [ ! -f "$so" ]; then
        echo "FAIL $so  (file not found)"
        failed=1
        continue
    fi
    check_file "$(realpath "$so")" || failed=1
done
exit $failed
