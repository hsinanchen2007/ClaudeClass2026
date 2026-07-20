// ============================================================================
// 課題 7：Implicit conversion、explicit 與 narrowing
// ============================================================================
//
// implicit conversion 讓 assignment/function call/operator 自動轉型，便利但可能隱藏
// precision loss、意外 constructor 或 overload 選擇。單參數 constructor 通常標
// explicit，要求呼叫端明寫 `Port{8080}`；conversion operator（尤其 operator bool）也
// 通常 explicit。
//
// brace initialization 會拒絕 narrowing，例如 `int x{3.5}` 編譯失敗。這比 `int x=3.5`
// 靜默截斷安全。integer promotions/usual arithmetic conversions 也可能把 signed/unsigned
// 混成 unexpected unsigned，嚴格 warning 很有價值。
// ============================================================================

#include <cassert>
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

class Port {
public:
    explicit Port(int value) : value_(value)
    {
        if (value < 1 || value > 65'535) throw std::out_of_range("port");
    }
    int value() const { return value_; }
private:
    int value_;
};

void connect([[maybe_unused]] Port port)
{
    assert(port.value() == 443);
}

void basic_example()
{
    connect(Port{443}); // connect(443) 因 explicit 而不能編譯，避免單位/型別誤傳。
    [[maybe_unused]] const int truncated = static_cast<int>(3.9); // 明寫政策，而非 implicit narrowing。
    assert(truncated == 3);
    std::cout << "[基礎] explicit Port and visible numeric truncation\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1656. Design an Ordered Stream（設計有序資料流）
// 題目：依 id 插入字串，只有從目前 ptr 起連續就緒時才回傳該段；先插 2，再插 1 時回 [aa,bb]。
// 為何使用本章主題：explicit constructor 防止 int 在其他 API 呼叫中意外轉成配置 N+1 格的 stateful OrderedStream。
// 思路：1. 配置 1-based slots；2. insert 將 value 寫入 id；3. 從 next_ 連續收集非空值並前進。
// 複雜度：N 為 slot 數；儲存 O(N)，單次 insert 加輸出 K 筆為 O(K)，所有插入總走訪 O(N)。
// 易錯點：id 要驗 1..N；空字串被當作尚未就緒 sentinel，若題目允許空 payload 就需獨立 presence 狀態。
// -----------------------------------------------------------------------------
class OrderedStream {
public:
    explicit OrderedStream(int count) : values_(checked_size(count)), next_(1U) {}

    std::vector<std::string> insert(int id, std::string value)
    {
        if (id <= 0 || static_cast<std::size_t>(id) >= values_.size()) {
            throw std::out_of_range("stream id");
        }
        values_.at(static_cast<std::size_t>(id)) = std::move(value);
        std::vector<std::string> output;
        while (next_ < values_.size() && !values_.at(next_).empty()) {
            output.push_back(values_.at(next_++));
        }
        return output;
    }

private:
    static std::size_t checked_size(int count)
    {
        if (count <= 0) throw std::invalid_argument("count must be positive");
        return static_cast<std::size_t>(count) + 1U;
    }
    std::vector<std::string> values_;
    std::size_t next_;
};

void leetcode_1656_example()
{
    OrderedStream stream(3);
    // insert 會寫入 slot 並推進 next_；NDEBUG 不可把必要操作連同 assert 一起移除。
    [[maybe_unused]] const auto waiting = stream.insert(2, "bb");
    assert(waiting.empty());
    [[maybe_unused]] const auto ready = stream.insert(1, "aa");
    assert((ready == std::vector<std::string>{"aa", "bb"}));
    std::cout << "[LeetCode 1656] explicit stateful constructor, ordered chunk aa/bb\n";
}

// -----------------------------------------------------------------------------
// 【日常實務範例】使用者 ID 強型別路徑
// 情境：user ID 與 job ID 底層都可能是整數，但 `/users/...` API 只能接受使用者 ID。
// 為何使用本章主題：explicit UserId constructor 阻止 unsigned long 自動轉換，要求呼叫點明寫領域型別。
// 設計：1. UserId 私有保存數值；2. explicit 建構；3. user_path 只接受 UserId 並格式化路徑。
// 成本：建立 ID O(1)，輸出字串依十進位位數 D 為 O(D) 時間與空間。
// 上線注意：strong type 仍要驗 ID 範圍與不存在值；不要用 `using UserId=unsigned long`，那不會建立型別隔離。
// -----------------------------------------------------------------------------
class UserId {
public:
    explicit UserId(unsigned long value) : value_(value) {}
    unsigned long value() const { return value_; }
private:
    unsigned long value_;
};

std::string user_path(UserId id) { return "/users/" + std::to_string(id.value()); }

void practical_example()
{
    assert(user_path(UserId{42UL}) == "/users/42");
    std::cout << "[實務] strong UserId path=/users/42\n";
}

int main()
{
    basic_example();
    leetcode_1656_example();
    practical_example();
}

// 易錯與面試：單參數 constructor 若未 explicit，可能在 overload resolution 中偷偷建立
// temporary。Strong type 必須真的建立新 class；`using UserId=int` 只是 alias，防不了誤傳。
// 練習：新增 JobId；證明 user_path(JobId{42}) 在 compile time 被拒絕。
// 複雜度：built-in conversion 常是 O(1)，class conversion 則包含 constructor/operator 的成本。
// 生命週期：implicit conversion 可能建立只活到 full-expression 結尾的 temporary，勿保存其 reference。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '07_implicit_explicit.cpp' -o '/tmp/codex_cpp_C_Cast_07_implicit_explicit' && '/tmp/codex_cpp_C_Cast_07_implicit_explicit'
//
// === 預期輸出（節錄）===
// [實務] strong UserId path=/users/42
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
