// =============================================================================
//  第 26 課：this 指標 2  —  同名消歧（參數與成員同名）
// =============================================================================
//
// 【主題資訊 Information】
//   語法:  this->member = member;     // 左邊是成員，右邊是參數
//   型別:  在 C 的非 const 成員函式中，this 的型別是 C* const
//          在 const 成員函式中則是 const C* const
//   標準版本: C++98
//   標頭檔: <string>
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼會有同名問題】
//   C++ 的名稱查找是「由內而外」：
//   在成員函式裡寫一個名字，會先找函式的區域範圍（含參數），
//   找不到才往類別範圍找。
//   所以參數 name 會「遮蔽」（shadow）成員 name_。
//   此時寫 name = name; 兩邊都是參數 —— 變成參數自我賦值，
//   成員從頭到尾沒被碰過。編譯器不會報錯，物件默默保持預設值。
//
//   ★ 一個重要的補充（本機 g++ 15 實測）：
//     「name = name; 會安靜地失敗」這句話成立與否，取決於參數怎麼傳。
//       * 參數是「傳值」string name 或「非 const 參考」string& name
//         → 自我賦值，合法、無效果，成員沒被設定。這才是安靜的 bug。
//       * 參數是「const 參考」const string& name（本檔建構子的寫法）
//         → 等於對 const 物件賦值，直接編譯失敗：
//           error: no match for 'operator=' ...
//           passing 'const std::string*' as 'this' argument discards qualifiers
//         換句話說，const& 參數反而幫你把錯誤擋在編譯期。
//     下方實務範例的錯誤示範刻意改用「傳值」參數，
//     才能真正重現「可編譯、可執行、但成員沒被設定」的情形。
//
// 【2. this-> 為什麼能解決】
//   this 是指向「目前這個物件」的指標，
//   this->name 明確指定「到類別範圍去找」，繞過了區域範圍的遮蔽。
//   這不是特殊語法，就是一般的成員存取，只是把隱含的 this 寫出來。
//
// 【3. 更好的作法：成員初始化列表】
//   本檔在建構子的「函式體」裡賦值，其實不是最佳寫法：
//       Weapon(const string& name, ...) { this->name = name; }   // 先預設建構再賦值
//       Weapon(const string& name, ...) : name(name) { }          // 直接初始化
//   初始化列表的版本對 string 這種有配置成本的型別更有效率，
//   因為它省掉「先預設建構、再賦值」的兩步。
//   而且初始化列表裡的 name(name) 不會有歧義 ——
//   括號外的一定是成員、括號內的一定是參數，這是標準明文規定的。
//
// 【4. 實務上更常見的作法：直接避開同名】
//   多數團隊用命名慣例讓問題消失：成員加後綴底線（name_）、
//   或參數加前綴（newName）。這樣既不需要 this->，也不會有遮蔽風險。
//   本課其他檔案用的正是 name_ 的慣例。
//   本檔刻意保留同名，是為了示範 this-> 的必要性。
//
// 【概念補充 Concept Deep Dive】
//   * setDamage 裡的 damage = 0; 改的是「參數」，不是成員 ——
//     它只是先把參數修正到合法範圍，下一行才寫回成員。
//     這個寫法可行但容易誤讀，實務上建議用不同的名字。
//   * g++ 的 -Wshadow 可以警告這種遮蔽，但它預設不開，
//     而且會對「參數遮蔽成員」這種常見寫法產生大量警告，
//     所以多數專案改用命名慣例而非開這個警告。
//   * this 本身是個 prvalue，型別是 C* const —— 指標本身不可改
//     （不能寫 this = ...），但指向的物件可以改（除非是 const 成員函式）。
//   * C++11 起 this 可以出現在 lambda 的捕獲列表裡（[this]），
//     那會捕獲指標而非物件 —— 若 lambda 存活得比物件久就是懸空指標。
//
// 【注意事項 Pay Attention】
//   1. name = name; 是合法程式碼，編譯器通常不報錯，但成員完全沒被設定。
//   2. 建構子優先用成員初始化列表，效率較好且無歧義。
//   3. 成員初始化「一律依宣告順序」執行，與列表撰寫順序無關（-Wreorder）。
//   4. 實務上用命名慣例（name_ / newName）比依賴 this-> 更安全。
//   5. this 的型別是 C* const，不能對 this 本身賦值。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】this 指標與同名消歧
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 建構子參數與成員同名時，寫 name = name; 會發生什麼事?
//     答：兩邊都會解析成「參數」，變成參數對自己賦值，成員完全沒被修改，
//         物件會保持預設值（string 是空字串、int 是不確定值）。
//         編譯器通常不報錯，所以這是個很安靜的 bug。
//         原因是名稱查找由內而外：參數在區域範圍，會遮蔽類別範圍的成員。
//     追問：怎麼解?
//         → 三種：寫 this->name = name;、
//         用成員初始化列表 : name(name)（括號外必為成員，無歧義）、
//         或直接用命名慣例避開同名（成員寫 name_）。
//
// 🔥 Q2. 成員初始化列表和在建構子函式體裡賦值，差在哪?
//     答：初始化列表是「直接初始化」；函式體裡是「先預設建構，再賦值」。
//         對 std::string 這類有配置成本的型別，後者多做一次無謂的工作。
//         更關鍵的是：const 成員與參考成員「只能」用初始化列表，
//         因為它們一旦建立就不能再被賦值。
//     追問：初始化的順序由誰決定?
//         → 由成員的「宣告順序」決定，與初始化列表寫的順序無關。
//         兩者不一致時 g++ -Wall 會發出 -Wreorder 警告 ——
//         若某個成員的初始化依賴另一個成員，順序寫錯就是真 bug。
//
// ⚠️ 陷阱. 「setDamage 裡寫了 if (damage < 0) damage = 0;
//            所以成員 damage 已經被修正成 0 了。」
//     答：那一行改的是「參數」，不是成員。
//         真正寫進成員的是下一行的 this->damage = damage;。
//         換句話說，這段程式碼之所以正確，完全靠那兩行的先後順序 ——
//         如果有人把 this->damage = damage; 移到 if 之前，
//         驗證就會完全失效，而且編譯器不會有任何抱怨。
//     為什麼會錯：在同名的情況下閱讀時，
//         腦中會自動把每個 damage 都當成「同一個東西」。
//         實際上有沒有 this-> 決定了它是兩個完全不同的變數。
//         這正是實務上寧可用命名慣例避開同名的理由 ——
//         不是因為 this-> 不能用，而是因為人類讀起來太容易出錯。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】—— 從缺，理由如下
//   同名消歧是名稱查找規則，LeetCode 判題只驗輸入輸出。
//   本檔改以「設定物件的 builder 風格 setter」實務範例，
//   示範同名參數在真實程式碼中最常出現的位置，以及三種寫法的對照。
//
// =============================================================================

#include <iostream>
#include <string>
using namespace std;

class Weapon {
private:
    string name;
    int damage;
    int durability;

public:
    // 參數名和成員名完全相同！
    Weapon(const string& name, int damage, int durability) {
        // 不用 this 的話，name = name 是自我賦值（參數給參數）
        // 必須用 this-> 區分：
        this->name = name;
        this->damage = damage;
        this->durability = durability;
    }

    // setter 也常遇到同名問題
    void setDamage(int damage) {
        if (damage < 0) damage = 0;   // 這裡的 damage 是參數
        this->damage = damage;         // this->damage 是成員
    }

    void print() const {
        cout << "  " << name << " (傷害:" << damage
             << " 耐久:" << durability << ")" << endl;
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】三種寫法的對照：同名 this-> / 初始化列表 / 命名慣例
//   情境：伺服器設定物件，欄位名稱在 API 與內部是一致的（host、port），
//         所以「參數與成員同名」在真實程式碼裡非常容易發生。
//   下面用同一組欄位寫三個版本，並實際印出結果，
//   證明「忘了 this->」的那個版本會安靜地留下未設定的成員。
// -----------------------------------------------------------------------------

// 版本 A：同名 + 忘了寫 this->（錯誤示範，可以編譯、可以執行）
class ConfigBuggy {
private:
    string host;         // string 預設建構為空字串
    int    port = 0;     // 刻意給預設成員初始值（C++11）
                         // 若不給，這個 int 就是「不確定值」，讀它是未定義行為，
                         // 本範例會因此無法產生穩定可貼的輸出。
                         // 給了預設值之後，錯誤仍然存在（成員沒被參數設定），
                         // 但至少是「確定地錯」——這也正是實務上該給預設值的理由。

public:
    // 注意參數刻意用「傳值」而非 const&，這是有原因的 ——
    // 若參數是 const string&，host = host; 會因為「無法賦值給 const」
    // 而直接編譯失敗（錯誤訊息：no match for 'operator='，
    // passing 'const std::string*' as 'this' argument discards qualifiers）。
    // 也就是說，const& 參數反而幫你擋下了這個錯誤。
    // 真正安靜的 bug 發生在參數是「傳值」或「非 const 參考」的時候，
    // 而傳值參數在現代 C++ 是很常見的寫法，所以這個坑並不罕見。
    ConfigBuggy(string host, int port) {
        // 這兩行都是「參數 = 參數」，成員完全沒被碰到。
        // 名稱查找由內而外：參數遮蔽了成員，兩邊解析到的都是參數。
        host = host;      // 刻意保留的錯誤示範（自我賦值，無任何效果）
        port = port;      // 刻意保留的錯誤示範
    }
    void print() const {
        cout << "    host=\"" << host << "\" port=" << port << endl;
    }
};

// 版本 B：同名 + 正確使用 this->
class ConfigThis {
private:
    string host;
    int    port;

public:
    ConfigThis(const string& host, int port) {
        this->host = host;   // 左邊 this-> 是成員，右邊是參數
        this->port = port;
    }
    void print() const {
        cout << "    host=\"" << host << "\" port=" << port << endl;
    }
};

// 版本 C：成員初始化列表 + 命名慣例（實務上的建議做法）
class ConfigClean {
private:
    string host_;        // 底線後綴：從命名上就不可能同名
    int    port_;

public:
    // 初始化列表：直接初始化，不是「先預設建構再賦值」
    // 順序與宣告順序一致（host_ 先於 port_），避免 -Wreorder
    ConfigClean(const string& host, int port)
        : host_(host), port_(port) {}

    void print() const {
        cout << "    host=\"" << host_ << "\" port=" << port_ << endl;
    }
};

int main() {
    cout << "=== 場景一：同名消歧 ===" << endl;

    Weapon sword("鐵劍", 25, 100);
    sword.print();

    sword.setDamage(40);
    sword.print();

    cout << "\n  註：setDamage 裡的 if (damage < 0) damage = 0; 改的是「參數」，" << endl;
    cout << "      真正寫進成員的是下一行的 this->damage = damage;。" << endl;
    cout << "      這段程式碼的正確性完全依賴這兩行的先後順序。" << endl;

    cout << "\n=== 日常實務：三種寫法的對照 ===" << endl;
    cout << "  同樣傳入 host=\"api.example.com\" port=8080：" << endl;

    cout << "\n  A. 同名但忘了 this->（錯誤，但可編譯可執行）" << endl;
    ConfigBuggy("api.example.com", 8080).print();

    cout << "\n  B. 同名 + 正確使用 this->" << endl;
    ConfigThis("api.example.com", 8080).print();

    cout << "\n  C. 初始化列表 + 命名慣例（實務建議）" << endl;
    ConfigClean("api.example.com", 8080).print();

    cout << "\n  ↑ A 的成員從未被參數設定，印出的是預設值（空字串與 0）。" << endl;
    cout << "    整段程式沒有任何編譯錯誤或警告，錯誤只會在執行期以" << endl;
    cout << "    「設定怎麼是空的」的形式浮現 —— 這就是同名遮蔽的代價。" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第 26 課：this 指標2.cpp -o this_ptr2

// === 預期輸出 ===
// === 場景一：同名消歧 ===
//   鐵劍 (傷害:25 耐久:100)
//   鐵劍 (傷害:40 耐久:100)
//
//   註：setDamage 裡的 if (damage < 0) damage = 0; 改的是「參數」，
//       真正寫進成員的是下一行的 this->damage = damage;。
//       這段程式碼的正確性完全依賴這兩行的先後順序。
//
// === 日常實務：三種寫法的對照 ===
//   同樣傳入 host="api.example.com" port=8080：
//
//   A. 同名但忘了 this->（錯誤，但可編譯可執行）
//     host="" port=0
//
//   B. 同名 + 正確使用 this->
//     host="api.example.com" port=8080
//
//   C. 初始化列表 + 命名慣例（實務建議）
//     host="api.example.com" port=8080
//
//   ↑ A 的成員從未被參數設定，印出的是預設值（空字串與 0）。
//     整段程式沒有任何編譯錯誤或警告，錯誤只會在執行期以
//     「設定怎麼是空的」的形式浮現 —— 這就是同名遮蔽的代價。
