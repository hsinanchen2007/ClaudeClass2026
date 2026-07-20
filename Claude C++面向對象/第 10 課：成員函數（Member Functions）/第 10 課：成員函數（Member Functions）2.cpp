// =============================================================================
//  第 10 課：成員函數 2  —  參數／回傳值的四種組合，與「回傳 *this」的鏈式調用
// =============================================================================
//
// 【主題資訊 Information】
//   語法：  void f();            // 無參無回傳
//           void f(T);           // 有參無回傳
//           R    f();            // 無參有回傳
//           R    f(T, U);        // 有參有回傳
//           X&   f(T) { ...; return *this; }   // 回傳自身引用 → 可鏈式調用
//   標準：  C++98 起即有；本檔語法無版本相依。
//   標頭檔：<iostream>、<string>
//   複雜度：append/+= 平攤 O(附加長度)；find O(N*M) 最壞；substr O(len)。
//
// 【詳細解釋 Explanation】
//
// 【1. 參數與回傳值的四種組合，其實是在分「意圖」】
//   四種組合不是語法練習，而是在表達函式的角色：
//     void f()          → 命令（command）：改變狀態，如 clear()
//     void f(T)         → 帶參數的命令：append(suffix)
//     R    f() const    → 查詢（query）：只讀不改，如 length()
//     R    f(T)         → 帶參數的查詢：charAt(i)、contains(t)
//   這就是 Command-Query Separation 原則：**一個函式要嘛改狀態、要嘛回答問題，
//   不要兩者都做**。混在一起的函式（例如「取值順便清空」）最難測試也最容易誤用。
//
// 【2. 為什麼參數是 const string& 而不是 string】
//   append(const string& suffix) 用引用避免複製整個字串。加 const 有兩個作用：
//     (a) 向呼叫端保證「我不會改你的字串」；
//     (b) 讓它能接受 **臨時物件**（rvalue）—— 非 const 的 string& 綁不住
//         sh.append("Hello") 產生的臨時 string，會直接編譯錯誤。
//   這是 C++ 參數傳遞的預設選擇：唯讀的非平凡型別一律 const&。
//
// 【3. 回傳 *this 為什麼能鏈式調用】
//   appendChain 回傳型別是 StringHelper&（引用，不是值）。
//   `return *this;` 中 this 是指向當前物件的指標，*this 解引用得到物件本身，
//   綁到回傳的引用上 —— 於是 sh2.appendChain("A") 這個表達式的結果
//   **就是 sh2 自己**，可以再接 .appendChain("B")，如此串成一串。
//   ★ 關鍵：回傳型別必須是引用 StringHelper&。若寫成 StringHelper（回傳值），
//     每次呼叫都會複製一份新物件，後續的 .appendChain 改的是那份副本，
//     原物件不會被修改 —— 這是初學者最常見的鏈式調用失敗原因。
//
// 【4. charAt 的邊界檢查與「錯誤要怎麼回報」】
//   本檔 charAt 越界時印訊息並回傳 '\0'。這是教學上最簡單的做法，
//   但工程上有三個問題：(a) 函式庫不該擅自印東西到 stdout；
//   (b) '\0' 是合法字元，無法與「真的存了 '\0'」區分；
//   (c) 呼叫端可以完全忽略錯誤。實務選項是：丟例外（如 std::string::at）、
//   回傳 std::optional<char>（C++17），或用 assert 表達「這是呼叫端的 bug」。
//
// 【概念補充 Concept Deep Dive】
//   `text.length()` 回傳 std::string::size_type（無號）。本檔在比較時寫了
//   (int)text.length() 做顯式轉換，正是為了避免 `index < 0 || index >= text.length()`
//   這種 signed/unsigned 混用比較 —— 在該寫法下 index 會被隱式提升為無號數，
//   負的 index 會變成極大值，`index < 0` 雖仍成立，但 -Wsign-compare 會警告，
//   而且若少寫了 index < 0 那一半，負索引就會直接穿過檢查造成越界 UB。
//
//   鏈式調用還有一個常被忽略的細節：本檔 appendChain 沒有 const 版本。
//   若要讓 const 物件也能鏈式讀取，需要提供 const 成員函式並回傳 const X&。
//   標準庫的 operator<< 也是同一套設計（回傳 ostream&），
//   所以 cout << a << b 才能串起來。
//
// 【注意事項 Pay Attention】
//   1. 回傳 *this 的函式**絕不能回傳區域物件的引用**；回傳 *this 是安全的，
//      因為物件的生命週期由呼叫端持有。
//   2. length() 回傳 int 會在超長字串（> INT_MAX）時溢位；
//      正確型別是 std::string::size_type 或 std::size_t。本檔為教學簡化。
//   3. substring(start, len) 若 start 合法但 len 超過剩餘長度，
//      std::string::substr 會自動夾到結尾，**不會**丟例外；
//      但 start > size() 才會丟 std::out_of_range。本檔已先擋掉 start 越界。
//   4. contains() 這個名字在 C++23 起 std::string 已有標準成員 contains()；
//      本檔是自行實作，語意相同但別混淆版本。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】成員函數簽名設計與鏈式調用
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 鏈式調用（fluent interface）是怎麼做到的？回傳型別能不能寫成值？
//     答：讓成員函式回傳 *this，且回傳型別必須是**引用** X&。
//         回傳引用時整串操作的都是同一個物件；
//         若寫成回傳值 X，每一步都複製一份新物件，
//         原物件不會被改到，鏈式看似能編譯卻完全失效。
//     追問：那 operator= 為什麼也回傳引用？
//         → 同樣是為了支援 a = b = c 這種串接，並與內建型別語意一致。
//
// 🔥 Q2. 參數為什麼要寫 const string& 而不是 string 或 string&？
//     答：string 會複製一份，浪費；string&（非 const）綁不住臨時物件，
//         連 f("literal") 都編不過。const string& 兩個問題都解決：
//         零複製、又能接受 lvalue 與 rvalue。
//     追問：那什麼時候該用傳值 + std::move？
//         → 當函式**確定要保留一份**時（例如存進成員），
//           傳值可讓呼叫端用 move 免掉一次複製（sink parameter 慣例）。
//
// ⚠️ 陷阱. 把 appendChain 的回傳型別從 StringHelper& 改成 StringHelper，
//         sh2.appendChain("C++").appendChain(" is") 還會編譯成功嗎？結果是什麼？
//     答：**會編譯成功**，這正是危險之處。但每次呼叫都作用在前一次回傳的
//         臨時副本上，最後全部丟棄；sh2.text 只會拿到第一次的 "C++"。
//     為什麼會錯：多數人以為「編得過就代表語意對」，
//         而且鏈式寫法看起來完全一樣，錯誤只在執行結果才浮現。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <vector>
using namespace std;

class StringHelper {
public:
    string text = "";

    // 無參數、無返回值
    void clear() {
        text = "";
    }

    // 有參數、無返回值
    void append(const string& suffix) {
        text += suffix;
    }

    // 無參數、有返回值
    int length() {
        return text.length();
    }

    // 有參數、有返回值
    char charAt(int index) {
        if (index < 0 || index >= (int)text.length()) {
            cout << "錯誤：索引超出範圍" << endl;
            return '\0';
        }
        return text[index];
    }

    // 多個參數
    string substring(int start, int len) {
        if (start < 0 || start >= (int)text.length()) {
            return "";
        }
        return text.substr(start, len);
    }

    // 返回 bool
    bool contains(const string& target) {
        return text.find(target) != string::npos;
    }

    // 返回自身的引用（鏈式調用的基礎）, 這裡的返回類型是 StringHelper&，表示返回對象本身的引用
    // 這樣做的好處是可以實現鏈式調用，例如 sh.appendChain("Hello").appendChain(" World");
    // 在 appendChain 函數中，我們首先將傳入的 suffix 附加到 text 成員變量上，然後返回 *this，
    // 這裡的 *this 是一個指向當前對象的指標，通過解引用（*）我們獲得了對象本身，從而實現了鏈式調用。
    // 這種設計模式在許多 C++ 庫中都很常見，特別是在需要連續調用多個方法的情況下，可以使代碼更加簡潔和易讀。
    StringHelper& appendChain(const string& suffix) {
        text += suffix;
        return *this;  // 返回自己（this 指標會在第 26 課詳講）
    }
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1408. String Matching in an Array
//   題目：給一個字串陣列，回傳其中「是另一個字串的子字串」的所有字串。
//   為什麼用到本主題：直接就是 contains() 這個「有參數、回傳 bool」的查詢型
//         成員函數的應用 —— 對每一對 (i, j) 問「words[j] 含不含 words[i]」。
//   複雜度：O(n^2 * L)，n 為字串數、L 為平均長度。
// -----------------------------------------------------------------------------
vector<string> stringMatching(const vector<string>& words) {
    vector<string> result;
    for (size_t i = 0; i < words.size(); ++i) {
        StringHelper helper;
        for (size_t j = 0; j < words.size(); ++j) {
            if (i == j) continue;
            helper.text = words[j];              // 把「大字串」放進 helper
            if (helper.contains(words[i])) {     // 問：words[i] 是它的子字串嗎
                result.push_back(words[i]);
                break;                           // 找到一個就夠，避免重複加入
            }
        }
    }
    return result;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】用鏈式調用組裝 URL query string
//   情境：呼叫後端 API 前要組出 "?page=2&size=50&sort=created_at"。
//         用 fluent interface 寫成 qb.add("page","2").add("size","50")...
//         比一堆 += 字串拼接可讀得多，也是各家 HTTP client / ORM 的常見設計。
//   為什麼用到本主題：每個 add() 都 return *this 並回傳 QueryBuilder&，
//         這正是本檔 appendChain 的實務版本；同時示範
//         「唯讀查詢 build() const」與「改狀態 add()」的職責分離。
// -----------------------------------------------------------------------------
class QueryBuilder {
    string m_query;          // 已累積的 query（不含開頭的 '?'）

public:
    // mutator：回傳自身引用 → 可鏈式串接
    QueryBuilder& add(const string& key, const string& value) {
        if (key.empty()) return *this;           // 空 key 直接略過
        if (!m_query.empty()) m_query += '&';    // 第二項起才需要分隔符
        m_query += key;
        m_query += '=';
        m_query += value;
        return *this;
    }

    // 唯讀查詢：加 const，const 物件也能呼叫
    string build() const {
        return m_query.empty() ? string{} : "?" + m_query;
    }

    bool empty() const { return m_query.empty(); }
};

int main() {
    cout << "=== 基本：四種參數/回傳值組合 ===" << endl;
    StringHelper sh;

    sh.append("Hello");
    sh.append(", ");
    sh.append("World!");

    cout << "文字: " << sh.text << endl;
    cout << "長度: " << sh.length() << endl;
    cout << "第 0 個字元: " << sh.charAt(0) << endl;
    cout << "子字串(7, 5): " << sh.substring(7, 5) << endl;
    cout << "包含 \"World\": " << (sh.contains("World") ? "是" : "否") << endl;

    cout << "\n--- 鏈式調用 ---" << endl;
    StringHelper sh2;
    sh2.appendChain("C++").appendChain(" is").appendChain(" awesome!");
    cout << sh2.text << endl;

    cout << "\n=== 邊界：charAt 越界 ===" << endl;
    char bad = sh.charAt(999);                   // 會印錯誤訊息並回傳 '\0'
    cout << "回傳值是否為 '\\0': " << (bad == '\0' ? "是" : "否") << endl;

    cout << "\n=== LeetCode 1408. String Matching in an Array ===" << endl;
    vector<string> words{"mass", "as", "hero", "superhero"};
    for (const string& w : stringMatching(words)) cout << w << " ";
    cout << endl;   // as hero（"as" 在 "mass" 裡、"hero" 在 "superhero" 裡）

    cout << "\n=== 日常實務：鏈式組裝 URL query string ===" << endl;
    QueryBuilder qb;
    string url = qb.add("page", "2").add("size", "50").add("sort", "created_at").build();
    cout << "https://api.example.com/items" << url << endl;

    QueryBuilder emptyQb;
    cout << "沒有任何參數時: [" << emptyQb.build() << "]" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 10 課：成員函數（Member Functions）2.cpp" -o member2

// === 預期輸出 ===
// === 基本：四種參數/回傳值組合 ===
// 文字: Hello, World!
// 長度: 13
// 第 0 個字元: H
// 子字串(7, 5): World
// 包含 "World": 是
//
// --- 鏈式調用 ---
// C++ is awesome!
//
// === 邊界：charAt 越界 ===
// 錯誤：索引超出範圍
// 回傳值是否為 '\0': 是
//
// === LeetCode 1408. String Matching in an Array ===
// as hero
//
// === 日常實務：鏈式組裝 URL query string ===
// https://api.example.com/items?page=2&size=50&sort=created_at
// 沒有任何參數時: []
