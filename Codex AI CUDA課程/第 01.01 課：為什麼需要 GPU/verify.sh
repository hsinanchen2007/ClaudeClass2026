#!/usr/bin/env bash
# 課 1.1：獨立驗證數學範例、錯誤輸入與本機 CUDA runtime 結果。

set -euo pipefail

LESSON_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)
SRC_DIR="$LESSON_DIR/src"
DRY=0

usage() {
    cat <<'EOF'
verify.sh —— 課 1.1 編譯與 runtime 正確性驗證

用法：
  ./verify.sh [--dry-run]

驗證：
  1. 以 C++20 + warnings 編譯 scaling_laws.cpp
  2. 驗證 Amdahl/Gustafson 數值與錯誤輸入 exit code 2
  3. 以 -arch=native 編譯、執行 latency_throughput.cu
  4. 確認三組 SAXPY 都 PASS，且沒有 FAIL

環境變數：
  CODEX_COMPUTE_SANITIZER=1  額外執行 compute-sanitizer memcheck（較慢）

退出碼：0=成功，1=編譯/測試失敗，2=用法或環境錯誤。
EOF
}

die_usage() {
    printf '錯誤：%s\n' "$1" >&2
    exit 2
}

while [ $# -gt 0 ]; do
    case "$1" in
        -n|--dry-run) DRY=1; shift ;;
        -h|--help) usage; exit 0 ;;
        *) die_usage "未知參數 '$1'" ;;
    esac
done

[ "$(id -u)" -ne 0 ] || die_usage "請以一般使用者執行，不要用 sudo"
[ -f "$SRC_DIR/scaling_laws.cpp" ] || die_usage "缺少 scaling_laws.cpp"
[ -f "$SRC_DIR/latency_throughput.cu" ] || die_usage "缺少 latency_throughput.cu"

CXX_CMD=(g++ -std=c++20 -O2 -Wall -Wextra -Wpedantic "$SRC_DIR/scaling_laws.cpp")
NVCC_CMD=(nvcc -std=c++20 -O3 "-Xcompiler=-Wall,-Wextra" -arch=native "$SRC_DIR/latency_throughput.cu")

if [ "$DRY" -eq 1 ]; then
    printf '[dry-run] '
    printf '%q ' "${CXX_CMD[@]}" -o /tmp/scaling_laws
    printf '\n[dry-run] /tmp/scaling_laws 0.05 64，並驗證錯誤輸入\n'
    printf '[dry-run] '
    printf '%q ' "${NVCC_CMD[@]}" -o /tmp/latency_throughput
    printf '\n[dry-run] /tmp/latency_throughput，確認三組 PASS\n'
    if [ "${CODEX_COMPUTE_SANITIZER:-0}" = "1" ]; then
        printf '[dry-run] compute-sanitizer --tool memcheck /tmp/latency_throughput\n'
    fi
    exit 0
fi

command -v g++ >/dev/null 2>&1 || die_usage "找不到 g++"
command -v nvcc >/dev/null 2>&1 || die_usage "找不到 nvcc"

TMP=$(mktemp -d) || die_usage "無法建立短命驗證目錄"
trap 'rm -rf -- "$TMP"' EXIT

"${CXX_CMD[@]}" -o "$TMP/scaling_laws"
"$TMP/scaling_laws" 0.05 64 >"$TMP/scaling.out"
grep -F '15.42' "$TMP/scaling.out" >/dev/null || {
    printf 'Amdahl 數值驗證失敗。\n' >&2
    exit 1
}
grep -F '60.85' "$TMP/scaling.out" >/dev/null || {
    printf 'Gustafson 數值驗證失敗。\n' >&2
    exit 1
}

set +e
"$TMP/scaling_laws" 1.2 64 >"$TMP/bad-fraction.out" 2>&1
BAD_FRACTION_RC=$?
"$TMP/scaling_laws" 0.1 1.5 >"$TMP/bad-count.out" 2>&1
BAD_COUNT_RC=$?
set -e
[ "$BAD_FRACTION_RC" -eq 2 ] && [ "$BAD_COUNT_RC" -eq 2 ] || {
    printf '錯誤輸入退出碼不正確：fraction=%d count=%d\n' \
        "$BAD_FRACTION_RC" "$BAD_COUNT_RC" >&2
    exit 1
}

"${NVCC_CMD[@]}" -o "$TMP/latency_throughput"
"$TMP/latency_throughput" >"$TMP/latency.out"
PASS_COUNT=$(grep -cE '[[:space:]]PASS$' "$TMP/latency.out" || true)
if [ "$PASS_COUNT" -ne 3 ] || grep -qE '[[:space:]]FAIL$' "$TMP/latency.out"; then
    printf 'SAXPY runtime 驗證失敗：\n' >&2
    sed -n '1,20p' "$TMP/latency.out" >&2
    exit 1
fi

if [ "${CODEX_COMPUTE_SANITIZER:-0}" = "1" ]; then
    command -v compute-sanitizer >/dev/null 2>&1 || die_usage "找不到 compute-sanitizer"
    compute-sanitizer --tool memcheck --error-exitcode=99 \
        "$TMP/latency_throughput" >"$TMP/sanitizer.out" 2>&1 || {
        sed -n '1,80p' "$TMP/sanitizer.out" >&2
        exit 1
    }
fi

printf '課 1.1 驗證完成：數學輸出、錯誤退出碼與 3 組 CUDA SAXPY 皆正確。\n'
