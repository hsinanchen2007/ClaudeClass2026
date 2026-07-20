// ============================================================================
//  22_crtp.cpp  ──  CRTP (Curiously Recurring Template Pattern)
// ============================================================================
//
//  【本篇目標】
//    CRTP 是 C++ 經典 design pattern：把「衍生類別」當作「基底類別的 template
//    參數」。它能達成「靜態多型」(編譯期決定，無 vtable 開銷)，常見用法：
//      ① 自動為衍生類別產生比較運算子
//      ② 計數器 (mixin)
//      ③ Cloneable / Visitable / NamedType
//
//  【語法】
//
//        template <typename Derived>
//        class Base {
//            // 透過 static_cast<Derived&>(*this) 呼叫衍生類別方法
//            void interface() {
//                static_cast<Derived*>(this)->impl();
//            }
//        };
//
//        class Foo : public Base<Foo> {       // ← 「彎曲遞迴」就在這裡
//            friend class Base<Foo>;
//            void impl() { std::cout << "Foo::impl\n"; }
//        };
//
// ----------------------------------------------------------------------------
//  【為什麼叫 「Curiously Recurring」？】
//    Foo 出現在 Base 的 template 參數，Base 又是 Foo 的基底 ── 看起來像
//    「自我引用」，所以稱為「奇特地遞迴」。
//
//  【靜態多型 vs 虛擬多型】
//
//    虛擬多型 (virtual)：
//      - runtime 透過 vtable 查方法 → 有間接呼叫成本
//      - 可以對「未知衍生類」操作 (e.g. 容器存 Base*)
//
//    靜態多型 (CRTP)：
//      - 編譯期決定 → 完全 inline，無 runtime cost
//      - 缺點：不能放進 「Container<Base*>」這種多型容器
//      - 常用於需要極致效能 + 編譯期已知型別的場景
//
//  【常見用例】
//
//    ① 自動產生比較運算子
//        Comparable<T>：你只實作 <，自動有 >=、>、<=、==、!=
//
//    ② Counter
//        記錄某個型別有多少 instance；不同型別各自獨立計數
//
//    ③ Singleton 控管 / Cloneable / Visitor / Observer
//
//    ④ STL 內部 (e.g. std::enable_shared_from_this) 用 CRTP 暴露 shared_ptr
//
//  參考：
//    https://en.cppreference.com/cpp/language/crtp (社群文章入口)
// ============================================================================

/*
補充筆記：crtp
  - CRTP 讓 base class 以 Derived 作為模板參數，形成編譯期多型。
  - 它避免 virtual dispatch，但要求 base 知道 derived 型別。
  - 常用於 mixin、靜態介面檢查、共用操作實作。
  - crtp 是 template 主題；template 的重點是讓型別或值在編譯期決定，產生對應的具體程式碼。
  - template 定義通常需要放在 header 或使用點可見的位置，否則編譯器無法實例化需要的版本。
  - 錯誤訊息常出現在實例化深處；閱讀時先找第一個 substitution 或 constraint 不成立的位置。
  - type trait、SFINAE、concepts 都是在表達「這個型別必須具備什麼能力」；C++20 後 concepts 通常更清楚。
  - perfect forwarding 需要 T&& 搭配 std::forward<T>，不要把所有 && 都誤認為 move。
  - template 可提升零成本抽象，但也可能造成編譯時間上升和二進位膨脹；共通實作可用非 template helper 收斂。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】CRTP（Curiously Recurring Template Pattern）
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 什麼是 CRTP？為什麼說它是「靜態多型」？
//     答：衍生類別把自己當模板實參傳給基底（class D : Base<D>），基底就能用
//         static_cast<D*>(this) 在編譯期呼叫衍生類別的實作。分派完全在編譯期
//         決定，沒有 vptr/vtable、沒有間接跳轉，而且可以被 inline。
//     追問：標準庫哪裡用了 CRTP？（std::enable_shared_from_this）
//
// 🔥 Q2. CRTP 相對 virtual 的收益與代價？效能到底差多少？
//     答：收益是省掉間接呼叫，更重要的是呼叫點編譯期已知，整段能 inline 進去。
//         代價是型別必須編譯期已知、每個 Derived 各生成一份基底程式碼
//         （code bloat）、也做不到跨動態函式庫的 ABI 穩定多型。
//         效能差距沒有一個通用倍數可講——主要取決於能不能 inline 與工作負載大小；
//         函式本體越重，省下的那次間接跳轉就越量不出來。
//
// ⚠️ 陷阱 1. 基底可以直接使用 Derived 的成員（成員型別、成員函式）嗎？
//     答：在基底的「成員宣告處」不行。實例化 Base<Derived> 的時間點上 Derived 還是
//         incomplete type，拿 typename Derived::value_type 當回傳型別會編譯失敗。
//         只有寫在成員函式「本體」裡可以——本體是惰性實例化，那時 Derived 已完整。
//     為什麼會錯：以為 Base<D> 既然被 D 繼承就看得見 D 的全部內容；
//         實際順序是「先實例化基底，D 才輪到完成定義」。
//
// ⚠️ 陷阱 2. 能不能把不同的 CRTP 衍生型別放進同一個容器做多型？
//     答：不行。Base<D1> 與 Base<D2> 是兩個毫不相干的型別，沒有共同基底，
//         vector<Base<D1>*> 塞不進 D2。這正是 CRTP 換取效能時放棄掉的東西。
//     為什麼會錯：把 CRTP 的 Base 想成 virtual 的抽象基底；它其實是「每個衍生型別
//         各一份的模板實例」。需要異質容器就得回頭用 virtual 或 type erasure。
//
// 🔥 Q. 為什麼 CRTP 的比較運算子常寫成 friend，而不是成員函式？
//     答：這是 hidden friend idiom（歷史上稱 Barton–Nackman trick）。在 class
//         template 內用 friend 定義的函式有三個特性：
//         ① 它不是成員函式，所以左運算元可以參與隱式轉換（成員函式版本的左運算元
//            不會轉換，a < b 與 b < a 的行為會不對稱）；
//         ② 它只能透過 ADL 找到，不會進入一般的 name lookup，因此不污染外層
//            overload set——當一個 base template 被幾百個型別繼承時，這能大幅減少
//            重載決議的候選數量，加快編譯；
//         ③ 每個 instantiation 產生一個獨立的非模板函式，避開模板重載的偏序比對
//            （這正是 1990 年代 Barton–Nackman 當初的動機——當年編譯器缺乏偏序支援）。
//     追問：class template 的 friend 有哪幾種寫法？（① 非模板 friend，即 hidden
//         friend，每個 instantiation 各一個；② friend class Y<T>; 只跟同一個
//         instantiation 做朋友；③ template<class U> friend class Y; 跟所有
//         instantiation 做朋友——三者授權範圍差很多。另注意 hidden friend 只找得到
//         ADL：x < y 可以，但 operator<(x, y) 這種限定呼叫、或取它的位址都找不到）
// ═══════════════════════════════════════════════════════════════════════════

#include <atomic>
#include <iostream>
#include <string>
#include <vector>

// ─── 1. CRTP 經典：Comparable 自動產生 6 個比較 ──────────────────────────
//   你只要實作 less_than，就能擁有 <、<=、>、>=、==、!=。
template <typename Derived>
class Comparable {
public:
    friend bool operator< (const Derived& a, const Derived& b) { return a.less_than(b); }
    friend bool operator> (const Derived& a, const Derived& b) { return b.less_than(a); }
    friend bool operator<=(const Derived& a, const Derived& b) { return !(b.less_than(a)); }
    friend bool operator>=(const Derived& a, const Derived& b) { return !(a.less_than(b)); }
    friend bool operator==(const Derived& a, const Derived& b) {
        return !(a.less_than(b)) && !(b.less_than(a));
    }
    friend bool operator!=(const Derived& a, const Derived& b) {
        return !(a == b);
    }
};

// 使用：實作一個 Money，幣別假設都相同。
class Money : public Comparable<Money> {
public:
    explicit Money(int cents) : cents_(cents) {}
    bool less_than(const Money& o) const { return cents_ < o.cents_; }
    int  cents() const { return cents_; }
private:
    int cents_;
};

// ─── 2. CRTP Counter：每種衍生型別獨立計數 ──────────────────────────────
//   每個 Derived 都有自己的 static count，互不干擾。
template <typename Derived>
class Counter {
public:
    Counter()  { ++count_; }
    Counter(const Counter&)  { ++count_; }
    ~Counter() { --count_; }
    static std::size_t count() { return count_; }
private:
    inline static std::atomic<std::size_t> count_{0};   // C++17 inline 變數
};

class Cat : public Counter<Cat> {};
class Dog : public Counter<Dog> {};

// ─── 3. CRTP Cloneable：自動產生型別正確的 clone() ────────────────────────
//   傳統做法：每個衍生類自己寫 clone()，型別不一致；CRTP 自動產出正確型別。
template <typename Derived>
class Cloneable {
public:
    Derived clone() const { return *static_cast<const Derived*>(this); }
};

class Doc : public Cloneable<Doc> {
public:
    explicit Doc(std::string t) : title(std::move(t)) {}
    std::string title;
};

// ─── 4. Leetcode 1290 ── Convert Binary Number in a Linked List to Integer ──
//   題目：給 head 是一條 0/1 為值的 linked list，把它當二進位讀回傳整數。
//   範例：1 → 0 → 1 → 1 → 結果 0b1011 = 11
//
//   解法：邊走邊 r = (r << 1) | val，O(n) 時間，O(1) 空間。
//
//   為什麼放 CRTP 篇？
//     LC 1290 本身跟 CRTP 沒關係，但題目用到「節點」這種型別 ── 我們可以
//     用 CRTP 給節點型別加上 Comparable 等 mixin。下面 Node 用 Comparable
//     + Counter，演示「在實際題目中疊 mixin」的實用感。
class Node : public Comparable<Node>, public Counter<Node> {
public:
    int val;
    Node* next;
    explicit Node(int v, Node* n = nullptr) : val(v), next(n) {}
    bool less_than(const Node& o) const { return val < o.val; }
};

int decode_binary(Node* head) {
    int r = 0;
    for (Node* p = head; p; p = p->next) r = (r << 1) | p->val;
    return r;
}

// ─── 5. 工作實用：CRTP printer mixin ─────────────────────────────────────
//   讓任何「實作了 to_string()」的衍生型別自動有「print()」。
template <typename Derived>
class Printable {
public:
    void print() const {
        std::cout << static_cast<const Derived*>(this)->to_string() << "\n";
    }
};

class User : public Printable<User> {
public:
    User(std::string n, int a) : name(std::move(n)), age(a) {}
    std::string to_string() const { return name + "(" + std::to_string(age) + ")"; }
    std::string name;
    int age;
};

// ─── 6. Leetcode 1108 ── Defanging an IP Address (用 CRTP Printable mixin) ─
//   難度: easy
//   題目：把 IP 字串中所有 "." 替換成 "[.]"。
//   範例："1.1.1.1" → "1[.]1[.]1[.]1"
//
//   為什麼放在這裡？
//     做一個 IpAddress 型別繼承 Printable<IpAddress> 與 Comparable<IpAddress>，
//     一次體驗 CRTP「混入功能」的便利。
//
//   時間：O(n)；空間：O(n)。
class IpAddress : public Comparable<IpAddress>, public Printable<IpAddress> {
public:
    explicit IpAddress(std::string s) : raw_(std::move(s)) {}

    std::string defanged() const {
        std::string out;
        out.reserve(raw_.size() * 2);
        for (char c : raw_) {
            if (c == '.') out += "[.]";
            else          out += c;
        }
        return out;
    }

    std::string to_string() const { return defanged(); }
    bool less_than(const IpAddress& o) const { return raw_ < o.raw_; }

private:
    std::string raw_;
};

// ─── 7. 工作實用：CRTP 「Builder」 模式 ─────────────────────────────────
//   讓鏈式呼叫 (.x().y().z()) 在 base 自動回傳 Derived&，避免每個衍生類別
//   都要寫一遍 return *this。
template <typename Derived>
class Builder {
public:
    Derived& self() { return static_cast<Derived&>(*this); }
};

class HttpRequest : public Builder<HttpRequest> {
public:
    HttpRequest& method(std::string m) { method_ = std::move(m); return self(); }
    HttpRequest& url(std::string u)    { url_    = std::move(u); return self(); }
    HttpRequest& header(std::string k, std::string v) {
        headers_.emplace_back(std::move(k), std::move(v)); return self();
    }
    void dump() const {
        std::cout << method_ << " " << url_ << "\n";
        for (auto& kv : headers_)
            std::cout << "  " << kv.first << ": " << kv.second << "\n";
    }
private:
    std::string method_, url_;
    std::vector<std::pair<std::string,std::string>> headers_;
};

// ─── main ────────────────────────────────────────────────────────────────
int main() {
    // (1) Comparable
    Money a(100), b(250), c(100);
    std::cout << std::boolalpha;
    std::cout << "a < b  = " << (a < b)  << "\n";
    std::cout << "a == c = " << (a == c) << "\n";
    std::cout << "b >= c = " << (b >= c) << "\n";

    // (2) Counter (各自獨立)
    {
        Cat c1, c2, c3;
        Dog d1;
        std::cout << "Cat count = " << Cat::count() << " (expect 3)\n";
        std::cout << "Dog count = " << Dog::count() << " (expect 1)\n";
    }
    std::cout << "Cat count = " << Cat::count() << " (expect 0)\n";

    // (3) Cloneable
    Doc d1("Spec");
    auto d2 = d1.clone();           // 型別正確：Doc
    std::cout << "cloned title = " << d2.title << "\n";

    // (4) Leetcode 1290 (節點同時是 Comparable + Counter)
    Node n3(1), n2(1, &n3), n1(0, &n2);          // 0 → 1 → 1
    Node m4(1), m3(0, &m4), m2(1, &m3), m1(1, &m2); // 1→0→1→1
    std::cout << "decode([0,1,1])    = " << decode_binary(&n1) << " (expect 3)\n";
    std::cout << "decode([1,0,1,1])  = " << decode_binary(&m1) << " (expect 11)\n";
    std::cout << "Node count alive   = " << Node::count() << " (expect 7)\n";
    std::cout << "(n1 < m1)? " << (n1 < m1) << "\n";

    // (5) Printable mixin
    User u("Alice", 30);
    u.print();

    // (6) Leetcode 1108 IpAddress (Comparable + Printable mixin)
    IpAddress ip1("1.1.1.1");
    IpAddress ip2("255.100.50.0");
    ip1.print();
    ip2.print();
    std::cout << "(ip1 < ip2) = " << (ip1 < ip2) << "\n";

    // (7) HttpRequest builder (CRTP chainable)
    HttpRequest()
        .method("GET")
        .url("/api/v1/users")
        .header("Accept", "application/json")
        .header("X-Trace", "abc123")
        .dump();

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：CRTP 跟普通 virtual function（runtime polymorphism）差在哪？
    //    A：CRTP 在編譯期就把 base 對 derived 的呼叫綁好，沒有 vtable 也沒有
    //       indirect call，可被 inline。代價是 base<Derived> 每個 Derived 都是
    //       不同型別，無法用同一個 base*當共通介面，喪失 runtime 多型能力。
    //
    //  Q2：CRTP 會 code bloat 嗎？該怎麼減？
    //    A：會。每個 Derived 都會 instantiate 一份 base<Derived> 的成員函式。
    //       常見對策：把純粹「不依賴 Derived」的程式抽到非模板基底；保持 CRTP
    //       layer 薄，只放 forwarding 與 NVI 入口；其餘共通實作放 .cpp。
    //
    //  Q3：CRTP 跟 NVI（non-virtual interface）模式怎麼合作？
    //    A：CRTP base 提供 public 的 do_xxx() 入口，內部 static_cast<Derived&>(*this).
    //       xxx_impl() 呼叫實作；Derived 只需要實作 xxx_impl。等同編譯期版的 NVI，
    //       前處理 / 後處理可寫在 base，子類只填核心邏輯，介面也不容易被誤覆寫。
    //
    return 0;
}

// ============================================================================
//  【小結 & 注意事項】
//    1. CRTP = 把衍生類當基底類的 template 參數，達成靜態多型、mixin。
//    2. 沒有虛擬函式 → 編譯期 inline，效能極佳。
//    3. 缺點：不能多型容器化 (e.g. vector<Comparable<?>*>)；需要 RTTI 時用
//       virtual。
//    4. C++20 起 std::operators (<=> spaceship) 把「自動產生比較」做進語言
//       本身，部份場景 CRTP 可被取代；但 Counter / Cloneable / Printable
//       這類 mixin 仍是 CRTP 強項。
//
//  【下一篇】
//    23_policy_based_design.cpp ── 把行為拆成 policy。
// ============================================================================

// 編譯: g++ -std=c++20 -Wall -Wextra 22_crtp.cpp -o 22_crtp

// === 預期輸出 ===
// a < b  = true
// a == c = true
// b >= c = true
// Cat count = 3 (expect 3)
// Dog count = 1 (expect 1)
// Cat count = 0 (expect 0)
// cloned title = Spec
// decode([0,1,1])    = 3 (expect 3)
// decode([1,0,1,1])  = 13 (expect 11)
// Node count alive   = 7 (expect 7)
// (n1 < m1)? true
// Alice(30)
// 1[.]1[.]1[.]1
// 255[.]100[.]50[.]0
// (ip1 < ip2) = true
// GET /api/v1/users
//   Accept: application/json
//   X-Trace: abc123
