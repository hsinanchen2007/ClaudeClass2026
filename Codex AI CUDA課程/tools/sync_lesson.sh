#!/usr/bin/env bash
# Codex AI CUDA 課程：兩份 Codex 自足目錄的驗證與發佈工具。

set -u

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)
COURSE_ROOT=$(dirname -- "$SCRIPT_DIR")
DEFAULT_EXPERIMENT="$HOME/AI/AI_Course/AI_CUDA/Codex AI CUDA課程"
DEFAULT_BASELINE="$HOME/AI/github/ClaudeClass2026/Codex AI CUDA課程"
EXPERIMENT_DIR="${CODEX_EXPERIMENT_DIR:-$DEFAULT_EXPERIMENT}"
BASELINE_DIR="${CODEX_BASELINE_DIR:-$DEFAULT_BASELINE}"
PUBLISH=""
DRY=0

usage() {
    cat <<'EOF'
sync_lesson.sh —— 兩份 Codex AI CUDA 課程的驗證 / 發佈工具

本工具只處理 Codex 軌：
  實驗場   ~/AI/AI_Course/AI_CUDA/Codex AI CUDA課程
  發佈基準 ~/AI/github/ClaudeClass2026/Codex AI CUDA課程

用法：
  sync_lesson.sh
  sync_lesson.sh --publish "<課別>" [--dry-run]
  sync_lesson.sh --publish all [--dry-run]

選項：
  --publish <課別>  將單一課及 PROGRESS.md 從實驗場發佈到基準
  --publish all     將整個 Codex 自足目錄鏡像到基準；會刪除基準多餘項目
  -n, --dry-run     預覽發佈，不寫檔；純驗證模式本來就是唯讀
  -h, --help        顯示本說明

環境變數：
  CODEX_EXPERIMENT_DIR  覆寫實驗場 Codex 根目錄
  CODEX_BASELINE_DIR    覆寫發佈基準 Codex 根目錄
  兩者都必須含 .codex-course-root，且不可指向同一目錄。

退出碼：
  0  兩份一致，或發佈成功且發佈後一致
  1  內容有差異或 rsync 發佈失敗
  2  用法、工具、路徑或根目錄標記錯誤

預設驗證會比較內容、symlink 與權限，忽略各課 build/ 等可重建產物。
單課發佈不會同步 tools/templates/resources；修改支援資產時應審慎使用
--publish all。這支工具永遠不掃描或寫入 Claude 軌。
EOF
}

die_usage() {
    printf '錯誤：%s\n' "$1" >&2
    exit 2
}

while [ $# -gt 0 ]; do
    case "$1" in
        --publish)
            [ $# -ge 2 ] || die_usage "--publish 需要課別或 all"
            PUBLISH=$2
            shift 2
            ;;
        -n|--dry-run) DRY=1; shift ;;
        -h|--help) usage; exit 0 ;;
        *) die_usage "未知參數 '$1'（用 --help 看說明）" ;;
    esac
done

[ "$(id -u)" -ne 0 ] || die_usage "請以一般使用者執行，不要用 sudo"
command -v rsync >/dev/null 2>&1 || die_usage "找不到 rsync"
[ -f "$COURSE_ROOT/.codex-course-root" ] || die_usage "目前工具不在有效的 Codex 課程根目錄內"
[ -d "$EXPERIMENT_DIR" ] || die_usage "實驗場不存在：$EXPERIMENT_DIR"
[ -d "$BASELINE_DIR" ] || die_usage "發佈基準不存在：$BASELINE_DIR"
[ -f "$EXPERIMENT_DIR/.codex-course-root" ] || die_usage "實驗場缺少 .codex-course-root"
[ -f "$BASELINE_DIR/.codex-course-root" ] || die_usage "發佈基準缺少 .codex-course-root"

EXPERIMENT_REAL=$(realpath -- "$EXPERIMENT_DIR") || die_usage "無法解析實驗場路徑"
BASELINE_REAL=$(realpath -- "$BASELINE_DIR") || die_usage "無法解析發佈基準路徑"
[ "$EXPERIMENT_REAL" != "$BASELINE_REAL" ] || die_usage "實驗場與發佈基準不可是同一目錄"

case "$PUBLISH" in
    ""|all) ;;
    */*|.|..) die_usage "--publish 只能指定根目錄下的單一課程名稱或 all" ;;
esac

RSYNC_EXCLUDES=(
    --exclude='build/'
    --exclude='bin/'
    --exclude='CMakeFiles/'
    --exclude='.cache/'
    --exclude='__pycache__/'
    --exclude='*.o'
    --exclude='*.obj'
    --exclude='*.fatbin'
    --exclude='*.cubin'
    --exclude='*.out'
    --exclude='CMakeCache.txt'
    --exclude='cmake_install.cmake'
    --exclude='compile_commands.json'
    --exclude='build.ninja'
    --exclude='.ninja_deps'
    --exclude='.ninja_log'
)

verify_copies() {
    local diff_file
    diff_file=$(mktemp) || return 2
    if ! rsync -rclpni --delete "${RSYNC_EXCLUDES[@]}" "$EXPERIMENT_DIR/" "$BASELINE_DIR/" >"$diff_file" 2>&1; then
        printf '錯誤：rsync 驗證失敗：\n' >&2
        sed 's/^/  /' "$diff_file" >&2
        rm -f -- "$diff_file"
        return 2
    fi

    if [ -s "$diff_file" ]; then
        printf '兩份 Codex 課程有差異：\n'
        sed 's/^/  /' "$diff_file"
        rm -f -- "$diff_file"
        return 1
    fi
    rm -f -- "$diff_file"
    printf '兩份 Codex 課程內容、symlink 與權限一致。\n'
    return 0
}

printf '實驗場  ：%s\n' "$EXPERIMENT_DIR"
printf '發佈基準：%s\n' "$BASELINE_DIR"

if [ -z "$PUBLISH" ]; then
    [ "$DRY" -eq 0 ] || printf '[dry-run] 驗證模式本來就是唯讀。\n'
    verify_copies
    exit $?
fi

if [ "$PUBLISH" = "all" ]; then
    printf '警告：將整個 Codex 自足目錄鏡像到發佈基準，並刪除基準多餘項目。\n'
    SRC="$EXPERIMENT_DIR/"
    DST="$BASELINE_DIR/"
else
    SRC_DIR="$EXPERIMENT_DIR/$PUBLISH"
    DST_DIR="$BASELINE_DIR/$PUBLISH"
    [ -d "$SRC_DIR" ] || die_usage "找不到課程：$SRC_DIR"
    SRC="$SRC_DIR/"
    DST="$DST_DIR/"
fi

if [ "$DRY" -eq 1 ]; then
    printf '[dry-run] 課程內容變更：\n'
    rsync -an --delete --itemize-changes "${RSYNC_EXCLUDES[@]}" "$SRC" "$DST" || exit 1
    if [ "$PUBLISH" != "all" ]; then
        printf '[dry-run] 另會同步：PROGRESS.md\n'
        rsync -an --itemize-changes "$EXPERIMENT_DIR/PROGRESS.md" "$BASELINE_DIR/PROGRESS.md" || exit 1
    fi
    printf '[dry-run] 未寫入任何檔案。\n'
    exit 0
fi

mkdir -p -- "$DST" || exit 1
if ! rsync -a --delete "${RSYNC_EXCLUDES[@]}" "$SRC" "$DST"; then
    printf '發佈失敗。\n' >&2
    exit 1
fi
if [ "$PUBLISH" != "all" ]; then
    rsync -a -- "$EXPERIMENT_DIR/PROGRESS.md" "$BASELINE_DIR/PROGRESS.md" || exit 1
fi

printf '發佈完成，開始驗證完整 Codex 目錄。\n'
verify_copies
