// ============================================================
// 第 2.8 章 總結：Rule of Five — 現代資源管理規則
// 編譯：g++ -std=c++17 -o summary summary.cpp
// ============================================================
// 【Rule of Five】
//   如果類別需要自訂以下任何一個，通常五個都需要自訂：
//   1. ~ClassName()                           解構子
//   2. ClassName(const ClassName&)             複製建構子
//   3. ClassName& operator=(const ClassName&)  複製賦值
//   4. ClassName(ClassName&&) noexcept         移動建構子
//   5. ClassName& operator=(ClassName&&) noexcept 移動賦值
//
// 【Rule of Zero（最推薦！）】
//   只用 RAII 成員（string, vector, unique_ptr, shared_ptr...）
//   → 不需要寫任何特殊函式，編譯器自動生成正確版本
//   → unique_ptr 讓類別自動變成「只能移動不能複製」
//
// 【違反 Rule of Five 的後果】
//   只寫解構子不寫其他 → 淺拷貝 → double free → 程式崩潰
// ============================================================

#include <iostream>
#include <cstring>
#include <utility>
#include <string>
#include <vector>
#include <memory>

// ============================================================
// 完整的 Rule of Five 類別
// ============================================================
class String {
    char* data_;
    size_t size_;
public:
    // 建構子
    String(const char* s = "")
        : size_(std::strlen(s)), data_(new char[size_ + 1]) {
        std::strcpy(data_, s);
        std::cout << "  [建構] \"" << data_ << "\"\n";
    }

    // ════════════════════════════════════
    // ★ Rule of Five ★
    // ════════════════════════════════════

    // 1. 解構子
    ~String() {
        std::cout << "  [解構] \"" << (data_ ? data_ : "null") << "\"\n";
        delete[] data_;
    }

    // 2. 複製建構子
    String(const String& o)
        : size_(o.size_), data_(new char[o.size_ + 1]) {
        std::strcpy(data_, o.data_);
        std::cout << "  [複製建構] \"" << data_ << "\"\n";
    }

    // 3. 複製賦值
    String& operator=(const String& o) {
        if (this != &o) {
            delete[] data_;
            size_ = o.size_;
            data_ = new char[size_ + 1];
            std::strcpy(data_, o.data_);
        }
        std::cout << "  [複製賦值] \"" << data_ << "\"\n";
        return *this;
    }

    // 4. 移動建構子
    String(String&& o) noexcept
        : data_(o.data_), size_(o.size_) {
        o.data_ = nullptr; o.size_ = 0;
        std::cout << "  [移動建構] \"" << data_ << "\"\n";
    }

    // 5. 移動賦值
    String& operator=(String&& o) noexcept {
        if (this != &o) {
            delete[] data_;
            data_ = o.data_; size_ = o.size_;
            o.data_ = nullptr; o.size_ = 0;
        }
        std::cout << "  [移動賦值] \"" << data_ << "\"\n";
        return *this;
    }

    // ════════════════════════════════════

    const char* c_str() const { return data_ ? data_ : ""; }
};

// ============================================================
// 違反 Rule of Five：只寫解構子
// ============================================================
class Broken {
    int* data_;
public:
    Broken(int val) : data_(new int(val)) {}
    ~Broken() { delete data_; }
    // ⚠️ 沒有拷貝建構、拷貝賦值、移動建構、移動賦值
    // → 預設淺拷貝 → double free！
    int value() const { return *data_; }
};

// ============================================================
// Rule of Zero：最推薦的做法
// ============================================================
class User {
    std::string name_;                  // RAII：自動管理
    std::vector<int> scores_;           // RAII：自動管理
    std::unique_ptr<int[]> buffer_;     // RAII：自動管理（不可複製）

    // 不需要寫任何特殊函式！
    // 解構子：自動呼叫每個成員的解構子
    // 移動：自動逐成員移動
    // 複製：因 unique_ptr 不可複製 → 整個類別不可複製
public:
    User(std::string name, std::vector<int> scores, size_t buf_size)
        : name_(std::move(name))
        , scores_(std::move(scores))
        , buffer_(std::make_unique<int[]>(buf_size)) {}

    const std::string& name() const { return name_; }
};

int main() {
    // ============================================================
    // 1. Rule of Five 完整使用
    // ============================================================
    std::cout << "===== 1. Rule of Five =====\n";
    {
        String a("Hello");
        String b = a;              // 複製建構
        String c;
        c = a;                     // 複製賦值
        String d = std::move(a);   // 移動建構
        String e;
        e = std::move(b);          // 移動賦值

        std::cout << "  a: \"" << a.c_str() << "\" (已被移動)\n";
        std::cout << "  b: \"" << b.c_str() << "\" (已被移動)\n";
        std::cout << "  c: \"" << c.c_str() << "\"\n";
        std::cout << "  d: \"" << d.c_str() << "\"\n";
        std::cout << "  e: \"" << e.c_str() << "\"\n";
    }
    std::cout << "\n";

    // ============================================================
    // 2. 違反 Rule of Five 的後果
    // ============================================================
    std::cout << "===== 2. 違反 Rule of Five =====\n";
    std::cout << "  Broken 類別：只寫解構子 → double free！\n";
    std::cout << "  Broken a(42); Broken b = a;\n";
    std::cout << "  a.data_ 和 b.data_ 指向同一塊記憶體\n";
    std::cout << "  兩者解構時都 delete → 💥 crash\n";
    // 不實際執行，避免崩潰
    std::cout << "\n";

    // ============================================================
    // 3. Rule of Zero
    // ============================================================
    std::cout << "===== 3. Rule of Zero（最推薦）=====\n";
    {
        User u1("Alice", {90, 85, 95}, 100);
        // User u2 = u1;          // ❌ 因為 unique_ptr 不可複製
        User u2 = std::move(u1);  // ✅ 逐成員移動
        std::cout << "  u2.name() = " << u2.name() << "\n";
        std::cout << "  u1.name() = \"" << u1.name() << "\" (已被移動)\n";
        std::cout << "  不需要寫任何特殊函式！\n";
    }

    std::cout << "\n=== 重點整理 ===\n";
    std::cout << "  Rule of Five：解構+複製建構+複製賦值+移動建構+移動賦值\n";
    std::cout << "  只寫解構不寫其他 → 淺拷貝 → double free\n";
    std::cout << "  Rule of Zero（最佳）：只用 RAII 成員 → 什麼都不寫\n";
    std::cout << "  unique_ptr → 自動禁止複製，只能移動\n";

    return 0;
}
