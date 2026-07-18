/*
================================================================================
【C++_OOP/summary.cpp】

本目錄主題：C++ 面向物件（class/encapsulation/inheritance/polymorphism）

你應該掌握的主線：
  - class 與物件：建構/解構、封裝（private/public）
  - RAII：資源用物件生命週期管理（比「記得 delete」可靠）
  - 繼承與多型：virtual、override、dynamic dispatch
  - Rule of 0/3/5：資源型別的拷貝/移動語意
  - 工廠模式/介面：用抽象類別當 API 邊界

本 summary 原則：
  - 不加入 題庫 類範例
  - 以 C++17 可編譯

編譯：
  g++ -std=c++17 -Wall -Wextra summary.cpp -o summary && ./summary
================================================================================
*/

/*
補充筆記：C++_OOP/C++_OOP summary
  - 如果兩個範例看起來都能完成同一件事，優先比較它們是否擁有資料、是否配置記憶體、是否改變輸入。
  - OOP 章節的主線是：先用 class/object 把資料和行為放在一起，再用封裝保護不變條件，最後用繼承與 virtual 表達可替換行為。
  - 建構子、解構子、copy、move 是物件生命週期的核心；學 OOP 不能只看 public 函式，也要知道物件何時出生、如何複製、何時釋放資源。
  - RAII 是 C++ OOP 和資源管理的交會點；懂 RAII 後，智慧指標、容器、fstream、lock_guard 都會變成同一種思路。
  - 設計類別時先問 ownership，再問 interface；誰擁有資源、誰能修改狀態、誰只能觀察，這三件事決定 class 的安全性。
  - 繼承適合 is-a 和 runtime polymorphism，不適合單純共享程式碼；能用成員組合表達的關係，通常比深繼承更容易維護。
  - 這個 summary.cpp 只做章節整理，不新增題庫題解；需要實作練習時回到各主題檔。
  - C++_OOP/C++_OOP summary 的複習方式是把 API 依用途分組，再比較輸入條件、輸出語意、失敗狀態和複雜度。
  - 初學複習 summary 時，不要只背函式名稱；要能說出何時該用、何時不該用、和相近工具差在哪裡。
*/
#include <iostream>
#include <memory>
#include <string>
#include <utility>

// -----------------------------------------------------------------------------
// 【重點 1】封裝：用 class 把狀態與行為綁在一起
// -----------------------------------------------------------------------------
class Counter {
public:
    void inc() { ++value_; }
    int value() const { return value_; }
private:
    int value_{0}; // private：外界不能直接亂改
};

static void demo_encapsulation() {
    std::cout << "\n[demo_encapsulation]\n";
    Counter c;
    c.inc();
    c.inc();
    std::cout << "  counter=" << c.value() << "\n";
}

// -----------------------------------------------------------------------------
// 【重點 2】RAII：解構函式確保資源釋放（例外也安全）
// -----------------------------------------------------------------------------
struct FileLike {
    explicit FileLike(const char* name) : name(name) {
        std::cout << "  open " << name << "\n";
    }
    ~FileLike() { std::cout << "  close " << name << "\n"; }
    const char* name;
};

static void demo_raii() {
    std::cout << "\n[demo_raii]\n";
    FileLike f("demo.txt");
    std::cout << "  do something...\n";
}

// -----------------------------------------------------------------------------
// 【重點 3】多型：用 virtual + override
// -----------------------------------------------------------------------------
struct Shape {
    virtual ~Shape() = default;
    virtual double area() const = 0;
};

struct Rect : Shape {
    Rect(double w, double h) : w(w), h(h) {}
    double area() const override { return w * h; }
    double w, h;
};

struct Circle : Shape {
    explicit Circle(double r) : r(r) {}
    double area() const override { return 3.1415926535 * r * r; }
    double r;
};

static void demo_polymorphism() {
    std::cout << "\n[demo_polymorphism]\n";
    std::unique_ptr<Shape> s1 = std::make_unique<Rect>(3, 4);
    std::unique_ptr<Shape> s2 = std::make_unique<Circle>(2);
    std::cout << "  rect area=" << s1->area() << "\n";
    std::cout << "  circle area=" << s2->area() << "\n";
}

// -----------------------------------------------------------------------------
// 【重點 4】Rule of 0/5：資源型別要定義拷貝/移動（或直接用標準容器/智慧指標）
// -----------------------------------------------------------------------------
class Buffer {
public:
    explicit Buffer(size_t n) : n_(n), data_(new int[n]{}) {}
    ~Buffer() { delete[] data_; }

    // copy ctor
    Buffer(const Buffer& other) : n_(other.n_), data_(new int[other.n_]{}) {
        for (size_t i = 0; i < n_; ++i) data_[i] = other.data_[i];
    }
    // copy assign
    Buffer& operator=(const Buffer& other) {
        if (this == &other) return *this;
        Buffer tmp(other);        // copy-and-swap：強保證
        swap(tmp);
        return *this;
    }
    // move ctor
    Buffer(Buffer&& other) noexcept : n_(other.n_), data_(other.data_) {
        other.n_ = 0;
        other.data_ = nullptr;
    }
    // move assign
    Buffer& operator=(Buffer&& other) noexcept {
        if (this == &other) return *this;
        delete[] data_;
        n_ = other.n_;
        data_ = other.data_;
        other.n_ = 0;
        other.data_ = nullptr;
        return *this;
    }

    void swap(Buffer& other) noexcept {
        std::swap(n_, other.n_);
        std::swap(data_, other.data_);
    }

    size_t size() const { return n_; }
private:
    size_t n_{0};
    int* data_{nullptr};
};

static void demo_rule_of_five() {
    std::cout << "\n[demo_rule_of_five]\n";
    Buffer a(3);
    Buffer b = a;          // copy
    Buffer c = std::move(a); // move
    std::cout << "  b.size=" << b.size() << ", c.size=" << c.size() << "\n";
}

int main() {
    demo_encapsulation();
    demo_raii();
    demo_polymorphism();
    demo_rule_of_five();

    std::cout << "\n[done]\n";
    return 0;
}

