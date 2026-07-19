#!/usr/bin/env bash
# Codex AI CUDA 課程：逐課、多架構編譯稽核。

set -u

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)
COURSE_ROOT="${CODEX_COURSE_ROOT:-$(dirname -- "$SCRIPT_DIR")}"
WORKSPACE_MARKER_VALUE="codex-ai-cuda-workspace-v1"
PUBLISH_MARKER_VALUE="codex-ai-cuda-publish-v1"
LESSONS_ROOT=""
LAYOUT=""
CUSTOM_VERIFY_MARKER_VALUE="codex-custom-verify-v1"
ARCHES="${ARCHES:-75 89 90 120}"
CXX_STD="${CXX_STD:-c++20}"
LESSON=""
DRY=0

usage() {
    cat <<'EOF'
check_build.sh —— Codex 課程程式碼的多架構編譯稽核

用法：
  check_build.sh
  check_build.sh --lesson "第 01.01 課：為什麼需要 GPU"
  check_build.sh --lesson "<課別>" --arch "75 120"

選項：
  --lesson <課別>   只稽核指定課；預設掃描 Codex AI CUDA課程/ 下全部課程
  --arch "<清單>"  空白分隔的 CUDA compute capability（預設 "75 89 90 120"）
  -n, --dry-run     只列出驗證計畫，不需要工具鏈，不寫課程目錄
  -h, --help        顯示本說明

環境變數：
  CODEX_COURSE_ROOT  覆寫工具根目錄；可含 .codex-course-root（私有工作區）
                     或 .codex-publish-target（獨立發布課程）
  ARCHES             同 --arch
  CXX_STD            C++ 標準（預設 c++20）

退出碼：
  0  全部成功、沒有可編譯來源，或 dry-run/help 成功
  1  至少一個來源編譯失敗
  2  用法、路徑、工具鏈或課程根目錄錯誤

預設遞迴掃描 src/ 的 .cu/.cpp/.cc/.cxx/.py；C/CUDA 檔案必須可獨立
連結，Python 只做語法驗證。若課程有多檔、CMake、Triton、額外函式庫或特殊
runtime 要求，應提供可執行 verify.sh；若要完全取代通用掃描，再加內容
精確為 codex-custom-verify-v1 的 .codex-custom-verify。sm_89/sm_90/sm_120
的執行正確性與效能，仍必須在對應實體 GPU 上驗證。
EOF
}

die_usage() {
    printf '錯誤：%s\n' "$1" >&2
    exit 2
}

marker_is_exact() {
    local file=$1 expected=$2 line_count value
    [ -f "$file" ] || return 1
    line_count=$(LC_ALL=C wc -l <"$file" 2>/dev/null) || return 1
    [ "$line_count" -eq 1 ] || return 1
    IFS= read -r value <"$file" || return 1
    [ "$value" = "$expected" ]
}

collect_sorted_nul() {
    local output=$1
    shift
    if ! find "$@" -print0 >"$output.unsorted"; then
        return 2
    fi
    sort -z "$output.unsorted" >"$output" || return 2
}

while [ $# -gt 0 ]; do
    case "$1" in
        --lesson)
            [ $# -ge 2 ] || die_usage "--lesson 需要參數"
            LESSON=$2
            shift 2
            ;;
        --arch)
            [ $# -ge 2 ] || die_usage "--arch 需要參數"
            ARCHES=$2
            shift 2
            ;;
        -n|--dry-run) DRY=1; shift ;;
        -h|--help) usage; exit 0 ;;
        *) die_usage "未知參數 '$1'（用 --help 看說明）" ;;
    esac
done

[ "$(id -u)" -ne 0 ] || die_usage "請以一般使用者執行，不要用 sudo"
if marker_is_exact "$COURSE_ROOT/.codex-course-root" "$WORKSPACE_MARKER_VALUE"; then
    LESSONS_ROOT="$COURSE_ROOT/Codex AI CUDA課程"
    LAYOUT="私有工作區"
elif marker_is_exact "$COURSE_ROOT/.codex-publish-target" "$PUBLISH_MARKER_VALUE"; then
    LESSONS_ROOT="$COURSE_ROOT"
    LAYOUT="獨立發布課程"
else
    die_usage "不是有效的 Codex 私有工作區或發布課程根目錄：$COURSE_ROOT"
fi
[ -d "$LESSONS_ROOT" ] || die_usage "缺少 Codex 課程目錄：$LESSONS_ROOT"
case "$LESSON" in
    */*|.|..) die_usage "--lesson 只能是課程根目錄下的單一目錄名稱" ;;
esac

read -r -a ARCH_ARRAY <<< "$ARCHES"
[ "${#ARCH_ARRAY[@]}" -gt 0 ] || die_usage "架構清單不可為空"
for ARCH in "${ARCH_ARRAY[@]}"; do
    [[ "$ARCH" =~ ^[0-9]+$ ]] || die_usage "無效架構 '$ARCH'；請用 75、89、90、120 這類數字"
done

TMP=$(mktemp -d) || die_usage "無法建立短命掃描目錄"
trap 'rm -rf -- "$TMP"' EXIT

LESSON_DIRS=()
if [ -n "$LESSON" ]; then
    [ -d "$LESSONS_ROOT/$LESSON" ] || die_usage "找不到課程目錄：$LESSONS_ROOT/$LESSON"
    LESSON_DIRS=("$LESSONS_ROOT/$LESSON")
else
    collect_sorted_nul "$TMP/lessons" "$LESSONS_ROOT" -mindepth 1 -maxdepth 1 \
        -type d -name '第 * 課：*' || die_usage "掃描課程目錄失敗"
    mapfile -d '' -t LESSON_DIRS <"$TMP/lessons"
fi

[ "${#LESSON_DIRS[@]}" -gt 0 ] || die_usage "找不到任何課程目錄"

PASS=0
FAIL=0
SKIP=0

printf '工作區根目錄：%s\n' "$COURSE_ROOT"
printf '課程內容目錄：%s\n' "$LESSONS_ROOT"
printf '工具布局  ：%s\n' "$LAYOUT"
printf 'CUDA 架構  ：%s\n' "${ARCH_ARRAY[*]}"
printf 'C++ 標準   ：%s\n\n' "$CXX_STD"

for LESSON_DIR in "${LESSON_DIRS[@]}"; do
    LESSON_NAME=$(basename -- "$LESSON_DIR")
    SRC_DIR="$LESSON_DIR/src"
    VERIFY_SCRIPT="$LESSON_DIR/verify.sh"
    CUSTOM_MARKER="$LESSON_DIR/.codex-custom-verify"
    USE_GENERIC=1
    printf '── %s\n' "$LESSON_NAME"
    [ -f "$LESSON_DIR/NOTE.md" ] || die_usage "課程缺少 NOTE.md：$LESSON_NAME"

    if [ -e "$CUSTOM_MARKER" ]; then
        marker_is_exact "$CUSTOM_MARKER" "$CUSTOM_VERIFY_MARKER_VALUE" ||
            die_usage "自訂驗證 marker 內容不正確：$CUSTOM_MARKER"
        [ -x "$VERIFY_SCRIPT" ] || die_usage "自訂驗證模式缺少可執行 verify.sh：$LESSON_NAME"
        USE_GENERIC=0
        printf '   使用課程自訂 verify.sh，不執行通用單檔掃描。\n'
    fi

    FILES=()
    if [ "$USE_GENERIC" -eq 1 ] && [ -d "$SRC_DIR" ]; then
        collect_sorted_nul "$TMP/files" "$SRC_DIR" -type f \
            \( -name '*.cu' -o -name '*.cpp' -o -name '*.cc' -o -name '*.cxx' -o -name '*.py' \) ||
            die_usage "掃描來源檔失敗：$SRC_DIR"
        mapfile -d '' -t FILES <"$TMP/files"
    fi

    for FILE in "${FILES[@]}"; do
        DISPLAY=${FILE#"$SRC_DIR/"}
        case "$FILE" in
            *.cu)
                if [ "$DRY" -eq 0 ]; then
                    command -v nvcc >/dev/null 2>&1 || die_usage "找不到 nvcc"
                fi
                for ARCH in "${ARCH_ARRAY[@]}"; do
                    if [ "$DRY" -eq 1 ]; then
                        printf '   [dry-run] nvcc -std=%s -O3 -Xcompiler=-Wall,-Wextra -arch=sm_%s %s\n' "$CXX_STD" "$ARCH" "$DISPLAY"
                        continue
                    fi
                    if nvcc -std="$CXX_STD" -O3 -Xcompiler=-Wall,-Wextra \
                        -arch="sm_$ARCH" "$FILE" -o "$TMP/cuda-out" 2>"$TMP/error"; then
                        printf '   OK   %-34s sm_%s\n' "$DISPLAY" "$ARCH"
                        PASS=$((PASS + 1))
                    else
                        printf '   FAIL %-34s sm_%s\n' "$DISPLAY" "$ARCH"
                        sed -n '1,8{s/^/        /;p;}' "$TMP/error"
                        FAIL=$((FAIL + 1))
                    fi
                done
                ;;
            *.cpp|*.cc|*.cxx)
                if [ "$DRY" -eq 0 ]; then
                    command -v g++ >/dev/null 2>&1 || die_usage "找不到 g++"
                fi
                if [ "$DRY" -eq 1 ]; then
                    printf '   [dry-run] g++ -std=%s -O2 -Wall -Wextra -Wpedantic %s\n' "$CXX_STD" "$DISPLAY"
                elif g++ -std="$CXX_STD" -O2 -Wall -Wextra -Wpedantic \
                    "$FILE" -o "$TMP/cpp-out" 2>"$TMP/error"; then
                    printf '   OK   %-34s host g++\n' "$DISPLAY"
                    PASS=$((PASS + 1))
                else
                    printf '   FAIL %-34s host g++\n' "$DISPLAY"
                    sed -n '1,8{s/^/        /;p;}' "$TMP/error"
                    FAIL=$((FAIL + 1))
                fi
                ;;
            *.py)
                if [ "$DRY" -eq 1 ]; then
                    printf '   [dry-run] python3 -m py_compile %s\n' "$DISPLAY"
                elif ! command -v python3 >/dev/null 2>&1; then
                    die_usage "找不到 python3"
                elif PYTHONPYCACHEPREFIX="$TMP/pycache" python3 -m py_compile "$FILE" 2>"$TMP/error"; then
                    printf '   OK   %-34s Python syntax\n' "$DISPLAY"
                    PASS=$((PASS + 1))
                else
                    printf '   FAIL %-34s Python syntax\n' "$DISPLAY"
                    sed -n '1,8{s/^/        /;p;}' "$TMP/error"
                    FAIL=$((FAIL + 1))
                fi
                ;;
        esac
    done

    if [ -e "$VERIFY_SCRIPT" ] && [ ! -x "$VERIFY_SCRIPT" ]; then
        die_usage "verify.sh 存在但不可執行：$LESSON_NAME"
    fi
    if [ -x "$VERIFY_SCRIPT" ]; then
        if [ "$DRY" -eq 1 ]; then
            if "$VERIFY_SCRIPT" --dry-run; then
                printf '   [dry-run] verify.sh 計畫驗證完成。\n'
            else
                printf '   FAIL verify.sh --dry-run\n'
                FAIL=$((FAIL + 1))
            fi
        elif "$VERIFY_SCRIPT"; then
            printf '   OK   verify.sh runtime/tests\n'
            PASS=$((PASS + 1))
        else
            printf '   FAIL verify.sh runtime/tests\n'
            FAIL=$((FAIL + 1))
        fi
    elif [ "${#FILES[@]}" -eq 0 ]; then
        printf '   沒有可驗證來源或 verify.sh，略過。\n'
        SKIP=$((SKIP + 1))
    fi
    printf '\n'
done

if [ "$DRY" -eq 1 ]; then
    if [ "$FAIL" -gt 0 ]; then
        printf '[dry-run] 驗證計畫有失敗。\n' >&2
        exit 1
    fi
    printf '[dry-run] 未執行編譯或 runtime 測試。\n'
    exit 0
fi

printf '結果：成功 %d / 失敗 %d / 略過 %d\n' "$PASS" "$FAIL" "$SKIP"
if [ "$FAIL" -gt 0 ]; then
    printf '至少一個編譯失敗；修正後才能交付。\n' >&2
    exit 1
fi
printf '全部編譯成功。sm_89/sm_90/sm_120 僅代表編譯驗證，尚非實機執行驗證。\n'
