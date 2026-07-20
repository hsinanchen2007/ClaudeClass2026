// =============================================================================
//  第 19 課-10：從 C 的動態陣列建立 vector —— 接管資料、交還記憶體
// =============================================================================
//
// 【主題資訊 Information】
//   template<class InputIt>
//   vector(InputIt first, InputIt last, const Allocator& = Allocator());
//     以「迭代器區間」建構；原生指標本身就是合格的 random access 迭代器，
//     所以 vector<int> v(arr, arr + n) 完全合法。
//   相關：v.assign(first, last)（重新填充既有 vector）
//   標準版本：C++98 起
//   複雜度：O(n)，並可能配置一次記憶體
//   標頭檔：<vector>
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼裸指標可以當迭代器用】
//   STL 的迭代器是一組「行為契約」而非某個類別：
//   能 *it 解參考、能 ++it 前進、能 it1 != it2 比較，就是迭代器。
//   原生指標天生就滿足全部條件，而且滿足最強的 random access 類別。
//   這正是 STL 設計的精妙之處——它讓「C 的陣列」與「C++ 的容器」
//   共用同一套演算法。
//   所以 std::sort(arr, arr + n) 對原生陣列也直接可用。
//
// 【2. 這個建構子做的是「複製」，不是「接管」】
//   vector<int> v(arr, arr + n) 會配置自己的記憶體、
//   把 n 個元素逐一複製過去。
//   之後 v 與 arr 是兩份完全獨立的資料：
//   改 v 不影響 arr、delete[] arr 也不影響 v。
//   這正是為什麼複製完就可以（也應該）立刻釋放原始陣列——
//   vector 已經擁有自己的副本了。
//
// 【3. C++ 沒有「把既有 heap 指標交給 vector 託管」的辦法】
//   這是很常見的期待，但標準明確不支援：
//   vector 一定要用自己的 allocator 配置記憶體，
//   它的解構子會呼叫 allocator 的 deallocate，
//   而那個 allocator 對「別人 malloc/new 出來的指標」一無所知。
//   若真的要零複製地接管一塊既有記憶體，選項是：
//     ▸ std::unique_ptr<int[]>（C++11）——單純管理生命週期
//     ▸ std::span<int>（C++20）——非擁有的視圖，不管生命週期
//     ▸ 自訂 allocator（複雜，很少值得）
//   實務上多數情況「複製一次」的成本是可以接受的。
//
// 【4. 三種從陣列建 vector 的寫法】
//     vector<int> v1(arr, arr + n);              // 迭代器區間（最常用）
//     vector<int> v2; v2.assign(arr, arr + n);   // 對既有 vector 重新填充
//     vector<int> v3(std::begin(arr), std::end(arr));  // C++11，僅限真陣列
//   注意第三種：std::begin/end 只對「真正的陣列型別」有效。
//   陣列一旦退化成指標（例如傳進函式後），就拿不到長度，這招失效。
//
// 【5. 陣列退化（array decay）是這裡最大的坑】
//   void f(int arr[]) { size_t n = sizeof(arr) / sizeof(arr[0]); }
//   參數 arr 實際上是 int*，sizeof(arr) 是指標大小（本機 8），
//   除以 sizeof(int)（4）得到 2——不論你傳進來的陣列多大都是 2。
//   所以 C API 一律要求「指標 + 長度」兩個參數，
//   而這正是 C++20 引入 std::span 想解決的問題。
//
// 【概念補充 Concept Deep Dive】
//   ▸ 為什麼 vector 的區間建構子能推導出「這是迭代器不是數量」
//     vector 有兩個看起來衝突的建構子：
//       vector(size_type count, const T& value);   // n 個 value
//       vector(InputIt first, InputIt last);       // 區間
//     寫 vector<int> v(3, 5); 時該選哪個？
//     標準規定：若 InputIt 是整數型別，則視為第一個版本。
//     實作上透過 iterator_traits 與 enable_if 完成這個判斷。
//     這就是為什麼 vector<int> v(3, 5) 得到 {5,5,5} 而不是編譯錯誤。
//   ▸ 複製 trivially copyable 型別時會被最佳化成 memmove
//     對 int 這類型別，libstdc++ 的區間建構會走到 std::__copy_move
//     的特化，最終呼叫 memmove。所以「逐一複製」在效能上
//     等同一次整塊複製，不必擔心。
//   ▸ 為什麼建議用 unique_ptr<T[]> 而不是裸 new[]
//     即使只是「暫時持有再複製給 vector」，中間若有任何路徑丟出例外，
//     裸 new[] 就洩漏了。unique_ptr<T[]> 的解構子保證會 delete[]。
//
// 【注意事項 Pay Attention】
//   1. 區間建構是「複製」，vector 不會接管原指標的所有權。
//   2. 複製完成後才可以 delete[] 原陣列；兩者之後完全獨立。
//   3. C++ 沒有標準方法讓 vector 託管既有的 heap 指標。
//   4. 陣列傳進函式會退化成指標，sizeof 技巧失效——必須另外傳長度。
//   5. std::begin/end 只對真正的陣列型別有效，對指標無效。
//   6. new[] 必須配 delete[]（不是 delete）；更好的做法是用
//      unique_ptr<T[]> 或直接用 vector。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】從原始陣列建立 vector
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 vector<int> v(arr, arr + n); 這樣寫是合法的？
//     答：因為 STL 的迭代器是一組行為契約而非特定類別——
//         能解參考、能遞增、能比較就是迭代器，
//         而原生指標天生滿足全部條件，且屬於最強的 random access 類別。
//         所以 arr 與 arr + n 構成一個合法的迭代器區間。
//         同理 std::sort(arr, arr + n) 對原生陣列也直接可用。
//     追問：這個建構子是複製還是接管？
//         → 複製。vector 會配置自己的記憶體並逐一複製元素，
//           之後兩者完全獨立，所以複製完就可以立刻 delete[] 原陣列。
//
// 🔥 Q2. 可以讓 vector 直接「接管」一塊 new 出來的記憶體嗎？
//     答：標準沒有提供這個能力。vector 必須用自己的 allocator 配置記憶體，
//         它的解構子會呼叫該 allocator 的 deallocate，
//         而 allocator 對別人 new 出來的指標一無所知。
//         若要零複製地持有既有記憶體，可用
//         std::unique_ptr<T[]>（管生命週期）或
//         std::span<T>（C++20，非擁有的視圖，不管生命週期）。
//     追問：那 vector 有沒有辦法把自己的記憶體「交出去」？
//         → 標準也沒有。有人提過 release() 之類的介面但未被採納，
//           理由同樣是 allocator 的對稱性——拿走之後誰來 deallocate？
//
// ⚠️ 陷阱. void process(int arr[]) {
//              size_t n = sizeof(arr) / sizeof(arr[0]);   // ← 為什麼永遠算錯？
//              std::vector<int> v(arr, arr + n);
//          }
//     答：函式參數中的 int arr[] 只是 int* 的另一種寫法（陣列退化）。
//         sizeof(arr) 得到的是指標大小（本機 x86-64 為 8），
//         除以 sizeof(int)（4）恆為 2——不論實際傳進來的陣列多大。
//         於是 vector 只會複製到 2 個元素。
//     為什麼會錯：以為 sizeof 技巧在函式內外都成立。
//         它只在「真正的陣列型別」上有效；一旦作為函式參數，
//         陣列就退化成指標，長度資訊在編譯期就已經遺失。
//         正確做法是額外傳長度，或用 std::span（C++20）
//         把指標與長度綁在一起。
// ═══════════════════════════════════════════════════════════════════════════

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

// 模擬一個 C 函式庫：配置並填充一塊資料，由呼叫端負責釋放
int* c_library_fetch(std::size_t* outSize) {
    const std::size_t n = 5;
    int* buf = new int[n];
    for (std::size_t i = 0; i < n; ++i) buf[i] = static_cast<int>((i + 1) * 11);
    *outSize = n;
    return buf;
}

// 錯誤示範（僅供對照，不呼叫）：陣列退化導致長度算錯
// void processWrong(int arr[]) {
//     std::size_t n = sizeof(arr) / sizeof(arr[0]);   // 恆為 8/4 = 2
//     std::vector<int> v(arr, arr + n);               // 只複製到 2 個元素
// }

// 正確做法：指標 + 長度分開傳
std::vector<int> processRight(const int* arr, std::size_t n) {
    return std::vector<int>(arr, arr + n);
}

// -----------------------------------------------------------------------------
// 【日常實務範例】把 C 函式庫回傳的緩衝區安全轉成 vector
//   情境：影像/音訊/資料庫的 C 函式庫常見的介面是
//         「回傳一根 malloc/new 出來的指標 + 一個長度」，
//         並要求呼叫端負責釋放。
//   為什麼用本主題：這是 C++ 專案接 C 函式庫最常見的接縫。
//         正確做法是「立刻用 RAII 接住指標 → 複製進 vector →
//         讓 RAII 自動釋放」，這樣即使中途丟出例外也不會洩漏。
//         直接寫 new[] ... delete[] 的版本只要中間有任何 return 或
//         例外就漏定了。
// -----------------------------------------------------------------------------
std::vector<int> fetchAsVector() {
    std::size_t n = 0;
    // 立刻用 unique_ptr<int[]> 接住，保證離開作用域時 delete[]
    std::unique_ptr<int[]> raw(c_library_fetch(&n));

    // 複製成 vector（即使這一步丟出 bad_alloc，raw 仍會被正確釋放）
    std::vector<int> v(raw.get(), raw.get() + n);
    return v;
    // raw 在此自動 delete[]，不需要（也不可以）手動釋放
}

int main() {
    std::cout << "=== 一、從動態配置的陣列建立 vector ===" << std::endl;
    int size = 5;
    int* dynamic_arr = new int[size];
    for (int i = 0; i < size; ++i) {
        dynamic_arr[i] = (i + 1) * 11;
    }

    // 複製到 vector（注意：是複製，不是接管所有權）
    std::vector<int> v(dynamic_arr, dynamic_arr + size);

    // 釋放原始陣列（vector 已有自己的副本，完全不受影響）
    delete[] dynamic_arr;
    dynamic_arr = nullptr;

    std::cout << "vector 內容：";
    for (int x : v) std::cout << x << " ";
    std::cout << std::endl;
    std::cout << "（原陣列已 delete[]，vector 的資料獨立存在）" << std::endl;

    std::cout << "\n=== 二、三種從陣列建立 vector 的寫法 ===" << std::endl;
    int arr[] = {10, 20, 30, 40, 50};
    const std::size_t n = sizeof(arr) / sizeof(arr[0]);   // 真陣列，sizeof 有效

    std::vector<int> v1(arr, arr + n);                    // 迭代器區間
    std::vector<int> v2;
    v2.assign(arr, arr + n);                              // assign 重新填充
    std::vector<int> v3(std::begin(arr), std::end(arr));  // C++11，僅限真陣列

    std::cout << "v1(區間)      : ";
    for (int x : v1) std::cout << x << " ";
    std::cout << std::endl;
    std::cout << "v2(assign)    : ";
    for (int x : v2) std::cout << x << " ";
    std::cout << std::endl;
    std::cout << "v3(begin/end) : ";
    for (int x : v3) std::cout << x << " ";
    std::cout << std::endl;
    std::cout << "三者相同: " << std::boolalpha << (v1 == v2 && v2 == v3) << std::endl;

    std::cout << "\n=== 三、複製之後兩者完全獨立 ===" << std::endl;
    int src[] = {1, 2, 3};
    std::vector<int> copy(src, src + 3);
    src[0] = 999;              // 修改原陣列
    copy[1] = 888;             // 修改 vector
    std::cout << "改原陣列後 src[0]=" << src[0] << ", copy[0]=" << copy[0] << std::endl;
    std::cout << "改 vector 後 src[1]=" << src[1] << ", copy[1]=" << copy[1] << std::endl;

    std::cout << "\n=== 四、陣列退化：sizeof 技巧為什麼會失效 ===" << std::endl;
    std::cout << "在 main 中（真陣列）sizeof(arr)/sizeof(arr[0]) = " << n << std::endl;
    // 註：這裡刻意把兩個 sizeof 先存進變數再相除。
    //     若直接寫成 sizeof(int*) / sizeof(int)，GCC 會以
    //     -Wsizeof-pointer-div 警告「這不是在計算陣列元素個數」——
    //     那正是本段要示範的 bug，但我們不希望教材本身帶警告。
    const std::size_t ptrSize = sizeof(int*);
    const std::size_t eltSize = sizeof(int);
    std::cout << "但傳進函式後 arr 退化成 int*，sizeof(int*)=" << ptrSize
              << " / sizeof(int)=" << eltSize
              << " → 恆為 " << (ptrSize / eltSize) << "，與實際長度無關" << std::endl;
    std::cout << "（GCC 有 -Wsizeof-pointer-div 專門警告這個錯誤，"
              << "但只在直接寫出該運算式時才偵測得到）" << std::endl;
    std::vector<int> right = processRight(arr, n);
    std::cout << "改傳「指標 + 長度」的正確結果：";
    for (int x : right) std::cout << x << " ";
    std::cout << std::endl;

    std::cout << "\n=== 五、原生指標就是合格的迭代器 ===" << std::endl;
    int unsorted[] = {50, 30, 10, 40, 20};
    std::sort(unsorted, unsorted + 5);          // STL 演算法直接吃裸指標
    std::cout << "std::sort 直接對原生陣列排序：";
    for (int x : unsorted) std::cout << x << " ";
    std::cout << std::endl;
    std::cout << "std::count(arr, arr+5, 30) = "
              << std::count(unsorted, unsorted + 5, 30) << std::endl;

    std::cout << "\n=== 六、日常實務：安全接收 C 函式庫的緩衝區 ===" << std::endl;
    std::vector<int> fetched = fetchAsVector();
    std::cout << "從 C 函式庫取得並轉成 vector：";
    for (int x : fetched) std::cout << x << " ";
    std::cout << std::endl;
    std::cout << "（用 unique_ptr<int[]> 接住原指標，離開作用域自動 delete[]，"
              << "即使中途丟例外也不會洩漏）" << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 19 課：vector 與原始陣列的互操作10.cpp" -o array_to_vector
//
// 【關於下方預期輸出的但書】
//   第四段的 sizeof(int*) = 8、sizeof(int) = 4 是本機
//   （x86-64 / Ubuntu / GCC 15.2）的實測值，屬實作定義；
//   在 32-bit 平台上 sizeof(int*) 會是 4，該行的「恆為 2」會變成「恆為 1」。
//   陣列退化導致長度資訊遺失這個結論本身則不受平台影響。
//
// 【本檔未附 LeetCode 範例的理由】
//   本檔的主題是「C 與 C++ 之間的記憶體所有權交接」——
//   誰配置、誰釋放、什麼時候可以安全 delete[]。
//   LeetCode 的函式簽章一律直接給 vector<int>&，
//   既沒有裸指標也沒有所有權問題，硬套一題無法呈現重點。
//   因此改以「安全接收 C 函式庫緩衝區」這個真實接縫呈現。

// === 預期輸出 ===
// === 一、從動態配置的陣列建立 vector ===
// vector 內容：11 22 33 44 55
// （原陣列已 delete[]，vector 的資料獨立存在）
//
// === 二、三種從陣列建立 vector 的寫法 ===
// v1(區間)      : 10 20 30 40 50
// v2(assign)    : 10 20 30 40 50
// v3(begin/end) : 10 20 30 40 50
// 三者相同: true
//
// === 三、複製之後兩者完全獨立 ===
// 改原陣列後 src[0]=999, copy[0]=1
// 改 vector 後 src[1]=2, copy[1]=888
//
// === 四、陣列退化：sizeof 技巧為什麼會失效 ===
// 在 main 中（真陣列）sizeof(arr)/sizeof(arr[0]) = 5
// 但傳進函式後 arr 退化成 int*，sizeof(int*)=8 / sizeof(int)=4 → 恆為 2，與實際長度無關
// （GCC 有 -Wsizeof-pointer-div 專門警告這個錯誤，但只在直接寫出該運算式時才偵測得到）
// 改傳「指標 + 長度」的正確結果：10 20 30 40 50
//
// === 五、原生指標就是合格的迭代器 ===
// std::sort 直接對原生陣列排序：10 20 30 40 50
// std::count(arr, arr+5, 30) = 1
//
// === 六、日常實務：安全接收 C 函式庫的緩衝區 ===
// 從 C 函式庫取得並轉成 vector：11 22 33 44 55
// （用 unique_ptr<int[]> 接住原指標，離開作用域自動 delete[]，即使中途丟例外也不會洩漏）
