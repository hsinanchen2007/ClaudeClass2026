// =============================================================================
// 主題: 深拷貝 vs 移動 —— 用一個自管記憶體的 Buffer 看清楚差在哪
// =============================================================================
//
// 【主題資訊 Information】
//   語法    ：Buffer(const Buffer& other);        // 複製建構子：深拷貝
//             Buffer(Buffer&& other) noexcept;    // 移動建構子：搬指標
//   標準版本：移動建構子 / 右值參考 / noexcept   皆為 C++11
//   標頭檔  ：<cstring>（strlen/strcpy）、<utility>（std::move）
//   複雜度  ：複製 O(N)（配置 + 逐位元組拷貝）；移動 O(1)（只搬指標）
//   本檔宣告的標準：C++17
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼複製「貴」】
//   Buffer 持有一塊 new 出來的字元陣列。複製建構子必須：
//       (a) 重新 new 一塊同樣大的記憶體   ← 堆積配置，可能觸發系統呼叫
//       (b) 逐位元組把內容拷貝過去        ← O(N)
//   兩件事都與資料量成正比。資料越大，複製越痛。
//
// 【2. 移動為什麼「幾乎免費」】
//   移動建構子做的事只有：
//       (a) 把 other.data_ 這個指標抄過來
//       (b) 把 other.data_ 設成 nullptr
//   完全不碰堆積上的資料。無論緩衝區是 1 KB 還是 1 GB，成本都一樣是 O(1)。
//
// 【3. 「把來源設成 nullptr」不是禮貌，是必要】
//   如果移動後不把 other.data_ 設成 nullptr，就會有兩個物件持有同一個指標，
//   兩者解構時都會 delete[] 它 —— 這就是 double free（未定義行為）。
//   所以移動建構子永遠是「偷走 + 讓來源變成安全的空殼」兩步，缺一不可。
//   本檔的解構子特意處理了 data_ == nullptr 的情況，輸出「(空 buffer)」，
//   讓你親眼看到被移動後的物件仍然被正常解構、而且沒有重複釋放。
//
// 【4. 重載如何自動分流】
//   store(const Buffer&) 與 store(Buffer&&) 兩個版本並存時：
//       store(original)              → original 是 lvalue → const& 版本 → 複製
//       store(Buffer("Temp"))        → prvalue           → && 版本    → 移動
//       store(std::move(original))   → xvalue            → && 版本    → 移動
//   呼叫端不必做任何特別的事，編譯器依 value category 自動選對版本。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼移動建構子要標 noexcept
//     std::vector 擴容時要把舊元素搬到新緩衝區。若移動途中可能拋例外，
//     搬到一半失敗就無法回復（舊的已被掏空、新的不完整），
//     因此 vector 只在移動操作標了 noexcept 時才使用移動，否則退回複製。
//     移動建構子本來就只是搬指標、不配置記憶體，本來就不會拋 —— 標上去即可。
//
// (B) 成員初始化順序：本檔為何安全
//     本類別的成員宣告順序是 data_ 然後 size_。
//     一般建構子只在初始化列表寫了 size_(strlen(str))，
//     data_ 則是在「函式本體」中才配置 —— 此時 size_ 已初始化完成，因此安全。
//     若改寫成 : data_(new char[size_ + 1]), size_(strlen(str)) 就會出事：
//     初始化一律依「宣告順序」進行，data_ 會先用到尚未初始化的 size_。
//     這是本課程反覆強調的規則，請勿調整此處的寫法。
//
// (C) 移動後的物件仍必須是「有效」的
//     被移動後的 Buffer 其 data_ 為 nullptr。它仍然可以被安全解構、
//     可以被重新賦值。這符合標準對「有效但未指定」狀態的要求。
//     本檔的 c_str() 也特意處理了 nullptr，回傳 "(null)" 而不是直接解參考 ——
//     這是自管資源類別應有的防禦。
//
// 【注意事項 Pay Attention】
//   1. 移動建構子必須把來源的指標設成 nullptr，否則會 double free（UB）。
//   2. 移動建構子與移動賦值應標 noexcept，否則 vector 擴容時會退回複製。
//   3. 成員初始化依「宣告順序」，不是初始化列表的書寫順序。
//   4. 被移動後的物件仍須保持可安全解構、可重新賦值的狀態。
//   5. 有名字的 && 參數是 lvalue：store(Buffer&& buf) 內要移動仍須寫 std::move(buf)。
//   6. 這個類別自管 new/delete[] 是為了教學；實務上請直接用
//      std::string 或 std::vector<char>（Rule of Zero），不要自己管記憶體。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】移動建構子的實作
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 移動建構子為什麼一定要把來源的指標設成 nullptr？
//     答：否則兩個物件會持有同一個指標，各自解構時都會 delete[]，
//         造成 double free（未定義行為）。
//         移動的完整語意是「偷走資源 + 讓來源變成安全的空殼」，兩步缺一不可。
//     追問：那來源之後還能用嗎？→ 可以安全解構、可以重新賦值，
//         但不可讀取其內容（標準稱為「有效但未指定」狀態）。
//
// 🔥 Q2. 移動比複製快多少？跟資料量有關嗎？
//     答：複製是 O(N)：要配置新記憶體再逐位元組拷貝，成本與資料量成正比。
//         移動是 O(1)：只抄一個指標、清一個指標，與資料量完全無關。
//         緩衝區越大，兩者差距越懸殊。
//     追問：那小物件也該用移動嗎？→ 對 int 這類基本型別，移動就是複製，
//         加 std::move 沒有意義。移動只對「持有堆積資源」的型別才有價值。
//
// ⚠️ 陷阱. void store(Buffer&& buf) 裡面寫 Buffer local(buf); 會觸發移動嗎？
//     答：不會，會觸發「複製」。因為 buf 有名字，它作為表達式是 lvalue，
//         重載決議會選中複製建構子。必須寫 Buffer local(std::move(buf));
//         才會選中移動建構子 —— 本檔的 store(Buffer&&) 正是這樣寫的。
//     為什麼會錯：以為參數宣告成 && 就代表「函式內用它時都是右值」。
//         實際上 && 只描述「我能接受什麼」，
//         函式內要不要真的搬走，取決於你有沒有寫 std::move。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <cstring>
#include <utility>

class Buffer {
    // ⚠️ 成員初始化一律依「宣告順序」，與初始化列表的書寫順序無關。
    //    本類別的一般建構子只在初始化列表初始化 size_，
    //    data_ 於函式本體才配置（此時 size_ 已就緒），故安全。
    //    請勿改成 : data_(new char[size_ + 1]), size_(...) —— 那會讀到未初始化的 size_。
    char* data_;
    size_t size_;

public:
    // 一般建構子
    Buffer(const char* str) : size_(std::strlen(str)) {
        data_ = new char[size_ + 1];
        std::strcpy(data_, str);
        std::cout << "  [建構] \"" << data_ << "\" (配置 "
                  << (size_ + 1) << " bytes)\n";
    }

    // 解構子
    ~Buffer() {
        if (data_) {
            std::cout << "  [解構] \"" << data_ << "\"\n";
            delete[] data_;
        } else {
            std::cout << "  [解構] (空 buffer)\n";
        }
    }

    // 複製建構子
    Buffer(const Buffer& other) : size_(other.size_) {
        data_ = new char[size_ + 1];
        std::strcpy(data_, other.data_);
        std::cout << "  [複製建構] \"" << data_ << "\" ← 深拷貝，代價高！\n";
    }

    // 移動建構子（C++11 新增）
    Buffer(Buffer&& other) noexcept
        : data_(other.data_), size_(other.size_) {
        other.data_ = nullptr;  // 重要：讓來源物件不再擁有資源
        other.size_ = 0;
        std::cout << "  [移動建構] \"" << data_ << "\" ← 只搬指標，幾乎零成本！\n";
    }

    const char* c_str() const { return data_ ? data_ : "(null)"; }
};

// 接收左值
void store(const Buffer& buf) {
    std::cout << "store(const Buffer&): 需要複製\n";
    Buffer local(buf);  // 複製建構
}

// 接收右值
void store(Buffer&& buf) {
    std::cout << "store(Buffer&&): 可以移動\n";
    Buffer local(std::move(buf));  // 移動建構
}

int main() {
    std::cout << "=== 建立原始 Buffer ===\n";
    Buffer original("Important Data Here");

    std::cout << "\n=== 傳入左值（複製）===\n";
    store(original);

    std::cout << "\n=== 傳入右值（移動）===\n";
    store(Buffer("Temporary Data"));

    std::cout << "\n=== 用 std::move 傳入（移動）===\n";
    store(std::move(original));
    std::cout << "original after move: " << original.c_str() << "\n";

    std::cout << "\n=== 離開 main ===\n";
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 2.2 章：右值參考 && — 基本語法與用途7.cpp" -o buffer_move_demo

// === 預期輸出 ===
// === 建立原始 Buffer ===
//   [建構] "Important Data Here" (配置 20 bytes)
//
// === 傳入左值（複製）===
// store(const Buffer&): 需要複製
//   [複製建構] "Important Data Here" ← 深拷貝，代價高！
//   [解構] "Important Data Here"
//
// === 傳入右值（移動）===
//   [建構] "Temporary Data" (配置 15 bytes)
// store(Buffer&&): 可以移動
//   [移動建構] "Temporary Data" ← 只搬指標，幾乎零成本！
//   [解構] "Temporary Data"
//   [解構] (空 buffer)
//
// === 用 std::move 傳入（移動）===
// store(Buffer&&): 可以移動
//   [移動建構] "Important Data Here" ← 只搬指標，幾乎零成本！
//   [解構] "Important Data Here"
// original after move: (null)
//
// === 離開 main ===
//   [解構] (空 buffer)
