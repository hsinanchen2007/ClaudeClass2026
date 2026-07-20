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

// ---------------------------------------------------------------------------
// LeetCode 8：String to Integer (atoi)
// 重點不是「最後 static_cast 一下」，而是在每次乘 10 前保證不越界。
// 複雜度 O(n) time / O(1) space。
// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
// 實務：wire protocol 以明確整數寬度表示百分比（basis points）。
// domain validation 與 rounding policy 都在 API 內；不讓呼叫端猜 cast 的截斷行為。
// ---------------------------------------------------------------------------
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
