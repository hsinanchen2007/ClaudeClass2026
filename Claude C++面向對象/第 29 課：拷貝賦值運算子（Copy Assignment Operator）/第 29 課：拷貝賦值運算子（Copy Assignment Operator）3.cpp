// =============================================================================
//  第 29 課：拷貝賦值運算子 3  —  用 placement new 硬解「不可賦值」的類別
// =============================================================================
//
// 【主題資訊 Information】
//   語法    ： new (指標) 型別(建構參數...)     // placement new，不配置記憶體
//   標頭檔  ： <new>（placement new 的 operator new 多載、std::launder）
//   標準版本： placement new 自 C++98 起；std::launder 是 C++17 新增
//             （本機以 -std=c++14 -pedantic-errors 實測：
//              error: 'launder' is not a member of 'std'）
//   複雜度  ： 一次解構 + 一次建構，與該型別本身的成本相同
//   風險等級： ★★★★☆ —— 能動，但它是本課三種解法裡最危險的一種
//
// 【詳細解釋 Explanation】
//
// 【1. 這個技巧的核心想法：把「賦值」換成「重生」】
//   const 成員與引用成員擋的是「賦值」，不是「建構」。既然建構這條路是通的，
//   就先把舊物件解構掉，再在同一塊記憶體上用拷貝建構造一個新的：
//       this->~Config();            // ① 結束舊物件的生命週期
//       new (this) Config(other);   // ② 在原地開始一個新物件的生命週期
//   placement new 的特別之處是它「不配置記憶體」，只在你給的位址上跑建構函式。
//   這正是 std::vector、std::optional、std::variant 內部管理元素的方式。
//
// 【2. 為什麼 copy-and-swap 救不了這種類別】
//   copy-and-swap 需要一個能交換全部成員的 swap，而 std::swap 內部是賦值。
//   const 成員不能賦值、引用不能重新綁定，問題只是從 operator= 原封不動搬到
//   swap 裡。除非 swap 自己也用 placement new —— 那就只是換個地方寫同一招，
//   沒有任何好處。
//
// 【3. 三個真實存在的風險（不是理論上的）】
//   風險 A：例外中斷會留下「屍體」
//       ① 已經把舊物件解構掉；若 ② 的建構函式丟出例外，這塊記憶體上就
//       沒有任何活著的物件了。之後解構函式仍會被呼叫 → 對已死物件二次解構。
//       這連基本例外保證都達不到。
//   風險 B：繼承體系會出事
//       若 Config 是某個基底的一部分，this->~Config() 只解構自己這一層；
//       而且如果有虛擬函式，placement new 會重寫 vptr —— 對衍生類別物件
//       的基底子物件這樣做，會把它的動態型別改掉，後果嚴重。
//   風險 C：物件身分（object identity）在標準上被切斷 ← 最容易被忽略
//       見下面【概念補充 (A)】，這是本檔真正的重點。
//
// 【4. 為什麼下面的程式碼仍然保留原樣】
//   因為它就是這一課要示範的「教科書版 placement new 解法」，也是網路上
//   最常見的寫法。我們不改它，但把它的每一個問題明講出來 ——
//   包括它其實踩到了一個標準層級的未定義行為（見概念補充 A）。
//   會看、會判斷、知道什麼時候不該用，比背下一個招式重要。
//
// 【概念補充 Concept Deep Dive】
//
// (A) [basic.life]/8：placement new 之後，舊名字不一定還算數
//   標準規定：在原地重建物件後，「原本的指標／參考／變數名」要能自動指向
//   新物件，必須同時滿足數個條件，其中一條是：
//       原物件的型別不是 const 限定，且若是類別型別，
//       不得含有任何 const 限定或引用型別的非靜態資料成員。
//   Config 剛好兩條都踩：它有 const int m_id，也有 std::string& m_nameRef。
//   因此 placement new 之後：
//       * operator= 裡的 return *this;  —— this 是重建前的指標
//       * main 裡的 c1.print();         —— c1 是重建前的名字
//   兩者在標準上都不保證指向新物件，屬於未定義行為。
//   注意：未定義行為「不等於」一定會出錯。本機這份建置實際印出的是重建後的
//   正確值（見檔尾預期輸出），但那是這個編譯器這次的選擇，不是標準的承諾；
//   換編譯器、換最佳化等級都可能不同，不可以拿它當成保證。
//
//   C++17 起的正解是 std::launder：
//       Config& operator=(const Config& other) {
//           if (this != &other) {
//               this->~Config();
//               new (this) Config(other);
//           }
//           return *std::launder(this);   // ← 明確告訴編譯器：重新看這塊記憶體
//       }
//   呼叫端也必須用 laundered 指標讀取，而不是原本的名字。
//   ——「必須全鏈路 launder」本身就說明了：這條路窄得不值得走。
//
// (B) 為什麼 g++ 對這段程式碼發出 -Wdeprecated-copy 警告
//   本機 g++ 15.2 會報：
//       warning: implicitly-declared 'constexpr Config::Config(const Config&)'
//                is deprecated [-Wdeprecated-copy]
//   原因是：類別「自己宣告了」拷貝賦值運算子，此時編譯器仍會生成隱式拷貝
//   建構函式，但這個行為自 C++11 起被標記為 deprecated —— 因為既然你認為
//   拷貝賦值需要特別處理，拷貝建構多半也需要，兩者不該一個手寫一個自動生成。
//   這正是三法則（第 30 課）想提醒的事，而 placement new 這一招偏偏
//   同時「手寫 operator=」又「依賴隱式拷貝建構」，剛好撞在警告上。
//   要消除警告，明確寫出 Config(const Config&) = default; 即可。
//
// (C) 業界真正在用 placement new 的地方
//   不是拿來救 const 成員，而是：
//     * std::vector：先配置未初始化的原始記憶體，再逐一 placement new 建構元素，
//       讓「配置容量」與「建構元素」徹底分離。
//     * std::optional / std::variant：在內嵌的 storage 上建構與解構。
//     * 記憶體池、arena allocator、低延遲系統的物件重用。
//   共同點是：它們都被封裝在函式庫內部，呼叫端拿到的是安全介面，
//   而不是要求使用者自己去 launder。
//
// 【注意事項 Pay Attention】
//   1. 本檔的做法可以動，但不建議在正式產品中對「含 const／引用成員的類別」
//      使用。優先順序應該是：改成員設計 ＞ 明確 = delete ＞ std::optional
//      ＞＞ placement new。
//   2. this->~Config() 之後、placement new 成功之前，物件是不存在的。
//      這段空窗期若有例外，就沒有任何合法方式收拾。
//   3. 自我賦值檢查在這裡不是最佳化，是必要條件：少了 if (this != &other)，
//      a = a 會先把 a 解構掉，再拿已解構的 a 當拷貝來源。
//   4. 千萬不要對「衍生類別物件的基底子物件」做這招，也不要對含虛擬函式的
//      類別隨意使用 —— vptr 會被改寫。
//   5. 真正務實的替代方案是 std::optional<T>：它把「銷毀 + 重建」封裝成
//      emplace()，例外時狀態明確（變成 disengaged），而且完全不需要 launder。
//      本檔下方的實務範例就是示範這個做法。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】placement new 與物件生命週期
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. placement new 跟一般的 new 差在哪裡？
//     答：一般 new 做兩件事 —— 配置記憶體、然後在上面建構物件。
//         placement new 只做第二件：你給它一塊已經存在的記憶體，
//         它就在那個位址上呼叫建構函式，不做任何配置。
//         對應地，它也沒有 placement delete 可用，
//         必須自己明確呼叫解構函式 p->~T()。
//     追問：那什麼時候該呼叫解構函式、什麼時候該 free 記憶體？
//         → 兩者完全分離：p->~T() 結束物件生命週期，
//           釋放記憶體則交給當初配置它的那一方（operator delete／池／堆疊）。
//
// 🔥 Q2. 為什麼 std::vector 內部要用 placement new？
//     答：因為 vector 必須把「容量」和「元素個數」分開。reserve(1000) 只該
//         配置記憶體、不該建構 1000 個物件（元素型別可能根本沒有預設建構函式）。
//         所以它先取得未初始化的原始記憶體，再於 push_back 時逐一
//         placement new 建構，pop_back 時明確呼叫解構函式。
//     追問：這跟 new T[1000] 的差別？
//         → new T[1000] 會立刻預設建構全部 1000 個元素，
//           而且要求 T 必須可預設建構，兩者都不符合 vector 的需求。
//
// ⚠️ 陷阱 1. 「placement new 之後，原本的變數名就會指向新物件」——不一定。
//     答：依 [basic.life]/8，只有在滿足一組條件時舊名字才自動指向新物件；
//         其中一條是「類別不得含有 const 限定或引用型別的非靜態資料成員」。
//         本檔的 Config 兩者都有，所以 return *this 與 main 裡的 c1.print()
//         在標準上都是未定義行為。C++17 起要用 std::launder 才算數。
//     為什麼會錯：把 C++ 物件想成「就是那塊記憶體」。但標準區分「儲存空間」
//         與「物件」——儲存空間還在，原本那個物件的生命週期卻已經結束了，
//         編譯器有權基於「const 成員不會變」這類假設做最佳化。
//
// ⚠️ 陷阱 2. 「反正跑起來結果正確，就代表這樣寫沒問題」——為什麼危險？
//     答：未定義行為的定義是「標準不對結果做任何要求」，不是「一定會壞」。
//         本機這份建置確實印出正確的值，但那是這個編譯器、這個最佳化等級
//         的偶然結果。開了 -O2、換 clang、或編譯器某天更積極地利用
//         const 成員的不變性，行為就可能改變，而且不會有任何警告。
//     為什麼會錯：拿「測試通過」當成「行為有保證」。
//         UB 最麻煩的地方正是它經常在測試環境下看起來完全正常。
//
// ⚠️ 陷阱 3. placement new 版的 operator= 少了自我賦值檢查會怎樣？
//     答：a = a 時會先執行 a.~Config() 把來源自己解構掉，
//         接著 new (this) Config(other) 又拿這個已經解構的 a 當拷貝來源。
//         這是讀取已結束生命週期的物件，同樣是未定義行為。
//     為什麼會錯：習慣了 copy-and-swap「不需要自我賦值檢查」，
//         就以為那是通則。事實相反 —— copy-and-swap 是特例，
//         它之所以安全，是因為複製發生在任何破壞性動作之前。
// ═══════════════════════════════════════════════════════════════════════════

#include <string>
#include <iostream>
#include <new>       // placement new 所需的標頭檔

class Config {
private:
    const int m_id;          // const 成員：初始化後不可賦值
    std::string& m_nameRef;  // 引用成員：綁定後不可重新綁定

public:
    Config(int id, std::string& name) : m_id(id), m_nameRef(name) {}

    // ================================================================
    // 自訂拷貝賦值運算子 —— 使用 placement new 技巧
    // ================================================================
    // 問題：編譯器無法自動生成 operator=，因為：
    //   - const 成員 (m_id) 不能被重新賦值
    //   - 引用成員 (m_nameRef) 不能被重新綁定
    //
    // 解法：既然「賦值」行不通，我們改用「銷毀 + 重建」
    //   1. 先呼叫解構函數，銷毀當前物件
    //   2. 再用 placement new 在同一塊記憶體上重新建構
    //      placement new 會呼叫拷貝建構函數（編譯器可以自動生成，
    //      因為 const 和引用在「建構」時都能正常初始化）
    //
    // ⚠️ 注意：這是一種進階技巧，有一定風險：
    //   - 若建構函數拋出例外，物件會處於已銷毀但未重建的狀態
    //   - 若類別有虛擬函數或處於繼承體系中，需要更謹慎處理
    //   - 一般建議優先考慮改用指標取代引用、移除 const 等設計調整
    // ================================================================
    Config& operator=(const Config& other) {
        if (this != &other) {           // 防止自我賦值（自我賦值會導致先銷毀再讀取已銷毀的資料）
            this->~Config();            // 步驟 1：手動呼叫解構函數，銷毀當前物件
            new (this) Config(other);   // 步驟 2：placement new，在 this 的記憶體上
                                        //         呼叫拷貝建構函數，用 other 的值重建
        }
        return *this;                   // 回傳 *this 以支援鏈式賦值 (a = b = c)
    }

    void print() const {
        std::cout << "Config { id=" << m_id
                  << ", name=\"" << m_nameRef << "\" }" << std::endl;
    }
};

#include <optional>
#include <type_traits>

// -----------------------------------------------------------------------------
// 【日常實務範例】用 std::optional 取代手寫 placement new
//   情境：連線階段（handshake）解析出來的 SessionInfo 帶有「一旦建立就不可
//         變更」的欄位（session id、協定版本），自然會寫成 const 成員；
//         但同一個連線物件在重新協商（renegotiation）時，需要整份換掉。
//   為什麼用到本主題：這正是「不可賦值、卻需要重新指派」的典型情境 ——
//         也就是上面 placement new 想解決的問題。
//   為什麼改用 std::optional：emplace() 內部就是「解構舊的 + 在原地建構新的」，
//         但它是由標準函式庫實作的：
//           * 例外安全有明確定義 —— 若建構丟出例外，optional 變成 disengaged
//             （has_value() == false），而不是留下一具無法收拾的屍體。
//           * 完全不需要 std::launder：存取一律經過 optional 的介面。
//           * 「目前有沒有值」變成型別能表達的狀態，而不是靠註解約定。
// -----------------------------------------------------------------------------
class SessionInfo {
private:
    const int         m_sessionId;    // 建立後不可變更
    const std::string m_protocol;     // 建立後不可變更
public:
    SessionInfo(int id, std::string protocol)
        : m_sessionId(id), m_protocol(std::move(protocol)) {}

    int                id()       const { return m_sessionId; }
    const std::string& protocol() const { return m_protocol; }
};

// SessionInfo 因為 const 成員而不可賦值 —— 由編譯器背書
static_assert(!std::is_copy_assignable<SessionInfo>::value,
              "SessionInfo 含 const 成員，理應不可拷貝賦值");

class Connection {
private:
    std::optional<SessionInfo> m_session;   // 尚未 handshake 時為 disengaged
public:
    void handshake(int id, const std::string& protocol) {
        // emplace = 銷毀舊的（若有）+ 就地建構新的，語意等同 placement new，
        //           但例外安全與物件生命週期都由標準函式庫負責
        m_session.emplace(id, protocol);
    }

    void reset() { m_session.reset(); }

    void report() const {
        if (m_session) {
            std::cout << "  session #" << m_session->id()
                      << " protocol=" << m_session->protocol() << "\n";
        } else {
            std::cout << "  尚未建立 session（disengaged）\n";
        }
    }
};

void optionalDemo() {
    Connection conn;
    conn.report();                       // 還沒 handshake

    conn.handshake(1001, "TLS1.3");
    conn.report();

    // 重新協商：整份 SessionInfo 換掉。若用手寫 placement new，
    // 這一步必須自己處理解構、例外與 launder；用 optional 只要一行。
    conn.handshake(1002, "TLS1.3-resumed");
    conn.report();

    conn.reset();
    conn.report();
}

int main() {
    std::string name1 = "Alice";
    std::string name2 = "Bob";
    Config c1(1, name1);
    Config c2(2, name2);

    std::cout << "=== 賦值前 ===" << std::endl;
    std::cout << "c1: "; c1.print();
    std::cout << "c2: "; c2.print();

    c1 = c2;  // ✅ 現在可以編譯！使用自訂的 operator= (placement new)

    std::cout << "\n=== 賦值後 (c1 = c2) ===" << std::endl;
    std::cout << "c1: "; c1.print();
    std::cout << "c2: "; c2.print();

    std::cout << "\n=== 日常實務：用 std::optional 取代 placement new ===" << std::endl;
    optionalDemo();

    return 0;
}

/*
**不行。** Copy-and-swap 無法解決 `const` 成員和引用成員的問題。

### 原因分析

Copy-and-swap 的標準寫法：

Config& operator=(Config other) {   // 傳值 → 呼叫拷貝建構（這步沒問題）
    swap(*this, other);              // ❌ 問題在這裡！
    return *this;
}

swap 的內部實作需要**交換每個成員**：

friend void swap(Config& a, Config& b) {
    std::swap(a.m_id, b.m_id);         // ❌ m_id 是 const，不能賦值
    std::swap(a.m_nameRef, b.m_nameRef); // ❌ 引用不能重新綁定
}

問題只是從 `operator=` **轉移**到了 `swap` — 根本原因完全一樣：

| 成員 | `operator=` 的問題 | `swap` 的問題 |
|------|-------------------|--------------|
| `const int m_id` | 不能賦值 | `std::swap` 內部也是賦值，一樣失敗 |
| `std::string& m_nameRef` | 不能重新綁定 | `std::swap` 無法交換引用的綁定目標 |

所以 copy-and-swap 並沒有繞過根本限制。除非你在 `swap` 裡面也用 placement new，但那就等於把同一個技巧換了個位置寫，沒有任何好處。

### 結論

要讓有 `const`/引用成員的類別支援賦值，只有三條路：

1. **Placement new**（如目前檔案中的做法）— 銷毀 + 重建
2. **改變成員設計** — `const` → non-const + getter，引用 → 指標
3. **不支援賦值** — 明確 `operator= = delete`，改用其他方式（如 `std::optional` 重新建構）
*/


// 編譯: g++ -std=c++17 -Wall -Wextra "第 29 課：拷貝賦值運算子（Copy Assignment Operator）3.cpp" -o lesson29_3

// 【重要但書 —— 請務必連同這段一起讀】
//
// 1. 編譯時 g++ 會發出一個警告（本機 g++ 15.2）：
//        warning: implicitly-declared 'constexpr Config::Config(const Config&)'
//                 is deprecated [-Wdeprecated-copy]
//    這不是筆誤，而是刻意保留的教材內容：類別自己宣告了 operator=，
//    卻仍依賴編譯器隱式生成的拷貝建構函式，正是三法則（第 30 課）要提醒的事。
//    加一行 Config(const Config&) = default; 即可消除。
//
// 2. 下方「賦值後」那兩行的輸出，在標準上並沒有保證。
//    Config 含有 const 成員與引用成員，依 [basic.life]/8，placement new 之後
//    原本的名字 c1（以及 operator= 裡的 this）不會自動指向新物件，
//    對它們的使用屬於未定義行為。
//    未定義行為不代表「一定出錯」——本機這份建置（g++ 15.2、無最佳化）
//    印出的就是重建後的正確值；但換編譯器或開啟最佳化都可能不同，
//    不可以把下面的輸出當成標準保證。要合法，C++17 起需改用
//    return *std::launder(this);，且呼叫端也必須透過 laundered 指標存取。
//
// 3. 相對地，「日常實務」那一段用的是 std::optional::emplace，
//    完全沒有這個問題，其輸出是有保證的。
//
// 【為何本檔沒有 LeetCode 範例】
//   placement new 與物件生命週期屬於語言規則與記憶體管理主題，
//   LeetCode 沒有任何一題在考它（設計題如 146 LRU Cache、707 Design
//   Linked List 都用得到動態配置，但不會用到就地重建）。
//   硬掛一題只會製造假關聯，故從缺。

// === 預期輸出 ===
// === 賦值前 ===
// c1: Config { id=1, name="Alice" }
// c2: Config { id=2, name="Bob" }
//
// === 賦值後 (c1 = c2) ===
// c1: Config { id=2, name="Bob" }
// c2: Config { id=2, name="Bob" }
//
// === 日常實務：用 std::optional 取代 placement new ===
//   尚未建立 session（disengaged）
//   session #1001 protocol=TLS1.3
//   session #1002 protocol=TLS1.3-resumed
//   尚未建立 session（disengaged）
