// =============================================================================
//  07_default_delete.cpp  —  = default 與 = delete (C++11)
// =============================================================================
//  參考：
//    https://en.cppreference.com/w/cpp/language/default_initialization
//    https://en.cppreference.com/w/cpp/language/function#Deleted_functions
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、= default                                              │
//  └────────────────────────────────────────────────────────────┘
//
//  「明寫請編譯器幫我產生 default 版本」 — 對「特殊成員函式」(special
//  member functions) 適用：
//
//      class T {
//      public:
//          T() = default;
//          T(const T&) = default;
//          T& operator=(const T&) = default;
//          T(T&&) noexcept = default;
//          T& operator=(T&&) noexcept = default;
//          ~T() = default;
//      };
//
//  為什麼要明寫？
//   1) 提示讀者「這幾個我有意義保留 default」 — 文件作用
//   2) 跟「= delete」對照時清楚
//   3) 有些情況下「自己寫了某些成員函式」會抑制編譯器自動產生其它的；
//      = default 把它們補回來
//
//  例如：
//      class Foo {
//      public:
//          Foo(int x);              // 自寫 ctor → 編譯器不再生 default
//          Foo() = default;         // 明寫補回 default ctor
//      };
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、= delete                                               │
//  └────────────────────────────────────────────────────────────┘
//
//  「明寫禁止這個函式」 — 任何呼叫直接 compile error。
//
//  最常見：禁止 copy（讓 class 變成 move-only）：
//
//      class FileHandle {
//      public:
//          FileHandle(const FileHandle&) = delete;
//          FileHandle& operator=(const FileHandle&) = delete;
//          // move 自動產生
//      };
//
//  也可以禁止特定 overload：
//
//      void process(int);
//      void process(double) = delete;   // 明禁傳 double — 你必須自己 cast
//
//  比 C++98 的「把宣告放 private 但不實作」更明確、錯誤訊息更好。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、什麼時候需要 = default？                               │
//  └────────────────────────────────────────────────────────────┘
//
//   * 你寫了 user-declared ctor，編譯器停止自動生 default ctor → 想要回來
//   * 你想顯式宣告 ctor 是 noexcept、constexpr 等屬性
//   * 文件作用 — 明寫意圖比留白好讀
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 四、本檔示範                                               │
//  └────────────────────────────────────────────────────────────┘
//
//   * Demo 1：FileHandle move-only
//   * Demo 2：用 = delete 禁止特定 overload
//   * Demo 3：明寫 = default 補回 default ctor
// =============================================================================

/*
補充筆記：default_delete
  - default_delete 是現代 C++ 語法或標準庫特性；學習時要把「少寫字」和「語意更精確」分開看。
  - auto 讓型別由初始化式推導，但會丟掉 top-level const/reference；需要保留引用語意時要寫 auto&、const auto& 或 decltype(auto)。
  - brace initialization 能減少未初始化與 narrowing，但遇到 initializer_list overload 可能選到不同建構子。
  - constexpr、static_assert、if constexpr 把部分錯誤和計算提前到編譯期，能讓 template 和常數邏輯更清楚。
  - 屬性如 [[nodiscard]]、[[maybe_unused]]、[[fallthrough]] 是對編譯器和讀者的意圖標記，不應拿來掩蓋設計問題。
  - string_view、optional、variant、structured binding 等特性改善介面表達力，但也帶來生命週期或狀態檢查責任。
  - = default 要求編譯器產生預設特殊成員函式，能讓意圖比空函式本體更清楚。
  - = delete 可明確禁止複製、轉型或特定 overload，錯誤會在編譯期被抓出。
*/
#include <iostream>
#include <string>
#include <utility>

// 模擬「擁有資源」的 RAII 類別 — move-only
class FileHandle {
public:
    FileHandle() = default;
    explicit FileHandle(std::string n) : name_(std::move(n)) {
        std::cout << "  [+] FileHandle(" << name_ << ")\n";
    }
    ~FileHandle() {
        if (!name_.empty()) std::cout << "  [-] FileHandle(" << name_ << ")\n";
    }

    // 禁止 copy
    FileHandle(const FileHandle&) = delete;
    FileHandle& operator=(const FileHandle&) = delete;

    // 顯式啟用 move
    FileHandle(FileHandle&& o) noexcept : name_(std::move(o.name_)) {
        std::cout << "  [moved] FileHandle\n";
    }
    FileHandle& operator=(FileHandle&& o) noexcept {
        name_ = std::move(o.name_);
        return *this;
    }

private:
    std::string name_;
};

// 禁止特定 overload — 強迫呼叫者明寫 cast
void process(int v) { std::cout << "  process(int) v=" << v << '\n'; }
void process(double) = delete;          // ❌ 明禁
// void process(char)   = delete;       // 也可以禁掉 char

class Foo {
public:
    Foo(int v) : v_(v) {}                // 自寫 ctor → 抑制 default ctor
    Foo() = default;                     // 明寫補回 default ctor
    int v_ = 0;
};

int main() {
    // ─────────────────────────────────────────────────────────
    // Demo 1：FileHandle move-only
    // ─────────────────────────────────────────────────────────
    {
        FileHandle a{"data.bin"};
        // FileHandle b = a;       // ❌ 編譯錯（copy 被刪）
        FileHandle c = std::move(a); // ✅ move OK
        (void)c;
    } // c 析構印 [-]

    // ─────────────────────────────────────────────────────────
    // Demo 2：禁特定 overload
    // ─────────────────────────────────────────────────────────
    process(42);                  // OK
    // process(3.14);             // ❌ 編譯錯
    process(static_cast<int>(3.14));   // 強迫 caller 明寫意圖

    // ─────────────────────────────────────────────────────────
    // Demo 3：default ctor 被補回
    // ─────────────────────────────────────────────────────────
    Foo f1;                       // ✅ default ctor
    Foo f2{99};                   // user ctor
    std::cout << "[Demo3] f1.v=" << f1.v_ << " f2.v=" << f2.v_ << '\n';

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：「特殊成員函式」是什麼？
    //    A：6 個編譯器可能自動產生的：
    //         default ctor、destructor、copy ctor、copy assign、
    //         move ctor、move assign。
    //       規則複雜（Rule of 5/0/3）— 寫了其中之一，其它的自動產生會被
    //       抑制；要回來就 = default。
    //
    //  Q2：= delete 一定要 public 嗎？
    //    A：不一定 — private 也行。但慣例放 public，這樣呼叫端錯誤訊息更
    //       清楚（會說「函式被刪除」而不是「無法存取」）。
    //
    //  Q3：跟 C++98 的「private + 未實作」差在哪？
    //    A：(a) 錯誤訊息更明確 — 直接說 deleted
    //       (b) 編譯期擋下，不需要 link 階段
    //       (c) 可以用在「非成員函式」上（C++98 招式只能用在成員函式）
    //
    // ─────────────────────────────────────────────────────────
    // 實用範例：強型別 UserId — 用 = delete 禁止「危險」隱式轉換
    //   工作上常見：避免把「int 帳號 ID」傳到吃「int 金額」的 API。
    // ─────────────────────────────────────────────────────────
    class UserId {
    public:
        explicit UserId(int v) : v_(v) {}
        int value() const { return v_; }
        // 禁掉跟 double 互傳 — 強迫 caller 想清楚意圖
        UserId(double) = delete;
        UserId(bool)   = delete;     // 防 "UserId u(true)" 這種錯
    private:
        int v_;
    };
    UserId u{123};
    // UserId u2{3.14};   // ❌ 編譯錯：刪掉 double ctor
    // UserId u3{true};   // ❌ 編譯錯：刪掉 bool ctor
    std::cout << "[Demo4] UserId.value() = " << u.value() << '\n';

    // ─────────────────────────────────────────────────────────
    // 實用範例 2：NonCopyable mixin（pre-Rule-of-5 寫法）
    //   工作上常見：繼承這個就拿到「禁拷貝」的設計
    // ─────────────────────────────────────────────────────────
    class NonCopyable {
    public:
        NonCopyable() = default;
        ~NonCopyable() = default;
        NonCopyable(const NonCopyable&) = delete;
        NonCopyable& operator=(const NonCopyable&) = delete;
    };
    class Mutex : private NonCopyable {       // 繼承後自動禁拷貝
    public:
        void lock()   { std::cout << "  mutex lock\n"; }
        void unlock() { std::cout << "  mutex unlock\n"; }
    };
    Mutex m;
    m.lock(); m.unlock();
    // Mutex m2 = m;   // ❌ 編譯錯：copy 被刪掉
    return 0;
}
