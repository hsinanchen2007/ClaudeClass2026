// =============================================================================
//  第 14 課：預設建構函數 2  —  為什麼「寫了帶參 constructor，預設的就消失了」
// =============================================================================
//
// 【主題資訊 Information】
//   規則      : 只要使用者宣告了**任何一個** constructor，
//               編譯器就不再隱式宣告 default constructor
//   標準版本  : C++98 起；C++11 起可用 = default 把它要回來
//   標頭檔    : <iostream>、<type_traits>
//   典型錯誤  : error: no matching function for call to 'Point::Point()'
//
// 【詳細解釋 Explanation】
//
// 【1. 規則的精確措辭很重要】
//   標準說的是「若類別沒有**使用者宣告的** constructor，
//   編譯器就隱式宣告一個 default constructor」。
//   注意是「宣告」不是「定義」，也注意是「任何 constructor」而不是
//   「任何 default constructor」。所以下面每一種都會讓預設版本消失：
//       Point(int, int);            // 帶參數的
//       Point(const Point&);        // copy constructor 也算！
//       Point(Point&&);             // move constructor 也算！
//       explicit Point(int);        // explicit 的也算
//   最容易被忽略的是 copy constructor：很多人為了加個 log 而自己寫了
//   copy constructor，結果 Point p; 突然編不過，一時找不到原因。
//
// 【2. 這個設計背後的推理】
//   編譯器的邏輯是：「你既然親自表達了『建立這個物件需要特定資訊』，
//   那麼『不給任何資訊就建出一個物件』多半不是你要的語意。」
//   以 Point 為例，若編譯器好心補一個 default constructor，
//   你會得到一個座標不確定的點——那幾乎一定是 bug，而不是便利。
//   換句話說，這個規則是在**保護**你，不是在為難你。
//
// 【3. 什麼時候會踩到這個坑（比你想的更頻繁）】
//   即使你自己從來不寫 Point p;，下面這些場景都需要 default constructor：
//       Point arr[10];                  // 陣列，每個元素都要預設建構
//       std::vector<Point> v(10);       // 這個 overload 會值初始化 10 個元素
//       std::map<string, Point> m;
//       m["origin"].x = 1;              // operator[] 找不到 key 時會插入預設建構的值
//       Point p;                        // 想先宣告、稍後再賦值
//   `std::map::operator[]` 是最常見的意外來源——很多人不知道它會建立元素。
//
// 【4. 四種解法，以及各自的語意】
//   (a) 自己補一個：Point() { x = 0; y = 0; }   → 有明確的初始值
//   (b) = default（C++11）：Point() = default;   → 行為同編譯器生成的
//                                                 （內建型別成員仍不初始化！）
//   (c) NSDMI + = default：int x = 0; 搭配 Point() = default;
//                                                 → 既簡潔又有確定的值（推薦）
//   (d) = delete（C++11）：Point() = delete;      → 明確表達「本來就不該預設建構」
//   選 (b) 時務必記得它**不會**幫你歸零；這是 4.cpp 與 5.cpp 的主題。
//
// 【概念補充 Concept Deep Dive】
//   ▍std::vector<Point> v(10) 與 v.reserve(10) 的差別
//     v(10) 會真的建立 10 個元素，因此需要 default constructor；
//     v.reserve(10) 只配置記憶體、不建立元素，因此**不需要**。
//     這也是「為什麼有時候編得過、有時候編不過」的常見答案。
//
//   ▍is_default_constructible 可以編譯期檢查
//     std::is_default_constructible<T>::value 會告訴你 T 能不能預設建構。
//     本檔用 static_assert 把「Point 不能預設建構」這件事變成編譯期事實，
//     而不只是註解裡的宣稱。
//
//   ▍aggregate initialization 是另一條路
//     若 Point 是 aggregate（全 public、無使用者宣告的 constructor…），
//     可以寫 Point p{3, 4}; 完全不需要 constructor。
//     但本檔的 Point 宣告了 constructor，因此不是 aggregate。
//     注意 C++20 起「任何使用者宣告的 constructor」都會取消 aggregate 資格，
//     C++17 的規則稍寬鬆（只有 user-provided 才取消）——這個差異在 4.cpp 詳述。
//
// 【注意事項 Pay Attention】
//   1. copy / move constructor 也算「使用者宣告的 constructor」，同樣會讓預設版本消失。
//   2. std::map::operator[] 會對不存在的 key 插入一個預設建構的值——需要 default ctor。
//   3. = default 要回來的版本**不會**初始化內建型別成員，別以為加了就安全。
//   4. 本檔中會編譯失敗的程式碼一律以註解呈現，並附上真實的 GCC 錯誤訊息。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】預設 constructor 何時消失
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 什麼情況下編譯器不會提供 default constructor？
//     答：只要類別有**任何使用者宣告的 constructor**就不會——包括帶參數的、
//         copy constructor、move constructor、explicit 的都算。
//         另外若含有沒有 default constructor 的成員、沒有 NSDMI 的 const 成員、
//         或 reference 成員，隱式生成的版本會被定義為 deleted。
//     追問：怎麼要回來？→ C++11 的 Point() = default;，
//         但要注意它不會初始化內建型別成員，通常要搭配 NSDMI。
//
// 🔥 Q2. 哪些看起來無關的操作其實需要 default constructor？
//     答：T arr[N];、std::vector<T> v(n);（注意是圓括號的 count 版本）、
//         std::map<K,T>::operator[] 在 key 不存在時的插入、
//         以及所有「先宣告、之後再賦值」的寫法。
//     追問：vector<T> v; v.reserve(10); 需要嗎？→ 不需要。reserve 只配置記憶體、
//         不建立元素；v(10) 才會真的值初始化 10 個元素。
//
// ⚠️ 陷阱. 「我只是幫 copy constructor 加了一行 log，怎麼 Point p; 突然編不過了？」
//     答：因為 copy constructor 也是「使用者宣告的 constructor」。
//         你宣告它的那一刻，編譯器就撤回了隱式的 default constructor。
//         補一行 Point() = default; 即可（視需要搭配 NSDMI）。
//     為什麼會錯：以為規則是「宣告了 default constructor 才會取代預設的」。
//         實際規則是「宣告了**任何** constructor」，範圍比直覺大得多。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <map>
#include <string>
#include <type_traits>
#include <vector>
using namespace std;

class Point {
public:
    int x, y;

    // 只定義了帶參數的建構函數 → 隱式的 default constructor 就此消失
    Point(int px, int py) {
        x = px;
        y = py;
        cout << "Point(" << x << ", " << y << ") 建構完成" << endl;
    }
};

// 只加了 copy constructor 的類別——預設建構同樣會消失（最容易忽略的情況）
class Tracked {
public:
    int id;
    Tracked(const Tracked& other) : id(other.id) {
        cout << "  [copy] 複製 id=" << id << endl;
    }
    explicit Tracked(int i) : id(i) {}
};

// 把預設建構要回來，並且用 NSDMI 保證有確定的值（推薦寫法）
class PointFixed {
public:
    int x = 0;
    int y = 0;

    PointFixed() = default;                       // 要回預設建構
    PointFixed(int px, int py) : x(px), y(py) {}  // 保留帶參版本
};

// 編譯期事實查核（本機 GCC 15.2 實測）
static_assert(!std::is_default_constructible<Point>::value,
              "Point 宣告了帶參 constructor → 不可預設建構");
static_assert(!std::is_default_constructible<Tracked>::value,
              "Tracked 只宣告了 copy constructor，預設建構同樣消失");
static_assert(std::is_default_constructible<PointFixed>::value,
              "PointFixed 用 = default 要回了預設建構");

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】從缺
//   理由：LeetCode 設計題的 constructor 簽名由題目固定，不會出現
//         「因為宣告了帶參 constructor 導致預設版本消失」這個編譯期問題。
//         本檔講的是編譯器何時提供隱式成員函式，屬於語言規則。
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 【日常實務範例】std::map 的 operator[] 為什麼要求 default constructor
//   情境：統計每個使用者的請求數與錯誤數，很自然會寫 stats[user].requests++。
//         operator[] 在 key 不存在時會**先插入一個值初始化的元素**再回傳它，
//         所以 mapped_type 必須能預設建構。若你的統計型別宣告了帶參 constructor
//         又忘了補回預設版本，這行就會編譯失敗。
//   另一個常被忽略的點：operator[] 會「悄悄插入」元素，
//         純查詢時應該用 find()／at()，否則 map 會被查詢動作撐大。
// -----------------------------------------------------------------------------
struct UserStat {
    int requests = 0;      // NSDMI + 隱式 default constructor → operator[] 可用
    int errors   = 0;
};

int main() {
    cout << "=== 帶參建構函數可用 ===" << endl;
    Point p1(3, 4);    // OK：匹配帶參建構函數
    cout << "  p1 = (" << p1.x << ", " << p1.y << ")" << endl;

    cout << "\n=== 以下寫法會編譯失敗（故以註解呈現）===" << endl;
    // Point p2;        // error: no matching function for call to 'Point::Point()'
    // Point p3{};      // error: 同上，值初始化一樣需要 default constructor
    // Point arr[3];    // error: 陣列每個元素都要預設建構
    cout << "  Point p2;      → no matching function for call to 'Point::Point()'" << endl;
    cout << "  Point p3{};    → 同上（值初始化也需要 default constructor）" << endl;
    cout << "  Point arr[3];  → 同上（陣列每個元素都要預設建構）" << endl;
    cout << "  is_default_constructible<Point> = "
         << std::is_default_constructible<Point>::value << "（0 = 不可以）" << endl;

    cout << "\n=== 最容易忽略：只宣告 copy constructor 也會讓預設版消失 ===" << endl;
    Tracked t1(42);
    Tracked t2 = t1;              // 用到 copy constructor
    cout << "  t2.id = " << t2.id << endl;
    // Tracked t3;                // error: no matching function ... Tracked::Tracked()
    cout << "  Tracked t3;    → 一樣編譯失敗，因為 copy constructor 也算「使用者宣告」" << endl;
    cout << "  is_default_constructible<Tracked> = "
         << std::is_default_constructible<Tracked>::value << endl;

    cout << "\n=== 修好之後：= default + NSDMI ===" << endl;
    PointFixed f1;                 // 可以預設建構了
    PointFixed f2(7, 8);
    cout << "  f1 = (" << f1.x << ", " << f1.y << ")  ← NSDMI 保證是 0，不是垃圾值" << endl;
    cout << "  f2 = (" << f2.x << ", " << f2.y << ")" << endl;

    cout << "\n=== 需要 default constructor 的隱藏場景 ===" << endl;
    PointFixed arr[3];             // 陣列：每個元素都預設建構
    cout << "  PointFixed arr[3] → arr[1] = (" << arr[1].x << ", " << arr[1].y << ")" << endl;

    vector<PointFixed> v(3);       // 圓括號版本：值初始化 3 個元素
    cout << "  vector<PointFixed> v(3) → v.size() = " << v.size()
         << ", v[2] = (" << v[2].x << ", " << v[2].y << ")" << endl;

    vector<Point> vr;              // Point 不能預設建構，但 reserve 不需要
    vr.reserve(3);                 // 只配置記憶體，不建立元素 → 合法
    vr.emplace_back(1, 2);         // 直接就地建構，也不需要預設建構
    cout << "  vector<Point> vr; vr.reserve(3); vr.emplace_back(1,2); → OK" << endl;
    cout << "    （vector<Point> vr(3); 才會失敗，因為那要值初始化 3 個元素）" << endl;

    cout << "\n=== 日常實務：map::operator[] 需要 default constructor ===" << endl;
    map<string, UserStat> stats;
    stats["alice"].requests++;     // key 不存在 → 先插入值初始化的 UserStat
    stats["alice"].requests++;
    stats["alice"].errors++;
    stats["bob"].requests++;
    for (const auto& kv : stats) {
        cout << "  " << kv.first << ": requests=" << kv.second.requests
             << ", errors=" << kv.second.errors << endl;
    }
    cout << "  注意：operator[] 會「悄悄插入」元素——" << endl;
    cout << "  純查詢請用 find()／at()，否則 map 會被查詢動作撐大。" << endl;
    cout << "  查詢一個不存在的 key 之前，size = " << stats.size() << endl;
    stats["carol"];                // 只是查詢，卻插入了一筆
    cout << "  用 operator[] 查了 \"carol\" 之後，size = " << stats.size()
         << "  ← 被撐大了" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 14 課：預設建構函數（Default Constructor）2.cpp" -o def2

// === 預期輸出 ===
// === 帶參建構函數可用 ===
// Point(3, 4) 建構完成
//   p1 = (3, 4)
//
// === 以下寫法會編譯失敗（故以註解呈現）===
//   Point p2;      → no matching function for call to 'Point::Point()'
//   Point p3{};    → 同上（值初始化也需要 default constructor）
//   Point arr[3];  → 同上（陣列每個元素都要預設建構）
//   is_default_constructible<Point> = 0（0 = 不可以）
//
// === 最容易忽略：只宣告 copy constructor 也會讓預設版消失 ===
//   [copy] 複製 id=42
//   t2.id = 42
//   Tracked t3;    → 一樣編譯失敗，因為 copy constructor 也算「使用者宣告」
//   is_default_constructible<Tracked> = 0
//
// === 修好之後：= default + NSDMI ===
//   f1 = (0, 0)  ← NSDMI 保證是 0，不是垃圾值
//   f2 = (7, 8)
//
// === 需要 default constructor 的隱藏場景 ===
//   PointFixed arr[3] → arr[1] = (0, 0)
//   vector<PointFixed> v(3) → v.size() = 3, v[2] = (0, 0)
// Point(1, 2) 建構完成
//   vector<Point> vr; vr.reserve(3); vr.emplace_back(1,2); → OK
//     （vector<Point> vr(3); 才會失敗，因為那要值初始化 3 個元素）
//
// === 日常實務：map::operator[] 需要 default constructor ===
//   alice: requests=2, errors=1
//   bob: requests=1, errors=0
//   注意：operator[] 會「悄悄插入」元素——
//   純查詢請用 find()／at()，否則 map 會被查詢動作撐大。
//   查詢一個不存在的 key 之前，size = 2
//   用 operator[] 查了 "carol" 之後，size = 3  ← 被撐大了
