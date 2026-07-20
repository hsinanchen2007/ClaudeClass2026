/*
 * ============================================================================
 * C++ Utility 面試前總複習（pair / tuple / optional / variant / any / value tools）
 * ============================================================================
 * 本檔目的不是展示一兩個語法，而是在面試前用單檔快速恢復「選擇能力」。
 * 所有範例可直接執行；先讀表格，再口述面試題，最後看整合 pipeline。
 *
 * --------------------------------------------------------------------------
 * 【一、工具選擇表】
 * --------------------------------------------------------------------------
 * 工具                    表達的資料模型                         何時選
 * pair<A,B>               同時有兩個異質值                       局部、兩欄語意明顯
 * tuple<Ts...>            同時有固定多個異質值                   短距離資料搬運/apply
 * optional<T>             零或一個、nested within object 的 T     缺值是正常情況
 * variant<Ts...>          已知且封閉的多選一                     狀態/訊息/AST node
 * any                     開放的任意一型                         外掛 metadata 邊界
 * swap                    對稱交換兩物件                         commit/copy-and-swap
 * move                    無條件轉成 xvalue                       明確轉移 ownership
 * forward<T>              保留推導出的 value category            factory/emplace/wrapper
 * exchange(obj,new)       指定新值並回傳舊值                     狀態機/move source 清空
 * as_const(obj)           取得 const lvalue reference             強制只讀 overload
 * bitset<N>               固定 N 個 bits                          權限/協定旗標
 * hash + unordered_*      以 hash bucket 查 key                   平均 O(1) lookup
 * initializer_list<T>     指向 temporary const array 的輕量 view  braces 小清單/API
 *
 * 「欄位需要名稱/不變量」時，struct/class 優於 pair/tuple。
 * 「需要錯誤原因」時，expected<T,E> 優於 optional<T>。
 * 「型別集合可列舉」時，variant 優於 any，因編譯器可檢查處理是否完整。
 *
 * --------------------------------------------------------------------------
 * 【二、擁有權與生命週期速查】
 * --------------------------------------------------------------------------
 * - pair/tuple/variant/any 預設皆為 value container；optional 若 engaged，標準明訂其 T
 *   nested within optional object。這排除另以 heap node 代存 T；但 T 自己仍可配置資源。
 * - 但它們可以保存 pointer/reference_wrapper；此時不擁有 referent，仍可能懸空。
 * - initializer_list 不擁有 backing array；複製 list 只複製 begin/size，也不會把
 *   backing array 的生命延長到原始 initializer_list owner 之外。
 * - structured binding：`auto [a,b]` 複製；`auto& [a,b]` 綁原值；`const auto&` 唯讀不複製。
 * - tuple 的 get<I>/get<T>、tie/apply/tuple_cat 都是 compile-time shape；get<T> 要求 T 唯一。
 * - std::move/forward 都不延長 temporary 生命週期，也不保證真的呼叫 move constructor。
 * - moved-from object 有效但狀態通常 unspecified；只做型別保證允許的操作。
 * - any/variant visitor 若 capture reference，wrapper 擁有 visitor 不代表擁有 referent。
 *
 * --------------------------------------------------------------------------
 * 【三、optional / variant / any 錯誤行為】
 * --------------------------------------------------------------------------
 * optional:
 *   has_value()/operator bool 先查；value() 無值丟 bad_optional_access；* 無值是 UB。
 * variant:
 *   holds_alternative 先查；get 取錯丟 bad_variant_access；get_if 回 nullptr。
 * any:
 *   has_value/type 查狀態；any_cast<T>(value) 取錯丟 bad_any_cast；pointer 版回 nullptr。
 *
 * 不要用 exception 當日常分支：探測型 API 使用 value_or/get_if/pointer any_cast。
 * `optional<bool>{false}` 是 engaged false，不等於 nullopt。
 * `variant<monostate,...>` 可表達尚未選擇；否則預設建構第一 alternative。
 * C++20 any 要求被存型別 CopyConstructible，所以不能直接放 move-only unique_ptr。
 *
 * --------------------------------------------------------------------------
 * 【四、move / forward / swap / exchange / as_const】
 * --------------------------------------------------------------------------
 * move:
 *   `std::move(x)` 約等於 cast<T&&>(x)，讓 move overload 可被選；真正搬移在接收端。
 * forward:
 *   forwarding reference `T&&` + `std::forward<T>(x)` 保留 caller 的 lvalue/rvalue。
 * swap:
 *   generic code 使用 `using std::swap; swap(a,b);`，保留 ADL 自訂高效 noexcept swap。
 * exchange:
 *   `old = exchange(state, next)`；適合狀態轉移和 move constructor 清空來源 handle。
 * as_const:
 *   `std::as_const(x)` 不複製，回 const T&，明確選 const overload；不接受 rvalue。
 *
 * reference collapsing：只要組合中有 `&`，結果通常是 `&`；只有 `&& + &&` 留作 `&&`。
 * `std::move(const T)` 多半不能呼叫 T(T&&)，最後仍 copy，因不能從 const source 偷資源。
 * `return std::move(local)` 可能阻礙 NRVO；一般直接 `return local`。
 *
 * --------------------------------------------------------------------------
 * 【五、bitset / hash / initializer_list 成本與規則】
 * --------------------------------------------------------------------------
 * bitset<N>:
 *   size 固定；單 bit 操作通常 O(1)，count 依 machine word 數，約 O(N/word_bits)。
 *   index 0 是最低位，但 to_string 左邊顯示最高位；to_ulong 太寬會 overflow_error。
 * hash/unordered_map:
 *   find/insert 平均 O(1)、最壞 O(n)；rehash O(n)，會使 iterator 失效。
 *   KeyEqual(a,b)==true => Hash(a)==Hash(b) 是硬契約；collision 不代表相等。
 *   std::hash 不是密碼學 hash，值也不保證跨平台/版本/執行固定。
 * initializer_list:
 *   建立 O(n) backing const array；複製 list handle O(1)，但不額外延長 backing array
 *   的生命；複製元素到 vector 等 owned storage 才是 O(n) 且真正取得所有權。
 *   元素 const，無法 move 出 move-only value；constructor overload 有高優先權。
 *
 * --------------------------------------------------------------------------
 * 【六、常見陷阱清單】
 * --------------------------------------------------------------------------
 * 1. pair.first/tuple get<3> 在跨模組 API 缺乏語意，應改 named struct。
 * 2. get<T>(tuple/variant) 遇到重複型別無法決定，改用 index。
 * 3. optional operator*、variant get、any_cast value 版都要先處理錯誤路徑。
 * 4. any 中的 `const char*` 不會自動變 std::string。
 * 5. `auto tuple = forward_as_tuple(temporary)` 保存 dangling rvalue reference。
 * 6. generic swap 寫死 std::swap 會跳過 ADL 特化版本。
 * 7. 同一 forwarding arg 轉發兩次，第二個消費者可能拿到已搬空物件。
 * 8. exchange 不是 atomic；跨 thread 狀態需 atomic.exchange 或 mutex。
 * 9. as_const 是 shallow const，pointer pointee 未必唯讀。
 * 10. 修改 unordered container key 會破壞 bucket 不變量。
 * 11. temporary initializer_list（例如函式引數）的 backing array 只活到完整運算式結束；
 *     具名 initializer_list 會把 backing array 壽命延長到該原始物件的 scope；再複製
 *     一份 list handle 不會把陣列延長到更久。兩者都不擁有可移出元素。
 * 12. vector{10,20} 是兩元素；vector(10,20) 是十個 20。
 * 13. 空 queue 呼叫 front/pop 是 UB；先查 empty，並用 queue/deque 的 O(1) pop-front，
 *     不要以 vector.erase(begin()) 偽裝 FIFO，因每次都要搬移後方 O(n) 個元素。
 *
 * --------------------------------------------------------------------------
 * 【七、面試快問快答】
 * --------------------------------------------------------------------------
 * Q1：pair 與 tuple 是否建立新領域型別？
 * A：是具體 library 型別，但欄位沒有領域名稱；公共契約通常仍用自訂 struct。
 *
 * Q2：optional 是否 heap allocate？
 * A：標準保證 engaged 的 T nested within optional object，不另用 heap node 保存 T；
 *    但 T 的內部仍可配置，而且實際 layout、padding 與大小不可寫死。
 *
 * Q3：variant 與 virtual hierarchy 怎麼選？
 * A：封閉型別集合且操作集中用 variant；開放擴充且行為隨 derived 分散用 virtual。
 *
 * Q4：variant 何時 valueless_by_exception？
 * A：替換 alternative 時建構失敗且無法保留舊值的少數情況，可查該狀態。
 *
 * Q5：any、variant、optional 一句話差異？
 * A：任意一型、已知多選一、零或一個指定型。
 *
 * Q6：std::move 做了什麼？
 * A：cast；不配置、不搬 bytes、不清空 source。
 *
 * Q7：std::forward 的 T 從哪來？
 * A：必須沿 forwarding reference 的模板推導結果傳入，才能恢復 caller value category。
 *
 * Q8：為何 swap 要 noexcept？
 * A：許多容器/演算法據此提供 strong guarantee 或選擇高效路徑。
 *
 * Q9：unordered_map 最壞為何 O(n)？
 * A：大量 key 可能落同 bucket，需逐一 equality 比較；惡意輸入也可能碰撞攻擊。
 *
 * Q10：hash equality 契約是雙向嗎？
 * A：相等必須同 hash；同 hash 不必相等。
 *
 * Q11：initializer_list 為何不能搬 unique_ptr？
 * A：backing array 元素是 const unique_ptr，move constructor 需要非 const rvalue。
 *
 * Q12：as_const 是否等於建立 const copy？
 * A：不是，它只產生原物件的 const lvalue reference。
 *
 * --------------------------------------------------------------------------
 * 【八、考前選擇檢查表】
 * --------------------------------------------------------------------------
 * [ ] 缺值是否正常？正常 optional；需原因 expected；程式錯誤才 exception。
 * [ ] 型別集合是否封閉？封閉 variant；開放外掛邊界才 any/type erasure。
 * [ ] 回傳兩/多欄是否跨 API？跨 API 改 named struct。
 * [ ] ownership 是否真的轉移？move 後 source 只依文件允許操作。
 * [ ] wrapper 是否用 forwarding reference，且每個 arg 只 forward 一次？
 * [ ] 自訂 value type 是否有 noexcept swap/move，並保持 class invariant？
 * [ ] hash/equality 是否使用同一正規化？是否錯把 hash 當安全功能？
 * [ ] initializer_list 是否立即複製到 owned storage？
 */

/*
==============================================================================
【面試深挖：Utility Types 與 Vocabulary Types】

U1｜`pair` 與 `tuple` 何時不如自訂 struct？
答：短暫 local 組合可用；跨 API/領域資料若 first/get<2> 無語意，應自訂命名型別並維持 invariant。
型別名稱與欄位名是可維護性，不是「多寫幾行」浪費。

U2｜`optional<T>` 是否等同 nullable pointer？
答：optional inline 擁有一個可能不存在的 T value；pointer 涉及 identity/borrow/ownership。
大型 T 的 optional 會放大 object 大小，且 optional<bool> 有三狀態，API 要說清楚。

U3｜`value_or(expensive())` 是否 lazy？
答：不是，function arguments 在呼叫前求值，即使 optional 有值，fallback 也先建。
昂貴 fallback 用 explicit if、`or_else`（較新標準）或 helper lambda。

U4｜variant 的 visitor 如何做到 exhaustive？
答：`std::visit` 會對 active alternative 呼叫 visitor；overloaded lambdas 可一型一 handler。
若放 generic catch-all，新增 alternative 可能靜默走 fallback，失去 compile-time 提醒。

U5｜`any_cast` 失敗如何處理？
答：value/reference overload 丟 bad_any_cast；pointer overload 回 nullptr。any 適合少數 plugin/
metadata boundary，不應把所有 domain data 裝 any 後到處 runtime cast。

U6｜`std::move`、`forward`、`exchange` 各做什麼？
答：move 是 unconditional xvalue cast；forward 保留 deduced category；exchange 把 object 換成
new value 並回舊值，常寫 move constructor/reset state。三者不是 resource manager。

U7｜generic `swap` 如何支援 user type？
答：在 generic code 中 `using std::swap; swap(a,b);`，讓 ADL 找 user overload並有 std fallback。
型別可提供 noexcept friend swap；不要未經允許替 user-defined type 特化 std::swap。

U8｜`as_const` 的用途？
答：取得 const view 以選 const overload、防止意外修改，不做 copy。對 rvalue 的 overload 被 delete，
避免建立立即 dangling 的 const reference helper。

U9｜hash/equality 契約？
答：若 KeyEqual(a,b) true，Hash(a)==Hash(b) 必須成立；反向不要求，collision 合法。
hash 不等於 cryptographic hash，也不承諾跨執行/實作穩定。

U10｜`initializer_list` 的 lifetime 陷阱？
答：backing array 通常只活到完整表達式/依初始化規則延長；class 若保存 begin/end pointer，
constructor 結束後可能 dangling。initializer_list elements 是 const，move-only type 也常受限。

U11｜`bitset<N>` 的 N 為何必須 compile time？
答：N 是 template argument，storage 固定在 object；支援 bitwise/count/test 與 string conversion。
runtime bit count 需 dynamic representation，不能 resize bitset。

U12｜`std::clamp` 結果可以保存成 const reference 嗎？
答：若選中的參數是 temporary，回傳 const T& 會 dangling；保存 value 較安全。
low>high 違反前置條件，且 NaN/order policy 必須由 caller 定義。
==============================================================================
*/

#include <algorithm>
#include <any>
#include <bitset>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <limits>
#include <optional>
#include <queue>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

using Indices = std::pair<std::size_t, std::size_t>;

using ServiceRecord = std::tuple<std::string, int, bool>;

std::string render_service_record(const ServiceRecord& record)
{
    return std::apply([](const std::string& name, const int port, const bool healthy) {
        return name + ":" + std::to_string(port) + (healthy ? ":up" : ":down");
    }, record);
}

void pair_tuple_optional_demo()
{
    const std::pair<std::string, int> metric{"gpu_temp", 72};
    const auto& [metric_name, metric_value] = metric; // const references，不複製欄位。
    assert(metric_name == "gpu_temp" && metric_value == 72);

    const ServiceRecord service{"inference", 8080, true};
    assert(std::get<0>(service) == "inference");
    assert(std::get<int>(service) == 8080); // int 在 tuple 中唯一，才能用型別 get。
    const auto& [name, port, healthy] = service;
    assert(name == "inference" && port == 8080 && healthy);
    assert(render_service_record(service) == "inference:8080:up");

    int extracted_port = 0;
    bool extracted_health = false;
    std::tie(std::ignore, extracted_port, extracted_health) = service;
    assert(extracted_port == 8080 && extracted_health);

    const std::optional<bool> disabled{false};
    assert(disabled.has_value()); // engaged false 與 nullopt 是不同狀態。
    assert(!*disabled);
}

// LeetCode 1：找不到時用 optional 表達，而不是 magic pair。
std::optional<Indices> leetcode_two_sum(const std::vector<int>& values, int target) {
    std::unordered_map<int, std::size_t> seen;
    seen.reserve(values.size());
    for (std::size_t index = 0; index < values.size(); ++index) {
        // 先提升到 long long，避免 target-values[index] 的 signed overflow UB。
        const long long wanted = static_cast<long long>(target)
                               - static_cast<long long>(values[index]);
        if (wanted >= static_cast<long long>(std::numeric_limits<int>::min()) &&
            wanted <= static_cast<long long>(std::numeric_limits<int>::max())) {
            if (const auto found = seen.find(static_cast<int>(wanted)); found != seen.end()) {
                return Indices{found->second, index};
            }
        }
        seen.emplace(values[index], index);
    }
    return std::nullopt;
}

enum class Permission : std::size_t { read, write, deploy, admin };

class PermissionSet {
public:
    void grant(Permission permission) { bits_.set(to_index(permission)); }
    bool has(Permission permission) const { return bits_.test(to_index(permission)); }
    std::size_t count() const noexcept { return bits_.count(); }

private:
    static constexpr std::size_t to_index(Permission permission) {
        return static_cast<std::size_t>(permission);
    }
    std::bitset<4> bits_;
};

struct Metric {
    std::string name;
    double value{};
};

struct Alert {
    std::string message;
    int severity{};
};

using Event = std::variant<Metric, Alert>;

template <typename... Functions>
struct Overloaded : Functions... { using Functions::operator()...; };
template <typename... Functions>
Overloaded(Functions...) -> Overloaded<Functions...>;

std::string render_event(const Event& event) {
    return std::visit(Overloaded{
        [](const Metric& metric) {
            return "metric:" + metric.name + "=" + std::to_string(metric.value);
        },
        [](const Alert& alert) {
            return "alert:" + std::to_string(alert.severity) + ":" + alert.message;
        }
    }, event);
}

class EventQueue {
public:
    template <typename T>
    void push(T&& event) {
        events_.emplace(std::forward<T>(event));
    }

    std::optional<Event> take_front() {
        if (events_.empty()) {
            return std::nullopt;
        }
        Event first = std::move(events_.front());
        events_.pop(); // std::queue 預設 deque，front removal 為 O(1)。
        return first;
    }

    bool empty() const noexcept { return events_.empty(); }

private:
    std::queue<Event> events_;
};

using Metadata = std::unordered_map<std::string, std::any>;

std::string metadata_string(const Metadata& metadata, const std::string& key) {
    const auto found = metadata.find(key);
    if (found == metadata.end()) {
        return "missing";
    }
    if (const auto* text = std::any_cast<std::string>(&found->second)) {
        return *text;
    }
    if (const auto* number = std::any_cast<int>(&found->second)) {
        return std::to_string(*number);
    }
    return "unsupported";
}

// 【實務整合】權限守衛 + variant 事件 + any metadata + move/forward FIFO queue。
void practical_event_pipeline_test() {
    PermissionSet permissions;
    permissions.grant(Permission::read);
    permissions.grant(Permission::deploy);
    assert(permissions.has(Permission::deploy));
    assert(permissions.count() == 2U);

    EventQueue queue;
    Metric metric{"gpu_temp", 72.5};
    queue.push(metric);                    // lvalue -> copy
    queue.push(Alert{"overheat", 2});    // rvalue -> move

    const std::optional<Event> first = queue.take_front();
    assert(first.has_value());
    assert(render_event(*first).starts_with("metric:gpu_temp=72.5"));
    const std::optional<Event> second = queue.take_front();
    assert(second.has_value());
    assert(render_event(*second) == "alert:2:overheat");
    assert(queue.empty());
    const std::optional<Event> absent = queue.take_front();
    assert(!absent.has_value()); // 空 queue 不呼叫 front/pop，因此沒有 UB。

    const Metadata metadata{{"owner", std::string{"ops"}}, {"retries", 3}};
    assert(metadata_string(metadata, "owner") == "ops");
    assert(metadata_string(metadata, "retries") == "3");

    // exchange 回舊狀態並設定新狀態。
    std::string state = "queued";
    const std::string previous = std::exchange(state, std::string{"running"});
    assert(previous == "queued" && state == "running");
}

int main() {
    pair_tuple_optional_demo();

    const auto answer = leetcode_two_sum({2, 7, 11, 15}, 9);
    assert(answer.has_value());
    assert(answer->first == 0U && answer->second == 1U);
    assert(!leetcode_two_sum({1, 2, 3}, 99));

    practical_event_pipeline_test();

    // swap 與 as_const 的最小確認。
    std::string active = "v1";
    std::string staging = "v2";
    using std::swap;
    swap(active, staging);
    assert(active == "v2" && staging == "v1");
    const std::string& read_only = std::as_const(active);
    assert(read_only == "v2");

    std::cout << "Utility summary：所有整合測試通過\n";
}

/*
 * 【最後練習】
 * 1. 將 Metadata 從 any 改成 variant<int,string>，刪除 unsupported runtime 分支。
 * 2. 比較 std::queue<Event> 與直接使用 deque<Event>；何時需要 iterator/中間刪除？
 * 3. 替 Event 增加 Email alternative；故意不改 visitor，觀察 compile-time exhaustive 錯誤。
 * 4. 為 PermissionSet 加 union/intersection，說明 bitset 運算複雜度。
 * 5. 將 leetcode_two_sum 的回傳改 named struct，和 optional<pair> 比較可讀性。
 */
