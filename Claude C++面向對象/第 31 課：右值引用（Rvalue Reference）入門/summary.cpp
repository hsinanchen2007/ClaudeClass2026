// ============================================================
// 第 31 課 總結：右值引用（Rvalue Reference）入門
// 編譯：g++ -std=c++17 -o summary summary.cpp
// ============================================================
// 【Lvalue vs Rvalue】
//   Lvalue（左值）：有名字、可取位址 → int x = 10; 中的 x
//   Rvalue（右值）：臨時的、沒名字   → 10、x+y、func() 的回傳值
//
// 【三種引用類型與綁定規則】
//   T&        左值引用       → 只能綁定左值
//   const T&  const 左值引用 → 可綁定左值和右值（萬能讀取）
//   T&&       右值引用       → 只能綁定右值
//
// 【重要陷阱！】右值引用變數本身是左值！
//   int&& rref = 42;       // rref 綁定右值 42
//   // 但 rref 本身有名字、可取位址 → rref 是左值！
//   // int&& rref2 = rref;  // ❌ 左值不能綁到 T&&
//   int&& rref2 = std::move(rref);  // ✅ 用 std::move 轉回右值
//
// 【函數重載：const T& vs T&&】
//   傳左值 → 選 const T& 版本
//   傳右值 → 優先選 T&& 版本（更精確匹配）
//   這是移動語義的基礎：拷貝和移動分別走不同的重載
//
// 【std::move 預覽】
//   std::move(x) 只是 static_cast<T&&>(x)，把左值「標記為」右值
//   本身不移動任何東西，真正的移動由接收端（移動建構/賦值）完成
//   move 後的物件處於「有效但未指定」狀態
// ============================================================

#include <iostream>
#include <string>
#include <utility>  // std::move

// ============================================================
// 函數重載示範
// ============================================================
void process(const std::string& s) {
    std::cout << "  [const T& 版本] \"" << s << "\"\n";
}

void process(std::string&& s) {
    std::cout << "  [T&& 版本]      \"" << s << "\"\n";
}

// ============================================================
// 右值引用參數在函數內是左值
// ============================================================
void takeRvalueRef(int&& val) {
    std::cout << "  takeRvalueRef: val=" << val << "\n";
    std::cout << "  &val=" << &val << " （可取址 → val 是左值！）\n";
    // takeRvalueRef(val);  // ❌ val 是左值，不能再傳給 T&&
}

void takeLvalueRef(int& val) {
    std::cout << "  takeLvalueRef: val=" << val << "\n";
}

int main() {
    // ============================================================
    // 1. 綁定規則
    // ============================================================
    std::cout << "===== 1. 綁定規則 =====\n";
    int x = 42;

    int& lref = x;             // ✅ 左值引用 → 左值
    // int& lref2 = 42;        // ❌ 左值引用 → 右值（不行）

    const int& clref1 = x;    // ✅ const 左值引用 → 左值
    const int& clref2 = 42;   // ✅ const 左值引用 → 右值（特殊規則！）

    int&& rref = 42;           // ✅ 右值引用 → 右值
    // int&& rref2 = x;        // ❌ 右值引用 → 左值（不行）

    rref = 100;                // ✅ 右值引用本身可修改
    std::cout << "  rref = " << rref << "\n\n";

    // ============================================================
    // 2. 右值引用變數本身是左值（重要陷阱！）
    // ============================================================
    std::cout << "===== 2. 右值引用變數是左值 =====\n";
    int&& r = 42;
    std::cout << "  r = " << r << "\n";
    std::cout << "  &r = " << &r << " ← 可取位址，所以 r 是左值\n";

    // r 是左值 → 可以傳給 T&
    takeLvalueRef(r);      // ✅

    // r 是左值 → 不能傳給 T&&
    // takeRvalueRef(r);   // ❌

    // 用 std::move 把 r 轉回右值
    takeRvalueRef(std::move(r));  // ✅
    std::cout << "\n";

    // 函數參數 T&& 在函數內也是左值
    std::cout << "===== 函數參數 T&& 在函數內是左值 =====\n";
    takeRvalueRef(99);
    std::cout << "\n";

    // ============================================================
    // 3. 函數重載：const T& vs T&&
    // ============================================================
    std::cout << "===== 3. 函數重載 =====\n";
    std::string name = "Dragon";

    std::cout << "  傳入左值：\n";
    process(name);                      // 左值 → const T& 版本

    std::cout << "  傳入右值（臨時物件）：\n";
    process(std::string("Phoenix"));    // 右值 → T&& 版本

    std::cout << "  傳入字面量：\n";
    process("Knight");                  // 字面量 → 建構臨時 string → T&& 版本

    std::cout << "  傳入運算結果：\n";
    process(name + " King");            // 運算結果是臨時物件 → T&& 版本
    std::cout << "\n";

    // ============================================================
    // 4. std::move 預覽
    // ============================================================
    std::cout << "===== 4. std::move 預覽 =====\n";
    std::string a = "Alpha";

    std::cout << "  傳入左值 a：\n";
    process(a);                   // const T& 版本

    std::cout << "  傳入 std::move(a)：\n";
    process(std::move(a));        // T&& 版本

    std::cout << "  move 後 a 的狀態：\n";
    std::cout << "  a = \"" << a << "\" (通常是空字串，但不保證)\n";
    std::cout << "  a.size() = " << a.size() << "\n";
    std::cout << "  （有效但未指定的狀態，可以重新賦值）\n";

    a = "Gamma";  // ✅ 重新賦值是安全的
    std::cout << "  重新賦值後 a = \"" << a << "\"\n\n";

    // ============================================================
    // 5. 右值引用變數傳給重載函數
    // ============================================================
    std::cout << "===== 5. 右值引用變數傳給重載函數 =====\n";
    std::string&& sref = std::string("Omega");
    process(sref);              // const T& 版本！因為 sref 是左值
    process(std::move(sref));   // T&& 版本

    std::cout << "\n=== 重點整理 ===\n";
    std::cout << "  T&       → 只綁左值\n";
    std::cout << "  const T& → 綁左值和右值（萬能）\n";
    std::cout << "  T&&      → 只綁右值\n";
    std::cout << "  右值引用變數本身是左值（有名字就是左值）\n";
    std::cout << "  std::move 只是 cast，不做移動\n";

    return 0;
}
