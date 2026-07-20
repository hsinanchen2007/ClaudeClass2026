// ============================================================================
// 課題 4：const_cast - 只調整 const/volatile qualification
// ============================================================================
//
// const_cast 不會複製物件，也不使真正 const 的 storage 變可寫：
//   int value; const int& view=value; const_cast<int&>(view)=...;  // 原物件非 const，合法。
//   const int value=...; const_cast<int&>(value)=...;              // 修改是 UB。
//
// 合理用途主要是包裝錯誤標示為 mutable 的 legacy C API，而你能證明它不修改資料；更好
// 的方式是修正 API 或在 boundary 複製一份 mutable buffer。不要用 const_cast 偷懶讓
// const member 修改 logical state；cache/mutex 有明確理由時才用 mutable。
// ============================================================================

#include <algorithm>
#include <cassert>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

// 模擬舊 C API：參數錯寫 char*，實際只讀取長度。
std::size_t legacy_measure(char* text)
{
    return std::strlen(text);
}

std::size_t measured_length(const std::string& text)
{
    // 只有在確認 legacy_measure 絕不寫入時，這個 boundary cast 才可接受。
    return legacy_measure(const_cast<char*>(text.c_str()));
}

void basic_example()
{
    const std::string text = "CUDA";
    assert(measured_length(text) == 4U);

    int mutable_value = 10;
    const int& read_only_view = mutable_value;
    const_cast<int&>(read_only_view) = 20; // 原 storage 是 mutable，因此合法。
    assert(mutable_value == 20);
    std::cout << "[基礎] legacy read-only boundary + mutable original example\n";
}

// LeetCode 344：Reverse String 要求 in-place 修改 vector<char>。
// 正確 API 直接接 non-const reference；不要把 const input const_cast 後修改。
void reverse_string(std::vector<char>& text)
{
    std::reverse(text.begin(), text.end());
}

void leetcode_344_example()
{
    std::vector<char> text{'h', 'e', 'l', 'l', 'o'};
    reverse_string(text);
    assert((text == std::vector<char>{'o', 'l', 'l', 'e', 'h'}));
    std::cout << "[LeetCode 344] mutable API clearly performs in-place reverse\n";
}

// 實務：若 legacy API 可能修改，就複製一份，不可 const_cast 原字串。
void legacy_uppercase(char* text)
{
    for (; *text != '\0'; ++text) {
        if (*text >= 'a' && *text <= 'z') *text = static_cast<char>(*text - 'a' + 'A');
    }
}

std::string uppercase_copy(const std::string& input)
{
    std::string copy = input;
    legacy_uppercase(copy.data()); // C++17 起 non-const string::data 回 writable contiguous data。
    return copy;
}

void practical_example()
{
    const std::string original = "build";
    assert(uppercase_copy(original) == "BUILD");
    assert(original == "build");
    std::cout << "[實務] mutating legacy API receives a safe copy\n";
}

int main()
{
    basic_example();
    leetcode_344_example();
    practical_example();
}

// 練習：把 legacy_measure 簽章改 `const char*`，即可刪除 const_cast。
// 複雜度：const_cast 本身是 O(1) qualification 調整；它不複製資料，也不做 runtime 驗證。
// 生命週期：cast 後的 pointer 仍綁同一物件；物件已解構就懸空，原物件真為 const 時寫入是 UB。

/*
【本課面試問答】
Q1：`const_cast` 後何時可以寫？
A：只有底層 object 原本不是 const，只是目前經 const-qualified view 存取時，去 const 後修改才合法；
若 object 本身宣告為 const，寫入是 UB，即使記憶體實際看似可寫。

Q2：`const_cast` 能否改變數值或在 unrelated types 間轉換？
A：不能；它只調整 cv qualification（以及相關 pointer/reference 形式），不做 runtime check、allocation
或 representation 轉換。其他意圖要用 static/dynamic/reinterpret cast 或重新設計 API。

Q3：為呼叫錯誤的 legacy `char*` API 去 const 是否合理？
A：若 API 契約保證不修改，可在很窄 adapter 中使用並記錄原因；若可能修改，應建立 mutable copy。
長期最佳修法是把 API 簽章改成 `const char*`，而不是把 UB 風險散到每個 caller。
*/

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '04_const_cast.cpp' -o '/tmp/codex_cpp_C_Cast_04_const_cast' && '/tmp/codex_cpp_C_Cast_04_const_cast'
//
// === 預期輸出（節錄）===
// [實務] mutating legacy API receives a safe copy
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
