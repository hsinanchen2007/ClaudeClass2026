#!/usr/bin/env bash

set -u

SCRIPT_DIR=$(CDPATH='' cd -- "$(dirname -- "$0")" && pwd -P)
ROOT_DIR=$(CDPATH='' cd -- "$SCRIPT_DIR/.." && pwd -P)
CXX_BIN=${CXX:-g++}
MODE=check

usage() {
    cat <<'EOF'
用法：
  check_all.sh [--run]
  check_all.sh --release
  check_all.sh --sanitize
  check_all.sh --dry-run
  check_all.sh --help

選項：
  --run       編譯後逐一執行，每個程式最多 10 秒
  --release   加上 -O2 -DNDEBUG 編譯並逐一執行，檢查必要操作未藏在 assert
  --sanitize  以 AddressSanitizer + UBSan 編譯並逐一執行
  --dry-run   只列出將執行的編譯命令
  --help      顯示本說明

環境變數：
  CXX         C++ 編譯器，預設 g++

退出碼：
  0  全部成功
  1  至少一個範例編譯或執行失敗
  2  用法或環境錯誤

所有輸出都放在 mktemp 建立的目錄；不會在 source tree 產生 executable。
所有 warnings 視為錯誤（-Werror），避免教材把可疑寫法當成正常範例。
EOF
}

while [ "$#" -gt 0 ]; do
    case "$1" in
        --run)
            [ "$MODE" = check ] || {
                printf '錯誤：--run 與其他模式不可並用。\n' >&2
                exit 2
            }
            MODE=run
            ;;
        --dry-run)
            [ "$MODE" = check ] || {
                printf '錯誤：--dry-run 與其他模式不可並用。\n' >&2
                exit 2
            }
            MODE=dry-run
            ;;
        --sanitize)
            [ "$MODE" = check ] || {
                printf '錯誤：--sanitize 與其他模式不可並用。\n' >&2
                exit 2
            }
            MODE=sanitize
            ;;
        --release)
            [ "$MODE" = check ] || {
                printf '錯誤：--release 與其他模式不可並用。\n' >&2
                exit 2
            }
            MODE=release
            ;;
        --help|-h)
            usage
            exit 0
            ;;
        *)
            printf '錯誤：未知參數：%s\n' "$1" >&2
            usage >&2
            exit 2
            ;;
    esac
    shift
done

for command_name in "$CXX_BIN" find sort mktemp timeout; do
    command -v "$command_name" >/dev/null 2>&1 || {
        printf '錯誤：找不到必要命令：%s\n' "$command_name" >&2
        exit 2
    }
done

standard_for() {
    case "$1" in
        "$ROOT_DIR/C++_Cpp11/"*) printf '%s\n' c++11 ;;
        "$ROOT_DIR/C++_Cpp14/"*) printf '%s\n' c++14 ;;
        "$ROOT_DIR/C++_Cpp17/"*) printf '%s\n' c++17 ;;
        *) printf '%s\n' c++20 ;;
    esac
}

mapfile -d '' -t SOURCES < <(
    find "$ROOT_DIR" -mindepth 2 -type f -name '*.cpp' -print0 |
        sort -z
)

[ "${#SOURCES[@]}" -gt 0 ] || {
    printf '錯誤：找不到任何 .cpp 範例。\n' >&2
    exit 2
}

if [ "$MODE" = dry-run ]; then
    for source in "${SOURCES[@]}"; do
        standard=$(standard_for "$source")
        printf '%q ' "$CXX_BIN" "-std=$standard" -Wall -Wextra -Wpedantic \
            -Wconversion -Wshadow -Werror -pthread "$source" -o '/tmp/<temporary-output>'
        printf '\n'
    done
    printf '共 %d 個範例；dry-run 未建立任何輸出。\n' "${#SOURCES[@]}"
    exit 0
fi

TMP_DIR=$(mktemp -d "${TMPDIR:-/tmp}/codex-cpp-examples.XXXXXX") || {
    printf '錯誤：無法建立暫存目錄。\n' >&2
    exit 2
}
trap 'rm -rf -- "$TMP_DIR"' EXIT HUP INT TERM

passed=0
failed=0
index=0
BUILD_FLAGS=()
if [ "$MODE" = sanitize ]; then
    BUILD_FLAGS=(
        -O1
        -g
        '-fsanitize=address,undefined'
        -fno-omit-frame-pointer
    )
    case "${CXX_BIN##*/}" in
        g++|g++-*)
            # GCC 15 在 -O1 + sanitizer 下會對 libstdc++ <regex> 內部狀態機
            # 產生 -Wmaybe-uninitialized 假陽性。只限此模式抑制 system-header
            # 誤報；一般與 release 建置仍保留完整的 -Werror 守衛。
            BUILD_FLAGS+=(
                -Wno-maybe-uninitialized
            )
            ;;
    esac
elif [ "$MODE" = release ]; then
    BUILD_FLAGS=(
        -O2
        -DNDEBUG
        # assert expression 被移除後，純測試用 local/function 可能合理地變成未使用；
        # 一般模式仍保留全部 -Werror，release 只抑制這三類 NDEBUG 衍生雜訊。
        -Wno-unused-variable
        -Wno-unused-but-set-variable
        -Wno-unused-function
    )
fi

for source in "${SOURCES[@]}"; do
    index=$((index + 1))
    standard=$(standard_for "$source")
    output="$TMP_DIR/example_$index"
    relative=${source#"$ROOT_DIR/"}
    command=(
        "$CXX_BIN"
        "-std=$standard"
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wshadow
        -Werror
        -pthread
        "${BUILD_FLAGS[@]}"
        "$source"
        -o
        "$output"
    )

    printf '[%3d/%3d] %-7s %s\n' \
        "$index" "${#SOURCES[@]}" "$standard" "$relative"

    if ! "${command[@]}"; then
        printf '  編譯失敗\n' >&2
        failed=$((failed + 1))
        continue
    fi

    if [ "$MODE" = run ] || [ "$MODE" = release ] || [ "$MODE" = sanitize ]; then
        if ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 \
           UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
           timeout 10s "$output" >/dev/null; then
            :
        else
            rc=$?
            printf '  執行失敗或逾時（rc=%d）\n' "$rc" >&2
            failed=$((failed + 1))
            continue
        fi
    fi

    passed=$((passed + 1))
done

printf '\n結果：成功 %d，失敗 %d，總計 %d。\n' \
    "$passed" "$failed" "${#SOURCES[@]}"

[ "$failed" -eq 0 ]
