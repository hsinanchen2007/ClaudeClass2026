// =============================================================================
//  第 12 課 (4)  —  front() 與 back()：存取首尾元素的語意化寫法
// =============================================================================
//
// 【主題資訊 Information】
//   reference       front();        const_reference front() const;
//   reference       back();         const_reference back()  const;
//
//   標頭檔：<vector>
//   標準版本：C++98 起即有；C++20 起加上 constexpr
//   複雜度：O(1)
//   等價定義：front() 等價於 *begin()，也就是 v[0]
//             back()  等價於 *(end() - 1)，也就是 v[v.size() - 1]
//   前置條件：**容器不可為空**。對空容器呼叫是未定義行為（不是擲例外！）
//   回傳：元素的參考，因此可讀可寫
//
// 【詳細解釋 Explanation】
//
// 【1. 既然等價於 v[0]，為什麼還要 front()？】
// 三個理由，一個比一個實際：
//
//   (a) 可讀性：front() 直接說出「我要的是第一個」這個意圖。
//       v[0] 則讓讀者必須停下來想「0 是常數還是某個特別的位置？」
//
//   (b) back() 省掉一個容易出錯的算式：
//       v[v.size() - 1] 這個寫法有兩個陷阱 —— 要多打一次 v、
//       而且當 v 為空時 v.size() - 1 會是無號數溢位後的天文數字
//       （見本課第 10 檔）。back() 沒有這個算式，也就沒有這個錯的機會。
//
//   (c) 泛型程式碼的通用性：這才是最重要的。
//       std::list、std::deque、std::forward_list 都有 front()，
//       但 **std::list 沒有 operator[]**（它不支援隨機存取）。
//       用 front()/back() 寫的演算法可以直接套用在多種容器上；
//       用 v[0] 寫的就只能給 vector / deque / array 用。
//
// 【2. 回傳參考 → 首尾也可以被修改】
// front() 與 back() 回傳的是 reference，所以下面兩種用法都成立：
//       int x = v.front();      // 讀
//       v.front() = 100;        // 寫（改到容器內部真正的元素）
// 「函式呼叫可以出現在等號左邊」對初學者往往很反直覺，
// 但只要記得「它回傳的是參考、參考就是那個元素的別名」就通了。
//
// 【3. 最重要的一點：空容器上呼叫是 UB，不是例外】
// 這是本課最高頻的面試陷阱，值得單獨記住：
//       std::vector<int> v;
//       v.front();          // ← 未定義行為，不會擲出任何例外！
//       v.at(0);            // ← 這個才會擲出 std::out_of_range
// front()/back() **沒有** 對應的「安全版本」。標準沒有提供 at_front()。
// 唯一的防護就是呼叫前自己確認 !v.empty()。
//
// 本機實測（g++ 15.2.0，對空 vector 呼叫 front()）：
//       -O0 建置：libstdc++ hardening assertion 攔下 → abort（結束碼 134）
//       -O2 建置：沒有檢查 → 直接 SIGSEGV（結束碼 139），且緩衝中的輸出遺失
// 兩種結果都是 UB 的合法表現，而且都不是「擲出例外」。
//
// 【概念補充 Concept Deep Dive】
// back() 為什麼是 *(end() - 1) 而不是 *end()？
// 因為 STL 的區間慣例是**左閉右開** [begin, end)：
//
//     ┌────┬────┬────┬────┬────┐
//     │ 10 │ 20 │ 30 │ 40 │ 50 │  ✗
//     └────┴────┴────┴────┴────┘
//       ▲                        ▲
//     begin()                  end()  ← 指向「最後一個之後」，不可解參考
//       ▲                   ▲
//    front()              back()
//
// end() 指的是「尾後位置（past-the-end）」，它是個合法的迭代器值
// （可以拿來比較、可以做 end() - 1），但**不可解參考**。
// 這個設計讓 end() - begin() 直接等於元素個數，也讓空區間自然表示為 begin() == end()。
//
// 順帶一提，這也解釋了為什麼空容器的 front() 是 UB：
// 空容器的 begin() == end()，front() 等價於 *begin() 就等於 *end()，
// 而解參考 end() 本身就是未定義行為。
//
// 【注意事項 Pay Attention】
// 1. **front() / back() 對空容器是 UB，不是例外**。呼叫前務必 !v.empty()。
// 2. back() 回傳的參考在 push_back 之後可能失效（若發生擴容）。
//    常見錯誤：int& last = v.back(); v.push_back(1); last = 5;  // UB
// 3. pop_back() 不回傳被移除的元素。要取值必須「先 back() 讀取、再 pop_back()」。
//    而且要注意：先取參考再 pop_back，那個參考就失效了 ——
//    要複製一份值出來（int x = v.back(); v.pop_back();），不能存參考。
// 4. std::list 有 front()/back() 但沒有 operator[]；
//    寫泛型演算法時優先用 front()/back() 能涵蓋更多容器。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】front() 與 back()
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. front() 和 v[0] 完全等價，為什麼還要有 front()？
//     答：語意更清楚；back() 更能省掉 v[v.size()-1] 這個易錯算式；
//         最重要的是泛型 —— std::list 有 front()/back() 卻沒有 operator[]，
//         用 front()/back() 寫的程式碼能套用在更多容器上。
//     追問：那 back() 和 *(v.end() - 1) 呢？→ 也等價，但 end()-1 只在
//         random-access iterator 上能這樣寫，list 的雙向迭代器要用 --it，
//         back() 一樣是最通用的寫法。
//
// 🔥 Q2. v.front() = 100 為什麼可以放在等號左邊？
//     答：因為 front() 回傳 reference（T&），是左值，等於那個元素的別名，
//         賦值就直接改到容器內部。若容器是 const，回傳 const_reference，
//         這行就會編譯失敗。
//     追問：那 pop_back() 為什麼不回傳被刪掉的元素？→ 為了例外安全：
//         回傳值需要複製，若複製建構子擲出例外，元素已被移除卻沒交到呼叫端手上，
//         資料就遺失了。所以 STL 拆成「back() 讀 + pop_back() 移除」兩步。
//
// ⚠️ 陷阱. 對空 vector 呼叫 front()，會擲出 std::out_of_range 嗎？
//     答：**不會**。front()/back() 對空容器是未定義行為，不做任何檢查、
//         也不擲出任何例外。只有 at() 才保證擲例外。
//         本機實測：-O0 被 hardening assertion 攔下 abort（134）；
//         -O2 沒有檢查，直接 SIGSEGV（139）。兩者都不是例外。
//     為什麼會錯：看到 at() 會擲例外，就推論「STL 的存取函式越界都會擲例外」。
//         實際上 at() 是整個 vector 介面裡**唯一**有此保證的函式，是特例不是通則。
// ═══════════════════════════════════════════════════════════════════════════

#include <vector>
#include <iostream>
#include <string>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 66. Plus One
//   題目：用陣列表示一個非負整數（digits[0] 是最高位），對它加一，回傳結果陣列。
//   為什麼用到本主題：進位是從**最低位**（也就是 back()）開始往前處理的，
//         而「全部進位完畢」的特例（999 + 1 = 1000）需要在**最前面**插入 1。
//         這題把 back()（尾端運算）與 front()（頭端語意）的角色分得很清楚。
//   複雜度：時間 O(n)；最壞情況（全 9）需要在頭部插入，該次插入是 O(n)。
// -----------------------------------------------------------------------------
std::vector<int> plusOne(std::vector<int> digits) {
    // 從最低位往前找第一個不是 9 的位數
    for (std::size_t i = digits.size(); i > 0; --i) {
        if (digits[i - 1] < 9) {
            ++digits[i - 1];
            return digits;              // 沒有連鎖進位，直接完成
        }
        digits[i - 1] = 0;              // 這一位是 9 → 變 0，繼續往前進位
    }
    // 走到這裡代表原本每一位都是 9（例如 999），需要多一位
    digits.insert(digits.begin(), 1);   // 999 -> 1000
    return digits;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】固定容量的環形緩衝區（ring buffer）：保留最近 N 筆量測值
//   情境：監控程式每秒收一筆 CPU 溫度，畫面上只顯示「最近 5 筆」，
//         並且要隨時知道「最舊的一筆」與「最新的一筆」來判斷趨勢是升是降。
//   為什麼用 front()/back()：front() 就是最舊的、back() 就是最新的，
//         語意與業務需求一字對應，比 v[0] / v[v.size()-1] 好讀太多。
//   注意：每個取值前都先檢查 empty()，因為 front()/back() 對空容器是 UB。
// -----------------------------------------------------------------------------
class TemperatureWindow {
public:
    explicit TemperatureWindow(std::size_t capacity) : cap_(capacity) {}

    void push(double celsius) {
        if (buf_.size() == cap_) {
            buf_.erase(buf_.begin());   // 滿了就丟掉最舊的（示範用；實務可用 deque）
        }
        buf_.push_back(celsius);
    }

    bool empty() const { return buf_.empty(); }

    double oldest() const { return buf_.front(); }   // 呼叫端須先確認非空
    double latest() const { return buf_.back(); }    // 呼叫端須先確認非空

    // 趨勢判斷：比較最新與最舊，這正是 front()/back() 最自然的用途
    std::string trend() const {
        if (buf_.size() < 2) return "資料不足";
        if (buf_.back() > buf_.front()) return "上升 ▲";
        if (buf_.back() < buf_.front()) return "下降 ▼";
        return "持平 ─";
    }

    void dump() const {
        for (double d : buf_) std::cout << d << " ";
    }

private:
    std::size_t         cap_;
    std::vector<double> buf_;
};

int main() {
    std::cout << "=== front() / back() 讀取 ===\n";
    std::vector<int> v = {10, 20, 30, 40, 50};
    std::cout << "front() = " << v.front() << "\n";
    std::cout << "back()  = " << v.back()  << "\n";

    std::cout << "\n=== 等價寫法對照 ===\n";
    std::cout << "v[0]            = " << v[0] << "\n";
    std::cout << "v[v.size() - 1] = " << v[v.size() - 1] << "\n";
    std::cout << "&v.front() == &v[0]              ? "
              << (&v.front() == &v[0] ? "是" : "否") << "\n";
    std::cout << "&v.back()  == &v[v.size() - 1]   ? "
              << (&v.back() == &v[v.size() - 1] ? "是" : "否") << "\n";

    std::cout << "\n=== 回傳參考，所以可以修改 ===\n";
    v.front() = 100;
    v.back()  = 500;
    for (int x : v) std::cout << x << " ";
    std::cout << "\n";

    std::cout << "\n=== 安全習慣：取值前先確認非空 ===\n";
    std::vector<int> empty_vec;
    std::cout << "empty_vec.empty() = " << std::boolalpha << empty_vec.empty() << "\n";
    if (!empty_vec.empty()) {
        std::cout << "front = " << empty_vec.front() << "\n";
    } else {
        std::cout << "容器為空，略過 front()（直接呼叫會是未定義行為）\n";
    }

    std::cout << "\n=== 正確地取出並移除最後一個元素 ===\n";
    std::vector<int> stack = {1, 2, 3};
    int top = stack.back();      // 先「複製」出值，不能存參考
    stack.pop_back();            // 再移除；此時原本的參考已失效
    std::cout << "取出 " << top << "，剩下: ";
    for (int x : stack) std::cout << x << " ";
    std::cout << "\n";

    std::cout << "\n=== LeetCode 66. Plus One ===\n";
    for (int x : plusOne({1, 2, 3})) std::cout << x << " ";
    std::cout << "  <- 123 + 1\n";
    for (int x : plusOne({4, 3, 2, 1})) std::cout << x << " ";
    std::cout << "  <- 4321 + 1\n";
    for (int x : plusOne({9})) std::cout << x << " ";
    std::cout << "  <- 9 + 1（需要進位擴充）\n";
    for (int x : plusOne({9, 9, 9})) std::cout << x << " ";
    std::cout << "  <- 999 + 1（全進位）\n";

    std::cout << "\n=== 日常實務：溫度環形緩衝區 ===\n";
    TemperatureWindow win(5);
    const double samples[] = {61.5, 62.0, 63.2, 64.8, 66.1, 68.4, 70.2};
    for (double s : samples) {
        win.push(s);
        std::cout << "收到 " << s << "  視窗=[ ";
        win.dump();
        std::cout << "]  最舊=" << win.oldest()
                  << " 最新=" << win.latest()
                  << " 趨勢=" << win.trend() << "\n";
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 12 課：vector 元素存取：operator[]、at、front、back4.cpp" -o access4

// === 預期輸出 ===
// === front() / back() 讀取 ===
// front() = 10
// back()  = 50
//
// === 等價寫法對照 ===
// v[0]            = 10
// v[v.size() - 1] = 50
// &v.front() == &v[0]              ? 是
// &v.back()  == &v[v.size() - 1]   ? 是
//
// === 回傳參考，所以可以修改 ===
// 100 20 30 40 500 
//
// === 安全習慣：取值前先確認非空 ===
// empty_vec.empty() = true
// 容器為空，略過 front()（直接呼叫會是未定義行為）
//
// === 正確地取出並移除最後一個元素 ===
// 取出 3，剩下: 1 2 
//
// === LeetCode 66. Plus One ===
// 1 2 4   <- 123 + 1
// 4 3 2 2   <- 4321 + 1
// 1 0   <- 9 + 1（需要進位擴充）
// 1 0 0 0   <- 999 + 1（全進位）
//
// === 日常實務：溫度環形緩衝區 ===
// 收到 61.5  視窗=[ 61.5 ]  最舊=61.5 最新=61.5 趨勢=資料不足
// 收到 62  視窗=[ 61.5 62 ]  最舊=61.5 最新=62 趨勢=上升 ▲
// 收到 63.2  視窗=[ 61.5 62 63.2 ]  最舊=61.5 最新=63.2 趨勢=上升 ▲
// 收到 64.8  視窗=[ 61.5 62 63.2 64.8 ]  最舊=61.5 最新=64.8 趨勢=上升 ▲
// 收到 66.1  視窗=[ 61.5 62 63.2 64.8 66.1 ]  最舊=61.5 最新=66.1 趨勢=上升 ▲
// 收到 68.4  視窗=[ 62 63.2 64.8 66.1 68.4 ]  最舊=62 最新=68.4 趨勢=上升 ▲
// 收到 70.2  視窗=[ 63.2 64.8 66.1 68.4 70.2 ]  最舊=63.2 最新=70.2 趨勢=上升 ▲
