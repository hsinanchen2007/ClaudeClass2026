// ============================================================================
// 課題 22：Move semantics、rvalue reference 與 moved-from state
// ============================================================================
//
// copy 複製資源；move 把可轉移的內部資源從即將不用的 object 搬給新 owner。`T&&` 可
// 綁 rvalue；std::move 本身不搬任何東西，只把 expression cast 成可被 move 的 xvalue，
// 真正行為由 move constructor/assignment 決定。
//
// moved-from object 必須「valid but unspecified」：可解構、可賦新值，但除非型別另有
// 保證，不可假設 string/vector 一定 empty。const object 通常無法 move resource，因
// move operation 需要修改來源，最後常退回 copy。
//
// 【Rule】move operation 應盡量 noexcept，vector reallocation 才敢 move elements；否則
// 為 strong exception guarantee 可能改 copy。
// 【陷阱】return local 時通常不要 std::move，會阻礙 NRVO；直接 `return value;`。
// ============================================================================

#include <algorithm>
#include <cassert>
#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <vector>

class Message {
public:
    explicit Message(std::string payload) : payload_(std::move(payload)) {}
    Message(const Message&) = default;
    Message& operator=(const Message&) = default;
    Message(Message&&) noexcept = default;
    Message& operator=(Message&&) noexcept = default;
    const std::string& payload() const { return payload_; }

private:
    std::string payload_;
};

void basic_example()
{
    Message source("large payload");
    Message destination(std::move(source));
    assert(destination.payload() == "large payload");
    source = Message("reused"); // moved-from object 可重新 assignment。
    assert(source.payload() == "reused");
    std::cout << "[基礎] resource move 後來源重新賦值使用\n";
}

// LeetCode 49：Group Anagrams。
// 對每個 word 建排序 key；結果組裝時把 map 裡的 vector move 到 result，避免再 copy
// 每一個字串。演算法核心仍是 sorted-key grouping。
std::vector<std::vector<std::string>> group_anagrams(std::vector<std::string> words)
{
    std::map<std::string, std::vector<std::string>> groups;
    for (std::string& word : words) {
        std::string key = word;
        std::sort(key.begin(), key.end());
        groups[key].push_back(std::move(word));
    }
    std::vector<std::vector<std::string>> result;
    result.reserve(groups.size());
    for (auto& entry : groups) {
        result.push_back(std::move(entry.second));
    }
    return result; // NRVO/move，自行寫 std::move(result) 反而沒必要。
}

void leetcode_49_example()
{
    const auto groups = group_anagrams({"eat", "tea", "tan", "ate", "nat", "bat"});
    assert(groups.size() == 3U);
    std::size_t words = 0U;
    for (const auto& group : groups) words += group.size();
    assert(words == 6U);
    std::cout << "[LeetCode 49] 6 words move into 3 anagram groups\n";
}

// 實務案例：JobQueue::submit by value，可同時接受 copy 與 move；內部永遠 move 儲存。
class JobQueue {
public:
    void submit(Message message) { jobs_.push_back(std::move(message)); }
    const Message& front() const { return jobs_.front(); }
private:
    std::vector<Message> jobs_;
};

void practical_example()
{
    JobQueue queue;
    Message reusable("compile kernel");
    queue.submit(reusable);            // lvalue 先 copy 進參數。
    queue.submit(Message("run tests")); // temporary 可直接 move。
    assert(queue.front().payload() == "compile kernel");
    assert(reusable.payload() == "compile kernel");
    std::cout << "[實務] queue 同時接受 lvalue copy 與 rvalue move\n";
}

int main()
{
    basic_example();
    leetcode_49_example();
    practical_example();
}

// 練習：在 Message 的 copy/move constructors 加 counter，觀察 vector reserve 的差異。
// 複雜度：vector/string move 通常 O(1) 接手 buffer，但 allocator 不相容等情況可能線性。
// 生命週期：moved-from object 仍存活且可解構/指定，只保證 valid，不保證原值或一定 empty。
