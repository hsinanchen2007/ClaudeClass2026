// =============================================================================
//  第 26 課 -3  —  return *this：鏈式呼叫（Fluent Interface）的原理
// =============================================================================
//
// 【主題資訊 Information】
//   語法       ： ClassName& method(...) { ...; return *this; }
//   this 的型別： 在非 const 成員函式中是 ClassName* const
//                 在 const 成員函式中是 const ClassName* const
//   *this 的型別： ClassName&（左值）—— 解參考 this 得到物件本身
//   標準版本   ： this 指標自 C++98 即存在；本檔語法無版本限制
//   標頭檔     ： <string>（本檔的 to_string 需要）
//   複雜度     ： 回傳參考是 O(1)，不產生任何拷貝
//
// 【詳細解釋 Explanation】
//
// 【1. 鏈式呼叫成立的唯一條件：回傳的是「參考」而不是「值」】
//   query.from("players").where("lv > 10") 之所以能接下去，
//   是因為 from() 回傳了 QueryBuilder&，
//   而 .where(...) 就作用在這個參考所指的同一個物件上。
//   關鍵在於「同一個」：
//       QueryBuilder& from(...) { ...; return *this; }   // ✅ 同一個物件
//       QueryBuilder  from(...) { ...; return *this; }   // ❌ 每次都拷貝一份
//   若寫成回傳值（少一個 &），語法仍然合法、也編得過，
//   但每一環都在對「新的暫時副本」設定，
//   最後 build() 讀到的是最後一份副本 —— 前面的設定看似生效其實是巧合，
//   而且每一步都多一次拷貝。這是初學者最常見的一個字之差。
//
// 【2. this 是什麼，*this 又是什麼】
//   每個非靜態成員函式都隱含收到一個 this 指標，指向呼叫它的那個物件。
//   它的型別是 ClassName* const —— 指標本身是 const（不能讓 this 改指別人），
//   但指向的物件可以修改（除非該成員函式標了 const）。
//   *this 就是解參考它，得到物件本身，型別是 ClassName&，是個左值。
//   所以 return *this; 回傳的是「呼叫者自己」的參考，而不是副本。
//
// 【3. 為什麼 QueryBuilder() 這個暫時物件在整條鏈中都還活著】
//   main 裡的寫法是對一個「暫時物件」起手：
//       string sql = QueryBuilder().from("players").where(...)....build();
//   會不會鏈到一半暫時物件就被銷毀？不會。
//   標準規定暫時物件存活到「完整運算式（full-expression）」結束為止，
//   也就是這一整行的分號。整條鏈都在同一個完整運算式裡，
//   所以每一環拿到的參考都指向仍然活著的物件。
//   暫時物件在分號處才解構，而那時 build() 已經把結果「以值回傳」出來了。
//
// 【4. 危險的一步之差：把結果綁成參考】
//   上面說的安全，前提是 build() 以「值」回傳 string。
//   若寫成下面這樣就是懸空參考：
//       const string& bad = QueryBuilder().from("t").build();  // build() 回傳值 → 有生命週期延長，仍安全
//       const string& worse = QueryBuilder().from("t").tableRef();  // 回傳成員參考 → 分號後即懸空
//   前者因為「以 const 參考繫結到暫時物件」會觸發生命週期延長，仍然安全；
//   後者回傳的是「暫時物件的成員」的參考，生命週期延長規則不適用於它，
//   暫時物件在分號處解構後，那個參考就懸空了 —— 之後讀取是未定義行為。
//   守則：builder 的終結函式（build/str/toString）一律以「值」回傳。
//
// 【概念補充 Concept Deep Dive】
//
// (A) this 不是物件的一部分，也不佔物件的空間
//   sizeof(QueryBuilder) 不包含 this。它是呼叫成員函式時傳入的隱藏參數
//   （在常見 ABI 中透過暫存器傳遞，例如 System V AMD64 用 rdi）。
//   成員函式在機器碼層次其實就是「多一個物件位址參數的普通函式」，
//   所謂「物件呼叫方法」只是語法上的包裝。
//
// (B) const 成員函式裡的 this
//   標了 const 的成員函式（本檔的 build()）中，
//   this 的型別變成 const QueryBuilder* const，
//   因此 *this 是 const QueryBuilder&，不能用來修改成員。
//   這也是為什麼 build() 可以對 const 物件呼叫，而 from() 不行。
//
// (C) 進階：ref-qualifier 可以區分「對左值」與「對暫時物件」呼叫
//   C++11 起成員函式可以加 & 或 && 限定：
//       string build() const &  { return compose(); }              // 對具名物件
//       string build()       && { return std::move(compose()); }   // 對暫時物件，可搬走內部緩衝
//   標準庫的 builder 常用這招在暫時物件上直接把內部字串搬出來，省一次拷貝。
//   本檔為了聚焦在 return *this 的基本原理，沒有使用 ref-qualifier。
//
// (D) 本檔沿用原始碼的 using namespace std;
//   這在教學小檔可以接受，但正式專案（尤其標頭檔）應避免 ——
//   它把整個 std 拉進全域，容易與自訂名稱衝突（例如自己寫的 count、distance）。
//   這裡保留原樣以免偏離本課主題，但讀者不該把它當成風格範本。
//
// 【注意事項 Pay Attention】
//   1. 鏈式呼叫必須回傳「參考」（ClassName&）。少寫一個 & 會變成
//      每步都在拷貝新副本，語法照樣通過，是最常見的一個字之差。
//   2. const 成員函式不能 return *this 給非 const 的鏈（型別是 const T&）。
//      因此鏈上的設定函式都不能標 const。
//   3. 暫時物件存活到完整運算式（分號）結束，所以整條鏈是安全的；
//      但不要把「暫時物件的成員參考」留到分號之後使用。
//   4. builder 的終結函式請以「值」回傳，不要回傳內部成員的參考。
//   5. 回傳 *this 不會有拷貝成本；回傳 *this 的「值」版本則每步都有一次拷貝。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】return *this 與鏈式呼叫
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 鏈式呼叫（Fluent Interface）是怎麼運作的？回傳型別為什麼一定要是參考？
//     答：每個設定函式做完事後 return *this;，
//         回傳型別宣告為 ClassName&，代表回傳的是「呼叫者自己」的參考，
//         所以下一個 . 就作用在同一個物件上。
//         若回傳型別少寫一個 &（變成回傳值），每一環都會產生新的副本，
//         鏈上的設定作用在不同物件上，而且每步多一次拷貝。
//     追問：那 this 的型別到底是什麼？
//         → 非 const 成員函式中是 ClassName* const（指標本身 const）；
//           const 成員函式中是 const ClassName* const，
//           所以 *this 變成 const ClassName&，不能拿來接非 const 的鏈。
//
// 🔥 Q2. QueryBuilder().from(...).where(...).build() 這種對「暫時物件」起手的
//        寫法，會不會鏈到一半物件就被銷毀？
//     答：不會。標準規定暫時物件存活到「完整運算式」結束，
//         也就是這一整行的分號為止。整條鏈都在同一個完整運算式內，
//         每一環拿到的參考都指向仍然存活的物件。
//     追問：那什麼情況下會出事？
//         → 當終結函式回傳的是「內部成員的參考」而不是值。
//           暫時物件在分號處解構，那個參考隨即懸空，後續讀取是未定義行為。
//           所以 build() 必須以值回傳。
//
// ⚠️ 陷阱. 「把回傳型別從 QueryBuilder& 改成 QueryBuilder，反正都是 return *this，
//           編譯也過，行為應該一樣。」
//     答：完全不一樣。回傳值會呼叫拷貝建構函數產生一個新物件，
//         鏈上每一環都在設定「不同的暫時副本」。
//         最終 build() 讀到的只是最後一份副本的狀態；
//         看起來有時「好像也對」，是因為每份副本都從前一份拷貝而來，
//         但每一步都多付一次完整的深拷貝成本，而且語義已經不是原本的物件了。
//     為什麼會錯：把 return *this 當成關鍵，
//         但真正決定行為的是「函式的回傳型別」——
//         *this 是左值，回傳型別是值時它就會被拷貝，是參考時才不會。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】—— 本檔從缺，理由如下
//   鏈式呼叫是 API 介面設計手法（builder / fluent interface），
//   解決的是「可讀性與參數組合爆炸」的問題，不是演算法問題。
//   LeetCode 的設計類題目（146. LRU Cache、155. Min Stack、
//   1603. Design Parking System）都是由評測系統逐一呼叫指定方法，
//   方法簽名由題目固定、不能改成回傳 *this，
//   因此完全用不到本主題。硬掛題號只會誤導，故從缺。
//   本檔改以「SQL 查詢建構器」與「結構化 log 行建構器」兩個
//   真實會寫到的場景呈現。
//
// -----------------------------------------------------------------------------

#include <iostream>
#include <string>
#include <vector>
#include <utility>
using namespace std;

// -----------------------------------------------------------------------------
// 【日常實務範例 1】SQL 查詢建構器
//   情境：後端要依使用者選的篩選條件動態組 SQL。
//         條件可有可無、可重複（多個 WHERE），
//         若用「一個吃 5 個參數的函式」會出現大量預設值與難讀的呼叫端。
//   為什麼用到本主題：每個設定函式 return *this，
//         呼叫端就能只寫「這次真正需要的條件」，讀起來像在描述需求。
// -----------------------------------------------------------------------------
class QueryBuilder {
private:
    string table_;
    string conditions_;
    string orderBy_;
    int limit_;

public:
    QueryBuilder() : limit_(0) {}

    // 每個函數返回 *this（自身的引用）
    // ★ 回傳型別是 QueryBuilder&（有 &），所以整條鏈都在操作同一個物件。
    QueryBuilder& from(const string& table) {
        table_ = table;
        return *this;    // *this 解引用 this 指標，得到對象本身
    }

    QueryBuilder& where(const string& condition) {
        if (!conditions_.empty()) conditions_ += " AND ";
        conditions_ += condition;
        return *this;
    }

    QueryBuilder& orderBy(const string& field) {
        orderBy_ = field;
        return *this;
    }

    QueryBuilder& limit(int n) {
        limit_ = (n > 0) ? n : 0;
        return *this;
    }

    // 終結函式：以「值」回傳。
    // ★ 千萬不要改成回傳 const string&（內部成員的參考）——
    //   對暫時物件起手的鏈，分號之後那個參考就懸空了。
    string build() const {
        string sql = "SELECT * FROM " + table_;
        if (!conditions_.empty()) sql += " WHERE " + conditions_;
        if (!orderBy_.empty()) sql += " ORDER BY " + orderBy_;
        if (limit_ > 0) sql += " LIMIT " + to_string(limit_);
        return sql;
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例 2】結構化 log 行建構器
//   情境：現代服務不再寫 "user login failed" 這種純文字 log，
//         而是輸出帶欄位的結構化格式（level、event 加上任意 key=value），
//         方便 Loki／Elasticsearch 之類的系統索引與查詢。
//   為什麼用到本主題：欄位數量不固定，鏈式呼叫讓呼叫端
//         「有幾個欄位就串幾次」，不必為每種組合準備一個多載。
//   對照組：故意提供 addFieldByValue()，它回傳「值」而不是參考，
//         用來實測「少一個 & 」會發生什麼事。
// -----------------------------------------------------------------------------
class LogLine {
private:
    string level_;
    string event_;
    vector<pair<string, string>> fields_;

public:
    LogLine() : level_("INFO") {}

    LogLine& level(const string& lv) {
        level_ = lv;
        return *this;
    }

    LogLine& event(const string& ev) {
        event_ = ev;
        return *this;
    }

    // ✅ 正確：回傳參考，串接時操作同一個物件
    LogLine& field(const string& key, const string& value) {
        fields_.emplace_back(key, value);
        return *this;
    }

    // ❌ 對照組：回傳「值」。語法合法、編譯通過，但每一環都在改新的副本。
    LogLine addFieldByValue(const string& key, const string& value) {
        fields_.emplace_back(key, value);
        return *this;    // 這裡會呼叫拷貝建構函數，產生一份副本回傳
    }

    string str() const {
        string out = "level=" + level_ + " event=" + event_;
        for (const auto& kv : fields_) {
            out += " " + kv.first + "=" + kv.second;
        }
        return out;
    }

    size_t fieldCount() const { return fields_.size(); }
};

int main() {
    cout << "=== 場景二：鏈式調用 ===" << endl;

    // 鏈式調用的原理：
    // query.from("players")  → 返回 query 自身的引用
    //       .where("lv > 10")→ 在返回的引用上繼續調用 → 返回 query 引用
    //       .orderBy("lv")   → 繼續...
    //       .limit(20)       → 繼續...
    //       .build()         → 最後產出 SQL
    //
    // 整條鏈都在同一個完整運算式（到分號為止）內，
    // 所以 QueryBuilder() 這個暫時物件全程都還活著。

    string sql = QueryBuilder()
        .from("players")
        .where("level > 10")
        .where("hp > 0")
        .orderBy("level DESC")
        .limit(20)
        .build();

    cout << "  " << sql << endl;

    cout << "\n--- 另一個查詢 ---" << endl;
    string sql2 = QueryBuilder()
        .from("items")
        .where("rarity = 'legendary'")
        .where("price < 10000")
        .limit(5)
        .build();

    cout << "  " << sql2 << endl;

    cout << "\n=== 實務 2：結構化 log 行 ===" << endl;
    string line = LogLine()
        .level("WARN")
        .event("login_failed")
        .field("user", "hsinan")
        .field("ip", "192.168.1.20")
        .field("attempts", "3")
        .str();
    cout << "  " << line << endl;

    cout << "\n=== 少一個 & 會怎樣：回傳參考 vs 回傳值 ===" << endl;
    LogLine good;
    good.field("a", "1").field("b", "2").field("c", "3");
    cout << "  回傳參考 → good 的欄位數 = " << good.fieldCount()
         << "（三次都作用在 good 身上）" << endl;

    LogLine bad;
    bad.addFieldByValue("a", "1").addFieldByValue("b", "2").addFieldByValue("c", "3");
    cout << "  回傳值   → bad  的欄位數 = " << bad.fieldCount()
         << "（只有第一次作用在 bad，後兩次改到暫時副本上了）" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -Wreorder "第 26 課：this 指標3.cpp" -o lesson26c

// ── 輸出說明（讀之前先看這裡）────────────────────────────────────────────
// * 本檔輸出完全決定性：純字串組裝與計數，沒有位址、沒有耗時、沒有執行緒
//   （本機實測連跑 5 次逐位元組相同）。
// * 最後一段是本課的關鍵證據，請仔細對照：
//       回傳參考（LogLine&）        → 三次 field() 都作用在 good → 欄位數 3
//       回傳值  （LogLine）         → 只有第一次作用在 bad       → 欄位數 1
//   後兩次呼叫作用在「上一次回傳的暫時副本」上，
//   那些副本在分號處就被銷毀了，所以 bad 完全沒收到。
//   兩者的差別只有回傳型別上的一個 & —— 而且都能通過編譯，
//   不會有任何錯誤或警告提醒你。
// * SQL 那兩段示範對「暫時物件」起手的鏈：QueryBuilder() 產生的暫時物件
//   存活到該行分號為止，所以整條鏈安全；build() 以值回傳，
//   結果被複製／搬移進 sql 之後才輪到暫時物件解構。
// ─────────────────────────────────────────────────────────────────────────

// === 預期輸出 ===
// === 場景二：鏈式調用 ===
//   SELECT * FROM players WHERE level > 10 AND hp > 0 ORDER BY level DESC LIMIT 20
//
// --- 另一個查詢 ---
//   SELECT * FROM items WHERE rarity = 'legendary' AND price < 10000 LIMIT 5
//
// === 實務 2：結構化 log 行 ===
//   level=WARN event=login_failed user=hsinan ip=192.168.1.20 attempts=3
//
// === 少一個 & 會怎樣：回傳參考 vs 回傳值 ===
//   回傳參考 → good 的欄位數 = 3（三次都作用在 good 身上）
//   回傳值   → bad  的欄位數 = 1（只有第一次作用在 bad，後兩次改到暫時副本上了）
