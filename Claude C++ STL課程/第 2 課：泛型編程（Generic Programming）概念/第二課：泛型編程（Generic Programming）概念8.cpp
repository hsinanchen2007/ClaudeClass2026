// =============================================================================
//  第二課：泛型編程（Generic Programming）概念8.cpp
//   —  當型別不滿足隱含介面：錯誤在哪裡爆炸、訊息為什麼指向別人的程式碼
// =============================================================================
//
// 【主題資訊 Information】
//   本檔是**刻意保留的失敗案例**，但它可以正常編譯執行 ——
//   因為那行會失敗的程式碼在 main() 裡被註解起來了
//   （搜尋「Point max_point = find_max(p1, p2);」即可找到）。
//   把它取消註解重新編譯，就能親眼看到本檔要講的錯誤。
//
//   標準版本：C++98 起（本檔用 -std=c++17 編譯）
//   標頭檔  ：<iostream>
//   關鍵詞  ：implicit interface、instantiation-time error、
//             instantiation backtrace（實例化鏈）
//
//   相關檔案：概念3.cpp 說明什麼是隱含介面；
//             概念9.cpp 示範怎麼修好它；
//             概念10.cpp 用 C++20 concepts 讓錯誤訊息變得可讀。
//
// 【詳細解釋 Explanation】
//
// 【1. 錯在哪裡：Point 沒有 operator>】
// find_max 的本體寫了 `a > b`，這條隱含要求對 int、double、std::string 都成立，
// 但 Point 只是一個裝了兩個 int 的類別，沒有定義 operator>。
// 所以 find_max(p1, p2) 在實例化 find_max<Point> 時失敗。
//
// 關鍵是**失敗的時機**：Point 這個類別本身完全合法，find_max 這個模板本身也
// 完全合法，錯誤只在「兩者相遇」時才出現。這正是概念3.cpp 講的兩階段編譯：
// 與 T 有關的檢查一律推遲到實例化。
//
// 【2. 錯誤訊息的實際長相（本機 g++ 15.2 實測全文）】
// 把那行取消註解後，編譯器輸出如下（共 7 行）：
//
//     e8.cpp: In instantiation of 'T find_max(T, T) [with T = Point]':
//     e8.cpp:4:55:   required from here
//         4 | ... Point m = find_max(p1, p2); ...
//           |               ~~~~~~~~^~~~~~~~
//     e8.cpp:2:56: error: no match for 'operator>' (operand types are 'Point' and 'Point')
//         2 | template <typename T> T find_max(T a, T b) { return (a > b) ? a : b; }
//           |                                                     ~~~^~~~
//
// 請仔細看這則訊息的結構，它有三個部分，這個結構在所有模板錯誤中都一樣：
//
//   (a) "In instantiation of ... [with T = Point]"
//       → 告訴你「是在實例化哪個模板、代入什麼型別」時出的事。
//         **這是最重要的一行**，多數人卻直接跳過。
//   (b) "required from here"
//       → 指向**你寫的那一行呼叫**。這是你唯一能改的地方。
//   (c) "error: no match for 'operator>'"
//       → 指向**模板作者寫的那一行**。錯誤位置在別人的程式碼裡。
//
// 【3. 為什麼錯誤訊息指向「別人的程式碼」】
// 這是模板錯誤最違反直覺的地方：你明明只是呼叫了一個函式，錯誤卻報在
// 函式庫的原始碼裡（若換成 std::sort，就會報在 <bits/stl_algo.h> 深處）。
//
// 原因是編譯器**真的**在那裡失敗了 —— 它把 Point 代入模板後，
// 逐行檢查模板本體，在 `a > b` 這一行發現沒有可用的 operator>。
// 從編譯器的角度，出錯的位置就是那裡。
//
// 實務上的閱讀順序應該是**由下往上、先找 [with T = ...]**：
//   1) 先看 [with T = ...]，確認是哪個型別引發的
//   2) 再看 required from here，找到你自己的那行呼叫
//   3) 最後才看 error 本身，理解缺了什麼能力
// 這個順序能讓你在幾秒內定位 90% 的模板錯誤。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 錯誤訊息長度隨模板深度爆炸
//   本檔只有一層模板，錯誤才 7 行。但若是多層巢狀（STL 演算法內部常有 3~5 層），
//   訊息會長得多。本機 g++ 15.2 實測，對沒有 operator< 的 Point 呼叫
//   std::sort(v.begin(), v.end())，錯誤訊息高達 **131 行**。
//   這正是 C++ 模板長年被詬病的體驗問題，也是 concepts 的直接動機
//   （概念10.cpp 有本機實測的前後對照）。
//
// (B) 三種讓錯誤變好讀的手段
//   1) static_assert（C++11）—— 在模板開頭主動檢查並給出人話訊息：
//          static_assert(std::is_arithmetic<T>::value, "find_max 只接受算術型別");
//      優點是立刻可用；缺點是它只能檢查 type traits 能表達的性質，
//      而且不影響 overload resolution（見下）。
//   2) SFINAE / std::enable_if（C++11）—— 讓不滿足條件的多載安靜地退出候選集。
//      能影響 overload resolution，但語法極為難讀。
//   3) concepts（C++20）—— 前兩者的正統取代品，語法直觀且參與多載決議。
//
// (C) static_assert 與 concepts 的關鍵差異
//   static_assert 是「**已經選中這個函式了**，然後才發現型別不對，於是報錯」。
//   concepts 是「這個函式**根本不該被列入候選**」。
//   差別在有多個多載時特別重要：
//     * 用 static_assert：不合適的多載仍會被選中，然後硬生生報錯，
//       即使旁邊有另一個完全適用的多載也救不了。
//     * 用 concepts：不滿足約束的多載自動退出候選集，
//       編譯器會順利選中另一個適用的版本。
//   這就是為什麼 concepts 不只是「更好看的 static_assert」。
//
// 【注意事項 Pay Attention】
// 1. 本檔可以正常編譯執行 —— 因為失敗的那行被註解掉了。
//    這是刻意的：讓檔案能通過驗證，同時保留可親手重現的錯誤。
//    **請務必自己取消註解編譯一次**，親眼看過那則訊息比讀十遍說明有用。
// 2. Point 本身完全合法，find_max 本身也完全合法。不要因為錯誤訊息指向
//    模板本體，就誤以為是模板寫錯了。
// 3. 「編譯器報錯在函式庫裡」不代表函式庫有 bug。99.9% 的情況是你的型別
//    不滿足該模板的隱含要求。
// 4. 錯誤訊息裡的 [with T = Point] 是最有價值的線索。閱讀模板錯誤時
//    請先找這一段，而不是從第一行開始逐字讀。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】模板錯誤診斷
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 對一個沒有 operator> 的自訂型別呼叫 find_max，
//        錯誤會在什麼時候、什麼位置出現？
//     答：在**實例化時**出現，位置在**模板本體**那一行（`a > b`），
//         不是在你的呼叫那一行。編譯器會另外用
//         "In instantiation of ... [with T = Point]" 與 "required from here"
//         把你的呼叫點串起來。型別本身與模板本身都合法，錯誤只在兩者結合時發生。
//     追問：那要怎麼快速定位這種錯誤？
//         → 先找 [with T = ...] 確認是哪個型別，再看 required from here
//           找到自己的呼叫行，最後才讀 error 本身。由下往上讀比從頭讀快得多。
//
// 🔥 Q2. 為什麼 STL 的模板錯誤訊息會這麼長？
//     答：因為錯誤發生在實例化之後，編譯器必須把整條實例化鏈
//         （instantiation backtrace）印出來才能說清楚「誰導致誰」。
//         層數越深訊息越長 —— 本機實測，一層模板 7 行，
//         而對缺少 operator< 的型別呼叫 std::sort 則是 131 行。
//     追問：C++20 怎麼改善這件事？
//         → concepts 把要求提前到「函式簽名」層級，違反約束時在多載決議階段
//           就能報「constraints not satisfied」並指出是哪條 requirement 沒滿足，
//           不必再展開整個實例化過程。
//
// ⚠️ 陷阱. 既然錯誤報在函式庫的原始碼行號上，是不是代表那個函式庫有 bug？
//     答：不是。編譯器是**真的**在那一行失敗的 —— 它把你的型別代入模板後，
//         在那裡發現缺少所需的運算。位置指向函式庫，責任在你的型別。
//     為什麼會錯：把「錯誤位置」等同於「錯誤責任」。一般函式呼叫時這兩者
//         通常一致（型別不符會報在呼叫點），但模板把「檢查」推遲到了
//         實例化，於是檢查失敗的位置自然落在模板本體裡。
//         要正確歸因，就得靠 required from here 那條線索往回追。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>

template <typename T>
T find_max(T a, T b) {
    return (a > b) ? a : b;  // 使用了 > 運算子 —— 這就是隱含要求
}

// 自訂類別，沒有定義 > 運算子
class Point {
public:
    int x, y;
    Point(int x, int y) : x(x), y(y) {}
};

// 為了讓 main 能印出 Point（這只是輸出用，與 operator> 無關）
std::ostream& operator<<(std::ostream& os, const Point& p) {
    return os << "(" << p.x << ", " << p.y << ")";
}

int main() {
    Point p1(1, 2), p2(3, 4);

    std::cout << "=== Point 本身完全合法 ===" << std::endl;
    std::cout << "p1 = " << p1 << ", p2 = " << p2 << std::endl;
    std::cout << "Point 可以建構、可以複製、可以印出來 —— 它沒有任何問題。"
              << std::endl;

    std::cout << "\n=== find_max 本身也完全合法 ===" << std::endl;
    std::cout << "find_max(10, 20) = " << find_max(10, 20)
              << "   <- 對 int 運作正常" << std::endl;
    std::cout << "find_max(2.5, 1.5) = " << find_max(2.5, 1.5)
              << "   <- 對 double 也正常" << std::endl;

    std::cout << "\n=== 但兩者相遇就爆炸 ===" << std::endl;
    // ↓↓↓ 把下面這行取消註解，就能親眼看到本檔要講的錯誤 ↓↓↓
    // Point max_point = find_max(p1, p2);  // 編譯錯誤！
    //
    // 本機 g++ 15.2 實測的完整錯誤訊息（共 7 行）：
    //   In instantiation of 'T find_max(T, T) [with T = Point]':
    //   required from here
    //         Point max_point = find_max(p1, p2);
    //                           ~~~~~~~~^~~~~~~~
    //   error: no match for 'operator>' (operand types are 'Point' and 'Point')
    //         return (a > b) ? a : b;
    //                ~~~^~~~
    std::cout << "find_max(p1, p2) 無法編譯：" << std::endl;
    std::cout << "  error: no match for 'operator>' "
                 "(operand types are 'Point' and 'Point')" << std::endl;
    std::cout << "  錯誤位置在 find_max 的函式本體，不是在呼叫這一行。"
              << std::endl;

    std::cout << "\n=== 怎麼讀這則錯誤 ===" << std::endl;
    std::cout << "1) 先找 [with T = Point]  -> 確認是哪個型別引發的" << std::endl;
    std::cout << "2) 再看 required from here -> 找到自己寫的那行呼叫" << std::endl;
    std::cout << "3) 最後讀 error 本身       -> 理解缺了什麼能力" << std::endl;
    std::cout << "由下往上讀，比從第一行逐字讀快得多。" << std::endl;

    std::cout << "\n=== 解法 ===" << std::endl;
    std::cout << "為 Point 定義 operator>（見概念9.cpp），" << std::endl;
    std::cout << "或用 C++20 concepts 把要求寫進簽名（見概念10.cpp）。"
              << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第二課：泛型編程（Generic Programming）概念8.cpp -o concept8

// === 預期輸出 ===
// === Point 本身完全合法 ===
// p1 = (1, 2), p2 = (3, 4)
// Point 可以建構、可以複製、可以印出來 —— 它沒有任何問題。
//
// === find_max 本身也完全合法 ===
// find_max(10, 20) = 20   <- 對 int 運作正常
// find_max(2.5, 1.5) = 2.5   <- 對 double 也正常
//
// === 但兩者相遇就爆炸 ===
// find_max(p1, p2) 無法編譯：
//   error: no match for 'operator>' (operand types are 'Point' and 'Point')
//   錯誤位置在 find_max 的函式本體，不是在呼叫這一行。
//
// === 怎麼讀這則錯誤 ===
// 1) 先找 [with T = Point]  -> 確認是哪個型別引發的
// 2) 再看 required from here -> 找到自己寫的那行呼叫
// 3) 最後讀 error 本身       -> 理解缺了什麼能力
// 由下往上讀，比從第一行逐字讀快得多。
//
// === 解法 ===
// 為 Point 定義 operator>（見概念9.cpp），
// 或用 C++20 concepts 把要求寫進簽名（見概念10.cpp）。
