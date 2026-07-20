// =============================================================================
//  03_mutable_lambda.cpp  —  mutable 關鍵字
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/language/lambda
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、為什麼要 mutable？                                      │
//  └────────────────────────────────────────────────────────────┘
//
//  Lambda 編譯器產生的 closure class 之 operator() 預設是 const：
//
//      class __lambda {
//      public:
//          int x;
//          int operator()() const { /* 不能改 x */ }
//      };
//
//  如果要在 lambda 內修改「by-value 捕獲」的成員（也就是 closure 自己這份
//  拷貝），就要把 operator() 變成非 const，做法是加 `mutable`：
//
//      auto f = [x]() mutable { ++x; return x; };
//                       ^^^^^^^
//                  使 operator() 不再是 const
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、重要觀念：mutable 改的是「lambda 自己的拷貝」          │
//  └────────────────────────────────────────────────────────────┘
//
//  by-value 捕獲是把外部變數「複製進 closure」，所以 mutable 改的是這份
//  內部副本，外部變數完全不受影響。
//
//      int outer = 10;
//      auto f = [outer]() mutable { ++outer; return outer; };
//      f();                   // 內部副本變 11
//      f();                   // 內部副本變 12  ← 這份「持續累積」
//      // outer 還是 10，從頭到尾沒被改
//
//  另外注意：每次 lambda 被「拷貝」會帶著當前狀態走。
//
//      auto g = f;            // g 拿到當前 outer 的值（已經是 12）
//      g();                   // → 13（g 自己往前一步）
//      f();                   // → 13（f 也往前一步，但跟 g 是兩條線）
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、mutable 不需要時就不要加                               │
//  └────────────────────────────────────────────────────────────┘
//
//  * 純讀取的 lambda → 不加 mutable（const 提示讀者「我不會改狀態」）
//  * 只用 by-ref 捕獲 → 改的是外部變數本身，不需要 mutable
//  * 真的要在 lambda 裡累積狀態（counter、cache）→ 才加 mutable
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 四、本檔範例與 LeetCode                                    │
//  └────────────────────────────────────────────────────────────┘
//
//  * Demo 1：counter lambda（最經典 mutable 用法）
//  * Demo 2：每次拷貝後狀態獨立
//  * LeetCode 1480. Running Sum of 1d Array
// =============================================================================

/*
補充筆記：mutable_lambda
  - mutable 允許修改值捕獲的副本，不會修改外部原變數。
  - 若需要修改外部狀態，必須參考捕獲或捕獲指標，但要負責生命週期。
  - mutable lambda 的 operator() 不再是 const，這會影響它能否在某些容器或 wrapper 中使用。
  - mutable_lambda 要從 closure object 理解：lambda 不是神祕語法，而是編譯器產生的匿名函式物件。
  - 捕獲 by value 是在建立 lambda 時複製，by reference 是保存別名；延後執行時 reference 捕獲最容易 dangling。
  - mutable 只讓 by value 捕獲的副本可修改，不會修改外部原變數。
  - generic lambda 的 auto 參數本質上是 function call operator template，錯誤可能在呼叫時才出現。
  - std::function 可保存不同 callable，但可能有型別抹除成本和配置成本；效能敏感處可優先用 template 接 callable。
  - lambda 放進 algorithm 時應讓 predicate 無副作用或副作用明確，否則演算法意圖會變難讀。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】mutable lambda
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. mutable 到底改變了什麼？
//     答：它移除 operator() 的隱含 const，使「值捕獲的成員」可在本體內被修改。改的是
//     閉包內的副本，外部原變數不受影響；而因為那是成員，修改會跨多次呼叫累積——閉包
//     因此成為一個有狀態的物件。
//     追問：mutable 影響參考捕獲嗎？（不影響。const 的是 operator()，即成員本身 const；
//     一個 const 的 T& 成員仍然可以改它所指的物件）
//
// 🔥 Q2. auto c = [n = 0]() mutable { return n++; }; 連呼叫三次回傳什麼？再複製一份呢？
//     答：依序回傳 0、1、2（狀態存在閉包成員裡，跨呼叫累積）。auto copy = c; 會連同當時
//     的 n == 3 一起複製，copy() 回 3，之後 copy 與 c 各自獨立演進。
//     追問：這種有狀態的閉包可以直接傳給標準演算法嗎？（危險。演算法可以自由複製傳入的
//     functor，對 remove_if 這類演算法使用有狀態 predicate 的行為未定義）
//
// ⚠️ 陷阱. 加了 mutable，外部那個變數會不會跟著被改？
//     答：不會。mutable 只是解除 operator() 的 const，被改的永遠是閉包裡的那份副本。
//     為什麼會錯：mutable 這個字讓人聯想到「可變 = 直達原物件」，但要直達原物件是
//     參考捕獲 [&x] 的職責，兩件事完全無關。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <algorithm>

int main() {
    // ─────────────────────────────────────────────────────────
    // Demo 1：counter — 用 mutable 在 lambda 內累積狀態
    // ─────────────────────────────────────────────────────────
    auto counter = [n = 0]() mutable { return ++n; };  // init capture (C++14)
    std::cout << "[Demo1] " << counter() << ' '        // 1
              << counter() << ' '                      // 2
              << counter() << '\n';                    // 3

    // ─────────────────────────────────────────────────────────
    // Demo 2：拷貝 lambda 後，兩條獨立計數器
    // ─────────────────────────────────────────────────────────
    auto a = counter;          // 拷貝當前 n（值是 3）
    auto b = counter;
    std::cout << "[Demo2] a=" << a() << ' '      // a 從 3 → 4
              << "a=" << a() << ' '              // a 從 4 → 5
              << "b=" << b() << '\n';            // b 從 3 → 4，與 a 獨立

    // ─────────────────────────────────────────────────────────
    // 比對：不加 mutable 會編譯失敗
    //   auto bad = [n = 0]() { return ++n; };  // ❌ 對 const 成員 ++
    // ─────────────────────────────────────────────────────────

    // ─────────────────────────────────────────────────────────
    // LeetCode 1480. Running Sum of 1d Array
    //   題意：給 nums，回傳 [nums[0], nums[0]+nums[1], ...]
    //         也就是「前綴和」。
    //
    //   示範：用 std::transform + 一個 mutable lambda 內部累積 sum，
    //   原地把 nums 變成 prefix sum。
    // ─────────────────────────────────────────────────────────
    std::vector<int> nums{1, 2, 3, 4};
    std::transform(nums.begin(), nums.end(), nums.begin(),
                   [sum = 0](int x) mutable {
                       sum += x;        // 累積（mutable 才能修改 sum）
                       return sum;
                   });
    std::cout << "[LC1480] running sum =";
    for (int x : nums) std::cout << ' ' << x;
    std::cout << '\n'; // 預期：1 3 6 10

    // ─────────────────────────────────────────────────────────
    // LeetCode 1822. Sign of the Product of an Array
    //   題意：回傳陣列「所有元素相乘」的符號 (1 / -1 / 0)，但不要直接相乘
    //         (會 overflow)，只追蹤符號。
    //   方法：用 mutable lambda 內部維護一個 sign 狀態，跑過所有元素。
    // ─────────────────────────────────────────────────────────
    {
        std::vector<int> v{-1, -2, -3, -4, 3, 2, 1};
        // signTrack：值捕獲 sign=1，mutable 才能更新；遇 0 直接歸 0、遇負數翻號
        auto signTrack = [sign = 1](int x) mutable {
            if (x == 0) sign = 0;
            else if (x < 0 && sign != 0) sign = -sign;
            return sign;
        };
        int result = 1;
        for (int x : v) result = signTrack(x);    // 最後一次回傳就是最終 sign
        std::cout << "[LC1822] arraySign = " << result << '\n';  // 預期：1
    }

    // ─────────────────────────────────────────────────────────
    // 實用範例：mutable lambda 做 ID 產生器（每次呼叫拿到下一個 ID）
    //   工作中常見：批次匯入資料時需要產生連續 ID
    // ─────────────────────────────────────────────────────────
    {
        // 從 1000 開始，每次回傳下一個 ID
        auto nextId = [id = 1000]() mutable { return ++id; };

        std::vector<std::string> users{"alice", "bob", "charlie"};
        std::cout << "[id-gen] ";
        for (const auto& u : users) {
            std::cout << u << "=" << nextId() << ' ';
        }
        std::cout << '\n'; // 預期：alice=1001 bob=1002 charlie=1003
    }

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：std::accumulate 不就能算前綴和嗎？
    //    A：accumulate 算的是「最後總和」，不是每一步的中間值。
    //       做前綴和有兩條路：
    //         (1) std::partial_sum — 標準函式，最直接
    //         (2) 自己用 mutable lambda + transform — 教學示意
    //       實務當然用 partial_sum；這裡示範 mutable 的使用價值。
    //
    //  Q2：為什麼 STL 演算法傳遞 functor 時建議「無狀態」？
    //    A：許多演算法不保證對 functor「只呼叫一次拷貝」— 它可能拷貝多份
    //       平行呼叫，導致狀態「分裂」（這也是 Demo 2 看到的現象）。所以
    //       STL 風格建議：functor 無狀態，狀態放外部 by-ref 捕獲。
    //       但 std::transform 是有保證序列順序的，所以上面這個範例安全。
    //
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra 03_mutable_lambda.cpp -o 03_mutable_lambda

// === 預期輸出 ===
// [Demo1] 1 2 3
// [Demo2] a=4 a=5 b=4
// [LC1480] running sum = 1 3 6 10
// [LC1822] arraySign = 1
// [id-gen] alice=1001 bob=1002 charlie=1003
