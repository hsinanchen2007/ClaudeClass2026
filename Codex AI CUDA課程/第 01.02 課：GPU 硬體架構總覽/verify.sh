#!/usr/bin/env bash
# 課 1.2：編譯並驗證 CUDA device 屬性與 block/warp/SM runtime 不變量。

set -euo pipefail

LESSON_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)
SRC="$LESSON_DIR/src/gpu_arch_probe.cu"
DRY=0

usage() {
    cat <<'EOF'
verify.sh —— 課 1.2 GPU 架構探針驗證

用法：
  ./verify.sh [--dry-run]

驗證：
  1. 以 C++20、warnings、-arch=native 編譯 gpu_arch_probe.cu
  2. 查詢 SM/warp/register/shared/L2/DRAM 等 CUDA-visible 屬性
  3. 在 GPU 執行 kernel，驗 lane 對應並觀察每個 block 的 sampled SM
  4. 驗證錯誤 device、錯誤 sampling 值與未知參數皆回 exit code 2

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
[ -f "$SRC" ] || die_usage "缺少 gpu_arch_probe.cu"

NVCC_CMD=(nvcc -std=c++20 -O2 -lineinfo "-Xcompiler=-Wall,-Wextra" -arch=native "$SRC")

if [ "$DRY" -eq 1 ]; then
    printf '[dry-run] '
    printf '%q ' "${NVCC_CMD[@]}" -o /tmp/gpu_arch_probe
    printf '\n[dry-run] /tmp/gpu_arch_probe，確認 CUDA 屬性與 sample.validation=PASS\n'
    printf '[dry-run] 驗證 --device 999999、--blocks-per-sm 0、未知參數皆 rc=2\n'
    if [ "${CODEX_COMPUTE_SANITIZER:-0}" = "1" ]; then
        printf '[dry-run] compute-sanitizer --tool memcheck /tmp/gpu_arch_probe\n'
    fi
    exit 0
fi

command -v nvcc >/dev/null 2>&1 || die_usage "找不到 nvcc"

TMP=$(mktemp -d) || die_usage "無法建立短命驗證目錄"
trap 'rm -rf -- "$TMP"' EXIT

"${NVCC_CMD[@]}" -o "$TMP/gpu_arch_probe"
"$TMP/gpu_arch_probe" >"$TMP/probe.out"

grep -Fx 'probe.format=1' "$TMP/probe.out" >/dev/null || {
    printf '探針格式欄位缺失。\n' >&2
    exit 1
}
grep -Eq '^device.compute_capability=[0-9]+\.[0-9]+$' "$TMP/probe.out" || {
    printf 'compute capability 欄位不合法。\n' >&2
    exit 1
}
grep -Fx 'sm.warp_size=32' "$TMP/probe.out" >/dev/null || {
    printf '本課 NVIDIA CUDA warp size 預期為 32。\n' >&2
    exit 1
}
grep -Fx 'sample.invalid_records=0' "$TMP/probe.out" >/dev/null || {
    printf 'lane/warp/SM sample 驗證失敗。\n' >&2
    exit 1
}
grep -Fx 'sample.validation=PASS' "$TMP/probe.out" >/dev/null || {
    printf 'GPU runtime 自我驗證沒有 PASS。\n' >&2
    sed -n '1,80p' "$TMP/probe.out" >&2
    exit 1
}

for key in sm.count sm.max_threads sm.registers_32bit sm.shared_memory_bytes \
    cache.l2_bytes memory.bus_width_bits sample.observed_unique_sm_ids \
    sample.kernel_active_blocks_per_sm; do
    awk -F= -v wanted="$key" '
        $1 == wanted && $2 ~ /^[0-9]+$/ && ($2 + 0) > 0 { found = 1 }
        END { exit(found ? 0 : 1) }
    ' "$TMP/probe.out" || {
        printf '必要正整數欄位缺失或無效：%s\n' "$key" >&2
        exit 1
    }
done
grep -Eq '^sample\.block_sm_conflicts=[0-9]+$' "$TMP/probe.out" || {
    printf 'sampled SM conflict 診斷欄位缺失或無效。\n' >&2
    exit 1
}

set +e
"$TMP/gpu_arch_probe" --device 999999 >"$TMP/bad-device.out" 2>&1
BAD_DEVICE_RC=$?
"$TMP/gpu_arch_probe" --blocks-per-sm 0 >"$TMP/bad-blocks.out" 2>&1
BAD_BLOCKS_RC=$?
"$TMP/gpu_arch_probe" --not-an-option >"$TMP/bad-option.out" 2>&1
BAD_OPTION_RC=$?
set -e
if [ "$BAD_DEVICE_RC" -ne 2 ] || [ "$BAD_BLOCKS_RC" -ne 2 ] || \
    [ "$BAD_OPTION_RC" -ne 2 ]; then
    printf '錯誤輸入退出碼不正確：device=%d blocks=%d option=%d\n' \
        "$BAD_DEVICE_RC" "$BAD_BLOCKS_RC" "$BAD_OPTION_RC" >&2
    exit 1
fi

if [ "${CODEX_COMPUTE_SANITIZER:-0}" = "1" ]; then
    command -v compute-sanitizer >/dev/null 2>&1 || die_usage "找不到 compute-sanitizer"
    compute-sanitizer --tool memcheck --error-exitcode=99 \
        "$TMP/gpu_arch_probe" >"$TMP/sanitizer.out" 2>&1 || {
        sed -n '1,100p' "$TMP/sanitizer.out" >&2
        exit 1
    }
fi

printf '課 1.2 驗證完成：\n'
grep -E '^(device.name|device.compute_capability|sm.count|sm.warp_size|cache.l2_bytes|memory.theoretical_peak_gb_s|sample.observed_unique_sm_ids|sample.block_sm_conflicts|sample.validation)=' \
    "$TMP/probe.out"
