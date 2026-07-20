#!/usr/bin/env bash

# Codex C++ 教材的唯讀結構/內容守衛。
# 目的不是取代編譯測試，而是防止 1:1 映射、教材段落與 summary 深度日後漂移。

set -u

SCRIPT_DIR=$(CDPATH='' cd -- "$(dirname -- "$0")" && pwd -P)
CODEX_ROOT=$(CDPATH='' cd -- "$SCRIPT_DIR/.." && pwd -P)
REPO_ROOT=$(CDPATH='' cd -- "$CODEX_ROOT/.." && pwd -P)
CLAUDE_ROOT="$REPO_ROOT/Claude Code C++by Examples"
MODE=audit

usage() {
    cat <<'EOF'
用法：
  audit_textbook.sh
  audit_textbook.sh --dry-run
  audit_textbook.sh --help

唯讀檢查：
  1. Claude/Codex 的 .cpp 相對路徑及其教材目錄集合是否 1:1（動態取得）
  2. Codex 是否有多餘 .cpp、ELF、或與 Claude 逐位元組相同的 .cpp
  3. 每個 .cpp 是否有 main、LeetCode、實務與足夠的繁中教材內容
  4. 每個 summary.cpp 是否達面試速查深度並涵蓋必要段落

退出碼：0=全部通過；1=至少一項教材稽核失敗；2=用法/環境錯誤。
EOF
}

case "${1:-}" in
    '') ;;
    --dry-run) MODE=dry-run ;;
    --help|-h) usage; exit 0 ;;
    *) printf '錯誤：未知參數：%s\n' "$1" >&2; usage >&2; exit 2 ;;
esac
[ "$#" -le 1 ] || { printf '錯誤：參數過多。\n' >&2; exit 2; }

[ -d "$CLAUDE_ROOT" ] || {
    printf '錯誤：找不到 Claude 對照目錄：%s\n' "$CLAUDE_ROOT" >&2
    exit 2
}

if [ "$MODE" = dry-run ]; then
    printf '將唯讀比較：%s\n' "$CLAUDE_ROOT"
    printf '將唯讀稽核：%s\n' "$CODEX_ROOT"
    printf '不編譯、不執行、不修改教材。\n'
    exit 0
fi

for command_name in find sort comm sha256sum wc grep file mktemp sed cut; do
    command -v "$command_name" >/dev/null 2>&1 || {
        printf '錯誤：缺少必要命令：%s\n' "$command_name" >&2
        exit 2
    }
done

TMP_DIR=$(mktemp -d "${TMPDIR:-/tmp}/codex-cpp-audit.XXXXXX") || exit 2
trap 'rm -rf -- "$TMP_DIR"' EXIT HUP INT TERM

find "$CLAUDE_ROOT" -type f -name '*.cpp' -printf '%P\n' | sort > "$TMP_DIR/claude.txt"
find "$CODEX_ROOT" -type f -name '*.cpp' -printf '%P\n' | sort > "$TMP_DIR/codex.txt"
comm -23 "$TMP_DIR/claude.txt" "$TMP_DIR/codex.txt" > "$TMP_DIR/missing.txt"
comm -13 "$TMP_DIR/claude.txt" "$TMP_DIR/codex.txt" > "$TMP_DIR/extra.txt"
sed 's#/[^/]*$##' "$TMP_DIR/claude.txt" | sort -u > "$TMP_DIR/claude_dirs.txt"
sed 's#/[^/]*$##' "$TMP_DIR/codex.txt" | sort -u > "$TMP_DIR/codex_dirs.txt"
comm -23 "$TMP_DIR/claude_dirs.txt" "$TMP_DIR/codex_dirs.txt" > "$TMP_DIR/missing_dirs.txt"
comm -13 "$TMP_DIR/claude_dirs.txt" "$TMP_DIR/codex_dirs.txt" > "$TMP_DIR/extra_dirs.txt"

claude_count=$(wc -l < "$TMP_DIR/claude.txt")
codex_count=$(wc -l < "$TMP_DIR/codex.txt")
failed=0

printf '映射：Claude=%d，Codex=%d\n' "$claude_count" "$codex_count"
if [ -s "$TMP_DIR/missing.txt" ]; then
    printf '\n[FAIL] Codex 缺少以下 .cpp：\n' >&2
    sed 's/^/  - /' "$TMP_DIR/missing.txt" >&2
    failed=1
fi
if [ -s "$TMP_DIR/extra.txt" ]; then
    printf '\n[FAIL] Codex 多出以下非映射 .cpp：\n' >&2
    sed 's/^/  - /' "$TMP_DIR/extra.txt" >&2
    failed=1
fi
if [ -s "$TMP_DIR/missing_dirs.txt" ] || [ -s "$TMP_DIR/extra_dirs.txt" ]; then
    printf '\n[FAIL] 含 .cpp 的教材目錄集合不一致：\n' >&2
    sed 's/^/  Codex 缺少：/' "$TMP_DIR/missing_dirs.txt" >&2
    sed 's/^/  Codex 多出：/' "$TMP_DIR/extra_dirs.txt" >&2
    failed=1
fi

: > "$TMP_DIR/content_fail.txt"
: > "$TMP_DIR/identical.txt"
while IFS= read -r relative; do
    source="$CODEX_ROOT/$relative"
    [ -f "$source" ] || continue

    lines=$(wc -l < "$source")
    minimum=70
    minimum_chinese_lines=12
    case "$relative" in
        */summary.cpp)
            minimum=150
            minimum_chinese_lines=35
            ;;
    esac

    reasons=''
    [ "$lines" -ge "$minimum" ] || reasons="${reasons} lines<${minimum};"
    chinese_lines=$(grep -Pc '[\x{4e00}-\x{9fff}]' "$source" 2>/dev/null || true)
    [ "$chinese_lines" -ge "$minimum_chinese_lines" ] || \
        reasons="${reasons} zh-lines<${minimum_chinese_lines};"
    grep -Eq 'int[[:space:]]+main[[:space:]]*\(' "$source" || reasons="${reasons} no-main;"
    [ "$(grep -c 'LeetCode' "$source" || true)" -ge 1 ] || reasons="${reasons} missing-LeetCode;"
    [ "$(grep -c '實務' "$source" || true)" -ge 1 ] || reasons="${reasons} missing-practical;"
    grep -Eiq 'leetcode[_[:alnum:]]*[[:space:]]*\(' "$source" || reasons="${reasons} no-executable-LeetCode;"
    grep -Eiq 'practical[_[:alnum:]]*[[:space:]]*\(' "$source" || reasons="${reasons} no-executable-practical;"
    grep -q '陷阱\|易錯\|注意\|警告\|不可' "$source" || reasons="${reasons} no-pitfall;"
    grep -q '面試\|自問\|思考\|練習' "$source" || reasons="${reasons} no-interview;"

    case "$relative" in
        */summary.cpp)
            grep -q '複雜度' "$source" || reasons="${reasons} summary-no-complexity;"
            grep -q '選' "$source" || reasons="${reasons} summary-no-selection;"
            ;;
    esac

    [ -z "$reasons" ] || printf '%s\t%s\n' "$relative" "$reasons" >> "$TMP_DIR/content_fail.txt"

    peer="$CLAUDE_ROOT/$relative"
    if [ -f "$peer" ] && [ "$(sha256sum "$source" | cut -d' ' -f1)" = \
                           "$(sha256sum "$peer" | cut -d' ' -f1)" ]; then
        printf '%s\n' "$relative" >> "$TMP_DIR/identical.txt"
    fi
done < "$TMP_DIR/codex.txt"

if [ -s "$TMP_DIR/content_fail.txt" ]; then
    printf '\n[FAIL] 教材結構不足（檔案<TAB>原因）：\n' >&2
    sed 's/^/  /' "$TMP_DIR/content_fail.txt" >&2
    failed=1
fi
if [ -s "$TMP_DIR/identical.txt" ]; then
    printf '\n[FAIL] 下列檔案與 Claude 逐位元組相同，違反獨立撰寫：\n' >&2
    sed 's/^/  - /' "$TMP_DIR/identical.txt" >&2
    failed=1
fi

if find "$CODEX_ROOT" -type f -exec file --brief --mime-type {} + 2>/dev/null |
    grep -q 'application/x-.*executable'; then
    printf '\n[FAIL] Codex 教材樹內偵測到 executable 類型檔案。\n' >&2
    failed=1
fi

if [ "$failed" -ne 0 ]; then
    printf '\n教材稽核：FAIL\n' >&2
    exit 1
fi

directory_count=$(wc -l < "$TMP_DIR/codex_dirs.txt")
printf '教材稽核：PASS（%d/%d 路徑、%d 個教材目錄一對一，內容守衛全部通過）\n' \
    "$codex_count" "$claude_count" "$directory_count"
