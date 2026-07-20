// =============================================================================
//  summary.cpp（第 35 課 總結） — std::move:把「可以偷」這件事告訴編譯器
// =============================================================================
//
// 【主題資訊 Information】
//   template <class T>
//   constexpr std::remove_reference_t<T>&& move(T&& t) noexcept;   // <utility>
//
//   標準版本：C++11 引入;C++14 起為 constexpr;C++20 起標記 [[nodiscard]]。
//   標頭檔  ：<utility>
//   複雜度  ：O(1) — 它只是一次 static_cast,編譯後不產生任何指令。
//   真正的成本發生在「接收端」的 move constructor / move assignment。
//
// 【詳細解釋 Explanation】
//
// 【1. std::move 是個名字取錯的函式】
//   std::move(x) 不搬動任何位元組。它的完整實作只有一行:
//       return static_cast<std::remove_reference_t<T>&&>(t);
//   也就是「把左值 x 轉型成右值引用」。轉型之後,x 這個運算式的 value category
//   從 lvalue 變成 xvalue(將亡值),於是重載解析會挑到 T&& 版本的建構子/賦值。
//   真正把資源(heap 指標、file descriptor、buffer)搬走的,是那個被挑中的
//   move constructor。所以正確的心智模型是:
//       std::move = 「我授權你偷走我的內臟」
//       move ctor = 「好,我真的偷了」
//   如果接收端根本沒有 move ctor(或它被 const 擋住),授權就落空,退化成拷貝。
//
// 【2. 為什麼需要「授權」這個動作 — 左值不會自動被移動】
//   C++ 的核心安全規則是:具名變數(左值)在你還可能再用到它的時候,不可以被偷。
//       TrackedString a("X");
//       TrackedString b = a;              // 必須拷貝 — 因為 a 後面可能還會用
//       TrackedString c = std::move(a);   // 你簽了名,我才敢偷
//   相對地,臨時物件(prvalue,如 TrackedString("X"))本來就沒有名字、
//   下一個分號就死,編譯器不必問就能偷,所以不需要 std::move。
//   一句話總結:std::move 存在的唯一理由,是把「有名字」的東西降級成「沒名字」。
//
// 【3. 五大使用場景】
//   1) 放棄所有權：局部變數確定不再用 → 移動給別人。
//   2) 建構子 pass-by-value + move：成員初始化的現代推薦寫法(見第 4 點)。
//   3) push_back(std::move(obj))：避免容器內多一份拷貝。
//   4) 從容器提取元素：GameItem it = std::move(v.back()); v.pop_back();
//      注意順序 — 一定要先 move 出來,再 pop_back(),反過來就是存取已銷毀物件。
//   5) swap 實作：3 次移動即可交換,完全不拷貝。
//
// 【4. pass-by-value + move 為什麼是「一種寫法吃兩種情況」】
//   Hero(TrackedString name) : m_name(std::move(name)) {}
//   傳左值 → 參數 name 由拷貝建構(1 拷貝),再 move 進成員(1 移動) = 1 拷貝 + 1 移動
//   傳右值 → 參數 name 由移動建構(1 移動),再 move 進成員(1 移動) = 2 移動,零拷貝
//   比起寫兩個重載(const T& 與 T&&),這個寫法只多付「一次移動」的代價,
//   卻省下 N 個參數時的 2^N 種重載組合。移動便宜時(string/vector)這筆交易很划算;
//   但若型別的 move 和 copy 一樣貴(例如 std::array<int,1000>),就別用這招。
//
// 【5. push_back vs emplace_back】
//   push_back(obj)             → 拷貝建構一份進容器
//   push_back(std::move(obj))  → 移動建構一份進容器
//   emplace_back(args...)      → 直接在容器記憶體上「原地建構」,零拷貝零移動
//   本檔場景 3 用計數器實測這三條路徑的差異。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼 move ctor 幾乎一定要標 noexcept
//   std::vector 擴容時要把舊元素搬到新記憶體。它使用
//   std::move_if_noexcept:若元素的 move ctor 不是 noexcept,vector 會選擇
//   「拷貝」而不是「移動」。原因是強異常保證 — 搬到一半丟例外的話,舊 buffer
//   已經被掏空、新 buffer 又不完整,無法回復。拷貝則隨時可以放棄新 buffer。
//   結論:忘了寫 noexcept,你的 move ctor 在 vector 擴容時等於不存在。
//
// (B) std::move 的回傳型別為什麼要 remove_reference_t
//   因為 T 在轉發情境下可能被推導成左值引用(T = X&)。若直接寫 T&&,
//   套用 reference collapsing(X& && → X&)會得到左值引用,轉型就失效了。
//   先 remove_reference 拿到純型別 X,再加 && 才能保證得到 X&&。
//
// (C) 移動後的物件到底變成什麼
//   標準的措辭是「valid but unspecified state」——物件仍然合法(可以解構、
//   可以重新賦值、可以呼叫沒有前置條件的成員函式如 size()/empty()/clear()),
//   但你不可以假設它的「值」是什麼。libstdc++ 的 std::string 移動後實測是空的,
//   那是實作行為,不是標準保證。本檔場景 1 刻意不印出這個值,原因見該處註解。
//
// 【注意事項 Pay Attention】
//   1. ❌ 不要 move const 物件。std::move(const T) 得到 const T&&,
//      它綁不到 T&&(移動建構子的參數),只能綁到 const T&,於是靜靜地退化成拷貝。
//      危險之處在於「它會編譯成功、也會執行正確」,只是效能悄悄消失。
//      本檔最後一段用計數器把這件事實測出來。
//   2. ❌ 不要寫 return std::move(local);
//      這會把 NRVO(具名回傳值最佳化)擋掉:本來編譯器可以直接在呼叫端的位置
//      建構這個物件、一次搬移都不用;你加了 move 反而強迫它做一次移動建構。
//      正確寫法就是 return local;(C++11 起,回傳局部變數本來就會優先當右值)。
//   3. ❌ move 之後不要再讀取該物件的值。可以重新賦值、可以解構、可以 clear()。
//   4. std::move 對「沒有 move ctor 的型別」完全無效(例如含 const 成員、
//      或自己宣告了解構子導致 move 未被隱式生成的類別),同樣靜靜退化為拷貝。
//   5. 本檔場景 2 的輸出中,兩個參數的建構順序是「未指定」的:
//      C++17 只保證每個引數的求值不交錯(indeterminately sequenced),
//      不保證誰先誰後。實測本機 g++ 15.2.0 是由右至左,MSVC 常見為由左至右。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::move / 移動語意
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::move 到底移動了什麼?
//     答：什麼都沒移動。它是一個 static_cast<remove_reference_t<T>&&>,
//         只把左值的 value category 轉成右值,讓重載解析挑到 move ctor。
//         真正搬資源的是接收端的 move constructor / move assignment。
//         編譯後 std::move 本身不產生任何機器碼。
//     追問：那如果那個型別沒有 move constructor 會怎樣?
//         → 重載解析退而求其次綁到 const T&,靜靜退化成拷貝,程式照跑照對,
//           只是效能白白消失,而且編譯器不會警告。
//
// 🔥 Q2. 為什麼 move constructor 幾乎一定要加 noexcept?
//     答：vector 擴容時用 std::move_if_noexcept 決定要搬還是要拷貝。
//         move ctor 不是 noexcept 時,vector 為了維持強異常保證會選擇「拷貝」,
//         你寫的移動最佳化在最關鍵的擴容路徑上完全不會被使用。
//     追問：為什麼不 noexcept 就不能移動?
//         → 搬到一半丟例外時,舊 buffer 已被掏空、新 buffer 不完整,無法回復原狀;
//           拷貝則隨時可以直接丟棄新 buffer,原容器毫髮無傷。
//
// ⚠️ 陷阱 1. const std::string s = "x"; auto t = std::move(s); 會移動嗎?
//     答：不會。std::move(s) 的型別是 const std::string&&,
//         而 move ctor 的參數是 std::string&&,綁不上;於是綁到 const string&,
//         呼叫的是「拷貝建構子」。
//     為什麼會錯：多數人記的規則是「std::move 就會移動」,忽略了 move 只是轉型,
//         轉出來的型別還帶著 const,而移動必然要修改來源(把它掏空),
//         所以 const 物件在原理上就不可能被移動。
//
// ⚠️ 陷阱 2. return std::move(local); 是不是比 return local; 快?
//     答：相反,它比較慢。return local; 可以觸發 NRVO,直接在呼叫端的
//         記憶體上建構,零次搬移;寫成 return std::move(local); 之後
//         回傳的是引用運算式,NRVO 條件被破壞,反而強制多做一次移動建構。
//     為什麼會錯：腦中的模型是「move 一定比 copy 快,所以加了不虧」,
//         但這裡真正的競爭對手不是 copy,而是「一次都不做」的複製省略。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <vector>
#include <utility>

// ============================================================
// 追蹤拷貝/移動次數的字串類別
// ============================================================
class TrackedString {
    std::string m_data;
    static int s_copyCount;
    static int s_moveCount;
public:
    TrackedString(const char* s = "") : m_data(s) {
        std::cout << "    [建構] \"" << m_data << "\"\n";
    }

    TrackedString(const TrackedString& other) : m_data(other.m_data) {
        ++s_copyCount;
        std::cout << "    [拷貝💰] \"" << m_data << "\"\n";
    }

    TrackedString(TrackedString&& other) noexcept : m_data(std::move(other.m_data)) {
        ++s_moveCount;
        std::cout << "    [移動⚡] \"" << m_data << "\"\n";
    }

    TrackedString& operator=(const TrackedString& other) {
        m_data = other.m_data; ++s_copyCount;
        std::cout << "    [拷貝賦值] \"" << m_data << "\"\n";
        return *this;
    }

    TrackedString& operator=(TrackedString&& other) noexcept {
        m_data = std::move(other.m_data); ++s_moveCount;
        std::cout << "    [移動賦值] \"" << m_data << "\"\n";
        return *this;
    }

    const std::string& str() const { return m_data; }

    static void reset() { s_copyCount = s_moveCount = 0; }
    static void report() {
        std::cout << "    >>> 拷貝 " << s_copyCount
                  << " 次，移動 " << s_moveCount << " 次\n";
    }
};
int TrackedString::s_copyCount = 0;
int TrackedString::s_moveCount = 0;

// ============================================================
// 場景 2：建構函數 pass-by-value + move
// ============================================================
class Hero {
    TrackedString m_name;
    TrackedString m_title;
public:
    // ★ 推薦寫法：傳值 + move 進成員
    // 傳左值：拷貝到參數 → move 到成員（1 拷貝 + 1 移動）
    // 傳右值：移動到參數 → move 到成員（2 移動，零拷貝）
    Hero(TrackedString name, TrackedString title)
        : m_name(std::move(name))     // ← move 傳值參數到成員
        , m_title(std::move(title))
    {}

    void print() const {
        std::cout << "    Hero: " << m_name.str() << " - " << m_title.str() << "\n";
    }
};

// ============================================================
// 場景 4：Inventory — 從容器提取元素
// ============================================================
class GameItem {
    std::string m_name;
    int m_power;
public:
    GameItem(std::string name, int power)
        : m_name(std::move(name)), m_power(power) {}

    GameItem(const GameItem& o) : m_name(o.m_name), m_power(o.m_power) {
        std::cout << "    [GameItem 拷貝] " << m_name << "\n";
    }
    GameItem(GameItem&& o) noexcept
        : m_name(std::move(o.m_name)), m_power(o.m_power) {
        o.m_power = 0;
        std::cout << "    [GameItem 移動] " << m_name << "\n";
    }
    GameItem& operator=(GameItem o) {
        std::swap(m_name, o.m_name);
        std::swap(m_power, o.m_power);
        return *this;
    }
    ~GameItem() = default;

    const std::string& name() const { return m_name; }
    int power() const { return m_power; }
};

class Inventory {
    std::vector<GameItem> m_items;
public:
    // const T& 版本（拷貝路徑）
    void addItem(const GameItem& item) {
        std::cout << "    addItem (拷貝路徑):\n";
        m_items.push_back(item);
    }
    // T&& 版本（移動路徑）
    void addItem(GameItem&& item) {
        std::cout << "    addItem (移動路徑):\n";
        m_items.push_back(std::move(item));
    }
    // 從容器移出最後一個
    GameItem takeLastItem() {
        GameItem item = std::move(m_items.back());
        m_items.pop_back();
        return item;
    }

    void print() const {
        std::cout << "    背包 (" << m_items.size() << " 個): ";
        for (const auto& i : m_items)
            std::cout << "[" << i.name() << " +" << i.power() << "] ";
        std::cout << "\n";
    }
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1656. Design an Ordered Stream
// 題目：insert(idKey, value) 依序插入,每次回傳「從目前指標開始能連續輸出的區塊」,
//       不能連續時回傳空 vector。
// 為什麼用到本主題：這題的介面天生就是 std::move 的兩個經典位置 ——
//   (1) value 以 by-value 傳入 → 存進容器時用 std::move,避免多一次字串拷貝;
//   (2) 回傳累積好的 chunk → return 局部 vector 時「不要」寫 std::move,
//       讓 NRVO 直接在呼叫端建構(正好對應本課注意事項第 2 點)。
// 複雜度：每次 insert 均攤 O(1),整體 O(n)。
// -----------------------------------------------------------------------------
class OrderedStream {
    std::vector<std::string> m_data;   // 1-based,index 0 不用
    std::size_t m_ptr = 1;
public:
    explicit OrderedStream(int n) : m_data(static_cast<std::size_t>(n) + 1) {}

    std::vector<std::string> insert(int idKey, std::string value) {
        // (1) value 是傳值參數(已經是一份自己的副本)→ move 進容器,零額外拷貝
        m_data[static_cast<std::size_t>(idKey)] = std::move(value);

        std::vector<std::string> chunk;
        while (m_ptr < m_data.size() && !m_data[m_ptr].empty()) {
            // 容器內的字串之後不會再用到 → 授權搬走
            chunk.push_back(std::move(m_data[m_ptr]));
            ++m_ptr;
        }
        // (2) 這裡刻意寫 return chunk; 而不是 return std::move(chunk);
        return chunk;
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】log 收集器:把解析好的訊息批次搬進上傳佇列
// 情境：服務端每秒收數千筆 log,先在記憶體累積成一批,滿了就整批交給上傳執行緒。
//       交接時整個 batch 的字串內容不需要複製 —— 用 move 把 buffer 的所有權
//       整包轉移,只搬指標,不搬字元。這是 std::move 在真實系統中最常見的用途:
//       「所有權交接」,而不是「效能微調」。
// -----------------------------------------------------------------------------
class LogBatcher {
    std::vector<std::string> m_buffer;
    std::size_t m_flushSize;
    std::vector<std::vector<std::string>> m_uploaded;  // 模擬已送出的批次
public:
    explicit LogBatcher(std::size_t flushSize) : m_flushSize(flushSize) {}

    void add(std::string line) {
        m_buffer.push_back(std::move(line));   // 傳值參數 → move 進 buffer
        if (m_buffer.size() >= m_flushSize) flush();
    }

    void flush() {
        if (m_buffer.empty()) return;
        // 整個 vector 的所有權交出去:只搬 3 個指標,不動任何字元
        m_uploaded.push_back(std::move(m_buffer));
        // move 後的 m_buffer 是 valid but unspecified → 立刻 clear() 讓它回到已知狀態
        m_buffer.clear();
    }

    std::size_t batchCount()   const { return m_uploaded.size(); }
    std::size_t pendingCount() const { return m_buffer.size(); }
    std::size_t linesInBatch(std::size_t i) const { return m_uploaded[i].size(); }
};

int main() {
    // ============================================================
    // 場景 1：放棄所有權
    // ============================================================
    std::cout << "===== 場景 1：放棄所有權 =====\n";
    TrackedString::reset();
    {
        TrackedString a("Excalibur");
        std::cout << "  不用 move（拷貝）：\n";
        TrackedString b = a;                // 拷貝
        std::cout << "  用 move（移動）：\n";
        TrackedString c = std::move(a);     // 移動
        // ★ 刻意不印出 a 的內容:被移動後的物件處於「valid but unspecified state」,
        //   標準不保證它是空字串(libstdc++ 實測為空,但那是實作行為,不是保證)。
        //   把不保證的值印進教材,等於教學生依賴未定義的行為。
        std::cout << "  a 已被移動:其值為 valid but unspecified,標準不保證內容,故不印出\n";
    }
    TrackedString::report();
    std::cout << "\n";

    // ============================================================
    // 場景 2：建構函數 pass-by-value + move
    // ============================================================
    std::cout << "===== 場景 2：pass-by-value + move =====\n";
    {
        std::cout << "  傳入左值（1拷貝 + 1移動 per 參數）：\n";
        TrackedString::reset();
        TrackedString name("Arthur"), title("King");
        Hero h1(name, title);
        TrackedString::report();

        std::cout << "\n  傳入右值（2移動 per 參數，零拷貝）：\n";
        TrackedString::reset();
        Hero h2(TrackedString("Merlin"), TrackedString("Wizard"));
        TrackedString::report();

        std::cout << "\n  用 std::move 傳入（也是移動路徑）：\n";
        TrackedString::reset();
        TrackedString n("Lancelot"), t("Knight");
        Hero h3(std::move(n), std::move(t));
        TrackedString::report();
    }
    std::cout << "\n";

    // ============================================================
    // 場景 3：push_back vs emplace_back
    // ============================================================
    std::cout << "===== 場景 3：push_back vs emplace_back =====\n";
    TrackedString::reset();
    {
        std::vector<TrackedString> vec;
        vec.reserve(3);

        TrackedString s1("Alpha");
        std::cout << "  push_back 拷貝：\n";
        vec.push_back(s1);                // 拷貝

        std::cout << "  push_back 移動：\n";
        vec.push_back(std::move(s1));     // 移動

        std::cout << "  emplace_back 原地建構：\n";
        vec.emplace_back("Delta");        // 原地建構（零拷貝零移動）
    }
    TrackedString::report();
    std::cout << "\n";

    // ============================================================
    // 場景 4：從容器提取元素
    // ============================================================
    std::cout << "===== 場景 4：從容器提取元素 =====\n";
    {
        Inventory bag;
        GameItem sword("Fire Sword", 50);
        bag.addItem(sword);                          // 拷貝加入
        bag.addItem(GameItem("Ice Shield", 30));     // 移動加入
        bag.addItem(GameItem("Thunder Staff", 70));  // 移動加入
        bag.print();

        std::cout << "\n  取出最後一個：\n";
        GameItem taken = bag.takeLastItem();  // move(back()) + pop_back()
        std::cout << "    取出：" << taken.name() << " +" << taken.power() << "\n";
        bag.print();
    }
    std::cout << "\n";

    // ============================================================
    // 場景 5：swap（3 次移動，0 次拷貝）
    // ============================================================
    std::cout << "===== 場景 5：swap =====\n";
    TrackedString::reset();
    {
        TrackedString x("Left"), y("Right");
        std::cout << "  swap 前：x=\"" << x.str() << "\" y=\"" << y.str() << "\"\n";
        // std::swap 內部：
        //   T temp = move(x);  ① x → temp
        //   x = move(y);       ② y → x
        //   y = move(temp);    ③ temp → y
        std::swap(x, y);
        std::cout << "  swap 後：x=\"" << x.str() << "\" y=\"" << y.str() << "\"\n";
    }
    TrackedString::report();
    std::cout << "\n";

    // ============================================================
    // 注意事項：const move 退化為拷貝
    // ============================================================
    std::cout << "===== 注意：const move 退化為拷貝 =====\n";
    TrackedString::reset();
    {
        const TrackedString constStr("Immovable");
        TrackedString copied = std::move(constStr);  // const → 無法移動 → 拷貝！
    }
    TrackedString::report();
    std::cout << "\n";

    // ============================================================
    // LeetCode 1656. Design an Ordered Stream
    // ============================================================
    std::cout << "=== LeetCode 1656. Design an Ordered Stream ===\n";
    {
        OrderedStream os(5);
        const int    keys[] = {3, 1, 2, 5, 4};
        const char*  vals[] = {"ccccc", "aaaaa", "bbbbb", "eeeee", "ddddd"};
        for (int i = 0; i < 5; ++i) {
            auto chunk = os.insert(keys[i], vals[i]);
            std::cout << "  insert(" << keys[i] << ", " << vals[i] << ") -> [";
            for (std::size_t j = 0; j < chunk.size(); ++j)
                std::cout << (j ? " " : "") << chunk[j];
            std::cout << "]\n";
        }
    }
    std::cout << "\n";

    // ============================================================
    // 日常實務：log 批次上傳
    // ============================================================
    std::cout << "=== 日常實務：LogBatcher 所有權交接 ===\n";
    {
        LogBatcher batcher(3);
        for (int i = 1; i <= 7; ++i)
            batcher.add("2026-07-19 10:00:0" + std::to_string(i) + " [INFO] request handled");
        std::cout << "  已送出批次數：" << batcher.batchCount() << "\n";
        for (std::size_t i = 0; i < batcher.batchCount(); ++i)
            std::cout << "    批次 " << i << " 筆數：" << batcher.linesInBatch(i) << "\n";
        std::cout << "  尚未送出（pending）：" << batcher.pendingCount() << "\n";
        batcher.flush();
        std::cout << "  flush 後批次數：" << batcher.batchCount()
                  << "，pending：" << batcher.pendingCount() << "\n";
    }
    std::cout << "\n";

    // ============================================================
    // 重點整理
    // ============================================================
    std::cout << "=== 重點整理 ===\n";
    std::cout << "  std::move 只是 static_cast<T&&>，不做實際移動\n";
    std::cout << "  五大場景：放棄所有權、ctor 傳值+move、push_back、容器提取、swap\n";
    std::cout << "  push_back(obj) 拷貝 / push_back(move(obj)) 移動 / emplace_back 原地建構\n";
    std::cout << "  ❌ 不要 move const 物件（退化為拷貝）\n";
    std::cout << "  ❌ 不要 return move(local)（阻止 NRVO）\n";
    std::cout << "  ❌ move 後不要再使用物件的值\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra summary.cpp -o summary

// ─────────────────────────────────────────────────────────────────────────────
// 【輸出但書 — 讀之前務必先看】
//  1. 場景 2 中「兩個參數誰先建構」是 未指定行為(unspecified)。C++17 只保證
//     各引數的求值不互相交錯,不保證先後順序。本機 g++ 15.2.0 實測為由右至左
//     (先 King 後 Arthur);MSVC 常見為由左至右,輸出行序會不同,拷貝/移動的
//     「次數」則一致。
//  2. 場景 4 沒有 reserve(),vector 擴容時會把既有元素整批移動,因此第 2、3 次
//     addItem 會多出「移動 Fire Sword / Ice Shield」的行。擴容倍率是
//     實作定義的 —— libstdc++ 為 2 倍(1→2→4)。換 MSVC(1.5 倍)行數會不同。
//  3. 場景 1 刻意不印出被移動後的物件內容:那是 valid but unspecified state,
//     不是可以寫進教材的固定值。
//  4. 以下輸出為本機 g++ 15.2.0 (Ubuntu 26.04) 連續執行 5 次、內容完全相同的結果。
//     唯一的差異:程式實際輸出中「背包 (...): [...] 」等行的尾端帶有空白字元
//     (print() 每印一個物品就補一個空白),下方為避免行尾空白已將其去除。
// ─────────────────────────────────────────────────────────────────────────────

// === 預期輸出 ===
// ===== 場景 1：放棄所有權 =====
//     [建構] "Excalibur"
//   不用 move（拷貝）：
//     [拷貝💰] "Excalibur"
//   用 move（移動）：
//     [移動⚡] "Excalibur"
//   a 已被移動:其值為 valid but unspecified,標準不保證內容,故不印出
//     >>> 拷貝 1 次，移動 1 次
//
// ===== 場景 2：pass-by-value + move =====
//   傳入左值（1拷貝 + 1移動 per 參數）：
//     [建構] "Arthur"
//     [建構] "King"
//     [拷貝💰] "King"
//     [拷貝💰] "Arthur"
//     [移動⚡] "Arthur"
//     [移動⚡] "King"
//     >>> 拷貝 2 次，移動 2 次
//
//   傳入右值（2移動 per 參數，零拷貝）：
//     [建構] "Wizard"
//     [建構] "Merlin"
//     [移動⚡] "Merlin"
//     [移動⚡] "Wizard"
//     >>> 拷貝 0 次，移動 2 次
//
//   用 std::move 傳入（也是移動路徑）：
//     [建構] "Lancelot"
//     [建構] "Knight"
//     [移動⚡] "Knight"
//     [移動⚡] "Lancelot"
//     [移動⚡] "Lancelot"
//     [移動⚡] "Knight"
//     >>> 拷貝 0 次，移動 4 次
//
// ===== 場景 3：push_back vs emplace_back =====
//     [建構] "Alpha"
//   push_back 拷貝：
//     [拷貝💰] "Alpha"
//   push_back 移動：
//     [移動⚡] "Alpha"
//   emplace_back 原地建構：
//     [建構] "Delta"
//     >>> 拷貝 1 次，移動 1 次
//
// ===== 場景 4：從容器提取元素 =====
//     addItem (拷貝路徑):
//     [GameItem 拷貝] Fire Sword
//     addItem (移動路徑):
//     [GameItem 移動] Ice Shield
//     [GameItem 移動] Fire Sword
//     addItem (移動路徑):
//     [GameItem 移動] Thunder Staff
//     [GameItem 移動] Fire Sword
//     [GameItem 移動] Ice Shield
//     背包 (3 個): [Fire Sword +50] [Ice Shield +30] [Thunder Staff +70]
//
//   取出最後一個：
//     [GameItem 移動] Thunder Staff
//     取出：Thunder Staff +70
//     背包 (2 個): [Fire Sword +50] [Ice Shield +30]
//
// ===== 場景 5：swap =====
//     [建構] "Left"
//     [建構] "Right"
//   swap 前：x="Left" y="Right"
//     [移動⚡] "Left"
//     [移動賦值] "Right"
//     [移動賦值] "Left"
//   swap 後：x="Right" y="Left"
//     >>> 拷貝 0 次，移動 3 次
//
// ===== 注意：const move 退化為拷貝 =====
//     [建構] "Immovable"
//     [拷貝💰] "Immovable"
//     >>> 拷貝 1 次，移動 0 次
//
// === LeetCode 1656. Design an Ordered Stream ===
//   insert(3, ccccc) -> []
//   insert(1, aaaaa) -> [aaaaa]
//   insert(2, bbbbb) -> [bbbbb ccccc]
//   insert(5, eeeee) -> []
//   insert(4, ddddd) -> [ddddd eeeee]
//
// === 日常實務：LogBatcher 所有權交接 ===
//   已送出批次數：2
//     批次 0 筆數：3
//     批次 1 筆數：3
//   尚未送出（pending）：1
//   flush 後批次數：3，pending：0
//
// === 重點整理 ===
//   std::move 只是 static_cast<T&&>，不做實際移動
//   五大場景：放棄所有權、ctor 傳值+move、push_back、容器提取、swap
//   push_back(obj) 拷貝 / push_back(move(obj)) 移動 / emplace_back 原地建構
//   ❌ 不要 move const 物件（退化為拷貝）
//   ❌ 不要 return move(local)（阻止 NRVO）
//   ❌ move 後不要再使用物件的值
