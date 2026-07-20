/*
 * 第 09 章：std::exchange
 *
 * `old = exchange(object, new_value)` 會把 object 的舊值搬出，再指定新值，最後回傳舊值。
 * 它非常適合狀態機、move constructor 清空來源、一次性 token 與 pointer rewiring。
 * exchange 不是原子操作；多執行緒共享資料仍需要 atomic::exchange 或 mutex。
 */

#include <cassert>
#include <iostream>
#include <string>
#include <utility>

struct Node {
    int value{};
    Node* next{};
};

// LeetCode 206：Reverse Linked List。
// exchange 先把 current->next 換成 previous，並把舊 next 回傳給 current。
Node* leetcode_reverse_list(Node* head) {
    Node* previous = nullptr;
    Node* current = head;
    while (current != nullptr) {
        // 先把 current->next 改指 previous；exchange 同時回傳尚未反轉的 next。
        Node* next = std::exchange(current->next, previous);
        previous = current;
        current = next;
    }
    return previous;
}

enum class JobState { queued, running, succeeded, failed };

std::string state_name(JobState state) {
    switch (state) {
        case JobState::queued: return "queued";
        case JobState::running: return "running";
        case JobState::succeeded: return "succeeded";
        case JobState::failed: return "failed";
    }
    return "unknown";
}

struct Job {
    std::string id;
    JobState state{JobState::queued};
};

// 實務：回傳舊狀態便於寫 audit log，同時完成狀態更新。
JobState practical_transition(Job& job, JobState next) {
    return std::exchange(job.state, next);
}

class FileHandle {
public:
    explicit FileHandle(int descriptor = -1) : descriptor_(descriptor) {}

    FileHandle(FileHandle&& other) noexcept
        : descriptor_(std::exchange(other.descriptor_, -1)) {}

    int get() const noexcept { return descriptor_; }

private:
    int descriptor_;
};

int main() {
    int value = 10;
    const int old = std::exchange(value, 20);
    assert(old == 10 && value == 20);

    Node third{3, nullptr};
    Node second{2, &third};
    Node first{1, &second};
    Node* reversed = leetcode_reverse_list(&first);
    assert(reversed == &third);
    assert(reversed->next == &second && second.next == &first && first.next == nullptr);

    Job job{"build-7"};
    const JobState before = practical_transition(job, JobState::running);
    assert(before == JobState::queued);
    assert(job.state == JobState::running);
    assert(state_name(before) == "queued");

    FileHandle original(9);
    FileHandle moved(std::move(original));
    assert(original.get() == -1 && moved.get() == 9);

    std::cout << "exchange 測試完成\n";
}

/*
 * 【常見陷阱】
 * - `exchange(x,y)` 會指定 x，x 必須可指定；回傳舊值可能做 move/copy。
 * - 它沒有同步保證；shared state 用 atomic.exchange(memory_order) 或鎖。
 * - pointer rewiring 的求值順序要逐步畫圖，不要為了短而寫成難懂的一行。
 * - RAII handle 的 move assignment 還要先釋放自身舊 handle，本例只展示 move constructor。
 *
 * 【面試段落】exchange 與 swap：exchange 用新值替換一方並回舊值；swap 對稱交換兩方。
 * 【練習】完成 FileHandle destructor/move assignment，使用假的 close counter 驗證不重複關閉。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '09_exchange.cpp' -o '/tmp/codex_cpp_C_Utility_09_exchange' && '/tmp/codex_cpp_C_Utility_09_exchange'
//
// === 預期輸出（節錄）===
// exchange 測試完成
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
