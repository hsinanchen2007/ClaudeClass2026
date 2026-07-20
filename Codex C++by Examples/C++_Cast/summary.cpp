// ============================================================================
// C++ 型別轉換總複習：先證明前置條件，再選最窄的 cast
// ============================================================================
//
// 【本章地圖：對應 01～08】
//   overview -> static_cast -> dynamic_cast -> const_cast -> reinterpret_cast
//   -> C++20 bit_cast -> implicit/explicit -> pitfalls
//
// 【選型表】
//   需求                                  工具                         檢查時機/失敗
//   數值轉換、enum、void* 回原型別、upcast  static_cast                  compile time；窄化仍需自行驗值域
//   polymorphic base -> derived            dynamic_cast<T*>             runtime；失敗回 nullptr
//   polymorphic base -> derived reference  dynamic_cast<T&>             runtime；失敗丟 std::bad_cast
//   只增減 cv qualification                const_cast                    無 runtime 保護；原物件真 const 時寫入 UB
//   指標/位址/ABI low-level reinterpret    reinterpret_cast              只改解讀方式；不保證可 dereference
//   同大小物件表示（trivially copyable）   std::bit_cast<To>(from)       compile-time constraints
//   建構轉換需防止意外發生                  explicit constructor/operator 呼叫端必須明示
//
// 【重要規則】
//   1. cast 不是 range check。double -> int 超出可表示範圍會出問題；先驗證再轉。
//   2. signed/unsigned 比較會做 usual arithmetic conversions；負 int 可能先變巨大 unsigned。
//   3. base -> derived 的 static_cast 只在你已由其他 invariant 證明 dynamic type 時合理；
//      一般 plugin/event hierarchy 優先 dynamic_cast 或更好的 virtual dispatch。
//   4. dynamic_cast 需要 polymorphic base（至少一個 virtual function，通常 virtual destructor）。
//   5. `const_cast` 只能修正 API 的 cv 介面；若 storage 本身是 const，寫它就是 UB。
//   6. `reinterpret_cast` 不解決 alignment、object lifetime、strict aliasing 或 endian。
//   7. object representation 複製可用 memcpy/bit_cast；不要用不相容指標解參照 type-pun。
//   8. C-style cast 可能依序嘗試 const/static/reinterpret 組合，review 看不出你的意圖。
//
// 【生命週期與指標陷阱】
//   - cast 後的 pointer/reference 不延長 object lifetime。
//   - `std::bit_cast` 回傳新 value，不產生指向來源 bytes 的 alias。
//   - integer -> pointer -> integer 只有實作/平台條件保證；不要當跨程序持久 ID。
//   - pointer arithmetic 僅在同一 array object（含 one-past）內有定義。
//
// 【面試快問快答】
//   Q: static_cast 與 dynamic_cast 的 upcast 有何不同？
//   A: 合法 public base upcast 都可 compile-time 完成；static_cast 足夠，dynamic_cast 多餘。
//   Q: 為何 `dynamic_cast<void*>` 有用？
//   A: 從 polymorphic subobject 取得 most-derived object 位址，常見於低階框架診斷。
//   Q: bit_cast 能把 float 安全轉成 int 數值嗎？
//   A: 它保留 bits，不保留數值；1.0F 的 bits 不是整數 1。
//   Q: 何時完全不該 cast？
//   A: cast 只是掩蓋 API 型別錯誤、ownership 錯誤或 warning 時，先修設計。
// ============================================================================

/*
==============================================================================
【面試深挖：C++ Casts】

CA1｜四種 named cast 的職責？
答：static_cast 做已知編譯期轉換；dynamic_cast 做 polymorphic runtime checked conversion；
const_cast 改 cv qualification；reinterpret_cast 做低階表示/指標類轉換。名字讓意圖可 review。

CA2｜dynamic_cast 需要什麼？失敗怎麼表示？
答：涉及 runtime down/cross cast 時 source class 必須 polymorphic。轉 pointer 失敗回 nullptr；
轉 reference 失敗丟 std::bad_cast。轉 `void*` 可取得 most-derived object address。

CA3｜static_cast downcast 為何危險？
答：編譯器只檢查 hierarchy 關係，不驗 dynamic type；若 object 實際不是目標 derived，
後續使用造成 UB。只有由外部 invariant 已證明型別時才可用，否則 virtual/dynamic_cast/variant。

CA4｜const_cast 後寫入一定合法嗎？
答：只有原 object 實際非 const，只是經 const view 看到時才可寫；若 object 本來宣告 const，
移除 const 後修改是 UB。const_cast 不會把唯讀 storage 變可寫。

CA5｜reinterpret_cast 是否等同 bit reinterpretation？
答：不是。它可轉 pointer/integer 等，但不自動建立目標型別 object，也不繞過 strict aliasing、
alignment、lifetime。等大小 trivially copyable value representation 用 bit_cast/memcpy。

CA6｜C-style cast 為何不推薦？
答：它會依規則嘗試 const/static/reinterpret 等組合，reviewer 看不出危險程度，甚至可同時
去 const 與重解釋。named cast 限縮能力，也利於搜尋與靜態分析。

CA7｜`std::bit_cast` 的前置條件與用途？
答：來源/目標大小相同且 trivially copyable；按 object representation 產生目標 value，
常用 float bits、protocol fields。padding/indeterminate bits 與目標值合法性仍需注意。

CA8｜numeric static_cast 如何處理 narrowing？
答：它明確允許許多 narrowing；超範圍 signed/float-to-int 有精確但容易誤用的規則，
不能當 runtime range check。先驗範圍，或用 checked conversion utility。

CA9｜dynamic_cast 的成本可以簡化成 O(1) 嗎？
答：標準不給固定複雜度；成本依 ABI、hierarchy、多重繼承與 cast 方向。先確保設計正確，
hot path 再 profile；不要用 type tag + static_cast 自製不安全 RTTI。

CA10｜pointer 轉 integer 再轉回一定安全嗎？
答：若 integer type 足夠容納且依規則 round-trip，可回原 pointer；不是所有 integer 都夠，
也不代表可序列化跨 process/machine 或對任意 arithmetic 後仍有效。uintptr_t 本身也是 optional。

CA11｜`dynamic_pointer_cast` 與 raw `dynamic_cast` 的差別？
答：它對 shared_ptr 的 stored pointer 做 checked cast，成功後共享同 control block；
失敗回空 shared_ptr，不建立第二 ownership。先 get/dynamic_cast 再新 shared_ptr(raw) 會破壞 ownership。
==============================================================================
*/

#include <bit>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

// 受檢查的數值轉換：policy 是截斷小數；若業務要四捨五入，應明寫 std::round。
std::optional<int> checked_truncate(double value)
{
    if (!std::isfinite(value)) {
        return std::nullopt;
    }
    const double low = static_cast<double>(std::numeric_limits<int>::min());
    const double high = static_cast<double>(std::numeric_limits<int>::max());
    if (value < low || value > high) {
        return std::nullopt;
    }
    return static_cast<int>(value);
}

// -----------------------------------------------------------------------------
// 【日常實務範例】異質事件的 click 能力查詢
// 情境：事件佇列同時含 ClickEvent 與 CloseEvent，統計器只需讀取 click button，不應假設每筆都是 click。
// 為何使用本章主題：polymorphic Event 允許 dynamic_cast checked downcast，失敗自然映射成 std::nullopt。
// 設計：1. Event 提供 virtual base；2. clicked_button 嘗試 pointer cast；3. 成功回 button，其他事件回空。
// 成本：每次 cast 成本依 ABI/繼承圖，標準不保證 O(1)；事件 vector 本身空間 O(N)。
// 上線注意：cast 結果只借用原事件；若 click 行為普遍需要，應改 virtual operation/visitor 而非擴大 cast chain。
// -----------------------------------------------------------------------------
class Event {
public:
    virtual ~Event() = default;
    [[nodiscard]] virtual std::string name() const = 0;
};

class ClickEvent final : public Event {
public:
    explicit ClickEvent(int button) : button_(button) {}
    [[nodiscard]] std::string name() const override { return "click"; }
    [[nodiscard]] int button() const { return button_; }
private:
    int button_;
};

class CloseEvent final : public Event {
public:
    [[nodiscard]] std::string name() const override { return "close"; }
};

std::optional<int> clicked_button(const Event& event)
{
    // reference cast 失敗會 throw；查詢式 API 用 pointer cast 可自然映射 optional。
    const auto* click = dynamic_cast<const ClickEvent*>(&event);
    if (click == nullptr) {
        return std::nullopt;
    }
    return click->button();
}

void basic_cast_demo()
{
    assert(checked_truncate(42.9).value() == 42);
    assert(!checked_truncate(std::numeric_limits<double>::infinity()).has_value());

    const ClickEvent click(2);
    const CloseEvent close;
    assert(clicked_button(click).value() == 2);
    assert(!clicked_button(close).has_value());

    // bit_cast：取得 IEEE-754 實作上的 object representation；測試固定 bits 前先確認環境。
    if constexpr (std::numeric_limits<float>::is_iec559) {
        const std::uint32_t bits = std::bit_cast<std::uint32_t>(1.0F);
        assert(bits == 0x3F800000U);
        assert(std::bit_cast<float>(bits) == 1.0F);
    }
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 8. String to Integer (atoi)（字串轉整數）
// 題目：解析前導空白、正負號與數字，超出 int 時 clamp；"   -42 words" 回 -42。
// 為何使用本章主題：每次乘 10 前先證明 int 範圍；重點是 checked arithmetic，不是用最後的 cast 隱藏溢位。
// 思路：1. 略過空白並讀 sign；2. 逐 digit 先檢查 `(max-digit)/10`；3. 超界回端點，否則累加並套 sign。
// 複雜度：N 為字串長度；時間 O(N)、額外空間 O(1)。
// 易錯點：負端點的 magnitude 比 INT_MAX 多 1；本實作以正值累加的公式對負溢位提早回 INT_MIN。
// -----------------------------------------------------------------------------
int my_atoi(const std::string& text)
{
    std::size_t index = 0U;
    while (index < text.size() && text[index] == ' ') {
        ++index;
    }

    int sign = 1;
    if (index < text.size() && (text[index] == '+' || text[index] == '-')) {
        sign = text[index] == '-' ? -1 : 1;
        ++index;
    }

    int value = 0;
    while (index < text.size() && text[index] >= '0' && text[index] <= '9') {
        const int digit = text[index] - '0';
        if (value > (std::numeric_limits<int>::max() - digit) / 10) {
            return sign > 0 ? std::numeric_limits<int>::max()
                            : std::numeric_limits<int>::min();
        }
        value = value * 10 + digit;
        ++index;
    }
    return sign > 0 ? value : -value;
}

void leetcode_demo()
{
    assert(my_atoi("42") == 42);
    assert(my_atoi("   -42 words") == -42);
    assert(my_atoi("4193 with words") == 4193);
    assert(my_atoi("91283472332") == std::numeric_limits<int>::max());
    assert(my_atoi("-91283472332") == std::numeric_limits<int>::min());
}

// -----------------------------------------------------------------------------
// 【日常實務範例】利用率 basis points 量化
// 情境：將 0..1 的 double utilization 寫入 uint16_t basis points，並可還原為四位小數比例。
// 為何使用本章主題：API 先驗 domain，再以 lround 明訂四捨五入，最後 static_cast 到已證明可表示的 uint16_t。
// 設計：1. 拒絕非 finite 或超出 0..1；2. 乘 10,000 並 lround；3. 保存整數，讀取時轉 double 除回比例。
// 成本：每次轉換時間與空間皆 O(1)。
// 上線注意：量化會遺失精度；wire endian、版本與 NaN 政策需另訂，並避免 caller 誤認還原值等於原始 double。
// -----------------------------------------------------------------------------
class Utilization {
public:
    static Utilization from_ratio(double ratio)
    {
        if (!std::isfinite(ratio) || ratio < 0.0 || ratio > 1.0) {
            throw std::out_of_range("utilization ratio");
        }
        const auto basis_points = static_cast<std::uint16_t>(std::lround(ratio * 10'000.0));
        return Utilization(basis_points);
    }

    [[nodiscard]] std::uint16_t basis_points() const { return basis_points_; }
    [[nodiscard]] double ratio() const
    {
        return static_cast<double>(basis_points_) / 10'000.0;
    }

private:
    explicit Utilization(std::uint16_t basis_points) : basis_points_(basis_points) {}
    std::uint16_t basis_points_;
};

void practical_demo()
{
    const Utilization load = Utilization::from_ratio(0.75555);
    assert(load.basis_points() == 7'556U);
    assert(std::abs(load.ratio() - 0.7556) < 1e-12);

    std::vector<std::unique_ptr<Event>> events;
    events.push_back(std::make_unique<ClickEvent>(1));
    events.push_back(std::make_unique<CloseEvent>());
    int click_count = 0;
    for (const auto& event : events) {
        if (clicked_button(*event).has_value()) {
            ++click_count;
        }
    }
    assert(click_count == 1);
}

int main()
{
    basic_cast_demo();
    leetcode_demo();
    practical_demo();
    std::cout << "Cast summary: all assertions passed\n";
}

// 【章末自測】替一段 C-style cast 分別判定應改 static/dynamic/const/reinterpret 或完全移除。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'summary.cpp' -o '/tmp/codex_cpp_C_Cast_summary' && '/tmp/codex_cpp_C_Cast_summary'
//
// === 預期輸出（節錄）===
// Cast summary: all assertions passed
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
