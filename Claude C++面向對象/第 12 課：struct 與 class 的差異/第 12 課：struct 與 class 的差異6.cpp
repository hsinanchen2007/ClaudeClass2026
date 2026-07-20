// =============================================================================
//  第 12 課：struct 與 class 的差異 6  —  struct 也可以有成員函式
// =============================================================================
//
// 【主題資訊 Information】
//   主題：替純資料 struct 加上「便利函式」，並不會讓它失去純資料的本質
//   標準版本：C++98 起 struct 即可有成員函式；NSDMI 需 C++11
//   標頭檔：<cmath>（sqrt）
//   關鍵判準：函式有沒有「維護不變條件」？沒有的話它只是便利函式，struct 依然合適
//
// 【詳細解釋 Explanation】
//
// 【1. 成員函式不會取消 struct 的「純資料」身分】
//   很多人以為「一旦加了函式就該改用 class」，這是誤解。真正的判準始終是
//   「有沒有不變條件要維護」，而不是「有沒有函式」。
//   Point::distanceTo() 只是讀取 x/y 做計算，它不維護任何規則、不阻止任何賦值 ——
//   把它拿掉，Point 的合法狀態集合完全不變。這種函式叫「便利函式」，
//   放進 struct 完全正當，而且比寫成 free function 更好找、更好用。
//
// 【2. 這些函式為什麼一律該標 const？】
//   distanceTo / print / area / perimeter 都不修改成員，標 const 有三個好處：
//     (a) 編譯器擋住你在函式內誤寫成員
//     (b) const 物件與 const 參考也能呼叫（否則 `void f(const Point& p)`
//         裡面連 p.print() 都不能寫，這是實務上最常踩到的痛點）
//     (c) 對讀者傳達「這個操作是查詢，不是命令」
//   原始版本沒標 const，本檔全部補上了 —— 這是 const-correctness 的基本功。
//
// 【3. 便利函式 vs free function：怎麼選？】
//   Scott Meyers 有個著名主張：能寫成 non-member 就別寫成 member，
//   因為 non-member 降低耦合、且對兩個運算元對稱（例如 operator+）。
//   但實務上有個更重要的判準：這個操作是不是「這個型別的核心概念」？
//     * distanceTo：兩點距離是 Point 的核心語意 → member 合理
//     * 「把 Point 序列化成 JSON」：那是序列化模組的事，不該塞進 Point → free function
//   本檔採 member，因為距離與面積都是幾何型別的核心操作。
//
// 【4. aggregate 資格不受成員函式影響】
//   加了 distanceTo/print 之後，Point 仍然是 aggregate，
//   因此 `Point p{3.0, 4.0};` 依然合法。取消 aggregate 資格的是
//   「使用者提供的建構函式」，不是成員函式。這是很常被搞混的一點。
//
// 【概念補充 Concept Deep Dive】
//   * 成員函式不佔物件空間。非虛擬成員函式在編譯後只是一般函式，
//     呼叫時把 this 當隱藏參數傳進去；因此加再多成員函式，sizeof(Point) 都是 16
//     （兩個 double，本機實測）。真正會改變物件大小的是「加上 virtual」——
//     那會插入一個 vptr（實作定義，本機 x86-64 為 8 bytes）。
//   * 加上 virtual 之後型別就不再是 aggregate、也不再 standard-layout，
//     memcpy 與 C 互通都會失效。純資料型別要極力避免虛擬函式。
//   * distanceTo 用 sqrt 會有浮點開銷。在只需要「比大小」的場合（例如找最近點），
//     慣例是比較距離平方（省掉 sqrt 且避免精度損失）——本檔提供了 distanceSquaredTo()。
//
// 【注意事項 Pay Attention】
//   1. 便利函式一律標 const，否則 const 參考傳進來就無法呼叫。
//   2. 別因為「加了函式」就反射性改成 class；判準永遠是不變條件。
//   3. 浮點比較不要用 ==。距離計算的結果帶誤差，要比較請用容差
//      （例如 fabs(a - b) < 1e-9），本檔輸出中示範了這一點。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】struct 的成員函式
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. struct 加了成員函式之後，還算「純資料型別」嗎？
//     答：算。判準是「有沒有不變條件要維護」，不是「有沒有函式」。
//         distanceTo 只讀不寫，拿掉它 Point 的合法狀態集合完全不變，
//         這種便利函式放在 struct 內完全正當。
//     追問：那什麼時候該改成 class？→ 當你開始需要「檢查某個賦值是否合法」時，
//           例如要求 x/y 必須落在畫布範圍內，那就該把欄位藏起來。
//
// 🔥 Q2. 加成員函式會讓 sizeof(Point) 變大嗎？
//     答：不會。非虛擬成員函式編譯後就是普通函式，this 以隱藏參數傳入，
//         不佔物件空間。sizeof 只取決於非靜態資料成員與對齊 padding。
//     追問：那什麼會讓它變大？→ 加上 virtual。編譯器會插入 vptr
//           （實作定義，本機 x86-64 為 8 bytes），且型別不再是 aggregate。
//
// ⚠️ 陷阱. 為什麼找「最近的點」時該用距離平方而不是距離？
//     答：sqrt 是單調遞增函式，比較大小時完全不需要它 —— 省掉開銷，
//         同時避免開根號引入的額外浮點誤差。只有真的要輸出距離值時才開根號。
//     為什麼會錯：直覺覺得「要比距離就得先算距離」，忽略了單調性讓比較可以
//         在平方空間直接進行。這在碰撞偵測、KNN 這類熱路徑上是標準最佳化。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <cmath>
#include <vector>
#include <algorithm>
using namespace std;

struct Point {
    double x = 0;
    double y = 0;

    // 便利函數 —— 不改變「純資料集合」的本質
    // 這些函式只讀取資料、不維護任何不變條件，因此 struct 依然是正確選擇。
    // 全部標 const：不修改成員，且讓 const Point& 也能呼叫。
    double distanceTo(const Point& other) const {
        double dx = x - other.x;
        double dy = y - other.y;
        return sqrt(dx * dx + dy * dy);
    }

    // 只需要「比大小」時用平方距離：省掉 sqrt，也避免開根號的額外誤差
    double distanceSquaredTo(const Point& other) const {
        double dx = x - other.x;
        double dy = y - other.y;
        return dx * dx + dy * dy;
    }

    void print() const {
        cout << "(" << x << ", " << y << ")" << endl;
    }
};

struct Rectangle {
    double width = 0;
    double height = 0;

    double area()      const { return width * height; }
    double perimeter() const { return 2 * (width + height); }
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 223. Rectangle Area
//   題目：給兩個「軸對齊」矩形（各以左下角與右上角座標表示），
//         求兩者覆蓋的總面積（重疊部分只算一次）。
//   為什麼用到本主題：這題天生就該用 struct 表達矩形 —— 四個座標彼此獨立、
//         沒有不變條件，而 area()／intersectionArea() 正是典型的便利成員函式。
//         把資料與幾何運算放在一起，程式碼比傳八個裸 int 清楚得多。
//   複雜度：O(1)。
// -----------------------------------------------------------------------------
struct AxisAlignedRect {
    double x1 = 0, y1 = 0;   // 左下角
    double x2 = 0, y2 = 0;   // 右上角

    double area() const { return (x2 - x1) * (y2 - y1); }

    // 重疊區域面積；沒有重疊時回傳 0
    double intersectionArea(const AxisAlignedRect& o) const {
        double overlapW = min(x2, o.x2) - max(x1, o.x1);
        double overlapH = min(y2, o.y2) - max(y1, o.y1);
        if (overlapW <= 0 || overlapH <= 0) return 0.0;   // 完全沒相交
        return overlapW * overlapH;
    }
};

double computeTotalArea(const AxisAlignedRect& a, const AxisAlignedRect& b) {
    // 容斥原理：兩者面積相加，再扣掉被算了兩次的重疊部分
    return a.area() + b.area() - a.intersectionArea(b);
}

// -----------------------------------------------------------------------------
// 【日常實務範例】在一批感測器座標中找出離基地台最近的那一個
//   情境：IoT 系統收到一批裝置回報的座標，要指派最近的基地台處理。
//   重點在於「比距離時用平方距離」——這是熱路徑上的標準最佳化：
//   sqrt 單調遞增，比較大小完全用不到它，省下每個點一次開根號。
//   只有最後真的要「顯示距離」時才開一次根號。
// -----------------------------------------------------------------------------
struct Sensor {
    int   id = 0;
    Point pos;
};

const Sensor* findNearest(const vector<Sensor>& sensors, const Point& station) {
    if (sensors.empty()) return nullptr;         // 空清單要先擋掉，否則回傳懸空指標

    const Sensor* best = &sensors[0];
    double bestDistSq = sensors[0].pos.distanceSquaredTo(station);
    for (size_t i = 1; i < sensors.size(); ++i) {
        double d = sensors[i].pos.distanceSquaredTo(station);   // 全程不開根號
        if (d < bestDistSq) {
            bestDistSq = d;
            best = &sensors[i];
        }
    }
    return best;
}

int main() {
    cout << "=== Point 與便利函式 ===" << endl;
    Point a{0, 0};        // aggregate initialization：成員函式不影響此資格
    Point b{3, 4};

    a.print();
    b.print();
    cout << "距離: " << a.distanceTo(b) << endl;
    cout << "距離平方: " << a.distanceSquaredTo(b) << "（比大小時用這個即可）" << endl;

    cout << "\n=== 浮點比較要用容差 ===" << endl;
    // 注意：3-4-5 這組剛好能被二進位精確表示，所以本例 == 也成立。
    // 但這是「碰巧」，不是保證 —— 換一組座標就未必了，見下方第二個例子。
    double d = a.distanceTo(b);
    cout << "  3-4-5: d == 5.0 ?           " << (d == 5.0 ? "是" : "否")
         << "（此例碰巧精確，不可一般化）" << endl;

    Point p1{0, 0}, p2{0.1, 0.2};
    double d2 = p1.distanceTo(p2);
    double expected = sqrt(0.05);
    cout << "  0.1/0.2: d == sqrt(0.05) ?  " << (d2 == expected ? "是" : "否") << endl;
    cout << "  改用容差 fabs(d - e) < 1e-9 ? " << (fabs(d2 - expected) < 1e-9 ? "是" : "否")
         << "  ← 浮點比較的正確寫法" << endl;

    cout << "\n=== Rectangle ===" << endl;
    Rectangle r{5, 3};
    cout << "面積: " << r.area() << ", 周長: " << r.perimeter() << endl;

    cout << "\n=== const 正確性 ===" << endl;
    const Point& cref = b;
    cout << "  const 參考仍可呼叫 const 成員函式: ";
    cref.print();

    cout << "\n=== LeetCode 223. Rectangle Area ===" << endl;
    // 官方範例 1：ax1=-3,ay1=0,ax2=3,ay2=4, bx1=0,by1=-1,bx2=9,by2=2 → 45
    AxisAlignedRect r1{-3, 0, 3, 4};
    AxisAlignedRect r2{0, -1, 9, 2};
    cout << "  範例 1 總面積 = " << computeTotalArea(r1, r2) << "（預期 45）" << endl;
    // 官方範例 2：兩個完全重合的矩形 → 16
    AxisAlignedRect r3{-2, -2, 2, 2};
    AxisAlignedRect r4{-2, -2, 2, 2};
    cout << "  範例 2 總面積 = " << computeTotalArea(r3, r4) << "（預期 16）" << endl;
    // 補一個完全不相交的情況
    AxisAlignedRect r5{0, 0, 1, 1};
    AxisAlignedRect r6{5, 5, 6, 6};
    cout << "  不相交總面積 = " << computeTotalArea(r5, r6) << "（預期 2）" << endl;

    cout << "\n=== 實務：找最近的感測器 ===" << endl;
    vector<Sensor> sensors = {
        {101, {10.0,  5.0}},
        {102, { 2.0,  2.0}},
        {103, {-4.0,  8.0}},
        {104, { 1.5,  1.0}},
    };
    Point station{0.0, 0.0};
    const Sensor* nearest = findNearest(sensors, station);
    if (nearest) {
        cout << "  基地台 (0, 0) 最近的是感測器 #" << nearest->id
             << " 位於 (" << nearest->pos.x << ", " << nearest->pos.y << ")"
             << "，距離 " << nearest->pos.distanceTo(station) << endl;
    }

    cout << "\n=== 記憶體佈局（實作定義，本機 GCC 15.2 x86-64）===" << endl;
    cout << "  sizeof(Point) = " << sizeof(Point)
         << "（兩個 double；成員函式不佔空間）" << endl;
    cout << "  sizeof(Rectangle) = " << sizeof(Rectangle) << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 12 課：struct 與 class 的差異6.cpp" -o demo6

// === 預期輸出 ===
// === Point 與便利函式 ===
// (0, 0)
// (3, 4)
// 距離: 5
// 距離平方: 25（比大小時用這個即可）
//
// === 浮點比較要用容差 ===
//   3-4-5: d == 5.0 ?           是（此例碰巧精確，不可一般化）
//   0.1/0.2: d == sqrt(0.05) ?  否
//   改用容差 fabs(d - e) < 1e-9 ? 是  ← 浮點比較的正確寫法
//
// === Rectangle ===
// 面積: 15, 周長: 16
//
// === const 正確性 ===
//   const 參考仍可呼叫 const 成員函式: (3, 4)
//
// === LeetCode 223. Rectangle Area ===
//   範例 1 總面積 = 45（預期 45）
//   範例 2 總面積 = 16（預期 16）
//   不相交總面積 = 2（預期 2）
//
// === 實務：找最近的感測器 ===
//   基地台 (0, 0) 最近的是感測器 #104 位於 (1.5, 1)，距離 1.80278
//
// === 記憶體佈局（實作定義，本機 GCC 15.2 x86-64）===
//   sizeof(Point) = 16（兩個 double；成員函式不佔空間）
//   sizeof(Rectangle) = 16
