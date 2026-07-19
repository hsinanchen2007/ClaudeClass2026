#!/usr/bin/env bash
# Codex AI CUDA 課程：逐課、多架構編譯稽核。

set -u

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)
COURSE_ROOT="${CODEX_COURSE_ROOT:-$(dirname -- "$SCRIPT_DIR")}"
ARCHES="${ARCHES:-75 89 120}"
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
  --lesson <課別>   只稽核指定課；預設掃描本 Codex 根目錄下全部課程
  --arch "<清單>"  空白分隔的 CUDA compute capability（預設 "75 89 120"）
  -n, --dry-run     只列出編譯計畫，不需要 nvcc/g++，不產生檔案
  -h, --help        顯示本說明

環境變數：
  CODEX_COURSE_ROOT  覆寫 Codex 課程根目錄（必須含 .codex-course-root）
  ARCHES             同 --arch
  CXX_STD            C++ 標準（預設 c++20）

退出碼：
  0  全部成功、沒有可編譯來源，或 dry-run/help 成功
  1  至少一個來源編譯失敗
  2  用法、路徑、工具鏈或課程根目錄錯誤

誠實邊界：這支工具只驗證編譯。sm_89/sm_120 的執行正確性與效能，必須
在對應實體 GPU 上另行驗證。需要特殊 linker/library 旗標的課，應提供該課
自己的可重現建置命令並在 NOTE.md 說明。
EOF
}

die_usage() {
    printf '錯誤：%s\n' "$1" >&2
    exit 2
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
[ -f "$COURSE_ROOT/.codex-course-root" ] || die_usage "不是有效的 Codex 課程根目錄：$COURSE_ROOT"
case "$LESSON" in
    */*|.|..) die_usage "--lesson 只能是課程根目錄下的單一目錄名稱" ;;
esac

read -r -a ARCH_ARRAY <<< "$ARCHES"
[ "${#ARCH_ARRAY[@]}" -gt 0 ] || die_usage "架構清單不可為空"
for ARCH in "${ARCH_ARRAY[@]}"; do
    [[ "$ARCH" =~ ^[0-9]+$ ]] || die_usage "無效架構 '$ARCH'；請用 75、89、120 這類數字"
done

if [ "$DRY" -eq 0 ]; then
    command -v nvcc >/dev/null 2>&1 || die_usage "找不到 nvcc"
    command -v g++ >/dev/null 2>&1 || die_usage "找不到 g++"
    TMP=$(mktemp -d) || die_usage "無法建立暫存目錄"
    trap 'rm -rf -- "$TMP"' EXIT
else
    TMP=""
fi

LESSON_DIRS=()
if [ -n "$LESSON" ]; then
    [ -d "$COURSE_ROOT/$LESSON" ] || die_usage "找不到課程目錄：$COURSE_ROOT/$LESSON"
    LESSON_DIRS=("$COURSE_ROOT/$LESSON")
else
    while IFS= read -r -d '' DIR; do
        LESSON_DIRS+=("$DIR")
    done < <(find "$COURSE_ROOT" -mindepth 1 -maxdepth 1 -type d -name '第 * 課：*' -print0 | sort -z)
fi

[ "${#LESSON_DIRS[@]}" -gt 0 ] || die_usage "找不到任何課程目錄"

PASS=0
FAIL=0
SKIP=0

printf '課程根目錄：%s\n' "$COURSE_ROOT"
printf 'CUDA 架構  ：%s\n' "${ARCH_ARRAY[*]}"
printf 'C++ 標準   ：%s\n\n' "$CXX_STD"

for LESSON_DIR in "${LESSON_DIRS[@]}"; do
    LESSON_NAME=$(basename -- "$LESSON_DIR")
    SRC_DIR="$LESSON_DIR/src"
    printf '── %s\n' "$LESSON_NAME"
    if [ ! -d "$SRC_DIR" ]; then
        printf '   無 src/，略過。\n\n'
        SKIP=$((SKIP + 1))
        continue
    fi

    FILES=()
    while IFS= read -r -d '' FILE; do
        FILES+=("$FILE")
    done < <(find "$SRC_DIR" -maxdepth 1 -type f \( -name '*.cu' -o -name '*.cpp' \) -print0 | sort -z)

    if [ "${#FILES[@]}" -eq 0 ]; then
        printf '   src/ 沒有 .cu 或 .cpp，略過。\n\n'
        SKIP=$((SKIP + 1))
        continue
    fi

    for FILE in "${FILES[@]}"; do
        BASE=$(basename -- "$FILE")
        case "$FILE" in
            *.cu)
                for ARCH in "${ARCH_ARRAY[@]}"; do
                    if [ "$DRY" -eq 1 ]; then
                        printf '   [dry-run] nvcc -std=%s -O3 -arch=sm_%s %s\n' "$CXX_STD" "$ARCH" "$BASE"
                        continue
                    fi
                    if nvcc -std="$CXX_STD" -O3 -arch="sm_$ARCH" "$FILE" -o "$TMP/cuda-out" 2>"$TMP/error"; then
                        printf '   OK   %-34s sm_%s\n' "$BASE" "$ARCH"
                        PASS=$((PASS + 1))
                    else
                        printf '   FAIL %-34s sm_%s\n' "$BASE" "$ARCH"
                        sed -n '1,8{s/^/        /;p;}' "$TMP/error"
                        FAIL=$((FAIL + 1))
                    fi
                done
                ;;
            *.cpp)
                if [ "$DRY" -eq 1 ]; then
                    printf '   [dry-run] g++ -std=%s -O2 %s\n' "$CXX_STD" "$BASE"
                elif g++ -std="$CXX_STD" -O2 "$FILE" -o "$TMP/cpp-out" 2>"$TMP/error"; then
                    printf '   OK   %-34s host g++\n' "$BASE"
                    PASS=$((PASS + 1))
                else
                    printf '   FAIL %-34s host g++\n' "$BASE"
                    sed -n '1,8{s/^/        /;p;}' "$TMP/error"
                    FAIL=$((FAIL + 1))
                fi
                ;;
        esac
    done
    printf '\n'
done

if [ "$DRY" -eq 1 ]; then
    printf '[dry-run] 未執行編譯。\n'
    exit 0
fi

printf '結果：成功 %d / 失敗 %d / 略過 %d\n' "$PASS" "$FAIL" "$SKIP"
if [ "$FAIL" -gt 0 ]; then
    printf '至少一個編譯失敗；修正後才能交付。\n' >&2
    exit 1
fi
printf '全部編譯成功。sm_89/sm_120 僅代表編譯驗證，尚非實機執行驗證。\n'
