// =============================================================================
//  第 15 課：帶參數的建構函數 4  —  用 this-> 解決參數與成員同名
// =============================================================================
//
// 【主題資訊 Information】
//   語法：  this->member = param;
//   this 的型別：在非 const 成員函數中是 Class* const
//                在 const 成員函數中是 const Class* const
//   標準版本：C++98 起即有
//   複雜度：O(1)，this-> 不產生任何額外執行期成本
//   標頭檔：<string>
//
// 【詳細解釋 Explanation】
//
// 【1. this 是什麼】
//   每個非靜態成員函數被呼叫時，編譯器都會偷偷傳進一個指向「當前物件」的
//   指標，名字叫 this。你寫的
//       void print() const { cout << name; }
//   編譯器實際處理成類似
//       void print(const Solution1* const this) { cout << this->name; }
//   所以類別內直接寫 name 其實是 this->name 的簡寫——當名稱沒有被遮蔽時。
//
// 【2. 為什麼在這裡「必須」寫出 this->】
//   建構函數 Solution1(string name, int age) 的參數 name 遮蔽了成員 name。
//   在函數體內單寫 name，名稱查找會先在函數作用域找到參數，成員被蓋掉。
//   而 this->name 明確指定「到當前物件裡找成員 name」，完全繞過名稱遮蔽——
//   因為 -> 右邊的名字只會在類別作用域查找，不會找到區域變數或參數。
//   所以 this->name = name; 意思是「成員 = 參數」，正是我們要的。
//
// 【3. this-> 有沒有效能成本】
//   沒有。成員存取本來就是透過 this 指標做位移定址（例如 mov [rdi+8], eax），
//   寫不寫 this-> 產生的機器碼完全一樣。this-> 純粹是給編譯器的名稱查找指示，
//   不是執行期的動作。
//
// 【4. 這個寫法的優點與缺點】
//   優點：參數可以用最自然的名字（name 就叫 name，不用叫 inName 或 _name），
//         呼叫端看到的介面最好讀；不需要團隊統一命名前綴。
//   缺點：遮蔽本身還在。若函數體很長，後面有人不小心寫了裸的 name，
//         他拿到的是參數而不是成員，而且編譯器預設不會警告。
//   因此比較穩健的做法是「消除遮蔽」（改參數名或加成員前綴），
//   或直接用初始化列表（第 16 課），把賦值變成初始化。
//
// 【概念補充 Concept Deep Dive】
//
//   ● this 是 prvalue，不能被賦值
//     你不能寫 this = &other;。this 的型別是 Class* const（指標本身是 const），
//     這保證了「成員函數執行期間，當前物件不會被換掉」。
//
//   ● const 成員函數裡的 this
//     本檔的 print() const 中，this 的型別是 const Solution1* const，
//     所以透過它只能讀不能寫。這就是 const 成員函數不能修改成員的底層機制。
//
//   ● 在 template base class 的情境下 this-> 不只是風格，而是必需
//     當你繼承一個 template base class，base 的成員屬於「相依名稱」，
//     編譯器在兩階段名稱查找的第一階段看不到它們，必須寫 this->member
//     （或 Base<T>::member）才找得到。那是 this-> 真正無可取代的場合。
//
//   ● 賦值 vs 初始化
//     本檔在函數體內用 =，代表成員 name 先被預設建構成空字串，再被 operator=
//     覆蓋，共兩步。改成初始化列表 : name(name), age(age) 只要一步，
//     而且初始化列表的括號外一定是成員，同名也不衝突，連 this-> 都省了。
//
// 【注意事項 Pay Attention】
//   1. this-> 只解決「這一行」的遮蔽，不解決遮蔽本身；同一個函數的其他地方
//      仍可能誤用參數。
//   2. 靜態成員函數沒有 this，因為它不屬於任何物件。
//   3. 不要在建構函數裡把 this 存起來給別人用——物件此時還沒建構完成，
//      別人拿去呼叫虛擬函數會得到不完整的物件。
//   4. 本檔參數用 string name 值傳遞，會多一次複製；實務上建議 const string&
//      （見本課 2.cpp）或初始化列表（第 16 課）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】this 指標
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. this 指標的型別是什麼？可以修改它嗎？
//     答：在非 const 成員函數中是 Class* const，在 const 成員函數中是
//         const Class* const。指標本身是 const，所以不能被賦值——
//         成員函數執行期間，它指向的物件不會改變。
//     追問：靜態成員函數裡有 this 嗎？
//         → 沒有。靜態成員函數屬於類別而非物件，沒有「當前物件」可指。
//
// 🔥 Q2. 寫 this->name 和直接寫 name 有什麼差別？效能一樣嗎？
//     答：效能完全一樣，機器碼相同——成員存取本來就是透過 this 做位移定址。
//         差別只在名稱查找：直接寫 name 可能被同名的參數或區域變數遮蔽，
//         this->name 則只在類別作用域查找，一定是成員。
//     追問：有沒有非寫 this-> 不可的場合？
//         → 有。繼承 template base class 時，base 的成員是相依名稱，
//           必須用 this->member 或 Base<T>::member 才找得到。
//
// ⚠️ 陷阱. 在建構函數裡呼叫虛擬函數，會呼叫到衍生類別的版本嗎？
//     答：不會。建構期間物件的動態型別就是「當前正在建構的那一層」，
//         虛擬呼叫會停在本層，不會下派到衍生類別。
//     為什麼會錯：以為 this 一直都是「最終物件」的指標，所以虛擬機制應該
//         全程有效。實際上衍生類別的部分在此時還沒建構完，若真的下派過去，
//         就會操作到尚未初始化的成員，所以語言刻意禁止。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
using namespace std;

class Solution1 {
private:
    string name;
    int age;

public:
    // 參數與成員同名 → 函數體內裸寫 name/age 會拿到「參數」
    // 用 this-> 明確指定左邊是成員，右邊維持參數
    Solution1(string name, int age) {
        this->name = name;   // 成員 name ← 參數 name
        this->age = age;     // 成員 age  ← 參數 age
    }

    void print() const {
        // 這裡沒有同名參數，所以裸寫 name 就是成員（等同 this->name）
        cout << "  name = " << name << ", age = " << age << endl;
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】鏈式呼叫（method chaining）的查詢建構器
//   情境：組 SQL 或 HTTP 查詢時，希望寫成
//       q.table("users").where("age > 18").limit(10)
//   做法：每個設定方法都 return *this，回傳當前物件的引用，就能接著往下串。
//         這是 this 指標最常見、也最實用的用途之一。
// -----------------------------------------------------------------------------
class QueryBuilder {
private:
    string table_;
    string where_;
    int limit_;

public:
    QueryBuilder() : table_("(未指定)"), where_(""), limit_(0) { }

    // 回傳 *this 的引用 → 可以繼續串下一個呼叫
    QueryBuilder& table(const string& t) {
        this->table_ = t;
        return *this;        // 解參考 this，回傳物件本身
    }

    QueryBuilder& where(const string& cond) {
        this->where_ = cond;
        return *this;
    }

    QueryBuilder& limit(int n) {
        this->limit_ = n;
        return *this;
    }

    void build() const {
        cout << "  SELECT * FROM " << table_;
        if (!where_.empty()) cout << " WHERE " << where_;
        if (limit_ > 0)      cout << " LIMIT " << limit_;
        cout << endl;
    }
};

int main() {
    cout << "=== this-> 解決參數與成員同名 ===" << endl;
    Solution1 s("李四", 30);
    s.print();

    cout << "\n=== 日常實務：鏈式呼叫查詢建構器 ===" << endl;
    QueryBuilder q;
    q.table("users").where("age > 18").limit(10).build();

    QueryBuilder q2;
    q2.table("orders").build();          // 只設 table，其餘用預設

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 15 課：帶參數的建構函數4.cpp" -o demo4

// === 預期輸出 ===
// === this-> 解決參數與成員同名 ===
//   name = 李四, age = 30
//
// === 日常實務：鏈式呼叫查詢建構器 ===
//   SELECT * FROM users WHERE age > 18 LIMIT 10
//   SELECT * FROM orders
