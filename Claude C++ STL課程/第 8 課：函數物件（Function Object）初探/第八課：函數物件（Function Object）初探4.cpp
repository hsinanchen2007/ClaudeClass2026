// =============================================================================
//  第八課：函數物件 4  —  帶狀態的函數物件：把「設定」存進物件裡
// =============================================================================
//
// 【主題資訊 Information】
//   模式：
//       class GreaterThan {
//           int threshold;                                  // 狀態
//       public:
//           GreaterThan(int t) : threshold(t) {}            // 建構時決定設定
//           bool operator()(int x) const { return x > threshold; }
//       };
//       std::count_if(v.begin(), v.end(), GreaterThan(5));  // 當場產生一個「函數」
//   標頭檔：<algorithm>（演算法用）；functor 本身不需要。
//   標準版本：C++98 起即有；C++11 之後同樣的事多半改用 lambda 捕獲（第 10、12 檔）。
//   複雜度：count_if 是 O(n)，述詞呼叫本身是 O(1)。
//
// 【詳細解釋 Explanation】
//
// 【1. 這一檔要解決的問題，就是上一檔的死路】
//   第 3 檔卡在：
//       bool greater_than_n(int x) { return x > ???; }   // N 從哪來？
//   普通函數沒有地方存 N。函數物件的答案很直接——**存成成員變數**。
//   一旦 N 變成物件的一部分，就可以：
//       GreaterThan gt5(5);      // 一個「大於 5」的函數
//       GreaterThan gt7(7);      // 另一個「大於 7」的函數
//   兩個是互相獨立的物件，各自記住自己的設定。
//   這就是「函數物件 = 帶狀態的函數」這句話的完整意思。
//
// 【2. 一個很重要的心智轉換：從「函數」變成「函數工廠」】
//   GreaterThan 這個**類別**不是一個函數，而是「一整族函數」的模板。
//   GreaterThan(5) 這個**運算式**才產生一個具體的函數。
//   換句話說，類別的建構子扮演了「函數工廠」的角色：
//       建構參數   → 決定這個函數的行為
//       operator() → 這個函數的本體
//   這個分離讓「設定」與「執行」發生在不同時間點：
//   設定只做一次，執行做 n 次。
//   對照普通函數，你只能每次呼叫都重傳一次設定。
//
// 【3. 為什麼可以直接寫 GreaterThan(5) 當引數】
//   std::count_if 的第三個參數是模板參數，接受**任何**可呼叫物件。
//   GreaterThan(5) 建立一個臨時物件，直接以值傳入。
//   注意這個臨時物件很小（本例只有一個 int），複製成本可以忽略——
//   這也是為什麼 STL 統一用「值傳遞」述詞的原因之一。
//   但值傳遞也帶來一個後果：**演算法內部改動的是副本**，
//   這正是下一檔（第 5 檔）要講的經典陷阱。
//
// 【4. 什麼時候該用 functor、什麼時候該用 lambda】
//   C++11 之後，本檔這種寫法的多數場合都可以用 lambda 一行取代：
//       std::count_if(v.begin(), v.end(), [](int x){ return x > 5; });
//   仍然值得寫成具名 functor 的情況：
//     * 同一個判斷邏輯要在**多處重用**（lambda 不好重用）
//     * 需要**命名**讓意圖清楚（IsExpired、IsValidEmail 比一坨 lambda 好讀）
//     * 需要**額外的成員函式**（例如取出累積的統計值）
//     * 需要作為**模板參數的型別**（例如 std::set<T, MyComparator>）
//       —— 這一點特別重要：set/map 的比較器是型別參數，
//          C++20 之前 lambda 沒有預設建構子，不能直接當型別參數用。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼 operator() 要標 const
//     STL 演算法可能以 const 的方式持有你的述詞。
//     若 operator() 沒標 const，某些用法會編譯失敗。
//     更深一層的理由是語意：述詞應該是**純函數**，
//     呼叫它不該改變任何東西。標 const 是把這個承諾寫進型別裡，
//     讓編譯器幫你檢查。
//     只有刻意要累積狀態時才拿掉 const（第 5 檔的 Counter），
//     而那正是會出意外的地方。
//
// (B) 函數物件的大小就是它的狀態大小
//     GreaterThan 只有一個 int，本機實測 sizeof 是 4。
//     這代表把它當引數傳遞幾乎沒有成本，
//     編譯器多半會把整個物件放進暫存器。
//     對照之下 std::function 的 sizeof 是 32（本機 libstdc++ 實測，
//     實作定義），而且可能牽涉 heap 配置——差別很大。
//
// (C) 這個模式在標準函式庫裡到處都是
//     std::set<int, std::greater<int>>          比較器是型別參數
//     std::unordered_map<K, V, MyHash>          hash 函式是型別參數
//     std::priority_queue<T, Container, Cmp>    比較器是型別參數
//     這些都要求「一個可預設建構的可呼叫型別」，
//     所以傳統上必須用具名 functor（C++20 起無捕獲 lambda 也可預設建構）。
//
// (D) C++20 之前，lambda 不能直接當型別參數用
//     std::set<int, decltype(myLambda)> s;      // C++20 之前需要額外傳實例
//     因為 C++20 之前 closure type 沒有預設建構子。
//     C++20 起，**無捕獲**的 lambda 有了預設建構子與賦值運算子，
//     這個限制才消失。有捕獲的仍然不行。
//
// 【注意事項 Pay Attention】
//   1. operator() 預設請標 const；只有刻意累積狀態才拿掉（見第 5 檔）。
//   2. 述詞以**值**傳給演算法，演算法可以自由複製它——
//      別依賴述詞內部累積的狀態（第 5 檔的陷阱）。
//   3. 狀態要小；大狀態請存指標/參考，但要確保生命週期比演算法呼叫長。
//   4. 拿 functor 當 set/map 的比較器時，它必須可預設建構
//      （或在建構容器時傳入實例）。
//   5. 比較器型別不同 → 容器型別就不同：
//      std::set<int> 與 std::set<int, std::greater<int>> 是兩個無關的型別。
//   6. 用浮點數當 set 的鍵時，比較器打平的情況要另外用唯一鍵補足，
//      否則會被視為重複元素而漏掉資料（本檔 ByValueDesc 用 id 補）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】帶狀態的函數物件
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 函數物件相對普通函數最大的優勢是什麼？舉一個普通函數辦不到的例子。
//     答：能攜帶狀態。例如「判斷是否大於 N」，N 是執行期才知道的設定：
//         普通函數沒有地方存 N，只能靠全域變數（不可重入、不執行緒安全、
//         兩處想用不同的 N 就打架），或多加一個參數（但 count_if
//         只會傳一個引數給述詞，簽章對不上）。
//         函數物件把 N 存成成員變數，GreaterThan(5) 與 GreaterThan(7)
//         是兩個互相獨立、各自記住設定的物件。
//     追問：那 C++11 之後還需要手寫 functor 嗎？
//         → 多數場合可以用 lambda 捕獲取代。但四種情況仍該具名：
//           邏輯要多處重用、需要命名讓意圖清楚、需要額外成員函式、
//           以及要當**型別參數**（set/map/priority_queue 的比較器）。
//
// 🔥 Q2. std::set<int, std::greater<int>> 和 std::set<int> 是同一個型別嗎？
//     答：不是，是兩個完全無關的型別。比較器是 set 的**模板參數**，
//         型別不同 → 容器型別就不同，彼此不能互相賦值，
//         也不能傳給接受另一種的函式。
//         這也說明比較器是在**編譯期**綁定的，所以可以完全內聯，
//         set 的每次比較都沒有間接呼叫成本。
//     追問：那要在執行期才決定排序方向怎麼辦？
//         → 用 std::function<bool(int,int)> 當比較器型別（有執行期開銷），
//           或存兩種容器、或存資料後再依需求排序。
//           在編譯期綁定與執行期彈性之間，C++ 讓你自己選。
//
// ⚠️ 陷阱. 「我把 lambda 傳給 std::set 當比較器：
//         std::set<int, decltype(cmp)> s;  為什麼編譯失敗？」
//     答：在 C++17 及之前，lambda 的 closure type **沒有預設建構子**。
//         std::set 預設建構時會嘗試預設建構它的比較器，於是失敗。
//         解法是把實例一起傳進去：
//             auto cmp = [](int a, int b){ return a > b; };
//             std::set<int, decltype(cmp)> s(cmp);      // 傳入實例
//         C++20 起，**無捕獲**的 lambda 有了預設建構子，
//         寫 std::set<int, decltype(cmp)> s; 才開始合法（有捕獲的仍然不行）。
//     為什麼會錯：把「能當比較器用」等同於「能當比較器型別用」。
//         傳給 std::sort 的是**值**（一個物件），
//         傳給 std::set 的是**型別**（模板參數）——
//         後者要求該型別能被容器自己建構出來，這是完全不同的要求。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <set>
#include <string>
#include <algorithm>
#include <functional>
#include <iterator>
#include <type_traits>

// 普通函數：無法攜帶狀態
// 如果要判斷「大於 N」，N 從哪來？
// bool greater_than_n(int x) { return x > ???; }

// 函數物件：可以攜帶狀態！
class GreaterThan {
private:
    int threshold;

public:
    GreaterThan(int t) : threshold(t) {}

    bool operator()(int x) const {
        return x > threshold;
    }
};

class IsDivisibleBy {
private:
    int divisor;

public:
    IsDivisibleBy(int d) : divisor(d) {}

    bool operator()(int x) const {
        return x % divisor == 0;
    }
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】—— 本檔刻意不放
//   理由：本檔的主題是「把設定存進物件」這個**設計手法**，
//   而 LeetCode 的解題函式簽章由平台固定，門檻值直接就是函式參數，
//   在函式內寫 lambda 捕獲它是最自然的寫法
//   （第 3 檔的 27. Remove Element 已經示範過這個轉折）。
//   真正需要「具名帶狀態 functor」的是**跨函式重用**與
//   **當作型別參數**（std::set / priority_queue 的比較器）這兩件事，
//   兩者都不是單題 LeetCode 會出現的結構。
//   下面的實務範例改用 std::set 的自訂比較器來示範這個不可取代的用途。
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 【日常實務範例】用「可設定的規則物件」做感測器告警分級
//   情境：機房監控要對一批感測器讀數做多重規則判斷——
//         溫度超過警告門檻、超過嚴重門檻。
//         門檻值是從設定檔讀進來的，會隨環境調整，不能寫死。
//   為什麼用到本主題：門檻是執行期的設定值，普通函數存不了。
//     做成 functor 後，一個門檻對應一個物件，
//     可以放進容器、可以傳來傳去、可以重複套用在不同資料上。
//     這裡刻意用具名類別而非 lambda，因為這些規則要在多處重用，
//     而且有名字（OverThreshold）比一坨 lambda 好懂得多。
//   同時示範 functor 不可取代的第二個用途：當 std::set 的**型別參數**。
// -----------------------------------------------------------------------------
struct Reading {
    int         id;
    std::string sensor;
    double      value;
};

class OverThreshold {
public:
    explicit OverThreshold(double limit) : limit_(limit) {}
    bool operator()(const Reading& r) const { return r.value > limit_; }
private:
    double limit_;
};

// 具名 functor 的另一個不可取代用途：當作 std::set 的**型別參數**
struct ByValueDesc {
    bool operator()(const Reading& a, const Reading& b) const {
        if (a.value != b.value) return a.value > b.value;   // 讀數大的排前面
        return a.id < b.id;   // 打平時用 id 補足，確保嚴格弱序、也不會漏資料
    }
};

int main() {
    std::vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    // 使用帶狀態的函數物件
    int count_gt_5 = std::count_if(vec.begin(), vec.end(), GreaterThan(5));
    std::cout << "大於 5 的個數: " << count_gt_5 << std::endl;

    int count_gt_7 = std::count_if(vec.begin(), vec.end(), GreaterThan(7));
    std::cout << "大於 7 的個數: " << count_gt_7 << std::endl;

    int count_div_3 = std::count_if(vec.begin(), vec.end(), IsDivisibleBy(3));
    std::cout << "3 的倍數個數: " << count_div_3 << std::endl;

    int count_div_4 = std::count_if(vec.begin(), vec.end(), IsDivisibleBy(4));
    std::cout << "4 的倍數個數: " << count_div_4 << std::endl;

    // -------------------------------------------------------------------------
    std::cout << "\n=== 一個類別 = 一整族函數 ===" << std::endl;
    {
        GreaterThan gt3(3), gt5(5), gt8(8);
        std::cout << "GreaterThan 這個「類別」不是一個函數，是一族函數的工廠："
                  << std::endl;
        std::cout << "  gt3(4) = " << std::boolalpha << gt3(4)
                  << "   gt5(4) = " << gt5(4)
                  << "   gt8(4) = " << gt8(4) << std::endl;
        std::cout << "→ 三個獨立物件，各自記住自己的門檻，互不干擾。" << std::endl;
        std::cout << "sizeof(GreaterThan) = " << sizeof(GreaterThan)
                  << " bytes（就是它的狀態大小，傳遞幾乎零成本）" << std::endl;
        std::cout << "sizeof(std::function<bool(int)>) = "
                  << sizeof(std::function<bool(int)>)
                  << " bytes（本機 libstdc++ 實測，實作定義）" << std::endl;
    }

    // -------------------------------------------------------------------------
    std::cout << "\n=== 設定與執行分離：設定一次，執行 n 次 ===" << std::endl;
    {
        std::vector<int> data = {12, 45, 7, 23, 56, 89, 3, 34};
        GreaterThan over30(30);         // 設定只做一次

        std::cout << "資料: ";
        for (int n : data) std::cout << n << " ";
        std::cout << std::endl;

        std::cout << "大於 30 的有: ";
        std::copy_if(data.begin(), data.end(),
                     std::ostream_iterator<int>(std::cout, " "), over30);
        std::cout << std::endl;

        // 同一個物件可以重複套用在不同資料上
        std::vector<int> other = {31, 29, 100};
        std::cout << "同一個 over30 套在另一組資料 {31,29,100}: ";
        std::copy_if(other.begin(), other.end(),
                     std::ostream_iterator<int>(std::cout, " "), over30);
        std::cout << std::endl;
    }

    // -------------------------------------------------------------------------
    std::cout << "\n=== functor 不可取代的用途：當「型別參數」 ===" << std::endl;
    {
        // std::set 的比較器是**型別**參數，在編譯期綁定
        std::set<int>                     asc  = {5, 2, 8, 1};
        std::set<int, std::greater<int>>  desc = {5, 2, 8, 1};

        std::cout << "std::set<int>                    : ";
        for (int n : asc) std::cout << n << " ";
        std::cout << std::endl;
        std::cout << "std::set<int, std::greater<int>> : ";
        for (int n : desc) std::cout << n << " ";
        std::cout << std::endl;
        std::cout << "這兩個是同一個型別嗎? " << std::boolalpha
                  << std::is_same<decltype(asc), decltype(desc)>::value << std::endl;
        std::cout << "→ 比較器是模板參數，型別不同 → 容器型別就不同，"
                  << "彼此不能互相賦值。" << std::endl;
        std::cout << "  好處是編譯期綁定，每次比較都能內聯，沒有間接呼叫成本。"
                  << std::endl;
        std::cout << "  代價是排序方向不能在執行期改變。" << std::endl;
    }

    // -------------------------------------------------------------------------
    std::cout << "\n=== 日常實務：可設定門檻的感測器告警 ===" << std::endl;
    {
        const std::vector<Reading> readings = {
            {1, "temp-A",  68.5},
            {2, "temp-B",  81.2},
            {3, "temp-C",  74.0},
            {4, "temp-D",  92.7},
            {5, "temp-E",  55.1},
            {6, "temp-F",  88.3},
        };

        // 門檻從設定檔來，會隨機房環境調整 —— 不能寫死
        const double warnLimit = 75.0;
        const double critLimit = 85.0;

        OverThreshold isWarn(warnLimit);
        OverThreshold isCrit(critLimit);

        std::cout << "警告門檻 " << warnLimit << "、嚴重門檻 " << critLimit << std::endl;
        std::cout << "  超過警告門檻: "
                  << std::count_if(readings.begin(), readings.end(), isWarn)
                  << " 筆" << std::endl;
        std::cout << "  超過嚴重門檻: "
                  << std::count_if(readings.begin(), readings.end(), isCrit)
                  << " 筆" << std::endl;

        std::cout << "  嚴重清單: ";
        for (const auto& r : readings)
            if (isCrit(r)) std::cout << r.sensor << "(" << r.value << ") ";
        std::cout << std::endl;

        // 具名 functor 當型別參數：自動維持「讀數由大到小」的容器
        std::set<Reading, ByValueDesc> ranked(readings.begin(), readings.end());
        std::cout << "  依讀數排名（set 用 ByValueDesc 當型別參數，自動維持順序）:"
                  << std::endl;
        int rank = 1;
        for (const auto& r : ranked) {
            std::cout << "    " << rank++ << ". " << r.sensor
                      << "  " << r.value << std::endl;
        }
        std::cout << "→ 門檻是執行期設定 → 存進物件；" << std::endl;
        std::cout << "  排序規則是編譯期契約 → 當型別參數。兩種用法各司其職。"
                  << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第八課：函數物件（Function Object）初探4.cpp -o demo4

// === 預期輸出 ===
// 大於 5 的個數: 5
// 大於 7 的個數: 3
// 3 的倍數個數: 3
// 4 的倍數個數: 2
//
// === 一個類別 = 一整族函數 ===
// GreaterThan 這個「類別」不是一個函數，是一族函數的工廠：
//   gt3(4) = true   gt5(4) = false   gt8(4) = false
// → 三個獨立物件，各自記住自己的門檻，互不干擾。
// sizeof(GreaterThan) = 4 bytes（就是它的狀態大小，傳遞幾乎零成本）
// sizeof(std::function<bool(int)>) = 32 bytes（本機 libstdc++ 實測，實作定義）
//
// === 設定與執行分離：設定一次，執行 n 次 ===
// 資料: 12 45 7 23 56 89 3 34
// 大於 30 的有: 45 56 89 34
// 同一個 over30 套在另一組資料 {31,29,100}: 31 100
//
// === functor 不可取代的用途：當「型別參數」 ===
// std::set<int>                    : 1 2 5 8
// std::set<int, std::greater<int>> : 8 5 2 1
// 這兩個是同一個型別嗎? false
// → 比較器是模板參數，型別不同 → 容器型別就不同，彼此不能互相賦值。
//   好處是編譯期綁定，每次比較都能內聯，沒有間接呼叫成本。
//   代價是排序方向不能在執行期改變。
//
// === 日常實務：可設定門檻的感測器告警 ===
// 警告門檻 75、嚴重門檻 85
//   超過警告門檻: 3 筆
//   超過嚴重門檻: 2 筆
//   嚴重清單: temp-D(92.7) temp-F(88.3)
//   依讀數排名（set 用 ByValueDesc 當型別參數，自動維持順序）:
//     1. temp-D  92.7
//     2. temp-F  88.3
//     3. temp-B  81.2
//     4. temp-C  74
//     5. temp-A  68.5
//     6. temp-E  55.1
// → 門檻是執行期設定 → 存進物件；
//   排序規則是編譯期契約 → 當型別參數。兩種用法各司其職。
