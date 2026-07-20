// =============================================================================
//  第八課：函數物件 1  —  什麼是函數物件（Functor）：重載 operator()
// =============================================================================
//
// 【主題資訊 Information】
//   語法：在類別中重載 operator()，該類別的實例就能像函數一樣被呼叫。
//       class Adder {
//       public:
//           int operator()(int a, int b) const { return a + b; }
//       };
//       Adder add;  add(3, 5);      // 等同 add.operator()(3, 5)
//   別名：函數物件（function object）、仿函數（functor）、可呼叫物件（callable）
//   標頭檔：不需要（語言特性）；<functional> 提供現成的一批。
//   標準版本：C++98 起即有。C++11 的 lambda 本質上就是編譯器自動生成的函數物件。
//   複雜度：呼叫成本等同一次普通成員函式呼叫，通常會被完全內聯（inline）。
//
// 【詳細解釋 Explanation】
//
// 【1. operator() 是唯一可以有「任意數量參數」的運算子】
//   C++ 的運算子多半有固定參數個數：operator+ 是二元、operator! 是一元。
//   operator() 是例外——它想收幾個參數就收幾個，也可以重載多個版本：
//       int  operator()(int a, int b) const;
//       int  operator()(int a) const;
//       void operator()() const;
//   這個彈性正是它能「模擬函數」的原因。
//   而且它是**成員函式**，所以可以存取類別的成員變數——
//   這就帶出下一點，也是函數物件真正的價值。
//
// 【2. 函數物件的真正價值：它是「帶狀態的函數」】
//   普通函數只有程式碼，沒有資料。要讓它記住東西只能靠全域變數
//   （不可重入、不執行緒安全）或每次都多傳一個參數（污染介面）。
//   函數物件是**物件**，成員變數就是它的狀態：
//       class GreaterThan { int t; public: bool operator()(int x) const { return x > t; } };
//       GreaterThan gt5(5), gt7(7);      // 兩個獨立的「函數」，各自記住自己的門檻
//   這件事普通函數辦不到。第 4 檔會完整示範。
//
// 【3. 為什麼 STL 大量使用函數物件而不是函數指標：效能】
//   把普通函數傳給 std::sort，傳過去的是一個**函數指標**（執行期的位址），
//   編譯器通常無法內聯它——每次比較都是一次真正的間接呼叫。
//   把函數物件傳過去，傳的是一個**型別**（每個 functor 是不同的型別），
//   模板實例化時 operator() 的內容直接可見，可以完全內聯展開。
//   這就是為什麼 C++ 的 std::sort 常常打敗 C 的 qsort——
//   qsort 只能收函數指標，每次比較都無法內聯。第 3 檔有實測對照。
//
// 【4. const 該不該加】
//   本例的 operator() 標了 const，代表「呼叫它不會改變物件狀態」。
//   建議預設都加 const：
//     * STL 演算法常以 const 參考持有述詞（predicate），非 const 版本會編譯失敗。
//     * 述詞若有副作用，演算法可能因為複製而給出出乎意料的結果（第 5 檔）。
//   只有在刻意要累積狀態時（例如計數器）才拿掉 const。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 「可呼叫物件（Callable）」在 C++ 裡有五種
//     1. 普通函數                       int f(int);
//     2. 函數指標                       int (*p)(int) = f;
//     3. 函數物件（本檔主題）           struct F { int operator()(int); };
//     4. lambda（其實就是 3 的語法糖）  auto g = [](int x){ return x; };
//     5. 成員函式指標 / std::bind 結果 / std::function
//     std::invoke（C++17）可以用統一語法呼叫這五種。
//
// (B) 空的函數物件不佔空間：EBO（Empty Base Optimization）
//     Adder 沒有任何成員變數，但 C++ 規定「完整物件的大小不得為 0」，
//     所以 sizeof(Adder) 是 1（本機實測，實作定義但幾乎所有平台都是 1）。
//     不過當它被當成**基底類別**時，空基底可以不佔空間——
//     這就是 EBO。std::vector 把 allocator 當基底存正是靠這招，
//     所以 sizeof(std::vector<int>) 是 24（三個指標）而不是 32。
//     本檔 main 有實測。
//
// (C) 每個 lambda 都是一個獨一無二的型別
//     auto a = [](int x){ return x; };
//     auto b = [](int x){ return x; };      // 寫得一模一樣
//     decltype(a) 與 decltype(b) 是**不同型別**。
//     這正說明 lambda 就是「編譯器幫你生成的匿名 functor 類別」，
//     兩次書寫產生兩個不同的類別。第 12 檔會詳細對照。
//
// 【注意事項 Pay Attention】
//   1. operator() 必須是**非靜態成員函式**，不能寫成 static，也不能是全域函式。
//   2. 呼叫時的物件本身要存在：Adder{}(3,5) 建立臨時物件也可以，
//      但別回傳指向區域 functor 的參考。
//   3. 述詞（predicate）建議標 const 且**不要有副作用**——
//      STL 演算法可以任意複製述詞，也不保證呼叫次數與順序。
//   4. sizeof(空 functor) 是 1 不是 0（實作定義，本機 g++ 15.2 實測為 1）。
//   5. 函數物件當模板參數傳遞才有內聯優勢；
//      包進 std::function 會退回間接呼叫（見第 15 檔）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】函數物件（Functor）
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 什麼是函數物件？它跟普通函數最大的差別在哪？
//     答：函數物件是「重載了 operator() 的類別」的實例，可以用 f(args) 的
//         語法呼叫。最大的差別是**它是物件，所以能攜帶狀態**——
//         成員變數就是狀態，每個實例可以記住不同的設定值。
//         普通函數只有程式碼沒有資料，要記住東西只能靠全域變數
//         （不可重入、不執行緒安全）或加參數（污染介面）。
//     追問：那 lambda 呢？
//         → lambda 就是編譯器自動生成的函數物件類別。
//           捕獲的變數會變成它的成員變數，operator() 就是 lambda 的本體。
//
// 🔥 Q2. 為什麼 std::sort 通常比 C 的 qsort 快？兩者都是 O(n log n)。
//     答：關鍵在**能不能內聯比較動作**。qsort 收的是函數指標，
//         那是執行期的一個位址，編譯器無從得知內容，
//         每次比較都是一次真正的間接呼叫，無法內聯。
//         std::sort 收的是**型別**（每個 functor / lambda 都是獨立型別），
//         模板實例化時比較邏輯直接可見，可以完全展開成幾行指令。
//         n log n 次比較，每次省下一次呼叫，差距就出來了。
//     追問：那把 lambda 包進 std::function 再傳給 sort 呢？
//         → 會退回和函數指標差不多的間接呼叫（type erasure 需要虛擬分派），
//           內聯優勢消失。所以熱路徑上別把 lambda 包進 std::function。
//
// ⚠️ 陷阱. 「函數物件沒有成員變數的話，sizeof 應該是 0 吧？反正它不存資料。」
//     答：不是 0，是 1（本機 g++ 15.2 實測；標準只要求「不得為 0」，
//         實際數值是實作定義的）。C++ 規定任何完整物件都必須有
//         獨一無二的位址，大小為 0 會讓陣列中兩個相鄰元素位址相同。
//         **但**當空類別被當成**基底類別**時，可以不佔空間——
//         這叫 Empty Base Optimization（EBO）。
//         std::vector 就是把 allocator 當基底而非成員，
//         所以 sizeof(vector<int>) 是 24（三個指標）而不是 32。
//     為什麼會錯：把「沒有資料」直接等同於「不佔空間」。
//         真正的規則是「完整物件不得為 0，子物件（基底）可以」，
//         C++20 的 [[no_unique_address]] 就是把 EBO 的能力
//         從基底類別擴展到成員變數上。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <vector>

// 這是一個函數物件類別
class Adder {
public:
    int operator()(int a, int b) const {
        return a + b;
    }
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】—— 本檔刻意不放
//   理由：本檔只講「operator() 讓物件能被呼叫」這一個語法點，
//   還沒進到「帶狀態的述詞」或「自訂比較器」——那些才是 LeetCode
//   真正用得上函數物件的地方（例如 179. Largest Number 的自訂比較器，
//   見本課第 2 檔）。在這裡硬掛一題，讀者只會看到一個
//   「其實用普通函數也能寫」的解法，反而模糊了本檔的重點。
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 【日常實務範例】可設定的單位換算器
//   情境：系統要顯示檔案大小、網路流量、記憶體用量，三者的換算基準不同
//         （1024 vs 1000），而且要顯示的小數位數也不一樣。
//   為什麼用到本主題：換算「規則」本身就是設定值。
//     用普通函數的話，每次呼叫都得把 base 和 precision 一起傳進去，
//     或是為每種單位各寫一個函數。
//     做成函數物件之後，設定在建構時決定一次，
//     之後就是一個乾淨的「一個參數進、一個字串出」的可呼叫物件。
// -----------------------------------------------------------------------------
class ByteFormatter {
public:
    // base=1024 → KiB/MiB（作業系統慣例）；base=1000 → KB/MB（硬碟廠商慣例）
    ByteFormatter(double base, int precision)
        : base_(base), precision_(precision) {}

    std::string operator()(double bytes) const {
        static const char* units[] = {"B", "K", "M", "G", "T"};
        int idx = 0;
        while (bytes >= base_ && idx < 4) {
            bytes /= base_;
            ++idx;
        }
        // 手動格式化到指定小數位，避免動用 <iomanip> 影響全域 cout 狀態
        double scale = 1.0;
        for (int i = 0; i < precision_; ++i) scale *= 10.0;
        double rounded = static_cast<double>(static_cast<long long>(bytes * scale + 0.5)) / scale;

        std::string s = std::to_string(rounded);
        // to_string 固定 6 位小數，截到需要的位數
        std::size_t dot = s.find('.');
        if (dot != std::string::npos) {
            s = (precision_ == 0) ? s.substr(0, dot)
                                  : s.substr(0, dot + 1 + static_cast<std::size_t>(precision_));
        }
        return s + units[idx];
    }

private:
    double base_;
    int    precision_;
};

int main() {
    Adder add;  // 建立一個物件

    // 像呼叫函數一樣使用物件
    int result = add(3, 5);  // 實際上是 add.operator()(3, 5)

    std::cout << "3 + 5 = " << result << std::endl;

    // -------------------------------------------------------------------------
    std::cout << "\n=== add(3,5) 其實就是 add.operator()(3,5) ===" << std::endl;
    std::cout << "add(3, 5)              = " << add(3, 5) << std::endl;
    std::cout << "add.operator()(3, 5)   = " << add.operator()(3, 5) << std::endl;
    std::cout << "Adder{}(3, 5)（臨時物件）= " << Adder{}(3, 5) << std::endl;

    // -------------------------------------------------------------------------
    std::cout << "\n=== 空的函數物件佔多少空間？ ===" << std::endl;
    {
        std::cout << "sizeof(Adder) = " << sizeof(Adder)
                  << "（本機 g++ 15.2 實測；標準只要求不得為 0，實際值實作定義）"
                  << std::endl;
        Adder a1, a2;
        std::cout << "兩個實例的位址不同嗎? " << std::boolalpha
                  << (&a1 != &a2) << "（這就是不能為 0 的理由）" << std::endl;
        std::cout << "sizeof(std::vector<int>) = " << sizeof(std::vector<int>)
                  << "（三個指標；allocator 當基底存，靠 EBO 不佔空間）"
                  << std::endl;
    }

    // -------------------------------------------------------------------------
    std::cout << "\n=== 五種可呼叫物件，語法完全一樣 ===" << std::endl;
    {
        struct Mul { int operator()(int a, int b) const { return a * b; } };

        auto lam = [](int a, int b) { return a - b; };   // lambda = 匿名 functor

        std::cout << "函數物件 Adder : " << add(10, 3) << std::endl;
        std::cout << "函數物件 Mul   : " << Mul{}(10, 3) << std::endl;
        std::cout << "lambda         : " << lam(10, 3) << std::endl;
        std::cout << "→ 呼叫語法一致，這就是「可呼叫物件」這個抽象的價值"
                  << std::endl;
    }

    // -------------------------------------------------------------------------
    std::cout << "\n=== 日常實務：可設定的單位換算器 ===" << std::endl;
    {
        ByteFormatter osStyle(1024.0, 1);    // 作業系統：1 KiB = 1024 B
        ByteFormatter diskStyle(1000.0, 2);  // 硬碟廠商：1 KB  = 1000 B

        const double sizes[] = {512, 2048, 1048576, 3221225472.0};

        std::cout << "同一筆數值，兩種換算基準：" << std::endl;
        for (double b : sizes) {
            std::cout << "  " << static_cast<long long>(b) << " bytes"
                      << "  ->  1024 基準: " << osStyle(b)
                      << "   |   1000 基準: " << diskStyle(b) << std::endl;
        }
        std::cout << "→ 換算規則存在物件裡，呼叫端只要傳一個數字。" << std::endl;
        std::cout << "  用普通函數的話，每次都得把 base 和 precision 一起傳。"
                  << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第八課：函數物件（Function Object）初探1.cpp -o demo1

// === 預期輸出 ===
// 3 + 5 = 8
//
// === add(3,5) 其實就是 add.operator()(3,5) ===
// add(3, 5)              = 8
// add.operator()(3, 5)   = 8
// Adder{}(3, 5)（臨時物件）= 8
//
// === 空的函數物件佔多少空間？ ===
// sizeof(Adder) = 1（本機 g++ 15.2 實測；標準只要求不得為 0，實際值實作定義）
// 兩個實例的位址不同嗎? true（這就是不能為 0 的理由）
// sizeof(std::vector<int>) = 24（三個指標；allocator 當基底存，靠 EBO 不佔空間）
//
// === 五種可呼叫物件，語法完全一樣 ===
// 函數物件 Adder : 13
// 函數物件 Mul   : 30
// lambda         : 7
// → 呼叫語法一致，這就是「可呼叫物件」這個抽象的價值
//
// === 日常實務：可設定的單位換算器 ===
// 同一筆數值，兩種換算基準：
//   512 bytes  ->  1024 基準: 512.0B   |   1000 基準: 512.00B
//   2048 bytes  ->  1024 基準: 2.0K   |   1000 基準: 2.05K
//   1048576 bytes  ->  1024 基準: 1.0M   |   1000 基準: 1.05M
//   3221225472 bytes  ->  1024 基準: 3.0G   |   1000 基準: 3.22G
// → 換算規則存在物件裡，呼叫端只要傳一個數字。
//   用普通函數的話，每次都得把 base 和 precision 一起傳。
