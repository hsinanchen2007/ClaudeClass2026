// =============================================================================
//  第三課：STL 的六大組件概覽 12  —  配置器（Allocator）：配置與建構的分離
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<memory>
//   核心介面（std::allocator<T>）：
//     T*   allocate(size_t n);            // 只配置 n*sizeof(T) 的原始記憶體，不建構物件
//     void deallocate(T* p, size_t n);    // 釋放；n 必須與當初 allocate 的值相同
//   建構／解構走 allocator_traits（C++11 起的正式管道）：
//     std::allocator_traits<A>::construct(a, p, args...);   // placement new
//     std::allocator_traits<A>::destroy(a, p);              // 呼叫 ~T()
//   標準版本：
//     std::allocator             C++98
//     std::allocator_traits      C++11（統一介面，讓自訂 allocator 只需實作少數成員）
//     allocator::construct/destroy   C++17 deprecated、**C++20 移除**
//       → 必須走 allocator_traits（本檔即採用此寫法）
//     std::pmr（多型配置器）     C++17，<memory_resource>
//   複雜度：allocate/deallocate 取決於底層（預設轉呼叫 ::operator new/delete）。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼要把「配置記憶體」與「建構物件」分開】
//   new T[n] 做了兩件事：配置記憶體 + 對每個元素呼叫建構子。
//   對 std::vector 而言這是災難性的：
//       std::vector<std::string> v;
//       v.reserve(1000);        // 只想預留空間，不想建構 1000 個空字串！
//   如果 reserve 用 new T[1000]，就會白白建構（之後又解構）一千個物件。
//   allocator 把兩階段拆開：
//       T* raw = alloc.allocate(1000);          // 只要記憶體，元素尚不存在
//       // ... push_back 時才在需要的位置逐一 construct
//   這就是 capacity 與 size 能夠分離的底層機制 ——
//   capacity 是「配置了多少空間」，size 是「建構了多少物件」。
//
// 【2. construct 的底層是 placement new】
//   allocator_traits<A>::construct(a, p, args...) 預設實作大致是：
//       ::new (static_cast<void*>(p)) T(std::forward<Args>(args)...);
//   這個語法叫 placement new：**不配置記憶體**，只在 p 指定的位址上呼叫建構子。
//   對稱地，destroy 呼叫 p->~T() —— 顯式解構子呼叫，只解構不釋放記憶體。
//   四個動作必須嚴格成對：
//       allocate  ←→  deallocate     （記憶體）
//       construct ←→  destroy        （物件生命週期）
//   順序也不能錯：必須先 destroy 全部物件，才能 deallocate 記憶體。
//
// 【3. 為什麼 vector<int> 其實是 vector<int, allocator<int>>】
//   所有標準容器都有一個預設的 allocator 樣板參數：
//       template <class T, class Allocator = std::allocator<T>> class vector;
//   把它做成樣板參數（而非執行期參數）的理由是效能：
//   allocator 的型別在編譯期就確定，因此 allocate 的呼叫可以完全 inline，
//   零額外成本。代價是「不同 allocator 的 vector 是不同型別」，
//   無法互相賦值、也不能傳給同一個函式 —— 這正是 C++17 引入
//   std::pmr::vector（執行期多型 allocator）的動機。
//
// 【4. 什麼時候真的需要自訂 allocator】
//   絕大多數程式碼永遠不需要。真的有價值的場景：
//     - 記憶體池：大量小物件的頻繁配置/釋放（如遊戲的實體、網路封包）
//     - 即時系統：不能容忍 malloc 的不確定延遲，改用預先配置的固定池
//     - 共享記憶體：跨行程的容器必須用共享記憶體區段配置
//     - 對齊需求：SIMD 要求 32/64 byte 對齊
//     - 除錯：記錄每次配置的呼叫堆疊以追查記憶體洩漏
//   否則預設 allocator（背後是經過高度最佳化的 malloc）通常已經很好。
//
// 【概念補充 Concept Deep Dive】
//   allocator 是「空類別」，這對 sizeof(vector) 有直接影響。
//   std::allocator<T> 沒有任何成員變數（它只是 ::operator new 的薄包裝），
//   所以 sizeof(std::allocator<int>) 是 1。
//   但 vector 需要同時持有三個指標（begin / end / capacity_end）與一個 allocator，
//   若 allocator 老實佔 1 byte，加上對齊會讓 vector 變成 32 bytes。
//   libstdc++ 的作法是讓內部結構**繼承**自 allocator，
//   觸發空基底最佳化（Empty Base Optimization, EBO）—— 空基底不佔任何空間。
//   因此本機實測 sizeof(std::vector<int>) 仍然是 24 bytes（= 三個指標）。
//   這是 EBO 在標準函式庫中最有名的應用之一，也是為什麼
//   「無狀態的策略類別應該用繼承而非成員」在 C++ 中是常見慣例。
//
// 【注意事項 Pay Attention】
//   1. allocate 回傳的是**未初始化的原始記憶體**。直接讀取或當成物件使用
//      是未定義行為，必須先 construct。
//   2. deallocate 的第二個參數 n 必須與當初 allocate 的完全相同，否則是 UB。
//   3. 必須先對所有已建構的元素 destroy，才能 deallocate。
//      漏掉 destroy 對 int 這種平凡型別看似無事，但對 std::string 就是記憶體洩漏。
//   4. **allocator::construct / destroy 成員函式在 C++20 已被移除**，
//      一律改用 std::allocator_traits<A>::construct / destroy（本檔採用）。
//   5. 不同 allocator 型別的容器是不同型別，不能互相賦值。
//      需要執行期切換配置策略請用 C++17 的 std::pmr。
//   6. 例外安全：若 construct 到一半丟出例外，已建構的元素必須手動 destroy，
//      否則會洩漏。標準提供 std::uninitialized_copy 等函式代為處理這件事。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】配置器（Allocator）
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 STL 要把「配置記憶體」和「建構物件」分成兩個步驟？
//     答：因為容器需要「有空間但還沒有物件」這個中間狀態。
//         vector::reserve(1000) 只想要空間，不該建構一千個物件；
//         若用 new T[1000] 就會白白建構又解構。
//         allocate 只給原始記憶體，construct（placement new）在需要時才逐一建構。
//         這正是 capacity（配置了多少）與 size（建構了多少）能分離的底層原因。
//     追問：那 construct 的底層是什麼？
//           → placement new：`::new (p) T(args...)`，不配置記憶體、
//             只在指定位址呼叫建構子。對稱的 destroy 則是顯式呼叫 `p->~T()`。
//
// 🔥 Q2. sizeof(std::vector<int>) 是多少？裡面的 allocator 佔了多少空間？
//     答：本機 g++ 15.2 / libstdc++ / x86-64 實測是 24 bytes，
//         也就是三個指標（起點、終點、容量終點），allocator **一個 byte 都沒佔**。
//         原因是 std::allocator 是空類別，libstdc++ 讓內部結構繼承自它，
//         觸發空基底最佳化（EBO），空基底不佔空間。
//     追問：如果自訂一個「有狀態」的 allocator（例如持有記憶體池指標）呢？
//           → 那就佔空間了，sizeof(vector) 會變大。這也是為什麼標準函式庫
//             偏好無狀態 allocator，以及 C++17 的 pmr 改用「指向
//             memory_resource 的單一指標」來承載狀態。
//
// ⚠️ 陷阱. 用 allocator 手動管理一段 std::string 陣列，只呼叫了 deallocate
//          而忘記 destroy —— 用 int 測試時完全正常，為什麼換成 string 就洩漏？
//     答：int 是平凡可解構（trivially destructible）型別，沒有解構子要跑，
//         漏掉 destroy 剛好沒有可見後果。std::string 的解構子負責釋放它
//         內部 heap 上的字元緩衝區；沒呼叫解構子，那塊緩衝區就永遠洩漏。
//         deallocate 只釋放你當初 allocate 的那塊記憶體，
//         完全不知道物件內部還持有別的資源。
//     為什麼會錯：把 deallocate 想成 delete。
//         delete 是「解構 + 釋放」兩件事，deallocate 只有「釋放」。
//         用 int 測試通過會給人錯誤的信心 —— 這類 bug 通常要到
//         換成非平凡型別、或用 Valgrind 掃描時才會現形。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <string>
#include <memory>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】不強加。
//   理由：LeetCode 是演算法題庫，一律使用預設 allocator 且不評測記憶體配置策略；
//         沒有任何一題的解法會牽涉 allocate/construct 的手動管理
//         （真那樣寫反而會因為容易出錯而被扣分）。
//         allocator 的價值在系統程式設計 —— 記憶體池、共享記憶體、即時系統，
//         那是下面「固定大小記憶體池」實務範例展示的場景。
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 【日常實務範例】未初始化緩衝區：先配置、後逐一建構（vector 的內部作法）
//   情境：高頻交易/遊戲伺服器要處理大量短命的訊息物件。
//         每次 new/delete 都走 malloc 有不可忽視的延遲與碎片問題，
//         標準作法是「一次配置一大塊，之後在裡面逐一建構/解構」。
//   為什麼用到本主題：這正是 std::vector 內部在做的事，
//         把它手動寫一遍就能真正理解 capacity 與 size 的差別，
//         以及為什麼 destroy 與 deallocate 必須成對且順序不能錯。
//   注意：這是教學示範。實際專案請直接用 std::vector 或 std::pmr，
//         手寫這段極易在例外路徑上洩漏。
// -----------------------------------------------------------------------------
class MessageBuffer {
    using Alloc  = std::allocator<std::string>;
    using Traits = std::allocator_traits<Alloc>;

    Alloc        alloc_;
    std::string* data_     = nullptr;
    std::size_t  capacity_ = 0;   // 配置了多少空間
    std::size_t  size_     = 0;   // 實際建構了幾個物件

public:
    explicit MessageBuffer(std::size_t capacity)
        : capacity_(capacity) {
        // 只配置原始記憶體，此時一個 std::string 物件都還不存在
        data_ = Traits::allocate(alloc_, capacity_);
    }

    ~MessageBuffer() {
        // 順序關鍵：先解構全部已建構的物件，再釋放記憶體
        for (std::size_t i = 0; i < size_; ++i) {
            Traits::destroy(alloc_, data_ + i);   // 釋放每個 string 內部的 heap 緩衝區
        }
        Traits::deallocate(alloc_, data_, capacity_);   // n 必須與 allocate 時相同
    }

    // 教學用途，禁止複製以免二次釋放
    MessageBuffer(const MessageBuffer&)            = delete;
    MessageBuffer& operator=(const MessageBuffer&) = delete;

    bool emplace(const std::string& msg) {
        if (size_ >= capacity_) return false;      // 這裡不做擴充，滿了就拒絕
        Traits::construct(alloc_, data_ + size_, msg);   // placement new
        ++size_;
        return true;
    }

    std::size_t size()     const { return size_; }
    std::size_t capacity() const { return capacity_; }
    const std::string& at(std::size_t i) const { return data_[i]; }
};

int main() {
    // 所有容器都有一個預設的配置器
    // vector<int> 實際上是 vector<int, allocator<int>>

    std::vector<int> vec1;  // 使用預設配置器
    std::vector<int, std::allocator<int>> vec2;  // 明確指定（效果相同）

    vec1.push_back(1);
    vec2.push_back(2);

    std::cout << "vec1[0] = " << vec1[0] << std::endl;
    std::cout << "vec2[0] = " << vec2[0] << std::endl;

    // allocator 的基本用法
    std::allocator<int> alloc;

    // 配置記憶體（但不建構物件）
    int* ptr = alloc.allocate(5);  // 配置 5 個 int 的空間

    // 建構物件
    for (int i = 0; i < 5; ++i) {
        std::allocator_traits<std::allocator<int>>::construct(alloc, ptr + i, i * 10);
    }

    std::cout << "手動配置的陣列: ";
    for (int i = 0; i < 5; ++i) {
        std::cout << ptr[i] << " ";
    }
    std::cout << std::endl;

    // 解構物件
    for (int i = 0; i < 5; ++i) {
        std::allocator_traits<std::allocator<int>>::destroy(alloc, ptr + i);
    }

    // 釋放記憶體
    alloc.deallocate(ptr, 5);

    // capacity 與 size 的分離 —— allocator 兩階段設計的直接後果
    std::cout << "\n=== capacity vs size（配置 vs 建構）===" << std::endl;
    std::vector<std::string> v;
    std::cout << "  初始         : size=" << v.size() << " capacity=" << v.capacity() << std::endl;
    v.reserve(100);
    std::cout << "  reserve(100) : size=" << v.size() << " capacity=" << v.capacity()
              << "  ← 配置了空間，但一個 string 都還沒建構" << std::endl;
    v.push_back("hello");
    std::cout << "  push_back 後 : size=" << v.size() << " capacity=" << v.capacity()
              << "  ← 這時才真的 construct 了一個" << std::endl;

    // vector 的成長策略（實作定義，見檔尾說明）
    std::cout << "\n=== vector 的成長策略（實作定義）===" << std::endl;
    std::vector<int> grow;
    std::size_t last_cap = grow.capacity();
    std::cout << "  初始 capacity = " << last_cap << std::endl;
    for (int i = 0; i < 40; ++i) {
        grow.push_back(i);
        if (grow.capacity() != last_cap) {
            std::cout << "  size=" << grow.size() << " 時 capacity: "
                      << last_cap << " → " << grow.capacity() << std::endl;
            last_cap = grow.capacity();
        }
    }

    // allocator 是空類別 → EBO 讓 vector 維持三個指標大小
    std::cout << "\n=== EBO：allocator 不佔 vector 的空間 ===" << std::endl;
    std::cout << "  sizeof(std::allocator<int>) = " << sizeof(std::allocator<int>)
              << " byte（空類別）" << std::endl;
    std::cout << "  sizeof(int*) * 3            = " << sizeof(int*) * 3 << " bytes" << std::endl;
    std::cout << "  sizeof(std::vector<int>)    = " << sizeof(std::vector<int>)
              << " bytes ← allocator 沒有佔空間" << std::endl;

    std::cout << "\n=== 日常實務：手動管理的訊息緩衝區 ===" << std::endl;
    {
        MessageBuffer buf(4);
        std::cout << "  建立容量 " << buf.capacity() << " 的緩衝區，"
                  << "此時已建構物件數 = " << buf.size() << std::endl;

        const char* msgs[] = {"ORDER_NEW", "ORDER_FILL", "ORDER_CANCEL", "HEARTBEAT", "OVERFLOW"};
        for (const char* m : msgs) {
            bool ok = buf.emplace(m);
            std::cout << "  emplace(\"" << m << "\") → " << (ok ? "成功" : "拒絕（已滿）")
                      << "，size=" << buf.size() << std::endl;
        }

        std::cout << "  緩衝區內容: ";
        for (std::size_t i = 0; i < buf.size(); ++i) std::cout << buf.at(i) << " ";
        std::cout << std::endl;
    }   // 解構子在此執行：先 destroy 4 個 string，再 deallocate 記憶體
    std::cout << "  離開作用域 → destroy 全部物件後才 deallocate（順序不可顛倒）"
              << std::endl;

    return 0;
}

// 注意：以下數值為「實作定義」，是本機 g++ 15.2 / libstdc++ / x86-64 的實測結果：
//         - sizeof(std::allocator<int>) = 1（空類別）
//         - sizeof(std::vector<int>)    = 24（三個指標，allocator 因 EBO 不佔空間）
//         - vector 的成長倍率為 2×（libstdc++ 的選擇；MSVC 的 STL 用 1.5×，
//           標準只要求 push_back 的攤銷複雜度為 O(1)，並未規定倍率）
//       其他標準函式庫或平台可能給出不同數值。

// 編譯: g++ -std=c++17 -Wall -Wextra 第三課：STL 的六大組件概覽12.cpp -o demo12

// === 預期輸出 ===
// vec1[0] = 1
// vec2[0] = 2
// 手動配置的陣列: 0 10 20 30 40
//
// === capacity vs size（配置 vs 建構）===
//   初始         : size=0 capacity=0
//   reserve(100) : size=0 capacity=100  ← 配置了空間，但一個 string 都還沒建構
//   push_back 後 : size=1 capacity=100  ← 這時才真的 construct 了一個
//
// === vector 的成長策略（實作定義）===
//   初始 capacity = 0
//   size=1 時 capacity: 0 → 1
//   size=2 時 capacity: 1 → 2
//   size=3 時 capacity: 2 → 4
//   size=5 時 capacity: 4 → 8
//   size=9 時 capacity: 8 → 16
//   size=17 時 capacity: 16 → 32
//   size=33 時 capacity: 32 → 64
//
// === EBO：allocator 不佔 vector 的空間 ===
//   sizeof(std::allocator<int>) = 1 byte（空類別）
//   sizeof(int*) * 3            = 24 bytes
//   sizeof(std::vector<int>)    = 24 bytes ← allocator 沒有佔空間
//
// === 日常實務：手動管理的訊息緩衝區 ===
//   建立容量 4 的緩衝區，此時已建構物件數 = 0
//   emplace("ORDER_NEW") → 成功，size=1
//   emplace("ORDER_FILL") → 成功，size=2
//   emplace("ORDER_CANCEL") → 成功，size=3
//   emplace("HEARTBEAT") → 成功，size=4
//   emplace("OVERFLOW") → 拒絕（已滿），size=4
//   緩衝區內容: ORDER_NEW ORDER_FILL ORDER_CANCEL HEARTBEAT
//   離開作用域 → destroy 全部物件後才 deallocate（順序不可顛倒）
