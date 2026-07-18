#include <string>
#include <iostream>
#include <new>       // placement new 所需的標頭檔

class Config {
private:
    const int m_id;          // const 成員：初始化後不可賦值
    std::string& m_nameRef;  // 引用成員：綁定後不可重新綁定

public:
    Config(int id, std::string& name) : m_id(id), m_nameRef(name) {}

    // ================================================================
    // 自訂拷貝賦值運算子 —— 使用 placement new 技巧
    // ================================================================
    // 問題：編譯器無法自動生成 operator=，因為：
    //   - const 成員 (m_id) 不能被重新賦值
    //   - 引用成員 (m_nameRef) 不能被重新綁定
    //
    // 解法：既然「賦值」行不通，我們改用「銷毀 + 重建」
    //   1. 先呼叫解構函數，銷毀當前物件
    //   2. 再用 placement new 在同一塊記憶體上重新建構
    //      placement new 會呼叫拷貝建構函數（編譯器可以自動生成，
    //      因為 const 和引用在「建構」時都能正常初始化）
    //
    // ⚠️ 注意：這是一種進階技巧，有一定風險：
    //   - 若建構函數拋出例外，物件會處於已銷毀但未重建的狀態
    //   - 若類別有虛擬函數或處於繼承體系中，需要更謹慎處理
    //   - 一般建議優先考慮改用指標取代引用、移除 const 等設計調整
    // ================================================================
    Config& operator=(const Config& other) {
        if (this != &other) {           // 防止自我賦值（自我賦值會導致先銷毀再讀取已銷毀的資料）
            this->~Config();            // 步驟 1：手動呼叫解構函數，銷毀當前物件
            new (this) Config(other);   // 步驟 2：placement new，在 this 的記憶體上
                                        //         呼叫拷貝建構函數，用 other 的值重建
        }
        return *this;                   // 回傳 *this 以支援鏈式賦值 (a = b = c)
    }

    void print() const {
        std::cout << "Config { id=" << m_id
                  << ", name=\"" << m_nameRef << "\" }" << std::endl;
    }
};

int main() {
    std::string name1 = "Alice";
    std::string name2 = "Bob";
    Config c1(1, name1);
    Config c2(2, name2);

    std::cout << "=== 賦值前 ===" << std::endl;
    std::cout << "c1: "; c1.print();
    std::cout << "c2: "; c2.print();

    c1 = c2;  // ✅ 現在可以編譯！使用自訂的 operator= (placement new)

    std::cout << "\n=== 賦值後 (c1 = c2) ===" << std::endl;
    std::cout << "c1: "; c1.print();
    std::cout << "c2: "; c2.print();

    return 0;
}

/*
**不行。** Copy-and-swap 無法解決 `const` 成員和引用成員的問題。

### 原因分析

Copy-and-swap 的標準寫法：

Config& operator=(Config other) {   // 傳值 → 呼叫拷貝建構（這步沒問題）
    swap(*this, other);              // ❌ 問題在這裡！
    return *this;
}

swap 的內部實作需要**交換每個成員**：

friend void swap(Config& a, Config& b) {
    std::swap(a.m_id, b.m_id);         // ❌ m_id 是 const，不能賦值
    std::swap(a.m_nameRef, b.m_nameRef); // ❌ 引用不能重新綁定
}

問題只是從 `operator=` **轉移**到了 `swap` — 根本原因完全一樣：

| 成員 | `operator=` 的問題 | `swap` 的問題 |
|------|-------------------|--------------|
| `const int m_id` | 不能賦值 | `std::swap` 內部也是賦值，一樣失敗 |
| `std::string& m_nameRef` | 不能重新綁定 | `std::swap` 無法交換引用的綁定目標 |

所以 copy-and-swap 並沒有繞過根本限制。除非你在 `swap` 裡面也用 placement new，但那就等於把同一個技巧換了個位置寫，沒有任何好處。

### 結論

要讓有 `const`/引用成員的類別支援賦值，只有三條路：

1. **Placement new**（如目前檔案中的做法）— 銷毀 + 重建
2. **改變成員設計** — `const` → non-const + getter，引用 → 指標
3. **不支援賦值** — 明確 `operator= = delete`，改用其他方式（如 `std::optional` 重新建構）
*/

