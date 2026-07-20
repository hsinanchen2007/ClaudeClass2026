// =============================================================================
// 主題: 多重資源的移動建構子 —— 每一種資源都要「偷 + 歸零」
// =============================================================================
//
// 【主題資訊 Information】
//   簽名    ：Resource(Resource&& other) noexcept;
//   標準版本：移動建構子 / 右值參考 / noexcept   C++11
//   標頭檔  ：<cstring>（strlen/strcpy）、<utility>（std::move）
//   複雜度  ：複製 O(N+M)（兩塊記憶體都要重新配置並拷貝）；移動 O(1)
//   本檔宣告的標準：C++17
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼要用「多個資源」來示範】
//   前面的範例只有一個指標，很容易讓人以為「移動 = 搬那一個指標」。
//   真實的類別常同時持有多種資源：兩塊堆積記憶體、一個檔案描述符、
//   甚至還有一個鎖。移動建構子必須對「每一項」都做完整的兩步：
//       (a) 把它接過來
//       (b) 把來源那一份設成無效值
//   漏掉任何一項，就會有一個資源被釋放兩次或洩漏。
//
// 【2. 不同資源的「無效值」不一樣】
//       指標           → nullptr
//       計數/長度      → 0
//       檔案描述符(fd) → -1（0/1/2 是 stdin/stdout/stderr，不能拿來當無效值）
//   本檔用 fd = -1 表示「已移交」，解構子據此判斷要不要關閉。
//   這是實務上非常重要的細節：無效值必須是「該資源型別中不可能合法」的值。
//
// 【3. 成員初始化順序：本檔為何安全】
//   本類別的宣告順序是 integers_ → int_count_ → text_ → file_descriptor_。
//   一般建構子的初始化列表寫成
//       : integers_(new int[count]()), int_count_(count),
//         text_(new char[strlen(str)+1]), file_descriptor_(fd)
//   其中 integers_ 用的是「參數 count」而不是成員 int_count_，
//   text_ 用的是「參數 str」，兩者都不依賴其他成員 —— 所以順序無虞。
//   ⚠️ 若改寫成 text_(new char[int_count_]) 之類「依賴前面成員」的形式，
//      就必須確認宣告順序正確（初始化一律依宣告順序，不看書寫順序）。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 複製的成本在這裡特別明顯
//     複製建構子要為 integers_ 配置 count 個 int、為 text_ 配置字串長度，
//     兩塊都要逐一拷貝 —— 本檔的 count 是 1000，複製一次就是 4 KB 加字串。
//     移動則永遠只是抄幾個指標與整數，與資料量完全無關。
//
// (B) 檔案描述符不能被「複製」
//     整數 fd 可以被複製，但那在語意上是錯的 ——
//     兩個物件都以為自己擁有這個 fd，解構時會 close 兩次。
//     第二次 close 可能關到「剛好被重新配置給別人的同號 fd」，
//     造成極難追查的 bug。所以移動時一定要把來源設成 -1。
//
// (C) 為什麼移動後要讓 print() 仍可安全呼叫
//     被移動後的物件仍須「有效」。本檔的 print() 對 text_ 做了 nullptr 檢查，
//     所以即使 r1 已被掏空，印出來也只是 (null) 而不會崩潰。
//     這是自管資源類別應有的防禦性寫法。
//
// 【注意事項 Pay Attention】
//   1. 每一項資源都要「接手 + 讓來源失效」，漏一項就會重複釋放或洩漏。
//   2. 無效值要選該型別中不可能合法的值：指標用 nullptr、fd 用 -1（不是 0）。
//   3. 移動建構子應標 noexcept。
//   4. 被移動後的物件仍須可安全解構、可安全查詢狀態。
//   5. 成員初始化依「宣告順序」；本檔各成員互不依賴，故安全。
//   6. 本檔自管 new/delete[] 是教學用；實務請優先用 vector/string/unique_ptr。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】多重資源的移動
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 類別同時持有兩塊記憶體與一個檔案描述符，移動建構子要注意什麼？
//     答：每一項資源都要做完整的兩步 —— 接手、然後讓來源失效。
//         指標設 nullptr、長度設 0、fd 設 -1。
//         漏掉任何一項，該資源就會被釋放兩次（或永遠不被釋放）。
//     追問：為什麼 fd 的無效值用 -1 而不是 0？→ 因為 0 是 stdin，是合法的 fd。
//         無效值必須挑「該型別中不可能合法」的值。
//
// ⚠️ 陷阱. 移動建構子只把指標歸零、忘了處理 fd，會發生什麼？
//     答：兩個物件都認為自己擁有同一個 fd，解構時各 close 一次。
//         第二次 close 是對已關閉的描述符操作 —— 若該號碼此時已被
//         重新配置給其他檔案或 socket，就會誤關別人的資源，
//         造成極難追查的錯誤（表現不固定）。
//     為什麼會錯：把「移動」窄化成「處理指標」。
//         正確理解是：移動處理的是「所有權」，
//         而任何具有獨佔所有權的東西（記憶體、fd、鎖、handle）都要一起移交。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <cstring>
#include <utility>

class Resource {
    int* integers_;
    size_t int_count_;
    char* text_;
    int file_descriptor_;  // 模擬檔案描述符

public:
    Resource(size_t count, const char* str, int fd)
        : integers_(new int[count]())
        , int_count_(count)
        , text_(new char[std::strlen(str) + 1])
        , file_descriptor_(fd)
    {
        std::strcpy(text_, str);
        std::cout << "  [建構] ints=" << int_count_
                  << ", text=\"" << text_
                  << "\", fd=" << file_descriptor_ << "\n";
    }

    ~Resource() {
        std::cout << "  [解構] ints=" << integers_
                  << ", text=" << (void*)text_
                  << ", fd=" << file_descriptor_ << "\n";
        delete[] integers_;       // nullptr 時 delete 安全
        delete[] text_;           // nullptr 時 delete 安全
        if (file_descriptor_ >= 0) {
            // close(file_descriptor_);  // 真實場景會關閉檔案
        }
    }

    // 複製建構子
    Resource(const Resource& other)
        : integers_(new int[other.int_count_])
        , int_count_(other.int_count_)
        , text_(new char[std::strlen(other.text_) + 1])
        , file_descriptor_(other.file_descriptor_)  // fd 的複製語意需要特別考慮
    {
        std::copy(other.integers_, other.integers_ + int_count_, integers_);
        std::strcpy(text_, other.text_);
        std::cout << "  [複製建構] 2 次 new + 資料複製\n";
    }

    // 移動建構子
    Resource(Resource&& other) noexcept
        : integers_(other.integers_)           // 接管整數陣列
        , int_count_(other.int_count_)         // 接管計數
        , text_(other.text_)                   // 接管文字緩衝區
        , file_descriptor_(other.file_descriptor_) // 接管檔案描述符
    {
        // 關鍵：讓來源放棄所有資源
        other.integers_ = nullptr;
        other.int_count_ = 0;
        other.text_ = nullptr;
        other.file_descriptor_ = -1;  // -1 表示無效的 fd

        std::cout << "  [移動建構] 0 次 new，只搬指標\n";
    }

    void print() const {
        std::cout << "  integers_=" << integers_
                  << ", count=" << int_count_
                  << ", text=" << (text_ ? text_ : "(null)")
                  << ", fd=" << file_descriptor_ << "\n";
    }
};

int main() {
    std::cout << "=== 建立原始物件 ===\n";
    Resource r1(1000, "important data", 42);
    r1.print();

    std::cout << "\n=== 複製建構 ===\n";
    Resource r2(r1);
    std::cout << "r1: "; r1.print();
    std::cout << "r2: "; r2.print();

    std::cout << "\n=== 移動建構 ===\n";
    Resource r3(std::move(r1));
    std::cout << "r1: "; r1.print();  // 已被掏空
    std::cout << "r3: "; r3.print();  // 擁有原本 r1 的資源

    std::cout << "\n=== 離開 main ===\n";
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 2.3 章：移動建構子 (Move Constructor) — 實作與原理3.cpp" -o mc_demo3
//  ⚠️ 注意：下列輸出中的記憶體位址「每次執行都不同」（受堆積/堆疊配置與 ASLR 影響），
//     此處僅為某一次實際執行的樣本，不具重現性；其餘文字內容則是穩定可重現的。

// === 預期輸出 ===
// === 建立原始物件 ===
//   [建構] ints=1000, text="important data", fd=42
//   integers_=0x5d8fc8b5b340, count=1000, text=important data, fd=42
//
// === 複製建構 ===
//   [複製建構] 2 次 new + 資料複製
// r1:   integers_=0x5d8fc8b5b340, count=1000, text=important data, fd=42
// r2:   integers_=0x5d8fc8b5c2f0, count=1000, text=important data, fd=42
//
// === 移動建構 ===
//   [移動建構] 0 次 new，只搬指標
// r1:   integers_=0, count=0, text=(null), fd=-1
// r3:   integers_=0x5d8fc8b5b340, count=1000, text=important data, fd=42
//
// === 離開 main ===
//   [解構] ints=0x5d8fc8b5b340, text=0x5d8fc8b5b020, fd=42
//   [解構] ints=0x5d8fc8b5c2f0, text=0x5d8fc8b5d2a0, fd=42
//   [解構] ints=0, text=0, fd=-1
