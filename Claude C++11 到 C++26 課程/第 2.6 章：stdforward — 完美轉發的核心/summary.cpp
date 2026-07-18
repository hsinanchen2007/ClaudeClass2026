// ============================================================
// 第 2.6 章 總結：std::forward — 完美轉發的核心
// 編譯：g++ -std=c++17 -o summary summary.cpp
// ============================================================
// 【問題：為什麼需要 forward？】
//   void wrapper(T&& arg) {
//       target(arg);  // ❌ arg 有名字 → 是左值 → 永遠呼叫左值版本！
//   }
//   不管傳入左值還是右值，arg 在函數內都是左值
//
// 【三種轉發方式的比較】
//   target(arg);                   永遠是左值
//   target(std::move(arg));        永遠是右值
//   target(std::forward<T>(arg));  保持原始值類別 ✅
//
// 【std::forward 運作原理】
//   搭配 T&& 和引用折疊規則：
//   傳入左值 string s → T = string& → T&& = string& (& + && = &)
//     → forward<string&>(arg) = static_cast<string&>(arg) → 左值 ✅
//   傳入右值 string("x") → T = string → T&& = string&&
//     → forward<string>(arg) = static_cast<string&&>(arg) → 右值 ✅
//
// 【典型應用場景】
//   1. 泛型工廠函式：make<T>(args...) → 完美轉發到 T 的建構子
//   2. emplace_back：直接在容器內建構，避免臨時物件
//   3. 計時包裝器：timed_call(func, args...) → 轉發到被包裝函式
//   4. 多層轉發：layer1 → layer2 → layer3 → target
//   5. auto&&：range-based for 中保持原始型別
// ============================================================

#include <iostream>
#include <string>
#include <utility>
#include <memory>
#include <vector>
#include <chrono>

// ============================================================
// 目標函式（重載左值/右值版本）
// ============================================================
void target(const std::string& s) { std::cout << "  target(const T&) 左值\n"; }
void target(std::string&& s)      { std::cout << "  target(T&&) 右值\n"; }

// ============================================================
// 錯誤寫法：三種失敗的嘗試
// ============================================================
template<typename T>
void wrapper_bad1(const T& arg) {
    target(arg);  // 永遠是左值（const T& 遮蓋了右值資訊）
}

template<typename T>
void wrapper_bad2(T&& arg) {
    target(arg);  // arg 有名字 → 永遠是左值
}

// ============================================================
// 正確寫法：std::forward
// ============================================================
template<typename T>
void wrapper_good(T&& arg) {
    target(std::forward<T>(arg));  // ★ 保持原始值類別
}

// ============================================================
// 比較三種行為
// ============================================================
void check(const std::string& s) { std::cout << "  → 左值\n"; }
void check(std::string&& s)      { std::cout << "  → 右值\n"; }

template<typename T>
void compare_three(T&& arg) {
    std::cout << "  直接傳 arg:       ";
    check(arg);                          // 永遠左值

    std::cout << "  std::move(arg):   ";
    check(std::move(arg));               // 永遠右值

    std::cout << "  std::forward<T>:  ";
    check(std::forward<T>(arg));         // 保持原始 ✅
}

// ============================================================
// 應用 1：泛型工廠函式
// ============================================================
class Person {
    std::string name_;
    int age_;
public:
    Person(const std::string& n, int a) : name_(n), age_(a) {
        std::cout << "    Person(const string&) 複製\n";
    }
    Person(std::string&& n, int a) : name_(std::move(n)), age_(a) {
        std::cout << "    Person(string&&) 移動\n";
    }
    void print() const { std::cout << "    " << name_ << ", age " << age_ << "\n"; }
};

template<typename T, typename... Args>
std::unique_ptr<T> make(Args&&... args) {
    return std::unique_ptr<T>(
        new T(std::forward<Args>(args)...)  // ★ 完美轉發所有引數
    );
}

// ============================================================
// 應用 2：多層轉發
// ============================================================
void final_target(const std::string& s) { std::cout << "  最終: 左值\n"; }
void final_target(std::string&& s)      { std::cout << "  最終: 右值\n"; }

template<typename T>
void layer3(T&& arg) { final_target(std::forward<T>(arg)); }

template<typename T>
void layer2(T&& arg) { layer3(std::forward<T>(arg)); }

template<typename T>
void layer1(T&& arg) { layer2(std::forward<T>(arg)); }

// ============================================================
// 應用 3：計時包裝器
// ============================================================
template<typename Func, typename... Args>
auto timed_call(Func&& func, Args&&... args)
    -> decltype(std::forward<Func>(func)(std::forward<Args>(args)...))
{
    auto start = std::chrono::high_resolution_clock::now();
    auto result = std::forward<Func>(func)(std::forward<Args>(args)...);
    auto us = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now() - start).count();
    std::cout << "    耗時: " << us << " us\n";
    return result;
}

std::string process(const std::string& input, int repeat) {
    std::string result;
    for (int i = 0; i < repeat; ++i) result += input;
    return result;
}

int main() {
    std::string s = "Hello";

    // ============================================================
    // 1. 問題展示：不用 forward 的結果
    // ============================================================
    std::cout << "===== 1. 不用 forward 的問題 =====\n";
    std::cout << "  wrapper_bad1（const T&）:\n";
    wrapper_bad1(s);                     // 左值 ✅
    wrapper_bad1(std::string("tmp"));    // 應右值 → 卻是左值 ❌
    std::cout << "  wrapper_bad2（T&& 但不 forward）:\n";
    wrapper_bad2(s);                     // 左值 ✅
    wrapper_bad2(std::string("tmp"));    // 應右值 → 卻是左值 ❌
    std::cout << "\n";

    // ============================================================
    // 2. 正確寫法：std::forward
    // ============================================================
    std::cout << "===== 2. std::forward =====\n";
    std::cout << "  傳入左值:\n";
    wrapper_good(s);                     // 左值 ✅
    std::cout << "  傳入右值:\n";
    wrapper_good(std::string("tmp"));    // 右值 ✅
    std::cout << "  傳入 std::move:\n";
    wrapper_good(std::move(s));          // 右值 ✅
    s = "Hello";  // 恢復
    std::cout << "\n";

    // ============================================================
    // 3. 三種行為比較
    // ============================================================
    std::cout << "===== 3. 三種行為比較 =====\n";
    std::cout << "  傳入左值:\n";
    compare_three(s);
    std::cout << "  傳入右值:\n";
    compare_three(std::string("tmp"));
    std::cout << "\n";

    // ============================================================
    // 4. 泛型工廠函式
    // ============================================================
    std::cout << "===== 4. 泛型工廠函式 =====\n";
    std::string name = "Alice";
    std::cout << "  傳入左值:\n";
    auto p1 = make<Person>(name, 30);     // 複製
    std::cout << "  傳入右值:\n";
    auto p2 = make<Person>(std::string("Bob"), 25);  // 移動
    std::cout << "\n";

    // ============================================================
    // 5. 多層轉發
    // ============================================================
    std::cout << "===== 5. 多層轉發（穿越三層）=====\n";
    std::cout << "  傳入左值:\n";
    layer1(s);                            // 穿越三層後仍是左值
    std::cout << "  傳入右值:\n";
    layer1(std::string("tmp"));           // 穿越三層後仍是右值
    std::cout << "\n";

    // ============================================================
    // 6. emplace_back vs push_back
    // ============================================================
    std::cout << "===== 6. emplace_back（內部使用 forward）=====\n";
    {
        struct Entry {
            std::string key;
            int val;
            Entry(const std::string& k, int v) : key(k), val(v) {
                std::cout << "    Entry(const string&)\n";
            }
            Entry(std::string&& k, int v) : key(std::move(k)), val(v) {
                std::cout << "    Entry(string&&)\n";
            }
        };

        std::vector<Entry> vec;
        vec.reserve(2);
        std::string key = "alpha";

        std::cout << "  emplace_back 傳左值:\n";
        vec.emplace_back(key, 1);       // 完美轉發 → const string&

        std::cout << "  emplace_back 傳右值:\n";
        vec.emplace_back(std::string("beta"), 2);  // 完美轉發 → string&&
    }
    std::cout << "\n";

    // ============================================================
    // 7. 計時包裝器
    // ============================================================
    std::cout << "===== 7. 計時包裝器 =====\n";
    {
        std::string input = "Hi";
        auto result = timed_call(process, input, 10000);
        std::cout << "    結果長度: " << result.size() << "\n";
    }

    std::cout << "\n=== 重點整理 ===\n";
    std::cout << "  直接傳 arg → 永遠左值\n";
    std::cout << "  std::move(arg) → 永遠右值\n";
    std::cout << "  std::forward<T>(arg) → 保持原始值類別 ✅\n";
    std::cout << "  必須搭配 T&& 模板參數使用\n";
    std::cout << "  典型應用：工廠函式、emplace_back、包裝器\n";

    return 0;
}
