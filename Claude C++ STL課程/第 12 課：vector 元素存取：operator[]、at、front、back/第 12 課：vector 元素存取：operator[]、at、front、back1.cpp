// =============================================================================
//  第 12 課 (1)  —  vector::operator[]：最快的元素存取，但不做任何邊界檢查
// =============================================================================
//
// 【主題資訊 Information】
//   reference       operator[](size_type pos);        // 非 const 版本
//   const_reference operator[](size_type pos) const;  // const 版本
//
//   標頭檔：<vector>
//   標準版本：C++98 起即有；C++20 起加上 constexpr
//   複雜度：O(1)（常數時間，與 vector 大小無關）
//   前置條件：pos < size()。違反即為未定義行為（Undefined Behavior, UB）
//   回傳：元素的「參考（reference）」，不是複本
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼 operator[] 不做邊界檢查？】
// 這是 C++ 的核心設計哲學：「You don't pay for what you don't use」
// （不使用的功能不必付出代價）。vector 的定位是「能取代原生陣列的容器」，
// 若 v[i] 比 arr[i] 慢，效能敏感的程式（遊戲引擎、數值計算、影像處理）
// 就會退回使用原生陣列，vector 也就失去存在意義。
//
// 因此標準把「保證 pos 合法」的責任交給呼叫端：
//   * 你已經確定索引合法（例如 for (i = 0; i < v.size(); ++i)）→ 用 operator[]
//   * 你不確定（索引來自使用者輸入、檔案、網路）→ 用 at()，見本課第 3 檔
// 需要檢查的人用 at() 付出代價，不需要的人一毛都不必付。
//
// 【2. 為什麼回傳 reference 而不是 value？】
// 回傳 T& 是「v[1] = 200 能成立」的唯一原因。若回傳 T（複本），
// v[1] = 200 就是在對一個暫時物件賦值，改完立刻丟掉，毫無意義
// （實際上編譯器會直接報錯：無法對 rvalue 賦值）。
//
// 因為回傳的是 reference，下面兩件事同時成立：
//   int x = v[0];    // 讀：從參考複製出值
//   v[0] = 99;       // 寫：透過參考寫回容器內部
// 同一個語法同時支援讀與寫，這正是「左值（lvalue）」的威力。
//
// 【3. v[i] 實際上被編譯成什麼？】
// libstdc++ 的實作核心只有一行位址運算：
//   return *(this->_M_impl._M_start + __n);
// 即「起始位址 + n * sizeof(T)」再解參考。因為 vector 保證元素在記憶體中
// 連續（contiguous），這個位址運算是單一指令等級的成本。在最佳化開啟時，
// v[i] 產生的機器碼與原生陣列 arr[i] 完全相同 —— 沒有函式呼叫、沒有比較、
// 沒有分支。這就是「zero-overhead abstraction」。
//
// 【概念補充 Concept Deep Dive】
// vector 物件本身通常只有 3 個指標（在 64-bit 上 sizeof(vector<int>) == 24，
// 屬於實作定義，不同標準函式庫可能不同）：
//   _M_start（資料起點）、_M_finish（最後一個元素之後）、_M_end_of_storage（容量終點）
//
//   vector 物件（在 stack 上）           heap 上的真正資料
//   ┌──────────────────┐
//   │ _M_start         │────────────▶  ┌────┬────┬────┬────┬────┐
//   ├──────────────────┤               │ 10 │ 20 │ 30 │ 40 │ 50 │
//   │ _M_finish        │─────────┐     └────┴────┴────┴────┴────┘
//   ├──────────────────┤         │       ▲                        ▲
//   │ _M_end_of_storage│───┐     └───────┼────────────────────────┘
//   └──────────────────┘   │             v[0] = *(_M_start + 0)
//                          └──▶ 容量終點  v[2] = *(_M_start + 2)
//
// size() 其實就是 _M_finish - _M_start。所以 operator[] 若要做檢查，
// 必須多讀一個指標、做一次減法與一次比較 —— 這正是它選擇不做的成本。
//
// 【注意事項 Pay Attention】
// 1. pos >= size() 是「未定義行為」，不是「一定當機」也不是「一定印出垃圾值」。
//    UB 的意思是標準完全不規範會發生什麼：可能看似正常、可能印出任意值、
//    可能當機、可能默默破壞其他資料。詳見本課第 2 檔的實測。
// 2. v[i] 回傳的 reference 會在 vector 擴容（push_back / insert / resize / reserve）
//    後失效，繼續使用就是 UB。詳見本課第 11 檔。
// 3. 索引型別是 size_type（無號）。傳入負數 int 會被隱式轉成極大的無號數，
//    直接越界。詳見本課第 10 檔。
// 4. v.size() 回傳無號型別，寫 for (int i = 0; i < v.size(); ++i) 會觸發
//    -Wsign-compare 警告；正確寫法用 std::size_t 或 vector 的 size_type。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】vector::operator[]
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. vector 的 operator[] 為什麼不做邊界檢查？既然不安全，為什麼還要保留？
//     答：因為 C++ 的「零成本抽象」原則 —— vector 必須能無條件取代原生陣列。
//         多一次 pos < size() 的比較會在熱迴圈中阻礙向量化與指令排程。
//         標準的做法是「分成兩個函式」：operator[] 給已確定索引合法的情況，
//         at() 給需要檢查的情況，讓呼叫端自己選擇要不要付這個代價。
//     追問：那你在專案裡怎麼選？→ 索引由我自己的迴圈產生（i < v.size()）用 [];
//           索引來自外部（使用者輸入、設定檔、網路封包）一律用 at() 或先驗證。
//
// 🔥 Q2. v[1] = 200 為什麼能成立？如果 operator[] 回傳 int 而不是 int& 會怎樣？
//     答：因為它回傳 reference（int&），是個左值，可以被賦值。
//         若回傳 int（複本），v[1] = 200 就是對一個即將消滅的暫時物件賦值，
//         編譯器會直接報錯（不能對 rvalue 賦值），而且就算能編過也改不到容器。
//     追問：const vector 呼叫 v[1] 回傳什麼？→ const_reference（const int&），
//           只能讀不能寫，這就是 const 版本重載存在的理由。
//
// ⚠️ 陷阱. 「v[10] 越界的話，執行時一定會噴錯誤讓我知道」—— 哪裡錯了？
//     答：錯在把「未定義行為」想成「一定會被偵測」。標準對越界完全不規範。
//         本機實測（g++ 15.2.0）：同一份原始碼，-O0 會被 libstdc++ 的
//         hardening assertion 攔下而 abort，但 -O1 以上這個檢查就被關掉，
//         程式讀了不屬於它的記憶體、印出一個任意值、然後「正常」結束。
//     為什麼會錯：多數人把 C++ 想成 Python/Java（越界必丟 IndexError /
//         ArrayIndexOutOfBoundsException）。C++ 沒有這個保證 —— 越界最可怕的
//         結果不是當機，而是「看起來正常」，直到上線後在別的機器上炸掉。
// ═══════════════════════════════════════════════════════════════════════════

#include <vector>
#include <iostream>
#include <cstddef>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 1】LeetCode 1929. Concatenation of Array
//   題目：給定長度 n 的陣列 nums，回傳長度 2n 的陣列 ans，
//         其中 ans[i] == nums[i] 且 ans[i + n] == nums[i]。
//   為什麼用到本主題：這題本質上就是「用索引寫入預先配好空間的 vector」。
//         先配足 2n 個元素（保證所有索引合法），之後全部用 operator[] 直接寫，
//         不需要任何邊界檢查 —— 這正是 operator[] 的標準使用情境。
//   複雜度：時間 O(n)，空間 O(n)（回傳值本身）
// -----------------------------------------------------------------------------
std::vector<int> getConcatenation(const std::vector<int>& nums) {
    const std::size_t n = nums.size();
    std::vector<int> ans(2 * n);          // 先配足空間，之後索引必定合法
    for (std::size_t i = 0; i < n; ++i) {
        ans[i]     = nums[i];             // 前半
        ans[i + n] = nums[i];             // 後半
    }
    return ans;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 2】LeetCode 1470. Shuffle the Array
//   題目：陣列 nums 長度為 2n，內容是 [x1,x2,...,xn, y1,y2,...,yn]，
//         請重排成 [x1,y1, x2,y2, ..., xn,yn]。
//   為什麼用到本主題：交錯輸出需要「同時用兩個不同的索引讀取」
//         （i 讀前半、i + n 讀後半），是 operator[] 隨機存取的典型應用。
//         這種存取模式用迭代器反而寫得更囉唆 —— 索引才是最自然的表達。
//   複雜度：時間 O(n)，空間 O(n)
// -----------------------------------------------------------------------------
std::vector<int> shuffleArray(const std::vector<int>& nums, std::size_t n) {
    std::vector<int> ans(2 * n);
    for (std::size_t i = 0; i < n; ++i) {
        ans[2 * i]     = nums[i];         // x_i
        ans[2 * i + 1] = nums[i + n];     // y_i
    }
    return ans;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】以「月份 → 天數」查表計算某日是當年的第幾天
//   情境：報表系統要把「2024-03-15」換算成 day of year，
//         這是計算年度累積量、KPI 進度、利息天數時每天都在做的事。
//   為什麼用 operator[]：查表的索引由我們自己控制（迴圈上界是 month，
//         而 month 在呼叫前已驗證於 1..12），保證不會越界，
//         因此不需要 at() 的檢查成本。
//   技巧：索引 0 補一個 dummy，讓 1 月對應到索引 1，避免到處寫 month - 1
//         造成 off-by-one —— 這是查表法很常見的實務寫法。
// -----------------------------------------------------------------------------
int dayOfYear(int year, int month, int day) {
    //                            idx: 0   1   2   3   4   5   6   7   8   9  10  11  12
    const std::vector<int> daysInMonth = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    const bool isLeap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);

    int total = day;
    for (int m = 1; m < month; ++m) {
        total += daysInMonth[m];          // m 最大到 month-1 (≤ 11)，安全
    }
    if (isLeap && month > 2) {
        ++total;                          // 過了 2 月才要補上閏日
    }
    return total;
}

int main() {
    std::cout << "=== operator[] 讀取與修改 ===\n";
    std::vector<int> v = {10, 20, 30, 40, 50};

    std::cout << "v[0] = " << v[0] << "\n";     // 10
    std::cout << "v[2] = " << v[2] << "\n";     // 30
    std::cout << "v[4] = " << v[4] << "\n";     // 50

    v[1] = 200;                                  // 因為回傳 reference，可以寫入
    std::cout << "修改後 v[1] = " << v[1] << "\n";

    std::cout << "\n=== operator[] 回傳的是參考，不是複本 ===\n";
    int& ref = v[2];                             // 綁定到容器內部的元素
    ref = 333;                                   // 透過參考改動，容器真的被改了
    std::cout << "透過 ref 改動後 v[2] = " << v[2] << "\n";
    std::cout << "&ref 與 &v[2] 是否同一位址: "
              << (&ref == &v[2] ? "是（同一個物件）" : "否") << "\n";

    std::cout << "\n=== LeetCode 1929. Concatenation of Array ===\n";
    for (int x : getConcatenation({1, 2, 1})) std::cout << x << " ";
    std::cout << "\n";
    for (int x : getConcatenation({1, 3, 2, 1})) std::cout << x << " ";
    std::cout << "\n";

    std::cout << "\n=== LeetCode 1470. Shuffle the Array ===\n";
    for (int x : shuffleArray({2, 5, 1, 3, 4, 7}, 3)) std::cout << x << " ";
    std::cout << "\n";
    for (int x : shuffleArray({1, 2, 3, 4, 4, 3, 2, 1}, 4)) std::cout << x << " ";
    std::cout << "\n";

    std::cout << "\n=== 日常實務：day of year 查表 ===\n";
    std::cout << "2024-03-15 (閏年) = 第 " << dayOfYear(2024, 3, 15) << " 天\n";
    std::cout << "2023-03-15 (平年) = 第 " << dayOfYear(2023, 3, 15) << " 天\n";
    std::cout << "2024-12-31 (閏年) = 第 " << dayOfYear(2024, 12, 31) << " 天\n";
    std::cout << "2023-12-31 (平年) = 第 " << dayOfYear(2023, 12, 31) << " 天\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 12 課：vector 元素存取：operator[]、at、front、back1.cpp" -o access1

// === 預期輸出 ===
// === operator[] 讀取與修改 ===
// v[0] = 10
// v[2] = 30
// v[4] = 50
// 修改後 v[1] = 200
//
// === operator[] 回傳的是參考，不是複本 ===
// 透過 ref 改動後 v[2] = 333
// &ref 與 &v[2] 是否同一位址: 是（同一個物件）
//
// === LeetCode 1929. Concatenation of Array ===
// 1 2 1 1 2 1 
// 1 3 2 1 1 3 2 1 
//
// === LeetCode 1470. Shuffle the Array ===
// 2 3 5 4 1 7 
// 1 4 2 3 3 2 4 1 
//
// === 日常實務：day of year 查表 ===
// 2024-03-15 (閏年) = 第 75 天
// 2023-03-15 (平年) = 第 74 天
// 2024-12-31 (閏年) = 第 366 天
// 2023-12-31 (平年) = 第 365 天
