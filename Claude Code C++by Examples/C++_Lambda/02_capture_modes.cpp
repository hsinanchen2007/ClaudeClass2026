// =============================================================================
//  02_capture_modes.cpp  —  Lambda 各種捕獲方式
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/language/lambda
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、捕獲列表 (capture list) 是什麼？                       │
//  └────────────────────────────────────────────────────────────┘
//
//  捕獲列表 = lambda 要從外面「帶進函式體」的變數清單。
//  編譯器會把這些變數當作 closure class 的成員變數，在建立 lambda 時拷貝
//  或綁定一份進去。
//
//  ┌──────────────┬──────────────────────────────────────────────┐
//  │   寫法        │   意義                                        │
//  ├──────────────┼──────────────────────────────────────────────┤
//  │ []            │ 不捕獲任何變數                                 │
//  │ [x]           │ 以「值」捕獲 x（拷貝一份）                      │
//  │ [&x]          │ 以「參考」捕獲 x（綁原本那個）                  │
//  │ [=]           │ 「值」捕獲所有用到的外部變數（預設 by value）   │
//  │ [&]           │ 「參考」捕獲所有用到的外部變數（預設 by ref）   │
//  │ [=, &x]       │ 預設 by value，但 x 用 ref                    │
//  │ [&, x]        │ 預設 by ref，但 x 用 value                    │
//  │ [this]        │ 捕獲外層物件的 this 指標                       │
//  │ [*this]       │ C++17：以「值」捕獲 *this（整個物件複製）       │
//  └──────────────┴──────────────────────────────────────────────┘
//
//  注意：
//   * 全域 / static 變數「不需要也不能」被捕獲 — 它們有固定位置，lambda 內
//     直接用名字就可以。
//   * `[=]` / `[&]` 「default capture」自 C++20 起對顯式捕獲外的變數要警告
//     某些情況（DR：明確捕獲 this 的需求）—— 實務上建議「明寫要哪幾個」，
//     避免不小心把整個世界拉進來。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、值 vs 參考的關鍵差異                                    │
//  └────────────────────────────────────────────────────────────┘
//
//  by value：拷貝一份「當下的值」 → 之後外部變數改變，lambda 看到的還是
//            建立時的快照。
//  by ref：  綁定外部變數本身 → 外部變更，lambda 內看到的也跟著變。
//
//  危險：如果 lambda 比被綁定的變數活得久（譬如塞進另一個 thread 或
//        container），那條 ref 就變成「懸掛參考」(dangling)。
//        詳細案例見 10_lambda_pitfalls.cpp。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、本檔範例與 LeetCode                                    │
//  └────────────────────────────────────────────────────────────┘
//
//  * Demo 1～5：示範各種捕獲寫法
//  * LeetCode 1431：Kids With the Greatest Number of Candies
// =============================================================================

/*
補充筆記：capture_modes
  - 值捕獲在 lambda 建立時複製，參考捕獲只保存外部物件別名。
  - lambda 若被延後執行或存起來，參考捕獲很容易 dangling。
  - [=] 與 [&] 方便但會隱藏捕獲內容；教材範例以明確列出捕獲變數最清楚。
  - capture_modes 要從 closure object 理解：lambda 不是神祕語法，而是編譯器產生的匿名函式物件。
  - 捕獲 by value 是在建立 lambda 時複製，by reference 是保存別名；延後執行時 reference 捕獲最容易 dangling。
  - mutable 只讓 by value 捕獲的副本可修改，不會修改外部原變數。
  - generic lambda 的 auto 參數本質上是 function call operator template，錯誤可能在呼叫時才出現。
  - std::function 可保存不同 callable，但可能有型別抹除成本和配置成本；效能敏感處可優先用 template 接 callable。
  - lambda 放進 algorithm 時應讓 predicate 無副作用或副作用明確，否則演算法意圖會變難讀。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】捕獲方式（by value / by reference）
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 值捕獲與參考捕獲的差別？各自的風險是什麼？
//     答：[x] 在 lambda「建立當下」把 x 複製成閉包的成員，之後外部 x 改變不影響閉包；
//     [&x] 的成員是參考，讀寫直達原變數、零複製，但原變數生命週期一結束閉包就懸垂。
//     判準：lambda 不逃逸（就地餵給演算法）→ [&] 安全；lambda 會被存起來、跨執行緒或
//     非同步執行 → 必須值捕獲，或用 shared_ptr 延長壽命。
//     追問：該用 [=] / [&] 嗎？（建議顯式列出每個捕獲，才會被迫逐一審視生命週期）
//
// 🔥 Q2. 這段輸出什麼？int x = 10; auto f = [x]{ return x; }; x = 20; std::cout << f();
//     答：輸出 10。值捕獲發生在 lambda 建立的那一刻，之後改 x 與閉包內的副本無關。
//     改成 [&x] 則輸出 20。
//     追問：改成 mutable 並在內部 ++x 會影響外部嗎？（不會，改的是閉包自己的副本）
//
// ⚠️ 陷阱. 成員函式裡寫 [=]{ return data; }，data 被複製進閉包了嗎？
//     答：沒有。[=] 在成員函式中是「以值複製 this 指標」，閉包內的 data 其實是
//     this->data。物件一銷毀，回傳的閉包就懸垂。解法：[*this]（C++17，複製整個物件）
//     或 [data = data]（init-capture，只複製該成員）。C++20 起 [=] 隱式捕獲 this 已被棄用。
//     為什麼會錯：腦中的模型是「= 代表全部按值、所以一定安全」，但 = 只作用在「用到的
//     自動變數」，成員不是自動變數，它是透過 this 被存取的。
// ═══════════════════════════════════════════════════════════════════════════

#include <algorithm>
#include <iostream>
#include <vector>

int main() {
    int x = 10, y = 20;

    // ─────────────────────────────────────────────────────────
    // Demo 1：[x] 值捕獲 — 改外部 x 不影響 lambda 看到的值
    // ─────────────────────────────────────────────────────────
    auto byValue = [x] { return x; };       // 拷貝 x = 10
    x = 999;
    std::cout << "[Demo1] x changed to 999, byValue() = " << byValue() << "\n";
    // 預期：byValue() 仍然是 10
    x = 10;

    // ─────────────────────────────────────────────────────────
    // Demo 2：[&x] 參考捕獲 — 跟外部 x 同步
    // ─────────────────────────────────────────────────────────
    auto byRef = [&x] { return x; };
    x = 777;
    std::cout << "[Demo2] x changed to 777, byRef() = " << byRef() << "\n";
    // 預期：777
    x = 10;

    // ─────────────────────────────────────────────────────────
    // Demo 3：[=] 全部用值捕獲
    //   注意：只會捕獲「在 body 裡用到的」變數，不是真的把所有區域變數塞進來
    // ─────────────────────────────────────────────────────────
    auto useAll = [=] { return x + y; };
    std::cout << "[Demo3] useAll() = " << useAll() << "\n";
    // 預期：10 + 20 = 30

    // ─────────────────────────────────────────────────────────
    // Demo 4：[&] 全部用參考捕獲
    //   常用於「把多個外部變數同步累積」的情境
    // ─────────────────────────────────────────────────────────
    int sum = 0;
    std::vector<int> nums{1, 2, 3, 4, 5};
    std::for_each(nums.begin(), nums.end(), [&](int n) { sum += n; });
    std::cout << "[Demo4] sum = " << sum << "\n"; // 預期 15

    // ─────────────────────────────────────────────────────────
    // Demo 5：混合 — [=, &sum] 預設值捕獲、但 sum 要 by ref
    // ─────────────────────────────────────────────────────────
    int factor = 3;
    int total = 0;
    std::for_each(nums.begin(), nums.end(),
                  [=, &total](int n) { total += factor * n; });
    std::cout << "[Demo5] sum(3*n) = " << total << "\n"; // 3*15 = 45

    // ─────────────────────────────────────────────────────────
    // LeetCode 1431. Kids With the Greatest Number of Candies
    //   題意：給每個小孩當前糖果數 candies[i] 與額外糖果 extra；判斷
    //         「i 拿到 extra 後是不是 >= 全班最多的人」。
    //
    //   為何放這？示範 [&] 在 std::transform / lambda 中讀取外部 maxC 與
    //   extra 的便利性。
    // ─────────────────────────────────────────────────────────
    {
        std::vector<int> candies{2, 3, 5, 1, 3};
        int extra = 3;

        int maxC = *std::max_element(candies.begin(), candies.end());

        std::vector<bool> result;
        result.reserve(candies.size());
        // 用 [&] 一次抓 maxC 與 extra（因為兩個都不會在 lambda 裡改）
        std::transform(candies.begin(), candies.end(),
                       std::back_inserter(result),
                       [&](int c) { return c + extra >= maxC; });

        std::cout << "[LC1431] result =";
        for (bool b : result) std::cout << ' ' << (b ? "T" : "F");
        std::cout << '\n'; // 預期：T T T F T
    }

    // ─────────────────────────────────────────────────────────
    // LeetCode 1365. How Many Numbers Are Smaller Than the Current Number
    //   題意：對每個 nums[i] 算「有多少個 nums[j] 嚴格小於它」。
    //   方法：用 count_if + lambda；目前元素值 cur 透過值捕獲 [cur] 帶進去，
    //         因為值捕獲安全又不影響外部、寫起來最清楚。
    // ─────────────────────────────────────────────────────────
    {
        std::vector<int> nums{8, 1, 2, 2, 3};
        std::vector<int> ans;
        ans.reserve(nums.size());
        for (int cur : nums) {
            // [cur] 值捕獲 — 每次 cur 變動，lambda 都以當下的 cur 為比對基準
            int cnt = std::count_if(nums.begin(), nums.end(),
                                    [cur](int x) { return x < cur; });
            ans.push_back(cnt);
        }
        std::cout << "[LC1365] ans =";
        for (int v : ans) std::cout << ' ' << v;
        std::cout << '\n'; // 預期：4 0 1 1 3
    }

    // ─────────────────────────────────────────────────────────
    // 實用範例：用 [&] 累積、用 [=] 拍快照 — 對照同一情境
    //   情境：篩出 log 等級 >= threshold 的訊息數
    // ─────────────────────────────────────────────────────────
    {
        std::vector<int> logLevels{0, 2, 1, 3, 2, 0, 3};   // 0=DEBUG ... 3=ERROR
        int threshold = 2;
        int hit = 0;

        // [&] 抓 hit (要累加) 與 threshold (要讀)；很實際但要小心生命週期
        std::for_each(logLevels.begin(), logLevels.end(), [&](int lv) {
            if (lv >= threshold) ++hit;
        });
        std::cout << "[log] entries >= WARN = " << hit << '\n';   // 4

        // 之後改 threshold，「快照版」用 [=] 把當下的 threshold 拍進去
        auto countAtLeast = [=](int lv) { return lv >= threshold; };
        threshold = 3;                                         // 改外部 threshold
        int errOnly = std::count_if(logLevels.begin(), logLevels.end(),
                                    countAtLeast);
        std::cout << "[log] snapshot threshold=2 still gives = " << errOnly << '\n';
        // 注意：countAtLeast 內看到的仍是建立 lambda 當下的 threshold=2，
        // 所以結果跟上面一樣 4，這就是「值捕獲是當下快照」的安全特性。
    }

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：[=] 跟 [&] 哪個常用？
    //    A：實務上「明寫要哪幾個」最安全，避免捕獲到不想要的東西。
    //       小範圍、stateless 的 lambda 可以放心用 [=] 或 []。
    //
    //  Q2：捕獲了 const 變數，lambda 裡還是 const 嗎？
    //    A：是。捕獲時保留 cv-qualifier；by value 捕獲的 const int 在 body
    //       裡還是 const int（即使加 mutable 也不能改 const 來源）。
    //
    //  Q3：可以捕獲 array 嗎？
    //    A：可以，但 by value 會「整塊複製」（就像 struct 成員一樣，不會
    //       退化成指標）。要小心成本。
    //
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra 02_capture_modes.cpp -o 02_capture_modes

// === 預期輸出 ===
// [Demo1] x changed to 999, byValue() = 10
// [Demo2] x changed to 777, byRef() = 777
// [Demo3] useAll() = 30
// [Demo4] sum = 15
// [Demo5] sum(3*n) = 45
// [LC1431] result = T T T F T
// [LC1365] ans = 4 0 1 1 3
// [log] entries >= WARN = 4
// [log] snapshot threshold=2 still gives = 4
