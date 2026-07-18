/*=============================================================================
 * 檔名：22_MoveSemantics.cpp
 * 主題：移動語意 (Move Semantics) - C++11 之後的重大效能利器
 * 適合：學完複製建構/賦值，準備理解「為什麼大物件可以幾乎免費搬家」的人
 *
 * 【課題介紹】
 *   想像 std::vector<int> 內部持有一塊堆積記憶體，存放百萬個整數。
 *   傳統「複製」(copy) 大概要這樣：
 *
 *       新 vector：另外配置一塊一樣大的記憶體 → 把元素一個個複製過去 (O(N))
 *
 *   但很多場景下我們其實不需要保留來源 (例如「臨時物件」、「即將離開作用域的變數」)，
 *   為什麼要複製？我們可以「直接把那塊記憶體的指標搬過來」就好，O(1)！
 *   來源那邊讓它變空 (反正馬上要被銷毀，也不會用到)。這就是「移動 (move)」。
 *
 *       「Move = 不複製內容，只把資源 (通常是指標) 從來源轉移到目標。」
 *
 *   要實作這個機制，C++11 引入：
 *     - 右值參考 (rvalue reference)：T&&
 *     - 移動建構子：T(T&&)
 *     - 移動賦值：T& operator=(T&&)
 *     - std::move：把一般變數「強轉成 rvalue」，告訴編譯器「我同意你搬走它」
 *
 * 【lvalue vs rvalue (粗略理解就好)】
 *   - lvalue：有名字、可以取位址的東西。像區域變數、成員變數。
 *   - rvalue：暫時的、字面值、即將消失的東西。像 42、x + y、回傳值物件。
 *
 *   T&  → 接受 lvalue (跟以前一樣)
 *   T&& → 接受 rvalue (新東西)。所以「move ctor 接受 rvalue」的意思就是
 *         「要搬走的這個傢伙是個臨時物件，動它沒關係」。
 *
 * 【std::move 不是真的搬】
 *   std::move(x) 只是個 cast，把 x 的型別轉成 T&& (rvalue ref)。
 *   真正的「搬」發生在被「move ctor / move 賦值」呼叫時。
 *
 *   特別注意：std::move 之後的變數雖然還可以使用，但其內容已被「掏空」 (狀態未定義
 *   但合法 — valid-but-unspecified)，請當作「只能再次賦值或銷毀」。
 *
 * 【日常實用範例】
 *   寫一個 Buffer 持有一塊 heap 記憶體，
 *   分別實作 copy ctor (深拷貝) 與 move ctor (搬指標)，
 *   印出建構訊息看哪一種被呼叫。
 *
 * 【對應 Leetcode】1480. Running Sum of 1d Array
 *   題目簡述：給 nums，回傳累計和陣列。
 *   為什麼選這題：當解法產生大型 std::vector 結果時，「回傳/接收這個 vector」
 *   到底是 copy 還是 move？我們示範用 std::move 把 vector 從一個變數搬到
 *   另一個，舊變數會被掏空 (size 變 0)，新變數承接全部資料 — 完全 O(1)。
 *
 * 【參考】
 *   https://en.cppreference.com/w/cpp/language/move_constructor
 *   https://en.cppreference.com/w/cpp/utility/move
 *   https://cplusplus.com/reference/utility/move/
 *=============================================================================*/

/*
補充筆記：MoveSemantics
  - move semantics 讓物件資源可以轉移而不是深拷貝。
  - std::move 只是允許移動的轉型，真正轉移由 move constructor/assignment 完成。
  - 被 move 的物件仍可銷毀或重新賦值，但不應依賴原值。
  - move semantics 的目的不是「搬動物件地址」，而是讓物件內部資源的所有權可以便宜轉移。
  - 右值 reference T&& 常用來表示可被偷資源的暫時值；std::move 只是轉型，真正搬移發生在 move constructor 或 move assignment。
  - 被 move 的物件仍然必須可解構、可重新賦值，但其值通常只保證 valid unspecified state；不要假設它還保留原內容。
  - move constructor 應接管來源資源，並把來源設成安全可解構狀態，例如指標改 nullptr。
  - noexcept move constructor 對容器很重要；std::vector 擴容時若 move 可能丟例外，可能退回 copy 以維持例外安全。
  - const 物件通常無法有效 move，因為 move 需要修改來源；std::move(const T&) 多半會呼叫 copy constructor。
  - 小型 trivially copyable 型別 move 和 copy 成本差不多；move 最有價值的是管理 heap memory、file handle、large buffer 的型別。
  - 寫 move 後要檢查 Rule of Five：destructor、copy constructor、copy assignment、move constructor、move assignment 是否語意一致。
*/
#include <iostream>
#include <vector>
#include <utility>      // std::move
#include <cstring>
#include <string>

class Buffer {
private:
    int*  data_;
    size_t size_;

public:
    explicit Buffer(size_t n) : data_(new int[n]{}), size_(n) {
        std::cout << "  [ctor]   配置大小 " << size_ << " 位址 " << (void*)data_ << "\n";
    }

    // ----- 複製建構子 (深拷貝，貴) -----
    Buffer(const Buffer& other) : data_(new int[other.size_]), size_(other.size_) {
        std::memcpy(data_, other.data_, size_ * sizeof(int));
        std::cout << "  [copy]   深拷貝 size=" << size_ << " 來自 " << (void*)other.data_
                  << " 新位址 " << (void*)data_ << "\n";
    }

    // ----- 移動建構子 (搬指標，幾乎免費) -----
    // 注意 noexcept：不丟例外的承諾，這對 std::vector 等容器自動選 move 很關鍵
    Buffer(Buffer&& other) noexcept : data_(other.data_), size_(other.size_) {
        // 把來源「掏空」，否則它解構時會 delete 同一塊記憶體
        other.data_ = nullptr;
        other.size_ = 0;
        std::cout << "  [move]   搬走指標 size=" << size_ << " 位址 " << (void*)data_ << "\n";
    }

    // ----- 複製賦值 -----
    Buffer& operator=(const Buffer& other) {
        if (this == &other) return *this;
        delete[] data_;
        size_ = other.size_;
        data_ = new int[size_];
        std::memcpy(data_, other.data_, size_ * sizeof(int));
        std::cout << "  [op=]    複製賦值 size=" << size_ << "\n";
        return *this;
    }

    // ----- 移動賦值 -----
    Buffer& operator=(Buffer&& other) noexcept {
        if (this == &other) return *this;
        delete[] data_;                  // 先放掉自己的
        data_ = other.data_;             // 接管對方資源
        size_ = other.size_;
        other.data_ = nullptr;           // 把對方掏空
        other.size_ = 0;
        std::cout << "  [op=&&]  移動賦值 size=" << size_ << "\n";
        return *this;
    }

    ~Buffer() {
        std::cout << "  [dtor]   釋放 " << (void*)data_ << "\n";
        delete[] data_;
    }

    size_t size() const { return size_; }
};

// 一個工廠函式：回傳 Buffer 物件 (rvalue)
Buffer makeBuffer(size_t n) {
    Buffer b(n);
    return b;     // C++17 起多半被 RVO/NRVO 直接「就地建構」省略複製/移動
}

int main() {
    std::cout << "===== (1) 複製建構 (lvalue → 走 copy) =====" << std::endl;
    Buffer a(1000000);
    Buffer b = a;             // a 是有名字的 lvalue → 走 copy ctor

    std::cout << "===== (2) 移動建構：用 std::move 把 lvalue 強轉成 rvalue =====" << std::endl;
    Buffer c = std::move(a);  // 告訴編譯器「a 你可以掏空」
    std::cout << "(此後 a.size() = " << a.size() << "，已被掏空)\n";

    std::cout << "===== (3) 從工廠函式回傳的物件本身就是 rvalue =====" << std::endl;
    Buffer d = makeBuffer(500); // 回傳值是臨時物件，理論上走 move ctor
                                // 但實務上 RVO 直接就地構造，連 move 都省了

    std::cout << "===== (4) vector 內部會自己選 move 還是 copy =====" << std::endl;
    std::vector<Buffer> vec;
    vec.reserve(2);             // 避免 push_back 中途 reallocate 干擾觀察
    Buffer e(10);
    vec.push_back(std::move(e));  // 標記成 rvalue → 容器走 move ctor

    std::cout << "===== (5) Leetcode 1480 - 用 std::move 把結果 vector 搬出去 =====" << std::endl;
    // 計算 LC 1480 的解：runningSum
    auto runningSum = [](std::vector<int> nums) {
        for (size_t i = 1; i < nums.size(); ++i) nums[i] += nums[i - 1];
        return nums;          // 注意：回傳值會自動走 move (NRVO 多半也直接省掉)
    };
    std::vector<int> input = {1, 2, 3, 4};
    std::vector<int> result = runningSum(input);   // {1, 3, 6, 10}

    // 把 result 「搬」給 saved，搬完 result 變空 — 大型資料這樣搬幾乎免成本
    std::vector<int> saved = std::move(result);
    std::cout << "saved.size = " << saved.size()
              << "  (預期 4)" << "\n";
    std::cout << "result.size = " << result.size()
              << "  (預期 0：被搬空了)" << "\n";
    std::cout << "saved 內容: ";
    for (int x : saved) std::cout << x << " ";
    std::cout << "\n";

    std::cout << "===== (6) Leetcode 1929 - move 結果陣列 =====" << std::endl;
    // 題目簡述：給 nums，回傳 [nums, nums] (連接自己一份)。
    auto buildConcat = [](std::vector<int> nums) {
        std::vector<int> ans;
        ans.reserve(nums.size() * 2);
        for (int x : nums) ans.push_back(x);
        for (int x : nums) ans.push_back(x);
        return ans;        // 回傳時編譯器會用 move/RVO，幾乎免費
    };
    std::vector<int> built = buildConcat({1, 3, 2, 1});
    std::cout << "建好的大小 = " << built.size() << " (預期 8)\n";
    std::vector<int> movedAway = std::move(built);   // O(1) 搬走
    std::cout << "搬走後 built.size = " << built.size()
              << ", movedAway.size = " << movedAway.size() << std::endl;

    std::cout << "===== (7) 日常實用：std::string 也支援 move =====" << std::endl;
    // std::string 是 RAII 物件，支援 move：搬指標而不複製字串內容
    std::string longText(50, 'A');     // 50 個 'A'
    std::cout << "原 longText 容量 = " << longText.size() << std::endl;
    std::string moved = std::move(longText);   // O(1) 搬走
    std::cout << "搬走後 longText.size = " << longText.size()
              << " (通常被掏空)，moved.size = " << moved.size() << std::endl;

    std::cout << "===== main 即將結束，看到的解構順序與位址=====" << std::endl;
    return 0;
}

/* 預期輸出（位址數值會異）：
 * ===== (1) 複製建構 (lvalue → 走 copy) =====
 *   [ctor]   配置大小 1000000 位址 0x...
 *   [copy]   深拷貝 size=1000000 ...
 * ===== (2) 移動建構：用 std::move 把 lvalue 強轉成 rvalue =====
 *   [move]   搬走指標 size=1000000 位址 0x...
 * (此後 a.size() = 0，已被掏空)
 * ===== (3) 從工廠函式回傳的物件本身就是 rvalue =====
 *   [ctor]   配置大小 500 位址 0x...
 * ===== (4) vector 內部會自己選 move 還是 copy =====
 *   [ctor]   配置大小 10 位址 0x...
 *   [move]   搬走指標 size=10 位址 0x...
 * ===== (5) Leetcode 1480 - 用 std::move 把結果 vector 搬出去 =====
 * saved.size = 4  (預期 4)
 * result.size = 0  (預期 0：被搬空了)
 * saved 內容: 1 3 6 10
 * ===== main 即將結束，看到的解構順序與位址=====
 *   [dtor]   釋放 ...
 *   [dtor]   釋放 ...    (其中包括之前被掏空、data_=0 的)
 */

/*=============================================================================
 * 【本篇重點回顧】
 *   1. Move semantics 讓「擁有大量資源」的物件可以幾乎免費搬家。
 *   2. T&&  是 rvalue reference，move ctor / move 賦值 接受它。
 *   3. std::move(x) 不會真的搬，只是把 x 的型別轉成 rvalue ref，讓 move 版本被選。
 *   4. 被 move 過的物件處於「valid-but-unspecified」狀態，
 *      除了賦值/銷毀，最好不要依賴其內容。
 *   5. 建議把自寫的 move 操作標 noexcept，這樣 std::vector 等容器才會選擇用 move。
 *
 * 【下一篇預告】
 *   23_RuleOfThreeFiveZero.cpp
 *   Rule of 3 / 5 / 0 — 何時要自己寫複製/移動/解構？何時可以全部不寫？
 *=============================================================================*/
