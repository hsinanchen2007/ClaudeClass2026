// =============================================================================
//  第 10 課：vector 的宣告與初始化方式 4  —  ★ 小括號 vs 大括號：本課最大陷阱
// =============================================================================
//
// 【主題資訊 Information】
//   兩組互相競爭的建構子：
//     vector(size_type count, const T& value);          // (A) 小括號會選中
//     vector(std::initializer_list<T> init);            // (B) 大括號優先選中
//
//   標頭檔　：<vector>
//   標準版本：(A) C++98、(B) C++11
//   一句話　：() 是「建構子參數」，{} 是「元素清單」。
//
//     std::vector<int> v1(5, 10);   // 5 個元素，每個都是 10  → {10,10,10,10,10}
//     std::vector<int> v2{5, 10};   // 2 個元素：5 和 10       → {5, 10}
//     std::vector<int> v3(5);       // 5 個元素，都是 0        → {0,0,0,0,0}
//     std::vector<int> v4{5};       // 1 個元素，值是 5        → {5}
//
// 【詳細解釋 Explanation】
//
// 【1. 規則到底是什麼？——「強烈優先」而不是「絕對」】
//   標準（[over.match.list]）規定，用大括號 {...} 做 list-initialization 時，
//   多載決議分成兩個階段：
//     ① 第一階段：只考慮 initializer_list 建構子。
//        只要有任何一個 initializer_list 建構子「可行（viable）」，就選它，
//        而且完全不看其他建構子——哪怕別的建構子明顯更「合適」。
//     ② 第二階段：只有在第一階段一個可行候選都沒有時，
//        才退回去考慮所有建構子（此時 {} 的行為就跟 () 差不多）。
//
//   所以正確的說法是「大括號強烈偏好 initializer_list」，
//   而不是坊間常說的「大括號一定是 initializer_list」。
//   下面第 2 點就是「一定」這個說法會出錯的地方。
//
// 【2. 反例：vector<std::string> v{3, "hi"} 竟然是 3 個 "hi"】
//   本機實測結果（會讓很多人跌破眼鏡）：
//       std::vector<std::string> d{3, "hi"};   // size=3，內容 hi hi hi
//       std::vector<std::string> e(3, "hi");   // size=3，內容 hi hi hi  ← 一模一樣！
//   為什麼？因為第一階段要找 initializer_list<std::string> 建構子，
//   而 int 3 無法轉換成 std::string → 這個候選「不可行」→ 第一階段落空
//   → 進入第二階段，(count, value) 建構子雀屏中選。
//
//   對比 vector<int>{5, 10}：int 5 和 int 10 都能當 initializer_list<int> 的元素，
//   第一階段成立，於是得到兩個元素。同樣是 {數字, 值} 的寫法，
//   只因元素型別不同就走了完全不同的分支——這正是它危險的地方。
//
//   延伸驗證：vector<double>{5, 10} 是 2 個元素（5.0 和 10.0），
//   因為 int → double 是合法的隱式轉換，第一階段依然成立。
//
// 【3. 為什麼標準要這樣設計？】
//   因為 initializer_list 的設計目的就是「讓大括號列舉初值」成為第一直覺。
//   如果它只是普通候選，vector<int>{3, 7} 就會因為 (count, value) 也可行而產生歧義，
//   使用者每次都得手動消歧，大括號語法就失去意義了。
//   代價就是本課這個陷阱：當你「想呼叫 (count, value)」卻用了大括號時，
//   編譯器不會警告，只會安靜地給你另一種結果。
//
// 【4. 實務守則】
//   * 要「n 個相同的值」→ 一律用小括號 vector<int> v(n, val)。
//   * 要「列舉具體元素」→ 用大括號 vector<int> v{a, b, c}。
//   * 心裡不確定時，就把它拆開寫或加註解——這種 bug 靠讀 code 很難發現，
//     因為兩種寫法都能編譯、都能跑，只是結果不同。
//   * Code review 時看到 vector<int> v{n, val} 這種形狀，一律停下來確認意圖。
//
// 【概念補充 Concept Deep Dive】
//   ▸ 為什麼這個 bug 特別難抓？
//     因為它不是編譯錯誤也不是崩潰，而是「size 不對」。
//     vector<int> buf{1024, 0} 的作者以為得到 1024 個 0，實際只有 2 個元素
//     {1024, 0}。後續 buf[500] 越界 → UB，可能當下沒事、隔很久才在別處爆炸。
//     這類「初始化寫錯導致的靜默越界」在真實專案裡屢見不鮮。
//
//   ▸ 與 auto 的交互作用（C++11 vs C++17 行為差異）：
//     auto x{1};  在 C++11/14 推導為 std::initializer_list<int>，
//     C++17 起改為 int（N3922 決議）。而 auto x = {1}; 在各版本都是
//     initializer_list<int>。這是另一個大括號相關的版本陷阱，
//     不影響 vector 但常一起被問到。
//
//   ▸ 這個陷阱不是 vector 獨有：任何同時提供 initializer_list 建構子
//     和其他建構子的類別都有（例如 std::map、使用者自訂容器）。
//
// 【注意事項 Pay Attention】
//   1. ★ vector<int> v(5, 10) 是五個 10；vector<int> v{5, 10} 是 5 和 10。
//   2. ★ vector<int> v(5) 是五個 0；vector<int> v{5} 是一個 5。
//   3. 但 vector<string> v{3, "hi"} 是三個 "hi"——因為 initializer_list<string>
//      不可行而退回 (count, value)。別把「大括號一定是串列」當成鐵律。
//   4. 元素型別是 double 時 vector<double>{5, 10} 仍是 2 個元素，
//      因為 int → double 的隱式轉換讓第一階段成立。
//   5. vector<int> v(); 完全是另一回事——那是 most vexing parse，
//      宣告了一個函式，不是 vector 物件。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】小括號 vs 大括號
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. vector<int> a(5, 10); 和 vector<int> b{5, 10}; 分別是什麼？
//     答：a 有 5 個元素，每個都是 10；b 有 2 個元素，分別是 5 和 10。
//         小括號呼叫 (count, value) 建構子；大括號優先選 initializer_list。
//     追問：那 vector<int> c(5); 和 vector<int> d{5}; 呢？
//         → c 是 5 個 0（值初始化），d 是 1 個元素值為 5。
//
// 🔥 Q2. 為什麼大括號會「優先」選 initializer_list 建構子？
//     答：標準規定 list-initialization 分兩階段：先只看 initializer_list
//         建構子，只要有可行的就選它、完全不考慮其他建構子；
//         一個可行的都沒有時才退回去看所有建構子。
//         這是為了讓「大括號列舉初值」成為無歧義的第一直覺。
//     追問：那有沒有辦法用大括號呼叫到 (count, value)？
//         → 有，就是讓 initializer_list 版本不可行，見下面的陷阱題。
//
// ⚠️ 陷阱. std::vector<std::string> v{3, "hi"}; 的 size() 是多少？
//     答：3，內容是 "hi" "hi" "hi"（本機實測）。
//         因為 initializer_list<std::string> 需要把 int 3 轉成 std::string，
//         做不到 → 第一階段沒有可行候選 → 退回第二階段，
//         選中 (count, value) 建構子，等同 vector<string> v(3, "hi")。
//     為什麼會錯：多數人記的是「{} 一定是 initializer_list」這條簡化規則，
//         於是答「2 個元素」。正確的規則是「強烈優先，但不可行時會退回」。
//         同樣形狀的 vector<int>{3, 7} 卻真的是 2 個元素——差別只在
//         元素型別能不能接受那個數字，這才是這題真正的考點。
//
// ⚠️ 陷阱 2. std::vector<int> v(); 建立了什麼？
//     答：什麼都沒建立。這是 most vexing parse——編譯器把它解讀成
//         「宣告一個名為 v、不吃參數、回傳 vector<int> 的函式」。
//         之後寫 v.size() 會編譯錯誤。
//     為什麼會錯：以為「加空括號 = 呼叫預設建構子」。
//         正確寫法是 vector<int> v; 或 vector<int> v{};——
//         這正是大括號初始化被推薦的理由之一：它沒有這個歧義。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <vector>

// 小工具：印出 vector 的 size 與內容
template <typename T>
void show(const std::string& expr, const std::vector<T>& v) {
    std::cout << "  " << expr;
    // 對齊用的填空
    for (std::size_t i = expr.size(); i < 30; ++i) std::cout << ' ';
    std::cout << "size=" << v.size() << "  內容: ";
    for (const T& x : v) std::cout << x << " ";
    std::cout << "\n";
}

// -----------------------------------------------------------------------------
// 【日常實務範例】影像處理：配置固定大小的像素緩衝區
//   情境：讀入一張 width × height 的灰階圖，需要一塊初值為 0（黑）的緩衝區。
//   為什麼這裡是活生生的陷阱：
//     正確 → std::vector<unsigned char> buf(width * height, 0);   // N 個 0
//     錯誤 → std::vector<unsigned char> buf{width * height, 0};   // 只有 2 個元素！
//   錯誤版本能編譯、能跑，但緩衝區只有 2 bytes，
//   之後 buf[i] 寫入影像資料就是靜默的堆積越界——
//   可能到很久以後才在完全無關的地方崩潰，極難追查。
// -----------------------------------------------------------------------------
class GrayImage {
public:
    GrayImage(std::size_t width, std::size_t height)
        : w_(width), h_(height), pixels_(width * height, 0) {}   // ← 必須是小括號

    void setPixel(std::size_t x, std::size_t y, unsigned char v) {
        if (x < w_ && y < h_) pixels_[y * w_ + x] = v;
    }

    // 計算平均亮度，用來驗證緩衝區真的有 w*h 個格子
    double averageBrightness() const {
        long sum = 0;
        for (unsigned char p : pixels_) sum += p;
        return pixels_.empty() ? 0.0 : static_cast<double>(sum) / pixels_.size();
    }

    std::size_t bufferSize() const { return pixels_.size(); }

private:
    std::size_t                w_;
    std::size_t                h_;
    std::vector<unsigned char> pixels_;
};

int main() {
    std::cout << "=== 核心對照：同樣的數字，括號決定一切 ===\n";
    std::vector<int> v1(5, 10);
    std::vector<int> v2{5, 10};
    show("vector<int> v1(5, 10)", v1);
    show("vector<int> v2{5, 10}", v2);

    std::vector<int> v3(5);
    std::vector<int> v4{5};
    show("vector<int> v3(5)", v3);
    show("vector<int> v4{5}", v4);

    std::vector<int> v5(3, 7);
    std::vector<int> v6{3, 7};
    show("vector<int> v5(3, 7)", v5);
    show("vector<int> v6{3, 7}", v6);

    std::cout << "\n=== 反例：大括號並非「一定」是 initializer_list ===\n";
    std::vector<std::string> s1{3, "hi"};   // initializer_list<string> 不可行 → 退回 (n, val)
    std::vector<std::string> s2(3, "hi");
    show("vector<string> s1{3, \"hi\"}", s1);
    show("vector<string> s2(3, \"hi\")", s2);
    std::cout << "  → 兩者結果相同！因為 int 3 無法轉成 std::string，\n"
              << "    initializer_list<string> 這個候選不可行，於是退回 (count, value)。\n";

    std::cout << "\n=== 對照組：元素型別能接受該數字時，第一階段就成立 ===\n";
    std::vector<double> d1{5, 10};          // int → double 可行 → 仍是 2 個元素
    show("vector<double> d1{5, 10}", d1);
    std::cout << "  → int 可隱式轉成 double，所以 initializer_list<double> 可行，\n"
              << "    結果是 2 個元素，和 vector<int>{5,10} 一樣。\n";

    std::cout << "\n=== 為什麼這個 bug 難抓：size 悄悄變成 2 ===\n";
    std::vector<int> wrongBuf{1024, 0};     // 作者以為是 1024 個 0
    std::vector<int> rightBuf(1024, 0);     // 真正的 1024 個 0
    std::cout << "  vector<int> buf{1024, 0} -> size=" << wrongBuf.size()
              << "（作者以為 1024，實際只有 2 個元素：1024 和 0）\n";
    std::cout << "  vector<int> buf(1024, 0) -> size=" << rightBuf.size()
              << "（這才是正確寫法）\n";
    std::cout << "  兩者都能編譯、都不會警告——只有執行期的越界才會顯現問題。\n";

    std::cout << "\n=== 日常實務：影像緩衝區必須用小括號 ===\n";
    GrayImage img(8, 4);                    // 8x4 = 32 個像素
    std::cout << "  GrayImage(8, 4) 緩衝區大小 = " << img.bufferSize()
              << "（= 8 × 4，若誤用大括號只會得到 2）\n";
    std::cout << "  初始平均亮度 = " << img.averageBrightness() << "（全黑）\n";
    // 在中間畫一條亮線
    for (std::size_t x = 0; x < 8; ++x) img.setPixel(x, 2, 200);
    std::cout << "  畫一條亮線後平均亮度 = " << img.averageBrightness()
              << "（8 個 200 分攤到 32 格 = 50）\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 10 課：vector 的宣告與初始化方式4.cpp" -o lesson10_4

// === 預期輸出 ===
// === 核心對照：同樣的數字，括號決定一切 ===
//   vector<int> v1(5, 10)         size=5  內容: 10 10 10 10 10
//   vector<int> v2{5, 10}         size=2  內容: 5 10
//   vector<int> v3(5)             size=5  內容: 0 0 0 0 0
//   vector<int> v4{5}             size=1  內容: 5
//   vector<int> v5(3, 7)          size=3  內容: 7 7 7
//   vector<int> v6{3, 7}          size=2  內容: 3 7
//
// === 反例：大括號並非「一定」是 initializer_list ===
//   vector<string> s1{3, "hi"}    size=3  內容: hi hi hi
//   vector<string> s2(3, "hi")    size=3  內容: hi hi hi
//   → 兩者結果相同！因為 int 3 無法轉成 std::string，
//     initializer_list<string> 這個候選不可行，於是退回 (count, value)。
//
// === 對照組：元素型別能接受該數字時，第一階段就成立 ===
//   vector<double> d1{5, 10}      size=2  內容: 5 10
//   → int 可隱式轉成 double，所以 initializer_list<double> 可行，
//     結果是 2 個元素，和 vector<int>{5,10} 一樣。
//
// === 為什麼這個 bug 難抓：size 悄悄變成 2 ===
//   vector<int> buf{1024, 0} -> size=2（作者以為 1024，實際只有 2 個元素：1024 和 0）
//   vector<int> buf(1024, 0) -> size=1024（這才是正確寫法）
//   兩者都能編譯、都不會警告——只有執行期的越界才會顯現問題。
//
// === 日常實務：影像緩衝區必須用小括號 ===
//   GrayImage(8, 4) 緩衝區大小 = 32（= 8 × 4，若誤用大括號只會得到 2）
//   初始平均亮度 = 0（全黑）
//   畫一條亮線後平均亮度 = 50（8 個 200 分攤到 32 格 = 50）
