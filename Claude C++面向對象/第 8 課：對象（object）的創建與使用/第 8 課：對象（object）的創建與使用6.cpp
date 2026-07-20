// =============================================================================
//  第 8 課：對象（object）的創建與使用6.cpp  —  物件互動：以參考傳遞另一個物件
// =============================================================================
//
// 【主題資訊 Information】
//   void attackTarget(Player& target);      // 以【非 const 參考】接收另一個物件
//   參考（reference）：物件的別名，不是指標、不佔額外空間（概念上）
//   特性：必須在宣告時綁定、綁定後不可改綁、不可為 null
//   標準版本：C++98 起（類內初始值為 C++11）
//   複雜度：傳參考 O(1)，不複製物件
//   標頭檔：語言核心特性
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼 target 必須是參考而不是傳值】
//   如果寫成 void attackTarget(Player target)，
//   函式會拿到一份【複製品】，扣血扣在複製品上，
//   函式結束時複製品消失，原本的法師 HP 完全沒變。
//   程式會安靜地「看起來有作用」（因為函式內的 cout 印出了扣血後的值），
//   但戰鬥結束時 showStatus() 會顯示 HP 從未改變 ——
//   這是很難察覺的邏輯錯誤。
//   本檔用 Player& 讓 target 成為【原物件的別名】，
//   對它的修改就是對原物件的修改。
//
// 【2. 一個成員函式同時操作兩個物件】
//   attackTarget 裡同時出現了兩個物件：
//       name / attack     → 隱含 this->，是【攻擊者】自己
//       target.name / hp  → 是【被攻擊者】
//   這是 OOP 中很重要的一步：成員函式不只能操作 this，
//   也能存取同類別其他物件的【private 成員】——
//   存取控制是【以類別為單位】而不是以物件為單位的。
//   （本檔成員都是 public，所以看不出來，
//     但即使 hp 是 private，attackTarget 依然可以寫 target.hp。）
//
// 【3. 三個回合的數值變化】
//   戰士 HP 120 攻擊 15；法師 HP 80 攻擊 25。
//       第 1 回合：戰士打法師 → 法師 80 − 15 = 65
//       第 2 回合：法師打戰士 → 戰士 120 − 25 = 95
//       第 3 回合：戰士打法師 → 法師 65 − 15 = 50
//   最終兩人都還活著（isAlive() 判斷 hp > 0）。
//   全部是確定的算術，沒有亂數 —— 輸出完全可重現。
//
// 【4. 這個設計的一個真實缺陷：沒有防止自我攻擊】
//       warrior.attackTarget(warrior);     // 完全合法，自己打自己
//   在真實遊戲中這通常是 bug。
//   更嚴重的是，若 attackTarget 內部涉及資源移動
//   （例如「把目標的裝備搶過來」），自我操作往往會造成資料損毀 ——
//   這正是 operator= 必須做自我賦值檢查（if (this != &other)）的原因。
//   防禦寫法是：
//       if (this == &target) return;       // 比較位址即可判斷是不是同一個物件
//
// 【概念補充 Concept Deep Dive】
//
// (A) 參考在底層通常就是指標
//   編譯器多半把 Player& 實作成 Player*，
//   只是語法上自動解參考、且保證不為 null、不可改綁。
//   所以傳參考的成本與傳指標相同（本機 8 bytes 的位址），
//   遠低於複製整個 Player 物件（本機 sizeof(Player) = 40）。
//   但參考在語意上不是指標 —— 它沒有自己的身分，
//   sizeof(參考) 得到的是【被參考物件】的大小。
//
// (B) 什麼時候該用 const 參考
//   本檔的 attackTarget 要修改 target，所以用 Player&。
//   但 showStatus() 若寫成自由函式，應該是：
//       void showStatus(const Player& p);
//   規則很清楚：
//       要修改       → T&
//       只讀且物件大 → const T&
//       只讀且物件小（int、指標）→ 直接傳值反而更快
//   同課 8 號檔專門示範這三種選擇。
//
// (C) 為什麼 isAlive() 與 showStatus() 應該是 const 成員函式
//   兩者都不修改狀態，應宣告為 bool isAlive() const。
//   若不加 const，一旦某處出現 const Player& 就無法呼叫它們。
//   本檔為簡化而省略，但這在真實程式碼中會立刻造成問題 ——
//   const 正確性必須從一開始就做對。
//
// 【注意事項 Pay Attention】
//   1. 要修改傳入的物件就必須用參考（或指標）；傳值只會改到複製品。
//   2. 成員函式可以存取【同類別其他物件】的 private 成員 ——
//      存取控制以類別為單位，不是以物件為單位。
//   3. 本檔沒有防止自我攻擊（warrior.attackTarget(warrior) 完全合法），
//      涉及資源移動的操作務必檢查 if (this == &other)。
//   4. isAlive() 與 showStatus() 應宣告為 const 成員函式（本檔為簡化省略）。
//   5. hp 可能被扣成負數，本檔的 isAlive() 用 hp > 0 判斷，
//      所以邏輯仍然正確；但顯示負數 HP 在真實遊戲中通常要另外處理。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】物件互動與參考傳遞
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. attackTarget 的參數如果從 Player& 改成 Player，會發生什麼？
//     答：函式會拿到一份【複製品】，target.hp -= attack 扣在複製品上，
//         函式一結束複製品就消失，原本的法師 HP 完全沒變。
//         最陰險的是函式內的 cout 仍會印出「法師 剩餘 HP: 65」
//         （複製品確實被扣了），
//         所以畫面上看起來完全正常，
//         直到戰鬥結束呼叫 showStatus() 才發現 HP 從未改變。
//     追問：那怎麼避免不小心寫成傳值？
//           → 對於「要修改」的參數用 T&，「只讀」的用 const T&，
//             並養成習慣：只有內建型別（int、指標、bool）才傳值。
//             程式碼審查時看到大型物件傳值就該是警訊。
//
// 🔥 Q2. attackTarget 存取了 target.hp，如果 hp 是 private 還能存取嗎？
//     答：可以。存取控制是【以類別為單位】而不是以物件為單位的 ——
//         Player 的成員函式可以存取【任何一個】Player 物件的 private 成員，
//         不限於 this 指向的那一個。
//         這正是為什麼 operator==、copy constructor、operator=
//         能直接讀取另一個物件的私有資料。
//     追問：那不同類別之間呢？
//           → 預設不行，必須宣告為 friend。
//             friend 也是以類別（或函式）為單位授權，
//             而且是【授權方】主動給出的，不能從外部強取。
//
// ⚠️ 陷阱. warrior.attackTarget(warrior); 這行程式碼合法嗎？
//          會發生什麼事？
//     答：完全合法，而且會編譯通過、執行時自己扣自己的血。
//         在本檔中結果還算「合理」（HP 減少 15），
//         但這幾乎必定是邏輯 bug。
//         更危險的是當函式涉及【資源移動】時 ——
//         例如「把目標的裝備搶過來」會先清空自己再從目標複製，
//         而目標就是自己，結果是資料全毀。
//     為什麼會錯：寫 attackTarget 時腦中的畫面是「兩個不同的角色」，
//         自然不會想到兩個參數可能是同一個物件。
//         這正是 operator= 必須寫
//             if (this != &other) { ... }
//         的原因 —— 自我賦值時若先釋放自己的資源再從 other 複製，
//         就是從已釋放的記憶體讀取。
//         判斷方法很簡單：比較位址 if (this == &target)。
//         凡是接收「同類別另一個物件」且會修改狀態的函式，
//         都該先想一想自我操作的情況。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
using namespace std;

class Player {
public:
    string name;
    int hp = 100;       // 生命值
    int attack = 10;    // 攻擊力

    void attackTarget(Player& target) {
        cout << name << " 攻擊了 " << target.name << "！" << endl;
        target.hp -= attack;
        cout << target.name << " 剩餘 HP: " << target.hp << endl;
    }

    void showStatus() {
        cout << "[" << name << "] HP: " << hp
             << " / 攻擊力: " << attack << endl;
    }

    bool isAlive() {
        return hp > 0;
    }
};

int main() {
    Player warrior;
    warrior.name = "戰士";
    warrior.hp = 120;
    warrior.attack = 15;

    Player mage;
    mage.name = "法師";
    mage.hp = 80;
    mage.attack = 25;

    cout << "===== 戰鬥開始 =====" << endl;
    warrior.showStatus();
    mage.showStatus();

    cout << "\n--- 第 1 回合 ---" << endl;
    warrior.attackTarget(mage);     // 戰士攻擊法師

    cout << "\n--- 第 2 回合 ---" << endl;
    mage.attackTarget(warrior);     // 法師攻擊戰士

    cout << "\n--- 第 3 回合 ---" << endl;
    warrior.attackTarget(mage);     // 戰士再攻擊法師

    cout << "\n===== 戰鬥結束 =====" << endl;
    warrior.showStatus();
    mage.showStatus();

    // 檢查存活狀態
    cout << "\n" << warrior.name << (warrior.isAlive() ? " 存活" : " 陣亡") << endl;
    cout << mage.name << (mage.isAlive() ? " 存活" : " 陣亡") << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 8 課：對象（object）的創建與使用6.cpp" -o obj6
//   類內初始值（int hp = 100;）為 C++11 起的功能。

// 註 1:本檔輸出是【完全確定的】—— 沒有輸入、亂數、執行緒或位址。
//      戰鬥數值全部是寫死的算術：
//        戰士 HP 120 / 攻擊 15；法師 HP 80 / 攻擊 25
//        第1回合 法師 80-15=65；第2回合 戰士 120-25=95；第3回合 法師 65-15=50

// 註 2:attackTarget 的參數是 Player&（參考）。若改成傳值 Player，
//      扣血會發生在複製品上 —— 而且函式內的 cout 依然會印出扣血後的數字，
//      畫面看起來完全正常，直到最後 showStatus() 才會發現 HP 從未改變。
//      這是「傳值 vs 傳參考」最容易被忽略的失敗模式。

// 註 3:attackTarget 同時操作兩個物件：name/attack 是攻擊者自己（隱含 this->），
//      target.name/hp 是被攻擊者。成員函式可以存取【同類別其他物件】的
//      private 成員 —— 存取控制以類別為單位，不是以物件為單位。

// 註 4:本檔沒有防止 warrior.attackTarget(warrior)（自己打自己）。
//      在這裡只是邏輯怪異，但同樣的疏漏在 operator= 中
//      會造成真正的資料損毀，所以才需要 if (this != &other) 的檢查。

// === 預期輸出 ===
// ===== 戰鬥開始 =====
// [戰士] HP: 120 / 攻擊力: 15
// [法師] HP: 80 / 攻擊力: 25
//
// --- 第 1 回合 ---
// 戰士 攻擊了 法師！
// 法師 剩餘 HP: 65
//
// --- 第 2 回合 ---
// 法師 攻擊了 戰士！
// 戰士 剩餘 HP: 95
//
// --- 第 3 回合 ---
// 戰士 攻擊了 法師！
// 法師 剩餘 HP: 50
//
// ===== 戰鬥結束 =====
// [戰士] HP: 95 / 攻擊力: 15
// [法師] HP: 50 / 攻擊力: 25
//
// 戰士 存活
// 法師 存活
