// ============================================================
// 第 2.4 章 總結：移動賦值運算子（Move Assignment Operator）
// ============================================================
//
// 【主題資訊 Information】
//   簽名    ：ClassName& operator=(ClassName&& other) noexcept;
//   標準版本：移動賦值運算子 / 右值參考 / noexcept   C++11
//             = default / = delete                    C++11
//   標頭檔  ：<utility>（std::move、std::swap）
//   複雜度  ：移動賦值 O(1)（釋放舊資源 + 搬指標）；複製賦值 O(N)
//   本檔宣告的標準：C++17
//
// 【詳細解釋 Explanation】
//
// 【1. 移動賦值比移動建構「多一件事」：先處理自己的舊資源】
//   移動建構子面對的是一個尚未初始化的物件，直接接手即可。
//   移動賦值運算子面對的是一個「已經擁有資源」的物件，
//   所以必須多一步：先把自己原本的資源釋放掉，否則就是記憶體洩漏。
//   完整五步驟：
//       ① 自我賦值檢查：if (this != &other)
//       ② 釋放自己的舊資源：delete[] data_;
//       ③ 接管來源的資源：data_ = other.data_;
//       ④ 清空來源：other.data_ = nullptr;
//       ⑤ 回傳 *this
//
// 【2. 為什麼一定要做自我賦值檢查】
//   考慮 a = std::move(a); 這種寫法（在泛型演算法中可能間接發生）：
//   若沒有檢查，步驟 ② 會先 delete 掉 data_，
//   而步驟 ③ 接著要讀取 other.data_ —— 那正是剛剛被釋放的那個指標。
//   結果是「先釋放再使用」，屬未定義行為。
//   所以 if (this != &other) 不是防呆，是正確性的必要條件。
//
// 【3. 三種實作風格的取捨】
//   風格 1（直接實作）：分別寫 operator=(const T&) 與 operator=(T&&)。
//       優點：每條路徑都最佳化到底，沒有多餘動作。
//       缺點：兩份幾乎重複的程式碼，容易改一邊忘另一邊。
//   風格 2（swap 慣用法）：各自實作，但用 swap 完成資源交換。
//       優點：例外安全性好，舊資源交給對方的解構子處理。
//   風格 3（Copy-and-Swap）：只寫一個 operator=(T other) + swap。
//       優點：一個函式同時處理複製與移動 ——
//             傳左值 → 參數走複製建構 → swap
//             傳右值 → 參數走移動建構 → swap
//             而且天然具備自我賦值安全與強例外保證。
//       缺點：對左值多做一次不必要的複製（相較風格 1 的最佳路徑）。
//   實務建議：正確性優先選風格 3；極端效能敏感的路徑才用風格 1。
//
// 【4. 為什麼移動賦值也要標 noexcept】
//   與移動建構子相同：std::vector 等容器在重新配置、std::swap、
//   以及各種泛型演算法中，會依 noexcept 決定要用移動還是退回複製。
//   沒標 noexcept 的移動賦值，在許多標準庫路徑上等於白寫。
//
// 【概念補充 Concept Deep Dive】
//
// (A) Copy-and-Swap 為何天然自我賦值安全
//     operator=(T other) 是「按值」接收參數 —— 進來時就已經複製/移動了一份。
//     即使呼叫端寫 a = a;，那也只是把 a 複製到 other，
//     再和 a 交換 —— 全程沒有任何「先釋放再使用」的機會。
//     這是它最被低估的優點：不需要寫 if (this != &other) 也不會出錯。
//
// (B) Copy-and-Swap 的強例外保證
//     所有可能拋例外的動作（配置記憶體）都發生在「參數建構」階段，
//     那時 *this 還完全沒被動過。
//     一旦進入函式本體，剩下的只有 swap —— 而 swap 是 noexcept 的。
//     所以要嘛完全成功、要嘛 *this 保持原狀，不會有「改到一半」的狀態。
//
// (C) 移動賦值與 std::swap 的關係
//     std::swap 的預設實作正是「一次移動建構 + 兩次移動賦值」。
//     所以一個型別的 swap 效能，完全取決於它的移動操作寫得好不好；
//     這也是為什麼本檔的示範中，std::swap 會觸發移動賦值的輸出。
//
// (D) 移動賦值後兩邊的狀態
//     來源必須進入「有效但未指定」狀態（可安全解構、可重新賦值）。
//     若用 Copy-and-Swap，來源會拿到「*this 原本的舊資源」——
//     那些資源會在參數 other 離開函式時被正常釋放，這是它處理舊資源的方式。
//
// 【注意事項 Pay Attention】
//   1. 移動賦值必須先釋放自己的舊資源，否則洩漏。
//   2. 必須處理自我賦值（a = std::move(a)），否則會「先釋放再使用」（UB）。
//   3. 移動賦值務必標 noexcept，否則容器與演算法會退回複製。
//   4. 必須回傳 *this（型別為 ClassName&），以支援連鎖賦值。
//   5. 來源在移動後須處於有效但未指定狀態，不可假設其內容。
//   6. Copy-and-Swap 天然具備自我賦值安全與強例外保證，代價是左值多一次複製。
//   7. 成員初始化與解構順序依「宣告順序」，與書寫順序無關。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】移動賦值運算子
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 移動賦值和移動建構差在哪？
//     答：移動建構面對的是尚未初始化的物件，直接接手資源即可。
//         移動賦值面對的是「已經擁有資源」的物件，必須多做兩件事：
//         先釋放自己的舊資源（否則洩漏），並處理自我賦值。
//         另外它要回傳 *this 以支援連鎖賦值。
//     追問：為什麼移動建構不用自我賦值檢查？→ 因為建構時 *this 還不存在，
//         不可能有 a(std::move(a)) 這種情況。
//
// 🔥 Q2. 為什麼移動賦值要寫 if (this != &other)？
//     答：考慮 a = std::move(a);。若沒有檢查，會先 delete 掉自己的 data_，
//         接著又要從 other.data_ 讀取 —— 而那正是剛被釋放的指標，
//         形成「先釋放再使用」的未定義行為。
//     追問：Copy-and-Swap 也需要這個檢查嗎？→ 不需要。
//         它按值接收參數，進來時就已經複製/移動一份，
//         全程不會出現「先釋放再使用」，天然自我賦值安全。
//
// 🔥 Q3. Copy-and-Swap 的優缺點是什麼？
//     答：優點是一個 operator=(T other) 同時處理複製與移動
//         （左值走複製建構、右值走移動建構），
//         而且天然具備自我賦值安全與強例外保證。
//         缺點是對左值賦值時，相較於「直接實作」的最佳路徑會多一次複製。
//     追問：什麼時候該選直接實作？→ 熱路徑上、且左值賦值極頻繁時。
//         一般情況正確性與可維護性更重要，Copy-and-Swap 是較好的預設。
//
// ⚠️ 陷阱. 移動賦值忘了釋放自己原本的資源，會發生什麼？
//     答：記憶體洩漏。原本 *this 持有的那塊記憶體再也沒有人指向它，
//         永遠不會被釋放。程式不會崩潰、不會有警告 ——
//         只會在長時間執行後記憶體越用越多。
//     為什麼會錯：把移動賦值誤當成移動建構來寫（只想著「接手來源」）。
//         關鍵差異在於：賦值的目標是一個「已經有內容」的物件，
//         接手新資源之前，必須先妥善處理舊的。
// ═══════════════════════════════════════════════════════════════════════════
//
// 編譯：g++ -std=c++17 -Wall -Wextra -o summary summary.cpp
// ============================================================
// 【簽名】ClassName& operator=(ClassName&& other) noexcept
//
// 【步驟】
//   1. 自我賦值檢查：if (this != &other)
//   2. 釋放自己的舊資源：delete[] data_;
//   3. 接管來源的資源：data_ = other.data_;
//   4. 清空來源：other.data_ = nullptr;
//   5. 回傳 *this
//
// 【三種實作風格】
//   風格 1（直接實作）：分別寫 operator=(const T&) 和 operator=(T&&)
//   風格 2（swap 慣用法）：各自用 swap 實作
//   風格 3（Copy-and-Swap）：operator=(T other) 傳值 + swap（推薦！）
//     → 一個函式同時處理複製和移動
//     → 左值 → 參數拷貝建構 → swap
//     → 右值 → 參數移動建構 → swap
//
// 【觸發時機】
//   a = std::move(b);        // 明確移動
//   a = Tracker("temp");     // 臨時物件是右值
//   a = make_func();         // 函式回傳值是右值
//   std::swap(a, b);         // swap 內部使用移動賦值
// ============================================================

#include <iostream>
#include <cstring>
#include <utility>
#include <algorithm>
#include <string>
#include <vector>

// ============================================================
// 風格 1：分別實作複製賦值和移動賦值
// ============================================================
class IntArray {
    int* data_;
    size_t size_;
public:
    explicit IntArray(size_t n = 0)
        : data_(n ? new int[n]() : nullptr), size_(n) {}

    ~IntArray() { delete[] data_; }

    IntArray(const IntArray& o)
        : data_(o.size_ ? new int[o.size_] : nullptr), size_(o.size_) {
        if (data_) std::copy(o.data_, o.data_ + size_, data_);
        std::cout << "  [複製建構]\n";
    }

    IntArray(IntArray&& o) noexcept : data_(o.data_), size_(o.size_) {
        o.data_ = nullptr; o.size_ = 0;
        std::cout << "  [移動建構]\n";
    }

    // ★ 複製賦值
    IntArray& operator=(const IntArray& o) {
        std::cout << "  [複製賦值]\n";
        if (this != &o) {
            delete[] data_;
            size_ = o.size_;
            data_ = size_ ? new int[size_] : nullptr;
            if (data_) std::copy(o.data_, o.data_ + size_, data_);
        }
        return *this;
    }

    // ★ 移動賦值
    IntArray& operator=(IntArray&& o) noexcept {
        std::cout << "  [移動賦值]\n";
        if (this != &o) {
            delete[] data_;
            data_ = o.data_;  size_ = o.size_;
            o.data_ = nullptr; o.size_ = 0;
        }
        return *this;
    }

    void fill(int v) { for (size_t i = 0; i < size_; ++i) data_[i] = v; }
    size_t size() const { return size_; }
    int* data() const { return data_; }
};

// ============================================================
// 風格 3：Copy-and-Swap（推薦）
// ============================================================
class SwapArray {
    int* data_;
    size_t size_;
public:
    explicit SwapArray(size_t n = 0)
        : data_(n ? new int[n]() : nullptr), size_(n) {}

    ~SwapArray() { delete[] data_; }

    SwapArray(const SwapArray& o)
        : data_(o.size_ ? new int[o.size_] : nullptr), size_(o.size_) {
        if (data_) std::copy(o.data_, o.data_ + size_, data_);
        std::cout << "  [SwapArray 複製建構]\n";
    }

    SwapArray(SwapArray&& o) noexcept : data_(o.data_), size_(o.size_) {
        o.data_ = nullptr; o.size_ = 0;
        std::cout << "  [SwapArray 移動建構]\n";
    }

    friend void swap(SwapArray& a, SwapArray& b) noexcept {
        std::swap(a.data_, b.data_);
        std::swap(a.size_, b.size_);
    }

    // ★ 一個 operator= 同時處理複製和移動
    SwapArray& operator=(SwapArray other) noexcept {  // 傳值！
        std::cout << "  [SwapArray 統一賦值] swap\n";
        swap(*this, other);
        return *this;
    }

    size_t size() const { return size_; }
    int* data() const { return data_; }
};

// ============================================================
// 追蹤觸發時機
// ============================================================
class Tracker {
    std::string label_;
public:
    Tracker(const char* l) : label_(l) {}
    Tracker(const Tracker& o) : label_(o.label_) {}
    Tracker(Tracker&& o) noexcept : label_(std::move(o.label_)) { o.label_ = "(empty)"; }

    Tracker& operator=(const Tracker& o) {
        label_ = o.label_;
        std::cout << "  [Tracker 複製賦值] " << label_ << "\n";
        return *this;
    }
    Tracker& operator=(Tracker&& o) noexcept {
        label_ = std::move(o.label_); o.label_ = "(empty)";
        std::cout << "  [Tracker 移動賦值] " << label_ << "\n";
        return *this;
    }
    const std::string& label() const { return label_; }
};

Tracker make_tracker() { return Tracker("factory"); }

int main() {
    // ============================================================
    // 1. 風格 1：分別實作
    // ============================================================
    std::cout << "===== 1. 分別實作 =====\n";
    {
        IntArray a(5); a.fill(10);
        IntArray b(3); b.fill(20);

        std::cout << "  複製賦值 b = a:\n";
        b = a;

        IntArray c(8); c.fill(30);
        std::cout << "  移動賦值 b = move(c):\n";
        b = std::move(c);
        std::cout << "  c.size()=" << c.size() << " (已被掏空)\n";
    }
    std::cout << "\n";

    // ============================================================
    // 2. 風格 3：Copy-and-Swap
    // ============================================================
    std::cout << "===== 2. Copy-and-Swap =====\n";
    {
        SwapArray a(5);
        SwapArray b;

        std::cout << "  複製路徑 b = a:\n";
        b = a;  // 參數以複製建構 → swap

        std::cout << "  移動路徑 b = move(a):\n";
        b = std::move(a);  // 參數以移動建構 → swap

        std::cout << "  臨時物件 b = SwapArray(10):\n";
        b = SwapArray(10);
    }
    std::cout << "\n";

    // ============================================================
    // 3. 觸發時機
    // ============================================================
    std::cout << "===== 3. 觸發時機 =====\n";
    {
        Tracker a("A"), b("B"), c("C");

        std::cout << "  直接賦值左值（複製）:\n";
        a = b;

        std::cout << "  std::move（移動）:\n";
        a = std::move(c);

        std::cout << "  臨時物件（移動）:\n";
        a = Tracker("temp");

        std::cout << "  函式回傳值（移動）:\n";
        a = make_tracker();

        std::cout << "  swap 內部（移動）:\n";
        Tracker x("X"), y("Y");
        std::swap(x, y);
        std::cout << "  x=" << x.label() << " y=" << y.label() << "\n";
    }

    std::cout << "\n=== 重點整理 ===\n";
    std::cout << "  風格 1：分別寫 operator=(const T&) 和 operator=(T&&)\n";
    std::cout << "  風格 3（推薦）：operator=(T other) + swap\n";
    std::cout << "  移動觸發：std::move、臨時物件、函式回傳值、swap\n";
    std::cout << "  移動賦值要加 noexcept\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "summary.cpp" -o moveassign_summary

// === 預期輸出 ===
// ===== 1. 分別實作 =====
//   複製賦值 b = a:
//   [複製賦值]
//   移動賦值 b = move(c):
//   [移動賦值]
//   c.size()=0 (已被掏空)
//
// ===== 2. Copy-and-Swap =====
//   複製路徑 b = a:
//   [SwapArray 複製建構]
//   [SwapArray 統一賦值] swap
//   移動路徑 b = move(a):
//   [SwapArray 移動建構]
//   [SwapArray 統一賦值] swap
//   臨時物件 b = SwapArray(10):
//   [SwapArray 統一賦值] swap
//
// ===== 3. 觸發時機 =====
//   直接賦值左值（複製）:
//   [Tracker 複製賦值] B
//   std::move（移動）:
//   [Tracker 移動賦值] C
//   臨時物件（移動）:
//   [Tracker 移動賦值] temp
//   函式回傳值（移動）:
//   [Tracker 移動賦值] factory
//   swap 內部（移動）:
//   [Tracker 移動賦值] Y
//   [Tracker 移動賦值] X
//   x=Y y=X
//
// === 重點整理 ===
//   風格 1：分別寫 operator=(const T&) 和 operator=(T&&)
//   風格 3（推薦）：operator=(T other) + swap
//   移動觸發：std::move、臨時物件、函式回傳值、swap
//   移動賦值要加 noexcept
