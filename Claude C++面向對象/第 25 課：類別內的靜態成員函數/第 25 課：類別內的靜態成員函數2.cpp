// =============================================================================
//  第 25 課：類別內的靜態成員函數 2  —  存取規則與「用參數代替 this」
// =============================================================================
//
// 【主題資訊 Information】
//   非靜態成員函式: 有 this → 可存取靜態 + 非靜態成員
//   靜態成員函式:   無 this → 只能存取靜態成員
//                   但可透過「參數」存取傳進來的物件，且仍享有 private 權限
//   標準版本: C++98；inline static 資料成員初始化為 C++17
//   標頭檔: <string>
//
// 【詳細解釋 Explanation】
//
// 【1. 一張表講完存取規則】
//   ┌────────────────────┬──────────────┬──────────────┐
//   │                    │ 非靜態函式    │ 靜態函式      │
//   ├────────────────────┼──────────────┼──────────────┤
//   │ this               │ 有           │ 沒有          │
//   │ 存取非靜態成員      │ 可以          │ 不行（註 1）  │
//   │ 存取靜態成員        │ 可以          │ 可以          │
//   │ 呼叫非靜態函式      │ 可以          │ 不行（註 1）  │
//   │ 呼叫靜態函式        │ 可以          │ 可以          │
//   └────────────────────┴──────────────┴──────────────┘
//   註 1：指「不能隱含地透過 this 存取」。若把物件當參數傳進來，
//         靜態函式一樣能存取它的成員 —— 這正是本檔 compare() 的做法。
//
// 【2. 關鍵觀念：限制的是 this，不是權限】
//   很多人把「靜態函式不能存取非靜態成員」記成「靜態函式權限比較低」，
//   這是錯的。compare() 是靜態函式，卻能讀 a.name_、a.hp_ 這些 private 成員 ——
//   因為存取控制是「類別層級」的，只要是 Character 的成員函式就有完整權限。
//   靜態函式真正缺的只有一樣東西：「預設要對哪個物件動手」的資訊。
//   把物件明確傳進來，這個缺口就補上了。
//
// 【3. 為什麼這件事很有用】
//   (a) 比較兩個物件：a.hp_ > b.hp_ 這種「對稱」的運算，
//       寫成非靜態成員函式會變得不對稱（a.compareWith(b)），
//       寫成靜態函式 Character::compare(a, b) 語意乾淨得多。
//   (b) 排序用的比較函式：std::sort 需要的是「吃兩個元素」的可呼叫物件，
//       靜態成員函式剛好符合，而且能存取 private 欄位，不必開放 getter。
//       下方 LeetCode 與實務範例都是這個用法。
//   (c) 對稱的二元運算（相等、距離、合併）同理。
//
// 【4. 靜態函式 vs friend 全域函式】
//   全域函式要存取 private 就得宣告成 friend。
//   靜態成員函式天生就有權限，不需要 friend，而且名字有歸屬
//   （Character::compare 而不是滿天飛的 compare）。
//   除非需要支援「左運算元不是本類別」的隱式轉換（典型是 operator<< 或
//   對稱的 operator+），否則靜態成員函式通常是更好的選擇。
//
// 【概念補充 Concept Deep Dive】
//   * printInfo() 宣告成 const，const 修飾的是 *this；
//     靜態函式沒有 this，所以連寫 const 的資格都沒有（語法直接不允許）。
//   * 靜態成員函式的位址型別是普通函式指標 R(*)(Args)，
//     這讓它能直接餵給 std::sort、qsort 或任何 C API callback。
//     非靜態成員函式的位址是 R(C::*)(Args)，型別完全不同、不能互換。
//   * 用作 std::sort 的比較函式時必須滿足 strict weak ordering
//     （簡單說：不能對相等的元素回傳 true）。
//     違反的話 sort 的行為是未定義的，實務上真的會發生越界崩潰。
//   * 本檔 compare() 三分支寫法（>、<、否則相等）是正確的，
//     但若拿來當排序用的比較函式，要記得「相等時回傳 false」。
//
// 【注意事項 Pay Attention】
//   1. 靜態函式缺的是 this，不是存取權限；傳入物件後照樣能碰 private。
//   2. 靜態函式不能加 const（沒有 this 可修飾）。
//   3. 當排序比較函式時必須滿足 strict weak ordering，否則行為未定義。
//   4. 靜態函式可當 C API callback，非靜態不行（型別不同）。
//   5. 靜態函式讀寫的靜態資料在多執行緒下需自行同步。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】靜態成員函數的存取規則
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 靜態成員函數能不能存取 private 成員?
//     答：能。存取控制是類別層級的 —— 只要是該類別的成員函式，
//         不論靜不靜態都有完整權限。
//         本檔 compare() 就直接讀了 a.name_、a.hp_ 這些 private 欄位。
//         靜態函式缺的只是 this（不知道預設要操作哪個物件），
//         把物件當參數傳進來就補上了。
//     追問：那和 friend 全域函式比，優勢在哪?
//         → 不需要 friend 宣告、名字有歸屬不會撞名、
//         也不會讓類別的封裝出現一個對外的破口。
//
// 🔥 Q2. 為什麼常把排序用的比較函式寫成 static 成員函式?
//     答：兩個理由。第一，std::sort 需要「吃兩個元素」的可呼叫物件，
//         靜態成員函式的位址是普通函式指標 R(*)(const T&, const T&)，
//         型別剛好相符；非靜態成員函式的位址是成員函式指標，不能直接用。
//         第二，它仍是成員，可以直接比較 private 欄位，
//         不必為了排序而對外開放一堆 getter。
//     追問：比較函式有什麼硬性要求?
//         → 必須滿足 strict weak ordering，最常見的錯誤是
//         對相等元素回傳 true（例如把 <= 當比較函式），
//         這會讓 std::sort 的行為變成未定義，實務上真的會崩潰。
//
// ⚠️ 陷阱. 「靜態函式碰不到非靜態成員，
//            所以要比較兩個物件就只能改寫成非靜態的 a.compareWith(b)。」
//     答：不必。靜態函式碰不到的是「隱含的 this 所指的那個物件」，
//         不是「所有物件」。把物件當參數明確傳進去
//         （Character::compare(a, b)），一樣能存取它們的 private 成員。
//         而且對稱的二元運算寫成靜態函式語意更乾淨 ——
//         a.compareWith(b) 讀起來像是 a 主動、b 被動，但比較本身是對稱的。
//     為什麼會錯：把「沒有 this」誤解成「拿不到任何物件」。
//         實際上 this 只是「隱含的第一個參數」，
//         明確寫成參數之後，能做的事完全一樣。
// ═══════════════════════════════════════════════════════════════════════════

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
using namespace std;

class Character {
private:
    string name_;               // 非靜態
    int hp_;                    // 非靜態
    inline static int count_ = 0;  // 靜態

public:
    Character(const string& name, int hp)
        : name_(name), hp_(hp)
    {
        count_++;
    }

    // ====== 非靜態函數：有 this，可以訪問一切 ======
    // 非靜態函數可以訪問非靜態成員（name_ 和 hp_）和靜態成員（count_）
    // 非靜態函數也可以調用其他非靜態函數（printInfo()），因為它們都有 this 指針
    void printInfo() const {
        cout << "  " << name_ << " HP:" << hp_     // ✅ 非靜態成員
             << " (共 " << count_ << " 人)" << endl; // ✅ 靜態成員也行
    }

    // ====== 靜態函數：沒有 this，只能訪問靜態 ======
    // 靜態函數只能訪問靜態成員，不能訪問非靜態成員，因為它沒有 this 指針
    // 靜態函數也不能調用非靜態函數，因為它們需要 this 指針
    static void printCount() {
        cout << "  角色總數：" << count_ << endl;  // ✅ 靜態成員

        // cout << name_;      // ❌ 哪個對象的 name_？
        // cout << hp_;        // ❌ 哪個對象的 hp_？
        // printInfo();        // ❌ 需要 this 才能調用
    }

    // 靜態函數可以接收對象參數來間接訪問非靜態成員
    // 這裡的 compare 是靜態函數，但它接受兩個 Character 對象作為參數
    // 因為 compare 是 Character 的成員函數，所以它可以訪問 Character 的 private 成員
    // 注意：這裡訪問的是「參數對象」的成員，不是通過 this，因為 compare 是靜態函數沒有 this 指針
    static void compare(const Character& a, const Character& b) {
        cout << "  比較：" << a.name_ << "(HP:" << a.hp_ << ") vs "
             << b.name_ << "(HP:" << b.hp_ << ")" << endl;

        // 注意：這裡訪問的是「參數對象」的成員，不是通過 this
        // 因為 compare 是 Character 的成員函數，可以訪問 private
        if (a.hp_ > b.hp_)
            cout << "  → " << a.name_ << " 更強" << endl;
        else if (a.hp_ < b.hp_)
            cout << "  → " << b.name_ << " 更強" << endl;
        else
            cout << "  → 不分上下" << endl;
    }
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 179. Largest Number
//   題目：給一組非負整數，重新排列順序，串接成「數值最大」的字串。
//   為什麼用到本主題：這題的核心就是一個自訂比較函式 ——
//         判斷 a 該排在 b 前面的條件是 (a+b) > (b+a) 的字典序比較。
//         把它寫成 static 成員函式正是本檔的主題：
//         比較是「對稱的二元運算」，需要兩個物件，卻不需要 this。
//   陷阱：輸入全是 0 時會串出 "000...."，必須特判回傳 "0"。
//   複雜度：O(n log n) 次比較，每次比較是字串串接 O(L)。
// -----------------------------------------------------------------------------
class LargestNumberSolver {
public:
    LargestNumberSolver() = delete;      // 純工具類，禁止實例化

    // 靜態比較函式：位址型別是普通函式指標，可直接交給 std::sort
    // 相等時（a+b == b+a）回傳 false，滿足 strict weak ordering
    static bool greaterConcat(const string& a, const string& b) {
        return a + b > b + a;
    }

    static string largestNumber(vector<int> nums) {
        vector<string> parts;
        parts.reserve(nums.size());
        for (int n : nums) parts.push_back(to_string(n));

        sort(parts.begin(), parts.end(), greaterConcat);

        // 全 0 特判：否則會得到 "00" 這種不合法的表示
        if (!parts.empty() && parts[0] == "0") return "0";

        string result;
        for (const auto& p : parts) result += p;
        return result;
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】伺服器清單依「健康狀態 + 負載」排序
//   情境：負載平衡器要挑一台後端伺服器，排序規則是
//         先看健康的排前面，同樣健康時比目前連線數（少的優先）。
//   為什麼用靜態成員函式：
//     (1) 比較規則需要讀 private 欄位，寫成 static 成員就不必開放 getter；
//     (2) 排序比較是對稱的二元運算，本來就不該綁在某一個物件上；
//     (3) 它的位址是普通函式指標，可以直接交給 std::sort。
//   正確性重點：同一台伺服器與自己比較時必須回傳 false，
//               否則違反 strict weak ordering，std::sort 的行為將是未定義。
// -----------------------------------------------------------------------------
class BackendServer {
private:
    string host_;
    int    activeConns_;
    bool   healthy_;

public:
    BackendServer(const string& host, int conns, bool healthy)
        : host_(host), activeConns_(conns), healthy_(healthy) {}

    // 靜態比較函式，直接讀兩個物件的 private 欄位
    static bool betterThan(const BackendServer& a, const BackendServer& b) {
        if (a.healthy_ != b.healthy_) return a.healthy_;   // 健康的優先
        return a.activeConns_ < b.activeConns_;            // 再比連線數（少的優先）
        // 兩者皆相同時回傳 false —— 這是 strict weak ordering 的要求
    }

    static void printAll(const vector<BackendServer>& list) {
        for (const auto& s : list) {
            cout << "    " << s.host_
                 << "  連線數=" << s.activeConns_
                 << "  " << (s.healthy_ ? "健康" : "異常") << endl;
        }
    }
};

int main() {
    cout << "=== 訪問規則 ===" << endl;

    Character warrior("戰士", 200);
    Character mage("法師", 120);

    cout << "\n--- 非靜態函數（需要對象）---" << endl;
    warrior.printInfo();
    mage.printInfo();

    cout << "\n--- 靜態函數（不需要對象）---" << endl;
    Character::printCount();

    cout << "\n--- 靜態函數接收對象參數 ---" << endl;
    Character::compare(warrior, mage);
    cout << "  ↑ compare 是靜態函式（沒有 this），" << endl;
    cout << "    卻能讀 a.name_ / a.hp_ 這些 private 成員 ——" << endl;
    cout << "    因為存取控制看的是「是不是本類別的成員」，與靜不靜態無關。" << endl;

    cout << "\n=== LeetCode 179. Largest Number ===" << endl;
    {
        vector<int> a{10, 2};
        cout << "  [10,2]              → " << LargestNumberSolver::largestNumber(a)
             << "（預期 210）" << endl;

        vector<int> b{3, 30, 34, 5, 9};
        cout << "  [3,30,34,5,9]       → " << LargestNumberSolver::largestNumber(b)
             << "（預期 9534330）" << endl;

        vector<int> c{0, 0};
        cout << "  [0,0]               → " << LargestNumberSolver::largestNumber(c)
             << "（預期 0，全 0 需特判）" << endl;

        vector<int> d{1};
        cout << "  [1]                 → " << LargestNumberSolver::largestNumber(d)
             << "（預期 1）" << endl;
    }

    cout << "\n=== 日常實務：後端伺服器挑選排序 ===" << endl;
    {
        vector<BackendServer> pool{
            {"web-01", 120, true},
            {"web-02",  35, true},
            {"web-03",   4, false},   // 連線數最少，但不健康
            {"web-04",  35, true},    // 與 web-02 同分，用來驗證相等情況
            {"web-05",  88, true}
        };

        cout << "  排序前：" << endl;
        BackendServer::printAll(pool);

        sort(pool.begin(), pool.end(), BackendServer::betterThan);

        cout << "  排序後（健康優先，其次連線數少者優先）：" << endl;
        BackendServer::printAll(pool);
        cout << "  ↑ 不健康的 web-03 雖然連線數最少，仍被排到最後。" << endl;
        cout << "    web-02 與 web-04 同分：比較函式對它們回傳 false，" << endl;
        cout << "    符合 strict weak ordering 的要求。" << endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第 25 課：類別內的靜態成員函數2.cpp -o static_func2

// 【輸出但書】
//   std::sort 不是穩定排序，因此 web-02 與 web-04（兩者同分）之間的
//   相對順序屬於「合法但未指定」——標準沒有保證，換一個實作或元素數量
//   就可能對調。下方輸出是本機 GCC 連續三次執行的一致結果，
//   但不應把這兩行的先後當成語言保證。需要保證時請改用 std::stable_sort。
//   其餘所有行都是確定的。

// === 預期輸出 ===
// === 訪問規則 ===
//
// --- 非靜態函數（需要對象）---
//   戰士 HP:200 (共 2 人)
//   法師 HP:120 (共 2 人)
//
// --- 靜態函數（不需要對象）---
//   角色總數：2
//
// --- 靜態函數接收對象參數 ---
//   比較：戰士(HP:200) vs 法師(HP:120)
//   → 戰士 更強
//   ↑ compare 是靜態函式（沒有 this），
//     卻能讀 a.name_ / a.hp_ 這些 private 成員 ——
//     因為存取控制看的是「是不是本類別的成員」，與靜不靜態無關。
//
// === LeetCode 179. Largest Number ===
//   [10,2]              → 210（預期 210）
//   [3,30,34,5,9]       → 9534330（預期 9534330）
//   [0,0]               → 0（預期 0，全 0 需特判）
//   [1]                 → 1（預期 1）
//
// === 日常實務：後端伺服器挑選排序 ===
//   排序前：
//     web-01  連線數=120  健康
//     web-02  連線數=35  健康
//     web-03  連線數=4  異常
//     web-04  連線數=35  健康
//     web-05  連線數=88  健康
//   排序後（健康優先，其次連線數少者優先）：
//     web-02  連線數=35  健康
//     web-04  連線數=35  健康
//     web-05  連線數=88  健康
//     web-01  連線數=120  健康
//     web-03  連線數=4  異常
//   ↑ 不健康的 web-03 雖然連線數最少，仍被排到最後。
//     web-02 與 web-04 同分：比較函式對它們回傳 false，
//     符合 strict weak ordering 的要求。
