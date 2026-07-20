// =============================================================================
//  第 2.3 章 #1  —  移動建構子的核心機制：偷指標 + 來源歸零
// =============================================================================
//
// 【主題資訊 Information】
//   簽名：ClassName(ClassName&& other) noexcept;
//   標準：C++11 起（rvalue reference `&&` 與 move semantics 一併引入）
//   複雜度：O(1)（只搬幾個純量欄位）— 對比複製建構子的 O(n)
//   標頭檔：<utility>（std::move）、本檔另用 <algorithm>（std::copy）
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼需要移動建構子：C++11 之前的浪費】
// 考慮 `IntArray c(std::move(a));`。若只有複製建構子，唯一能做的事是：
//   ① new 一塊同樣大的記憶體 ② 逐元素複製 ③ 原物件稍後被解構、記憶體被釋放。
// 但當來源是「即將消滅的物件」（臨時物件，或使用者用 std::move 明講「我不要了」），
// ①②③ 完全是白工 —— 資料被複製一份，然後原本那份立刻被丟掉。
// 真正需要的只是「把資源的所有權從 a 轉移給 c」，而所有權在實作上就是那一根指標。
// 移動建構子做的事因此只有兩件：**把指標抄過來**、**把來源的指標設成 nullptr**。
// 資料一個位元組都沒有動，成本與陣列大小無關 —— 這就是 O(n) → O(1) 的來源。
//
// 【2. 多載是怎麼被選中的：rvalue reference 的角色】
// 類別現在有兩個候選建構子：
//   IntArray(const IntArray&)   // 吃 lvalue，也吃得下 rvalue（const& 可綁定 rvalue）
//   IntArray(IntArray&&)        // 只吃 rvalue
// 當引數是 rvalue（臨時物件、std::move 的結果）時，兩者都可行，但 `IntArray&&`
// 是更精確的比對（exact match），overload resolution 選它。當引數是 lvalue 時，
// `IntArray&&` 根本綁不上，只剩 const& 可選 —— 這就是為什麼「不加 std::move
// 就不會移動」。
//
// 關鍵觀念：**std::move 本身不移動任何東西**。它是個 static_cast<T&&>，只負責
// 把 lvalue 轉成 rvalue，讓多載決議挑中移動建構子。真正搬東西的是移動建構子本身。
//
// 【3. 為什麼「來源歸零」不是可有可無的清潔工作】
// 步驟 3（other.data_ = nullptr）**不是禮貌，是正確性的必要條件**。
// 若省略，來源與新物件的 data_ 會指向同一塊記憶體，兩者解構時各 delete[] 一次 →
// double free。歸零之後來源手上是 nullptr，而 `delete[] nullptr` 是標準保證的
// no-op，來源的解構子因此變成安全的空操作。
// 記法：移動不是「複製後刪除來源」，而是「**轉移所有權**」——所有權必須唯一，
// 所以來源一定要放手。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 記憶體層次到底發生什麼
//   移動前：
//       堆疊 a: [ data_ = 0x...b030 ][ size_ = 5 ]
//       堆積   : 0x...b030 → [10][20][30][0][0]
//   移動後：
//       堆疊 a: [ data_ = nullptr   ][ size_ = 0 ]     ← 來源放棄所有權
//       堆疊 c: [ data_ = 0x...b030 ][ size_ = 5 ]     ← 接管，位址完全相同
//       堆積   : 0x...b030 → [10][20][30][0][0]        ← 這塊記憶體從未被碰過
//   本檔的實測輸出可以直接印證：移動後 c.data() 印出的位址，與移動前 a.data()
//   的位址**一模一樣**（位址值每次執行都不同，但「兩者相等」這件事恆成立）。
//
// (B) ★ 成員的初始化順序是「宣告順序」，不是初始化列表的書寫順序
//   這是本章最容易出人命的規則。編譯器初始化成員時，**一律照類別中的宣告順序**，
//   完全忽略你在 mem-initializer-list 裡寫的先後。所以這種寫法是錯的：
//       class Bad {
//           char*  m_data;          // 先宣告
//           size_t m_len;           // 後宣告
//           Bad(const char* s) : m_len(strlen(s)), m_data(new char[m_len+1]) {}
//       };                          //  ↑你以為先算 m_len，其實 m_data 先跑，
//                                   //   此時 m_len 還沒初始化 → 用未定值配置大小
//   正確做法是**讓宣告順序與依賴順序一致**（長度宣告在前）；本章 summary.cpp
//   保留了這個修正與註解，值得對照著看。GCC 的 `-Wreorder`（含在 -Wall）會在
//   「書寫順序與宣告順序不符」時警告，但它只提醒順序不一致，**不會**幫你發現
//   「讀到未初始化成員」這個真正的後果。
//
// (C) 為什麼移動建構子應該標 noexcept
//   std::vector 擴容時要把舊元素搬到新緩衝區。若搬到一半拋例外，舊緩衝區已被
//   破壞、新的又不完整，無法回滾 → 給不出 strong exception guarantee。
//   因此 vector 內部用 std::move_if_noexcept 決定：元素的移動建構子是 noexcept
//   時才移動，否則**靜默退回複製**（慢但可回滾）。細節與實測見本章 #2 檔。
//
// 【注意事項 Pay Attention】
// 1. **被移動後的來源處於「有效但未指定 (valid but unspecified)」狀態**。
//    標準保證它仍可安全解構、可被重新賦值；但**不保證**它變成什麼特定值。
//    本檔因為是我們自己寫的類別、自己寫了 `other.data_ = nullptr`，所以我們知道
//    它是 nullptr；但對 std::string、std::vector 這類標準庫型別，
//    「移動後一定變空」**不是標準保證**（多數實作如此，不可依賴）。
//    唯一安全的操作是：重新賦值，或直接讓它解構。
// 2. 移動建構子的參數是 `IntArray&& other`，**不能加 const**。加了 const 就沒辦法
//    把 other.data_ 改成 nullptr，來源無法放手，整個機制就垮了。
// 3. 參數 other 本身是具名變數，因此在函式內是 **lvalue**。若成員是類別型別而
//    想繼續往下移動，必須再寫一次 std::move（見本章 #5 的 label_ 成員）。
// 4. 移動不是「比較快的複製」。移動之後**來源已經不再擁有資料**，語意上完全不同；
//    還要繼續使用的物件絕對不能 std::move。
// 5. `delete[] nullptr` 是標準保證的 no-op，所以解構子不需要先檢查 if (data_)。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】移動建構子的核心機制
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 被移動之後，來源物件處於什麼狀態？
//     答：標準只保證「有效但未指定 (valid but unspecified)」——可以安全解構、
//         可以重新賦值，但**不保證是任何特定值**。以本檔自寫的 IntArray 而言，
//         因為我們自己寫了 other.data_ = nullptr，所以確實會變成 nullptr；
//         但對 std::string／std::vector，「移動後一定是空的」不是標準保證。
//     追問：那 moved-from 的 std::string 呼叫 size() 是 UB 嗎？→ 不是。size() 沒有
//         前置條件，呼叫合法，只是回傳值未指定。真正 UB 的是有前置條件的操作，
//         例如對空的 string 呼叫 front()／back()。
//
// 🔥 Q2. 移動建構子裡「把來源指標設成 nullptr」可以省略嗎？
//     答：不可以，那不是清潔工作而是正確性條件。省略後兩個物件的 data_ 指向同一塊
//         記憶體，解構時各 delete[] 一次 → double free（未定義行為）。
//         移動的語意是**所有權轉移**，所有權必須唯一，來源一定要放手。
//     追問：那 size_ 需要歸零嗎？→ 不歸零不會 double free，但會讓來源處於
//         「size_=5 卻 data_=nullptr」的自相矛盾狀態；任何走訪它的程式碼都會爆。
//         歸零是為了維持 class invariant。
//
// 🔥 Q3. std::move 到底做了什麼？
//     答：**它不移動任何東西**。std::move 是一個無條件的 static_cast<T&&>，
//         唯一作用是把 lvalue 轉型成 rvalue，好讓 overload resolution 選中
//         `T(T&&)` 而不是 `T(const T&)`。真正搬資料的是移動建構子本身。
//     追問：對 const 物件 std::move 會怎樣？→ 型別是 const T&&，綁不上 T&&，
//         只能落到 const T&，於是**靜默退化成複製**。這是效能白做工的經典來源。
//
// ⚠️ 陷阱. 這段程式碼哪裡錯了？
//         class Buf {
//             char*  m_data;
//             size_t m_len;
//         public:
//             Buf(const char* s) : m_len(strlen(s)), m_data(new char[m_len+1]) {}
//         };
//     答：成員初始化**一律照宣告順序**（m_data 先、m_len 後），與初始化列表的
//         書寫順序無關。所以 `new char[m_len+1]` 執行時 m_len **尚未初始化**，
//         等於拿未定值去決定配置大小。修法：把 m_len 宣告在 m_data 前面。
//     為什麼會錯：多數人腦中把 mem-initializer-list 讀成「由左到右依序執行的
//         程式敘述」。它其實只是「哪個成員用哪個初始值」的對照表，執行順序
//         由宣告順序決定。-Wall 的 -Wreorder 只會提醒順序不一致，
//         **不會**告訴你「你讀到了未初始化的成員」這個真正的後果。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <cstring>
#include <utility>
#include <algorithm>

class IntArray {
    // 注意宣告順序：data_ 的初始化式只用到參數 size，不依賴 size_ 成員，故此處安全。
    // 若寫成 data_(new int[size_]) 就必須把 size_ 宣告在前（見檔頭 ⚠️ 陷阱）。
    int* data_;
    size_t size_;

public:
    // ========== 一般建構子 ==========
    explicit IntArray(size_t size = 0)
        : data_(size ? new int[size]() : nullptr)
        , size_(size)
    {
        std::cout << "  [建構] size=" << size_
                  << ", data_=" << data_ << "\n";
    }

    // ========== 解構子 ==========
    ~IntArray() {
        std::cout << "  [解構] size=" << size_
                  << ", data_=" << data_ << "\n";
        delete[] data_;               // data_ 為 nullptr 時是標準保證的 no-op
    }

    // ========== 複製建構子 ==========
    IntArray(const IntArray& other)
        : data_(other.size_ ? new int[other.size_] : nullptr)
        , size_(other.size_)
    {
        if (data_) {
            std::copy(other.data_, other.data_ + size_, data_);
        }
        std::cout << "  [複製建構] size=" << size_
                  << " ← 配置新記憶體 + 逐元素複製\n";
    }

    // ========== 移動建構子 ==========
    IntArray(IntArray&& other) noexcept
        : data_(other.data_)      // 步驟 1：接管指標
        , size_(other.size_)      // 步驟 2：接管大小
    {
        other.data_ = nullptr;    // 步驟 3：讓來源放棄資源（正確性必要，非清潔）
        other.size_ = 0;
        std::cout << "  [移動建構] size=" << size_
                  << " ← 只搬指標，零成本\n";
    }

    size_t size() const { return size_; }
    int* data() const { return data_; }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】影像解碼後的像素緩衝區搬移
//   情境：解碼器把 JPEG/PNG 解成 raw RGBA 像素，一張 1920x1080 就是約 8 MB。
//         decode_image() 產生 buffer 後要交給呼叫端（放進快取、丟去 GPU 上傳）。
//   為什麼用到本主題：這 8 MB 若每次交接都深複製，光是 memcpy 就吃掉整個
//         frame budget。移動讓「交接」退化成搬一根指標，成本與影像大小無關。
//   對照：decode_image() 內是回傳具名區域變數（NRVO，可省略也可能走移動）；
//         交給快取則是明確 std::move。
// -----------------------------------------------------------------------------
class PixelBuffer {
    unsigned char* pixels_;
    size_t bytes_;
    int width_;
    int height_;

public:
    PixelBuffer(int w, int h)
        : pixels_(new unsigned char[static_cast<size_t>(w) * h * 4]())
        , bytes_(static_cast<size_t>(w) * h * 4)
        , width_(w), height_(h) {}

    ~PixelBuffer() { delete[] pixels_; }

    // 深複製：真的要 memcpy 幾 MB
    PixelBuffer(const PixelBuffer& o)
        : pixels_(new unsigned char[o.bytes_])
        , bytes_(o.bytes_), width_(o.width_), height_(o.height_) {
        std::copy(o.pixels_, o.pixels_ + bytes_, pixels_);
        std::cout << "  [PixelBuffer 複製] 深複製 " << bytes_ / 1024 << " KB\n";
    }

    // 移動：只搬 4 個純量欄位
    PixelBuffer(PixelBuffer&& o) noexcept
        : pixels_(o.pixels_), bytes_(o.bytes_)
        , width_(o.width_), height_(o.height_) {
        o.pixels_ = nullptr;          // 放棄所有權
        o.bytes_  = 0;
        o.width_ = o.height_ = 0;
        std::cout << "  [PixelBuffer 移動] 只搬指標（不論影像多大都一樣快）\n";
    }

    size_t bytes() const { return bytes_; }
    const unsigned char* pixels() const { return pixels_; }
    int width() const { return width_; }
    int height() const { return height_; }
};

// 模擬解碼器：產生一張 raw 影像後回傳
PixelBuffer decode_image(int w, int h) {
    PixelBuffer buf(w, h);
    // …此處實際會呼叫 libjpeg / libpng 填滿 buf…
    return buf;                       // NRVO（可省略）；未省略時走移動建構子
}

int main() {
    std::cout << "=== 建立原始物件 ===\n";
    IntArray a(5);
    a.data()[0] = 10;
    a.data()[1] = 20;
    a.data()[2] = 30;

    std::cout << "\n=== 複製建構 ===\n";
    IntArray b(a);  // 呼叫複製建構子
    std::cout << "a.data() = " << a.data() << " (原始物件完好)\n";
    std::cout << "b.data() = " << b.data() << " (指向不同的記憶體)\n";

    std::cout << "\n=== 移動建構 ===\n";
    IntArray c(std::move(a));  // 呼叫移動建構子
    std::cout << "a.data() = " << a.data() << " (已被掏空)\n";
    std::cout << "a.size() = " << a.size() << "\n";
    std::cout << "c.data() = " << c.data() << " (接管了原本 a 的記憶體)\n";
    std::cout << "c[0]=" << c.data()[0]
              << ", c[1]=" << c.data()[1]
              << ", c[2]=" << c.data()[2] << "\n";

    // ── 實務：影像緩衝區 ──────────────────────────────────────────
    std::cout << "\n=== 日常實務：影像解碼緩衝區 ===\n";
    PixelBuffer img = decode_image(1920, 1080);
    std::cout << "解碼完成：" << img.width() << "x" << img.height()
              << "，共 " << img.bytes() / 1024 << " KB\n";

    std::cout << "\n  -- 深複製一份（例如要保留原圖做 undo）--\n";
    PixelBuffer backup(img);
    std::cout << "  backup 位址與 img 不同：" << std::boolalpha
              << (backup.pixels() != img.pixels()) << "\n";

    std::cout << "\n  -- 交給快取（不再自己用，明確移動）--\n";
    const unsigned char* before = img.pixels();
    PixelBuffer cached(std::move(img));
    std::cout << "  快取拿到的位址 == 移動前 img 的位址："
              << (cached.pixels() == before) << "\n";
    std::cout << "  img 已放棄所有權，pixels()==nullptr："
              << (img.pixels() == nullptr) << "\n";

    std::cout << "\n=== 離開 main ===\n";
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 2.3 章：移動建構子 (Move Constructor) — 實作與原理1.cpp" -o move_ctor1

// （注意：所有 0x... 位址每次執行都不同，屬正常現象；

// === 預期輸出 ===
//   要看的是「移動後 c.data() 與移動前 a.data() 相等」這個關係。）
//
// === 建立原始物件 ===
//   [建構] size=5, data_=0x5c00b70e4030
//
// === 複製建構 ===
//   [複製建構] size=5 ← 配置新記憶體 + 逐元素複製
// a.data() = 0x5c00b70e4030 (原始物件完好)
// b.data() = 0x5c00b70e4050 (指向不同的記憶體)
//
// === 移動建構 ===
//   [移動建構] size=5 ← 只搬指標，零成本
// a.data() = 0 (已被掏空)
// a.size() = 0
// c.data() = 0x5c00b70e4030 (接管了原本 a 的記憶體)
// c[0]=10, c[1]=20, c[2]=30
//
// === 日常實務：影像解碼緩衝區 ===
// 解碼完成：1920x1080，共 8100 KB
//
//   -- 深複製一份（例如要保留原圖做 undo）--
//   [PixelBuffer 複製] 深複製 8100 KB
//   backup 位址與 img 不同：true
//
//   -- 交給快取（不再自己用，明確移動）--
//   [PixelBuffer 移動] 只搬指標（不論影像多大都一樣快）
//   快取拿到的位址 == 移動前 img 的位址：true
//   img 已放棄所有權，pixels()==nullptr：true
//
// === 離開 main ===
//   [解構] size=5, data_=0x5c00b70e4030
//   [解構] size=5, data_=0x5c00b70e4050
//   [解構] size=0, data_=0
