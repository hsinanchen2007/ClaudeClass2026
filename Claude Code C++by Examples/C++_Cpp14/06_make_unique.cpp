// =============================================================================
//  06_make_unique.cpp  —  std::make_unique (C++14)
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/memory/unique_ptr/make_unique
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、為什麼有 make_unique？                                 │
//  └────────────────────────────────────────────────────────────┘
//
//  C++11 引入 unique_ptr，但缺一個「工廠函式」與 make_shared 對應。要建
//  立 unique_ptr 只能：
//
//      std::unique_ptr<T> p(new T(args...));
//
//  C++14 補上 make_unique：
//
//      auto p = std::make_unique<T>(args...);
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、為什麼 make_unique 比裸 new 安全？                     │
//  └────────────────────────────────────────────────────────────┘
//
//  「exception safety」 — 多參數函式呼叫時：
//
//      f(std::unique_ptr<T>(new T), g());
//
//  C++17 之前求值順序未定義 — 編譯器允許先 new T、再呼叫 g()、再包成
//  unique_ptr。如果 g() throw，T 已配但還沒被 unique_ptr 接住 → 洩漏！
//
//  make_unique 把「new + 包」打包成單一函式 → 沒辦法被插入 g() 的呼叫：
//
//      f(std::make_unique<T>(), g());      // ✅ 安全
//
//  此外語法更短、不必重複型別名。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、用法                                                   │
//  └────────────────────────────────────────────────────────────┘
//
//      // 物件
//      auto p1 = std::make_unique<MyClass>(arg1, arg2);
//
//      // 陣列（C++14 也支援）
//      auto arr = std::make_unique<int[]>(10);     // int[10] 預設初始化
//
//  注意：
//   * 沒有「make_unique 帶 deleter」版本（不像 unique_ptr 本身可帶 deleter）
//   * 用 array 版時 size 是 runtime 給的；元素 default-init（基本型別 = 未初始化）
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 四、本檔示範                                               │
//  └────────────────────────────────────────────────────────────┘
//
//   * Demo 1：基本用法
//   * Demo 2：array 版
//   * Demo 3：跟 polymorphism 結合
// =============================================================================

/*
補充筆記：make_unique
  - make_unique 是現代 C++ 語法或標準庫特性；學習時要把「少寫字」和「語意更精確」分開看。
  - auto 讓型別由初始化式推導，但會丟掉 top-level const/reference；需要保留引用語意時要寫 auto&、const auto& 或 decltype(auto)。
  - brace initialization 能減少未初始化與 narrowing，但遇到 initializer_list overload 可能選到不同建構子。
  - constexpr、static_assert、if constexpr 把部分錯誤和計算提前到編譯期，能讓 template 和常數邏輯更清楚。
  - 屬性如 [[nodiscard]]、[[maybe_unused]]、[[fallthrough]] 是對編譯器和讀者的意圖標記，不應拿來掩蓋設計問題。
  - string_view、optional、variant、structured binding 等特性改善介面表達力，但也帶來生命週期或狀態檢查責任。
  - make_unique 把 new 包起來，避免建構參數求值時發生例外造成資源洩漏。
  - make_unique<T[]> 可建立動態陣列，但多數情況 vector 仍比裸陣列更好。
*/
#include <iostream>
#include <memory>
#include <string>

struct Animal {
    virtual ~Animal() = default;
    virtual void say() const = 0;
};
struct Dog : Animal { void say() const override { std::cout << "woof\n"; } };

class User {
public:
    User(std::string n, int a) : name_(std::move(n)), age_(a) {
        std::cout << "  User(" << name_ << ", " << age_ << ") ctor\n";
    }
    void greet() const {
        std::cout << "Hi, I'm " << name_ << " (" << age_ << ")\n";
    }
private:
    std::string name_;
    int         age_;
};

int main() {
    // ─────────────────────────────────────────────────────────
    // Demo 1：基本
    // ─────────────────────────────────────────────────────────
    auto u = std::make_unique<User>("Alice", 30);
    u->greet();

    // ─────────────────────────────────────────────────────────
    // Demo 2：array 版
    // ─────────────────────────────────────────────────────────
    auto arr = std::make_unique<int[]>(5);
    for (int i = 0; i < 5; ++i) arr[i] = i * i;
    std::cout << "[Demo2] arr =";
    for (int i = 0; i < 5; ++i) std::cout << ' ' << arr[i];
    std::cout << '\n';

    // ─────────────────────────────────────────────────────────
    // Demo 3：polymorphism — 回傳 base class 智能指標
    // ─────────────────────────────────────────────────────────
    std::unique_ptr<Animal> a = std::make_unique<Dog>();
    a->say();

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：什麼時候反而要寫裸 new？
    //    A：(a) 自訂 deleter — make_unique 不支援，要 unique_ptr<T, Del>
    //           p(new T, customDel);
    //       (b) 不想觸發建構（要 placement new、底層 buffer 操作）
    //
    //  Q2：make_shared 跟 make_unique 哪個重要？
    //    A：兩個都重要。make_shared 把「物件 + control block」一次配置 →
    //       省 1 次 alloc + cache 友好。make_unique 主要是「safety」，效能
    //       差距不大。
    //
    //  Q3：有 make_unique 還需要 unique_ptr ctor 嗎？
    //    A：要。unique_ptr 的 ctor 接受「現存指標」(ex. 從 C API 拿到的
    //       裸指標) 或自訂 deleter — make_unique 都做不到。
    //
    // ─────────────────────────────────────────────────────────
    // LeetCode 206. Reverse Linked List
    //   題意：反轉單向鏈表。
    //   為什麼放這？用 make_unique 安全建立 linked list 節點（無洩漏風險）；
    //                示範把 raw new ListNode 改寫為 unique_ptr ownership。
    // ─────────────────────────────────────────────────────────
    struct ListNode {
        int val;
        std::unique_ptr<ListNode> next;
        explicit ListNode(int v) : val(v), next(nullptr) {}
    };
    // 建 1 -> 2 -> 3
    auto head = std::make_unique<ListNode>(1);
    head->next = std::make_unique<ListNode>(2);
    head->next->next = std::make_unique<ListNode>(3);

    // 反轉：用 raw pointer 走訪、unique_ptr 重新接管
    auto reverse = [](std::unique_ptr<ListNode> h) {
        std::unique_ptr<ListNode> prev = nullptr;
        while (h) {
            auto next = std::move(h->next);    // 取下一個
            h->next = std::move(prev);          // 接到 prev
            prev = std::move(h);                // h 變 prev
            h = std::move(next);                // 繼續走
        }
        return prev;
    };
    auto rev = reverse(std::move(head));
    std::cout << "[LC206] reversed:";
    for (auto* p = rev.get(); p; p = p->next.get()) std::cout << ' ' << p->val;
    std::cout << '\n';

    // ─────────────────────────────────────────────────────────
    // 實用範例：工廠函式 — 安全建立子類別物件
    //   工作上常見：log writer 工廠，避免 raw new 漏接
    // ─────────────────────────────────────────────────────────
    struct LogWriter {
        virtual ~LogWriter() = default;
        virtual void write(const std::string& msg) = 0;
    };
    struct StdoutWriter : LogWriter {
        void write(const std::string& msg) override {
            std::cout << "[stdout] " << msg << '\n';
        }
    };
    auto makeWriter = [](const std::string& type) -> std::unique_ptr<LogWriter> {
        if (type == "stdout") return std::make_unique<StdoutWriter>();
        return nullptr;
    };
    auto w = makeWriter("stdout");
    if (w) w->write("hello make_unique");

    return 0;
}
