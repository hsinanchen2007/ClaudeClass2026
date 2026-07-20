// =============================================================================
// 主題: 賦值運算子的三種實作風格 —— 直接實作 vs swap vs Copy-and-Swap
// =============================================================================
//
// 【主題資訊 Information】
//   風格 1：operator=(const T&) 與 operator=(T&&) 各寫一份（直接實作）
//   風格 2：各自實作，但用 swap 交換資源
//   風格 3：operator=(T other) 單一函式 + swap（Copy-and-Swap）
//   標準版本：右值參考 / 移動賦值 / noexcept   C++11
//   標頭檔  ：<algorithm>（std::copy、std::swap）、<utility>
//   複雜度  ：移動賦值 O(1)；複製賦值 O(N)
//   本檔宣告的標準：C++17
//
// 【詳細解釋 Explanation】
//
// 【1. 風格 1：直接實作 —— 最快，但最容易寫錯】
//   分別寫 operator=(const T&) 與 operator=(T&&)，每條路徑各自最佳化。
//   優點：沒有任何多餘動作，左值賦值只做一次配置 + 拷貝。
//   缺點：
//     * 兩份幾乎重複的程式碼，改一邊忘另一邊是常見 bug
//     * 必須自己記得寫自我賦值檢查
//     * 例外安全性要自己顧：若 new 拋例外時舊資源已被 delete，
//       物件就處於毀損狀態（無法回復）
//
// 【2. 風格 3：Copy-and-Swap —— 一個函式解決全部】
//       ClassName& operator=(ClassName other) {   // 注意：按值接收
//           swap(*this, other);
//           return *this;
//       }
//   運作方式：
//     * 呼叫端傳左值 → 參數 other 走「複製建構」
//     * 呼叫端傳右值 → 參數 other 走「移動建構」
//     * 然後和 *this 交換；舊資源留在 other 身上，隨 other 解構自然釋放
//   一個函式同時服務兩種情況，這是它最大的價值。
//
// 【3. Copy-and-Swap 的兩個「免費」保證】
//   (a) 自我賦值安全：按值接收時已經先複製一份，
//       即使寫 a = a; 也不會出現「先釋放再使用」，不需要 if (this != &other)。
//   (b) 強例外保證：所有可能拋例外的動作（配置記憶體）都發生在
//       「參數建構」階段，那時 *this 完全沒被動過。
//       進入函式本體後只剩 swap（noexcept）——
//       所以要嘛完全成功、要嘛 *this 維持原狀，不會有改到一半的狀態。
//
// 【4. 代價與取捨】
//   Copy-and-Swap 對「左值賦值」會比風格 1 多一次複製
//   （風格 1 可以在容量足夠時重用既有緩衝區，Copy-and-Swap 一定重新配置）。
//   實務建議：
//     * 預設用 Copy-and-Swap —— 正確性與可維護性明顯較好
//     * 只在效能剖析證明這是瓶頸時，才改寫成風格 1
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼 swap 必須是 noexcept
//     Copy-and-Swap 的強例外保證完全建立在「swap 不會拋」這個前提上。
//     若 swap 可能拋例外，交換到一半失敗時兩個物件都會處於不一致狀態。
//     自管資源的 swap 只是交換幾個指標與整數，本來就不會拋 —— 標上 noexcept 即可。
//
// (B) 為什麼參數要「按值」而不是 const T&
//     按值接收才能讓編譯器依引數的 value category 自動選擇
//     複製建構或移動建構 —— 這正是「一個函式同時處理兩種情況」的關鍵。
//     若寫成 const T&，右值也只會被複製，就失去移動的好處了。
//
// (C) 這個模式與 Rule of Zero 的關係
//     本檔三種風格都假設類別自管原始記憶體。
//     若成員改用 std::vector / std::string / std::unique_ptr，
//     五個特殊成員函式一個都不用寫，編譯器生成的版本就已正確且高效 ——
//     那才是現代 C++ 的首選（Rule of Zero）。
//     本檔的價值在於理解「標準庫幫你做了什麼」。
//
// 【注意事項 Pay Attention】
//   1. Copy-and-Swap 的參數必須「按值」接收，寫成 const T& 就失去移動能力。
//   2. 用於 Copy-and-Swap 的 swap 必須是 noexcept，否則強例外保證不成立。
//   3. 風格 1 必須自己寫自我賦值檢查；Copy-and-Swap 不需要（天然安全）。
//   4. Copy-and-Swap 對左值賦值會多一次複製，是換取正確性的代價。
//   5. 移動賦值一律標 noexcept，否則容器與演算法會退回複製。
//   6. 現代首選是 Rule of Zero —— 用 RAII 成員，五個都不用寫。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】Copy-and-Swap 慣用法
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. Copy-and-Swap 為什麼一個函式就能同時處理複製與移動賦值？
//     答：因為參數是「按值」接收的。
//         呼叫端傳左值時，參數走複製建構；傳右值時走移動建構 ——
//         這一步由重載決議自動完成。
//         進入函式本體後只需 swap(*this, other)，兩種情況的後續完全相同。
//     追問：若把參數改成 const T& 會怎樣？→ 右值也只會被複製，
//         移動的好處完全消失，就退化成單純的複製賦值了。
//
// 🔥 Q2. Copy-and-Swap 為什麼不需要自我賦值檢查？
//     答：因為按值接收時已經先複製/移動了一份到參數 other。
//         即使呼叫端寫 a = a;，交換的也是 a 與「a 的副本」，
//         全程不存在「先釋放再使用」的空窗，天然安全。
//     追問：那它的例外安全性如何？→ 是強例外保證。
//         所有可能拋例外的配置都在參數建構階段完成，
//         那時 *this 尚未被動過；之後只剩 noexcept 的 swap。
//
// ⚠️ 陷阱. Copy-and-Swap 既然這麼好，是不是應該一律採用？
//     答：不是。它對「左值賦值」會比直接實作多一次複製 ——
//         直接實作可以在容量足夠時重用既有緩衝區，Copy-and-Swap 一定重新配置。
//         在賦值極頻繁的熱路徑上，這個差距是實際的。
//     為什麼會錯：把「例外安全 + 程式碼簡潔」當成無代價的優點。
//         正確作法是：預設用 Copy-and-Swap，
//         等效能剖析證明它是瓶頸時，才針對該處改寫成直接實作。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <cstring>
#include <utility>
#include <algorithm>

// ===== 風格 1：直接實作 =====
class Style1 {
    int* data_;
    size_t size_;
public:
    explicit Style1(size_t n = 0) : data_(n ? new int[n]() : nullptr), size_(n) {}
    ~Style1() { delete[] data_; }

    Style1(const Style1& o) : data_(o.size_ ? new int[o.size_] : nullptr), size_(o.size_) {
        if (data_) std::copy(o.data_, o.data_ + size_, data_);
    }
    Style1(Style1&& o) noexcept : data_(o.data_), size_(o.size_) {
        o.data_ = nullptr; o.size_ = 0;
    }

    Style1& operator=(const Style1& o) {
        if (this != &o) {
            delete[] data_;
            size_ = o.size_;
            data_ = size_ ? new int[size_] : nullptr;
            if (data_) std::copy(o.data_, o.data_ + size_, data_);
        }
        return *this;
    }

    Style1& operator=(Style1&& o) noexcept {
        if (this != &o) {
            delete[] data_;
            data_ = o.data_;  size_ = o.size_;
            o.data_ = nullptr; o.size_ = 0;
        }
        return *this;
    }
};

// ===== 風格 2：swap 慣用法 =====
class Style2 {
    int* data_;
    size_t size_;
public:
    explicit Style2(size_t n = 0) : data_(n ? new int[n]() : nullptr), size_(n) {}
    ~Style2() { delete[] data_; }

    Style2(const Style2& o) : data_(o.size_ ? new int[o.size_] : nullptr), size_(o.size_) {
        if (data_) std::copy(o.data_, o.data_ + size_, data_);
    }
    Style2(Style2&& o) noexcept : data_(o.data_), size_(o.size_) {
        o.data_ = nullptr; o.size_ = 0;
    }

    friend void swap(Style2& a, Style2& b) noexcept {
        using std::swap;
        swap(a.data_, b.data_);
        swap(a.size_, b.size_);
    }

    Style2& operator=(const Style2& o) {
        if (this != &o) {
            Style2 tmp(o);       // 複製到臨時物件
            swap(*this, tmp);    // 交換
        }                        // tmp 解構，釋放舊資源
        return *this;
    }

    Style2& operator=(Style2&& o) noexcept {
        swap(*this, o);          // 交換，o 帶走舊資源
        return *this;            // o 解構時釋放
    }
};

// ===== 風格 3：Copy-and-Swap（一個 operator= 搞定全部）=====
class Style3 {
    int* data_;
    size_t size_;
public:
    explicit Style3(size_t n = 0) : data_(n ? new int[n]() : nullptr), size_(n) {}
    ~Style3() { delete[] data_; }

    Style3(const Style3& o) : data_(o.size_ ? new int[o.size_] : nullptr), size_(o.size_) {
        if (data_) std::copy(o.data_, o.data_ + size_, data_);
    }
    Style3(Style3&& o) noexcept : data_(o.data_), size_(o.size_) {
        o.data_ = nullptr; o.size_ = 0;
    }

    friend void swap(Style3& a, Style3& b) noexcept {
        using std::swap;
        swap(a.data_, b.data_);
        swap(a.size_, b.size_);
    }

    // 一個 operator= 同時處理複製和移動
    Style3& operator=(Style3 other) noexcept {  // 注意：傳值！
        swap(*this, other);
        return *this;
    }

    size_t size() const { return size_; }
    int* data() const { return data_; }
};

// ===== 輔助：印出物件狀態 =====
template<typename T>
void show(const char* label, const T& obj) {
    std::cout << "  " << label << " → data=" << obj.data()
              << ", size=" << obj.size() << "\n";
}

int main() {
    std::cout << "===== 風格 1：直接實作 =====\n";
    {
        Style1 a(5);
        std::cout << "  a 建立 (size=5)\n";

        // 複製賦值
        Style1 b;
        b = a;
        std::cout << "  b = a（複製賦值）完成\n";

        // 移動賦值
        Style1 c;
        c = std::move(a);
        std::cout << "  c = std::move(a)（移動賦值）完成\n";

        // 自我賦值（有 if(this!=&o) 保護）
        b = b;
        std::cout << "  b = b（自我賦值）完成\n";
    }

    std::cout << "\n===== 風格 2：swap 慣用法 =====\n";
    {
        Style2 a(5);
        std::cout << "  a 建立 (size=5)\n";

        // 複製賦值：內部複製到 tmp 再 swap
        Style2 b;
        b = a;
        std::cout << "  b = a（複製賦值，內部 copy + swap）完成\n";

        // 移動賦值：直接 swap
        Style2 c;
        c = std::move(a);
        std::cout << "  c = std::move(a)（移動賦值，直接 swap）完成\n";
    }

    std::cout << "\n===== 風格 3：Copy-and-Swap（傳值參數）=====\n";
    {
        Style3 a(5);
        show("a 建立", a);

        // 複製賦值：參數以複製建構
        Style3 b;
        b = a;
        show("b = a 後的 b", b);
        show("b = a 後的 a", a);   // a 不受影響

        // 移動賦值：參數以移動建構
        Style3 c;
        c = std::move(a);
        show("c = move(a) 後的 c", c);
        show("c = move(a) 後的 a", a);  // a 已被搬空

        // 用暫時物件賦值
        b = Style3(10);
        show("b = Style3(10) 後的 b", b);
    }

    std::cout << "\n所有風格皆正常運作，資源已自動釋放。\n";
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 2.4 章：移動賦值運算子 (Move Assignment Operator) — 實作與原理2.cpp" -o ma_demo2
//  ⚠️ 注意：下列輸出中的記憶體位址「每次執行都不同」（受堆積/堆疊配置與 ASLR 影響），
//     此處僅為某一次實際執行的樣本，不具重現性；其餘文字內容則是穩定可重現的。

// === 預期輸出 ===
// ===== 風格 1：直接實作 =====
//   a 建立 (size=5)
//   b = a（複製賦值）完成
//   c = std::move(a)（移動賦值）完成
//   b = b（自我賦值）完成
//
// ===== 風格 2：swap 慣用法 =====
//   a 建立 (size=5)
//   b = a（複製賦值，內部 copy + swap）完成
//   c = std::move(a)（移動賦值，直接 swap）完成
//
// ===== 風格 3：Copy-and-Swap（傳值參數）=====
//   a 建立 → data=0x639b97219020, size=5
//   b = a 後的 b → data=0x639b97219340, size=5
//   b = a 後的 a → data=0x639b97219020, size=5
//   c = move(a) 後的 c → data=0x639b97219020, size=5
//   c = move(a) 後的 a → data=0, size=0
//   b = Style3(10) 後的 b → data=0x639b97219360, size=10
//
// 所有風格皆正常運作，資源已自動釋放。
