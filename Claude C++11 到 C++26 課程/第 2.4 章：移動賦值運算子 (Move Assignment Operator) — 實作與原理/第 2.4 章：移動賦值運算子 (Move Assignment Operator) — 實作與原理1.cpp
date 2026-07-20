// =============================================================================
// 主題: 移動賦值運算子的完整實作 —— 自我賦值、釋放舊資源、回傳 *this
// =============================================================================
//
// 【主題資訊 Information】
//   簽名    ：IntArray& operator=(IntArray&& other) noexcept;
//   標準版本：移動賦值 / 右值參考 / noexcept   C++11
//   標頭檔  ：<algorithm>（std::copy）、<utility>（std::move）
//   複雜度  ：移動賦值 O(1)；複製賦值 O(N)（配置 + 拷貝）
//   本檔宣告的標準：C++17
//
// 【詳細解釋 Explanation】
//
// 【1. 移動賦值的五個步驟】
//       ① if (this != &other)      自我賦值檢查
//       ② delete[] data_;          釋放自己原本的資源（移動建構不需要這步）
//       ③ data_ = other.data_;     接管來源的資源
//       ④ other.data_ = nullptr;   讓來源不再擁有它
//       ⑤ return *this;            支援連鎖賦值 a = b = c
//   ② 和 ⑤ 是移動賦值相對於移動建構「多出來」的兩件事。
//
// 【2. 為什麼 ① 不可省略】
//   若呼叫端寫 arr = std::move(arr);（在泛型演算法中可能間接發生），
//   沒有檢查時的執行順序會是：
//       delete[] data_;          ← 釋放了那塊記憶體
//       data_ = other.data_;     ← other 就是自己，讀到剛被釋放的指標
//   結果是「先釋放再使用」，屬未定義行為 —— 可能看似正常、可能崩潰，
//   也可能在無關的地方破壞堆積，沒有固定的表現。
//
// 【3. 為什麼 ② 不可省略】
//   賦值的目標是一個「已經擁有資源」的物件。
//   若直接把 data_ 指向新資源而不先 delete 舊的，
//   原本那塊記憶體就再也沒有人指向它 —— 記憶體洩漏。
//   這種錯誤不會崩潰、不會有警告，只會在長時間執行後越用越多記憶體。
//
// 【4. 為什麼要回傳 IntArray&（而不是 void 或值）】
//   回傳 *this 的參考才能支援連鎖賦值 a = b = c;
//   （這是內建型別的行為，使用者定義型別應保持一致）。
//   回傳「值」則會產生多餘的複製，回傳 void 則失去連鎖能力。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 移動賦值與移動建構的分工
//     移動建構：目標物件「還不存在」→ 只需接手 + 清空來源。
//     移動賦值：目標物件「已經存在且有資源」→ 還要先釋放舊的、處理自我賦值。
//     兩者都必須標 noexcept，且都必須讓來源進入有效但未指定的狀態。
//
// (B) 本檔為何用 std::copy 而非 memcpy
//     std::copy 對 int 這類平凡可複製型別，實作通常會退化成 memmove，
//     效能相同；但它同時適用於非平凡型別（會正確呼叫複製建構子），
//     語意更安全、也更泛型。自管緩衝區時應優先使用它。
//
// (C) 成員宣告順序在本檔為何安全
//     本類別宣告順序是 data_ → size_。
//     一般建構子寫 : data_(size ? new int[size]() : nullptr), size_(size)
//     其中 data_ 用的是「參數 size」而不是成員 size_，因此不依賴其他成員。
//     ⚠️ 若改成 data_(new int[size_]) 就會讀到尚未初始化的 size_ ——
//        初始化一律依「宣告順序」，不看初始化列表的書寫順序。
//
// 【注意事項 Pay Attention】
//   1. 移動賦值必須做自我賦值檢查，否則會「先釋放再使用」（UB）。
//   2. 必須先釋放自己原本的資源，否則洩漏。
//   3. 必須把來源清空（指標 nullptr、大小 0），否則 double free。
//   4. 必須標 noexcept，並回傳 *this。
//   5. 被移動後的物件仍須可安全解構、可重新賦值。
//   6. 成員初始化依「宣告順序」，與書寫順序無關。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】移動賦值的完整實作
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 請寫出移動賦值運算子，並說明為什麼需要自我賦值檢查。
//     答：ClassName& operator=(ClassName&& o) noexcept {
//             if (this != &o) {          // ① 自我賦值檢查
//                 delete[] data_;        // ② 釋放自己的舊資源
//                 data_ = o.data_;       // ③ 接管
//                 size_ = o.size_;
//                 o.data_ = nullptr;     // ④ 清空來源
//                 o.size_ = 0;
//             }
//             return *this;              // ⑤ 支援連鎖賦值
//         }
//         沒有 ① 時，a = std::move(a) 會先 delete 掉 data_，
//         再從 o.data_（同一個指標）讀取 —— 先釋放再使用，是未定義行為。
//     追問：移動建構子也需要 ① 嗎？→ 不需要，建構時 *this 還不存在。
//
// 🔥 Q2. 移動賦值和移動建構最大的差別是什麼？
//     答：移動賦值的目標物件「已經擁有資源」，
//         所以必須多做兩件事：先釋放舊資源（否則洩漏）、處理自我賦值。
//         另外它要回傳 *this 以支援連鎖賦值。
//     追問：兩者有什麼共同要求？→ 都要標 noexcept、
//         都要把來源清空成有效但未指定的狀態。
//
// ⚠️ 陷阱. 移動賦值裡忘了 delete[] data_ 會怎樣？程式會崩潰嗎？
//     答：不會崩潰，會「記憶體洩漏」。原本 *this 持有的那塊記憶體
//         再也沒有人指向它，永遠不會被釋放。
//         沒有警告、沒有錯誤 —— 只有在長時間執行後才會發現記憶體一直上升。
//     為什麼會錯：把移動賦值照著移動建構的樣子寫（只想到「接手來源」）。
//         關鍵差異在於：賦值的目標是「已經有內容」的物件，
//         接手新資源之前，必須先處理舊的。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <cstring>
#include <utility>
#include <algorithm>

class IntArray {
    int* data_;
    size_t size_;

    void log(const char* action) const {
        std::cout << "  [" << action << "] size=" << size_
                  << ", data_=" << data_ << "\n";
    }

public:
    // 一般建構子
    explicit IntArray(size_t size = 0)
        : data_(size ? new int[size]() : nullptr)
        , size_(size)
    {
        log("建構");
    }

    // 解構子
    ~IntArray() {
        log("解構");
        delete[] data_;
    }

    // 複製建構子
    IntArray(const IntArray& other)
        : data_(other.size_ ? new int[other.size_] : nullptr)
        , size_(other.size_)
    {
        if (data_) {
            std::copy(other.data_, other.data_ + size_, data_);
        }
        log("複製建構");
    }

    // 移動建構子
    IntArray(IntArray&& other) noexcept
        : data_(other.data_)
        , size_(other.size_)
    {
        other.data_ = nullptr;
        other.size_ = 0;
        log("移動建構");
    }

    // ========== 複製賦值運算子 ==========
    IntArray& operator=(const IntArray& other) {
        std::cout << "  [複製賦值] 開始\n";

        if (this == &other) {  // 自我賦值檢查
            std::cout << "    自我賦值，略過\n";
            return *this;
        }

        // 步驟 1：釋放舊資源
        delete[] data_;

        // 步驟 2：配置新資源並複製
        size_ = other.size_;
        data_ = size_ ? new int[size_] : nullptr;
        if (data_) {
            std::copy(other.data_, other.data_ + size_, data_);
        }

        std::cout << "    配置新記憶體 + 複製完成\n";
        return *this;
    }

    // ========== 移動賦值運算子 ==========
    IntArray& operator=(IntArray&& other) noexcept {
        std::cout << "  [移動賦值] 開始\n";

        if (this == &other) {  // 自我賦值檢查
            std::cout << "    自我移動賦值，略過\n";
            return *this;
        }

        // 步驟 1：釋放自己的舊資源
        delete[] data_;

        // 步驟 2：接管來源的資源
        data_ = other.data_;
        size_ = other.size_;

        // 步驟 3：清空來源
        other.data_ = nullptr;
        other.size_ = 0;

        std::cout << "    釋放舊資源 + 接管完成，零成本搬移\n";
        return *this;
    }

    // 輔助方法
    void fill(int value) {
        for (size_t i = 0; i < size_; ++i) data_[i] = value;
    }

    void print(const char* label) const {
        std::cout << label << ": size=" << size_ << ", data_=" << data_;
        if (data_ && size_ > 0) {
            std::cout << ", 前3個=[";
            for (size_t i = 0; i < 3 && i < size_; ++i) {
                if (i) std::cout << ",";
                std::cout << data_[i];
            }
            std::cout << "...]";
        }
        std::cout << "\n";
    }
};

int main() {
    std::cout << "=== 建立物件 ===\n";
    IntArray a(5);
    a.fill(10);
    IntArray b(3);
    b.fill(20);

    a.print("a");
    b.print("b");

    std::cout << "\n=== 複製賦值：b = a ===\n";
    b = a;
    a.print("a");
    b.print("b");

    std::cout << "\n=== 建立新物件 c ===\n";
    IntArray c(8);
    c.fill(30);
    c.print("c");

    std::cout << "\n=== 移動賦值：b = std::move(c) ===\n";
    b = std::move(c);
    b.print("b");
    c.print("c");  // 已被掏空

    std::cout << "\n=== 離開 main ===\n";
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 2.4 章：移動賦值運算子 (Move Assignment Operator) — 實作與原理1.cpp" -o ma_demo1
//  ⚠️ 注意：下列輸出中的記憶體位址「每次執行都不同」（受堆積/堆疊配置與 ASLR 影響），
//     此處僅為某一次實際執行的樣本，不具重現性；其餘文字內容則是穩定可重現的。

// === 預期輸出 ===
// === 建立物件 ===
//   [建構] size=5, data_=0x64abae3dd020
//   [建構] size=3, data_=0x64abae3dd340
// a: size=5, data_=0x64abae3dd020, 前3個=[10,10,10...]
// b: size=3, data_=0x64abae3dd340, 前3個=[20,20,20...]
//
// === 複製賦值：b = a ===
//   [複製賦值] 開始
//     配置新記憶體 + 複製完成
// a: size=5, data_=0x64abae3dd020, 前3個=[10,10,10...]
// b: size=5, data_=0x64abae3dd340, 前3個=[10,10,10...]
//
// === 建立新物件 c ===
//   [建構] size=8, data_=0x64abae3dd360
// c: size=8, data_=0x64abae3dd360, 前3個=[30,30,30...]
//
// === 移動賦值：b = std::move(c) ===
//   [移動賦值] 開始
//     釋放舊資源 + 接管完成，零成本搬移
// b: size=8, data_=0x64abae3dd360, 前3個=[30,30,30...]
// c: size=0, data_=0
//
// === 離開 main ===
//   [解構] size=0, data_=0
//   [解構] size=8, data_=0x64abae3dd360
//   [解構] size=5, data_=0x64abae3dd020
