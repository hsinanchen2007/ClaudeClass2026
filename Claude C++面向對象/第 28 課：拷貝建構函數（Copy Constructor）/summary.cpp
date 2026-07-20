// =============================================================================
//  第 28 課 總結  —  拷貝建構函數（Copy Constructor）
// =============================================================================
//
// 【主題資訊 Information】
//   簽名     ： ClassName(const ClassName& other);
//   為什麼是 const&：
//       * 用 & —— 若傳值，為了建構參數又要呼叫一次拷貝建構函數 → 無限遞迴
//       * 用 const —— 才能接住暫時物件，也才能拷貝 const 物件
//   標準版本 ： C++98
//   標頭檔   ： <cstring>（strlen/strcpy）、<algorithm>（std::copy）
//   複雜度   ： 深拷貝 O(n)（配置 + 複製）
//   對照     ： 拷貝賦值運算子 operator=（見第 29 課）；移動語義（第 32 課起）
//
// 【詳細解釋 Explanation】
//
// 【1. 六大觸發場景】
//   拷貝建構函數在「用既有物件初始化一個尚不存在的物件」時被呼叫：
//       1. 直接初始化：  MyClass b(a);
//       2. 拷貝初始化：  MyClass b = a;          ← 是初始化，不是賦值
//       3. 函數傳值參數：void func(MyClass obj)  ← 傳入時拷貝
//       4. 函數回傳值：  return obj;             ← 可能被 NRVO 省略
//       5. 陣列初始化：  MyClass arr[] = {a, b};
//       6. 容器操作：    vector.push_back(obj);
//   本檔逐一示範前四個場景，並用 IntArray 補上「含指標陣列的深拷貝」。
//
// 【2. 初始化 vs 賦值：= 這個符號的兩種身分】
//   這是最容易混淆的地方，判準是「左邊的物件存不存在」：
//       SimpleString c = a;   // c 尚不存在 → 初始化 → 拷貝建構函數
//       e = a;                // e 已經存在 → 賦值   → 拷貝賦值運算子
//   同一個 = 符號，兩件不同的事。輸出中一個印 [拷貝建構]、一個印 [拷貝賦值]。
//   差別不只名稱：拷貝賦值多了「先處理掉自己原有的資源」，
//   因此需要自我賦值防護；拷貝建構的物件還是空的，不需要。
//
// 【3. 按值傳參的隱藏成本（場景 3 vs const&）】
//   passByValue(a) 為了建構參數完整深拷貝一次，函式結束再解構那份副本。
//   對照 passByConstRef 的 const& —— 完全沒有拷貝。
//   守則：只讀不改的參數一律用 const&。
//
// 【4. Copy Elision / RVO / NRVO（場景 4）】
//   createString() 回傳具名區域變數，理論上要拷貝建構到 d，
//   但本機輸出只看到一次 [一般建構]，沒有 [拷貝建構] —— 這是 NRVO。
//   關鍵區分（C++17 劃下的線）：
//       * 純右值（prvalue）初始化的省略 → C++17 起「保證」，不可關閉。
//         例如 return MyClass("hi"); 直接就地建構在呼叫端。
//       * NRVO（回傳具名區域變數）→ 永遠是「可選」的，不保證。
//   -fno-elide-constructors 只關得掉可選的那種（NRVO）。
//   詳細的四組實測對照見第 28 課 -2。
//   ★ 因為省略可能發生，不要在拷貝建構函數裡放「一定要發生」的副作用。
//
// 【5. 含指標的類別必須深拷貝（IntArray）】
//   IntArray 持有 int* m_data。編譯器預設的拷貝是淺拷貝（只複製位址），
//   會導致共享修改與 double free（見第 27 課）。
//   自訂的深拷貝用 std::copy 把整段資料複製到新配置的記憶體。
//   本檔改 arr2[0] 而 arr1 不受影響，正是深拷貝成功的證據。
//
// 【概念補充 Concept Deep Dive】
//
// (A) ★ 成員宣告順序必須與初始化列表一致（-Wreorder）
//   成員的初始化順序「只」由宣告順序決定，與初始化列表的書寫順序無關。
//   本檔原始版本的 IntArray 宣告是 int* m_data; 在前、std::size_t m_size; 在後，
//   但所有建構函數的初始化列表都寫 : m_size(...), m_data(...)（size 在前），
//   兩者不一致 → 編譯時產生 -Wreorder 警告（本機實測共報 6 次）。
//   目前還沒出事，是因為配置長度用的是參數 size 或 other.m_size，
//   不是自己還沒初始化的 m_size。
//   但只要有人改成 new int[m_size]，就會用未初始化的長度配置記憶體，
//   接著 std::copy 寫出界 —— 典型的 heap buffer overflow。
//   本檔已把 IntArray 的宣告順序改為 m_size 在前、m_data 在後，警告全消。
//   （SimpleString 的宣告順序原本就正確，維持不動。）
//
// (B) ★ 本檔為什麼不印記憶體位址
//   原始版本的 SimpleString 拷貝建構印 static_cast<void*>(m_data)，
//   想證明「新舊兩塊記憶體不同」。但位址受 ASLR 影響、每次執行都不同
//   （本機實測連跑會得到不同位址），無法寫成可對照的預期輸出。
//   本檔改用「buf#N」緩衝區編號（每次真正配置記憶體時遞增），
//   可讀又可重現：場景 1、2、3 各自產生新編號，證明確實發生了深拷貝。
//
// (C) 為什麼拷貝建構函數的參數不能傳值
//   若寫成 SimpleString(SimpleString other)，
//   為了建構參數 other 又要呼叫一次拷貝建構函數，那次又要建構它的參數……
//   無限遞迴。所以標準規定參數必須是參考；加 const 讓它能接暫時物件與 const 物件。
//
// (D) 這兩個類別都只有三法則
//   SimpleString 與 IntArray 都寫了解構、拷貝建構、拷貝賦值，符合三法則，
//   但沒有移動操作，而且因為宣告了解構函數，編譯器也不會自動生成移動
//   （見第 32 課 -3）。所以它們的所有「移動」都會退回深拷貝。
//   補上兩個移動操作就是第 34 課的五法則；更根本的做法是 Rule of Zero
//   （把 char*／int* 換成 std::string／std::vector，什麼都不用寫）。
//
// 【注意事項 Pay Attention】
//   1. = 有兩種身分：初始化（拷貝建構）與賦值（拷貝賦值），看左邊物件存不存在。
//   2. 按值傳參會完整深拷貝一次；只讀不改的參數請用 const&。
//   3. 場景 4 看不到拷貝是 NRVO 的結果，而 NRVO 是「可選」的最佳化；
//      C++17 只保證純右值初始化的省略。不要依賴拷貝建構函數裡的副作用。
//   4. 成員宣告順序決定初始化順序，務必與初始化列表一致（用 -Wreorder 檢查）。
//   5. 教材輸出不要印原始位址（ASLR 每次不同）——見【概念補充 B】。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】拷貝建構函數
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 拷貝建構函數的六大觸發場景有哪些？哪一個最常被忽略？
//     答：直接初始化、拷貝初始化、按值傳參、按值回傳、陣列初始化、容器操作
//         （如 push_back）。最常被忽略的是「按值傳參」——
//         每次呼叫都完整深拷貝一次，是效能問題的常見來源。
//         對策是只讀不改的參數用 const&。
//     追問：MyClass b = a; 是拷貝建構還是拷貝賦值？
//         → 拷貝建構。b 是新物件，這行是「初始化」；
//           只有當左邊物件已經存在（e = a）才是拷貝賦值。
//
// 🔥 Q2. C++17 對 copy elision 做了什麼保證？NRVO 保證嗎？
//     答：C++17 把「純右值初始化」的省略變成強制保證（return MyClass("x") 這種），
//         不可用旗標關閉。而 NRVO（回傳具名區域變數）在 C++17 之後
//         仍然只是「允許」，不保證 —— -fno-elide-constructors 就能關掉它。
//     追問：那能不能在拷貝建構函數裡放計數器或 log？
//         → 不建議。拷貝省略是標準允許「改變可觀察行為」的例外，
//           省略發生時那些副作用完全不會執行。
//           要計數應在別處做，不要依賴拷貝建構函數一定被呼叫。
//
// ⚠️ 陷阱. 「拷貝建構函數的參數，寫成傳值和傳 const& 效果一樣，只是風格差異。」
//     答：完全不一樣。寫成傳值會無限遞迴 ——
//         為了把實參拷貝進值參數，需要呼叫拷貝建構函數，
//         而那次呼叫又要拷貝它自己的參數…… 永遠停不下來。
//         所以標準「規定」拷貝建構函數的參數必須是參考，這不是風格選擇。
//     為什麼會錯：把拷貝建構函數當成一般函式看待。
//         一般函式傳值只是多一次拷貝；但拷貝建構函數「本身」
//         就是「執行拷貝的那個函式」，讓它傳值等於要它呼叫自己來準備自己的參數。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】—— 本檔從缺，理由如下
//   拷貝建構函數的觸發時機與 copy elision 屬於語言機制與物件生命週期，
//   LeetCode 只驗證演算法的輸入輸出與時間限制，
//   評測系統既不會拷貝你的物件，也無從觀察建構函數被呼叫幾次。
//   硬掛題號只會誤導，故從缺；本檔以四個場景 + IntArray 深拷貝完整覆蓋主題。
//
// -----------------------------------------------------------------------------

#include <iostream>
#include <cstring>
#include <algorithm>  // std::copy

namespace {
// 緩衝區編號產生器：每次「真正配置一塊新記憶體」時遞增（取代原始位址）。
int g_nextBufId = 0;
}  // namespace

// ============================================================
// 追蹤拷貝行為的類別
// ============================================================
class SimpleString {
    // ⚠️ 成員的初始化順序由「宣告順序」決定，與初始化列表的書寫順序無關。
    //    m_data 的初始值用到了 m_len，所以 m_len 必須宣告在前。
    std::size_t m_len;
    char* m_data;
    int m_bufId;
public:
    // 一般建構
    SimpleString(const char* str = "")
        : m_len(std::strlen(str)), m_data(new char[m_len + 1]), m_bufId(++g_nextBufId) {
        std::strcpy(m_data, str);
        std::cout << "  [一般建構] \"" << m_data << "\" 配置 buf#" << m_bufId << "\n";
    }

    // ★ 拷貝建構函數（深拷貝）
    SimpleString(const SimpleString& other)
        : m_len(other.m_len), m_data(new char[m_len + 1]), m_bufId(++g_nextBufId) {
        std::strcpy(m_data, other.m_data);
        std::cout << "  [拷貝建構] \"" << m_data
                  << "\" 從 buf#" << other.m_bufId
                  << " 複製到「新的」buf#" << m_bufId << "\n";
    }

    // 拷貝賦值
    SimpleString& operator=(const SimpleString& other) {
        if (this == &other) return *this;
        delete[] m_data;
        m_len = other.m_len;
        m_data = new char[m_len + 1];
        m_bufId = ++g_nextBufId;
        std::strcpy(m_data, other.m_data);
        std::cout << "  [拷貝賦值] \"" << m_data << "\"（配置 buf#" << m_bufId << "）\n";
        return *this;
    }

    // 解構
    ~SimpleString() {
        std::cout << "  [解構] \"" << (m_data ? m_data : "nullptr")
                  << "\" 釋放 buf#" << m_bufId << "\n";
        delete[] m_data;
    }

    const char* c_str() const { return m_data; }
};

// 場景 3：按值傳參 → 觸發拷貝建構
void passByValue(SimpleString s) {
    std::cout << "    函數內：s = \"" << s.c_str() << "\"\n";
} // s 離開作用域被解構

// 場景 4：按 const 引用傳參 → 不觸發拷貝！
void passByConstRef(const SimpleString& s) {
    std::cout << "    函數內：s = \"" << s.c_str() << "\"\n";
}

// 場景 4（續）：按值回傳 → 可能觸發拷貝（通常被 NRVO 省略）
SimpleString createString() {
    SimpleString local("Created Inside");
    return local;  // NRVO 可能省略拷貝（可選，非保證）
}

// ============================================================
// 含指標陣列的深拷貝示範
// ============================================================
class IntArray {
    // ⚠️ 宣告順序改為 m_size 在前、m_data 在後，與所有初始化列表一致，
    //    避免 -Wreorder（原始版本相反，本機實測報 6 次警告，見檔頭【概念補充 A】）。
    std::size_t m_size;
    int* m_data;
public:
    explicit IntArray(std::size_t size)
        : m_size(size), m_data(new int[size]{}) {
        std::cout << "  [IntArray 建構] size=" << m_size << "\n";
    }

    // 深拷貝建構：用 std::copy 複製陣列
    IntArray(const IntArray& other)
        : m_size(other.m_size), m_data(new int[other.m_size]) {
        std::copy(other.m_data, other.m_data + m_size, m_data);
        std::cout << "  [IntArray 拷貝建構] size=" << m_size << "\n";
    }

    // 拷貝賦值
    IntArray& operator=(const IntArray& other) {
        if (this == &other) return *this;
        delete[] m_data;
        m_size = other.m_size;
        m_data = new int[m_size];
        std::copy(other.m_data, other.m_data + m_size, m_data);
        return *this;
    }

    ~IntArray() { delete[] m_data; }

    int& operator[](std::size_t i) { return m_data[i]; }
    const int& operator[](std::size_t i) const { return m_data[i]; }
    std::size_t size() const { return m_size; }

    void print(const char* label) const {
        std::cout << "  " << label << ": [";
        for (std::size_t i = 0; i < m_size; ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << m_data[i];
        }
        std::cout << "]\n";
    }
};

int main() {
    // ============================================================
    // 場景 1 & 2：直接初始化 / 拷貝初始化
    // ============================================================
    std::cout << "===== 場景 1 & 2：直接初始化 / 拷貝初始化 =====\n";
    SimpleString a("Alpha");
    SimpleString b(a);       // 場景 1：直接初始化 → 拷貝建構
    SimpleString c = a;      // 場景 2：拷貝初始化 → 也是拷貝建構（不是賦值！）
    std::cout << "\n";

    // ============================================================
    // 場景 3：按值傳參
    // ============================================================
    std::cout << "===== 場景 3：按值傳參（觸發拷貝）=====\n";
    passByValue(a);  // 拷貝 a → 函數內的 s → 函數結束時 s 被解構
    std::cout << "\n";

    // 對比：按 const 引用 → 不拷貝
    std::cout << "===== 對比：按 const 引用（不拷貝）=====\n";
    passByConstRef(a);
    std::cout << "\n";

    // ============================================================
    // 場景 4：按值回傳（NRVO 可能省略拷貝）
    // ============================================================
    std::cout << "===== 場景 4：按值回傳（NRVO）=====\n";
    SimpleString d = createString();
    std::cout << "  d = \"" << d.c_str() << "\"\n";
    std::cout << "  （只看到一次建構 → NRVO 省略了拷貝）\n\n";

    // ============================================================
    // 拷貝建構 vs 拷貝賦值的區別
    // ============================================================
    std::cout << "===== 拷貝建構 vs 拷貝賦值 =====\n";
    SimpleString e("Echo");
    std::cout << "  e = a（e 已存在 → 拷貝賦值，不是拷貝建構）:\n";
    e = a;  // 拷貝「賦值」— 因為 e 已經存在
    std::cout << "\n";

    // ============================================================
    // IntArray 深拷貝示範
    // ============================================================
    std::cout << "===== IntArray 深拷貝 =====\n";
    IntArray arr1(5);
    for (std::size_t i = 0; i < arr1.size(); ++i)
        arr1[i] = static_cast<int>((i + 1) * 10);
    arr1.print("arr1");

    IntArray arr2 = arr1;  // 深拷貝建構
    arr2[0] = 999;         // 修改 arr2
    arr1.print("arr1");    // arr1 不受影響
    arr2.print("arr2");    // 只有 arr2 改變
    std::cout << "\n";

    // ============================================================
    // Copy Elision 說明
    // ============================================================
    std::cout << "===== Copy Elision / RVO / NRVO =====\n";
    std::cout << "  RVO（純右值初始化）：return MyClass(\"x\");  C++17 起保證省略\n";
    std::cout << "  NRVO（具名物件）：MyClass m(\"x\"); return m;  可選，不保證\n";
    std::cout << "  C++17 只保證純右值那種；NRVO 加 -fno-elide-constructors 可關掉\n";
    std::cout << "  拷貝省略會改變可觀察行為 → 別在拷貝建構函數裡放必要副作用\n";

    std::cout << "\n===== 開始解構 =====\n";
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -Wreorder summary.cpp -o summary

// ── 輸出說明（讀之前先看這裡）────────────────────────────────────────────
// * 本檔輸出完全決定性：本機實測連跑 5 次逐位元組相同。
//   原始版本的 SimpleString 會印記憶體位址，那受 ASLR 影響、每次不同，
//   無法當作預期輸出；本檔改用「buf#N」緩衝區編號，可讀又可重現。
// * 場景 1、2、3 各出現一個「新的 buf#」，代表都發生了深拷貝；
//   場景 4（NRVO）只看到一次 [一般建構]，沒有 [拷貝建構]。
// * NRVO 是「可選」的最佳化，C++17 只保證純右值初始化的省略。
//   其他編譯器在場景 4 多印一次 [拷貝建構] 也是合法的。
// * IntArray 深拷貝以 arr2[0]=999 而 arr1 不受影響來證明獨立性，
//   不印任何位址。
// * 本檔已修正原始版本 IntArray 的 6 個 -Wreorder 警告（成員宣告順序），
//   修正前後執行結果相同（那是尚未觸發的潛在缺陷，見檔頭【概念補充 A】）。
// * 結尾 [解構] 順序為區域物件宣告的反向：arr2、arr1、e、d、c、b、a
//   （IntArray 的解構不印訊息，故只看到 SimpleString 的五行）。
// ─────────────────────────────────────────────────────────────────────────

// === 預期輸出 ===
// ===== 場景 1 & 2：直接初始化 / 拷貝初始化 =====
//   [一般建構] "Alpha" 配置 buf#1
//   [拷貝建構] "Alpha" 從 buf#1 複製到「新的」buf#2
//   [拷貝建構] "Alpha" 從 buf#1 複製到「新的」buf#3
//
// ===== 場景 3：按值傳參（觸發拷貝）=====
//   [拷貝建構] "Alpha" 從 buf#1 複製到「新的」buf#4
//     函數內：s = "Alpha"
//   [解構] "Alpha" 釋放 buf#4
//
// ===== 對比：按 const 引用（不拷貝）=====
//     函數內：s = "Alpha"
//
// ===== 場景 4：按值回傳（NRVO）=====
//   [一般建構] "Created Inside" 配置 buf#5
//   d = "Created Inside"
//   （只看到一次建構 → NRVO 省略了拷貝）
//
// ===== 拷貝建構 vs 拷貝賦值 =====
//   [一般建構] "Echo" 配置 buf#6
//   e = a（e 已存在 → 拷貝賦值，不是拷貝建構）:
//   [拷貝賦值] "Alpha"（配置 buf#7）
//
// ===== IntArray 深拷貝 =====
//   [IntArray 建構] size=5
//   arr1: [10, 20, 30, 40, 50]
//   [IntArray 拷貝建構] size=5
//   arr1: [10, 20, 30, 40, 50]
//   arr2: [999, 20, 30, 40, 50]
//
// ===== Copy Elision / RVO / NRVO =====
//   RVO（純右值初始化）：return MyClass("x");  C++17 起保證省略
//   NRVO（具名物件）：MyClass m("x"); return m;  可選，不保證
//   C++17 只保證純右值那種；NRVO 加 -fno-elide-constructors 可關掉
//   拷貝省略會改變可觀察行為 → 別在拷貝建構函數裡放必要副作用
//
// ===== 開始解構 =====
//   [解構] "Alpha" 釋放 buf#7
//   [解構] "Created Inside" 釋放 buf#5
//   [解構] "Alpha" 釋放 buf#3
//   [解構] "Alpha" 釋放 buf#2
//   [解構] "Alpha" 釋放 buf#1
