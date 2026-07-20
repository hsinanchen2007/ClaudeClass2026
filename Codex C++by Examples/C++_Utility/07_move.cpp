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

// LeetCode 49：Group Anagrams。完成 key 分類後，把字串 move 進群組，避免一次額外複製。
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

// 【實務情境】訊息 ownership 從 producer 明確轉交給 outbox。
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
