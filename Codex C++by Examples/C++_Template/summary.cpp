/*
 * ============================================================================
 * C++ Template 面試前總複習（對應 01～28 章）
 * ============================================================================
 * 使用方式：
 * 1. 先讀「選擇表」與「推導規則」，確認知道何時用哪一項工具。
 * 2. 再讀整合程式：FixedHistory 同時示範 type/NTTP/default/policy/concept 與例外安全。
 * 3. 關掉答案口述「面試快問」，最後修改練習並重編譯。
 *
 * --------------------------------------------------------------------------
 * 【一、需求 -> 工具選擇表】
 * --------------------------------------------------------------------------
 * 需求                                   優先工具                    主要代價
 * 同一演算法支援多型別                   function template           實體/code size
 * 同一資料結構支援多元素型別             class template              定義多放 header
 * 尺寸/策略在編譯期固定                  NTTP                         不同值是不同型別
 * 一個確切型別需完全不同實作             full specialization          易與 overload 混淆
 * 一族型別（T*、pair<A,B>）需不同實作    partial specialization       僅 class/variable
 * 型別名稱太長                           alias template               不建立強型別
 * 每型別一個編譯期常數                   variable template            注意 inline/ODR
 * 任意數量參數                           variadic + fold               注意空 pack/順序
 * 保留 lvalue/rvalue                     forwarding reference         生命週期/重複轉發
 * 舊式條件啟用 overload                  SFINAE/void_t                  診斷冗長
 * 型別詢問/轉換                          type_traits                   只保證型別性質
 * 編譯期分支                             if constexpr                  分支過多會難維護
 * C++20 API 契約                         concepts/requires             別過度限制
 * 靜態多型                               CRTP                          無異質 base 容器
 * 編譯期替換行為                         policy-based design           組合多會膨脹
 * 執行期保存異質型別                     type erasure                  間接呼叫/可能配置
 * 預先計算                               constexpr/consteval           增加編譯成本
 *
 * 【28 章一行索引，確認沒有複習死角】
 * 01 動機：將不變演算法與可變型別分離；模板不是巨集。
 * 02 函式模板：由函式引數推導 T，回傳型別通常不參與推導。
 * 03 類別模板：每組引數形成不同型別，成員只在需要時實體化。
 * 04 參數種類：type、non-type、template-template 可混合。
 * 05 完整特化：某一組確切引數完全改寫主模板規則。
 * 06 偏特化：匹配 T*、pair<A,B> 等一整族類別/變數模板。
 * 07 預設引數：降低常用途徑噪音，但預設也是 API 契約。
 * 08 typename：模板參數列可與 class 互換；相依型別前則是解析提示。
 * 09 NTTP：尺寸、enum、structural value 進入型別系統。
 * 10 template-template：注入尚未實體化的 container/policy template。
 * 11 alias template：簡化巢狀型別名稱，但不建立強型別。
 * 12 variable template：每組 T 一個 inline constexpr 值，可完整/偏特化。
 * 13 variadic template：parameter pack 表達任意個型別/值。
 * 14 fold expression：定義 pack 的折疊方向、operator 與空 pack identity。
 * 15 perfect forwarding：T&& 推導配 std::forward，保留 value category。
 * 16 SFINAE：立即語境代換失敗移除候選，現代 API 多以 concept 取代。
 * 17 type traits：編譯期詢問/轉換型別，常搭配 if constexpr。
 * 18 tag dispatch：以空 tag 型別在編譯期選 overload，iterator category 是經典案例。
 * 19 if constexpr：未選分支在該實體被 discard。
 * 20 concept 入門：命名最小能力需求，改善 API 契約與診斷。
 * 21 進階 requires：simple/type/compound/nested requirement 與 subsumption。
 * 22 CRTP：Derived 傳回 Base，提供可 inline 的靜態多型。
 * 23 policy design：主流程固定、決策型別可組合，注意組合爆炸。
 * 24 type erasure：執行期異質集合，不要求來源型別共同繼承。
 * 25 編譯模型：header 定義、ODR、extern/explicit instantiation 邊界。
 * 26 lambda：generic lambda 的 call operator 本身是函式模板。
 * 27 constexpr template：可做編譯期運算；consteval 才是強制。
 * 28 mini STL：整合 iterator、容量、轉發、限制與例外安全；logical index 不等於實體 slot。
 *
 * --------------------------------------------------------------------------
 * 【二、模板推導與 reference collapsing】
 * --------------------------------------------------------------------------
 * template<class T> f(T value)
 *   - 傳 const int&：T 通常是 int；傳值會移除 top-level const/reference。
 * template<class T> f(T& value)
 *   - 只接 lvalue；const lvalue 時 T 可推導為 const int。
 * template<class T> f(const T& value)
 *   - 可接 lvalue/rvalue，但介面只讀；暫時物件活到完整 expression 結束。
 * template<class T> f(T&& value)
 *   - T 正在推導時才是 forwarding reference。
 *   - 傳 Widget lvalue：T=Widget&，T&& -> Widget&。
 *   - 傳 Widget rvalue：T=Widget，T&& -> Widget&&。
 *   - named value 自身一律是 lvalue；往下傳需 std::forward<T>(value)。
 *
 * reference collapsing：
 *   &  + &  -> &
 *   &  + && -> &
 *   && + &  -> &
 *   && + && -> &&
 * 只要其中有 lvalue reference，結果就是 lvalue reference。
 *
 * --------------------------------------------------------------------------
 * 【三、特化、overload、if constexpr、concept 怎麼選】
 * --------------------------------------------------------------------------
 * - 函式行為不同：先考慮 overload；函式模板不能 partial specialization。
 * - 類別某一族型別結構不同：partial specialization。
 * - 同一實作內只有少量型別分支：if constexpr。
 * - 要從呼叫邊界拒絕不合法型別：concept（舊碼才用 SFINAE）。
 * - 不要為了「炫技」特化；一般 overload 通常更容易被讀者與編譯器排序。
 *
 * --------------------------------------------------------------------------
 * 【四、相依名稱與編譯模型】
 * --------------------------------------------------------------------------
 * - `typename T::value_type`：T::value_type 依賴 T，需告知 parser 它是型別。
 * - `object.template get<int>()`：get 依賴 T，需告知 parser 它是模板。
 * - 模板定義通常放 header，因實體化點要看見完整定義。
 * - 可在 .cpp explicit instantiate 固定型別，header 用 extern template 降重複工作。
 * - 每個 translation unit 看到的模板定義必須符合 ODR；不要用巨集讓定義漂移。
 * - 改 header template 常造成大量重編；大型專案可用 modules/PImpl/explicit instantiation 控制。
 *
 * --------------------------------------------------------------------------
 * 【五、複雜度與效能判讀】
 * --------------------------------------------------------------------------
 * 模板本身不改變 Big-O；它讓演算法針對型別實體化並有機會 inline。
 * - binary search：O(log n)，前提是 random access 且已依同一 comparator 排序。
 * - two sum + unordered_map：平均 O(n)、最壞 O(n^2)、空間 O(n)。
 * - prefix sum：預處理 O(n)、query O(1)、空間 O(n)。
 * - fold：O(sizeof...(Args))，編譯器可展開；pack 太大會拖慢編譯。
 * - type erasure：Big-O 常不變，但多一層 indirection，且可能 heap allocation。
 * - policy/CRTP：常可零額外 dispatch，但每組型別可能增加機器碼。
 *
 * --------------------------------------------------------------------------
 * 【六、生命週期與 undefined behavior 高風險區】
 * --------------------------------------------------------------------------
 * 1. 回傳 const T& 前，不能讓它指向函式內區域物件或即將死亡的 temporary。
 * 2. forwarding 不延長生命週期；callback capture reference 仍可能懸空。
 * 3. std::move 只是 cast；moved-from 物件有效但狀態通常 unspecified，只可重新指定/銷毀。
 * 4. iterator/reference 失效由底層容器決定，模板 wrapper 不會自動保護。
 * 5. CRTP 的 static_cast 假設 Derived 配對正確；限制 base 建構可減少誤用。
 * 6. type erasure 內保存的 callable 若捕捉 reference，wrapper 擁有 callable 但不擁有 referent。
 * 以上也是本主題最常見的【陷阱】；回答題目時必須把 owner 與失效時間說完整。
 *
 * --------------------------------------------------------------------------
 * 【七、Concept / requires 速查】
 * --------------------------------------------------------------------------
 * template<class T> concept C = requires(T x) {
 *     typename T::value_type;               // type requirement
 *     x.clear();                            // simple requirement
 *     { x.size() } noexcept -> same_as<size_t>; // compound requirement
 *     requires sizeof(T) <= 64;             // nested requirement
 * };
 * Concept 只驗「表達式與型別關係」，不證明語意：
 * - operator< 存在，不代表滿足 strict weak ordering。
 * - hash<T> 可呼叫，不代表相等物件一定有相同 hash。
 * - begin/end 存在，不代表 iterator 可多次走訪。
 * Constraint 應對應實作真正使用的最小能力：
 * - FixedHistory 的 T 一律需 move construction；只有 overwrite policy 才再需 move assignment，
 *   不應平白要求 default ctor 或讓 reject policy 承擔用不到的限制。
 * - FullPolicy 只需提供 compile-time bool `overwrite`，不需繼承某個 policy base。
 * - Two Sum 需要 copy、equality、hash 與 subtraction；「arithmetic 且可相加」反而過度限制。
 *
 * --------------------------------------------------------------------------
 * 【八、面試快問快答】
 * --------------------------------------------------------------------------
 * Q1：模板與巨集差異？
 * A：模板進入語法/型別系統、有作用域；巨集只是預處理文字替換。
 *
 * Q2：function template 可 partial specialization 嗎？
 * A：不行；使用 overload、helper class specialization 或 concept。
 *
 * Q3：`typename` 與 `class` 在模板參數列有差嗎？
 * A：無；但相依型別前的 typename 不是 class 可替代的用法。
 *
 * Q4：何謂 SFINAE immediate context？
 * A：函式型別、模板參數等代換直接涉及的部分；函式本體深處錯誤通常不是 SFINAE。
 *
 * Q5：if constexpr 與普通 if 差異？
 * A：前者未選分支在該實體被 discard，可含僅對其他 T 合法的程式。
 *
 * Q6：std::forward 是否搬移？
 * A：不，它依 T 做條件式 cast；接收端 move constructor/overload 才真正取得資源。
 *
 * Q7：CRTP 能取代 virtual 嗎？
 * A：只在型別編譯期已知時；外掛式執行期多型與異質集合仍需 virtual/type erasure。
 *
 * Q8：concept 是否讓程式更快？
 * A：主要改善介面與診斷；可能幫助選特化，但不是自動效能開關。
 *
 * Q9：constexpr 是否保證編譯期執行？
 * A：不；constant-evaluated context 才保證。consteval 函式則強制。
 *
 * Q10：模板 code bloat 如何處理？
 * A：減少不必要型別組合、抽出非相依邏輯、explicit instantiation、檢查 LTO/ICF。
 *
 * Q11：alias template 會建立新型別嗎？
 * A：不會。要避免 ID 混用，建立 explicit/strong wrapper class。
 *
 * Q12：empty parameter pack 的 fold 怎麼辦？
 * A：使用有 identity 的 binary fold，例如 `(0 + ... + args)`。
 * 這一節就是完整【面試段落】，建議遮住答案後逐題口述。
 *
 * --------------------------------------------------------------------------
 * 【九、考前實作檢查表】
 * --------------------------------------------------------------------------
 * [ ] 限制的是演算法真正需要的最小能力，不是特定容器名稱。
 * [ ] comparator 同時用於排序與搜尋，符合 strict weak ordering。
 * [ ] reference 回傳值的 owner 與失效條件寫清楚。
 * [ ] forwarding parameter 只 forward 一次，不在 forward 後再讀。
 * [ ] 公開錯誤在 concept 邊界被拒絕，錯誤訊息可讀。
 * [ ] constexpr 計算有溢位/編譯步數上限評估。
 * [ ] ring/full-buffer 的 logical oldest 映射與 mutation commit point 已明訂。
 * [ ] policy 組合數與 type-erasure runtime 成本有實測，不靠猜測。
 * [ ] 模板定義/explicit instantiation 的 translation-unit 邊界一致。
 */

/*
==============================================================================
【面試深挖：Templates 與 Generic Programming】

T1｜template 是 compile-time polymorphism 嗎？
答：通常是 static polymorphism，一份 source pattern 產生 specializations；但 template 也可
產生 runtime polymorphic class。核心是參數化 declaration/definition，不是單一句「零成本多型」。

T2｜function template 與 class template deduction 差別？
答：function arguments 可直接推 template parameters；class template 到 C++17 才有 CTAD，
且由 constructors/deduction guides 推導。return type-only parameter 通常無法由 call 推得。

T3｜dependent name 為何要 `typename` / `template`？
答：template definition 第一階段無法知道 `T::x` 是 type 還是 value、`obj.f<Y>` 的 <
是 template args 還是比較；disambiguator 告訴 parser，並與 two-phase lookup 相關。

T4｜full specialization 與 partial specialization？
答：class template 可 full/partial；function template 只有 full specialization，通常用 overload
取代，因 specialization 與 overload resolution 互動容易誤解。alias template 也不能直接特化。

T5｜SFINAE 的精確範圍？
答：substitution failure 只在 immediate context 使 candidate 被移除；function body 或額外
instantiation 的錯誤仍是 hard error。C++20 concepts 通常提供更直接 constraint 與診斷。

T6｜forwarding reference 如何辨識？
答：cv-unqualified template parameter T 在 deduction context 的 `T&&`，或相應 auto&&；
不是所有 rvalue reference 都是 forwarding，例如 `vector<T>&&` 與 class-template member 的固定 T。

T7｜reference collapsing 規則？
答：只要任一邊是 lvalue reference，結果 T&；只有 && 與 && 才是 T&&。配合 template deduction
讓 T&& 接 lvalue/rvalue，再由 `std::forward<T>` 恢復原 value category。

T8｜`std::forward` 與 `std::move`？
答：move 無條件轉 xvalue；forward 依 deduced T 有條件保留 caller category。
forward 必須帶 T，因它需要原 deduction 資訊；誤用會把 lvalue 意外搬走。

T9｜perfect forwarding 哪些情況不「perfect」？
答：braced initializer、0/NULL、overload set、bit-field、static const integral ODR-use 等可能
無法正常 deduction/forward。wrapper API 需要額外 overload 或明確 type。

T10｜variadic template 與 fold expression 的工程用途？
答：參數包可建立 tuple、logging、factory；C++17 fold 簡化 recursive expansion。
要定義 evaluation order、empty pack identity 與 operation associativity，不能只會 `(... + args)`。

T11｜template 為何通常定義在 header？
答：implicit instantiation 的 translation unit 需要看到 definition。可用 explicit instantiation
definition/declaration（extern template）集中常用 types，降低 build time/code duplication。

T12｜template code bloat 如何處理？
答：把 type-independent work 移到 non-template function、限制 instantiations、顯式 instantiation、
使用 type erasure/runtime polymorphism於穩定 ABI 邊界，並用 binary/profile 數據而非猜測。

T13｜CRTP 與 virtual 的取捨？
答：CRTP static dispatch、型別在 compile time 已知，可 mixin/compile-time interface；
virtual 支援 runtime heterogeneous objects 與穩定介面。CRTP 不能自然取代 open runtime plugin。

T14｜hidden friend 為何常出現在 class template？
答：在 class 內定義 non-member friend，由 ADL 找到，避免污染一般 lookup 並讓兩側 conversion
對稱；每個 specialization 產生對應 operation。它與 friend template 授權範圍要區分。

T15｜Concepts 比 SFINAE 好在哪？
答：直接描述 semantic/syntactic constraint、參與 overload ordering、錯誤更接近 call site。
但 concept 只檢查可表達的 requirement，不會自動證明複雜度、無副作用或完整語意契約。
==============================================================================
*/

#include <array>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <functional>
#include <iostream>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

template <typename Policy>
concept HistoryPolicy = requires {
    std::bool_constant<static_cast<bool>(Policy::overwrite)>{};
};

template <typename T, typename Policy>
concept HistoryValueFor = std::move_constructible<T> &&
    (!static_cast<bool>(Policy::overwrite) || std::assignable_from<T&, T>);

template <typename T>
concept TwoSumKey = std::integral<T> && std::copy_constructible<T> &&
    requires(const T& left, const T& right) {
        { left == right } -> std::convertible_to<bool>;
        { std::hash<T>{}(left) } -> std::convertible_to<std::size_t>;
    };

template <typename T>
concept PowerTableValue = std::default_initializable<T> &&
    std::constructible_from<T, int> && std::assignable_from<T&, const T&> &&
    requires(T& value, const T& factor) {
        { value *= factor } -> std::same_as<T&>;
};

struct RejectWhenFull {
    static constexpr bool overwrite = false;
};

struct OverwriteOldest {
    static constexpr bool overwrite = true;
};

// 整合 type parameter + NTTP + default argument + policy + concept + if constexpr。
// logical index 0 永遠是 oldest；head_ 保存 oldest 的 physical slot。
// optional 讓未使用 slot 不必先建構 T，也讓「已建構元素數」與 size_ 精確一致。
template <typename T, std::size_t Capacity, HistoryPolicy FullPolicy = RejectWhenFull>
requires HistoryValueFor<T, FullPolicy>
class FixedHistory {
public:
    static_assert(Capacity > 0U);

    [[nodiscard]] bool push(T value) {
        if (size_ < Capacity) {
            const std::size_t slot = physical_index(size_);
            data_[slot].emplace(std::move(value));
            // emplace 成功才 commit size；若 T move construction 丟例外，slot 仍 disengaged。
            ++size_;
            return true;
        }
        if constexpr (FullPolicy::overwrite) {
            // engaged optional 會 move-assign T。失敗時 size/head 不提交，所有 slot 仍 engaged；
            // T 自身僅保證其 assignment 的基本保證，若要「舊值完全不變」需另加 copy/swap 契約。
            data_[head_] = std::move(value);
            head_ = increment(head_);
            return true;
        } else {
            return false;
        }
    }

    const T& at(std::size_t index) const {
        if (index >= size_) {
            throw std::out_of_range("FixedHistory::at");
        }
        return data_[physical_index(index)].value();
    }

    [[nodiscard]] std::size_t size() const noexcept { return size_; }
    static constexpr std::size_t capacity() noexcept { return Capacity; }

private:
    static constexpr std::size_t increment(std::size_t slot) noexcept {
        return (slot + 1U) % Capacity;
    }

    [[nodiscard]] std::size_t physical_index(std::size_t logical_index) const noexcept {
        return (head_ + logical_index) % Capacity;
    }

    std::array<std::optional<T>, Capacity> data_{};
    std::size_t head_{};
    std::size_t size_{};
};

// LeetCode 1：Two Sum；concept 把 unordered_map 與減法所需契約放在呼叫邊界。
template <TwoSumKey T>
std::pair<std::size_t, std::size_t>
leetcode_two_sum(const std::vector<T>& values, T target) {
    std::unordered_map<T, std::size_t> seen;
    for (std::size_t i = 0; i < values.size(); ++i) {
        // 若數學上的補數超出 T，它不可能出現在 vector<T>；略過本輪而不是先做 UB。
        if constexpr (std::is_signed_v<T>) {
            if ((values[i] > 0 && target < std::numeric_limits<T>::lowest() + values[i]) ||
                (values[i] < 0 && target > std::numeric_limits<T>::max() + values[i])) {
                seen.emplace(values[i], i);
                continue;
            }
        } else if (values[i] > target) {
            seen.emplace(values[i], i);
            continue;
        }
        const T wanted = static_cast<T>(target - values[i]);
        if (const auto found = seen.find(wanted); found != seen.end()) {
            return {found->second, i};
        }
        seen.emplace(values[i], i);
    }
    return {values.size(), values.size()};
}

// constexpr/NTTP：編譯期建立指數表。只為「下一個要保存的值」乘 2；若最後一個結果已
// 寫入就停止，避免做一個不屬於結果、且對 signed integer 可能造成 UB 的尾端乘法。
// 呼叫端仍須保證每個實際要保存的冪都可由 T 表示；concept 無法證明 runtime value range。
template <PowerTableValue T, std::size_t Count>
constexpr std::array<T, Count> powers_of_two() {
    if constexpr (std::is_integral_v<T> && std::is_signed_v<T>) {
        static_assert(Count <= static_cast<std::size_t>(std::numeric_limits<T>::digits),
                      "最後一個 2 的冪必須可由 signed T 表示");
    }
    std::array<T, Count> values{};
    T current{1};
    for (std::size_t index = 0; index < Count; ++index) {
        values[index] = current;
        if (index + 1U < Count) {
            current *= T{2};
        }
    }
    return values;
}

// 測試用 value：move construction/assignment 可注入失敗，以驗證 FixedHistory 不會先
// commit size/head。正式 generic container 的 exception tests 也應涵蓋每個 mutation point。
struct ThrowingValue {
    int value{};
    static inline bool throw_on_move = false;

    explicit ThrowingValue(int input) : value(input) {}
    ThrowingValue(const ThrowingValue&) = default;
    ThrowingValue& operator=(const ThrowingValue&) = default;

    ThrowingValue(ThrowingValue&& other)
    {
        if (throw_on_move) throw std::runtime_error("injected move construction failure");
        value = other.value;
    }

    ThrowingValue& operator=(ThrowingValue&& other)
    {
        if (throw_on_move) throw std::runtime_error("injected move assignment failure");
        value = other.value;
        return *this;
    }
};

void fixed_history_exception_test() {
    FixedHistory<ThrowingValue, 2> growing;
    const ThrowingValue first{7};
    ThrowingValue::throw_on_move = true;
    [[maybe_unused]] bool append_failed = false;
    try {
        static_cast<void>(growing.push(first)); // parameter copy 成功，slot move construction 失敗。
    } catch (const std::runtime_error&) {
        append_failed = true;
    }
    ThrowingValue::throw_on_move = false;
    assert(append_failed && growing.size() == 0U);
    // NDEBUG 會移除整個 assert expression；push 必須先執行，否則 release 測試狀態不同。
    [[maybe_unused]] const bool growing_inserted = growing.push(first);
    assert(growing_inserted);
    assert(growing.at(0).value == 7);

    FixedHistory<ThrowingValue, 1, OverwriteOldest> full;
    [[maybe_unused]] const bool full_inserted = full.push(first);
    assert(full_inserted);
    const ThrowingValue replacement{9};
    ThrowingValue::throw_on_move = true;
    [[maybe_unused]] bool overwrite_failed = false;
    try {
        static_cast<void>(full.push(replacement)); // engaged slot 的 move assignment 失敗。
    } catch (const std::runtime_error&) {
        overwrite_failed = true;
    }
    ThrowingValue::throw_on_move = false;
    assert(overwrite_failed && full.size() == 1U);
    assert(full.at(0).value == 7); // 測試型別在丟出前未修改舊值；head 也未前進。
}

class ErasedTask {
public:
    ErasedTask() : function_([] { return std::string{}; }) {}

    template <typename Function>
    explicit ErasedTask(Function function) : function_(std::move(function)) {}
    std::string run() const { return function_(); }

private:
    std::function<std::string()> function_;
};

template <typename Validator, typename... Values>
bool validate_all(Validator validator, const Values&... values) {
    return (validator(values) && ...);
}

// 實務整合：policy 容器保存 type-erased task，fold 驗證輸入，lambda 作具體工作。
void practical_task_pipeline_test() {
    FixedHistory<ErasedTask, 2> tasks;
    // push/run 都是教材要實際執行的操作，不可讓 -DNDEBUG 把呼叫本身刪掉。
    [[maybe_unused]] const bool backup_inserted =
        tasks.push(ErasedTask([] { return std::string{"backup-ok"}; }));
    assert(backup_inserted);

    const std::string service = "search";
    [[maybe_unused]] const bool deploy_inserted =
        tasks.push(ErasedTask([service] { return "deploy-" + service; }));
    assert(deploy_inserted);
    [[maybe_unused]] const bool overflow_inserted =
        tasks.push(ErasedTask([] { return std::string{"overflow"}; }));
    assert(!overflow_inserted);

    [[maybe_unused]] const std::string backup_result = tasks.at(0).run();
    [[maybe_unused]] const std::string deploy_result = tasks.at(1).run();
    assert(backup_result == "backup-ok");
    assert(deploy_result == "deploy-search");

    const auto nonempty = []<typename T>(const T& value) {
        if constexpr (requires { value.empty(); }) {
            return !value.empty();
        } else {
            return value != T{};
        }
    };
    [[maybe_unused]] const bool valid_inputs =
        validate_all(nonempty, std::string{"gpu"}, 7, true);
    [[maybe_unused]] const bool invalid_inputs =
        validate_all(nonempty, std::string{}, 7);
    assert(valid_inputs);
    assert(!invalid_inputs);
}

int main() {
    // 1. 編譯期驗證。
    static_assert(FixedHistory<int, 4>::capacity() == 4U);
    constexpr auto powers = powers_of_two<int, 6>();
    static_assert(powers == std::array{1, 2, 4, 8, 16, 32});
    constexpr auto full_int_range =
        powers_of_two<int, static_cast<std::size_t>(std::numeric_limits<int>::digits)>();
    static_assert(full_int_range.back() == std::numeric_limits<int>::max() / 2 + 1);

    // 2. LeetCode 整合測試。
    [[maybe_unused]] const auto answer = leetcode_two_sum(std::vector<int>{2, 7, 11, 15}, 9);
    assert(answer.first == 0U && answer.second == 1U);
    [[maybe_unused]] const auto absent = leetcode_two_sum(std::vector<int>{1, 2, 3}, 100);
    assert(absent.first == 3U && absent.second == 3U);

    // 3. Policy 行為。
    FixedHistory<int, 2, OverwriteOldest> recent;
    [[maybe_unused]] const bool inserted_10 = recent.push(10);
    [[maybe_unused]] const bool inserted_20 = recent.push(20);
    [[maybe_unused]] const bool inserted_30 = recent.push(30);
    assert(inserted_10);
    assert(inserted_20);
    assert(inserted_30);
    assert(recent.size() == 2U);
    assert(recent.at(0) == 20); // logical index 0 永遠是目前 oldest
    assert(recent.at(1) == 30);
    [[maybe_unused]] const bool inserted_40 = recent.push(40);
    assert(inserted_40);
    assert(recent.at(0) == 30 && recent.at(1) == 40);

    // 4. 例外安全：失敗的 append/overwrite 不提交 size/head。
    fixed_history_exception_test();

    // 5. 實務整合測試。
    practical_task_pipeline_test();

    std::cout << "Template summary：所有整合測試通過\n";
}

/*
 * 【最後練習】
 * 1. 為 FixedHistory 寫 iterator，讓它能進 range-for。
 * 2. 擴充 HistoryPolicy 支援 reject/overwrite/throw，仍只要求各分支真正用到的能力。
 * 3. 將 ErasedTask 改成 move-only erasure，使其可保存捕捉 unique_ptr 的 lambda。
 * 4. 量測 FixedHistory<int,2,Policy> 不同 policy 實體的 sizeof 與機器碼。
 * 5. 把 leetcode_two_sum 改成接受 range 與 projection，維持清楚的 constraint diagnostics。
 */
