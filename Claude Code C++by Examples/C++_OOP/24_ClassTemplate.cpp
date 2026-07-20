/*=============================================================================
 * 檔名：24_ClassTemplate.cpp
 * 主題：類別模板 (Class Template) - 寫一個能裝任何型別的容器
 * 適合：學完前面所有 OOP 基礎，準備理解 STL 容器原理的人
 *
 * 【課題介紹】
 *   你已經會寫 IntStack (只能裝 int)，但如果有人想要 StringStack、DoubleStack？
 *   抄三份程式碼是下下策。模板 (template) 就是 C++ 給的解法：
 *
 *       「模板 = 一個『等待型別填入』的程式碼藍圖。
 *        用的時候才把 T 換成 int / string / 任何你想要的型別。」
 *
 *   有了模板，我們可以寫一次 Stack<T>，使用者寫 Stack<int> / Stack<std::string>
 *   就會自動產生對應版本，這個過程叫做「模板實例化 (template instantiation)」。
 *
 * 【寫法】
 *   template <typename T>          // 也可以寫 template <class T>，意思一樣
 *   class Stack {
 *       std::vector<T> data_;
 *   public:
 *       void push(const T& x);
 *       T    pop();
 *   };
 *
 *   使用：
 *       Stack<int> s;
 *       Stack<std::string> ss;
 *
 * 【為什麼通常都寫在 .h 裡？】
 *   模板的「定義」必須讓使用者看到，編譯器才能根據實際型別產生對應版本。
 *   若你把實作放在 .cpp，使用者引用 .h 時編譯器看不到實作 → 連結錯誤。
 *   慣例做法：把宣告與定義都寫在同一個 .h；或寫成 .hpp / .ipp 之類的純標頭。
 *
 * 【模板成員函式在 class 外定義的語法】
 *   template <typename T>
 *   void Stack<T>::push(const T& x) { ... }   // 注意有 template<>、Stack<T>::
 *   寫起來囉嗦但其實有規律。
 *
 * 【兩個常見細節】
 *   1. 模板參數可以有預設值：
 *        template <typename T = int> class Foo;
 *        Foo<>  → 用預設 int
 *
 *   2. 一個類別模板可以有多個型別參數：
 *        template <typename K, typename V> class Pair;
 *
 * 【函式模板 (順帶一提)】
 *   template <typename T>
 *   T max(const T& a, const T& b) { return a < b ? b : a; }
 *
 *   呼叫時通常不必明寫 <T>，編譯器會自己推斷。
 *
 * 【日常實用範例】
 *   寫一個 Stack<T>，再用 int / std::string 兩種型別實例化看看。
 *
 * 【參考】
 *   https://en.cppreference.com/w/cpp/language/class_template
 *=============================================================================*/

/*
補充筆記：ClassTemplate
  - ClassTemplate 這類 OOP 範例要追蹤物件狀態：建構後是否有效、操作後是否仍符合類別承諾。
  - 如果類別擁有資源，就要檢查 destructor、copy、move 是否表達同一套所有權規則。
  - 繼承、friend、static、operator overload 都應服務於清楚的物件語意，而不是只展示語法。
  - class template 是產生類別的藍圖；例如 Box<int> 和 Box<std::string> 是兩個不同具體型別。
  - template 參數可代表型別、整數常數或其他 template；初學先掌握 typename T 代表「稍後才決定的型別」。
  - template 定義通常放在 header，因為編譯器要在使用點看見完整定義才能實例化。
  - 成員函式在 class 外定義時，要同時寫 template <typename T> 和 ClassName<T>::func，少任何一段都會編譯失敗。
  - template 錯誤訊息常很長，因為編譯器會列出實例化鏈；讀錯誤時先找第一個真正不符合語法或需求的型別。
  - class template 可保留值語意，例如 std::vector<T>、std::optional<T>；重點是 T 需要支援類別內用到的操作。
  - 不要在 template 內假設 T 一定有某函式，除非你用 concepts、static_assert 或清楚註釋表明需求。
  - template 讓抽象發生在編譯期，通常沒有 virtual dispatch 成本，但會增加編譯時間與錯誤訊息複雜度。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】class template
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 template 的定義通常要放在 header？
//     答：因為編譯器必須在「使用點」看到完整定義，才能依實際型別產生對應版本
//         （template instantiation）。若把實作放在 .cpp，其他 translation unit
//         只看到宣告 → 實例化不出來 → 連結錯誤（undefined reference）。
//         慣例是宣告與定義都放同一個 .h／.hpp。
//     追問：有例外嗎？（有 —— 若你能列舉所有用到的型別，可以用顯式實例化
//           explicit instantiation 把定義留在 .cpp）
//
// 🔥 Q2. class template 與 function template 有何不同？何時需要 `typename`？
//     答：function template 一直都能從引數推導型別（本檔 myMax(3, 5) 不必寫 <int>）；
//         class template 在 C++17 之前必須顯式指定（`Stack<int> s;`），C++17 起
//         有 CTAD（類別範本引數推導）才能省略。另外在 template 內使用「相依型別」時
//         必須加 typename：`typename T::value_type x;` —— 否則編譯器無法判斷那是型別
//         還是靜態成員。
//     追問：在 class 外定義成員函式要寫什麼？（`template <typename T>` 加上
//           `Stack<T>::`，兩段少任何一段都會編譯失敗，如本檔的 popOrThrow）
//
// Q3. template 的編譯期多型與 virtual 的執行期多型，怎麼選？
//     答：template 在編譯期就把型別決定好，沒有 vptr、沒有間接呼叫，通常也能 inline
//         —— 但會增加編譯時間與程式碼大小，錯誤訊息也難讀，且型別必須編譯期就已知。
//         virtual 則能在執行期替換實作（如第 17 篇的 Strategy），代價是分派成本與
//         介面耦合。判準是：需不需要在「執行期」才決定用哪個實作。
//     追問：怎麼表達「T 必須支援某些操作」？（C++20 的 concepts 最清楚；
//           C++20 之前用 static_assert 或註解約定）
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>

// -----------------------------------------------------------------------------
// 範例 1：Stack<T> 類別模板
// -----------------------------------------------------------------------------
template <typename T>           // T 是型別參數，呼叫端傳入 int / string / Foo 都行
class Stack {
private:
    std::vector<T> data_;

public:
    void push(const T& x) { data_.push_back(x); }

    // 「在 class 內定義」最簡單。也可以拆成 class 外，下方 Stack<T>::popOrThrow 示範。
    bool empty() const { return data_.empty(); }
    size_t size() const { return data_.size(); }

    const T& top() const {
        if (data_.empty()) throw std::runtime_error("空 stack 沒有 top");
        return data_.back();
    }

    void pop() {
        if (data_.empty()) throw std::runtime_error("空 stack 不能 pop");
        data_.pop_back();
    }

    // 在 class 外定義的成員函式 (見下方)，這裡只先宣告
    T popOrThrow();
};

// 「在 class 外定義」的語法：注意 template<typename T> 與 Stack<T>::
template <typename T>
T Stack<T>::popOrThrow() {
    if (data_.empty()) throw std::runtime_error("空 stack");
    T tmp = std::move(data_.back());
    data_.pop_back();
    return tmp;
}

// -----------------------------------------------------------------------------
// 範例 2：兩個型別參數的 Pair (僅示意，正式請用 std::pair)
// -----------------------------------------------------------------------------
template <typename K, typename V>
class Pair {
public:
    K first;
    V second;

    Pair() = default;
    Pair(K k, V v) : first(std::move(k)), second(std::move(v)) {}

    void show() const {
        std::cout << "(" << first << ", " << second << ")\n";
    }
};

// -----------------------------------------------------------------------------
// 範例 3：函式模板 - 順帶一提
// -----------------------------------------------------------------------------
template <typename T>
T myMax(const T& a, const T& b) {
    return (a < b) ? b : a;
}

// -----------------------------------------------------------------------------
// 範例 4：對應 Leetcode 232 - Queue<T> using two stacks
// -----------------------------------------------------------------------------
// 把 LC 232 從「只能裝 int」變成「裝任何型別」的 Queue<T>，
// 這就是把資料結構模板化的典型練習。
template <typename T>
class MyTemplateQueue {
private:
    std::vector<T> in_;       // 進站
    std::vector<T> out_;      // 出站
    void shift() {
        while (!in_.empty()) {
            out_.push_back(std::move(in_.back()));
            in_.pop_back();
        }
    }
public:
    void push(const T& x) { in_.push_back(x); }
    T pop() {
        if (out_.empty()) shift();
        T v = std::move(out_.back());
        out_.pop_back();
        return v;
    }
    bool empty() const { return in_.empty() && out_.empty(); }
};

// -----------------------------------------------------------------------------
// 範例 5：日常實用 - Result<T> 包裝「成功值或錯誤訊息」
// -----------------------------------------------------------------------------
// 模仿 Rust 的 Result/Either，工作上常拿來代替例外或 (bool, T) 二元組。
template <typename T>
class Result {
private:
    bool         ok_;
    T            value_;
    std::string  errMsg_;
public:
    static Result success(T v) {
        Result r;
        r.ok_ = true;
        r.value_ = std::move(v);
        return r;
    }
    static Result failure(const std::string& msg) {
        Result r;
        r.ok_ = false;
        r.errMsg_ = msg;
        return r;
    }

    bool ok() const { return ok_; }
    const T& value() const { return value_; }
    const std::string& error() const { return errMsg_; }
};

int main() {
    std::cout << "===== Stack<int> =====" << std::endl;
    Stack<int> si;
    si.push(1); si.push(2); si.push(3);
    std::cout << "top = " << si.top() << ", size = " << si.size() << "\n";
    std::cout << "pop → " << si.popOrThrow() << "\n";   // 3
    std::cout << "top after pop = " << si.top() << "\n";   // 2

    std::cout << "===== Stack<std::string> =====" << std::endl;
    Stack<std::string> ss;
    ss.push("apple"); ss.push("banana");
    std::cout << "top = " << ss.top() << "\n";

    std::cout << "===== Pair<K,V> =====" << std::endl;
    Pair<std::string, int> age("Alice", 30);
    age.show();
    Pair<int, double> p2(7, 3.14);
    p2.show();

    std::cout << "===== 函式模板 myMax =====" << std::endl;
    std::cout << myMax(3, 5) << "\n";             // 5  (T 推斷為 int)
    std::cout << myMax(2.5, 1.7) << "\n";         // 2.5 (T 推斷為 double)
    std::cout << myMax<std::string>("ab", "ac") << "\n";  // ac

    std::cout << "===== Leetcode 232 模板化 Queue<T> =====" << std::endl;
    MyTemplateQueue<int> qi;
    qi.push(1); qi.push(2); qi.push(3);
    std::cout << "Queue<int> pop = " << qi.pop() << std::endl;        // 1
    std::cout << "Queue<int> pop = " << qi.pop() << std::endl;        // 2

    MyTemplateQueue<std::string> qs;
    qs.push("first"); qs.push("second");
    std::cout << "Queue<string> pop = " << qs.pop() << std::endl;     // first

    std::cout << "===== Result<T> 用法示範 =====" << std::endl;
    auto r1 = Result<int>::success(42);
    auto r2 = Result<int>::failure("解析錯誤");
    if (r1.ok()) std::cout << "成功，值 = " << r1.value() << std::endl;
    if (!r2.ok()) std::cout << "失敗，原因 = " << r2.error() << std::endl;

    auto r3 = Result<std::string>::success("Alice");
    if (r3.ok()) std::cout << "字串結果 = " << r3.value() << std::endl;
    return 0;
}

/* 預期輸出：
 * ===== Stack<int> =====
 * top = 3, size = 3
 * pop → 3
 * top after pop = 2
 * ===== Stack<std::string> =====
 * top = banana
 * ===== Pair<K,V> =====
 * (Alice, 30)
 * (7, 3.14)
 * ===== 函式模板 myMax =====
 * 5
 * 2.5
 * ac
 * ===== Leetcode 232 模板化 Queue<T> =====
 * Queue<int> pop = 1
 * Queue<int> pop = 2
 * Queue<string> pop = first
 * ===== Result<T> 用法示範 =====
 * 成功，值 = 42
 * 失敗，原因 = 解析錯誤
 * 字串結果 = Alice
 */

/*=============================================================================
 * 【本篇重點回顧】
 *   1. template <typename T> 開啟一個型別參數，class 內可用 T 當任何型別占位。
 *   2. 編譯時 (compile-time) 會把 Stack<int>、Stack<string> 各產生一份。
 *   3. 模板的定義通常要放在標頭檔，否則使用者那邊看不到實作。
 *   4. 模板成員函式在 class 外定義要寫 template<typename T> + Stack<T>::。
 *   5. STL 大量使用模板：vector<int>, map<string, int>, unique_ptr<Foo>。
 *
 * 【下一篇預告】
 *   25_StlWithCustomClass.cpp
 *   把自訂類別放進 STL 容器 (vector / map / set) 該注意什麼？
 *   並用 Leetcode 1845. Seat Reservation Manager 練習。
 *=============================================================================*/
