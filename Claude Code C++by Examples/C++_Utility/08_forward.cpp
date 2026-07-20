/*
================================================================================
主題:std::forward —— 完美轉發(perfect forwarding)
標準:C++11 起
標頭:<utility>
參考:https://en.cppreference.com/w/cpp/utility/forward
================================================================================

【一、課題介紹】
  std::forward 解決的是「轉發函式參數」時的型別資訊保持問題。

  情境:你寫一個工廠函式 wrap(arg),要把 arg 原封不動丟給 inner(arg)。
        如果使用者傳的是右值(rvalue,可被搬),你也希望 inner 收到右值;
        如果使用者傳的是左值(lvalue),你希望 inner 收到左值。
        如何讓 wrap 自己「不動」這個 arg 的左/右值身分?
        答案:用「forwarding reference」(T&&)+ std::forward<T>(arg)。

  記憶口訣:
    - **std::move 是「無條件丟成右值」**(我說了算,把它當搬走)。
    - **std::forward 是「有條件丟成右值」**(原本是右值才丟成右值)。

【二、觀念解釋】
  1. Forwarding reference(又稱 universal reference):
       template <class T>
       void wrap(T&& arg) { ... }
     當 T 是 templated 而且寫成 T&& 時,T&& 不一定是右值參考 ——
     會根據傳入的實際值「折疊規則(reference collapsing)」推導:
       - 傳左值:T 推導為 U&,arg 是 U&
       - 傳右值:T 推導為 U,arg 是 U&&

  2. 在函式內,arg 本身永遠是「具名變數」,所以是左值。
     若你直接 inner(arg),即使外面傳右值,也會以左值傳給 inner,失真。

  3. 解法:用 std::forward<T>(arg)。它的效果:
       - 如果 T 是 U&(原本是左值傳進來)→ forward 回傳左值參考,inner 收到左值。
       - 如果 T 是 U(原本是右值傳進來)→ forward 回傳右值參考,inner 收到右值。

  4. **必須**寫 std::forward<T>(arg) —— 不要漏寫 <T>,也不要拿 std::move 替代。

【三、常見陷阱】
  - 寫 forward 漏掉 <T> → 退化成普通拷貝。
  - 對非 forwarding reference 的參數寫 forward → 沒意義。
    (例如函式參數寫 const T& 就不需要 forward。)
  - 對「同一個變數」forward 兩次:第二次拿到的是已被搬走的物件。

【四、與其他 utility 的比較】
  - vs std::move:move 是「我硬要當搬走」;forward 是「保持原本身分」。
  - 大量在 STL 內部出現:make_shared、emplace_back、bind、function 等
    都用 forward 把使用者參數無損地轉給內部建構子。

【五、Leetcode 對應題目】
  題號:這類「工具型」utility 在 Leetcode 上沒有直接對應題目,因此這裡用一個
        類 Leetcode 風格的小題目示範:寫一個「呼叫器(invoker)」函式,
        接收一個函式 + 任意參數,完美轉發呼叫之並回傳結果。
        類似 LC 中常出現的「設計題」做法。
  選用理由:這就是 std::invoke / std::bind / std::function 的核心原理。

【六、日常工作實用範例】
  情境:寫一個自製的 makeShared<T>(args...),把參數完美轉發給 T 的建構子,
        類似 std::make_shared 的內部行為。
================================================================================
*/

/*
補充筆記：std::forward
  - std::forward 只應用在 forwarding reference 場景，用來保留呼叫端原本的 lvalue/rvalue 屬性。
  - forward<T>(x) 的 T 必須是函式模板推導出的 T，不是隨便填的型別。
  - 把 forward 寫成 move 會破壞 lvalue 呼叫者的語意，造成不必要搬移。
  - std::forward 屬於 utility 類工具；這些型別與函式常用來表達小型資料組合、可選值、型別安全聯集或值類別轉換。
  - pair/tuple 適合簡短聚合結果，但欄位語意複雜時應定義具名 struct，避免 first/second 或 get<0> 難讀。
  - optional 表示可能沒有值，使用前要檢查 has_value 或使用 value_or；value() 在無值時會丟例外。
  - variant 表示多選一型別，應用 visit 或 holds_alternative/get_if 安全存取目前替代項。
  - any 提供執行期任意型別保存，但取回需要知道正確型別；過度使用會失去靜態型別檢查優勢。
  - std::move/std::forward/std::exchange/as_const 都是表達意圖的工具；它們本身不一定搬移或複製資料。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::forward 與完美轉發
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::forward 和 std::move 差在哪？什麼是完美轉發？
//     答：std::move 無條件轉成右值；std::forward<T> 是有條件的——依模板參數 T 的推導
//     結果決定轉成左值或右值，用來保留原引數的值類別。完美轉發需要三件事同時成立：
//     ① 轉發參考 T&&（T 必須是在該位置被推導的模板參數）② 參考摺疊（& && → &、
//     && && → &&）③ 在使用點寫 std::forward<T>(arg)。
//     追問：為什麼不能直接寫 std::move(arg)？（那會把呼叫端傳進來的左值也搬走，破壞
//     呼叫端的物件）
//
// 🔥 Q2. 哪些寫法是轉發參考，哪些不是？
//     答：template<class T> void f(T&& x) 是轉發參考；auto&& 也是。但
//     void g(std::vector<T>&& v)（T 未在此處被推導）不是，那是右值參考；同理 class
//     模板的成員函式若用了 class 的模板參數 T 寫成 T&&，那也只是右值參考，因為 T 在
//     呼叫成員函式時並沒有被推導。
//
// ⚠️ 陷阱. 同一個轉發參考可以 forward 兩次嗎？
//     答：不可以。std::forward 的語意是「我要把它交出去了」，一旦轉成右值並被下游函式
//     搬走，該物件就處於有效但未指定的狀態；第二次 forward 只會轉發一個已被掏空的物件。
//     為什麼會錯：把 forward 想成「純轉型、沒有副作用」，於是覺得寫幾次都一樣；轉型本身
//     確實沒有副作用，但它授權了下游去搬走內容，那個授權只能給一次。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <utility>
#include <string>
#include <memory>
#include <vector>

// ---------------------------------------------------------------------------
// 輔助:可以印出參數是用什麼方式收到的(左值 / 右值)的小工具
// ---------------------------------------------------------------------------
void inner(std::string& s)        { std::cout << "    inner(lvalue): " << s << "\n"; }
void inner(const std::string& s)  { std::cout << "    inner(const&): " << s << "\n"; }
void inner(std::string&& s)       { std::cout << "    inner(rvalue): " << s << "\n"; }

// ---------------------------------------------------------------------------
// 範例 1:wrap 沒用 forward → 全部退化為左值
// ---------------------------------------------------------------------------
template <class T>
void wrap_no_forward(T&& arg) {
    inner(arg);                                   // arg 在函式內是左值,失真
}

// ---------------------------------------------------------------------------
// 範例 2:wrap 用 forward → 完美轉發
// ---------------------------------------------------------------------------
template <class T>
void wrap_with_forward(T&& arg) {
    inner(std::forward<T>(arg));                  // 保持原本左/右值身分
}

void demo_compare() {
    std::cout << "[demo_compare]\n";
    std::string s = "hello";

    std::cout << "  wrap_no_forward(s)              -> ";
    wrap_no_forward(s);
    std::cout << "  wrap_no_forward(std::move(s))   -> ";
    wrap_no_forward(std::move(s));                // 期望是 rvalue,但會退成 lvalue!
    s = "hello";
    std::cout << "  wrap_with_forward(s)            -> ";
    wrap_with_forward(s);
    std::cout << "  wrap_with_forward(std::move(s)) -> ";
    wrap_with_forward(std::move(s));              // 正確收到 rvalue
}

// ---------------------------------------------------------------------------
// 範例 3:Leetcode 風格設計題 —— 寫一個 Invoker
//
// 接受一個函式 f 與任意數量參數 args...,完美轉發後呼叫之。
// 你可以把它想成 std::invoke 的最小示範。
// ---------------------------------------------------------------------------
template <class F, class... Args>
auto invoker(F&& f, Args&&... args) -> decltype(f(std::forward<Args>(args)...)) {
    return std::forward<F>(f)(std::forward<Args>(args)...);
}

int add(int a, int b) { return a + b; }

void demo_invoker() {
    std::cout << "[demo_invoker]\n";
    int x = invoker(add, 3, 4);                   // 函式指標
    std::cout << "  invoker(add, 3, 4) = " << x << "\n";

    auto mult = [](int a, int b) { return a * b; };
    std::cout << "  invoker(lambda, 5, 6) = " << invoker(mult, 5, 6) << "\n";
}

// ---------------------------------------------------------------------------
// 範例 4:日常工作實用範例 —— 自製 makeShared
//
// 寫一個簡化版的 std::make_shared:把任意參數完美轉發給 T 的建構子。
// 真實 std::make_shared 的差別在於它「一次配置控制塊與物件」,效能更好,
// 不過內部「轉發參數」的精神就跟這裡一模一樣。
// ---------------------------------------------------------------------------
template <class T, class... Args>
std::shared_ptr<T> makeShared(Args&&... args) {
    return std::shared_ptr<T>(new T(std::forward<Args>(args)...));
}

struct User {
    User(int id, std::string name) : id_(id), name_(std::move(name)) {
        std::cout << "    User ctor: id=" << id_ << ", name=" << name_ << "\n";
    }
    int id_;
    std::string name_;
};

void demo_practical_make_shared() {
    std::cout << "[demo_practical_make_shared]\n";
    auto u1 = makeShared<User>(1, std::string("Alice"));    // 右值字串 → 直接搬
    std::string nm = "Bob";
    auto u2 = makeShared<User>(2, nm);                       // 左值 → 拷貝
    std::cout << "  u1 use_count=" << u1.use_count() << "\n";
    std::cout << "  u2 use_count=" << u2.use_count() << "\n";
}

// ---------------------------------------------------------------------------
// 實用範例 (額外):emplace-like helper —— 完美轉發進容器
//
// 工作中常見:寫一個 wrapper 容器, push() 介面要「完美轉發到底層容器的 emplace_back」,
// 否則使用者傳的是 rvalue,中間多一次拷貝就毀了原本的優化。
// ---------------------------------------------------------------------------
template <class T>
class MyBag {
public:
    template <class U>
    void push(U&& v) {
        // 把 v 用 std::forward 轉發給 vector 的 push_back,保留 lvalue/rvalue 身分
        data_.push_back(std::forward<U>(v));
    }
    size_t size() const { return data_.size(); }
    const T& at(size_t i) const { return data_[i]; }
private:
    std::vector<T> data_;
};

void demo_practical_my_bag() {
    std::cout << "[demo_practical_my_bag]\n";
    MyBag<std::string> bag;
    std::string s = "hello";
    bag.push(s);                                    // lvalue -> 拷貝
    bag.push(std::string("world"));                 // rvalue -> 移動
    bag.push("literal");                            // const char[] -> 從 literal 建構
    for (size_t i = 0; i < bag.size(); ++i)
        std::cout << "  bag[" << i << "]=" << bag.at(i) << "\n";
}

int main() {
    demo_compare();
    demo_invoker();
    demo_practical_make_shared();
    demo_practical_my_bag();
    return 0;
}

/*
================================================================================
編譯與執行:
    g++ -std=c++17 -Wall -Wextra 08_forward.cpp -o 08_forward && ./08_forward
================================================================================
*/
