// ============================================================
// 第 2.2 章 總結：右值參考 && — 基本語法與用途
// 編譯：g++ -std=c++17 -o summary summary.cpp
// ============================================================
// 【三種引用的綁定規則】
//   T&        只能綁定 lvalue
//   const T&  可綁定 lvalue 和 rvalue（萬能讀取）
//   T&&       只能綁定 rvalue
//
// 【右值參考的特性】
//   1. 綁定後延長臨時物件的生命週期（和 const T& 一樣）
//   2. 可以修改綁定的值（const T& 做不到！）
//   3. 右值參考變數本身是 lvalue！（有名字 → 是 lvalue）
//
// 【函數重載：const T& vs T&&】
//   傳入 lvalue → const T& 版本
//   傳入 rvalue → T&& 版本（更精確匹配）
//   這是實現「拷貝 vs 移動」語義的基礎
//
// 【重要陷阱：右值參考參數在函數內是 lvalue】
//   void func(string&& s) {
//       inner(s);            // const T& 版本！s 是 lvalue
//       inner(std::move(s)); // T&& 版本！需要 std::move 轉回 rvalue
//   }
//
// 【絕不能回傳局部變數的右值參考！】
//   string&& bad() { string s = "x"; return move(s); }  // ❌ 懸空參考！
//   string good() { string s = "x"; return s; }          // ✅ 回傳值 + RVO
// ============================================================

#include <iostream>
#include <string>
#include <cstring>
#include <utility>
using namespace std;

// ============================================================
// 函數重載：const T& vs T&&
// ============================================================
void process(const string& s) {
    cout << "  [const T& 版本] \"" << s << "\"\n";
}
void process(string&& s) {
    cout << "  [T&& 版本]      \"" << s << "\"\n";
}

// ============================================================
// 右值參考參數在函數內是 lvalue
// ============================================================
void inner(const string& s) { cout << "    inner: const T&\n"; }
void inner(string&& s)      { cout << "    inner: T&&\n"; }

void outer(string&& s) {
    cout << "  直接傳 s（s 是 lvalue！）：\n";
    inner(s);               // const T& 版本！

    cout << "  傳 std::move(s)：\n";
    inner(std::move(s));    // T&& 版本！
}

// ============================================================
// Buffer 類別：展示移動 vs 複製的效能差異
// ============================================================
class Buffer {
    char* data_;
    size_t size_;
public:
    Buffer(const char* str) : size_(strlen(str)) {
        data_ = new char[size_ + 1];
        strcpy(data_, str);
        cout << "    [建構] \"" << data_ << "\"\n";
    }
    ~Buffer() {
        if (data_) cout << "    [解構] \"" << data_ << "\"\n";
        else       cout << "    [解構] (空)\n";
        delete[] data_;
    }
    Buffer(const Buffer& o) : size_(o.size_) {
        data_ = new char[size_ + 1];
        strcpy(data_, o.data_);
        cout << "    [複製建構💰] \"" << data_ << "\"\n";
    }
    Buffer(Buffer&& o) noexcept : data_(o.data_), size_(o.size_) {
        o.data_ = nullptr; o.size_ = 0;
        cout << "    [移動建構⚡] \"" << data_ << "\"\n";
    }
    const char* c_str() const { return data_ ? data_ : "(null)"; }
};

void store(const Buffer& buf) {
    cout << "  store(const Buffer&)：\n";
    Buffer local(buf);  // 複製建構
}
void store(Buffer&& buf) {
    cout << "  store(Buffer&&)：\n";
    Buffer local(std::move(buf));  // 移動建構
}

// ============================================================
// 生命週期延長
// ============================================================
class Verbose {
    string name_;
public:
    Verbose(const string& n) : name_(n) { cout << "    [建構] " << name_ << "\n"; }
    ~Verbose() { cout << "    [解構] " << name_ << "\n"; }
    const string& name() const { return name_; }
};
Verbose make_object() { return Verbose("臨時物件"); }

int main() {
    // ============================================================
    // 1. 綁定規則
    // ============================================================
    cout << "===== 1. 綁定規則 =====\n";
    int a = 10;
    int& lref = a;          // ✅ T& → lvalue
    // int& bad = 42;       // ❌ T& → rvalue（不行）

    const int& clref = 42;  // ✅ const T& → rvalue（特殊規則）

    int&& rref = 42;        // ✅ T&& → rvalue
    // int&& bad = a;       // ❌ T&& → lvalue（不行）

    rref = 100;             // ✅ 右值參考可以修改！
    cout << "  rref = " << rref << "\n";
    cout << "  &rref = " << &rref << " ← 可取址，所以 rref 本身是 lvalue\n\n";

    // ============================================================
    // 2. 函數重載
    // ============================================================
    cout << "===== 2. 函數重載 =====\n";
    string name = "Alice";
    process(name);                   // const T& — lvalue
    process(string("Bob"));          // T&& — rvalue（臨時物件）
    process("Charlie");              // T&& — 隱含轉型產生臨時物件
    process(std::move(name));        // T&& — std::move 轉為 rvalue
    cout << "  name after move: \"" << name << "\"\n\n";

    // ============================================================
    // 3. 右值參考參數在函數內是 lvalue（重要陷阱！）
    // ============================================================
    cout << "===== 3. T&& 參數在函數內是 lvalue =====\n";
    outer(string("Hello"));
    cout << "\n";

    // ============================================================
    // 4. 生命週期延長
    // ============================================================
    cout << "===== 4. 生命週期延長 =====\n";
    {
        cout << "  沒綁定 → 臨時物件立即銷毀：\n";
        make_object();
        cout << "  （已銷毀）\n\n";

        cout << "  const T& 綁定 → 延長生命週期：\n";
        const Verbose& cref = make_object();
        cout << "  cref 仍有效：" << cref.name() << "\n\n";

        cout << "  T&& 綁定 → 也延長生命週期（且可修改）：\n";
        Verbose&& rrv = make_object();
        cout << "  rrv 仍有效：" << rrv.name() << "\n";
    }
    cout << "\n";

    // ============================================================
    // 5. 移動 vs 複製的效能差異
    // ============================================================
    cout << "===== 5. 移動 vs 複製 =====\n";
    {
        Buffer original("Important Data Here");

        cout << "\n  傳入 lvalue（複製）：\n";
        store(original);

        cout << "\n  傳入 rvalue（移動）：\n";
        store(Buffer("Temporary Data"));

        cout << "\n  用 std::move（移動）：\n";
        store(std::move(original));
        cout << "  original after move: " << original.c_str() << "\n";
    }

    cout << "\n=== 重點整理 ===\n";
    cout << "  T& → 只綁 lvalue    const T& → 萬能    T&& → 只綁 rvalue\n";
    cout << "  T&& 變數本身是 lvalue（有名字）\n";
    cout << "  函數內要轉發 T&& 參數，必須 std::move\n";
    cout << "  T&& 和 const T& 都能延長臨時物件生命週期\n";
    cout << "  絕不能回傳局部變數的 T&&（懸空參考！）\n";

    return 0;
}
