#!/usr/bin/env bash

# 依實際 *.cpp 重建每個主題 README 的範例索引；保留前方教材與後方練習。

set -u

SCRIPT_DIR=$(CDPATH='' cd -- "$(dirname -- "$0")" && pwd -P)
ROOT_DIR=$(CDPATH='' cd -- "$SCRIPT_DIR/.." && pwd -P)
MODE=apply

usage() {
    cat <<'EOF'
用法：
  update_readme_indexes.sh             重建所有主題 README 的 .cpp 索引
  update_readme_indexes.sh --check     只檢查索引是否需要更新
  update_readme_indexes.sh --dry-run   列出各章檔案數，不寫檔
  update_readme_indexes.sh --help

只替換從「## 範例」到下一個「## 練習」之前的區段；其他教材內容原樣保留。
退出碼：0=成功/已同步；1=--check 發現漂移；2=用法或 README 結構錯誤。
EOF
}

case "${1:-}" in
    '') ;;
    --check) MODE=check ;;
    --dry-run) MODE=dry-run ;;
    --help|-h) usage; exit 0 ;;
    *) printf '錯誤：未知參數：%s\n' "$1" >&2; usage >&2; exit 2 ;;
esac
[ "$#" -le 1 ] || { printf '錯誤：參數過多。\n' >&2; exit 2; }

TMP_DIR=$(mktemp -d "${TMPDIR:-/tmp}/codex-cpp-readme.XXXXXX") || exit 2
trap 'rm -rf -- "$TMP_DIR"' EXIT HUP INT TERM

changed=0
topics=0
for topic_dir in "$ROOT_DIR"/C++_*; do
    [ -d "$topic_dir" ] || continue
    readme="$topic_dir/README.md"
    [ -f "$readme" ] || {
        printf '錯誤：缺少 README：%s\n' "$readme" >&2
        exit 2
    }

    start=$(grep -n -m1 '^## 範例' "$readme" | cut -d: -f1)
    exercise=$(grep -n -m1 '^## 練習' "$readme" | cut -d: -f1)
    if [ -z "$start" ] || [ -z "$exercise" ] || [ "$start" -ge "$exercise" ]; then
        printf '錯誤：README 缺少合法的「## 範例...## 練習」區段：%s\n' "$readme" >&2
        exit 2
    fi

    mapfile -t sources < <(find "$topic_dir" -type f -name '*.cpp' -printf '%P\n' | sort)
    output="$TMP_DIR/$(basename "$topic_dir").md"
    {
        sed -n "1,$((start - 1))p" "$readme"
        printf '## 範例索引\n\n'
        printf '本章共 %d 個可獨立編譯執行的 `.cpp`；`summary.cpp` 是面試前速查。\n\n' \
            "${#sources[@]}"
        printf '<details>\n<summary>展開全部檔案</summary>\n\n'
        for relative in "${sources[@]}"; do
            printf -- '- [`%s`](%s)\n' "$relative" "$relative"
        done
        printf '\n</details>\n\n'
        sed -n "${exercise},\$p" "$readme"
    } > "$output"

    topics=$((topics + 1))
    if cmp -s "$readme" "$output"; then
        printf '[同步] %s（%d）\n' "$(basename "$topic_dir")" "${#sources[@]}"
        continue
    fi

    changed=$((changed + 1))
    case "$MODE" in
        apply)
            mv -- "$output" "$readme"
            printf '[更新] %s（%d）\n' "$(basename "$topic_dir")" "${#sources[@]}"
            ;;
        check)
            printf '[漂移] %s（%d）\n' "$(basename "$topic_dir")" "${#sources[@]}"
            ;;
        dry-run)
            printf '[預覽] %s（%d）\n' "$(basename "$topic_dir")" "${#sources[@]}"
            ;;
    esac
done

printf '主題=%d，需要更新=%d，模式=%s\n' "$topics" "$changed" "$MODE"
[ "$MODE" != check ] || [ "$changed" -eq 0 ]
