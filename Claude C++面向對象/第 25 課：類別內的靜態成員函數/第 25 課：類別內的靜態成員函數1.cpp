// =============================================================================
//  第 25 課：類別內的靜態成員函數 1  —  基礎語法與「沒有 this」
// =============================================================================
//
// 【主題資訊 Information】
//   宣告:  class C { static R f(Args...); };
//   呼叫:  C::f(...)     （推薦，語意清楚）
//          obj.f(...)    （合法，但物件根本沒被使用）
//   標準版本: C++98；本檔的 inline static 資料成員初始化為 C++17
//   複雜度: 與一般函式相同，且少傳一個隱含的 this 參數
//   標頭檔: <string>
//
// 【詳細解釋 Explanation】
//
// 【1. 一般成員函式其實偷偷收了一個參數】
//   編譯器把成員函式大致轉換成這樣：
//       void Bullet::fire()              →  void fire(Bullet* this)
//       static void Bullet::printStats() →  void printStats()        // 沒有 this
//   一般函式靠 this 才知道「要動哪一顆子彈的 damage_」；
//   靜態函式沒有 this，所以編譯器根本無從得知是哪個物件 ——
//   這就是第 48~50 行那三行被註解掉的真正原因，
//   不是「規定不能寫」，而是「寫了也無法決定要用誰的資料」。
//
// 【2. 靜態函式能碰什麼、不能碰什麼】
//   能碰：靜態成員變數（totalFired_、totalHit_）、其他靜態成員函式。
//         本檔 printStats() 裡呼叫 getHitRate() 就是靜態呼叫靜態，完全合法。
//   不能碰：非靜態成員變數、非靜態成員函式。
//   反過來，一般成員函式可以自由呼叫靜態成員（它本來就不需要額外資訊），
//   所以 fire() 裡遞增 totalFired_ 沒有任何問題。
//
// 【3. 為什麼 getHitRate 要先擋 totalFired_ == 0】
//   命中率是 totalHit_ / totalFired_。整數 0 當除數是未定義行為；
//   即使先轉成 double，0.0/0.0 得到的是 NaN，印出來會是 "-nan" 或 "nan"，
//   而不是使用者期待的 0%。所以除法前一定要處理分母為零。
//   本檔 resetStats() 之後立刻 printStats()，走的正是這條保護路徑。
//
// 【4. 呼叫方式：兩種寫法，同一份機器碼】
//   normal.printStats() 與 Bullet::printStats() 產生的程式碼完全相同 ——
//   編譯器只從 normal 取出「型別是 Bullet」，物件本身沒被使用。
//   但要注意物件「運算式」仍然會被求值：
//   makeBullet().printStats() 裡的 makeBullet() 照樣會執行。
//
// 【概念補充 Concept Deep Dive】
//   * 靜態成員函式不能加 const、不能是 virtual、不能有 ref-qualifier ——
//     這三者修飾的都是 this，沒有 this 就無從修飾。
//   * 它的位址型別是普通函式指標 R(*)(Args)，不是成員函式指標 R(C::*)(Args)，
//     所以可以直接當作 C API 的 callback。
//   * 若在類別外定義靜態成員函式，定義處「不可再寫 static」：
//       int Bullet::getTotalFired() { ... }   // 正確，沒有 static
//   * resetStats() 這種「重置全域狀態」的函式在單元測試很有用，
//     因為靜態狀態會跨測試案例殘留，測試前需要明確歸零。
//   * totalFired_++ 是非原子的讀-改-寫；多執行緒同時開火就是資料競爭
//     （未定義行為），需要 std::atomic<int> 或加鎖。
//
// 【注意事項 Pay Attention】
//   1. 靜態函式沒有 this，因此碰不到非靜態成員，也不能加 const / virtual。
//   2. 類別外定義時不要重複寫 static。
//   3. 除法前務必檢查分母，0.0/0.0 得到 NaN 而不是 0。
//   4. 用 obj.f() 呼叫時物件運算式仍會被求值（副作用照樣發生）。
//   5. 靜態計數器在多執行緒下需自行同步。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】靜態成員函數基礎
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼靜態成員函數不能存取非靜態成員變數?
//     答：因為它沒有 this 指標。非靜態成員變數是「每個物件各一份」，
//         要存取就必須先知道是哪一個物件，而這個資訊正是由 this 攜帶的。
//         靜態函式可以在完全沒有物件的情況下被呼叫（Bullet::printStats()），
//         編譯器自然無從決定要用誰的 damage_。
//     追問：那要怎麼讓靜態函式操作某個物件?
//         → 把物件當參數明確傳進去：static void show(const Bullet& b)。
//         此時物件是顯式的，不需要 this，而且因為它仍是成員函式，
//         還能存取那個物件的 private 成員。
//
// 🔥 Q2. 非靜態成員函數可以呼叫靜態成員函數嗎?反過來呢?
//     答：非靜態呼叫靜態：永遠可以。靜態函式不需要任何額外資訊，
//         所以在有 this 的環境裡呼叫它毫無問題（本檔 fire() 遞增 totalFired_）。
//         靜態呼叫非靜態：不行 —— 除非自己提供一個物件，
//         例如 static void run(Bullet& b) { b.fire(); }。
//     追問：那 printStats() 裡呼叫 getHitRate() 為什麼可以?
//         → 因為 getHitRate() 本身也是靜態函式，兩者都不需要 this。
//
// ⚠️ 陷阱. 「normal.printStats() 是用物件呼叫的，
//            所以它印出來的統計應該只算 normal 這顆子彈的。」
//     答：不是。printStats() 讀的是 totalFired_ / totalHit_，
//         這兩個是「整個類別共用一份」的靜態成員，
//         統計的是所有子彈的總和。
//         normal.printStats() 與 Bullet::printStats() 輸出完全相同 ——
//         本檔輸出可以直接驗證這一點。
//     為什麼會錯：把「用 obj. 呼叫」直覺理解成「作用範圍限縮到 obj」。
//         但點運算子在這裡只是名稱查找的入口，
//         函式實際能看到的資料由「它是不是靜態」決定，與寫法無關。
//         正因為容易誤解，才建議一律寫成 Class::func()。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】—— 從缺，理由如下
//   「函式有沒有 this」是語言機制，LeetCode 判題只驗輸入輸出。
//   本檔改以「快取命中率統計」的實務範例呈現同一個模式：
//   計數屬於整個快取而非單筆查詢，因此天生就該是靜態成員 + 靜態查詢函式。
//
// =============================================================================

#include <iostream>
#include <string>
using namespace std;

class Bullet {
private:
    int damage_;
    string type_;

    // 靜態成員變數, 所有子彈共享同一份統計數據
    inline static int totalFired_ = 0;
    inline static int totalHit_ = 0;

public:
    // 成員初始化一律依「宣告順序」執行（damage_ 先於 type_），
    // 與初始化列表寫的順序無關。這裡讓兩者一致，
    // 否則 g++ -Wall 會發出 -Wreorder 警告提醒兩者不符。
    Bullet(const string& type, int dmg)
        : damage_(dmg), type_(type)
    {
    }

    // 非靜態函數：有 this，可以訪問一切
    void fire() {
        totalFired_++;
        cout << "  發射 " << type_ << "（傷害:" << damage_ << "）" << endl;
    }

    void hit() {
        totalHit_++;
        cout << "  命中！" << type_ << " 造成 " << damage_ << " 傷害" << endl;
    }

    // ====== 靜態成員函數：沒有 this ======
    // 靜態函數只能訪問靜態成員（totalFired_ 和 totalHit_），不能訪問 damage_ 和 type_
    // 靜態函數也不能調用非靜態函數（fire() 和 hit()），因為它們需要 this 指針
    static int getTotalFired() { return totalFired_; }
    static int getTotalHit() { return totalHit_; }

    static double getHitRate() {
        if (totalFired_ == 0) return 0.0;
        return static_cast<double>(totalHit_) / totalFired_ * 100.0;
    }

    static void printStats() {
        cout << "  發射：" << totalFired_
             << "  命中：" << totalHit_
             << "  命中率：" << getHitRate() << "%" << endl;

        // 以下會編譯錯誤！靜態函數沒有 this
        // cout << damage_;    // ❌ 不能訪問非靜態成員
        // cout << type_;      // ❌ 不能訪問非靜態成員
        // fire();             // ❌ 不能調用非靜態函數
    }

    static void resetStats() {
        totalFired_ = 0;
        totalHit_ = 0;
        cout << "  統計數據已重置" << endl;
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】快取命中率統計
//   情境：服務端加了一層記憶體快取，上線後要回答「快取到底有沒有用」。
//         命中／未命中的計數屬於「整個快取」，不屬於任何一次查詢，
//         所以是天生的靜態成員；對外的查詢介面自然也是靜態函式 ——
//         監控程式想看命中率時，不應該還要先去生一個查詢物件出來。
//   注意：命中率的分母可能為 0（剛啟動、還沒有任何查詢），
//         必須先擋掉再做除法，否則得到的是 NaN 而不是 0%。
// -----------------------------------------------------------------------------
class CacheStats {
private:
    inline static long hits_   = 0;
    inline static long misses_ = 0;

public:
    static void recordHit()  { ++hits_;   }
    static void recordMiss() { ++misses_; }

    static long total() { return hits_ + misses_; }

    static double hitRate() {
        const long t = total();
        if (t == 0) return 0.0;          // 沒有任何查詢時回 0，不做除法
        return static_cast<double>(hits_) / static_cast<double>(t) * 100.0;
    }

    static void report(const string& phase) {
        cout << "  [" << phase << "] 查詢 " << total()
             << " 次，命中 " << hits_ << "，未命中 " << misses_
             << "，命中率 " << hitRate() << "%" << endl;
    }

    static void reset() { hits_ = 0; misses_ = 0; }
};

int main() {
    cout << "=== 靜態成員函數基礎 ===" << endl;

    Bullet normal("普通彈", 10);
    Bullet fire("火焰彈", 25);

    // 射擊
    normal.fire();
    normal.fire();
    fire.fire();
    normal.hit();
    fire.hit();
    fire.fire();
    fire.hit();

    // 通過類別名調用靜態函數——不需要對象！
    cout << "\n--- 戰鬥統計（類別名調用）---" << endl;
    Bullet::printStats();

    // 也可以通過對象調用（但不推薦）
    cout << "\n--- 通過對象調用（不推薦）---" << endl;
    normal.printStats();   // 和 Bullet::printStats() 完全一樣

    // 重置
    cout << "\n--- 重置 ---" << endl;
    Bullet::resetStats();
    Bullet::printStats();
    cout << "  ↑ 重置後分母為 0，getHitRate() 走保護路徑回傳 0，" << endl;
    cout << "    而不是做 0/0 得到 NaN。" << endl;

    cout << "\n=== 日常實務：快取命中率統計 ===" << endl;
    // 完全不需要任何物件，直接用 Class::func() 呼叫
    CacheStats::report("啟動時");

    // 模擬一段查詢：10 次查詢中 7 次命中
    for (int i = 0; i < 7; ++i) CacheStats::recordHit();
    for (int i = 0; i < 3; ++i) CacheStats::recordMiss();
    CacheStats::report("暖機後");

    // 再模擬一段：快取效果變好
    for (int i = 0; i < 90; ++i) CacheStats::recordHit();
    for (int i = 0; i < 10; ++i) CacheStats::recordMiss();
    CacheStats::report("穩定後");

    CacheStats::reset();
    CacheStats::report("重置後");
    cout << "  ↑ 監控端不必先建立任何物件就能取得統計 ——" << endl;
    cout << "    這正是靜態成員函式存在的理由。" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第 25 課：類別內的靜態成員函數1.cpp -o static_func1

// === 預期輸出 ===
// === 靜態成員函數基礎 ===
//   發射 普通彈（傷害:10）
//   發射 普通彈（傷害:10）
//   發射 火焰彈（傷害:25）
//   命中！普通彈 造成 10 傷害
//   命中！火焰彈 造成 25 傷害
//   發射 火焰彈（傷害:25）
//   命中！火焰彈 造成 25 傷害
//
// --- 戰鬥統計（類別名調用）---
//   發射：4  命中：3  命中率：75%
//
// --- 通過對象調用（不推薦）---
//   發射：4  命中：3  命中率：75%
//
// --- 重置 ---
//   統計數據已重置
//   發射：0  命中：0  命中率：0%
//   ↑ 重置後分母為 0，getHitRate() 走保護路徑回傳 0，
//     而不是做 0/0 得到 NaN。
//
// === 日常實務：快取命中率統計 ===
//   [啟動時] 查詢 0 次，命中 0，未命中 0，命中率 0%
//   [暖機後] 查詢 10 次，命中 7，未命中 3，命中率 70%
//   [穩定後] 查詢 110 次，命中 97，未命中 13，命中率 88.1818%
//   [重置後] 查詢 0 次，命中 0，未命中 0，命中率 0%
//   ↑ 監控端不必先建立任何物件就能取得統計 ——
//     這正是靜態成員函式存在的理由。
