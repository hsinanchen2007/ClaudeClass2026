// =============================================================================
//  完美轉發陷阱 (2)：braced-init-list `{1, 2, 3}` 無法被模板推導
// =============================================================================
//
// 【主題資訊 Information】
//   template<typename T> void wrapper(T&& arg);        // forwarding reference
//   標準版本：forwarding reference / reference collapsing 皆為 C++11
//   標頭檔  ：<utility>（std::forward）、<initializer_list>
//   關鍵規則：braced-init-list 是「語法結構」而非「表達式」，本身沒有型別，
//             因此無法參與 template argument deduction。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼 {1, 2, 3} 沒有型別？】
//   直覺上「{1,2,3} 看起來就是一串 int」，但在 C++ 的型別系統裡，
//   braced-init-list 並不是一個 expression —— 它沒有 type、沒有 value category。
//   它只是一段「初始化語法」，語意要等到「知道要初始化什麼」才確定：
//       std::vector<int> v{1,2,3};   // → 呼叫 initializer_list 建構子
//       int a[]{1,2,3};              // → 陣列聚合初始化
//       std::array<int,3> s{1,2,3};  // → 聚合初始化
//   同樣三個字元序列，落在不同目標型別上代表完全不同的初始化路徑。
//   既然它自己沒有型別，`template<typename T> void wrapper(T&&)` 的 T
//   自然「無從推導」——編譯器不是不想幫忙，是資訊真的不存在。
//
// 【2. 那為什麼 `auto x = {1,2,3};` 可以？】
//   因為那是標準為 auto 特別開的一條例外規則：當 auto 以 copy-list-init
//   的形式被 braced-init-list 初始化時，特別規定推導成
//   std::initializer_list<T>。這條規則「只給 auto」，並沒有推廣到
//   template argument deduction。所以底下兩行結果不同：
//       auto init = {1, 2, 3};   // ✅ 特例：std::initializer_list<int>
//       wrapper({1, 2, 3});      // ❌ 模板沒有這條特例 → deduction 失敗
//   這是 C++ 中「auto 與模板推導規則幾乎相同、但有一個著名例外」的那個例外。
//
// 【3. 為什麼直接呼叫 target({1,2,3}) 又可以？】
//   因為 target 的參數型別是「已知」的 std::vector<int>，不需要推導；
//   編譯器拿 {1,2,3} 去做 list-initialization 即可。
//   完美轉發之所以卡住，正是因為它把「型別決定權」延後到了推導階段。
//
// 【概念補充 Concept Deep Dive】
//   ▸ reference collapsing 是完美轉發的引擎：
//       在 `template<typename T> void wrapper(T&& arg)` 中，
//       - 傳 lvalue（X 型別）→ T 推導為 X&  → T&& = X& && → 摺疊成 X&
//       - 傳 rvalue（X 型別）→ T 推導為 X   → T&& = X&&
//     摺疊規則只有一條要記：「只要有一個 &，結果就是 &」（& 有傳染性），
//     只有 && && 才會得到 &&。T 是否帶 & 就成了「原引數是不是 lvalue」的載體。
//   ▸ std::forward<T> 為什麼一定要寫明模板引數？
//       因為 forward 的工作正是「把 T 裡記錄的那個值類別還原回去」。
//       arg 本身在函式內是具名變數（named variable）→ 永遠是 lvalue，
//       光看 arg 拿不到原始值類別。唯一還留著這份資訊的地方就是 T，
//       所以必須顯式寫 std::forward<T>(arg)。這也是它被刻意設計成
//       無法從引數推導（參數型別是 std::remove_reference_t<T>&）的原因——
//       寫成 std::forward(arg) 直接編譯不過，避免你不小心用錯。
//   ▸ 本例中「能不能編譯」的關卡比 forward 更早：卡在 deduction，
//     連 wrapper 的函式本體都還沒被實例化就已經失敗。
//
// 【注意事項 Pay Attention】
//   1. `wrapper({1,2,3})` 是編譯期錯誤（deduction 失敗），不是執行期問題。
//   2. 例外：若參數明寫成 `std::initializer_list<T>`，則可以推導 T。
//      也就是說「不能推導」的是 T&&，不是 initializer_list 本身。
//   3. `auto init = {1,2,3};` 得到的是 std::initializer_list<int>，
//      把它轉發給收 vector 的函式仍會多一次元素複製（見下方實務範例說明）。
//   4. std::initializer_list 只是「借看」底層陣列，不擁有元素；
//      把它存起來活得比原陣列久會造成懸空（dangling）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】braced-init-list 與完美轉發
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 `wrapper({1,2,3})` 編譯不過，`target({1,2,3})` 卻可以？
//     答：target 的參數型別已知（std::vector<int>），{1,2,3} 直接做
//         list-initialization 即可。wrapper 的 T 必須由引數推導，而
//         braced-init-list 不是表達式、沒有型別，無從推導 → deduction 失敗。
//     追問：那要怎麼讓 wrapper 收 {1,2,3}？
//         → 呼叫端明確建構 wrapper(std::vector<int>{1,2,3})，
//           或替 wrapper 另加一個 std::initializer_list<T> 的多載。
//
// ⚠️ 陷阱. `auto x = {1,2,3};` 能推導，代表模板推導也能推導 —— 對嗎？
//     答：不對。auto 與模板推導規則「幾乎」相同，唯一的著名例外正是這個：
//         auto 對 braced-init-list 有一條特例規則推導成 initializer_list，
//         模板推導沒有這條特例。
//     為什麼會錯：多數人記住的是「auto 就是套模板推導那一套」這句簡化說法，
//         卻沒記住它後面那句「除了 braced-init-list 以外」。
//
// ⚠️ 陷阱. 那把 `auto init = {1,2,3};` 存起來再 wrapper(init) 不就解決了？
//     答：能編譯，但轉發過去的型別是 std::initializer_list<int>，
//         不是 std::vector<int>。呼叫 target 時會再從 list 建一個 vector，
//         元素被逐一「複製」進去——因為 initializer_list 的元素是 const，
//         永遠 move 不走。想省掉這次複製只能一開始就建 vector。
//     為什麼會錯：以為「有轉發就等於零成本」，忽略了 initializer_list
//         的元素是 const 這個關鍵限制。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <utility>
#include <initializer_list>
#include <cstddef>

void target(std::vector<int> v) {
    std::cout << "收到 " << v.size() << " 個元素\n";
}

template<typename T>
void wrapper(T&& arg) {
    target(std::forward<T>(arg));
}

// -----------------------------------------------------------------------------
// 【日常實務範例】服務指標監控：批次上報 latency samples
//   情境：服務每隔數秒把一批延遲取樣丟進上報佇列。呼叫端最自然的寫法是
//         enqueue({12, 15, 9})，但只要 enqueue 是完美轉發模板，這行就
//         編譯不過——這是實務上最常撞到 braced-init-list 推導問題的地方。
//   解法：泛型轉發版負責「零複製吃下已存在的 vector」，另補一個
//         initializer_list 多載負責「字面量寫法」的便利性。
//         兩者並存才是完整的 API 設計。
// -----------------------------------------------------------------------------
class MetricsQueue {
    std::vector<std::vector<int>> batches_;
public:
    // (a) 泛型轉發版：左值 → 複製，右值 → 移動（零複製）
    template<typename V>
    void enqueue(V&& samples) {
        batches_.push_back(std::forward<V>(samples));
    }

    // (b) 便利多載：讓呼叫端可以直接寫 enqueue({12, 15, 9})
    //     這裡是明寫的 initializer_list<int>，不是 T&&，所以推導得動。
    void enqueue(std::initializer_list<int> samples) {
        batches_.emplace_back(samples);   // 元素為 const → 只能複製
    }

    void report() const {
        std::cout << "  佇列共 " << batches_.size() << " 批\n";
        for (std::size_t i = 0; i < batches_.size(); ++i) {
            std::cout << "    batch[" << i << "] size=" << batches_[i].size()
                      << " first=" << (batches_[i].empty() ? -1 : batches_[i][0]) << "\n";
        }
    }
};

int main() {
    std::cout << "=== 直接呼叫：參數型別已知，不需推導 ===\n";
    target({1, 2, 3});      // OK：編譯器直接對 vector<int> 做 list-initialization

    // wrapper({1, 2, 3});  // ❌ 編譯錯誤：模板無法推導 {1, 2, 3} 的型別
                            //    {1, 2, 3} 不是一個表達式，沒有型別

    std::cout << "\n=== 解法 1：呼叫端明確建構 ===\n";
    wrapper(std::vector<int>{1, 2, 3});  // OK：型別確定為 vector<int>（右值 → 移動）

    std::cout << "\n=== 解法 2：先存成變數（注意型別不是 vector）===\n";
    auto init = {1, 2, 3};   // auto 特例規則 → std::initializer_list<int>
    std::cout << "init 的元素個數 = " << init.size() << "\n";
    wrapper(init);           // 可編譯，但轉發的是 initializer_list，
                             // target 會再從它建一個 vector（元素逐一複製）

    std::cout << "\n=== 日常實務：metrics 批次上報 ===\n";
    MetricsQueue q;

    std::vector<int> collected = {31, 28, 45};
    q.enqueue(collected);                 // 左值 → 走 (a)，複製
    std::cout << "  轉發左值後 collected.size() = " << collected.size() << "（未被破壞）\n";

    q.enqueue(std::move(collected));      // 右值 → 走 (a)，移動（零複製）
    q.enqueue({12, 15, 9});               // 字面量 → 走 (b) 便利多載
    q.report();

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 2.7 章：完美轉發 (Perfect Forwarding) — 泛型程式設計的關鍵技術7.cpp" -o fwd07

// === 預期輸出 ===
// === 直接呼叫：參數型別已知，不需推導 ===
// 收到 3 個元素
//
// === 解法 1：呼叫端明確建構 ===
// 收到 3 個元素
//
// === 解法 2：先存成變數（注意型別不是 vector）===
// init 的元素個數 = 3
// 收到 3 個元素
//
// === 日常實務：metrics 批次上報 ===
//   轉發左值後 collected.size() = 3（未被破壞）
//   佇列共 3 批
//     batch[0] size=3 first=31
//     batch[1] size=3 first=31
//     batch[2] size=3 first=12
