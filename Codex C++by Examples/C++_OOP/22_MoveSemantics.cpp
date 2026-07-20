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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 49. Group Anagrams（字母異位詞分組）
// 題目：將字母組成相同的字串分組；例如 eat、tea、ate 同組，tan、nat 同組。
// 為何使用本章主題：排序 signature 是解題核心；本例按值取得 words，再 move 字串與群組以避免結果組裝時重複 copy。
// 思路：複製 word 作排序 key；將原 word move 進對應 map group；最後把每個 group vector move 到 result。
// 複雜度：時間 O(N*K log K + N log G)、額外空間 O(N*K)，G 為群組數、K 為最長字長。
// 易錯點：move 後不可讀原 word 值；群組順序由 map key 決定但題目不要求；return local 不需 std::move。
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// 【日常實務範例】同時接受可重用訊息與一次性訊息的工作佇列
// 情境：caller 有時提交仍需保留的 reusable Message，有時提交 temporary；queue 都要取得自己的 payload。
// 為何使用本章主題：submit 按值統一 copy/move 入口，再 move 進 vector；lvalue copy，rvalue 可直接轉移資源。
// 設計：參數建立獨立 Message；push_back(std::move(message)) 交給 jobs_；front 只借出 const reference。
// 成本：lvalue 路徑含 O(P) copy，rvalue 通常 O(1) move；vector 成長可能搬移既有 J 筆。
// 上線注意：front 在空 queue 或 vector 修改後不可用；Message move 應 noexcept，並行 submit/read 需同步。
// -----------------------------------------------------------------------------
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

/*
【本課面試問答】
Q1：`std::move` 會搬資料嗎？
A：不會；它本質是把 expression cast 成可供 move overload 選擇的 xvalue。真正做 copy、交換 pointer
或線性搬移的是被選到的 constructor/assignment；若型別沒有 move operation，仍可能 copy。

Q2：為何 move constructor 常標 `noexcept`？
A：`vector` reallocation 需要在失敗時保住原元素。若 move 可能丟例外而 copy 可用，實作常選 copy
以維持 strong guarantee；`noexcept` 可讓它安全選 move。但這不是「任何情況都一定 move」的保證。

Q3：moved-from object 可做什麼？
A：除非型別另有更強契約，標準庫物件通常只保證 valid but unspecified：可解構、重新指定，也可呼叫
不依賴特定值的操作。不要假設一定 empty/zero；若程式需要該狀態，就自行明訂並測試類別契約。
*/

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '22_MoveSemantics.cpp' -o '/tmp/codex_cpp_C_OOP_22_MoveSemantics' && '/tmp/codex_cpp_C_OOP_22_MoveSemantics'
//
// === 預期輸出（節錄）===
// [實務] queue 同時接受 lvalue copy 與 rvalue move
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
