#!/usr/bin/env bash
# Codex AI CUDA 課程：單課完整交付流程。

set -u

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)
COURSE_ROOT=$(dirname -- "$SCRIPT_DIR")
EXPERIMENT_DIR="${CODEX_EXPERIMENT_DIR:-$HOME/AI/AI_Course/AI_CUDA/Codex AI CUDA課程}"
BASELINE_DIR="${CODEX_BASELINE_DIR:-$HOME/AI/github/ClaudeClass2026/Codex AI CUDA課程}"
LESSON=""
MESSAGE=""
DRY=0
PUSH=1

usage() {
    cat <<'EOF'
finish_lesson.sh —— Codex 單課完整交付流程

流程：
  1. 編譯該課全部 .cu/.cpp（sm_75、sm_89、sm_120）
  2. 將該課與 Codex PROGRESS.md 發佈到基準副本
  3. 驗證兩份完整 Codex 目錄一致
  4. 在兩個 repo 中只 stage/commit 該課與 Codex PROGRESS.md
  5. 對有 origin 的 repo push，並驗證本地 HEAD 等於 origin/<branch>

用法：
  finish_lesson.sh --lesson "<課別>" --message "<commit 訊息>"

選項：
  --lesson <課別>       必填；Codex 根目錄下的單一課程目錄
  -m, --message <訊息>  正式執行時必填
  -n, --dry-run         真跑編譯，但只預覽同步；不 commit、不 push
  --no-push             commit 後不 push
  -h, --help            顯示本說明

環境變數：
  CODEX_EXPERIMENT_DIR  覆寫實驗場 Codex 根目錄
  CODEX_BASELINE_DIR    覆寫發佈基準 Codex 根目錄

退出碼：
  0  所選流程全部成功
  1  編譯、同步、Git 或 push 失敗
  2  用法、路徑、工具或安全前置條件錯誤

安全邊界：
  - 只處理 Codex 軌，不會 stage Claude 軌或其他未相關檔案。
  - 若任一 repo 已有 staged 變更會先中止，避免混入使用者的 commit。
  - sm_89/sm_120 在本機只做編譯驗證，不等於實機執行驗證。
EOF
}

die_usage() {
    printf '錯誤：%s\n' "$1" >&2
    exit 2
}

fail() {
    printf '\n交付失敗：%s\n' "$1" >&2
    exit 1
}

step() {
    printf '\n===== %s =====\n' "$1"
}

while [ $# -gt 0 ]; do
    case "$1" in
        --lesson)
            [ $# -ge 2 ] || die_usage "--lesson 需要參數"
            LESSON=$2
            shift 2
            ;;
        -m|--message)
            [ $# -ge 2 ] || die_usage "--message 需要參數"
            MESSAGE=$2
            shift 2
            ;;
        -n|--dry-run) DRY=1; shift ;;
        --no-push) PUSH=0; shift ;;
        -h|--help) usage; exit 0 ;;
        *) die_usage "未知參數 '$1'（用 --help 看說明）" ;;
    esac
done

[ "$(id -u)" -ne 0 ] || die_usage "請以一般使用者執行，不要用 sudo"
[ -f "$COURSE_ROOT/.codex-course-root" ] || die_usage "工具不在有效的 Codex 課程根目錄內"
[ -n "$LESSON" ] || die_usage "必須指定 --lesson"
case "$LESSON" in
    */*|.|..) die_usage "--lesson 只能是根目錄下的單一課程名稱" ;;
esac
if [ "$DRY" -eq 0 ]; then
    [ -n "$MESSAGE" ] || die_usage "正式執行必須指定 --message"
fi
[ -d "$EXPERIMENT_DIR/$LESSON" ] || die_usage "實驗場找不到課程：$EXPERIMENT_DIR/$LESSON"
[ -f "$EXPERIMENT_DIR/.codex-course-root" ] || die_usage "實驗場不是有效 Codex 根目錄"
[ -f "$BASELINE_DIR/.codex-course-root" ] || die_usage "發佈基準不是有效 Codex 根目錄"

repo_for_course() {
    git -C "$1" rev-parse --show-toplevel 2>/dev/null
}

relative_course_path() {
    local repo=$1 course=$2 relative
    relative=$(realpath --relative-to="$repo" -- "$course") || return 1
    case "$relative" in
        ..|../*) return 1 ;;
    esac
    printf '%s\n' "$relative"
}

EXPERIMENT_REPO=$(repo_for_course "$EXPERIMENT_DIR") || die_usage "實驗場不在 Git repo：$EXPERIMENT_DIR"
BASELINE_REPO=$(repo_for_course "$BASELINE_DIR") || die_usage "發佈基準不在 Git repo：$BASELINE_DIR"
EXPERIMENT_REL=$(relative_course_path "$EXPERIMENT_REPO" "$EXPERIMENT_DIR") || die_usage "無法取得實驗場的 repo 相對路徑"
BASELINE_REL=$(relative_course_path "$BASELINE_REPO" "$BASELINE_DIR") || die_usage "無法取得發佈基準的 repo 相對路徑"

if [ "$DRY" -eq 0 ]; then
    for REPO in "$EXPERIMENT_REPO" "$BASELINE_REPO"; do
        git -C "$REPO" diff --cached --quiet || die_usage "repo 已有 staged 變更，請先處理：$REPO"
    done
fi

step "1/5 編譯稽核"
CODEX_COURSE_ROOT="$EXPERIMENT_DIR" "$COURSE_ROOT/tools/check_build.sh" --lesson "$LESSON" ||
    fail "編譯稽核未通過"

step "2/5 發佈課程與進度"
if [ "$DRY" -eq 1 ]; then
    CODEX_EXPERIMENT_DIR="$EXPERIMENT_DIR" CODEX_BASELINE_DIR="$BASELINE_DIR" "$COURSE_ROOT/tools/sync_lesson.sh" --publish "$LESSON" --dry-run ||
        fail "同步 dry-run 失敗"
    printf '[dry-run] 因未寫入，略過一致性、commit 與 push。\n'
    exit 0
fi

CODEX_EXPERIMENT_DIR="$EXPERIMENT_DIR" CODEX_BASELINE_DIR="$BASELINE_DIR" "$COURSE_ROOT/tools/sync_lesson.sh" --publish "$LESSON" ||
    fail "發佈失敗"

step "3/5 驗證完整 Codex 目錄"
CODEX_EXPERIMENT_DIR="$EXPERIMENT_DIR" CODEX_BASELINE_DIR="$BASELINE_DIR" "$COURSE_ROOT/tools/sync_lesson.sh" ||
    fail "兩份 Codex 目錄不一致"

commit_course_scope() {
    local repo=$1 relative=$2 label=$3
    local specs=("$relative/$LESSON" "$relative/PROGRESS.md")

    if [ -z "$(git -C "$repo" status --porcelain=v1 -- "${specs[@]}")" ]; then
        printf '  %s：指定範圍沒有變更，略過 commit。\n' "$label"
        return 0
    fi

    git -C "$repo" add -- "${specs[@]}" || return 1
    if git -C "$repo" diff --cached --quiet; then
        printf '  %s：沒有可提交差異。\n' "$label"
        return 0
    fi
    git -C "$repo" commit -m "$MESSAGE" || return 1
    printf '  %s：%s\n' "$label" "$(git -C "$repo" log -1 --oneline)"
}

step "4/5 範圍限定 commit"
commit_course_scope "$EXPERIMENT_REPO" "$EXPERIMENT_REL" "實驗場" ||
    fail "實驗場 commit 失敗"
commit_course_scope "$BASELINE_REPO" "$BASELINE_REL" "發佈基準" ||
    fail "發佈基準 commit 失敗"

step "5/5 Push 與遠端驗證"
if [ "$PUSH" -eq 0 ]; then
    printf -- '--no-push：已完成 commit，未推送。\n'
    exit 0
fi

push_and_verify() {
    local repo=$1 label=$2 branch local_sha remote_sha output
    if [ -z "$(git -C "$repo" remote)" ]; then
        printf '  %s：沒有 remote，略過 push。\n' "$label"
        return 0
    fi
    git -C "$repo" remote get-url origin >/dev/null 2>&1 || {
        printf '  %s：沒有 origin，略過 push。\n' "$label"
        return 0
    }
    branch=$(git -C "$repo" symbolic-ref --quiet --short HEAD) || return 1
    if ! output=$(git -C "$repo" push origin "$branch" 2>&1); then
        printf '%s\n' "$output" | sed 's/^/    /' >&2
        return 1
    fi
    [ -z "$output" ] || printf '%s\n' "$output" | sed 's/^/    /'
    git -C "$repo" fetch --quiet origin "$branch" || return 1
    local_sha=$(git -C "$repo" rev-parse HEAD) || return 1
    remote_sha=$(git -C "$repo" rev-parse "origin/$branch") || return 1
    [ "$local_sha" = "$remote_sha" ] || {
        printf '  %s：本地 %s，遠端 %s。\n' "$label" "${local_sha:0:12}" "${remote_sha:0:12}" >&2
        return 1
    }
    printf '  %s：遠端已同步（%s）。\n' "$label" "${local_sha:0:12}"
}

push_and_verify "$EXPERIMENT_REPO" "實驗場" || fail "實驗場 push/驗證失敗"
push_and_verify "$BASELINE_REPO" "發佈基準" || fail "發佈基準 push/驗證失敗"

printf '\n交付完成：%s\n' "$LESSON"
