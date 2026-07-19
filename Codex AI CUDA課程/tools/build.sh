#!/usr/bin/env bash
# Codex AI CUDA 課程：單一 CUDA 範例的本機或多架構編譯器。

set -euo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)
COURSE_ROOT=$(dirname -- "$SCRIPT_DIR")
SRC=""
OUT=""
NATIVE=0
DRY=0

usage() {
    cat <<'EOF'
build.sh —— Codex AI CUDA 課程單檔編譯工具

用法：
  build.sh <file.cu> [--native] [-o 輸出檔]

模式：
  預設       建立 sm_75 + sm_89 + sm_120 SASS，並嵌入 compute_120 PTX
  --native   只為目前 GPU 編譯，適合本機快速實驗

選項：
  -o, --output <檔案>  指定輸出；預設為該課 build/<來源檔主名>
  -n, --dry-run        顯示完整 nvcc 命令，不建立目錄、不編譯
  -h, --help           顯示本說明

退出碼：
  0  編譯成功或 help/dry-run 成功
  1  nvcc 編譯失敗
  2  用法、來源、環境或課程根目錄錯誤

範例（從單一課程目錄執行）：
  ../tools/build.sh src/example.cu --native
  ../tools/build.sh src/example.cu
  ../tools/build.sh src/example.cu --native -o /tmp/example

誠實邊界：多架構模式只證明 nvcc 能產生對應映像；sm_89/sm_120 的執行
結果仍須在實體 GPU 上驗證。
EOF
}

die_usage() {
    printf '錯誤：%s\n' "$1" >&2
    exit 2
}

while [ $# -gt 0 ]; do
    case "$1" in
        --native) NATIVE=1; shift ;;
        -o|--output)
            [ $# -ge 2 ] || die_usage "$1 需要輸出檔路徑"
            OUT=$2
            shift 2
            ;;
        -n|--dry-run) DRY=1; shift ;;
        -h|--help) usage; exit 0 ;;
        --) shift; break ;;
        -*) die_usage "未知參數 '$1'（用 --help 看說明）" ;;
        *)
            [ -z "$SRC" ] || die_usage "只能指定一個 .cu 來源檔"
            SRC=$1
            shift
            ;;
    esac
done

[ $# -eq 0 ] || die_usage "-- 後出現未支援的額外參數"
[ -n "$SRC" ] || die_usage "缺少 .cu 來源檔"
[ -f "$COURSE_ROOT/.codex-course-root" ] || die_usage "工具不在有效的 Codex 課程根目錄內"
[ "$(id -u)" -ne 0 ] || die_usage "請以一般使用者執行，不要用 sudo"
[ -f "$SRC" ] || die_usage "來源檔不存在：$SRC"
case "$SRC" in
    *.cu) ;;
    *) die_usage "來源檔必須以 .cu 結尾：$SRC" ;;
esac

SRC_DIR=$(cd -- "$(dirname -- "$SRC")" && pwd -P)
SRC_ABS="$SRC_DIR/$(basename -- "$SRC")"
if [ -z "$OUT" ]; then
    if [ "$(basename -- "$SRC_DIR")" = "src" ]; then
        OUT="$(dirname -- "$SRC_DIR")/build/$(basename -- "${SRC%.cu}")"
    else
        OUT="$SRC_DIR/build/$(basename -- "${SRC%.cu}")"
    fi
fi

COMMON=(-std=c++20 -O3)
if [ "$NATIVE" -eq 1 ]; then
    CMD=(nvcc -arch=native "${COMMON[@]}" "$SRC_ABS" -o "$OUT")
    MODE="本機 native"
else
    CMD=(
        nvcc
        -gencode "arch=compute_75,code=sm_75"
        -gencode "arch=compute_89,code=sm_89"
        -gencode "arch=compute_120,code=sm_120"
        -gencode "arch=compute_120,code=compute_120"
        "${COMMON[@]}"
        "$SRC_ABS"
        -o "$OUT"
    )
    MODE="sm_75 + sm_89 + sm_120 + compute_120 PTX"
fi

printf '模式：%s\n' "$MODE"
printf '來源：%s\n' "$SRC_ABS"
printf '輸出：%s\n' "$OUT"
printf '命令：'
printf '%q ' "${CMD[@]}"
printf '\n'

if [ "$DRY" -eq 1 ]; then
    printf '[dry-run] 未建立輸出、未執行 nvcc。\n'
    exit 0
fi

command -v nvcc >/dev/null 2>&1 || die_usage "找不到 nvcc；請先完成 CUDA Toolkit 安裝"
mkdir -p -- "$(dirname -- "$OUT")"

if command -v nvidia-smi >/dev/null 2>&1; then
    if GPU_NAMES=$(nvidia-smi --query-gpu=name --format=csv,noheader 2>/dev/null | paste -sd ', ' -); then
        [ -z "$GPU_NAMES" ] || printf '本機 GPU：%s\n' "$GPU_NAMES"
    fi
fi

"${CMD[@]}"
printf '完成：%s\n' "$OUT"
