// =============================================================================
//  第 31 課 summary.cpp  —  右值引用（Rvalue Reference）
// =============================================================================
//
// 【主題資訊 Information】
//   語法      ： T&&  右值引用（C++11 新增）
//   綁定規則  ： T&        只綁左值
//                const T&  左值、右值都能綁（唯讀萬用）
//                T&&       只綁右值
//   標準版本  ： C++11 起；值類別（value category）在 C++17 被重新整理為
//                lvalue / xvalue / prvalue（glvalue = lvalue + xvalue，
//                rvalue = xvalue + prvalue）
//   標頭檔    ： <utility>（std::move）、<type_traits>（本檔用來「證明」值類別）
//   複雜度    ： 綁定本身 O(1)；它的價值在於讓「移動」取代「複製」
//   關鍵心法  ： 型別（type）和值類別（value category）是兩件不同的事
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼 C++11 要多發明一種引用】
//   C++98 只有 T& 和 const T&，於是函式無法區分兩件事：
//       f(name);                  // 呼叫端之後還要用 name → 必須複製
//       f(name + " King");        // 暫時物件，馬上就死 → 複製是白費工
//   兩者都只能綁到 const T&，函式看不出差別，只好一律複製。
//   T&& 的唯一任務就是把「這是個馬上就要消失的暫時物件」這個資訊帶進函式，
//   讓函式敢於「偷走」它的資源而不是複製。
//   所以右值引用不是「另一種引用」，而是一個重載解析用的標記。
//
// 【2. 左值與右值：用「有沒有名字」判斷九成夠用】
//   * 左值（lvalue）：有名字、指得到它是誰。x、arr[0]、*p、++i、
//     以及「回傳 T& 的函式呼叫」。
//   * 右值（rvalue）：暫時的、沒有名字。42、x + 1、f() 的傳值回傳、
//     std::string("tmp")、以及 std::move(x) 的結果。
//   進階一點的正確說法是 C++17 的三分法：
//     prvalue（純右值）：42、x+1、傳值回傳 —— 還沒有身分的「值」
//     xvalue （將亡值）：std::move(x)、回傳 T&& 的函式 —— 有身分但可被搬走
//     lvalue （左值）  ：有身分、不可被搬走
//
// 【3. 本課最重要的一句話：右值引用變數本身是左值】
//       void f(int&& val) {  /* 在這裡 val 是左值！ */ }
//       int&& r = 42;        // r 是左值
//   為什麼？因為 val 和 r「有名字」。有名字就代表你可以重複使用它，
//   編譯器不敢讓下一個函式把它搬空 —— 否則你下一行再用到它就完蛋了。
//   所以要繼續往下傳遞「可以搬走」這個許可，必須再寫一次 std::move。
//
//   本檔不用「可以取位址」來示範這件事（印位址每次執行都不同、無法重現），
//   而是用 decltype 直接證明，這也更精確：
//       decltype(r)    → int&&   （r 的「宣告型別」）
//       decltype((r))  → int&    （運算式 r 的「值類別」是左值）
//   多一組括號，問的問題就從「你被宣告成什麼」變成「你作為運算式是什麼」。
//   本機實測兩者分別為 is_rvalue_reference = true 與 is_lvalue_reference = true。
//
// 【4. const T& 為什麼能綁右值，而 T& 不行】
//   T& 綁右值若被允許，就會出現這種荒謬情形：
//       void bump(int& n) { ++n; }
//       bump(x + 1);      // 修改了一個馬上要消失的暫時物件 —— 改給誰看？
//   幾乎必定是筆誤（原意多半是 bump(x)），所以標準直接禁止。
//   const T& 則不同：既然唯讀，綁到暫時物件就沒有「改了卻沒人看到」的問題，
//   而且標準特別規定此時暫時物件的生命週期會延長到該引用的生命週期結束。
//   本檔的 clref2 就是這個規則的實例。
//
// 【5. 重載解析：const T& 與 T&& 並存時怎麼選】
//   傳左值 → 只有 const T& 可行（T&& 綁不了左值）→ 選 const T&
//   傳右值 → 兩者皆可行，但 T&& 是更精確的匹配 → 選 T&&
//   這正是「複製」與「移動」分流的機制。std::vector 的
//   push_back(const T&) 與 push_back(T&&) 就是這樣配對的。
//
// 【6. std::move 完全不移動任何東西】
//   它的實作本質上就是一個 cast：
//       template<class T>
//       constexpr std::remove_reference_t<T>&& move(T&& t) noexcept {
//           return static_cast<std::remove_reference_t<T>&&>(t);
//       }
//   它只是把左值「重新標記」成右值，一個位元組都沒搬。真正的搬移發生在
//   接收端 —— 移動建構函數或移動賦值運算子被選中並執行的時候。
//   所以 std::move(x) 之後 x 有沒有被清空，取決於有沒有人真的接手。
//   若接收端是 const T&（例如把 std::move(s) 傳給只吃 const T& 的函式），
//   那什麼都不會發生，x 完好如初。
//
// 【概念補充 Concept Deep Dive】
//
// (A) move 之後的物件到底是什麼狀態
//   標準的用語是 "valid but unspecified state"（有效但未指定）。
//   拆開來說：
//     * 有效  ：物件仍然存在，解構一定安全；所有「沒有前置條件」的操作
//               （clear()、operator=、size()）都可以呼叫，不是 UB。
//     * 未指定：它的「內容」沒有任何保證。
//   對 std::string 而言，本機 libstdc++ 實測（SSO 與 heap 兩種長度都測過）
//   move 後 size() 都是 0；但這是實作細節，不是標準要求。
//   正因如此，本檔刻意「不列印」move 後的內容或 size —— 把某次觀察到的值
//   寫進預期輸出，等於把實作細節偽裝成標準保證。
//   檔中改為先 clear() 再檢查 empty()，那才是標準真正保證的事。
//   實務守則：move 之後，只做「重新賦值」或「銷毀」，不要讀它。
//
// (B) 為什麼有名字的東西一律是左值
//   因為「有名字」等於「可以被再次提到」。編譯器無法知道你後面還會不會用它，
//   所以不能默許別人把它搬空。這條規則簡單、保守，而且可以由你自己用
//   std::move 明確覆寫 —— 責任回到寫程式的人身上，這正是 C++ 的一貫風格。
//
// (C) T&& 在樣板裡意義完全不同（本課尚未涉及，但先破除誤解）
//   當 T 是被推導的樣板參數時，T&& 叫做「轉發引用」（forwarding reference），
//   它左值右值都能綁，走的是引用摺疊規則。
//       template<class T> void f(T&& x);   // ← 轉發引用，不是右值引用
//       void g(std::string&& x);           // ← 這才是右值引用
//   看到 T&& 先確認 T 是不是「這個函式自己推導出來的」，這是兩套規則。
//
// (D) 為什麼 process("Knight") 會選中 T&& 版本
//   "Knight" 是 const char[8]，兩個重載都需要先轉成 std::string。
//   這個轉換會產生一個暫時物件（prvalue），而暫時物件是右值，
//   所以 T&& 版本勝出。這也說明：值類別是由「運算式」決定的，
//   不是由你寫下的字面樣貌決定的。
//
// 【注意事項 Pay Attention】
//   1. 右值引用變數是左值。要繼續傳遞「可搬走」的許可，必須再 std::move 一次。
//   2. std::move 不做任何搬移，只是 cast。真正的動作在接收端。
//   3. 不要讀取 move 之後的物件內容 —— 那是 valid but unspecified。
//      重新賦值或直接讓它解構才是正確用法。
//   4. 不要對區域變數的回傳值寫 return std::move(local);
//      那會阻止 NRVO，反而變慢（本來可以完全不複製）。
//   5. const 物件不能被移動：std::move(constObj) 的型別是 const T&&，
//      而移動建構函數收的是 T&&，最後會安靜地退回拷貝。
//      把該移動的東西宣告成 const，move 就白寫了。
//   6. 教材裡不要印位址來證明值類別 —— 位址每次執行都不同。
//      用 decltype 搭配 type traits，既精確又可重現。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】右值引用與值類別
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. T&、const T&、T&& 各自能綁什麼？為什麼 T& 不能綁右值？
//     答：T& 只綁左值；const T& 左值右值都能綁；T&& 只綁右值。
//         T& 不能綁右值是因為那會允許你修改一個馬上要消失的暫時物件，
//         幾乎必定是筆誤。const T& 沒這個問題（唯讀），
//         而且標準規定它會把暫時物件的生命週期延長到引用結束。
//     追問：那為什麼還需要 T&&，const T& 不是已經全包了嗎？
//         → const T& 只能讀，不能「偷」。T&& 的價值在於它同時告訴你
//           「這是暫時物件」且「你可以改它」，才有辦法搬走它的資源。
//
// 🔥 Q2. int&& r = 42; 之後，r 是左值還是右值？
//     答：r 是左值 —— 因為它有名字。它的「宣告型別」是右值引用，
//         但「作為運算式」它是左值。本機用 decltype 實測：
//         decltype(r) 是右值引用 = true，decltype((r)) 是左值引用 = true。
//         所以 f(r) 會選 const T& 版本，要選 T&& 版本必須寫 f(std::move(r))。
//     追問：那 decltype(r) 和 decltype((r)) 為什麼不一樣？
//         → decltype(名字) 給的是宣告型別；decltype(運算式)（加一層括號後
//           就是運算式）給的是值類別 —— 左值得到 T&、xvalue 得到 T&&、
//           prvalue 得到 T。這是 decltype 刻意設計的兩種模式。
//
// 🔥 Q3. std::move 到底做了什麼？
//     答：它是個 cast，實作等同 static_cast<remove_reference_t<T>&&>(t)，
//         一個位元組都沒搬。它只是把左值標記成右值，讓重載解析選中
//         移動建構／移動賦值。真正的搬移由接收端完成。
//     追問：如果沒有人接手呢？
//         → 那就什麼都不會發生。例如把 std::move(s) 傳給只吃 const T&
//           的函式，s 會完好無缺。「move 之後一定被清空」是錯的。
//
// ⚠️ 陷阱 1. 「函式參數宣告成 T&&，在函式裡它就是右值」——錯在哪？
//     答：在函式內它是左值，因為它有名字。這是最常見的誤解，後果是
//         你以為自己在移動，實際上一路都在複製。要往下傳遞必須寫
//         std::move(param)（或在樣板中用 std::forward）。
//     為什麼會錯：把「型別」和「值類別」混為一談。宣告型別是 T&&
//         描述的是「這個參數能綁什麼」，不是「它作為運算式是什麼」。
//
// ⚠️ 陷阱 2. 「std::move 之後原物件一定變成空的」——不保證。
//     答：標準只說它處於 valid but unspecified state。內容沒有保證，
//         而且如果接收端根本沒有移動操作（例如只吃 const T&），
//         原物件會完好如初。本機 libstdc++ 對 std::string 實測是變成
//         size()==0，但那是實作細節，不能寫成保證。
//     為什麼會錯：把「常見實作行為」當成「標準規定」。
//         正確心態是：move 之後只做重新賦值或銷毀，不要讀它。
//
// ⚠️ 陷阱 3. 「回傳區域變數時加上 std::move 可以避免拷貝」——反而變慢。
//     答：return std::move(local); 會把 local 變成 xvalue，
//         使編譯器無法套用具名回傳值最佳化（NRVO），從「完全不複製」
//         退化成「一次移動」。正確寫法就是 return local;。
//     為什麼會錯：以為「多寫 move 總不會更差」。實際上它抑制了
//         比 move 更強的最佳化 —— 直接在呼叫端就地建構。
//
// ⚠️ 陷阱 4. 「把成員宣告成 const 更安全」——會讓 move 失效。
//     答：std::move(constObj) 的結果型別是 const T&&，
//         而移動建構函數的參數是 T&&，接不了；於是安靜地退回
//         const T& 的拷貝版本。沒有錯誤、沒有警告，只是變慢。
//     為什麼會錯：把 const 當成純粹的保護。它同時也是型別的一部分，
//         會參與重載解析並改變選中的函式。
// ═══════════════════════════════════════════════════════════════════════════

// ============================================================
// 第 31 課 總結：右值引用（Rvalue Reference）入門
// 編譯：g++ -std=c++17 -o summary summary.cpp
// ============================================================
// 【Lvalue vs Rvalue】
//   Lvalue（左值）：有名字、可取位址 → int x = 10; 中的 x
//   Rvalue（右值）：臨時的、沒名字   → 10、x+y、func() 的回傳值
//
// 【三種引用類型與綁定規則】
//   T&        左值引用       → 只能綁定左值
//   const T&  const 左值引用 → 可綁定左值和右值（萬能讀取）
//   T&&       右值引用       → 只能綁定右值
//
// 【重要陷阱！】右值引用變數本身是左值！
//   int&& rref = 42;       // rref 綁定右值 42
//   // 但 rref 本身有名字、可取位址 → rref 是左值！
//   // int&& rref2 = rref;  // ❌ 左值不能綁到 T&&
//   int&& rref2 = std::move(rref);  // ✅ 用 std::move 轉回右值
//
// 【函數重載：const T& vs T&&】
//   傳左值 → 選 const T& 版本
//   傳右值 → 優先選 T&& 版本（更精確匹配）
//   這是移動語義的基礎：拷貝和移動分別走不同的重載
//
// 【std::move 預覽】
//   std::move(x) 只是 static_cast<T&&>(x)，把左值「標記為」右值
//   本身不移動任何東西，真正的移動由接收端（移動建構/賦值）完成
//   move 後的物件處於「有效但未指定」狀態
// ============================================================

#include <iostream>
#include <string>
#include <utility>      // std::move
#include <type_traits>  // std::is_lvalue_reference（用來「證明」值類別）

// ============================================================
// 函數重載示範
// ============================================================
void process(const std::string& s) {
    std::cout << "  [const T& 版本] \"" << s << "\"\n";
}

void process(std::string&& s) {
    std::cout << "  [T&& 版本]      \"" << s << "\"\n";
}

// ============================================================
// 右值引用參數在函數內是左值
// ============================================================
void takeRvalueRef(int&& val) {
    std::cout << "  takeRvalueRef: val=" << val << "\n";
    // 不印 &val 的位址（每次執行都不同、無法重現），改用 decltype 直接「證明」：
    //   decltype(val)   → 宣告型別，是 int&&
    //   decltype((val)) → 運算式 val 的值類別；是左值時會得到 int&
    std::cout << "  decltype(val)   是右值引用? "
              << std::boolalpha << std::is_rvalue_reference<decltype(val)>::value
              << "  ← 宣告型別確實是 int&&\n";
    std::cout << "  decltype((val)) 是左值引用? "
              << std::is_lvalue_reference<decltype((val))>::value
              << "  ← 但運算式 val 是左值！\n";
    // takeRvalueRef(val);  // ❌ val 是左值，不能再傳給 T&&
}

void takeLvalueRef(int& val) {
    std::cout << "  takeLvalueRef: val=" << val << "\n";
}

#include <vector>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1656. Design an Ordered Stream
//   題目：OrderedStream(int n) 建立 n 個槽位；insert(idKey, value) 放入一筆資料，
//         並回傳「從目前指標開始、連續已就位」的那一段 value（可能是空的）。
//   為什麼用到本主題：insert 的參數是 std::string value（按值傳入），
//         而它馬上要被存進槽位、之後再被搬進回傳的 vector。
//         整條路徑上這個字串都不需要被複製 —— 這正是右值引用與 std::move
//         存在的理由。本題是少數「移動語意能自然用上」的 LeetCode 設計題。
//   關鍵寫法：
//         * 參數用「按值 + std::move」（sink parameter 慣用法），
//           呼叫端傳右值時整條路徑零複製。
//         * 回傳時用 std::move(m_slots[i]) 把字串搬進結果 vector；
//           搬走之後那個槽位不會再被讀取，符合「move 後不要讀」的守則。
// -----------------------------------------------------------------------------
class OrderedStream {
private:
    std::vector<std::string> m_slots;
    int m_ptr;      // 下一個要輸出的位置（1-based）

public:
    explicit OrderedStream(int n) : m_slots(static_cast<std::size_t>(n) + 1), m_ptr(1) {}

    std::vector<std::string> insert(int idKey, std::string value) {
        // 按值收下後直接 move 進槽位：呼叫端若傳右值，全程零複製
        m_slots[static_cast<std::size_t>(idKey)] = std::move(value);

        std::vector<std::string> chunk;
        while (m_ptr < static_cast<int>(m_slots.size()) &&
               !m_slots[static_cast<std::size_t>(m_ptr)].empty()) {
            // 搬走而不是複製；搬完之後這個槽位不會再被讀取
            chunk.push_back(std::move(m_slots[static_cast<std::size_t>(m_ptr)]));
            ++m_ptr;
        }
        return chunk;   // 直接 return 具名區域變數，讓 NRVO 生效（不要寫 std::move）
    }
};

static std::string joinChunk(const std::vector<std::string>& v) {
    if (v.empty()) return "[]";
    std::string s = "[";
    for (std::size_t i = 0; i < v.size(); ++i) {
        if (i) s += ",";
        s += v[i];
    }
    return s + "]";
}

void leetcode1656Demo() {
    OrderedStream os(5);
    // LeetCode 官方範例的插入順序
    std::cout << "  insert(3,\"ccccc\") -> " << joinChunk(os.insert(3, "ccccc")) << "\n";
    std::cout << "  insert(1,\"aaaaa\") -> " << joinChunk(os.insert(1, "aaaaa")) << "\n";
    std::cout << "  insert(2,\"bbbbb\") -> " << joinChunk(os.insert(2, "bbbbb")) << "\n";
    std::cout << "  insert(5,\"eeeee\") -> " << joinChunk(os.insert(5, "eeeee")) << "\n";
    std::cout << "  insert(4,\"ddddd\") -> " << joinChunk(os.insert(4, "ddddd")) << "\n";
}

// -----------------------------------------------------------------------------
// 【日常實務範例】sink parameter：把「要收下的資料」按值收，再 move 進去
//   情境：一個訊息佇列，enqueue() 收下一則訊息並存起來。呼叫端有兩種：
//         有些人手上的訊息之後還要用（傳左值），有些人是現組現丟（傳右值）。
//   為什麼用到本主題：這正是 const T& 與 T&& 想解決的問題。有三種寫法：
//         (a) enqueue(const std::string&)      → 左值右值都吃，但一律複製
//         (b) 同時寫 const T& 與 T&& 兩個重載  → 最快，但參數一多會爆炸
//             （n 個參數要寫 2^n 個重載）
//         (c) enqueue(std::string msg) + std::move(msg)   ← 本例採用
//             按值收下：傳左值時複製一次、傳右值時移動一次，
//             然後統一 move 進容器。只寫一份程式碼，兩種情境都接近最佳。
//   本例用一個會自報行蹤的 TracedString 把「複製了幾次、移動了幾次」數出來，
//   數的是次數而不是耗時，所以每次執行結果都相同。
// -----------------------------------------------------------------------------
class TracedString {
private:
    std::string m_data;
public:
    static int s_copies;
    static int s_moves;

    TracedString(const char* s) : m_data(s) {}
    TracedString(const TracedString& o) : m_data(o.m_data) { ++s_copies; }
    TracedString(TracedString&& o) noexcept : m_data(std::move(o.m_data)) { ++s_moves; }
    TracedString& operator=(const TracedString& o) { m_data = o.m_data; ++s_copies; return *this; }
    TracedString& operator=(TracedString&& o) noexcept { m_data = std::move(o.m_data); ++s_moves; return *this; }

    const std::string& str() const { return m_data; }
    static void reset() { s_copies = 0; s_moves = 0; }
};
int TracedString::s_copies = 0;
int TracedString::s_moves  = 0;

class MessageQueue {
private:
    std::vector<TracedString> m_items;
public:
    MessageQueue() { m_items.reserve(8); }   // 先 reserve，排除擴容造成的干擾

    // sink parameter：按值收下，再 move 進容器
    void enqueue(TracedString msg) { m_items.push_back(std::move(msg)); }

    std::size_t size() const { return m_items.size(); }
};

void sinkParameterDemo() {
    {
        MessageQueue q;
        TracedString::reset();
        TracedString keepUsingIt("audit: user=alice");   // 之後還要用 → 傳左值
        q.enqueue(keepUsingIt);
        std::cout << "  傳左值  : 複製 " << TracedString::s_copies
                  << " 次, 移動 " << TracedString::s_moves
                  << " 次（必要的複製 1 次 + 進容器的移動 1 次）\n";
        std::cout << "            原物件仍可用: \"" << keepUsingIt.str() << "\"\n";
    }
    {
        MessageQueue q;
        TracedString::reset();
        q.enqueue(TracedString("audit: user=bob"));      // 現組現丟 → 傳右值
        std::cout << "  傳右值  : 複製 " << TracedString::s_copies
                  << " 次, 移動 " << TracedString::s_moves
                  << " 次（全程零複製）\n";
    }
    {
        MessageQueue q;
        TracedString::reset();
        TracedString wontUseAgain("audit: user=carol");  // 之後不再用 → 明確 move
        q.enqueue(std::move(wontUseAgain));
        std::cout << "  明確 move: 複製 " << TracedString::s_copies
                  << " 次, 移動 " << TracedString::s_moves
                  << " 次（零複製，但比傳暫時物件多一次移動）\n";
        std::cout << "            原因：具名變數 → 參數 msg 先搬一次，\n";
        std::cout << "                  參數 msg → 容器再搬一次，共 2 次。\n";
        std::cout << "                  傳暫時物件時第一段被 C++17 保證的\n";
        std::cout << "                  copy elision 省掉了，所以只有 1 次。\n";
        std::cout << "            （wontUseAgain 已被搬空，依規矩不再讀取它）\n";
    }
}
int main() {
    // ============================================================
    // 1. 綁定規則
    // ============================================================
    std::cout << "===== 1. 綁定規則 =====\n";
    int x = 42;

    int& lref = x;             // ✅ 左值引用 → 左值
    // int& lref2 = 42;        // ❌ 左值引用 → 右值（不行）

    const int& clref1 = x;    // ✅ const 左值引用 → 左值
    const int& clref2 = 42;   // ✅ const 左值引用 → 右值（特殊規則！）

    int&& rref = 42;           // ✅ 右值引用 → 右值
    // int&& rref2 = x;        // ❌ 右值引用 → 左值（不行）

    rref = 100;                // ✅ 右值引用本身可修改
    std::cout << "  lref   = " << lref   << " (T&       綁到左值 x)\n";
    std::cout << "  clref1 = " << clref1 << " (const T& 綁到左值 x)\n";
    std::cout << "  clref2 = " << clref2 << " (const T& 綁到右值 42 → 生命週期被延長)\n";
    std::cout << "  rref   = " << rref   << " (T&&      綁到右值，且可修改)\n\n";

    // ============================================================
    // 2. 右值引用變數本身是左值（重要陷阱！）
    // ============================================================
    std::cout << "===== 2. 右值引用變數是左值 =====\n";
    int&& r = 42;
    std::cout << "  r = " << r << "\n";
    // 同樣不印位址，用 decltype 證明「宣告型別是右值引用，但運算式是左值」
    std::cout << "  decltype(r)   是右值引用? "
              << std::boolalpha << std::is_rvalue_reference<decltype(r)>::value << "\n";
    std::cout << "  decltype((r)) 是左值引用? "
              << std::is_lvalue_reference<decltype((r))>::value
              << " ← 有名字就是左值\n";

    // r 是左值 → 可以傳給 T&
    takeLvalueRef(r);      // ✅

    // r 是左值 → 不能傳給 T&&
    // takeRvalueRef(r);   // ❌

    // 用 std::move 把 r 轉回右值
    takeRvalueRef(std::move(r));  // ✅
    std::cout << "\n";

    // 函數參數 T&& 在函數內也是左值
    std::cout << "===== 函數參數 T&& 在函數內是左值 =====\n";
    takeRvalueRef(99);
    std::cout << "\n";

    // ============================================================
    // 3. 函數重載：const T& vs T&&
    // ============================================================
    std::cout << "===== 3. 函數重載 =====\n";
    std::string name = "Dragon";

    std::cout << "  傳入左值：\n";
    process(name);                      // 左值 → const T& 版本

    std::cout << "  傳入右值（臨時物件）：\n";
    process(std::string("Phoenix"));    // 右值 → T&& 版本

    std::cout << "  傳入字面量：\n";
    process("Knight");                  // 字面量 → 建構臨時 string → T&& 版本

    std::cout << "  傳入運算結果：\n";
    process(name + " King");            // 運算結果是臨時物件 → T&& 版本
    std::cout << "\n";

    // ============================================================
    // 4. std::move 預覽
    // ============================================================
    std::cout << "===== 4. std::move 預覽 =====\n";
    std::string a = "Alpha";

    std::cout << "  傳入左值 a：\n";
    process(a);                   // const T& 版本

    std::cout << "  傳入 std::move(a)：\n";
    process(std::move(a));        // T&& 版本

    std::cout << "  move 後 a 的狀態：\n";
    // ⚠️ 這裡「刻意不列印」a 的內容或 size()。
    //    標準只保證 move 後的物件處於 valid but unspecified state，
    //    其確切內容沒有任何保證。把某次觀察到的值寫進預期輸出，
    //    等於把實作細節偽裝成標準保證 —— 換一個標準函式庫就可能不同。
    //    能保證的是：對它呼叫「沒有前置條件」的操作是安全的（例如重新賦值、
    //    clear()），以及解構一定安全。下面就只驗證這件「有保證」的事。
    std::cout << "  （valid but unspecified：內容無保證，故不列印）\n";
    a.clear();                    // 無前置條件的操作 → 保證安全
    std::cout << "  clear() 後 a.empty() = " << std::boolalpha << a.empty()
              << " ← 這一步才是標準有保證的\n";

    a = "Gamma";  // ✅ 重新賦值是安全的
    std::cout << "  重新賦值後 a = \"" << a << "\"\n\n";

    // ============================================================
    // 5. 右值引用變數傳給重載函數
    // ============================================================
    std::cout << "===== 5. 右值引用變數傳給重載函數 =====\n";
    std::string&& sref = std::string("Omega");
    process(sref);              // const T& 版本！因為 sref 是左值
    process(std::move(sref));   // T&& 版本

    std::cout << "===== 6. LeetCode 1656. Design an Ordered Stream =====\n";
    leetcode1656Demo();
    std::cout << "\n";

    std::cout << "===== 7. 日常實務：sink parameter（按值收 + move）=====\n";
    sinkParameterDemo();
    std::cout << "\n";
    std::cout << "\n=== 重點整理 ===\n";
    std::cout << "  T&       → 只綁左值\n";
    std::cout << "  const T& → 綁左值和右值（萬能）\n";
    std::cout << "  T&&      → 只綁右值\n";
    std::cout << "  右值引用變數本身是左值（有名字就是左值）\n";
    std::cout << "  std::move 只是 cast，不做移動\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra summary.cpp -o summary

// 【本檔對原始版本做的兩項修改，以及為什麼】
//   (1) 原版用 std::cout << &val 印位址來證明「右值引用變數是左值」。
//       位址受 ASLR 影響，每次執行都不同，無法寫成可重現的預期輸出。
//       本檔改用 decltype + type traits 直接證明：
//           decltype(val)   → 宣告型別 int&&   （is_rvalue_reference = true）
//           decltype((val)) → 運算式值類別 int& （is_lvalue_reference = true）
//       這比印位址更精確 —— 它證明的正是「型別」與「值類別」是兩件事。
//   (2) 原版在 std::move(a) 之後列印 a 的內容與 a.size()。
//       標準只保證 move 後的物件處於 valid but unspecified state，
//       內容沒有任何保證；把某次觀察到的值寫進預期輸出，
//       等於把實作細節偽裝成標準保證。本檔改為只驗證「標準真正保證的事」：
//       對它呼叫無前置條件的操作（clear()）是安全的。
//       參考資訊：本機 libstdc++ 對 std::string 實測（SSO 與 heap 兩種長度）
//       move 後 size() 皆為 0 —— 但這是實作細節，故不列入預期輸出。
//   另外把原本未使用的 lref / clref1 / clref2 實際印出來
//   （原版有 3 個 -Wunused-variable 警告，現在是 0 警告）。
//
// 【關於「明確 move」為什麼是 2 次移動】
//   這是實跑數據帶出的重點，值得特別說明：
//       enqueue(TracedString("..."))        → 移動 1 次
//       enqueue(std::move(namedVariable))   → 移動 2 次
//   差別在於傳暫時物件（prvalue）時，「暫時物件 → 參數 msg」這一段
//   被 C++17 保證的 copy elision 完全省掉，參數就地建構；
//   而具名變數必須先搬進參數，再從參數搬進容器，所以是兩段。
//   兩者都是零複製，但不是「一樣好」。
//
// 【計數為何可信】
//   TracedString 數的是複製／移動的「次數」，由程式邏輯決定，
//   不受機器負載影響，每次執行都相同。MessageQueue 建構時先 reserve(8)，
//   排除了 vector 擴容造成的額外移動干擾。

// === 預期輸出 ===
// ===== 1. 綁定規則 =====
//   lref   = 42 (T&       綁到左值 x)
//   clref1 = 42 (const T& 綁到左值 x)
//   clref2 = 42 (const T& 綁到右值 42 → 生命週期被延長)
//   rref   = 100 (T&&      綁到右值，且可修改)
//
// ===== 2. 右值引用變數是左值 =====
//   r = 42
//   decltype(r)   是右值引用? true
//   decltype((r)) 是左值引用? true ← 有名字就是左值
//   takeLvalueRef: val=42
//   takeRvalueRef: val=42
//   decltype(val)   是右值引用? true  ← 宣告型別確實是 int&&
//   decltype((val)) 是左值引用? true  ← 但運算式 val 是左值！
//
// ===== 函數參數 T&& 在函數內是左值 =====
//   takeRvalueRef: val=99
//   decltype(val)   是右值引用? true  ← 宣告型別確實是 int&&
//   decltype((val)) 是左值引用? true  ← 但運算式 val 是左值！
//
// ===== 3. 函數重載 =====
//   傳入左值：
//   [const T& 版本] "Dragon"
//   傳入右值（臨時物件）：
//   [T&& 版本]      "Phoenix"
//   傳入字面量：
//   [T&& 版本]      "Knight"
//   傳入運算結果：
//   [T&& 版本]      "Dragon King"
//
// ===== 4. std::move 預覽 =====
//   傳入左值 a：
//   [const T& 版本] "Alpha"
//   傳入 std::move(a)：
//   [T&& 版本]      "Alpha"
//   move 後 a 的狀態：
//   （valid but unspecified：內容無保證，故不列印）
//   clear() 後 a.empty() = true ← 這一步才是標準有保證的
//   重新賦值後 a = "Gamma"
//
// ===== 5. 右值引用變數傳給重載函數 =====
//   [const T& 版本] "Omega"
//   [T&& 版本]      "Omega"
// ===== 6. LeetCode 1656. Design an Ordered Stream =====
//   insert(3,"ccccc") -> []
//   insert(1,"aaaaa") -> [aaaaa]
//   insert(2,"bbbbb") -> [bbbbb,ccccc]
//   insert(5,"eeeee") -> []
//   insert(4,"ddddd") -> [ddddd,eeeee]
//
// ===== 7. 日常實務：sink parameter（按值收 + move）=====
//   傳左值  : 複製 1 次, 移動 1 次（必要的複製 1 次 + 進容器的移動 1 次）
//             原物件仍可用: "audit: user=alice"
//   傳右值  : 複製 0 次, 移動 1 次（全程零複製）
//   明確 move: 複製 0 次, 移動 2 次（零複製，但比傳暫時物件多一次移動）
//             原因：具名變數 → 參數 msg 先搬一次，
//                   參數 msg → 容器再搬一次，共 2 次。
//                   傳暫時物件時第一段被 C++17 保證的
//                   copy elision 省掉了，所以只有 1 次。
//             （wontUseAgain 已被搬空，依規矩不再讀取它）
//
//
// === 重點整理 ===
//   T&       → 只綁左值
//   const T& → 綁左值和右值（萬能）
//   T&&      → 只綁右值
//   右值引用變數本身是左值（有名字就是左值）
//   std::move 只是 cast，不做移動
