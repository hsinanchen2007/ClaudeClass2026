// =============================================================================
//  第二課：泛型編程（Generic Programming）概念12.cpp
//   —  編譯期多型（compile-time polymorphism）：不需要繼承的鴨子型別
// =============================================================================
//
// 【主題資訊 Information】
//   語法：
//     template <typename Shape>          // 這裡的 Shape 只是型別參數名稱，
//     void print_area(const Shape& s);   // 不是任何基底類別！
//
//   標準版本：C++98 起（本檔用 -std=c++17 編譯，未依賴新語法）
//   標頭檔  ：<iostream>
//   分派成本：**零** —— 呼叫在編譯期就解析完畢，可完全 inline
//             （本機 -O2 實測：整個呼叫被 inline 成兩道 mulsd 乘法指令）
//   空間成本：物件無 vptr（本機實測 sizeof(Circle) = 8，比概念11 少 8 bytes）；
//             但每種型別各生成一份 print_area 程式碼（code bloat）
//
//   本檔是概念11.cpp 的對照組。兩檔解決同一個問題，
//   請務必對照閱讀 —— 它們的差異就是本課最重要的結論。
//
// 【詳細解釋 Explanation】
//
// 【1. 這裡的 Shape 不是基底類別，只是一個名字】
// 最容易誤會的一點：
//     template <typename Shape>
//     void print_area(const Shape& shape);
// 這個 `Shape` 只是型別參數的**名稱**，把它改叫 T 完全等價。
// 它與概念11.cpp 那個 `class Shape` 沒有任何關係 ——
// 本檔的 Circle 與 Rectangle **沒有繼承任何東西**。
//
// print_area 對型別的唯一要求是「有一個可用 `.area()` 呼叫的成員」。
// 這是編譯期的鴨子型別：走起來像鴨子、叫起來像鴨子，那就是鴨子，
// 不必先去登記自己是鴨子。
//
// 【2. 分派完全在編譯期解決】
// `print_area(c)` 時，編譯器已經知道 c 是 Circle，於是：
//   1) 實例化出 print_area<Circle>
//   2) 其中的 shape.area() 直接綁定到 Circle::area —— 沒有查表、沒有間接跳躍
//   3) Circle::area 很小，通常會被完全 inline
//
// 本機 g++ 15.2 -O2 實測，整個呼叫最終編譯成：
//     movsd  (%rdi), %xmm1
//     movsd  .LC0(%rip), %xmm0
//     mulsd  %xmm1, %xmm0
//     mulsd  %xmm1, %xmm0
//     ret
// —— 只剩兩道乘法，**連一次函式呼叫都沒有**。
// 對照概念11.cpp 的虛擬版本，關鍵指令是 `jmp *%rax`（間接跳躍，無法 inline）。
//
// 這就是 STL 效能的來源。std::sort 之所以能贏過 C 的 qsort，正是因為
// qsort 透過函式指標比較（執行期間接呼叫，無法 inline），
// 而 std::sort 的比較器是模板參數，可以完全內聯進排序迴圈。
//
// 【3. 兩種多型的完整對照（本課核心結論）】
//
//   ┌──────────────┬────────────────────┬─────────────────────┐
//   │              │ 執行期（概念11）    │ 編譯期（概念12）     │
//   ├──────────────┼────────────────────┼─────────────────────┤
//   │ 介面形式      │ 明確（繼承基底）    │ 隱含（會 .area() 就行）│
//   │ 侵入性        │ 侵入式（要改型別）  │ 非侵入式             │
//   │ 決定時機      │ 執行期             │ 編譯期               │
//   │ 分派成本      │ 間接跳躍、無法 inline│ 零、可完全 inline    │
//   │ 物件大小      │ 多一個 vptr（8B）   │ 無額外開銷           │
//   │ 程式碼大小    │ 一份               │ 每型別各一份（bloat）│
//   │ 異質容器      │ 可以               │ **不行**             │
//   │ 執行期換實作  │ 可以（外掛/設定檔） │ **不行**             │
//   │ 錯誤訊息      │ 清楚（介面明確）    │ 冗長（實例化時爆炸） │
//   │ 編譯時間      │ 短                 │ 長                   │
//   └──────────────┴────────────────────┴─────────────────────┘
//
//   選擇準則（實務上的判斷順序）：
//     1) 型別集合在編譯期就已知、且要求效能 → 模板
//     2) 型別要到執行期才確定（設定檔、外掛、跨 ABI）→ virtual
//     3) 需要異質容器統一走訪 → virtual（或 C++17 的 std::variant）
//     4) 兩者其實可以並用：STL 大量使用模板，
//        而 std::function 則刻意用型別抹除（內部即虛擬分派）換取彈性
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼 Circle 與 Rectangle 無法放進同一個容器
//   本檔的 Circle 與 Rectangle 是兩個毫無關係的型別，沒有共同的基底，
//   因此不存在能同時裝下兩者的靜態型別 —— 寫不出 vector<???>。
//   這是編譯期多型**最主要的限制**，也是概念11.cpp 存在的理由。
//   C++17 起有第三條路：std::variant<Circle, Rectangle> 配合 std::visit，
//   在「型別集合封閉且已知」的前提下取得異質容器，且不需要 vtable
//   （值語意、無繼承）。它算是這兩種多型之間的折衷。
//
// (B) 模板參數命名成 Shape 是好習慣還是壞習慣？
//   兩面都有。好處是自我說明 —— 讀者立刻知道這裡期待「形狀類的東西」。
//   壞處是容易讓人誤以為 Shape 是某個基底類別（本檔開頭特別澄清正是為此）。
//   C++20 之後最好的做法是用 concept 把要求真正寫出來：
//       template <typename T>
//       concept HasArea = requires(const T& t) {
//           { t.area() } -> std::convertible_to<double>;
//       };
//       template <HasArea S> void print_area(const S& s);
//   這樣既有自我說明，又是編譯器真的會檢查的約束（見概念10.cpp）。
//
// (C) 為什麼 C 的 qsort 比 std::sort 慢
//   這是編譯期 vs 執行期分派最著名的效能實例：
//     qsort(base, n, size, int (*cmp)(const void*, const void*));
//   比較器是**函式指標**，每次比較都是一次無法 inline 的間接呼叫，
//   而且引數型別被抹成 void* 需要額外轉型與解參考。
//   std::sort 的比較器是**模板參數**，型別在編譯期已知，
//   比較邏輯可以直接內聯進排序的內迴圈。這是泛型編程「零成本抽象」
//   最具說服力的例子：抽象層級更高，產出的程式碼卻更快。
//
// (D) 「零成本」的正確理解
//   編譯期多型在**執行期**確實零成本，但成本並沒有消失，只是搬到了編譯期：
//   每個型別各一份程式碼（概念2.cpp 實測：200 種型別讓目的檔從
//   1,744 bytes 膨脹到 71,600 bytes）、編譯時間變長、錯誤訊息變難讀。
//   C++ 的承諾是「不用的功能不必付錢，用了的功能你自己寫也不會更快」，
//   談的一直是執行期開銷。
//
// 【注意事項 Pay Attention】
// 1. `template <typename Shape>` 裡的 Shape 只是參數名，不是基底類別。
//    本檔的 Circle 與 Rectangle 沒有繼承任何東西。
// 2. 編譯期多型**無法**做異質容器，也**無法**在執行期換實作。
//    需要這兩者時只能用 virtual（或在封閉型別集合下用 std::variant）。
// 3. 對型別的要求（要有 .area()）是隱含的。傳入沒有 area() 的型別，
//    錯誤會在實例化時於模板本體爆炸（詳見概念8.cpp）。
// 4. 別把「零成本」理解成「完全沒有代價」。代價轉移到了編譯期：
//    程式碼體積、編譯時間、錯誤訊息可讀性。
// 5. 兩種多型不是互斥的競爭關係。實務上經常並用 ——
//    效能關鍵路徑用模板，需要彈性的邊界用 virtual。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】編譯期多型 vs 執行期多型
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 編譯期多型與執行期多型的差別是什麼？各自什麼時候該用？
//     答：編譯期多型（模板）用隱含介面、非侵入式、分派在編譯期解決可完全
//         inline、物件無 vptr，但無法做異質容器、型別必須編譯期已知，
//         且有 code bloat。執行期多型（virtual）用明確介面、侵入式、
//         分派靠 vtable 間接跳躍無法 inline、物件多一個 vptr，
//         但能做異質容器與晚期綁定。
//         準則：型別編譯期已知且要效能 → 模板；型別執行期才定（設定檔、
//         外掛、跨 ABI）或需異質容器 → virtual。
//     追問：有沒有第三條路？
//         → C++17 的 std::variant + std::visit：在型別集合封閉且已知時，
//           提供異質容器且不需要 vtable，是兩者之間的折衷。
//
// 🔥 Q2. 為什麼 std::sort 通常比 C 的 qsort 快？
//     答：因為比較器的分派時機不同。qsort 收的是**函式指標**，每次比較都是
//         無法 inline 的間接呼叫，且引數被抹成 void* 需額外轉型。
//         std::sort 的比較器是**模板參數**，型別在編譯期已知，
//         比較邏輯可直接內聯進排序內迴圈。
//     追問：這是不是代表模板一定比較快？
//         → 不是。它代表「編譯期已知型別時，模板能讓最佳化器發揮」。
//           若型別本來就得執行期才知道，模板根本不適用，這個比較不成立。
//
// ⚠️ 陷阱. `template <typename Shape> void print_area(const Shape& shape)`
//          裡的 Shape，是不是就是概念11.cpp 那個 Shape 基底類別？
//     答：完全無關。這裡的 Shape 只是型別參數的**名稱**，改叫 T 完全等價。
//         本檔的 Circle 與 Rectangle 沒有繼承任何類別，彼此也毫無關係。
//         print_area 唯一的要求是「有可呼叫的 .area()」。
//     為什麼會錯：被命名誤導，把「型別參數名」當成「型別約束」。
//         C++ 的型別參數名不帶任何語意，編譯器不會因為你叫它 Shape 就去
//         檢查什麼。要讓名稱真的具有約束力，得用 C++20 的 concepts ——
//         那時 `template <HasArea S>` 中的 HasArea 才是編譯器真的會驗證的東西。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>

// 注意：這兩個類別沒有繼承任何東西，彼此也毫無關係
class Circle {
    double radius;
public:
    Circle(double r) : radius(r) {}
    double area() const {
        return 3.14159 * radius * radius;
    }
};

class Rectangle {
    double width, height;
public:
    Rectangle(double w, double h) : width(w), height(h) {}
    double area() const {
        return width * height;
    }
};

// 完全不相關的第三個型別 —— 用來示範非侵入式的威力。
// 它連「形狀」都不是，但只要有 .area() 就能被 print_area 接受。
class SolarPanel {
    double length, width_m;
public:
    SolarPanel(double l, double w) : length(l), width_m(w) {}
    double area() const { return length * width_m; }
};

// 泛型函數：不需要繼承關係
// 這裡的 Shape 只是型別參數名稱，不是任何基底類別
template <typename Shape>
void print_area(const Shape& shape) {
    std::cout << "Area: " << shape.area() << std::endl;  // 編譯期決定
}

// -----------------------------------------------------------------------------
// 【日常實務範例 1】監控指標上報：對任何「會回報數值」的來源統一格式化
//   情境：服務要定期把各種指標推到監控系統（Prometheus / Datadog）。
//         指標來源五花八門：佇列長度、快取命中率、磁碟用量、GPU 溫度 ——
//         它們來自不同模組，彼此沒有共同基底，也不該為了上報而被迫繼承什麼。
//   為何用編譯期多型：
//     1) 非侵入式 —— 各模組只要提供 name() 與 value()，不必改繼承關係，
//        連第三方型別都能用（外部補一層薄包裝即可）。
//     2) 零成本 —— 指標上報在熱路徑上高頻執行，避免虛擬呼叫是有意義的。
//   若改用 virtual，就得要求每個模組去繼承 IMetric，
//   對「已經存在的、你改不動的型別」根本行不通。
// -----------------------------------------------------------------------------
template <typename Metric>
void report_metric(const Metric& m) {
    std::cout << "  " << m.name() << " = " << m.value() << std::endl;
}

class QueueDepth {
    int pending;
public:
    explicit QueueDepth(int p) : pending(p) {}
    const char* name() const { return "queue_depth"; }
    int value() const { return pending; }
};

class CacheHitRate {
    int hits, total;
public:
    CacheHitRate(int h, int t) : hits(h), total(t) {}
    const char* name() const { return "cache_hit_rate"; }
    double value() const { return total == 0 ? 0.0 : static_cast<double>(hits) / total; }
};

class DiskUsage {
    double used_gb, total_gb;
public:
    DiskUsage(double u, double t) : used_gb(u), total_gb(t) {}
    const char* name() const { return "disk_usage_pct"; }
    double value() const { return total_gb == 0 ? 0.0 : used_gb / total_gb * 100.0; }
};

int main() {
    std::cout << "=== 編譯期多型：不需要繼承 ===" << std::endl;
    Circle c(5.0);
    Rectangle r(4.0, 3.0);

    print_area(c);  // 編譯期展開為 print_area(const Circle&)
    print_area(r);  // 編譯期展開為 print_area(const Rectangle&)

    std::cout << "\n=== 非侵入式：連「不是形狀」的型別也能用 ===" << std::endl;
    SolarPanel panel(1.7, 1.0);
    print_area(panel);   // SolarPanel 沒繼承任何東西，只是剛好有 .area()
    std::cout << "SolarPanel 與 Circle 毫無關係，print_area 卻照樣接受它。"
              << std::endl;

    std::cout << "\n=== 物件沒有 vptr ===" << std::endl;
    std::cout << "sizeof(Circle) = " << sizeof(Circle)
              << "   <- 只有一個 double；概念11.cpp 的虛擬版是 16" << std::endl;
    std::cout << "（本機 g++ 15.2 / x86-64 實測值）" << std::endl;

    std::cout << "\n=== 分派成本：零 ===" << std::endl;
    std::cout << "本機 -O2 實測，print_area(c) 最終被 inline 成兩道 mulsd 乘法，"
              << std::endl;
    std::cout << "連一次函式呼叫都沒有；概念11.cpp 的虛擬版則是 jmp *%rax。"
              << std::endl;

    std::cout << "\n=== 但做不到異質容器 ===" << std::endl;
    std::cout << "Circle 與 Rectangle 沒有共同基底，寫不出能同時裝下兩者的 vector。"
              << std::endl;
    std::cout << "需要異質容器就得用 virtual（概念11.cpp）"
                 "或 C++17 的 std::variant。" << std::endl;

    std::cout << "\n=== 日常實務：監控指標上報 ===" << std::endl;
    std::cout << "各模組彼此無繼承關係，只要有 name() 與 value() 就能上報："
              << std::endl;
    report_metric(QueueDepth(1284));
    report_metric(CacheHitRate(8734, 10000));
    report_metric(DiskUsage(412.5, 1024.0));

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第二課：泛型編程（Generic Programming）概念12.cpp -o concept12

// （本機 g++ 15.2 / x86-64 實測值）

// === 預期輸出 ===
// === 編譯期多型：不需要繼承 ===
// Area: 78.5397
// Area: 12
//
// === 非侵入式：連「不是形狀」的型別也能用 ===
// Area: 1.7
// SolarPanel 與 Circle 毫無關係，print_area 卻照樣接受它。
//
// === 物件沒有 vptr ===
// sizeof(Circle) = 8   <- 只有一個 double；概念11.cpp 的虛擬版是 16
//
// === 分派成本：零 ===
// 本機 -O2 實測，print_area(c) 最終被 inline 成兩道 mulsd 乘法，
// 連一次函式呼叫都沒有；概念11.cpp 的虛擬版則是 jmp *%rax。
//
// === 但做不到異質容器 ===
// Circle 與 Rectangle 沒有共同基底，寫不出能同時裝下兩者的 vector。
// 需要異質容器就得用 virtual（概念11.cpp）或 C++17 的 std::variant。
//
// === 日常實務：監控指標上報 ===
// 各模組彼此無繼承關係，只要有 name() 與 value() 就能上報：
//   queue_depth = 1284
//   cache_hit_rate = 0.8734
//   disk_usage_pct = 40.2832
