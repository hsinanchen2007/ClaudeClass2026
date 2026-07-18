// ============================================================
// 第 2.5 章 總結：std::move — 強制轉換為右值
// 編譯：g++ -std=c++17 -o summary summary.cpp
// ============================================================
// 【核心觀念】std::move 本身不移動任何東西！
//   std::move(x) ≡ static_cast<T&&>(x)
//   只是把左值轉成右值引用，讓接收端可以選擇移動建構/賦值
//
// 【std::move 的實作原理】
//   template<typename T>
//   typename std::remove_reference<T>::type&& move(T&& arg) noexcept {
//       return static_cast<typename std::remove_reference<T>::type&&>(arg);
//   }
//   關鍵：remove_reference 去掉引用後再加 &&，無論傳入什麼都得到 T&&
//
// 【三個移動不會發生的情況】
//   1. const 物件：const T&& 只能匹配 const T&（複製建構）
//      → const string c = "x"; string d = move(c); // 複製！不是移動
//   2. 基本型別（int, double...）：沒有資源可搬，移動 = 複製
//   3. SSO 短字串：string 的 Short String Optimization，資料在棧上，移動也是複製
//
// 【unique_ptr 的所有權轉移】
//   unique_ptr 禁止複製，只能移動
//   take_ownership(std::move(p));  // 明確轉移所有權
// ============================================================

#include <iostream>
#include <string>
#include <utility>
#include <memory>
#include <type_traits>
#include <chrono>

// ============================================================
// 自己實作 my_move
// ============================================================
template<typename T>
typename std::remove_reference<T>::type&& my_move(T&& arg) noexcept {
    return static_cast<typename std::remove_reference<T>::type&&>(arg);
}

void test(const std::string& s) { std::cout << "  [左值版本]\n"; }
void test(std::string&& s)      { std::cout << "  [右值版本]\n"; }

int main() {
    // ============================================================
    // 1. std::move ≡ static_cast<T&&>
    // ============================================================
    std::cout << "===== 1. move vs static_cast =====\n";
    {
        std::string s = "Hello, World!";

        std::string a = std::move(s);              // 用 std::move
        s = "Hello, World!";
        std::string b = static_cast<std::string&&>(s);  // 用 static_cast
        // 兩者行為完全相同
        std::cout << "  a = \"" << a << "\"\n";
        std::cout << "  b = \"" << b << "\"\n";
    }
    std::cout << "\n";

    // ============================================================
    // 2. 自製 my_move 驗證
    // ============================================================
    std::cout << "===== 2. 自製 my_move =====\n";
    {
        std::string s = "Hello";

        std::cout << "  std::move:  "; test(std::move(s));
        s = "Hello";
        std::cout << "  my_move:    "; test(my_move(s));
        std::cout << "  my_move(rv):"; test(my_move(std::string("temp")));
    }
    std::cout << "\n";

    // ============================================================
    // 3. 移動不會發生的三種情況
    // ============================================================
    std::cout << "===== 3. 移動不會發生的情況 =====\n";

    // 情況 1：const 物件
    std::cout << "  【const 物件】\n";
    {
        const std::string c = "World";
        std::string d = std::move(c);  // const string&& → 匹配複製建構！
        std::cout << "    c = \"" << c << "\" (仍然是 World！沒有被移動)\n";
        std::cout << "    d = \"" << d << "\"\n";
    }

    // 情況 2：基本型別
    std::cout << "  【基本型別 int】\n";
    {
        int x = 42;
        int y = std::move(x);  // int 沒有資源可搬
        std::cout << "    x = " << x << " (仍然是 42)\n";
    }

    // 情況 3：SSO 短字串
    std::cout << "  【SSO 短字串】\n";
    {
        std::string s = "Hi";  // 短字串 → SSO
        std::string t = std::move(s);
        std::cout << "    s = \"" << s << "\" (可能仍有值，SSO 不一定清空)\n";
        std::cout << "    t = \"" << t << "\"\n";
    }
    std::cout << "\n";

    // ============================================================
    // 4. unique_ptr 所有權轉移
    // ============================================================
    std::cout << "===== 4. unique_ptr 所有權轉移 =====\n";
    {
        auto p = std::make_unique<int>(42);
        std::cout << "  *p = " << *p << "\n";

        // auto q = p;           // ❌ unique_ptr 不可複製！
        auto q = std::move(p);   // ✅ 轉移所有權
        std::cout << "  *q = " << *q << "\n";
        std::cout << "  p = " << (p ? "非空" : "nullptr") << "\n";
    }
    std::cout << "\n";

    // ============================================================
    // 5. 效能比較：長字串複製 vs 移動
    // ============================================================
    std::cout << "===== 5. 效能比較 =====\n";
    {
        const int N = 1000000;
        std::string source(10000, 'x');

        auto t1 = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < N; ++i) {
            std::string copy = source;  // 複製
            (void)copy;
        }
        auto t2 = std::chrono::high_resolution_clock::now();

        auto t3 = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < N; ++i) {
            std::string temp = source;
            std::string moved = std::move(temp);  // 移動
            (void)moved;
            source = moved;  // 恢復
        }
        auto t4 = std::chrono::high_resolution_clock::now();

        auto copy_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
        auto move_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t4 - t3).count();
        std::cout << "  複製 " << N << " 次: " << copy_ms << " ms\n";
        std::cout << "  移動 " << N << " 次: " << move_ms << " ms\n";
    }

    std::cout << "\n=== 重點整理 ===\n";
    std::cout << "  std::move 只是 static_cast<T&&>，不做實際移動\n";
    std::cout << "  const 物件 move → 退化為複製（const T&& → const T&）\n";
    std::cout << "  基本型別 / SSO 短字串 → 移動 = 複製，無效能差異\n";
    std::cout << "  unique_ptr 只能 move 不能 copy → 明確轉移所有權\n";

    return 0;
}
