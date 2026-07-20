/*
 * 第 07 章：std::move
 *
 * std::move 不搬任何資料，只把 expression cast 成 xvalue，讓 move overload 有機會被選中。
 * 真正資源轉移發生在型別的 move constructor/assignment。moved-from 物件仍「有效但狀態
 * 未指定」（除非該型別另有保證），可安全銷毀或重新指定，不應假定一定為空。
 */

#include <algorithm>
#include <cassert>
#include <iostream>
#include <map>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

struct TrackedMessage {
    std::string body;
    bool moved_from{};

    explicit TrackedMessage(std::string text) : body(std::move(text)) {}

    TrackedMessage(TrackedMessage&& other) noexcept
        : body(std::move(other.body)) {
        other.moved_from = true;
    }

    TrackedMessage& operator=(TrackedMessage&&) = default;
    TrackedMessage(const TrackedMessage&) = default;
    TrackedMessage& operator=(const TrackedMessage&) = default;
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 49. Group Anagrams（字母異位詞分組）
// 題目：把由相同字母重排而成的字串分組；eat、tea、ate 同組，tan、nat 同組。
// 為何使用本章主題：排序副本 key 後，原 word 已不再需要，std::move 把其字串資源移入群組；
// 群組完成後再 move 到 result，避免兩輪不必要的深複製。
// 思路：為每個 word 建立排序 key；按 key 放入 map 群組；最後依 map 順序移出各群組。
// 複雜度：令 N 為字串數、K 為最大長度，時間 O(N*K*(log K+log N))、結果空間 O(N*K)。
// 易錯點：move 後 word/group 只能依有效但未指定狀態使用；輸出群組順序原題不保證，本例用 map 固定。
// -----------------------------------------------------------------------------
std::vector<std::vector<std::string>>
leetcode_group_anagrams(std::vector<std::string> words) {
    std::map<std::string, std::vector<std::string>> groups; // map 讓測試順序 deterministic
    for (std::string& word : words) {
        std::string key = word;
        std::sort(key.begin(), key.end());
        groups[key].push_back(std::move(word));
    }
    std::vector<std::vector<std::string>> result;
    result.reserve(groups.size());
    for (auto& [key, group] : groups) {
        static_cast<void>(key);
        result.push_back(std::move(group));
    }
    return result;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】Producer 將訊息移交 Outbox
// 情境：producer 建立訊息後交給 outbox 排隊，成功移交後不再依賴原訊息內容。
// 為何使用本章主題：std::move 明確把 ownership 從呼叫端傳入 by-value 參數，再移入 vector；
// 相較複製大型 body，可重用既有字串資源並讓 API 同時接受 copy 或 move。
// 設計：enqueue 接收自己的 message；push_back 將它移入容器；測試確認來源已 moved-from 且可重新指定。
// 成本：單次追加攤銷 O(1)，vector 擴容會搬移既有訊息；字串資源搬移成本依 allocator 契約。
// 上線注意：不可依賴 moved-from body 為空；併發 producer 需要鎖或專用佇列，並限制 queue 容量。
// -----------------------------------------------------------------------------
class Outbox {
public:
    void enqueue(TrackedMessage message) {
        messages_.push_back(std::move(message));
    }
    const TrackedMessage& last() const { return messages_.back(); }

private:
    std::vector<TrackedMessage> messages_;
};

void practical_outbox_test() {
    Outbox outbox;
    TrackedMessage message("build finished");
    outbox.enqueue(std::move(message));
    assert(message.moved_from);
    assert(outbox.last().body == "build finished");

    message = TrackedMessage("reused"); // moved-from 物件可重新指定
    assert(message.body == "reused");
}

int main() {
    TrackedMessage source("payload");
    TrackedMessage destination(std::move(source));
    assert(source.moved_from);
    assert(destination.body == "payload");

    const auto groups = leetcode_group_anagrams({"eat", "tea", "tan", "ate", "nat", "bat"});
    assert(groups.size() == 3U);
    assert((groups[0] == std::vector<std::string>{"bat"}));
    assert((groups[1] == std::vector<std::string>{"eat", "tea", "ate"}));

    practical_outbox_test();
    std::cout << "move 測試完成\n";
}

/*
 * 【常見陷阱】
 * - `std::move(const T)` 得到 const T&&，多數 move constructor 要 T&&，最後反而呼叫 copy。
 * - return local 時通常不要 `return std::move(local)`；它可能阻礙 NRVO，編譯器本會自動 move。
 * - vector reallocation 偏好 noexcept move；若 move 可能丟例外且 T 可 copy，可能改用 copy。
 * - 搬移後立刻假設 string.empty() 不可攜；只依標準/型別文件保證。
 *
 * 【面試段落】rvalue reference 與 move semantics 的關係？reference 只是分類/綁定工具，
 * move constructor 才定義資源如何轉移。
 * 【練習】用 exchange 寫 TrackedMessage move constructor，明確設定來源狀態。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '07_move.cpp' -o '/tmp/codex_cpp_C_Utility_07_move' && '/tmp/codex_cpp_C_Utility_07_move'
//
// === 預期輸出（節錄）===
// move 測試完成
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
