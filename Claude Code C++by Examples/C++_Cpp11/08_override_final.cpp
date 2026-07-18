// =============================================================================
//  08_override_final.cpp  —  override 與 final 關鍵字 (C++11)
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/language/override
//        https://en.cppreference.com/w/cpp/language/final
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、override 解決什麼問題？                                │
//  └────────────────────────────────────────────────────────────┘
//
//  C++98 寫 virtual function override 時，沒有任何「我想要 override」的標
//  記，極容易因為 typo / 簽名不匹配而「悄悄變成新函式」：
//
//      class Base { public: virtual void onClick() const; };
//      class Btn : public Base {
//      public:
//          void onClick();        // ⚠️ 漏 const → 不是 override，是新函式
//      };
//
//  Btn 物件呼叫 base ref 的 onClick 時走 Base 的版本，bug 隱蔽。
//
//  C++11 起加 override 關鍵字明寫意圖，編譯器會檢查：
//
//      void onClick() override;        // ❌ 編譯錯：base 沒有「non-const」版
//      void onClick() const override;  // ✅ 完全匹配
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、final — 禁止再被 override / 繼承                       │
//  └────────────────────────────────────────────────────────────┘
//
//  function final：禁止子類別 override
//      class Base { virtual void f(); };
//      class Mid : public Base { void f() final; };
//      class Leaf: public Mid  { void f() override; }; // ❌ 編譯錯
//
//  class final：禁止被繼承
//      class Sealed final {};
//      class X : public Sealed {};   // ❌ 編譯錯
//
//  好處：
//   * 表達設計意圖（「不要再擴充」）
//   * 編譯器可能做 devirtualization（直接 inline 呼叫，省 vtable）
//
//  注意：final / override 是 「contextual identifier」 — 它們不是 reserved
//  keyword，可以當作普通變數名（雖然不建議）：
//      int override = 10;     // 合法但詭異
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、實務建議                                               │
//  └────────────────────────────────────────────────────────────┘
//
//   * 子類別的 virtual function 永遠加 override — 抓 typo
//   * 確定不再被 override 的就加 final — 文件作用 + 可能優化
//   * 「全 class 不該被繼承」→ class final
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 四、本檔示範                                               │
//  └────────────────────────────────────────────────────────────┘
//
//   * Demo 1：override 抓出 const 不匹配
//   * Demo 2：final 禁止繼續 override
//   * Demo 3：class final
// =============================================================================

/*
補充筆記：override_final
  - override_final 是現代 C++ 語法或標準庫特性；學習時要把「少寫字」和「語意更精確」分開看。
  - auto 讓型別由初始化式推導，但會丟掉 top-level const/reference；需要保留引用語意時要寫 auto&、const auto& 或 decltype(auto)。
  - brace initialization 能減少未初始化與 narrowing，但遇到 initializer_list overload 可能選到不同建構子。
  - constexpr、static_assert、if constexpr 把部分錯誤和計算提前到編譯期，能讓 template 和常數邏輯更清楚。
  - 屬性如 [[nodiscard]]、[[maybe_unused]]、[[fallthrough]] 是對編譯器和讀者的意圖標記，不應拿來掩蓋設計問題。
  - string_view、optional、variant、structured binding 等特性改善介面表達力，但也帶來生命週期或狀態檢查責任。
  - override 檢查函式簽名真的覆寫 base virtual；少 const 或參數不同會立刻編譯錯。
  - final 放在 virtual function 可禁止再覆寫，放在 class 可禁止繼承。
*/
#include <iostream>
#include <string>
#include <utility>

class Animal {
public:
    virtual ~Animal() = default;
    virtual void say() const = 0;
};

class Dog : public Animal {
public:
    void say() const override {           // ✅ 跟 base 匹配
        std::cout << "woof\n";
    }
    // void say() override;               // ❌ 編譯錯：少 const，不是 override
};

class Mid : public Animal {
public:
    void say() const final {              // ✅ override 並設 final
        std::cout << "mid-final\n";
    }
};

class Leaf : public Mid {
public:
    // void say() const override;         // ❌ 編譯錯：Mid::say 是 final
};

class Sealed final {                       // 整個 class final
public:
    void hello() { std::cout << "hello from Sealed\n"; }
};

// class Tries : public Sealed {};       // ❌ 編譯錯：Sealed final

int main() {
    // ─────────────────────────────────────────────────────────
    // Demo 1：override 確保 virtual 派發
    // ─────────────────────────────────────────────────────────
    Dog d;
    Animal& a = d;
    a.say();                              // 預期 woof — virtual 派發到 Dog

    // ─────────────────────────────────────────────────────────
    // Demo 2：Mid::say final
    // ─────────────────────────────────────────────────────────
    Mid m;
    Animal& am = m;
    am.say();                             // mid-final
    Leaf l;
    Animal& al = l;
    al.say();                             // 仍然走 Mid::say（沒被 override）

    // ─────────────────────────────────────────────────────────
    // Demo 3：Sealed final
    // ─────────────────────────────────────────────────────────
    Sealed s;
    s.hello();

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：override 對 non-virtual 也能加嗎？
    //    A：不行。override 必須對應 base 的 virtual。寫了會編譯錯，這正是
    //       它要的安全性。
    //
    //  Q2：override 跟 virtual 兩個都寫嗎？
    //    A：寫 override 時，virtual 可寫可不寫（隱含繼承為 virtual）。慣例
    //       是「子類別省略 virtual、加 override」，視覺上更乾淨。
    //
    //  Q3：final 對效能有幫助嗎？
    //    A：可能。編譯器看到 final 知道「這條呼叫一定不會 dispatch 到別處」
    //       → 把虛擬呼叫換成直接呼叫（devirtualization），可能 inline。
    //       但實測效能差距通常很小，主要還是當「設計意圖」用。
    //
    // ─────────────────────────────────────────────────────────
    // 實用範例 1：日誌 Writer 介面 — 子類別必須 override
    //   工作上常見：定義 Writer 抽象介面，多種實作（檔案、stdout、network），
    //                每個子類別 override 才不會悄悄變成新函式。
    // ─────────────────────────────────────────────────────────
    class Writer {
    public:
        virtual ~Writer() = default;
        virtual void write(const std::string& line) = 0;
    };
    class StdoutWriter final : public Writer {            // 整個 class final
    public:
        void write(const std::string& line) override {     // ✅ 必寫 override
            std::cout << "[stdout] " << line << '\n';
        }
    };
    class TaggedWriter : public Writer {
    public:
        explicit TaggedWriter(std::string tag) : tag_(std::move(tag)) {}
        void write(const std::string& line) override {     // ✅ 抓 typo
            std::cout << "[" << tag_ << "] " << line << '\n';
        }
    private:
        std::string tag_;
    };
    StdoutWriter sw;
    TaggedWriter tw{"info"};
    Writer* writers[] = {&sw, &tw};
    for (auto* w : writers) w->write("hello override/final");

    // ─────────────────────────────────────────────────────────
    // 實用範例 2：Sealed class — utility 工具集禁止繼承
    //   工作上常見：StringHelpers 之類純靜態工具類用 final 表達「別繼承」
    // ─────────────────────────────────────────────────────────
    class StringHelpers final {                            // 不能被繼承
    public:
        static bool startsWith(const std::string& s, const std::string& p) {
            return s.size() >= p.size() &&
                   s.compare(0, p.size(), p) == 0;
        }
    };
    std::cout << std::boolalpha;
    std::cout << "[Demo5] startsWith(\"hello.cpp\", \"hello\") = "
              << StringHelpers::startsWith("hello.cpp", "hello") << '\n';

    return 0;
}
