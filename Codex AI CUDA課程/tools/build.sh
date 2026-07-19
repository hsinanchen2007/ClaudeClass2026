#!/usr/bin/env bash
# Codex AI CUDA 課程：單一 C++/CUDA 範例的安全編譯工具。

set -euo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)
COURSE_ROOT=$(dirname -- "$SCRIPT_DIR")
WORKSPACE_MARKER_VALUE="codex-ai-cuda-workspace-v1"
PUBLISH_MARKER_VALUE="codex-ai-cuda-publish-v1"
LESSONS_ROOT=""
LAYOUT=""
SRC=""
OUT=""
OUT_GIVEN=0
NATIVE=0
DRY=0
TMP_OUT=""

usage() {
    cat <<'EOF'
build.sh —— Codex AI CUDA 課程單檔安全編譯工具

用法：
  build.sh <file.cu|file.cpp|file.cc|file.cxx> [--native] [-o 輸出檔]

CUDA 模式：
  預設       建立 sm_75 + sm_89 + sm_90 + sm_120 SASS，並嵌入 compute_120 PTX
  --native   只為目前 GPU 編譯，適合本機快速實驗

純 C++ 模式：
  預設       使用 g++、C++20、O3 與 warnings 建立可攜式 host binary
  --native   另加 -march=native；產物可能無法在不同 CPU 上執行

選項：
  -o, --output <檔案>  指定輸出；預設為該課 build/<來源相對路徑去副檔名>
  -n, --dry-run        顯示編譯命令與發布路徑，不建立目錄、不編譯
  -h, --help           顯示本說明

輸出安全：
  * 可從 Codex 私有工作區 tools/，或發布課程副本 tools/ 執行。
  * 來源必須在 Codex AI CUDA課程的單一課程目錄內。
  * 預設永遠輸出到該課 build/，不會把執行檔寫回 src/。
  * 拒絕來源本身、src/、symlink、hardlink、Git tracked file，以及常見來源/文件
    副檔名作為 -o 目的地。
  * 編譯先寫同目錄短命檔；成功後才原子替換目的檔。編譯失敗會保留舊 binary。

退出碼：
  0  編譯成功或 help/dry-run 成功
  1  編譯或輸出發布失敗
  2  用法、來源、工具鏈、輸出安全或課程根目錄錯誤

範例（從單一課程目錄執行）：
  ../tools/build.sh src/example.cu --native
  ../tools/build.sh src/example.cu
  ../tools/build.sh src/model.cpp
  ../tools/build.sh src/model.cpp --native -o /tmp/model

誠實邊界：多架構模式只證明 nvcc 能產生對應映像；sm_89/sm_90/sm_120 的執行
結果仍須在實體 GPU 上驗證。單檔工具也不會自動補上特殊函式庫或多檔 linker flags。
EOF
}

die_usage() {
    printf '錯誤：%s\n' "$1" >&2
    exit 2
}

die_build() {
    printf '編譯錯誤：%s\n' "$1" >&2
    exit 1
}

cleanup() {
    if [ -n "$TMP_OUT" ]; then
        rm -f -- "$TMP_OUT"
    fi
}

marker_is_exact() {
    local file=$1 expected=$2 line_count value
    [ -f "$file" ] || return 1
    line_count=$(LC_ALL=C wc -l <"$file" 2>/dev/null) || return 1
    [ "$line_count" -eq 1 ] || return 1
    IFS= read -r value <"$file" || return 1
    [ "$value" = "$expected" ]
}

output_name_is_source_or_document() {
    local name=$1
    case "$name" in
        CMakeLists.txt|Makefile|Dockerfile|Containerfile|\
        *.c|*.C|*.cc|*.cpp|*.cxx|*.cu|*.cuh|*.h|*.hh|*.hpp|*.hxx|\
        *.py|*.pyi|*.sh|*.bash|*.zsh|*.fish|*.rs|*.go|*.java|\
        *.md|*.rst|*.txt|*.json|*.jsonl|*.toml|*.yaml|*.yml|*.xml|\
        *.ini|*.cfg|*.conf|*.cmake|*.ipynb)
            return 0
            ;;
    esac
    return 1
}

validate_output_path() {
    local phase=$1 output_base output_rel link_count

    case "$OUT" in
        *$'\n'*|*$'\r'*|*$'\t'*)
            die_usage "輸出路徑不可含 tab 或換行"
            ;;
    esac

    [ ! -L "$OUT_LEXICAL" ] ||
        die_usage "拒絕以 symlink 作為輸出目的地（$phase）：$OUT_LEXICAL"
    [ "$OUT_ABS" != "$SRC_ABS" ] ||
        die_usage "輸出不可等於來源檔：$SRC_ABS"

    case "$OUT_ABS" in
        "$LESSONS_REAL"/*/src|"$LESSONS_REAL"/*/src/*)
            die_usage "輸出不可位於任何課程的 src/ 來源樹：$OUT_ABS"
            ;;
        "$GIT_ROOT/.git"|"$GIT_ROOT/.git"/*)
            die_usage "輸出不可位於 Git metadata：$OUT_ABS"
            ;;
    esac

    output_base=$(basename -- "$OUT_ABS")
    case "$output_base" in
        ''|.|..) die_usage "無效輸出檔名：$OUT_ABS" ;;
    esac
    if output_name_is_source_or_document "$output_base"; then
        die_usage "拒絕以來源或文件檔名作為執行檔輸出：$OUT_ABS"
    fi

    case "$OUT_ABS" in
        "$GIT_ROOT"/*)
            output_rel=${OUT_ABS#"$GIT_ROOT"/}
            if git -C "$GIT_ROOT" ls-files --error-unmatch -- ":(literal)$output_rel" \
                >/dev/null 2>&1; then
                die_usage "拒絕覆寫 Git tracked file（$phase）：$OUT_ABS"
            fi
            ;;
    esac

    if [ -e "$OUT_ABS" ]; then
        [ -f "$OUT_ABS" ] ||
            die_usage "輸出已存在但不是一般檔案（$phase）：$OUT_ABS"
        if [ "$OUT_ABS" -ef "$SRC_ABS" ]; then
            die_usage "輸出與來源指向同一 inode（$phase）：$OUT_ABS"
        fi
        link_count=$(stat -Lc '%h' -- "$OUT_ABS" 2>/dev/null) ||
            die_usage "無法檢查既有輸出的 hardlink 數量：$OUT_ABS"
        [[ "$link_count" =~ ^[0-9]+$ ]] ||
            die_usage "輸出的 hardlink 數量格式不明：$OUT_ABS"
        [ "$link_count" -eq 1 ] ||
            die_usage "拒絕替換具有 $link_count 個 hardlink 的輸出：$OUT_ABS"
    fi
}

trap cleanup EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM

while [ $# -gt 0 ]; do
    case "$1" in
        --native) NATIVE=1; shift ;;
        -o|--output)
            [ $# -ge 2 ] || die_usage "$1 需要輸出檔路徑"
            [ "$OUT_GIVEN" -eq 0 ] || die_usage "只能指定一次輸出路徑"
            OUT=$2
            OUT_GIVEN=1
            [ -n "$OUT" ] || die_usage "輸出檔路徑不可為空"
            shift 2
            ;;
        -n|--dry-run) DRY=1; shift ;;
        -h|--help) usage; exit 0 ;;
        --) shift; break ;;
        -*) die_usage "未知參數 '$1'（用 --help 看說明）" ;;
        *)
            [ -z "$SRC" ] || die_usage "只能指定一個 C++/CUDA 來源檔"
            SRC=$1
            shift
            ;;
    esac
done

[ $# -eq 0 ] || die_usage "-- 後出現未支援的額外參數"
[ -n "$SRC" ] || die_usage "缺少 C++/CUDA 來源檔"
[ "$(id -u)" -ne 0 ] || die_usage "請以一般使用者執行，不要用 sudo"
[ -f "$SRC" ] || die_usage "來源檔不存在：$SRC"

if marker_is_exact "$COURSE_ROOT/.codex-course-root" "$WORKSPACE_MARKER_VALUE"; then
    LESSONS_ROOT="$COURSE_ROOT/Codex AI CUDA課程"
    LAYOUT="私有工作區"
elif marker_is_exact "$COURSE_ROOT/.codex-publish-target" "$PUBLISH_MARKER_VALUE"; then
    LESSONS_ROOT="$COURSE_ROOT"
    LAYOUT="獨立發布課程"
else
    die_usage "工具不在有效的 Codex 私有工作區或發布課程根目錄內"
fi

SRC_ABS=$(realpath -- "$SRC") || die_usage "無法解析來源檔：$SRC"
case "$SRC_ABS" in
    *.cu) SOURCE_KIND="CUDA" ;;
    *.cpp|*.cc|*.cxx) SOURCE_KIND="C++" ;;
    *) die_usage "只支援 .cu、.cpp、.cc、.cxx：$SRC_ABS" ;;
esac

LESSONS_REAL=$(realpath -- "$LESSONS_ROOT") || die_usage "無法解析 Codex 課程目錄"
case "$SRC_ABS" in
    "$LESSONS_REAL"/*) ;;
    *) die_usage "來源檔必須位於 Codex AI CUDA課程/ 內：$SRC_ABS" ;;
esac

SRC_REL=${SRC_ABS#"$LESSONS_REAL"/}
case "$SRC_REL" in
    */*) ;;
    *) die_usage "來源檔必須位於單一課程目錄內：$SRC_ABS" ;;
esac
LESSON_NAME=${SRC_REL%%/*}
case "$LESSON_NAME" in
    第\ *\ 課：*) ;;
    *) die_usage "來源檔不在合法的『第 XX.YY 課：課名』目錄：$SRC_ABS" ;;
esac
LESSON_DIR_REAL="$LESSONS_REAL/$LESSON_NAME"
[ -f "$LESSON_DIR_REAL/NOTE.md" ] || die_usage "課程缺少 NOTE.md：$LESSON_NAME"

if [ "$OUT_GIVEN" -eq 0 ]; then
    SRC_IN_LESSON=${SRC_ABS#"$LESSON_DIR_REAL"/}
    case "$SRC_IN_LESSON" in
        src/*) OUTPUT_REL=${SRC_IN_LESSON#src/} ;;
        *) OUTPUT_REL=$SRC_IN_LESSON ;;
    esac
    OUTPUT_REL=${OUTPUT_REL%.*}
    [ -n "$OUTPUT_REL" ] || die_usage "無法由來源名稱產生安全輸出檔名：$SRC_ABS"
    OUT="$LESSON_DIR_REAL/build/$OUTPUT_REL"
fi

case "$OUT" in
    /*) OUT_INPUT_ABS=$OUT ;;
    *) OUT_INPUT_ABS="$PWD/$OUT" ;;
esac
OUT_LEXICAL=$(realpath -ms -- "$OUT_INPUT_ABS") || die_usage "無法正規化輸出路徑：$OUT"
OUT_ABS=$(realpath -m -- "$OUT_LEXICAL") || die_usage "無法解析輸出路徑：$OUT"
OUT_DIR=$(dirname -- "$OUT_ABS")

command -v git >/dev/null 2>&1 || die_usage "找不到 git，無法執行 tracked-file 安全檢查"
GIT_ROOT=$(git -C "$COURSE_ROOT" rev-parse --show-toplevel 2>/dev/null) ||
    die_usage "Codex 課程根目錄不是有效 Git repo"
GIT_ROOT=$(realpath -- "$GIT_ROOT") || die_usage "無法解析 Git 根目錄"
validate_output_path "編譯前"

if [ "$SOURCE_KIND" = "CUDA" ]; then
    COMPILER=nvcc
    COMMON=(-std=c++20 -O3 "-Xcompiler=-Wall,-Wextra")
    if [ "$NATIVE" -eq 1 ]; then
        CMD_BASE=(nvcc -arch=native "${COMMON[@]}")
        MODE="CUDA 本機 native"
    else
        CMD_BASE=(
            nvcc
            -gencode "arch=compute_75,code=sm_75"
            -gencode "arch=compute_89,code=sm_89"
            -gencode "arch=compute_90,code=sm_90"
            -gencode "arch=compute_120,code=sm_120"
            -gencode "arch=compute_120,code=compute_120"
            "${COMMON[@]}"
        )
        MODE="CUDA sm_75 + sm_89 + sm_90 + sm_120 + compute_120 PTX"
    fi
else
    COMPILER=g++
    CMD_BASE=(g++ -std=c++20 -O3 -Wall -Wextra -Wpedantic)
    if [ "$NATIVE" -eq 1 ]; then
        CMD_BASE+=(-march=native)
        MODE="host C++ native (-march=native)"
    else
        MODE="host C++ 可攜式"
    fi
fi

printf '模式：%s\n' "$MODE"
printf '布局：%s\n' "$LAYOUT"
printf '來源：%s\n' "$SRC_ABS"
printf '輸出：%s\n' "$OUT_ABS"

if [ "$DRY" -eq 1 ]; then
    TEMP_DISPLAY="$OUT_DIR/.codex-build.XXXXXX"
    CMD=("${CMD_BASE[@]}" "$SRC_ABS" -o "$TEMP_DISPLAY")
    printf '命令：'
    printf '%q ' "${CMD[@]}"
    printf '\n[dry-run] 未建立目錄、未執行 %s；成功時才會原子發布到上述輸出。\n' "$COMPILER"
    exit 0
fi

command -v "$COMPILER" >/dev/null 2>&1 ||
    die_usage "找不到 $COMPILER；請先完成相應工具鏈安裝"
mkdir -p -- "$OUT_DIR" || die_build "無法建立輸出目錄：$OUT_DIR"
TMP_OUT=$(mktemp "$OUT_DIR/.codex-build.XXXXXX") ||
    die_build "無法在輸出目錄建立短命編譯檔：$OUT_DIR"
CMD=("${CMD_BASE[@]}" "$SRC_ABS" -o "$TMP_OUT")

printf '命令：'
printf '%q ' "${CMD[@]}"
printf '\n'

if [ "$SOURCE_KIND" = "CUDA" ] && command -v nvidia-smi >/dev/null 2>&1; then
    if GPU_NAMES=$(nvidia-smi --query-gpu=name --format=csv,noheader 2>/dev/null | paste -sd ',' -); then
        [ -z "$GPU_NAMES" ] || printf '本機 GPU：%s\n' "${GPU_NAMES//,/, }"
    fi
fi

if ! "${CMD[@]}"; then
    printf '編譯失敗；既有輸出未變更：%s\n' "$OUT_ABS" >&2
    exit 1
fi
[ -f "$TMP_OUT" ] && [ -x "$TMP_OUT" ] ||
    die_build "編譯器成功返回，但未產生可執行檔"

# 再檢查一次，縮小編譯期間目的檔被替換的 TOCTOU 空窗。
validate_output_path "發布前"
if ! mv -fT -- "$TMP_OUT" "$OUT_ABS"; then
    die_build "無法原子發布輸出：$OUT_ABS"
fi
TMP_OUT=""
printf '完成：%s\n' "$OUT_ABS"
