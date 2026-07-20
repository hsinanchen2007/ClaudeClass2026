// =============================================================================
//  第 11 課 -7  —  private 的粒度是「類別」，不是「物件」
// =============================================================================
//
// 【主題資訊 Information】
//   規則：  一個類別的成員函式，可以存取「該類別任何物件」的 private 成員，
//           不限於 this 所指的那一個。
//   標準：  C++98 起（[class.access] 的存取權以類別為單位）
//   標頭檔：本例需 <iostream>、<vector>、<string>、<cmath>、<algorithm>、<iomanip>
//   關鍵詞：class-level access、per-class not per-object、friend、operator overloading
//
// 【詳細解釋 Explanation】
//
// 【1. 規則本身：存取檢查以「類別」為單位】
// C++ 的存取控制回答的問題是：「寫出這個名字的那段程式碼，位在哪裡？」
// 而不是：「它正在操作哪一個物件？」
// 只要那段程式碼位於 Box 的成員函式（或 Box 的 friend）之內，
// 它就能寫出任何 Box 物件的 private 成員名稱：
//     bool isLargerThan(const Box& other) const {
//         return area() > other.width * other.height;   // ✅ other 的 private
//     }
// 這不是編譯器的漏洞，而是明確的設計。
//
// 【2. 為什麼要這樣設計：否則二元運算無法實作】
// 如果存取權是「以物件為單位」（只能碰 this 的 private），那麼任何
// 「需要同時看兩個同型別物件」的操作都寫不出來：
//     * 拷貝建構函式  Box(const Box& o) : width(o.width) ...   ← 必須讀 o 的 private
//     * 拷貝賦值      operator=(const Box& o)
//     * 比較運算      operator==、operator<
//     * 合併／交換    merge(const Box&)、swap(Box&)
// 這些都是型別的基本能力。若不開放同類別互看，就只能被迫為每個 private
// 成員加上 public getter —— 那等於把封裝全部拆掉，反而更糟。
// 所以「以類別為單位」其實是**維護封裝**的必要條件，而不是破壞它。
//
// 【3. 邊界在哪裡：什麼情況下仍然不能存取】
// 「同類別」的判定相當嚴格：
//   * 不同類別：Box 的成員函式不能碰 Circle 的 private（除非 Circle 宣告 friend）。
//   * 樣板的不同實例化：Box<int> 與 Box<double> 是**兩個不同的類別**，
//     彼此不能互看 private（除非寫 template<class U> friend class Box;）。
//   * 衍生類別：Derived 的成員函式不能碰 Base 的 private（那要 protected）。
//   * 非成員的自由函式：即使寫在同一個檔案、同一個 namespace，也不行。
// 本檔用 Box 與 Circle 對照示範前兩點。
//
// 【4. 這條規則的實際用途：operator== 與 friend】
// 成員版的二元運算子（如 `bool operator==(const Box&) const`）天生就享有
// 這個權利。但有些運算子必須寫成非成員（典型是 operator<<，因為左運算元是
// ostream 而不是你的類別），這時就要用 friend 明確授權：
//     friend std::ostream& operator<<(std::ostream&, const Box&);
// friend 宣告寫在類別內部 —— 也就是說，授權權仍然握在類別自己手上，
// 外部無法自行宣稱是誰的 friend。這保住了「封裝由類別自己決定」的原則。
//
// 【概念補充 Concept Deep Dive】
// (A) 存取檢查是最後一步，不影響多載決議
//     名稱查找 → 多載決議 → 存取檢查，順序固定。
//     所以一個 private 的多載函式仍會參與競爭，勝出後才報「is private」錯誤；
//     編譯器不會退而挑一個 public 的多載。這常讓人誤以為「編譯器選錯函式」。
//
// (B) friend 不對稱、不遞移、不可繼承
//     * 不對稱：A 宣告 B 是 friend，不代表 B 也讓 A 看。
//     * 不遞移：B 是 A 的 friend、C 是 B 的 friend，C 不是 A 的 friend。
//     * 不可繼承：A 的 friend 不會自動變成 A 的衍生類別的 friend。
//     friend 是「精確而有限的授權」，這正是它比 public 安全的原因。
//
// (C) 為什麼比較函式該加 const
//     isLargerThan 不修改任何一方，應宣告為 const 成員函式並收 const Box&。
//     否則 `const Box& a` 這種常見的傳參方式就無法呼叫它。
//     原始版本沒有 const，本檔已補上 —— 這是實務上真的會擋住編譯的問題。
//
// (D) 浮點數比較的注意事項
//     本例用 `>` 比較兩個 double 面積，對教學足夠；但實務上浮點運算有捨入誤差，
//     判斷「相等」時不該用 ==，而應比較差的絕對值是否小於某個容差
//     （且容差要依數值量級調整）。本檔的 approxEqualArea() 示範這個做法。
//
// 【注意事項 Pay Attention】
// 1. 存取權以「類別」為單位，不是以「物件」為單位 —— 這是規範，不是漏洞。
// 2. 樣板的不同實例化屬於不同類別，Box<int> 碰不到 Box<double> 的 private。
// 3. 衍生類別碰不到基底的 private（那是 protected 的職責）。
// 4. 比較／查詢類的成員函式請加 const，並用 const& 收參數。
// 5. 浮點數不要用 == 比較；用容差，且容差需隨數值量級調整。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】類別層級的存取權
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 Box 的成員函式可以存取「另一個 Box 物件」的 private 成員？
//     答：因為 C++ 的存取控制以「類別」為單位，不是以「物件實例」為單位。
//         判準是「寫出這個名字的程式碼位在哪裡」，只要在 Box 的成員函式
//         或 friend 內，任何 Box 物件的 private 都看得到。
//     追問：如果不這樣設計會怎樣？
//         → 拷貝建構函式、operator=、operator== 這些需要同時看兩個物件的
//           基本操作全都寫不出來，只能被迫為每個 private 成員加 public getter
//           —— 封裝反而被拆得更徹底。所以這條規則是在保護封裝。
//
// 🔥 Q2. Box<int> 的成員函式能存取 Box<double> 的 private 成員嗎？
//     答：不能。樣板的每個實例化都是**獨立的類別**，Box<int> 與 Box<double>
//         之間沒有任何存取特權。要開放必須明確寫
//         `template<typename U> friend class Box;`。
//     追問：那 friend 有哪些性質要注意？
//         → 不對稱、不遞移、不可繼承。它是精確而有限的授權。
//
// ⚠️ 陷阱. 「private 成員只有 this 這個物件自己碰得到，所以 other.width 應該編譯失敗」——錯在哪？
//     答：會編譯成功。private 管的是「哪段程式碼可以寫出這個名字」，
//         而不是「這個名字屬於哪個物件」。isLargerThan 的程式碼位於 Box 內部，
//         所以它有權存取任何 Box 物件的 private 成員。
//     為什麼會錯：多數人把 private 想像成「每個物件各自上鎖，鑰匙在 this 手上」
//         的執行期模型。但存取控制完全發生在編譯期，編譯器只看
//         「這行程式碼寫在哪個類別的作用域裡」，根本不知道執行期會有幾個物件。
//         正確的心智模型是：**鎖在類別的門上，不在每個物件身上**。
//
// 【LeetCode 實戰範例】—— 從缺（刻意不加）
//     「同類別可存取彼此 private」是語言的存取規則，不是演算法或資料結構設計，
//     LeetCode 上沒有對應題目。本檔改以「矩形／區間比較」這類實務情境示範。
//     （若想練與矩形相關的演算法題，可看 836. Rectangle Overlap、
//       223. Rectangle Area，但那兩題考的是幾何計算而非存取控制，
//       放在這裡會模糊本檔重點，故不硬湊。）
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <iomanip>   // setprecision（浮點比較示範用）
using namespace std;

class Box {
private:
    double width = 0;
    double height = 0;

public:
    void setSize(double w, double h) {
        width = w;
        height = h;
    }

    double area() const {
        return width * height;
    }

    // ── 本檔的核心示範 ────────────────────────────────────────────────
    // 這裡直接讀取 other.width / other.height，而它們是 private。
    // 之所以合法，是因為存取權以「類別」為單位：這段程式碼寫在 Box 內部，
    // 因此有權存取「任何 Box 物件」的 private 成員，不限於 this。
    // 若把這個函式改寫成類別外的自由函式，同樣兩行就會編譯失敗。
    // ──────────────────────────────────────────────────────────────────
    bool isLargerThan(const Box& other) const {
        return (width * height) > (other.width * other.height);
    }

    // 同樣的道理：合併兩個 Box（取各維度最大值）也需要讀 other 的 private
    Box merged(const Box& other) const {
        Box r;
        r.width  = max(width,  other.width);    // 寫入另一個 Box 的 private，一樣合法
        r.height = max(height, other.height);
        return r;
    }

    // 浮點面積比較：用容差而非 ==（見概念補充 D）
    bool approxEqualArea(const Box& other, double tol = 1e-9) const {
        double a = area(), b = other.area();
        return fabs(a - b) <= tol * max({1.0, fabs(a), fabs(b)});
    }

    void show() const {
        cout << "  " << width << " x " << height << " = " << area() << endl;
    }
};

// 對照組：不同類別就沒有這個特權
class Circle {
private:
    double radius = 1.0;
public:
    void setRadius(double r) { radius = r; }
    double area() const { return 3.14159265358979 * radius * radius; }

    // 下面這行若解除註解會編譯失敗：Circle 不是 Box，碰不到 Box 的 private
    // bool biggerThanBox(const Box& b) { return radius > b.width; }  // ❌
};

// 對照組：類別外的自由函式，即使在同一個檔案也不行
// bool freeCompare(const Box& a, const Box& b) {
//     return a.width > b.width;    // ❌ error: 'width' is private within this context
// }

// -----------------------------------------------------------------------------
// 【日常實務範例】語意化版本號（Semantic Version）的比較與相容性判斷
//   情境：套件管理器要判斷「已安裝版本」與「需求版本」的關係：
//     (a) 排序版本清單（找出最新版）
//     (b) 判斷是否相容（同 major 視為相容，這是 semver 的核心規則）
//   這正是「同類別互看 private」的典型應用：比較兩個 Version 需要同時讀
//   兩邊的 major/minor/patch，而這三個欄位必須 private
//   —— 因為不變量是「三個欄位都不可為負」，且未來可能要加上 pre-release 標籤。
// -----------------------------------------------------------------------------
class Version {
private:
    int m_major = 0;
    int m_minor = 0;
    int m_patch = 0;

    static int clampNonNeg(int v) { return v < 0 ? 0 : v; }

public:
    Version(int ma, int mi, int pa)
        : m_major(clampNonNeg(ma)), m_minor(clampNonNeg(mi)), m_patch(clampNonNeg(pa)) {}

    // 讀取 other 的三個 private 欄位 —— 同類別，合法
    bool operator<(const Version& other) const {
        if (m_major != other.m_major) return m_major < other.m_major;
        if (m_minor != other.m_minor) return m_minor < other.m_minor;
        return m_patch < other.m_patch;
    }
    bool operator==(const Version& other) const {
        return m_major == other.m_major
            && m_minor == other.m_minor
            && m_patch == other.m_patch;
    }

    // semver 規則：同一個 major 版本才保證向後相容
    bool compatibleWith(const Version& required) const {
        return m_major == required.m_major && !(*this < required);
    }

    string str() const {
        return to_string(m_major) + "." + to_string(m_minor) + "." + to_string(m_patch);
    }

    // operator<< 必須是非成員（左運算元是 ostream），所以用 friend 明確授權
    friend ostream& operator<<(ostream& os, const Version& v) {
        return os << v.m_major << "." << v.m_minor << "." << v.m_patch;  // 讀 private
    }
};

int main() {
    cout << "=== 同類別的成員函式可存取其他物件的 private ===" << endl;
    Box a, b;
    a.setSize(5, 4);
    b.setSize(3, 8);

    cout << "Box a:"; a.show();
    cout << "Box b:"; b.show();

    if (a.isLargerThan(b)) {
        cout << "  a 比 b 大" << endl;
    } else {
        cout << "  b 比 a 大（或一樣大）" << endl;
    }
    cout << "  → isLargerThan 讀取了 other.width / other.height（都是 private）" << endl;

    cout << "\n=== 不只能讀，也能寫另一個物件的 private ===" << endl;
    Box m = a.merged(b);
    cout << "a 與 b 合併（各取最大邊）:"; m.show();

    cout << "\n=== 浮點面積比較：用容差，不要用 == ===" << endl;
    Box c1, c2;
    c1.setSize(0.1, 3.0);          // 0.30000000000000004
    c2.setSize(0.3, 1.0);          // 0.3
    cout << "  預設精度看起來一樣： c1 = " << c1.area()
         << "，c2 = " << c2.area() << endl;
    cout << setprecision(17)
         << "  17 位精度才看得出差異： c1 = " << c1.area()
         << "，c2 = " << c2.area() << endl
         << setprecision(6);                      // 還原預設精度
    cout << "  直接用 == 比較： " << boolalpha << (c1.area() == c2.area()) << endl;
    cout << "  用容差比較：     " << c1.approxEqualArea(c2) << endl;
    cout << "  → 浮點捨入誤差讓 == 得到反直覺的結果，這與存取權無關但同樣常踩。" << endl;

    cout << "\n=== 日常實務：語意化版本號比較 ===" << endl;
    vector<Version> installed = { {1, 4, 2}, {2, 0, 0}, {1, 10, 0}, {1, 4, 10} };
    cout << "  排序前： ";
    for (const Version& v : installed) cout << v << "  ";
    cout << endl;

    sort(installed.begin(), installed.end());     // 用到 operator<（讀 other 的 private）
    cout << "  排序後： ";
    for (const Version& v : installed) cout << v << "  ";
    cout << endl;
    cout << "  → 注意 1.4.10 < 1.10.0：semver 逐段比「數值」，不是字串比較。" << endl;

    cout << "\n  相容性判斷（需求 >= 1.4.0 且同 major）：" << endl;
    Version required(1, 4, 0);
    for (const Version& v : installed) {
        cout << "    " << v << " 相容於 " << required << "？ "
             << (v.compatibleWith(required) ? "是" : "否") << endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 11 課：存取修飾符——public、private、protected7.cpp" -o access7

// === 預期輸出 ===
// === 同類別的成員函式可存取其他物件的 private ===
// Box a:  5 x 4 = 20
// Box b:  3 x 8 = 24
//   b 比 a 大（或一樣大）
//   → isLargerThan 讀取了 other.width / other.height（都是 private）
//
// === 不只能讀，也能寫另一個物件的 private ===
// a 與 b 合併（各取最大邊）:  5 x 8 = 40
//
// === 浮點面積比較：用容差，不要用 == ===
//   預設精度看起來一樣： c1 = 0.3，c2 = 0.3
//   17 位精度才看得出差異： c1 = 0.30000000000000004，c2 = 0.29999999999999999
//   直接用 == 比較： false
//   用容差比較：     true
//   → 浮點捨入誤差讓 == 得到反直覺的結果，這與存取權無關但同樣常踩。
//
// === 日常實務：語意化版本號比較 ===
//   排序前： 1.4.2  2.0.0  1.10.0  1.4.10  
//   排序後： 1.4.2  1.4.10  1.10.0  2.0.0  
//   → 注意 1.4.10 < 1.10.0：semver 逐段比「數值」，不是字串比較。
//
//   相容性判斷（需求 >= 1.4.0 且同 major）：
//     1.4.2 相容於 1.4.0？ 是
//     1.4.10 相容於 1.4.0？ 是
//     1.10.0 相容於 1.4.0？ 是
//     2.0.0 相容於 1.4.0？ 否
