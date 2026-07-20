// =============================================================================
//  第 26 課：this 指標 1  —  this 指向誰、隱式與顯式
// =============================================================================
//
// 【主題資訊 Information】
//   this 的型別:
//     非 const 成員函式    →  C* const        （指標本身不可改）
//     const 成員函式       →  const C* const  （物件也不可改）
//   值: 呼叫該成員函式的那個物件的位址
//   標準版本: C++98；C++11 起可用於 lambda 捕獲 [this]、C++17 起有 [*this]
//   標頭檔: <string>
//
// 【詳細解釋 Explanation】
//
// 【1. this 是編譯器偷偷加的第一個參數】
//   成員函式在底層大致等同於一個多帶一個參數的自由函式：
//       void Knight::takeDamage(int dmg)   →   void takeDamage(Knight* this, int dmg)
//       k1.takeDamage(30)                  →   takeDamage(&k1, 30)
//   所以「同一份函式碼為什麼能改到不同物件」這個問題的答案很簡單：
//   函式碼只有一份，變的是被傳進去的 this。
//   本檔輸出中 k1 與 k2 呼叫同一個 takeDamage，卻各自扣自己的血，就是證據。
//
// 【2. 隱式 vs 顯式：完全等價】
//   在成員函式裡寫 name_，編譯器會自動補成 this->name_。
//   printInfo() 裡那兩行印出完全相同的結果，因為它們就是同一件事。
//   真正「必須」寫出 this-> 的情況只有三種：
//     (a) 參數與成員同名，需要消歧（本課 2.cpp 的主題）；
//     (b) 在 template 的衍生類別中存取相依基底類別的成員
//         （two-phase lookup 找不到，得寫 this->base_member）；
//     (c) 要把物件本身交出去：return *this、傳 this 給別的函式。
//
// 【3. 為什麼本檔不印出位址】
//   位址是執行期才決定的，而且現代作業系統有 ASLR，
//   同一支程式每次執行印出來的數值都不一樣。
//   我們想驗證的其實是一個布林命題 ——「this 是不是就等於 &k1」——
//   直接比較指標即可，不需要看到數值。
//   這也讓本檔的輸出成為可重現的、可以寫進文件的結果。
//
// 【4. this 是 prvalue，不能被賦值】
//   this 的型別是 C* const：頂層 const 表示「指標本身不可改」，
//   所以 this = &other; 是編譯錯誤。
//   在 const 成員函式中型別變成 const C* const，
//   於是連 this->hp_ = 0; 也會被擋下 —— 這正是 const 成員函式的實作方式。
//
// 【概念補充 Concept Deep Dive】
//   * 靜態成員函式沒有 this，所以不能加 const、不能是 virtual，
//     也存取不到非靜態成員（第 25 課的全部規則都源自這一點）。
//   * 不同物件必然有不同位址（即使是空類別，sizeof 也至少為 1），
//     所以 k1.self() == k2.self() 必為「否」——這是標準保證，不是巧合。
//   * this 在建構子裡就已經可用，但此時物件尚未建構完成，
//     把 this 傳出去（註冊 callback）是常見的競態來源。
//   * C++11 的 lambda 捕獲 [this] 抓的是「指標」，不是物件副本 ——
//     若 lambda 活得比物件久，就是懸空指標。
//     C++17 起可寫 [*this] 捕獲物件的複本來避開這個問題。
//   * 對「已結束生命期的物件」的指標做比較，是實作定義的行為
//     （[basic.stc]），所以懸空指標連拿來比較都應該避免。
//
// 【注意事項 Pay Attention】
//   1. this 不可被賦值（型別是 C* const）。
//   2. const 成員函式中的 this 是 const C* const，無法修改非靜態成員。
//   3. 位址每次執行都不同（ASLR），不要把它寫進預期輸出。
//   4. 在建構子中把 this 傳出去很危險 —— 物件尚未建構完成。
//   5. lambda 的 [this] 捕獲的是指標，生命期要自行確保。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】this 指標的本質
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. this 指標是什麼?它是怎麼傳進成員函式的?
//     答：this 是指向「呼叫該成員函式的那個物件」的指標。
//         它由編譯器隱含地當成第一個參數傳入 ——
//         k1.takeDamage(30) 在底層大致等同於 takeDamage(&k1, 30)。
//         這解釋了為什麼同一份函式碼能操作不同物件：
//         程式碼只有一份，變的是 this。
//     追問：this 的確切型別是什麼?
//         → 非 const 成員函式中是 C* const，
//         const 成員函式中是 const C* const。
//         頂層 const 表示不能寫 this = ...。
//
// 🔥 Q2. 什麼時候「必須」明確寫出 this->?
//     答：三種情況。(1) 參數與成員同名時消歧；
//         (2) 在 template 的衍生類別中存取相依基底類別的成員 ——
//         two-phase lookup 在第一階段看不到相依基底，
//         必須寫 this->member 或 Base<T>::member 才找得到；
//         (3) 需要把物件本身交出去時，例如 return *this（鏈式呼叫）。
//     追問：其他時候寫 this-> 有差別嗎?
//         → 沒有，純風格。編譯器本來就會把 name_ 補成 this->name_。
//
// ⚠️ 陷阱. 「兩個物件如果所有成員的值都一樣，this 也可能一樣。」
//     答：不會。標準保證同型別的兩個「同時存活」的不同物件必定有不同位址 ——
//         這正是為什麼空類別的 sizeof 至少是 1（大小為 0 的話，
//         陣列中相鄰元素就會落在同一個位址）。
//         本檔輸出中 k1.self() == k2.self() 必然是「否」。
//     為什麼會錯：把「值相等」和「同一個物件」混為一談。
//         值相等是 operator== 的問題，同一性是位址的問題。
//         這個區分在寫 operator= 時特別關鍵 ——
//         自我賦值檢查寫的是 if (this == &other)（比同一性），
//         而不是 if (*this == other)（比值）。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】—— 從缺，理由如下
//   this 指標屬於語言的物件模型，LeetCode 判題不涉及。
//   （唯一沾得上邊的是設計類題目常用 return *this 做鏈式呼叫，
//   那是本課後續檔案的主題，在此不重複。）
//   本檔改以「同一份程式碼、不同物件」的可執行驗證呈現 this 的本質。
//
// =============================================================================

#include <iostream>
#include <string>
using namespace std;

class Knight {
private:
    string name_;
    int hp_;

public:
    Knight(const string& name, int hp)
        : name_(name), hp_(hp)
    {
    }

    // 注意：這裡刻意「不印出位址」。
    // 位址每次執行都不同（ASLR），印出來無法當成穩定的預期輸出，
    // 而且我們真正想驗證的是「this 是不是就等於物件的位址」——
    // 那是一個布林問題，比較指標即可，不需要看到數值。
    void showThis(const Knight* expected, const char* label) const {
        cout << "  " << name_ << "：this == &" << label << " ? "
             << (this == expected ? "是" : "否") << endl;
    }

    // 回傳 this，讓呼叫端可以自行比較（同樣不印數值）
    const Knight* self() const { return this; }

    void takeDamage(int dmg) {
        // 不同物件呼叫同一個函式，this 不同 ——
        // 這裡用 name_ 呈現，因為 name_ 本來就是透過 this 讀出來的
        cout << "  " << name_ << " 受到 " << dmg << " 傷害"
             << "（hp " << hp_ << " → " << (hp_ - dmg) << "）" << endl;
        hp_ -= dmg;
    }

    void printInfo() const {
        // 以下兩種寫法完全等價：
        cout << "  名字：" << name_ << endl;        // 隱式使用 this
        cout << "  名字：" << this->name_ << endl;   // 顯式使用 this
        // 編譯器會把 name_ 自動轉成 this->name_
    }
};

int main() {
    cout << "=== this 指向誰？ ===" << endl;

    Knight k1("亞瑟", 200);
    Knight k2("蘭斯洛特", 180);

    // 驗證 this 就是「呼叫這個函式的那個物件」的位址
    cout << "\n--- this 到底指向誰（比較指標，不印位址）---" << endl;
    k1.showThis(&k1, "k1");     // 是
    k1.showThis(&k2, "k2");     // 否
    k2.showThis(&k2, "k2");     // 是
    k2.showThis(&k1, "k1");     // 否

    cout << "\n--- 兩個物件的 this 互不相同 ---" << endl;
    cout << "  k1.self() == k2.self() ? "
         << (k1.self() == k2.self() ? "是" : "否")
         << "（不同物件必然不同位址）" << endl;
    cout << "  k1.self() == &k1      ? "
         << (k1.self() == &k1 ? "是" : "否") << endl;

    // 不同對象調用同一個函數，this 不同
    cout << "\n--- 不同對象，不同 this ---" << endl;
    k1.takeDamage(30);
    k2.takeDamage(50);
    cout << "  ↑ 同一份函式碼，卻改到不同物件的 hp_ ——" << endl;
    cout << "    差別完全來自編譯器傳進去的 this。" << endl;

    // 驗證隱式 vs 顯式
    cout << "\n--- 隱式 vs 顯式 this ---" << endl;
    k1.printInfo();

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第 26 課：this 指標1.cpp -o this_ptr1

// === 預期輸出 ===
// === this 指向誰？ ===
//
// --- this 到底指向誰（比較指標，不印位址）---
//   亞瑟：this == &k1 ? 是
//   亞瑟：this == &k2 ? 否
//   蘭斯洛特：this == &k2 ? 是
//   蘭斯洛特：this == &k1 ? 否
//
// --- 兩個物件的 this 互不相同 ---
//   k1.self() == k2.self() ? 否（不同物件必然不同位址）
//   k1.self() == &k1      ? 是
//
// --- 不同對象，不同 this ---
//   亞瑟 受到 30 傷害（hp 200 → 170）
//   蘭斯洛特 受到 50 傷害（hp 180 → 130）
//   ↑ 同一份函式碼，卻改到不同物件的 hp_ ——
//     差別完全來自編譯器傳進去的 this。
//
// --- 隱式 vs 顯式 this ---
//   名字：亞瑟
//   名字：亞瑟
