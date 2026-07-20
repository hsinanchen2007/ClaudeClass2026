/*
 * std::string::operator=：最常用的整體指派
 *
 * 【基本模型】operator= 取代整份內容，不是附加；成功後 target 代表右側提供的新值。
 * 【overload】常見來源含 string 複製/移動、C 字串、單一 char 與 initializer_list。
 * 【overload】C++17 起亦接受符合條件且可轉成 string_view 的來源，內容會在呼叫期間複製。
 * 【選型】整體取代優先用 =；需要 substring、count 或 iterator range 時 assign 更精確。
 * 【複雜度】複製通常與來源長度線性相關；move 只有 allocator 契約允許時才可能接管 buffer。
 * 【生命週期】copy 後兩字串各自擁有內容；move 後來源仍有效，但其值未指定。
 * 【失效】指派可能重用或更換儲存區，所有舊 iterator/reference/pointer 都應重新取得。
 * 【例外安全】配置可能丟 bad_alloc，超過 max_size 可能丟 length_error；呼叫端不可假設 noexcept。
 * 【易錯】從 const char* 指派要求有效且以 NUL 結尾的範圍；空指標不是空字串。
 */

#include <cassert>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>

namespace {

void basic_demo() {
    std::string value;
    value = "ready";
    assert(value == "ready");
    value = 'X';
    assert(value == "X");
    value = {'O', 'K'};
    assert(value == "OK");

    const std::string copied = "copied";
    value = copied;
    assert(value == copied);
    const std::string& alias = value;
    value = alias;  // 自我複製指派仍須保持原值。
    assert(value == "copied");

    const std::string_view view = "view content";
    value = view;
    assert(value == "view content");

    std::string source = "large response";
    value = std::move(source);
    assert(value == "large response");
    source = "reused";  // 移動後不要假設舊內容；直接賦予新值即可安全重用。
    assert(source == "reused");
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1108. Defanging an IP Address（IP 位址去敏化）
// 題目：把合法 IPv4 字串中的每個 `.` 替換成 `[.]`；例如 1.1.1.1 得 1[.]1[.]1[.]1。
// 為何使用本章主題：題解主要以 += 建立新答案，並未使用 operator= 做整體替換；放在本檔
//       是對照「建構新值」與下方狀態物件以 = 保存最新值的不同角色。
// 思路：1. 預留原長度加六；2. 逐字元掃描；3. 點附加 `[.]`，其他字元原樣附加。
// 複雜度：時間 O(N)、額外空間 O(N)，N 是 address 長度，IPv4 題目固定三個點。
// 易錯點：reserve 的 +6 依賴原題三點契約；一般輸入要驗 IPv4，不能只做文字替換。
// -----------------------------------------------------------------------------
std::string leetcode_defang_ip(const std::string& address) {
    std::string answer;
    answer.reserve(address.size() + 6U);
    for (const char ch : address) {
        if (ch == '.') {
            answer += "[.]";
        } else {
            answer += ch;
        }
    }
    return answer;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】工作狀態最新錯誤保存
// 情境：工作失敗時保存一份最新錯誤訊息，下一次成功則清除舊錯誤，查詢者只看目前狀態。
// 為何使用本章主題：operator= 表達整體取代，fail 複製 caller 訊息取得 ownership，succeed
//       以空字串重設；相較保存 string_view，不會依賴外部訊息生命週期。
// 設計：1. fail 時把 message 指派給 member；2. succeed 指派空字串；3. accessor 回 const reference。
// 成本：fail 時間/空間 O(M)，M 是訊息長度；清空通常 O(1)，但不保證釋放容量。
// 上線注意：空字串若也是合法錯誤就需 optional/獨立狀態；多執行緒讀寫需同步，accessor reference
//       不能活過物件或下一次修改。
// -----------------------------------------------------------------------------
class JobStatus {
public:
    void fail(const std::string& message) { last_error_ = message; }
    void succeed() { last_error_ = ""; }
    [[nodiscard]] const std::string& error() const { return last_error_; }

private:
    std::string last_error_;
};

void practical_job_status_lifecycle() {
    JobStatus status;
    status.fail("timeout");
    assert(status.error() == "timeout");
    status.succeed();
    assert(status.error().empty());
}

}  // namespace

int main() {
    basic_demo();
    assert(leetcode_defang_ip("1.1.1.1") == "1[.]1[.]1[.]1");
    assert(leetcode_defang_ip("255.100.50.0") == "255[.]100[.]50[.]0");

    practical_job_status_lifecycle();
    std::cout << "operator=: tests passed\n";
}

/*
 * 【LeetCode 契約】IPv4 題目保證三個點；每個點替換成 "[.]"，時間與輸出空間皆 O(n)。
 * 【實務契約】JobStatus::fail 保存 message 的副本；succeed 以空字串表示沒有錯誤。
 * 【實務易錯】若空字串也可能是合法錯誤訊息，就應另存 bool/optional，不能混用 sentinel。
 * 【交易邊界】連續指派多個欄位不是原子操作；跨欄位 invariant 應先建立 candidate 再提交。
 * 【陷阱】`p = text.c_str(); text = ...;` 後 p 可能失效；= 不會延長外部指標生命週期。
 * 【面試追問】自我 copy assignment 必須如何？結果保持原值且不能破壞資源管理。
 * 【面試追問】move assignment 一定 O(1) 嗎？不一定；allocator 不相容可迫使元素搬移。
 * 【練習】為 JobStatus 增加 `set(bool ok, std::string message)`，成功時忽略 message。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'operator_assign.cpp' -o '/tmp/codex_cpp_C_String_operator_assign' && '/tmp/codex_cpp_C_String_operator_assign'
//
// === 預期輸出（節錄）===
// operator=: tests passed
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
