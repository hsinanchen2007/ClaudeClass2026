// =============================================================================
//  第 23 課：mutable 關鍵字 —— summary.cpp（本課教科書級總整理）
// =============================================================================
//
// 【主題資訊 Information】
//   語法：
//     mutable 型別 成員名;     // 該成員豁免於 const 檢查
//   限制：不可用於 static 成員、const 成員、參考型別成員。
//   標準版本：C++98 起即有;C++11 起 lambda 也有 mutable 修飾(語意不同)。
//   複雜度：不影響複雜度;不改變物件佈局,亦無執行期成本。
//   標頭檔：<iostream>、<string>、<vector>
//
// 【詳細解釋 Explanation】
//
// 【1. 問題的起源:編譯器強制的是 bitwise const,人要的是 logical const】
//   編譯器檢查的規則很單純:const 成員函式裡不准寫任何非 mutable 直接成員
//   —— 這是「位元層級不變」(bitwise const)。
//   但設計者真正想表達的往往是「對外可觀察的狀態不變」(logical const):
//   物件回答問題的答案沒變,只是內部順便記了筆帳、存了份快取。
//   mutable 就是唯一能在語言層面表達這個差異的機制。
//
// 【2. 三個正當用途(本檔逐一示範)】
//   (a) 快取 / 記憶化:昂貴計算結果存起來,下次直接回傳。
//   (b) 延遲初始化(lazy init):第一次被查詢時才真正建構昂貴的資源。
//   (c) 統計 / 診斷 / 同步:存取次數、命中率,以及 mutable std::mutex ——
//       最後這項讓 const 成員函式也能上鎖,是實務上最無爭議的用途。
//   三者的共同點:這些欄位都不參與「這個物件是什麼」的認定。
//
// 【3. 判準:拿掉它,兩個物件的相等性會不會改變?】
//   不會 → 它不屬於邏輯狀態,可以 mutable。
//   會   → 它是邏輯狀態的一部分,標 mutable 就是濫用。
//   例如「被查看次數」不影響一隻怪物是誰,可以 mutable;
//   但「HP」顯然是怪物狀態的一部分,若為了讓 const 函式能扣血而標 mutable,
//   那是在規避型別系統,而不是在表達設計意圖。
//
// 【4. mutable vs const_cast:一個合法,一個可能是 UB】
//   mutable 是標準明確允許的:即使物件宣告為 const,
//   它的 mutable 成員仍可被修改,行為完全定義良好。
//   const_cast 則不同 —— 對「本來就宣告為 const 的物件」去 const 後再寫入,
//   是未定義行為(UB),不保證任何特定結果,包括「看起來正常運作」。
//   需要在 const 函式中修改內部狀態時,一律用 mutable,不要用 const_cast。
//
// 【5. 代價:const 不再等於執行緒安全】
//   標準庫保證「並行呼叫同一物件的 const 成員函式」不會有資料競爭,
//   但前提是型別確實遵守 const 語意。一旦 const 函式裡有
//   ++counter_ 這種非原子的讀-改-寫,並行呼叫就是資料競爭 → 未定義行為。
//   要維持那個保證,mutable 成員必須改用 std::atomic 或以 mutable mutex 保護。
//
// 【概念補充 Concept Deep Dive】
//   * mutable 純粹是編譯期的豁免標記,不影響 sizeof、不影響物件佈局、
//     不產生任何額外指令。
//   * 延遲初始化的 mutable 版本要小心「第一次查詢」的執行緒安全問題;
//     單執行緒沒事,多執行緒需要 std::once_flag 或鎖。
//   * lambda 的 mutable 是完全不同的語意:[x]() mutable { ++x; }
//     表示「允許修改按值捕獲的副本」,與成員的 mutable 無關,別混淆。
//   * 標準庫本身大量使用這個模式 —— 例如許多實作會在 const 查詢介面
//     背後持有 mutable 的同步物件,以維持 const 的並行呼叫保證。
//
// 【注意事項 Pay Attention】
//   1. mutable 不能用於 static、const、參考成員。
//   2. mutable 讓 const 成員函式不再保證「物件位元組不變」。
//   3. 多執行緒下,mutable 成員的寫入必須自行同步,否則是資料競爭(UB)。
//   4. 不要用 mutable 規避「這個函式本來就該是非 const」的事實。
//   5. 需要在 const 函式中修改狀態時用 mutable,不要用 const_cast ——
//      後者對真正 const 的物件是 UB。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】mutable 關鍵字(本課總整理)
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. mutable 解決什麼問題?請用 bitwise / logical const 來說明。
//     答：編譯器強制的是 bitwise const(const 函式裡不准寫非 mutable 成員),
//         但設計者要表達的常常是 logical const(對外可觀察狀態不變)。
//         快取、延遲初始化、存取統計、加鎖這幾種情況兩者不一致 ——
//         物件的答案沒變,只是內部順便做了些事。
//         mutable 就是用來標記「這個成員不屬於邏輯狀態」。
//     追問：判準怎麼下?→ 拿掉這個成員,兩個物件的相等性判斷會不會改變?
//         不會就可以 mutable,會就不行。
//
// 🔥 Q2. mutable 和 const_cast 都能在 const 函式裡改東西,該用哪個?
//     答：一律用 mutable。mutable 是標準明確允許的,即使物件本身宣告為
//         const,修改其 mutable 成員也完全定義良好。
//         const_cast 去掉 const 後若修改「本來就宣告為 const 的物件」,
//         是未定義行為 —— 不保證任何特定結果,而且不會有任何診斷訊息。
//     追問：那 const_cast 什麼時候才該用?→ 呼叫忘了加 const 的舊 C API,
//         以及讓非 const 版本共用 const 版本的實作(底層物件本來就非 const)。
//
// 🔥 Q3. 加了 mutable 之後,const 成員函式還能安全地並行呼叫嗎?
//     答：不一定,責任回到你身上。標準庫的保證前提是型別遵守 const 語意;
//         一旦 const 函式裡有 ++hits_ 這種非原子的讀-改-寫,
//         並行呼叫就是資料競爭,屬未定義行為。
//         要維持並行安全,把計數器換成 std::atomic,
//         或用 mutable std::mutex 保護快取的讀寫。
//     追問：mutable mutex 為什麼是最常見的用法?→ 因為上鎖本身不改變物件的
//         邏輯狀態,卻必須在 const 查詢函式中進行,這正是 mutable 的定義場景。
//
// ⚠️ 陷阱 1. 「這個函式該是 const,但它要改一個成員,那就把成員標 mutable 吧。」
//     答：順序反了。正確的問法是「這個成員屬不屬於物件的邏輯狀態」。
//         若屬於(HP、餘額、內容),那代表這個函式本來就不該是 const,
//         應該把函式改成非 const,而不是把成員標 mutable。
//         mutable 是用來表達設計意圖的,不是用來讓編譯器閉嘴的。
//     為什麼會錯：把 mutable 當成「消除編譯錯誤的工具」。
//         這樣做的後果是 const 徹底失去意義 —— 一個處處 mutable 的類別,
//         它的 const 成員函式和非 const 完全沒有差別,型別系統再也幫不上忙。
//
// ⚠️ 陷阱 2. 「LRU Cache 的 get() 只是查詢,應該可以用 mutable 做成 const。」
//     答：不行,這正是 mutable 的邊界。LRU 的 get() 會更新「最近使用順序」,
//         而那個順序直接決定下次 put() 要淘汰誰 —— 它是外界可觀察的狀態,
//         不是單純的加速快取。把它 mutable 化再標 const,等於對呼叫端說謊。
//         (詳見下方 LeetCode 146 的完整說明與實作。)
//     為什麼會錯：把「這個方法名字叫 get」等同於「它是純查詢」。
//         判準永遠是「有沒有改變外界可觀察的行為」,不是方法叫什麼名字。
// ═══════════════════════════════════════════════════════════════════════════

/*
 * ============================================================================
 *   第 23 課：mutable 關鍵字 —— 完整總結
 * ============================================================================
 *
 *   本檔案涵蓋第 23 課所有 .cpp 檔案的核心觀念與範例程式碼，
 *   閱讀此檔案即可完整複習，無需再翻閱其他檔案。
 *
 * ============================================================================
 *   目錄
 * ============================================================================
 *   一、問題的起源：為什麼需要 mutable？
 *   二、mutable 的語法與基本用法（訪問計數器）
 *   三、經典應用一：計算快取（Cache）
 *   四、經典應用二：延遲初始化（Lazy Initialization）
 *   五、mutable 的判斷準則（適合 vs 不適合）
 *   六、反面教材：濫用 mutable 的後果
 *   七、mutable vs const_cast 的比較
 *   八、綜合範例：怪物圖鑑系統（結合快取 + 延遲初始化 + 計數器）
 *   九、本課重點回顧
 * ============================================================================
 */

#include <iostream>
#include <string>
#include <vector>          // LeetCode 146 的節點池需要
#include <unordered_map>   // LeetCode 146 的 key → index 對照需要
using namespace std;

// ============================================================================
//   一、問題的起源：為什麼需要 mutable？
// ============================================================================
//
//   上一課我們學到：const 成員函數承諾「不修改任何成員變數」。
//   但實務中，有些操作「邏輯上是只讀」，「實作上卻需要修改某些內部數據」：
//
//   場景 1：快取（Cache）
//     → getResult() 第一次計算結果，之後直接返回快取值
//     → 邏輯上是「讀取」，但需要修改快取變數
//
//   場景 2：訪問計數器（Access Counter）
//     → getInfo() 每次被調用時記錄訪問次數
//     → 邏輯上是「讀取」，但需要修改計數器
//
//   場景 3：互斥鎖（Mutex）
//     → getData() 需要加鎖保護線程安全
//     → 邏輯上是「讀取」，但需要修改鎖的狀態
//
//   場景 4：延遲初始化（Lazy Initialization）
//     → getDescription() 第一次被調用時才生成描述文字
//     → 邏輯上是「讀取」，但需要修改內部狀態
//
//   如果沒有 mutable，你就被迫二選一：
//     (a) 去掉 const（破壞介面承諾）
//     (b) 用 const_cast 強制轉型（危險且醜陋，屬於未定義行為）
//
//   mutable 提供了乾淨的解決方案。
//
//   語法：
//     mutable 數據類型 變數名;
//
//   含義：
//     即使在 const 成員函數中，也允許修改這個變數。
//


// ============================================================================
//   二、mutable 的語法與基本用法 —— 訪問計數器
// ============================================================================
//
//   最基本的 mutable 用途：在 const 函數中遞增一個「查看次數」計數器。
//   計數器不影響對象的邏輯狀態（怪物的名字、HP、攻擊力都沒變），
//   所以將它標記為 mutable 是合理的。
//

class Monster {
private:
    string name_;
    int hp_;
    int attack_;

    // mutable 成員：const 函數也能修改它
    // inspectCount_ 用來記錄被查看了多少次，邏輯上不影響怪物狀態
    mutable int inspectCount_;

public:
    Monster(const string& name, int hp, int atk)
        : name_(name), hp_(hp), attack_(atk), inspectCount_(0)
    {
    }

    // const 函數 —— 邏輯上只讀，但可以修改 inspectCount_（因為它是 mutable）
    void printInfo() const {
        inspectCount_++;   // mutable 成員可以在 const 函數中被修改
        cout << "  " << name_ << " [HP:" << hp_ << " ATK:" << attack_
             << "] (被查看了 " << inspectCount_ << " 次)" << endl;
    }

    const string& getName() const { return name_; }
    int getHp() const { return hp_; }

    // 查看被檢查了幾次——這個函數也是 const 的，因為它不修改怪物的邏輯狀態
    int getInspectCount() const { return inspectCount_; }

    // 非 const 函數：實際修改怪物狀態，不允許在 const 對象上調用
    void takeDamage(int dmg) {
        hp_ = max(0, hp_ - dmg);
    }
};

// 示範函數：展示基本 mutable 用法
void demoBasicMutable() {
    cout << "=== 二、mutable 基本用法 —— 訪問計數器 ===" << endl;

    const Monster dragon("火龍", 500, 60);  // const 對象！

    // 可以調用 const 函數，inspectCount_ 會被修改（因為是 mutable）
    dragon.printInfo();   // 被查看了 1 次
    dragon.printInfo();   // 被查看了 2 次
    dragon.printInfo();   // 被查看了 3 次

    cout << "  總共被查看：" << dragon.getInspectCount() << " 次" << endl;

    // dragon.takeDamage(10);  // 編譯錯誤！const 對象不能調用非 const 函數

    cout << endl;
}


// ============================================================================
//   三、經典應用一：計算快取（Cache）
// ============================================================================
//
//   mutable 最常見的用途之一。
//   當某個計算很昂貴時，希望只算一次然後快取結果。
//
//   設計要點：
//   (1) 快取相關的成員變數（cachedArea_、areaCached_ 等）標記為 mutable
//   (2) getter 函數標記為 const（因為邏輯上不改變物件狀態）
//   (3) setter 修改核心數據時，清除快取（讓快取失效）
//

class Circle {
private:
    double radius_;   // 核心數據：半徑

    // 快取相關 —— 全部用 mutable，因為它們只是效能優化，不影響邏輯狀態
    // areaCached_ / circumCached_ ：標記快取是否有效
    // cachedArea_ / cachedCircum_ ：存儲計算結果
    mutable bool areaCached_;
    mutable double cachedArea_;
    mutable bool circumCached_;
    mutable double cachedCircum_;

    static constexpr double PI = 3.14159265358979;

public:
    Circle(double r)
        : radius_(r > 0 ? r : 1.0)
        , areaCached_(false), cachedArea_(0)
        , circumCached_(false), cachedCircum_(0)
    {
    }

    // const 函數：邏輯上只讀，但會更新快取（mutable 成員）
    // 第一次調用時觸發計算，之後直接返回快取值
    double getArea() const {
        if (!areaCached_) {
            cout << "    [計算面積...]" << endl;
            cachedArea_ = PI * radius_ * radius_;   // mutable 可修改
            areaCached_ = true;                      // mutable 可修改
        } else {
            cout << "    [使用快取]" << endl;
        }
        return cachedArea_;
    }

    double getCircumference() const {
        if (!circumCached_) {
            cout << "    [計算周長...]" << endl;
            cachedCircum_ = 2 * PI * radius_;
            circumCached_ = true;
        } else {
            cout << "    [使用快取]" << endl;
        }
        return cachedCircum_;
    }

    // setter：修改半徑時，快取失效，需要清除
    void setRadius(double r) {
        if (r <= 0) return;
        radius_ = r;
        areaCached_ = false;     // 清除快取
        circumCached_ = false;   // 清除快取
        cout << "  半徑改為 " << radius_ << "，快取已清除" << endl;
    }

    double getRadius() const { return radius_; }
};

// 示範函數：展示快取用法
void demoCaching() {
    cout << "=== 三、計算快取（Cache）===" << endl;

    Circle c(5.0);

    // 第一次調用——觸發計算
    cout << "\n--- 第一次查詢 ---" << endl;
    cout << "  面積 = " << c.getArea() << endl;
    cout << "  周長 = " << c.getCircumference() << endl;

    // 第二次調用——使用快取，不重複計算
    cout << "\n--- 第二次查詢（快取命中）---" << endl;
    cout << "  面積 = " << c.getArea() << endl;
    cout << "  周長 = " << c.getCircumference() << endl;

    // 修改半徑——快取失效
    cout << "\n--- 修改半徑 ---" << endl;
    c.setRadius(10.0);

    // 再次查詢——重新計算
    cout << "\n--- 修改後查詢 ---" << endl;
    cout << "  面積 = " << c.getArea() << endl;
    cout << "  周長 = " << c.getCircumference() << endl;

    // 再次查詢——又是快取
    cout << "\n--- 再次查詢（快取命中）---" << endl;
    cout << "  面積 = " << c.getArea() << endl;

    cout << endl;
}


// ============================================================================
//   四、經典應用二：延遲初始化（Lazy Initialization）
// ============================================================================
//
//   有些數據的初始化成本很高，希望在「真正需要時才初始化」。
//   建構時不生成詳細描述，等到第一次調用 getDescription() 時才觸發生成。
//   之後再次調用直接返回已生成的結果，不重複生成。
//
//   與快取的差異：
//   - 快取是「算過一次就記住」
//   - 延遲初始化是「建構時不算，等到需要時才算」
//   兩者都依賴 mutable 來實現。
//

class QuestLog {
private:
    string questName_;
    int difficulty_;

    // 延遲初始化的成員 —— mutable 因為邏輯上是只讀的，只是推遲了生成時機
    mutable bool descriptionReady_;        // 描述是否已經生成
    mutable string detailedDescription_;   // 存儲生成的描述

    // 私有 const 輔助函數：模擬昂貴的描述生成操作
    void generateDescription() const {
        cout << "    [生成詳細描述... 耗時操作]" << endl;
        detailedDescription_ = "【" + questName_ + "】\n";
        detailedDescription_ += "    難度：";
        for (int i = 0; i < difficulty_; i++) {
            detailedDescription_ += "★";
        }
        detailedDescription_ += "\n";
        detailedDescription_ += "    這是一個";
        if (difficulty_ >= 4) detailedDescription_ += "極其危險的";
        else if (difficulty_ >= 3) detailedDescription_ += "具有挑戰性的";
        else if (difficulty_ >= 2) detailedDescription_ += "需要謹慎的";
        else detailedDescription_ += "簡單的";
        detailedDescription_ += "任務。冒險者需做好充分準備。";

        descriptionReady_ = true;
    }

public:
    QuestLog(const string& name, int diff)
        : questName_(name)
        , difficulty_(diff > 0 && diff <= 5 ? diff : 1)
        , descriptionReady_(false)
    {
        cout << "  [登記任務] " << questName_ << endl;
        // 注意：不在建構時生成描述，延遲到需要時
    }

    // 簡單查詢——不需要詳細描述，不觸發昂貴操作
    const string& getName() const { return questName_; }
    int getDifficulty() const { return difficulty_; }

    // 需要詳細描述時才觸發初始化
    const string& getDescription() const {
        if (!descriptionReady_) {
            generateDescription();   // 延遲初始化：第一次調用才生成
        }
        return detailedDescription_;
    }
};

// 示範函數：展示延遲初始化
void demoLazyInit() {
    cout << "=== 四、延遲初始化（Lazy Initialization）===" << endl;

    // 創建多個任務——建構時都不生成描述
    cout << "\n--- 登記任務 ---" << endl;
    QuestLog quest1("討伐火龍", 5);
    QuestLog quest2("採集草藥", 1);
    QuestLog quest3("護送商隊", 3);

    // 只用簡單查詢——不觸發描述生成
    cout << "\n--- 簡單查詢（不觸發生成）---" << endl;
    cout << "  任務1：" << quest1.getName()
         << "（難度 " << quest1.getDifficulty() << "）" << endl;
    cout << "  任務2：" << quest2.getName()
         << "（難度 " << quest2.getDifficulty() << "）" << endl;

    // 只有查看詳細描述時才觸發生成
    cout << "\n--- 查看詳細描述（觸發生成）---" << endl;
    cout << quest1.getDescription() << endl;

    // 第二次查看——直接返回，不重複生成
    cout << "\n--- 再次查看（已生成，不重複）---" << endl;
    cout << quest1.getDescription() << endl;

    // quest2 和 quest3 的描述始終沒被生成——節省了資源
    cout << "\n--- quest2 和 quest3 從未生成描述，節省了資源 ---" << endl;

    cout << endl;
}


// ============================================================================
//   五、mutable 的判斷準則
// ============================================================================
//
//   mutable 不能隨便用，以下是判斷是否適合使用 mutable 的準則：
//
//   ✅ 適合用 mutable 的情況（修改的是「實現細節」，不是「邏輯狀態」）：
//     - 快取 / 備忘錄（Cache / Memoization）
//     - 訪問計數器（Access Counter）
//     - 延遲初始化（Lazy Initialization）
//     - 互斥鎖（Mutex）—— 多線程場景
//     - 調試 / 日誌用的輔助數據
//
//   ❌ 不適合用 mutable 的情況（修改的是「對象的邏輯狀態」）：
//     - HP、攻擊力、等級等核心數據
//     - 用戶名、帳戶餘額等業務數據
//     - 任何「外部可觀察到的變化」
//
//   核心原則：
//     從外部觀察者的角度，const 函數調用前後，對象「看起來」沒有變化。
//
//   判斷公式：
//     問：去掉這個 mutable 變數，從外部觀察者來看，
//         對象的行為是否完全相同？
//       → 是（只是少了快取/計數等內部機制）→ ✅ 適合 mutable
//       → 否（對象的邏輯狀態真的不同了）    → ❌ 不適合 mutable
//


// ============================================================================
//   六、反面教材：濫用 mutable 的後果
// ============================================================================
//
//   把核心數據（如 HP、金幣）設為 mutable，然後偽裝成 const 函數去修改它們。
//   這會讓 const 完全失去保護作用，是錯誤的設計。
//

// ----- 錯誤示範 -----
class BadExample {
private:
    string name_;
    mutable int hp_;        // 錯誤：把核心數據設為 mutable
    mutable int gold_;      // 錯誤：把核心數據設為 mutable

public:
    BadExample(const string& name, int hp, int gold)
        : name_(name), hp_(hp), gold_(gold) {}

    // 這些操作明明在修改對象的邏輯狀態，卻偽裝成 const！
    void takeDamage(int dmg) const {   // 錯誤：不應該是 const
        hp_ -= dmg;
        cout << "  " << name_ << " 受傷 HP:" << hp_ << endl;
    }

    void spendGold(int amount) const { // 錯誤：不應該是 const
        gold_ -= amount;
        cout << "  花費 " << amount << " 金幣" << endl;
    }

    void print() const {
        cout << "  " << name_ << " HP:" << hp_
             << " Gold:" << gold_ << endl;
    }
};

// ----- 正確示範 -----
class GoodExample {
private:
    string name_;
    int hp_;                     // 核心數據：不用 mutable
    int gold_;                   // 核心數據：不用 mutable
    mutable int viewCount_;      // 只有輔助數據用 mutable

public:
    GoodExample(const string& name, int hp, int gold)
        : name_(name), hp_(hp), gold_(gold), viewCount_(0) {}

    // 修改狀態的函數：不是 const（正確做法）
    void takeDamage(int dmg) {
        hp_ = max(0, hp_ - dmg);
        cout << "  " << name_ << " 受傷 HP:" << hp_ << endl;
    }

    void spendGold(int amount) {
        if (amount <= gold_) {
            gold_ -= amount;
            cout << "  花費 " << amount << " 金幣" << endl;
        }
    }

    // 只讀函數：是 const，只有 viewCount_ 用 mutable（合理用途）
    void print() const {
        viewCount_++;
        cout << "  " << name_ << " HP:" << hp_
             << " Gold:" << gold_
             << " (查看次數:" << viewCount_ << ")" << endl;
    }
};

// 示範函數：對比濫用與正確使用
void demoGoodVsBad() {
    cout << "=== 六、mutable 濫用 vs 正確使用 ===" << endl;

    // ----- 濫用的後果：const 失去意義 -----
    cout << "\n--- 濫用 mutable ---" << endl;
    const BadExample bad("壞設計", 100, 500);
    bad.takeDamage(30);     // const 對象居然能受傷？！const 完全失效
    bad.spendGold(200);     // const 對象居然能花錢？！
    bad.print();
    cout << "  const 完全失去了保護作用！" << endl;

    // ----- 正確的設計 -----
    cout << "\n--- 正確使用 ---" << endl;
    const GoodExample good("好設計", 100, 500);
    // good.takeDamage(30);   // 編譯錯誤！正確攔截——const 保護起作用了
    // good.spendGold(200);   // 編譯錯誤！正確攔截
    good.print();             // 只有查看次數變化（合理的 mutable）
    good.print();

    cout << endl;
}


// ============================================================================
//   七、mutable vs const_cast 的比較
// ============================================================================
//
//   你可能會想：不用 mutable，用 const_cast 強制去掉 const 不行嗎？
//
//   答案是：不行。兩者有本質差異：
//
//   mutable（推薦）：
//     - 在聲明時明確標記意圖
//     - 編譯器知道這個成員可以在 const 中修改
//     - 合法、安全、符合 C++ 標準
//     - 其他程式員看到 mutable 就知道設計意圖
//
//   const_cast（不推薦）：
//     - 隱藏了修改意圖
//     - 對 const 對象使用是「未定義行為」（Undefined Behavior）
//     - 編譯器可能把 const 對象放在只讀記憶體中，強制修改可能導致崩潰
//     - 代碼維護者不知道為什麼要這樣做
//
//   結論：需要在 const 函數中修改成員時，永遠使用 mutable，不要用 const_cast。
//

class Counter {
private:
    int count_;

public:
    Counter() : count_(0) {}

    // 方法 1（推薦）：用 mutable
    // 如果 count_ 宣告為 mutable int count_;
    // 則可以直接寫：
    //   void increment() const { count_++; }
    // 語義清晰，編譯器知道這是特例。

    // 方法 2（不推薦）：用 const_cast
    // 強行去掉 const 限定，語義混亂，容易出錯
    void incrementBad() const {
        // const_cast<Counter*>(this) 強制去掉 this 的 const 屬性
        // 技術上可以編譯，但對 const 對象這是未定義行為！
        const_cast<Counter*>(this)->count_++;
        cout << "  const_cast 方式：count = " << count_ << endl;
    }

    int getCount() const { return count_; }
};

// 示範函數：mutable vs const_cast
void demoMutableVsConstCast() {
    cout << "=== 七、mutable vs const_cast ===" << endl;

    const Counter c;     // const 對象
    c.incrementBad();    // 技術上能跑，但行為是未定義的！
    c.incrementBad();

    // 警告：對 const 對象使用 const_cast 修改數據
    // 在 C++ 標準中是「未定義行為」（Undefined Behavior）！
    // 編譯器可能把 const 對象放在只讀記憶體中，
    // 強制修改可能導致程式崩潰。

    cout << endl;
}


// ============================================================================
//   八、綜合範例：怪物圖鑑系統
// ============================================================================
//
//   這個範例結合了 mutable 的三大用途：
//   (1) 訪問計數器（viewCount_）—— 記錄每個條目被查看了幾次
//   (2) 延遲初始化（detailGenerated_ + detailCache_）—— 詳細資料只在需要時生成
//   (3) 計算快取 —— 生成後快取，不重複生成
//
//   所有查詢函數都是 const 的，可以安全地接收 const 引用，
//   但內部的 mutable 成員允許在 const 上下文中更新輔助狀態。
//

class MonsterEntry {
private:
    // ====== 邏輯狀態（不可變的圖鑑資料）======
    string name_;
    string element_;
    int baseHp_;
    int baseAttack_;
    int rarity_;              // 1~5 星

    // ====== 輔助狀態（mutable）======
    mutable int viewCount_;               // 被查看次數（訪問計數器）
    mutable bool detailGenerated_;        // 詳細資料是否已生成（延遲初始化旗標）
    mutable string detailCache_;          // 詳細資料快取（快取 + 延遲初始化）

    // 私有輔助函數（const）：生成詳細資料
    void generateDetail() const {
        cout << "    [生成詳細資料...]" << endl;

        detailCache_ = "=== " + name_ + " ===\n";
        detailCache_ += "  屬性：" + element_ + "\n";
        detailCache_ += "  稀有度：";
        for (int i = 0; i < rarity_; i++) detailCache_ += "★";
        detailCache_ += "\n";
        detailCache_ += "  基礎 HP：" + to_string(baseHp_) + "\n";
        detailCache_ += "  基礎 ATK：" + to_string(baseAttack_) + "\n";

        // 添加弱點資訊（根據屬性推算）
        detailCache_ += "  弱點：";
        if (element_ == "火") detailCache_ += "水";
        else if (element_ == "水") detailCache_ += "雷";
        else if (element_ == "雷") detailCache_ += "土";
        else if (element_ == "土") detailCache_ += "風";
        else if (element_ == "風") detailCache_ += "火";
        else detailCache_ += "無";
        detailCache_ += "\n";

        // 添加威脅評估
        int threat = baseHp_ / 100 + baseAttack_ / 10 + rarity_;
        detailCache_ += "  威脅指數：" + to_string(threat) + "\n";

        detailGenerated_ = true;   // 標記已生成，下次直接返回快取
    }

public:
    MonsterEntry(const string& name, const string& elem,
                 int hp, int atk, int rare)
        : name_(name), element_(elem)
        , baseHp_(hp), baseAttack_(atk)
        , rarity_(rare > 0 && rare <= 5 ? rare : 1)
        , viewCount_(0)
        , detailGenerated_(false)
    {
    }

    // ====== 所有查詢函數都是 const ======

    // 簡要資訊——只讀，但遞增查看次數（mutable）
    void printBrief() const {
        viewCount_++;
        cout << "  ";
        for (int i = 0; i < rarity_; i++) cout << "★";
        cout << " " << name_ << " [" << element_ << "]"
             << " (查看:" << viewCount_ << ")" << endl;
    }

    // 詳細資訊——延遲生成 + 快取（結合兩種 mutable 用法）
    const string& getDetail() const {
        viewCount_++;
        if (!detailGenerated_) {
            generateDetail();   // 延遲初始化：第一次才生成
        }
        return detailCache_;    // 之後直接返回快取
    }

    // 其他 getter
    const string& getName() const { return name_; }
    int getViewCount() const { return viewCount_; }
    int getRarity() const { return rarity_; }
};

// 展示圖鑑——接收 const 陣列，所有 mutable 操作在 const 上下文中正常工作
void showEncyclopedia(const MonsterEntry entries[], int count) {
    cout << "\n  +---------------------------+" << endl;
    cout << "  |     怪 物 圖 鑑           |" << endl;
    cout << "  +---------------------------+" << endl;

    for (int i = 0; i < count; i++) {
        entries[i].printBrief();   // const 函數，但內部 mutable 成員被更新
    }
}

// 查看特定怪物詳情——接收 const 引用
void showDetail(const MonsterEntry& entry) {
    cout << "\n" << entry.getDetail();   // 延遲生成 + 快取
}

// 示範函數：綜合範例
void demoComprehensive() {
    cout << "=== 八、綜合範例：怪物圖鑑系統 ===" << endl;

    // 創建圖鑑條目
    const int COUNT = 4;
    MonsterEntry entries[COUNT] = {
        MonsterEntry("炎龍王", "火", 800, 70, 5),
        MonsterEntry("冰霜狼", "水", 400, 45, 3),
        MonsterEntry("雷電鷹", "雷", 300, 60, 4),
        MonsterEntry("泥土蟲", "土", 600, 20, 1)
    };

    // 瀏覽圖鑑——只觸發簡要資訊，不生成詳細資料
    showEncyclopedia(entries, COUNT);

    // 查看炎龍王的詳細資料——第一次觸發生成
    cout << "\n--- 查看炎龍王詳情 ---";
    showDetail(entries[0]);

    // 再次查看——使用快取，不重複生成
    cout << "\n--- 再次查看炎龍王詳情（快取命中）---";
    showDetail(entries[0]);

    // 查看雷電鷹——觸發生成
    cout << "\n--- 查看雷電鷹詳情 ---";
    showDetail(entries[2]);

    // 查看各怪物被查看次數
    cout << "\n--- 查看統計 ---" << endl;
    for (int i = 0; i < COUNT; i++) {
        cout << "  " << entries[i].getName()
             << "：被查看 " << entries[i].getViewCount() << " 次" << endl;
    }

    // 冰霜狼和泥土蟲的詳細描述從未被生成——節省資源
    cout << "\n  冰霜狼和泥土蟲的詳細描述從未生成，節省了資源！" << endl;

    cout << endl;
}


// ============================================================================
//   九、本課重點回顧
// ============================================================================
//
//   | 概念            | 說明                                               |
//   |-----------------|----------------------------------------------------|
//   | mutable 語法    | mutable int count_;                                |
//   | mutable 含義    | 即使在 const 成員函數中也可以修改                   |
//   | 適用場景        | 快取、計數器、延遲初始化、互斥鎖                    |
//   | 核心原則        | 只用於「不影響邏輯狀態」的實現細節                   |
//   | 判斷方法        | 去掉 mutable 變數，外部觀察是否一樣？               |
//   | vs const_cast   | mutable 合法安全，const_cast 是未定義行為            |
//   | 濫用危險        | 把核心數據設為 mutable 會讓 const 完全失效           |
//   | 設計指導        | mutable 成員應該盡可能少                            |
//


// ============================================================================
//  【LeetCode 實戰範例】LeetCode 146. LRU Cache
//    —— 刻意選一個「不該用 mutable」的例子，用來標出 mutable 的邊界
// ============================================================================
//
//  題目：設計一個容量固定的 LRU（Least Recently Used）快取，
//        get(key)：存在則回傳值，否則回傳 -1；
//        put(key, value)：寫入；若超過容量，淘汰「最久未使用」的項目。
//        兩個操作都要求平均 O(1)。
//
//  為什麼放在 mutable 這一課：
//    直覺上 get() 是「查詢」，很多人會想「那把 recency 標成 mutable，
//    就能把 get() 寫成 const 了」。這個直覺是錯的，而且錯得很有代表性。
//
//    判準是「有沒有改變外界可觀察的狀態」。LRU 的 get() 會把該鍵移到
//    最近使用端，而這個順序直接決定「下一次 put() 會淘汰誰」——
//    也就是說，呼叫過 get() 之後，後續 put() 的可觀察行為就不同了。
//    因此 recency 是不折不扣的邏輯狀態，get() 本來就該是非 const。
//
//    對照第 23 課前面的 DNS / 計算快取：那些快取命中與否，
//    完全不影響任何一次查詢的回傳值 —— 那才是 mutable 的正當場景。
//    一句話：「加速用的快取」可以 mutable；
//            「會改變未來行為的狀態」不可以。
//
//  實作：雜湊表 + 雙向鏈結串列（頭端 = 最近使用，尾端 = 最久未使用）。
//        為了不依賴 <unordered_map> 以外的東西，這裡用標準容器組合。
//  複雜度：get / put 平均 O(1)；空間 O(capacity)。
// ============================================================================

class LRUCache {
private:
    struct Node {
        int key;
        int value;
        int prev;   // 前一個節點在 nodes_ 中的索引，-1 表示無
        int next;   // 後一個節點在 nodes_ 中的索引，-1 表示無
    };

    int capacity_;
    vector<Node> nodes_;            // 節點池（以索引取代指標，避免手動記憶體管理）
    unordered_map<int, int> index_; // key → nodes_ 的索引
    int head_;                      // 最近使用端
    int tail_;                      // 最久未使用端
    int freeList_;                  // 已回收節點的串列頭

    // 把節點 i 從鏈結串列中摘下來
    void unlink(int i) {
        int p = nodes_[i].prev, n = nodes_[i].next;
        if (p != -1) nodes_[p].next = n; else head_ = n;
        if (n != -1) nodes_[n].prev = p; else tail_ = p;
        nodes_[i].prev = nodes_[i].next = -1;
    }

    // 把節點 i 掛到最近使用端
    void linkFront(int i) {
        nodes_[i].prev = -1;
        nodes_[i].next = head_;
        if (head_ != -1) nodes_[head_].prev = i;
        head_ = i;
        if (tail_ == -1) tail_ = i;
    }

    int allocNode() {
        if (freeList_ != -1) {
            int i = freeList_;
            freeList_ = nodes_[i].next;
            return i;
        }
        nodes_.push_back(Node{0, 0, -1, -1});
        return static_cast<int>(nodes_.size()) - 1;
    }

public:
    explicit LRUCache(int capacity)
        : capacity_(capacity), head_(-1), tail_(-1), freeList_(-1) {
        nodes_.reserve(static_cast<size_t>(capacity > 0 ? capacity : 1));
    }

    // ★ 刻意「不是」const：它會更新 recency，而 recency 是邏輯狀態。
    //   若在此把 recency 標成 mutable 並宣告 get() const，
    //   等於對呼叫端謊稱「這次呼叫不影響之後的行為」—— 但它確實影響。
    int get(int key) {
        auto it = index_.find(key);
        if (it == index_.end()) return -1;
        int i = it->second;
        unlink(i);          // 這兩行就是「可觀察狀態被改變」的地方
        linkFront(i);
        return nodes_[i].value;
    }

    void put(int key, int value) {
        if (capacity_ <= 0) return;
        auto it = index_.find(key);
        if (it != index_.end()) {          // 已存在：更新值並提升為最近使用
            int i = it->second;
            nodes_[i].value = value;
            unlink(i);
            linkFront(i);
            return;
        }
        if (static_cast<int>(index_.size()) == capacity_) {   // 滿了 → 淘汰尾端
            int victim = tail_;
            index_.erase(nodes_[victim].key);
            unlink(victim);
            nodes_[victim].next = freeList_;                  // 回收節點
            freeList_ = victim;
        }
        int i = allocNode();
        nodes_[i].key = key;
        nodes_[i].value = value;
        linkFront(i);
        index_[key] = i;
    }

    // 這兩個才是真正的 const 查詢：不改變任何可觀察狀態，
    // 也不影響之後 put() 會淘汰誰。
    size_t size()     const { return index_.size(); }
    bool   contains(int key) const { return index_.find(key) != index_.end(); }

    // 由最近到最久列出目前內容 —— 純觀察，不動 recency，所以可以是 const
    string dumpOrder() const {
        string s;
        for (int i = head_; i != -1; i = nodes_[i].next) {
            if (!s.empty()) s += " -> ";
            s += to_string(nodes_[i].key) + ":" + to_string(nodes_[i].value);
        }
        return s.empty() ? "(空)" : s;
    }
};

// ============================================================================
//   主程式：執行所有範例
// ============================================================================

int main() {
    cout << "============================================" << endl;
    cout << "   第 23 課：mutable 關鍵字 —— 完整總結" << endl;
    cout << "============================================" << endl;
    cout << endl;

    // 二、基本用法：訪問計數器
    demoBasicMutable();

    // 三、計算快取
    demoCaching();

    // 四、延遲初始化
    demoLazyInit();

    // 六、濫用 vs 正確使用
    demoGoodVsBad();

    // 七、mutable vs const_cast
    demoMutableVsConstCast();

    // 八、綜合範例
    demoComprehensive();

    // ========================================================================
    //   LeetCode 146. LRU Cache —— mutable 的邊界示範
    // ========================================================================
    cout << "============================================" << endl;
    cout << "   LeetCode 146. LRU Cache（mutable 的邊界）" << endl;
    cout << "============================================" << endl;

    // LeetCode 官方範例：capacity = 2
    LRUCache lru(2);
    lru.put(1, 1);
    lru.put(2, 2);
    cout << "  put(1,1), put(2,2)   內容：" << lru.dumpOrder() << endl;

    cout << "  get(1)  = " << lru.get(1) << "   （預期 1）" << endl;
    cout << "    → get(1) 之後內容：" << lru.dumpOrder()
         << "   （1 被提升為最近使用）" << endl;

    lru.put(3, 3);   // 容量已滿 → 淘汰最久未使用的 key 2
    cout << "  put(3,3) 觸發淘汰    內容：" << lru.dumpOrder() << endl;
    cout << "  get(2)  = " << lru.get(2) << "  （預期 -1，已被淘汰）" << endl;

    lru.put(4, 4);   // 再次淘汰最久未使用的 key 1
    cout << "  put(4,4) 觸發淘汰    內容：" << lru.dumpOrder() << endl;
    cout << "  get(1)  = " << lru.get(1) << "  （預期 -1，已被淘汰）" << endl;
    cout << "  get(3)  = " << lru.get(3) << "   （預期 3）" << endl;
    cout << "  get(4)  = " << lru.get(4) << "   （預期 4）" << endl;

    cout << "\n  --- 為什麼 get() 不能靠 mutable 變成 const ---" << endl;
    LRUCache demo(2);
    demo.put(10, 100);
    demo.put(20, 200);
    cout << "  情境 A：put(10),put(20) 後直接 put(30) →" << endl;
    {
        LRUCache a(2);
        a.put(10, 100); a.put(20, 200);
        a.put(30, 300);
        cout << "          淘汰結果 " << a.dumpOrder()
             << "（10 被淘汰）" << endl;
    }
    cout << "  情境 B：put(10),put(20) 後先 get(10) 再 put(30) →" << endl;
    {
        LRUCache b(2);
        b.put(10, 100); b.put(20, 200);
        b.get(10);                     // 只是「查詢」，卻改變了淘汰對象
        b.put(30, 300);
        cout << "          淘汰結果 " << b.dumpOrder()
             << "（改成 20 被淘汰）" << endl;
    }
    cout << "  ↑ 同樣的 put(30)，只因中間多了一次 get()，淘汰對象就不同。" << endl;
    cout << "    get() 改變了外界可觀察的行為，所以它是邏輯狀態的修改，" << endl;
    cout << "    不能用 mutable 包裝成 const —— 這正是 mutable 的邊界。" << endl;

    // 真正的 const 查詢：不動 recency，可以安全標成 const
    const LRUCache& roLru = lru;
    cout << "\n  真正的 const 查詢（不影響淘汰順序）：" << endl;
    cout << "    size()        = " << roLru.size() << endl;
    cout << "    contains(3)   = " << (roLru.contains(3) ? "true" : "false") << endl;
    cout << "    contains(999) = " << (roLru.contains(999) ? "true" : "false") << endl;
    // roLru.get(3);   // ❌ 編譯錯誤：get() 是非 const —— 而且它本來就該是
    cout << endl;

    cout << "============================================" << endl;
    cout << "   總結完畢" << endl;
    cout << "============================================" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "summary.cpp" -o l23_sum
// 執行: ./l23_sum        (rc=0)

// === 預期輸出 ===
// ============================================
//    第 23 課：mutable 關鍵字 —— 完整總結
// ============================================
//
// === 二、mutable 基本用法 —— 訪問計數器 ===
//   火龍 [HP:500 ATK:60] (被查看了 1 次)
//   火龍 [HP:500 ATK:60] (被查看了 2 次)
//   火龍 [HP:500 ATK:60] (被查看了 3 次)
//   總共被查看：3 次
//
// === 三、計算快取（Cache）===
//
// --- 第一次查詢 ---
//   面積 =     [計算面積...]
// 78.5398
//   周長 =     [計算周長...]
// 31.4159
//
// --- 第二次查詢（快取命中）---
//   面積 =     [使用快取]
// 78.5398
//   周長 =     [使用快取]
// 31.4159
//
// --- 修改半徑 ---
//   半徑改為 10，快取已清除
//
// --- 修改後查詢 ---
//   面積 =     [計算面積...]
// 314.159
//   周長 =     [計算周長...]
// 62.8319
//
// --- 再次查詢（快取命中）---
//   面積 =     [使用快取]
// 314.159
//
// === 四、延遲初始化（Lazy Initialization）===
//
// --- 登記任務 ---
//   [登記任務] 討伐火龍
//   [登記任務] 採集草藥
//   [登記任務] 護送商隊
//
// --- 簡單查詢（不觸發生成）---
//   任務1：討伐火龍（難度 5）
//   任務2：採集草藥（難度 1）
//
// --- 查看詳細描述（觸發生成）---
//     [生成詳細描述... 耗時操作]
// 【討伐火龍】
//     難度：★★★★★
//     這是一個極其危險的任務。冒險者需做好充分準備。
//
// --- 再次查看（已生成，不重複）---
// 【討伐火龍】
//     難度：★★★★★
//     這是一個極其危險的任務。冒險者需做好充分準備。
//
// --- quest2 和 quest3 從未生成描述，節省了資源 ---
//
// === 六、mutable 濫用 vs 正確使用 ===
//
// --- 濫用 mutable ---
//   壞設計 受傷 HP:70
//   花費 200 金幣
//   壞設計 HP:70 Gold:300
//   const 完全失去了保護作用！
//
// --- 正確使用 ---
//   好設計 HP:100 Gold:500 (查看次數:1)
//   好設計 HP:100 Gold:500 (查看次數:2)
//
// === 七、mutable vs const_cast ===
//   const_cast 方式：count = 1
//   const_cast 方式：count = 2
//
// === 八、綜合範例：怪物圖鑑系統 ===
//
//   +---------------------------+
//   |     怪 物 圖 鑑           |
//   +---------------------------+
//   ★★★★★ 炎龍王 [火] (查看:1)
//   ★★★ 冰霜狼 [水] (查看:1)
//   ★★★★ 雷電鷹 [雷] (查看:1)
//   ★ 泥土蟲 [土] (查看:1)
//
// --- 查看炎龍王詳情 ---
//     [生成詳細資料...]
// === 炎龍王 ===
//   屬性：火
//   稀有度：★★★★★
//   基礎 HP：800
//   基礎 ATK：70
//   弱點：水
//   威脅指數：20
//
// --- 再次查看炎龍王詳情（快取命中）---
// === 炎龍王 ===
//   屬性：火
//   稀有度：★★★★★
//   基礎 HP：800
//   基礎 ATK：70
//   弱點：水
//   威脅指數：20
//
// --- 查看雷電鷹詳情 ---
//     [生成詳細資料...]
// === 雷電鷹 ===
//   屬性：雷
//   稀有度：★★★★
//   基礎 HP：300
//   基礎 ATK：60
//   弱點：土
//   威脅指數：13
//
// --- 查看統計 ---
//   炎龍王：被查看 3 次
//   冰霜狼：被查看 1 次
//   雷電鷹：被查看 2 次
//   泥土蟲：被查看 1 次
//
//   冰霜狼和泥土蟲的詳細描述從未生成，節省了資源！
//
// ============================================
//    LeetCode 146. LRU Cache（mutable 的邊界）
// ============================================
//   put(1,1), put(2,2)   內容：2:2 -> 1:1
//   get(1)  = 1   （預期 1）
//     → get(1) 之後內容：1:1 -> 2:2   （1 被提升為最近使用）
//   put(3,3) 觸發淘汰    內容：3:3 -> 1:1
//   get(2)  = -1  （預期 -1，已被淘汰）
//   put(4,4) 觸發淘汰    內容：4:4 -> 3:3
//   get(1)  = -1  （預期 -1，已被淘汰）
//   get(3)  = 3   （預期 3）
//   get(4)  = 4   （預期 4）
//
//   --- 為什麼 get() 不能靠 mutable 變成 const ---
//   情境 A：put(10),put(20) 後直接 put(30) →
//           淘汰結果 30:300 -> 20:200（10 被淘汰）
//   情境 B：put(10),put(20) 後先 get(10) 再 put(30) →
//           淘汰結果 30:300 -> 10:100（改成 20 被淘汰）
//   ↑ 同樣的 put(30)，只因中間多了一次 get()，淘汰對象就不同。
//     get() 改變了外界可觀察的行為，所以它是邏輯狀態的修改，
//     不能用 mutable 包裝成 const —— 這正是 mutable 的邊界。
//
//   真正的 const 查詢（不影響淘汰順序）：
//     size()        = 2
//     contains(3)   = true
//     contains(999) = false
//
// ============================================
//    總結完畢
// ============================================
