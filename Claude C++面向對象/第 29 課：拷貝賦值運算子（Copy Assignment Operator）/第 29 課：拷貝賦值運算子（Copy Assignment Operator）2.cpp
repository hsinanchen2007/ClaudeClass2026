// =============================================================================
//  第 29 課：拷貝賦值運算子 2  —  什麼情況下 operator= 會被「隱式刪除」
// =============================================================================
//
// 【主題資訊 Information】
//   主題    ： 編譯器在什麼條件下「不會」幫你生成 copy assignment operator
//   標準版本： C++11 起正式以 "defined as deleted" 描述（C++98 時期的說法是
//             「隱式宣告但一經使用即 ill-formed」，結果相同、措辭不同）
//   關鍵條文： [class.copy.assign] —— 隱式定義的 copy assignment 是
//             「對每個非靜態成員逐一做 memberwise assignment」
//   標頭檔  ： <type_traits>（std::is_copy_assignable / is_copy_constructible）
//   本檔可以編譯：真正會失敗的那一行已被註解掉，並改用 static_assert 來
//             「證明」它確實被刪除 —— 這比留一行編不過的程式碼更有用。
//
// 【詳細解釋 Explanation】
//
// 【1. 隱式 operator= 到底做了什麼】
//   編譯器生成的拷貝賦值不是什麼魔法，它就是「對每個非靜態成員做一次賦值」：
//       Config& operator=(const Config& o) {
//           this->m_id     = o.m_id;        // ← 對 const int 做賦值
//           this->m_nameRef = o.m_nameRef;  // ← 對 reference 做「重新綁定」
//           return *this;
//       }
//   把這段展開來看，兩行都不成立，所以編譯器乾脆把整個 operator= 定義為
//   deleted。理解這件事的訣竅永遠是：把隱式版本手動展開，看哪一行寫不出來。
//
// 【2. const 成員：初始化可以，賦值不行】
//   const int m_id 只能在建構的「初始化列表」被初始化一次，之後它的值就是
//   const 的一部分。賦值 m_id = o.m_id 等於修改一個 const 物件，語意上直接
//   被禁止。注意這裡有個很多人搞混的分界：初始化 ≠ 賦值。
//   所以 const 成員不擋拷貝「建構」，只擋拷貝「賦值」。
//
// 【3. 引用成員：C++ 沒有「重新綁定引用」這個操作】
//   std::string& m_nameRef 一旦綁定就終身綁定。更危險的是，
//   m_nameRef = o.m_nameRef 這行語法上其實是「合法」的 —— 但它的意思是
//   「把 o 指向的那個 string 的內容，複製到我指向的那個 string 裡」，
//   也就是改到了「別人的變數」，而不是讓自己改指向別的物件。
//   標準選擇不採用這個語意（它會造成賦值意外修改外部物件），
//   於是含引用成員的類別，隱式 operator= 一律 deleted。
//
// 【4. 其他也會導致隱式刪除的情況（一併記起來）】
//   * 有成員或基底類別本身的 operator= 是 deleted 或不可存取（private）。
//   * 有成員是 const 的類別型別，或是不可賦值的類別型別。
//   * 基底類別的 operator= 有歧義（多重繼承常見）。
//   * 你自己宣告了 move assignment 或 move constructor —— 這會讓
//     copy assignment 被隱式「刪除」（不是「不生成」），這是五法則的核心。
//
// 【5. 三條可行的解法】
//   (A) 改變成員設計（本檔示範、也是首選）：
//       const int → int + getter 維持唯讀語意；
//       std::string& → std::string*，指標可以重新指向。
//   (B) placement new：先解構再就地重建（見同課第 3 個檔案），
//       威力大但風險高，建構丟例外時物件會停在「已銷毀」狀態。
//   (C) 誠實地放棄：明確寫 operator=(const Config&) = delete;
//       讓呼叫端改用 std::optional<Config> 的 emplace 重新建構。
//       「不可賦值」對某些型別（如帶身分的資源持有者）本來就是正確的設計。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 「刪除」不等於「不存在」
//   deleted function 仍然參與 overload resolution。它會被選中，然後才報錯。
//   這是刻意設計的：如果 deleted 函式直接消失，呼叫端可能悄悄改去匹配某個
//   會發生隱式轉換的其他重載，錯得無聲無息。先選中、再拒絕，錯誤訊息才精準。
//
// (B) 用 type traits 把「編譯期性質」變成可驗證的斷言
//   std::is_copy_assignable_v<T> / std::is_copy_constructible_v<T> 讓你不必
//   靠「試著編編看」來確認。本檔用 static_assert 把結論釘死：
//   Config 不可拷貝賦值、但可以拷貝建構。這兩個 static_assert 都通過，
//   等於編譯器親自替這份教材背書。
//
// (C) 為什麼 const 成員擋賦值卻不擋建構
//   因為建構走的是「初始化」（initialization），const 物件的值就是在這一刻
//   決定的；賦值走的是「修改既有物件」，而 const 的定義就是不可修改。
//   同一個成員在生命週期的不同時點，受到的規則本來就不同。
//
// 【注意事項 Pay Attention】
//   1. 類別裡放 const 成員，代價往往被低估：整個類別立刻失去可賦值性，
//      連帶不能放進 std::vector 後做 erase/sort（那些演算法要靠 move 賦值）。
//      想表達唯讀，優先用 private 成員 + 只提供 getter，而不是 const 成員。
//   2. 引用成員還會額外帶來生命週期風險：被引用的物件若先死，成員就懸空。
//      成員想「指向別人」時，預設選擇應該是指標（或 std::reference_wrapper，
//      它是可賦值的，正是為了解決這個問題而存在）。
//   3. 拷貝建構沒被刪除，不代表這個類別安全。Config 可以拷貝建構，
//      拷出來的副本仍然引用同一個外部 string —— 淺層的別名關係還在。
//   4. 錯誤訊息會寫 "use of deleted function"，不會寫「你有 const 成員」。
//      看到這句話，第一件事就是把成員清單拿出來逐一檢查。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】隱式刪除的拷貝賦值運算子
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 哪些成員會讓編譯器不生成（刪除）拷貝賦值運算子？
//     答：最常見的兩個是 const 成員與引用成員。前者不能被賦值，後者不能被
//         重新綁定。另外還有：成員／基底的 operator= 本身是 deleted 或不可
//         存取、基底 operator= 有歧義、以及自己宣告了 move 操作。
//         判斷方法一律相同：把「逐成員賦值」展開，看哪一行寫不出來。
//     追問：那拷貝建構函式也會一起被刪除嗎？
//         → 不會。const 與引用都能被「初始化」，只是不能被「賦值」。
//           本檔的 static_assert 就同時證明了這兩件事。
//
// 🔥 Q2. 為什麼 C++ 不乾脆讓 reference 成員的賦值等於「重新綁定」？
//     答：因為 m_ref = other.m_ref 這個語法在 C++ 中已經有明確意義 ——
//         對被引用的物件賦值。若讓它在成員賦值時改為重新綁定，同一段語法
//         在不同位置會有兩種意義；而且逐成員賦值會變成「悄悄改到外部變數」，
//         副作用外溢。標準選擇維持一致性，代價就是這個類別不可賦值。
//     追問：那想要「可重新綁定的引用」該用什麼？
//         → std::reference_wrapper<T>（<functional>）。它是可拷貝賦值的，
//           賦值語意就是重新指向，且能隱式轉回 T&。
//
// ⚠️ 陷阱 1. 「編譯器沒生成 operator=，所以呼叫時會找不到函式」——錯在哪？
//     答：它其實有被「宣告」出來，只是定義為 deleted。deleted function 照樣
//         參與 overload resolution，會先被選中、再產生
//         "use of deleted function" 的錯誤。
//     為什麼會錯：把 deleted 想成「這個函式不存在」。它存在，
//         而且刻意存在 —— 為的是攔截呼叫，避免悄悄退而求其次去匹配別的重載，
//         造成無聲的錯誤行為。
//
// ⚠️ 陷阱 2. 「把成員宣告成 const 可以讓類別更安全」——代價是什麼？
//     答：整個類別立刻失去可賦值性。它不能被 std::sort 重排、不能被
//         std::vector::erase 搬移、不能整批覆寫，因為那些操作全都依賴
//         move/copy 賦值。想表達唯讀，正確做法是 private 成員 + getter。
//     為什麼會錯：直覺認為「const 只是加一層保護，不會有壞處」。
//         但 const 成員改變的是型別本身的能力，影響會擴散到所有容器演算法。
// ═══════════════════════════════════════════════════════════════════════════

#include <string>
#include <iostream>

class Config {
private:
    const int m_id;          // const 成員
    std::string& m_nameRef;  // 引用成員

public:
    Config(int id, std::string& name) : m_id(id), m_nameRef(name) {}
    // 編譯器無法生成拷貝賦值運算子！
    // 因為 const 成員不能被重新賦值，引用不能被重新綁定
};

#include <type_traits>
#include <vector>

// -----------------------------------------------------------------------------
// 用 static_assert 把結論釘死：不必「試著編編看」，讓編譯器直接背書。
//   兩個斷言都通過，才代表本檔的說明與這台編譯器的實際行為一致。
// -----------------------------------------------------------------------------
static_assert(!std::is_copy_assignable<Config>::value,
              "Config 應該不可拷貝賦值（const 成員 + 引用成員）");
static_assert(std::is_copy_constructible<Config>::value,
              "Config 仍然可以拷貝建構：const 與引用都能被『初始化』");

// -----------------------------------------------------------------------------
// 【日常實務範例】把「不可賦值」的類別改造成可放進容器的版本
//   情境：每個 worker thread 需要一個 RequestLogger，它要記住服務名稱，
//         並把訊息寫到某個既有的輸出串流。第一版很自然地寫成
//             const std::string m_serviceName;   // 建好就不該再改
//             std::ostream&     m_out;           // 不擁有，只是指向
//         結果整個型別不可賦值：無法放進 std::vector 之後重新指派、
//         無法用 std::sort 排序、連 v.erase(it) 都編不過（erase 要搬移元素）。
//   修法：唯讀語意改用 private + getter 表達，「指向別人」改用指標。
//         這樣既保留原本的設計意圖，又把可賦值性拿回來。
// -----------------------------------------------------------------------------
class RequestLogger {
private:
    std::string   m_serviceName;   // 原本是 const → 改成 private + getter
    std::ostream* m_out;           // 原本是 ostream& → 改成指標，可重新指向

public:
    RequestLogger(std::string serviceName, std::ostream& out)
        : m_serviceName(std::move(serviceName)), m_out(&out) {}

    // 唯讀語意由介面保證，而不是由成員的 const 保證
    const std::string& serviceName() const { return m_serviceName; }

    void log(const std::string& msg) const {
        *m_out << "  [" << m_serviceName << "] " << msg << "\n";
    }

    // 沒有自訂 operator=：編譯器生成的逐成員賦值現在完全可用
};

static_assert(std::is_copy_assignable<RequestLogger>::value,
              "改造後的 RequestLogger 必須可拷貝賦值，才能放進容器");

void containerDemo() {
    std::vector<RequestLogger> loggers;
    loggers.emplace_back("auth",    std::cout);
    loggers.emplace_back("payment", std::cout);
    loggers.emplace_back("search",  std::cout);

    std::cout << "  初始 logger 數量：" << loggers.size() << "\n";
    for (const auto& lg : loggers) lg.log("service started");

    // 這一行就是原始設計做不到的事：erase 需要把後面的元素往前「賦值」搬移
    loggers.erase(loggers.begin() + 1);
    std::cout << "  erase 之後數量：" << loggers.size() << "\n";
    for (const auto& lg : loggers) lg.log("still alive");

    // 整個元素重新指派 —— 同樣依賴 copy assignment
    loggers[0] = RequestLogger("auth-v2", std::cout);
    std::cout << "  重新指派 loggers[0] 之後：" << loggers[0].serviceName() << "\n";
}

int main() {
    std::string name1 = "Alice";
    std::string name2 = "Bob";
    Config c1(1, name1);
    Config c2(2, name2);

    std::cout << "=== 1. 編譯期證明：Config 的能力 ===\n";
    std::cout << "  is_copy_constructible<Config> = "
              << std::boolalpha << std::is_copy_constructible<Config>::value
              << "（const 與引用都能被『初始化』）\n";
    std::cout << "  is_copy_assignable<Config>    = "
              << std::is_copy_assignable<Config>::value
              << "（const 不能賦值、引用不能重新綁定 → 隱式刪除）\n";

    // 拷貝「建構」是可以的 —— 但要注意副本仍引用同一個外部 string
    Config c3 = c1;
    (void)c3;
    std::cout << "  Config c3 = c1;  → 合法（走的是拷貝建構）\n";

    std::cout << "\n=== 2. 改造後：RequestLogger 可放進容器 ===\n";
    std::cout << "  is_copy_assignable<RequestLogger> = "
              << std::is_copy_assignable<RequestLogger>::value << "\n";
    containerDemo();

    
    // ❌ 編譯錯誤！拷貝賦值被隱式刪除
    // c1 = c2;  
    
    return 0;
}


/*
Read [](file:///d%3A/git/ClaudeClass/Claude%20C%2B%2B%E9%9D%A2%E5%90%91%E5%B0%8D%E8%B1%A1/%E7%AC%AC%2029%20%E8%AA%B2%EF%BC%9A%E6%8B%B7%E8%B2%9D%E8%B3%A6%E5%80%BC%E9%81%8B%E7%AE%97%E5%AD%90%EF%BC%88Copy%20Assignment%20Operator%EF%BC%89/%E7%AC%AC%2029%20%E8%AA%B2%EF%BC%9A%E6%8B%B7%E8%B2%9D%E8%B3%A6%E5%80%BC%E9%81%8B%E7%AE%97%E5%AD%90%EF%BC%88Copy%20Assignment%20Operator%EF%BC%892.cpp)

## 詳細原因

第 23 行 `c1 = c2` 編譯失敗，因為編譯器**無法自動生成**拷貝賦值運算子（`operator=`），原因有兩個：

1. **`const int m_id`** — `const` 成員在初始化後不能被重新賦值，所以編譯器生成的 `operator=` 無法執行 `this->m_id = other.m_id`
2. **`std::string& m_nameRef`** — 引用一旦綁定就不能重新綁定到另一個物件，所以編譯器生成的 `operator=` 無法執行 `this->m_nameRef = other.m_nameRef`（語意上會變成「對引用目標賦值」而非「重新綁定」）

當類別含有 `const` 成員或引用成員時，編譯器會**隱式刪除** (`= delete`) 拷貝賦值運算子。

修復方式：將 `const` 成員改為非 `const`（透過 getter 保持唯讀語意），將引用改為指標（可以重新指向）。 

Replacing 25 lines with 93 lines in [](file:///d%3A/git/ClaudeClass/Claude%20C%2B%2B%E9%9D%A2%E5%90%91%E5%B0%8D%E8%B1%A1/%E7%AC%AC%2029%20%E8%AA%B2%EF%BC%9A%E6%8B%B7%E8%B2%9D%E8%B3%A6%E5%80%BC%E9%81%8B%E7%AE%97%E5%AD%90%EF%BC%88Copy%20Assignment%20Operator%EF%BC%89/%E7%AC%AC%2029%20%E8%AA%B2%EF%BC%9A%E6%8B%B7%E8%B2%9D%E8%B3%A6%E5%80%BC%E9%81%8B%E7%AE%97%E5%AD%90%EF%BC%88Copy%20Assignment%20Operator%EF%BC%892.cpp)

Now let me verify it compiles: 

Ran terminal command: cd "D:\git\ClaudeClass\Claude C++面向對象\第 29 課：拷貝賦值運算子（Copy Assignment Operator）"; cl.exe /Zi /EHsc /std:c++20 /nologo "/FeD:\git\ClaudeClass\Claude C++面向對象\第 29 課：拷貝賦值運算子（Copy Assignment Operator）\第 29 課：拷貝賦值運算子（Copy Assignment Operator）2.exe" "D:\git\ClaudeClass\Claude C++面向對象\第 29 課：拷貝賦值運算子（Copy Assignment Operator）\第 29 課：拷貝賦值運算子（Copy Assignment Operator）2.cpp"

Created task `Build lesson 29-2`

修改完成。以下是修復的總結：

### 編譯錯誤原因

原始類別有兩個「不可賦值」的成員：

| 成員 | 問題 |
|------|------|
| `const int m_id` | `const` 變數初始化後**不能再被賦值** |
| `std::string& m_nameRef` | 引用綁定後**不能重新綁定**到另一個物件 |

編譯器嘗試自動生成 `operator=` 時發現這兩個成員都無法處理，因此將 `operator=` **隱式標記為 `= delete`**，導致 `c1 = c2` 編譯失敗。

### 修復方式

1. **`const int m_id`** → **`int m_id`**：移除 `const`，改用 `private` + `getter` 維持外部唯讀語意
2. **`std::string& m_nameRef`** → **`std::string* m_namePtr`**：引用改為指標，指標可以重新指向不同物件
3. 顯式寫出 `operator=`，加上自我賦值防護和詳細註釋

Made changes.
*/

// 編譯: g++ -std=c++17 -Wall -Wextra "第 29 課：拷貝賦值運算子（Copy Assignment Operator）2.cpp" -o lesson29_2

// 【為何本檔沒有 LeetCode 範例】
//   本檔主題是「編譯器在什麼條件下刪除 operator=」，屬於型別系統規則，
//   不是演算法。LeetCode 的設計題（146 LRU Cache、155 Min Stack、
//   707 Design Linked List…）評測時都只建立單一實例、從不拷貝賦值，
//   硬掛一題只會製造「看起來相關、實際無關」的干擾，故從缺。
//   真正對應本主題的實戰場景是「型別能不能放進容器」，
//   已由上面的 RequestLogger 實務範例示範。

// === 預期輸出 ===
// === 1. 編譯期證明：Config 的能力 ===
//   is_copy_constructible<Config> = true（const 與引用都能被『初始化』）
//   is_copy_assignable<Config>    = false（const 不能賦值、引用不能重新綁定 → 隱式刪除）
//   Config c3 = c1;  → 合法（走的是拷貝建構）
//
// === 2. 改造後：RequestLogger 可放進容器 ===
//   is_copy_assignable<RequestLogger> = true
//   初始 logger 數量：3
//   [auth] service started
//   [payment] service started
//   [search] service started
//   erase 之後數量：2
//   [auth] still alive
//   [search] still alive
//   重新指派 loggers[0] 之後：auth-v2
