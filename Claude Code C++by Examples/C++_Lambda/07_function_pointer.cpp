// =============================================================================
//  07_function_pointer.cpp  —  Function Pointer / Lambda 與舊式 API 的橋樑
// =============================================================================
//  參考：
//    https://en.cppreference.com/w/cpp/language/pointer#Pointers_to_functions
//    https://en.cppreference.com/w/cpp/utility/functional/mem_fn
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、function pointer 是什麼？                              │
//  └────────────────────────────────────────────────────────────┘
//
//  指向「函式本身」的指標。語法很醜：
//
//      int (*fp)(int, int) = nullptr;     // 指向 int(int,int) 的指標
//      using FP = int(*)(int, int);       // typedef 化更清楚
//      FP fp2 = nullptr;
//
//  跟 std::function 的差別：
//   * function pointer = 純位址，沒有狀態（不能裝有捕獲的 lambda）
//   * std::function     = 任意可呼叫物件 + 狀態
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、無捕獲 lambda → function pointer 的隱式轉換            │
//  └────────────────────────────────────────────────────────────┘
//
//  關鍵性質：「捕獲列表為空」的 lambda 可以隱式轉成同簽名的 function
//  pointer。這讓 lambda 可以傳給「只吃 C 風格 callback」的舊 API
//  （qsort、pthread_create、Win32 / GTK API 等）：
//
//      auto cmp = [](const void* a, const void* b) {
//          return *(const int*)a - *(const int*)b;
//      };
//      int arr[] = {3,1,4,1,5,9};
//      std::qsort(arr, 6, sizeof(int), cmp);   // ✅ 隱式轉成 function pointer
//
//  但只要捕獲了任何東西（例如 [&out]），這個轉換就失效 — 因為「狀態」沒
//  地方放在純函式指標裡。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、std::mem_fn — 把成員函式包成可呼叫物件                 │
//  └────────────────────────────────────────────────────────────┘
//
//  成員函式指標語法極醜（涉及 this），實務上要把它拿來給 STL 演算法用，
//  推薦用 std::mem_fn 包成「對 obj 呼叫某成員函式」的可呼叫物件：
//
//      struct Foo { void hello() const; };
//      std::vector<Foo> v(3);
//      std::for_each(v.begin(), v.end(), std::mem_fn(&Foo::hello));
//
//  也可以直接用 lambda：
//      std::for_each(v.begin(), v.end(), [](const Foo& f){ f.hello(); });
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 四、本檔範例                                               │
//  └────────────────────────────────────────────────────────────┘
//
//  * Demo 1：無捕獲 lambda 隱式轉成 function pointer
//  * Demo 2：嘗試用「有捕獲」的 lambda → 編譯錯展示
//  * Demo 3：std::mem_fn 對 vector<string> 呼叫 .size()
// =============================================================================

/*
補充筆記：function_pointer
  - function_pointer 會產生一個 closure 物件；capture list 決定它保存資料還是借用資料。
  - 值捕獲安全但可能拷貝，參考捕獲便宜但有生命週期風險。
  - lambda 若被存起來、丟到 thread 或包進 std::function，捕獲物件必須活得夠久。
  - function_pointer 要從 closure object 理解：lambda 不是神祕語法，而是編譯器產生的匿名函式物件。
  - 捕獲 by value 是在建立 lambda 時複製，by reference 是保存別名；延後執行時 reference 捕獲最容易 dangling。
  - mutable 只讓 by value 捕獲的副本可修改，不會修改外部原變數。
  - generic lambda 的 auto 參數本質上是 function call operator template，錯誤可能在呼叫時才出現。
  - std::function 可保存不同 callable，但可能有型別抹除成本和配置成本；效能敏感處可優先用 template 接 callable。
  - lambda 放進 algorithm 時應讓 predicate 無副作用或副作用明確，否則演算法意圖會變難讀。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】函式指標與 lambda 的轉換
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 無捕獲 lambda 能轉成函式指標嗎？為什麼有捕獲就不行？
//     答：無捕獲的 lambda 有一個到 R(*)(Args...) 的隱式轉換運算子，所以可以傳給
//     qsort、pthread_create 這類只吃 C 風格回呼的 API。有捕獲就不行——函式指標只是一個
//     位址，沒有任何地方存放捕獲的狀態。這也正是 C 的回呼 API 幾乎都額外提供一個
//     void* user_data 參數的原因。
//     追問：有捕獲卻又非傳給 C API 不可，怎麼辦？（把狀態放進 void* user_data，用一個
//     無捕獲 lambda 當 trampoline，在裡面轉回原本的型別再呼叫）
//
// 🔥 Q2. 閉包型別還有哪些特殊性質？
//     答：有捕獲者不可賦值；C++20 前連無捕獲 lambda 都沒有預設建構子（C++20 起放寬）；
//     有隱式的複製／移動建構子；若成員是參考捕獲，複製後兩個閉包共享同一個被參考物；
//     每個 lambda 的型別唯一，因此 decltype(lambda) 可拿來當關聯容器的比較器型別。
//
// ⚠️ 陷阱. 「lambda 一定比函式指標快」——這句話對嗎？
//     答：不完全對。正確說法是「lambda 作為模板參數傳遞時，呼叫端知道確切型別，因此
//     可以 inline」。一旦把 lambda 塞進 std::function，就退化成型別擦除的間接呼叫，
//     可能比函式指標更慢；反之函式指標在編譯器能做去虛擬化／常數傳播時也可能被 inline。
//     為什麼會錯：把效能歸因到「lambda 這個語法」，而真正的關鍵是「呼叫端是否知道確切
//     型別」。
// ═══════════════════════════════════════════════════════════════════════════

#include <algorithm>
#include <cstdlib>          // qsort
#include <functional>       // mem_fn
#include <iostream>
#include <string>
#include <vector>

// 一個傳統的 C 風格 API：吃 function pointer 比較器
static void runQsort(int* a, std::size_t n, int (*cmp)(const void*, const void*)) {
    std::qsort(a, n, sizeof(int), cmp);
}

int main() {
    // ─────────────────────────────────────────────────────────
    // Demo 1：無捕獲 lambda → function pointer 隱式轉換
    // ─────────────────────────────────────────────────────────
    int arr[] = {3, 1, 4, 1, 5, 9, 2, 6};
    runQsort(arr, std::size(arr),
             [](const void* a, const void* b) {
                 int x = *static_cast<const int*>(a);
                 int y = *static_cast<const int*>(b);
                 return x - y;
             });
    std::cout << "[Demo1] qsort sorted:";
    for (int x : arr) std::cout << ' ' << x;
    std::cout << '\n';

    // ─────────────────────────────────────────────────────────
    // Demo 2：有捕獲時不能轉成 function pointer
    //   下面這段會編譯失敗，留作對照
    // ─────────────────────────────────────────────────────────
    // int multiplier = 2;
    // auto badCmp = [multiplier](const void* a, const void* b) {
    //     return (*(const int*)a - *(const int*)b) * multiplier;
    // };
    // runQsort(arr, std::size(arr), badCmp);  // ❌ 編譯錯：closure 不能轉 fp
    //
    // 解法：要嘛改 lambda 不要捕獲，要嘛改 API 改吃 std::function。

    // ─────────────────────────────────────────────────────────
    // Demo 3：std::mem_fn — 對容器內每個物件呼叫成員函式
    // ─────────────────────────────────────────────────────────
    std::vector<std::string> words{"apple", "banana", "cherry"};
    // 想做的事：印出每個 word 的長度
    // 寫法 A：std::mem_fn
    std::cout << "[Demo3-A] sizes via mem_fn:";
    std::for_each(words.begin(), words.end(),
                  [size = std::mem_fn(&std::string::size)](const std::string& s) {
                      std::cout << ' ' << size(s);
                  });
    std::cout << '\n';

    // 寫法 B：直接用 lambda（更直觀）
    std::cout << "[Demo3-B] sizes via lambda:";
    std::for_each(words.begin(), words.end(),
                  [](const std::string& s) {
                      std::cout << ' ' << s.size();
                  });
    std::cout << '\n';

    // ─────────────────────────────────────────────────────────
    // 實用範例：用 function pointer 表（jump table）做指令 dispatcher
    //   特點：所有處理函式簽名一致 → 簡單 table lookup；比 std::function 輕
    //   應用場景：嵌入式、低開銷狀態機、簡單 CLI 等
    // ─────────────────────────────────────────────────────────
    {
        using Op = int(*)(int, int);

        // 都是「無捕獲 lambda」 → 自動轉成 function pointer
        Op add = [](int a, int b) { return a + b; };
        Op sub = [](int a, int b) { return a - b; };
        Op mul = [](int a, int b) { return a * b; };

        struct Entry { const char* name; Op fn; };
        Entry table[] = {{"add", add}, {"sub", sub}, {"mul", mul}};

        auto run = [&](const char* name, int x, int y) {
            for (auto& e : table) {
                // C-string 比較用簡單字串相等（避免引入 cstring）
                bool same = true;
                for (int i = 0; e.name[i] || name[i]; ++i) {
                    if (e.name[i] != name[i]) { same = false; break; }
                }
                if (same) {
                    std::cout << "  " << name << "(" << x << "," << y
                              << ") = " << e.fn(x, y) << '\n';
                    return;
                }
            }
            std::cout << "  unknown: " << name << '\n';
        };
        std::cout << "[jump-table]\n";
        run("add", 3, 4);
        run("sub", 10, 7);
        run("mul", 6, 5);
    }

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：什麼時候真的會用到 function pointer？
    //    A：跨 C/C++ 邊界的 API（qsort、bsearch、signal handler、pthread、
    //       Win32 callback、GTK callback、driver 等）— 這些 API 不認識
    //       lambda 也不認識 std::function，只認 function pointer。
    //
    //  Q2：lambda 能取位址嗎？
    //    A：可以對「無捕獲」lambda 取得它對應的 function pointer：
    //         auto f = [](int x) { return x; };
    //         int (*fp)(int) = +f;       // unary + 觸發隱式轉換
    //
    //  Q3：mem_fn vs lambda 怎麼選？
    //    A：lambda 大致都能取代 mem_fn，且更直觀；mem_fn 的優勢是「短」、
    //       且能透過 std::bind 進一步組合。多數情況直接用 lambda 即可。
    //
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra 07_function_pointer.cpp -o 07_function_pointer

// === 預期輸出 ===
// [Demo1] qsort sorted: 1 1 2 3 4 5 6 9
// [Demo3-A] sizes via mem_fn: 5 6 6
// [Demo3-B] sizes via lambda: 5 6 6
// [jump-table]
//   add(3,4) = 7
//   sub(10,7) = 3
//   mul(6,5) = 30
