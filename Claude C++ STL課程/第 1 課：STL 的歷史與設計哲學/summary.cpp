/*
 * ============================================================
 * 【第一課：STL 的歷史與設計哲學】總複習 summary.cpp
 * ============================================================
 * 本課程重點：
 * 1. STL 的定義與用途
 * 2. STL 的歷史背景與創造者 Alexander Stepanov
 * 3. STL 的三大核心設計原則：正交性、效率優先、可組合性
 * 4. STL 與傳統 C 風格程式碼的對比
 * 5. 泛型編程的初步概念
 * ============================================================
 */

#include <iostream>
#include <vector>
#include <list>
#include <algorithm>
#include <numeric>
#include <iterator>
#include <chrono>
#include <string>


// ===== 重點一：什麼是 STL？=====
// STL（Standard Template Library，標準模板庫）是 C++ 標準函式庫的核心部分。
// 它提供了一套「經過高度優化、可重複使用」的程式元件：
//   - 容器（Containers）：vector, list, set, map...
//   - 演算法（Algorithms）：sort, find, copy...
//   - 迭代器（Iterators）：連接容器與演算法的橋樑
//
// 核心價值：不需要自己實作常見的資料結構和演算法，STL 已經幫你做好了。
// 使用前需要 include 對應的標頭檔：
//   #include <vector>     → std::vector
//   #include <algorithm>  → std::sort, std::find
//   #include <numeric>    → std::accumulate


// ===== 重點二：STL 的歷史背景 =====
// 創造者：Alexander Stepanov（亞歷山大·斯捷潘諾夫），出生於蘇聯的電腦科學家。
//
// 時間軸：
//   1970年代  → Stepanov 開始思考「泛型編程」的概念
//   1979年    → 在莫斯科開始研究抽象演算法
//   1985年    → 移居美國，在 GE 研究中心繼續研究
//   1987年    → 在 Bell Labs 與 Andrew Koenig 合作
//   1992年    → 加入 HP 實驗室，與 Meng Lee 合作開發 STL
//   1994年7月 → STL 被正式納入 C++ 標準草案（委員會只花幾天就決定納入！）
//   1998年    → 隨 C++98 標準正式發布
//
// 關鍵理念（Stepanov 的願景）：
//   「演算法不應該依賴於特定的資料結構，資料結構也不應該綁定特定的演算法。」


// ===== 重點三：STL 的設計哲學 =====
// 核心理念：「用最少的程式碼，解決最多的問題。」
// 這透過「泛型編程（Generic Programming）」達成。
//
// 三大設計原則：
//
// 【原則一：正交性（Orthogonality）】
//   容器 和 演算法 是獨立的兩個維度，透過 迭代器 連接。
//   好處：M 個容器 × N 個演算法，只需要 M + N 個元件（而非 M × N 個）！
//
//   架構示意：
//   ┌──────────────┐    ┌──────────────┐
//   │  容器         │◄──►│  演算法      │
//   │  vector      │    │  sort        │
//   │  list        │    │  find        │
//   │  set         │    │  copy        │
//   └──────────────┘    └──────────────┘
//          ↑ 透過迭代器連接 ↑
//
// 【原則二：效率優先（Efficiency）】
//   「泛型程式碼的效能應該與手寫特化程式碼相當。」
//   STL 透過 template 在「編譯期」展開，避免了執行期的效能損失。
//   例如：std::sort 內部使用 IntroSort（混合 QuickSort + HeapSort + InsertionSort）。
//
// 【原則三：可組合性（Composability）】
//   STL 的元件可以像樂高積木一樣自由組合，創造複雜功能。


// ===== 重點四：為什麼需要 STL？C 風格的問題 =====
// C 語言風格的動態陣列管理（展示問題）：

void demo_c_style_problem() {
    // C 風格：需要手動管理記憶體，容易出錯
    // int* numbers = (int*)malloc(capacity * sizeof(int));
    // ... 需要手動 realloc、free，且需要自己寫 qsort 的比較函數
    // 缺點：
    //   - 程式碼冗長（約 50 行做一件簡單的事）
    //   - 記憶體洩漏風險高（忘記 free）
    //   - 可讀性差（需要理解指標操作）
    //   - 維護成本高
    std::cout << "[C 風格] 需要手動管理記憶體，請對比下面的 STL 風格" << std::endl;
}

// STL 風格解決相同問題（更簡潔、更安全）：
void demo_stl_style() {
    // 步驟一：建立 vector（自動管理記憶體）
    std::vector<int> numbers = {5, 2, 8, 1, 9, 3, 7, 4, 6};

    // 步驟二：排序（不用自己寫排序演算法）
    std::sort(numbers.begin(), numbers.end());

    // 步驟三：找最大值（使用迭代器，自動解引用）
    int max_val = *std::max_element(numbers.begin(), numbers.end());

    // 步驟四：計算總和
    int sum = std::accumulate(numbers.begin(), numbers.end(), 0);

    std::cout << "[STL 風格] 排序後: ";
    for (int n : numbers) std::cout << n << " ";
    std::cout << std::endl;
    std::cout << "最大值: " << max_val << ", 總和: " << sum << std::endl;
    // 好處：
    //   - 約 10 行解決同樣的問題
    //   - 記憶體自動管理，不會洩漏
    //   - 意圖清晰，易讀易懂
    //   - 維護成本低
}


// ===== 重點五：正交性實際示範（泛型 sort 用於多種型別）=====
// 同一個 std::sort 可以處理任何有 < 運算子的型別！
// 這就是「泛型」的威力：一套程式碼，處理多種型別。
void demo_generic_sort() {
    std::vector<int> ints = {3, 1, 4, 1, 5};
    std::vector<double> doubles = {3.14, 1.41, 2.72};
    std::vector<std::string> strings = {"cherry", "apple", "banana"};

    // 同一個 sort，處理所有型別！
    std::sort(ints.begin(), ints.end());
    std::sort(doubles.begin(), doubles.end());
    std::sort(strings.begin(), strings.end());

    std::cout << "整數排序: ";
    for (int n : ints) std::cout << n << " ";
    std::cout << std::endl;

    std::cout << "浮點數排序: ";
    for (double d : doubles) std::cout << d << " ";
    std::cout << std::endl;

    std::cout << "字串排序: ";
    for (const auto& s : strings) std::cout << s << " ";
    std::cout << std::endl;
}


// ===== 重點六：可組合性示範 =====
// STL 元件可以組合使用，創造強大功能
void demo_composability() {
    std::vector<int> source = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    std::vector<int> destination;

    // 組合使用：
    //   1. copy_if（演算法）：只複製滿足條件的元素
    //   2. back_inserter（迭代器配接器）：自動在容器尾端插入
    //   3. Lambda（函數物件）：定義篩選條件
    std::copy_if(
        source.begin(),                            // 來源開始
        source.end(),                              // 來源結束
        std::back_inserter(destination),           // 目標（自動擴展）
        [](int n) { return n % 2 == 0; }          // 條件：偶數
    );

    std::cout << "從 1~10 中只複製偶數: ";
    for (int n : destination) std::cout << n << " ";
    std::cout << std::endl;
    // 輸出：2 4 6 8 10
}


// ===== 重點七：效率優先示範（測量 STL 演算法速度）=====
void demo_efficiency() {
    const int SIZE = 100000;  // 示範用，減小規模
    std::vector<int> data(SIZE);

    // 填充隨機數據
    for (int i = 0; i < SIZE; ++i) {
        data[i] = rand();
    }

    // 測量 std::sort 的時間
    auto start = std::chrono::high_resolution_clock::now();
    std::sort(data.begin(), data.end());
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "排序 " << SIZE << " 個元素耗時: "
              << duration.count() << " 微秒" << std::endl;
    // std::sort 使用 IntroSort，是目前最優秀的通用排序演算法之一
    // 效能幾乎等同於手寫的最優化排序
}


// ===== 對比表 =====
// ┌─────────────┬──────────────────┬───────────────────┐
// │    面向     │  C 風格          │  STL 風格          │
// ├─────────────┼──────────────────┼───────────────────┤
// │ 程式碼行數  │ ~50 行           │ ~10 行             │
// │ 記憶體管理  │ 手動 malloc/free │ 自動               │
// │ 洩漏風險    │ 高               │ 極低               │
// │ 可讀性      │ 需理解指標操作   │ 意圖清晰            │
// │ 維護成本    │ 高               │ 低                 │
// └─────────────┴──────────────────┴───────────────────┘


int main() {
    std::cout << "====================================================" << std::endl;
    std::cout << "   第一課：STL 的歷史與設計哲學 - 總複習示範" << std::endl;
    std::cout << "====================================================" << std::endl;

    std::cout << "\n--- 示範一：C 風格 vs STL 風格 ---" << std::endl;
    demo_c_style_problem();
    demo_stl_style();

    std::cout << "\n--- 示範二：泛型排序（正交性原則）---" << std::endl;
    demo_generic_sort();

    std::cout << "\n--- 示範三：元件可組合性 ---" << std::endl;
    demo_composability();

    std::cout << "\n--- 示範四：效率優先（測量排序時間）---" << std::endl;
    demo_efficiency();

    std::cout << "\n====================================================" << std::endl;
    std::cout << "重點整理：" << std::endl;
    std::cout << "  1. STL = Standard Template Library（C++ 標準函式庫核心）" << std::endl;
    std::cout << "  2. 創造者：Alexander Stepanov，1998 年隨 C++98 正式發布" << std::endl;
    std::cout << "  3. 核心理念：泛型編程（一套程式碼，處理多種型別）" << std::endl;
    std::cout << "  4. 三大設計原則：" << std::endl;
    std::cout << "     ① 正交性：容器與演算法獨立，透過迭代器連接" << std::endl;
    std::cout << "     ② 效率優先：泛型不犧牲效能（編譯期展開）" << std::endl;
    std::cout << "     ③ 可組合性：元件可自由組合" << std::endl;
    std::cout << "  5. 相比 C 風格：更少程式碼、更安全、更易讀、更易維護" << std::endl;
    std::cout << "====================================================" << std::endl;

    return 0;
}
