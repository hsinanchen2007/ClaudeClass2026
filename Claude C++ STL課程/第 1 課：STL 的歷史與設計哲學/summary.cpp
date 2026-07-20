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

// =============================================================================
//  第一課 總複習  —  STL 的歷史與設計哲學
// =============================================================================
//
// 【主題資訊 Information】
//   STL = Standard Template Library，C++ 標準函式庫的核心。
//   四個支柱（本課只碰前三個，配置器留待後續課程）：
//     容器 Containers   <vector> <list> <deque> <set> <map> …
//     演算法 Algorithms <algorithm> <numeric>
//     迭代器 Iterators  <iterator>       —— 連接前兩者的通用介面
//     配置器 Allocators <memory>         —— 抽換記憶體來源
//   標準版本：STL 隨 C++98 正式進入標準；本檔用到的
//     lambda / auto / range-for / std::move 皆為 C++11，
//     CTAD 為 C++17（本檔未用）。
//   本檔用到的演算法與複雜度：
//     std::sort          O(N log N)   IntroSort，需 random access iterator
//     std::max_element   O(N)         回傳迭代器
//     std::accumulate    O(N)         <numeric>，回傳型別由 init 決定
//     std::copy_if       O(N)         C++11
//
// 【詳細解釋 Explanation】
//
// 【1. 正交性到底省下了什麼：M + N 而不是 M × N】
//   假設有 M 個容器、N 個演算法。若每個演算法都要為每種容器各寫一份，
//   總共要維護 M × N 份程式碼。STL 的做法是讓演算法只依賴「迭代器」
//   這個抽象介面，於是：
//     * 容器只需負責「提供符合某種分級的迭代器」  → M 份
//     * 演算法只需負責「操作那種分級的迭代器」    → N 份
//   總共 M + N。以 10 個容器 × 50 個演算法計算，是 60 對比 500。
//   更重要的是「開放擴充」：你自己寫的容器只要提供合法迭代器，
//   立刻就能用上全部 N 個標準演算法——不必修改任何標準函式庫程式碼。
//   這是 STL 最深刻的設計，也是它擊敗當年其他函式庫提案的原因。
//
// 【2. 迭代器分級：正交性是怎麼「安全地」達成的】
//   如果所有迭代器都長一樣，std::sort 就會被用在 std::list 上，
//   然後在執行期爆炸。STL 用「迭代器分級」在編譯期擋掉這件事：
//     Input        只能單向讀、一次性     （istream_iterator）
//     Output       只能單向寫             （back_insert_iterator）
//     Forward      可多次走訪、單向       （forward_list）
//     Bidirectional 可前可後              （list、set、map）
//     Random Access 可 it+n、it2-it1      （vector、deque、原生陣列）
//   後面的分級包含前面的能力。演算法在簽章與實作中要求最低分級，
//   於是「list 不能用 std::sort」是一個編譯期錯誤，不是執行期災難。
//   C++20 的 concepts 讓這個分級從「文件約定」升級成「編譯器檢查得到的約束」，
//   錯誤訊息也大幅改善。
//
// 【3. 效率優先：靜態多型 vs 動態多型】
//   STL 完全不用虛擬函式。它的多型發生在編譯期（template 具現化），
//   而不是執行期（vtable 查表）。這帶來三個後果：
//     ① 比較子、述詞可以被 inline，消除函式呼叫開銷
//     ② 沒有 vtable 指標，元素的記憶體佈局和原生陣列一樣緊湊
//     ③ 編譯器看得到完整內容，能做常數傳播、迴圈展開、向量化
//   代價是 code bloat（每個型別各一份）與編譯時間。
//   這正是「零成本抽象」的實際內容：不是沒有成本，
//   而是把成本從執行期搬到編譯期。
//
// 【4. 可組合性：三個獨立的擴充維度】
//   copy_if(src.begin(), src.end(), back_inserter(dst), pred) 這一行裡，
//   「來源」「目的地」「條件」是三個彼此獨立、都能抽換的維度。
//   換 pred 改變篩選邏輯、換 back_inserter 為 ostream_iterator 改變輸出目標、
//   換 copy_if 為 transform 改變運算——任意組合都成立，
//   因為它們之間唯一的契約就是迭代器的操作介面。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼 STL 的容器沒有共同基底類別
//     Java 的 Collection、C# 的 IEnumerable 都是介面繼承。STL 刻意不這麼做：
//       ① 虛擬呼叫無法 inline，違反效率優先原則。
//       ② 共同基底會強迫所有容器有相同的介面，但 vector 有 operator[]、
//          list 沒有；list 有 O(1) splice、vector 沒有。硬要統一，
//          結果不是介面過肥就是效能謊報（提供 O(n) 的 list::operator[]）。
//       ③ STL 選擇讓「介面差異」直接反映「複雜度差異」——
//          容器只提供它能高效做到的操作。這是一種誠實的設計。
//
// (B) 這課的三個原則其實互相牽制
//     正交性想要「越通用越好」，效率優先想要「越特化越好」。
//     STL 的解法是 tag dispatch：std::advance 對 random access iterator
//     用 it += n（O(1)），對其他 iterator 用迴圈（O(n)），
//     在編譯期依據 iterator_category 選擇實作。
//     介面維持一個，實作依能力特化——這就是兩個原則的和解點。
//
// (C) 為什麼 std::string 不完全算 STL 容器
//     std::string 有 begin()/end()，也能用大部分演算法，但它早於 STL 存在，
//     介面上帶著大量字串專屬的成員函式（find、substr、c_str）。
//     嚴格說它是「相容於 STL 的字串類別」，不是 STL 設計哲學的產物。
//     這也是為什麼它有 npos 這種哨兵值，而不是回傳 end()。
//
// 【注意事項 Pay Attention】
//   1. 「STL」與「C++ 標準函式庫」不是同義詞。標準函式庫還包含
//      iostream、字串、locale、執行緒等與 STL 設計哲學無關的部分。
//   2. 演算法對容器一無所知，所以「刪除元素」這件事演算法做不到——
//      std::remove 只是搬移，真正刪除必須由容器的 erase 完成
//      （erase-remove 慣用法，第 15 課詳述）。
//   3. 泛型的成本是真實的：編譯時間、程式碼體積、以及出錯時
//      深達數十層的 template 錯誤訊息。C++20 的 concepts 改善了第三項。
//   4. 本檔 demo_efficiency 的耗時輸出走 stderr，因為它每次執行都不同；
//      stdout 只保留可重現的內容。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】STL 的設計哲學
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. STL 為什麼要有「迭代器」這一層？直接讓演算法收容器不行嗎？
//     答：不行，會退回 M × N 的組合爆炸。迭代器是容器與演算法之間的
//         通用契約：容器只負責提供迭代器，演算法只操作迭代器，
//         兩邊互不知道對方存在，於是只需要 M + N 個元件。
//         更關鍵的是開放性——自訂容器只要提供合法迭代器，
//         立刻能用上全部標準演算法，不必改動標準函式庫。
//     追問：那為什麼不乾脆讓所有容器繼承同一個基底類別？
//         → 虛擬呼叫無法 inline（違反效率優先），而且會強迫
//           list 提供 O(n) 的 operator[] 這種「介面說得到、
//           複雜度做不到」的謊報。STL 選擇讓介面差異誠實反映複雜度差異。
//
// 🔥 Q2. 「零成本抽象（zero-overhead abstraction）」是什麼意思？
//        STL 真的零成本嗎？
//     答：Bjarne 的定義有兩句：① 你不用的東西，不用付錢；
//         ② 你用的東西，手寫也不會更好。
//         STL 在「執行期」確實接近零成本（template 編譯期展開、
//         比較子 inline、無 vtable）。但成本沒有消失，只是搬到編譯期：
//         程式碼膨脹、編譯時間變長、錯誤訊息難懂。
//         而且前提是最佳化有開啟——-O0 下抽象成本是真實存在的。
//     追問：什麼情況下 STL 反而比手寫慢？
//         → 把 lambda 包進 std::function（型別抹除，退回間接呼叫）；
//           對小型固定大小資料用 vector（多一次堆積配置，
//           該用 std::array）；反覆 reserve(size()+1) 破壞幾何成長。
//
// ⚠️ 陷阱. 既然 std::vector 和 std::list 都有 begin()/end()，
//         那所有標準演算法對兩者應該都能用——這個推論錯在哪？
//     答：錯在把「有迭代器」等同於「有同樣能力的迭代器」。
//         vector 提供 random access iterator，list 只提供 bidirectional。
//         std::sort 需要 it + n 與 it2 - it1 才能取中位數、切分區間，
//         所以 std::sort(list.begin(), list.end()) 是編譯期錯誤。
//         list 必須用成員函式 list::sort（節點接合的 merge sort）。
//         同理 std::nth_element、std::random_shuffle 也不能用在 list 上。
//     為什麼會錯：把迭代器想成「都是指標的同義詞」。
//         實際上迭代器是一組分級的概念，能力由弱到強分五級，
//         每個演算法都在簽章中要求某個最低分級。
//         「有 begin()」只保證它至少是 input/output 級。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <list>
#include <algorithm>
#include <numeric>
#include <iterator>
#include <chrono>
#include <string>
#include <climits>

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
// 兩個刻意的設計：
//   ① 測資用固定種子的 LCG 產生，不用 rand()。rand() 的序列是實作定義的，
//      換平台就對不上，教材的輸出必須可重現。
//   ② 耗時印到 stderr，不印 stdout。時間每次執行都不同，
//      不該出現在「預期輸出」裡。stdout 只留可重現的事實。
void demo_efficiency() {
    const int SIZE = 100000;  // 示範用，減小規模
    std::vector<int> data(SIZE);

    // 填充可重現的偽亂數（LCG，係數取自 Numerical Recipes）
    unsigned int state = 20260720u;
    for (int i = 0; i < SIZE; ++i) {
        state = state * 1664525u + 1013904223u;   // unsigned 溢位是良好定義的環繞
        data[i] = static_cast<int>(state >> 1);   // 取正數
    }

    // 測量 std::sort 的時間。用 steady_clock：它保證單調遞增，
    // 不會因為 NTP 校時或使用者改系統時間而量出負值。
    // （high_resolution_clock 可能只是 system_clock 的別名，標準未規定。）
    auto start = std::chrono::steady_clock::now();
    std::sort(data.begin(), data.end());
    auto end = std::chrono::steady_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cerr << "[計時 | 每次執行都不同，故不列入預期輸出] 排序 "
              << SIZE << " 個元素耗時: " << duration.count() << " 微秒" << std::endl;

    // stdout 只印可重現的內容
    std::cout << "排序 " << SIZE << " 個元素：結果已排序 = "
              << std::boolalpha << std::is_sorted(data.begin(), data.end()) << std::endl;
    std::cout << "（耗時已輸出到 stderr——它每次執行都不同，不適合當預期輸出）"
              << std::endl;
    // std::sort 使用 IntroSort，是目前最優秀的通用排序演算法之一
    // 效能幾乎等同於手寫的最優化排序
}

// ===== 重點八：迭代器分級 —— 正交性的編譯期護欄 =====
// vector 給 random access iterator，list 只給 bidirectional。
// std::sort 需要 it+n / it2-it1，所以對 list 用 std::sort 是編譯期錯誤。
// 這不是缺陷，是 STL 刻意用型別系統擋掉「複雜度謊報」。
void demo_iterator_category() {
    std::cout << "\n===== 重點八：迭代器分級（誰能用 std::sort）=====\n";

    std::vector<int> v = {5, 3, 1, 4, 2};
    std::list<int>   l = {5, 3, 1, 4, 2};

    std::sort(v.begin(), v.end());          // OK：random access iterator
    // std::sort(l.begin(), l.end());       // 編譯錯誤！list 只有 bidirectional
    l.sort();                               // list 必須用自己的成員函式

    std::cout << "vector 用 std::sort  : ";
    for (int x : v) std::cout << x << " ";
    std::cout << "\n";
    std::cout << "list   用 list::sort : ";
    for (int x : l) std::cout << x << " ";
    std::cout << "\n";

    // 但「只需要往前走」的演算法，兩者都能用 —— 這就是正交性的收益
    auto vit = std::find(v.begin(), v.end(), 4);
    auto lit = std::find(l.begin(), l.end(), 4);
    std::cout << "同一個 std::find 兩邊都能用: vector 找到="
              << (vit != v.end()) << ", list 找到=" << (lit != l.end()) << "\n";

    // std::distance 對兩者都能用，但複雜度不同（tag dispatch）
    std::cout << "std::distance(begin, 找到的位置): vector="
              << std::distance(v.begin(), vit) << "（O(1)）, list="
              << std::distance(l.begin(), lit) << "（O(n)）\n";
    std::cout << "介面一樣，複雜度不同 —— STL 用 tag dispatch 在編譯期選實作\n";
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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 349. Intersection of Two Arrays
//   題目：給兩個整數陣列，回傳它們的交集（結果中每個元素唯一，順序不限）。
//   為什麼用到本主題：這題是「正交性 + 可組合性」的完美展示——
//     解法把四個彼此獨立的標準演算法串起來，中間全靠迭代器溝通：
//       std::sort            排序（兩邊各一次）
//       std::unique          把重複的搬到後面
//       erase                真正刪掉（演算法做不到，只有容器能做）
//       std::set_intersection 求交集，寫進 back_inserter
//     沒有任何一步需要為「這是 vector」寫特別的程式碼。
//   複雜度：O(N log N + M log M)，由兩次排序主導。
// -----------------------------------------------------------------------------
std::vector<int> intersection(std::vector<int> a, std::vector<int> b) {
    auto dedup = [](std::vector<int>& v) {
        std::sort(v.begin(), v.end());
        // unique 只是把重複元素往後搬，回傳新的邏輯結尾；
        // 真正刪除必須由容器的 erase 完成 —— 演算法碰不到容器大小。
        v.erase(std::unique(v.begin(), v.end()), v.end());
    };
    dedup(a);
    dedup(b);

    std::vector<int> out;
    std::set_intersection(a.begin(), a.end(),
                          b.begin(), b.end(),
                          std::back_inserter(out));
    return out;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】伺服器日誌的每日統計報表
//   情境：把一天的請求記錄（狀態碼 + 回應位元組數）跑成一份摘要：
//         總流量、最大單筆回應、成功率、以及所有出現過的錯誤碼清單。
//   為什麼用到本主題：這是 STL 三原則在真實工作中的樣子——
//     一份資料、四個標準演算法（accumulate / max_element / count_if /
//     copy_if），完全不必寫任何一個手動迴圈。
//   踩雷提醒：總流量用 accumulate 時 init 一定要傳 0LL。
//     單日流量輕易就會超過 INT_MAX（約 2.1 GB），
//     傳 0 會讓累加器是 int → 有號溢位 → 未定義行為。
// -----------------------------------------------------------------------------
struct Request {
    int  status;      // HTTP 狀態碼
    int  bytes;       // 回應大小
};

struct DailyReport {
    long long totalBytes;
    int       largestResponse;
    long long okCount;
    std::vector<int> errorCodes;   // 4xx / 5xx，去重後排序
};

DailyReport buildReport(const std::vector<Request>& reqs) {
    DailyReport r{};

    // 總流量：init 必須是 0LL，否則累加器是 int，單日流量很容易溢位
    r.totalBytes = std::accumulate(
        reqs.begin(), reqs.end(), 0LL,
        [](long long acc, const Request& q) { return acc + q.bytes; });

    // 最大單筆：max_element 回傳迭代器，空區間回傳 end()
    auto it = std::max_element(
        reqs.begin(), reqs.end(),
        [](const Request& x, const Request& y) { return x.bytes < y.bytes; });
    r.largestResponse = (it == reqs.end()) ? 0 : it->bytes;

    // 成功數：count_if
    r.okCount = std::count_if(reqs.begin(), reqs.end(),
                              [](const Request& q) { return q.status < 400; });

    // 錯誤碼清單：copy_if 篩出來，再排序去重
    std::vector<int> codes;
    std::transform(reqs.begin(), reqs.end(), std::back_inserter(codes),
                   [](const Request& q) { return q.status; });
    codes.erase(std::remove_if(codes.begin(), codes.end(),
                               [](int s) { return s < 400; }),
                codes.end());
    std::sort(codes.begin(), codes.end());
    codes.erase(std::unique(codes.begin(), codes.end()), codes.end());
    r.errorCodes = codes;

    return r;
}

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

    demo_iterator_category();

    std::cout << "\n--- 示範五：LeetCode 349 Intersection of Two Arrays ---" << std::endl;
    {
        auto show = [](const char* label, const std::vector<int>& v) {
            std::cout << label << "[";
            for (size_t i = 0; i < v.size(); ++i) std::cout << (i ? "," : "") << v[i];
            std::cout << "]\n";
        };
        show("[1,2,2,1] ∩ [2,2]       = ", intersection({1, 2, 2, 1}, {2, 2}));
        show("[4,9,5] ∩ [9,4,9,8,4]   = ", intersection({4, 9, 5}, {9, 4, 9, 8, 4}));
        show("[1,2,3] ∩ [4,5,6]       = ", intersection({1, 2, 3}, {4, 5, 6}));
    }

    std::cout << "\n--- 示範六：日常實務 — 伺服器日誌每日報表 ---" << std::endl;
    {
        std::vector<Request> reqs;
        // 模擬 30 萬筆請求，平均每筆 8 KB → 總流量約 2.4 GB，超過 INT_MAX
        unsigned int st = 7u;
        for (int i = 0; i < 300000; ++i) {
            st = st * 1664525u + 1013904223u;
            unsigned int r = st % 100;
            int status = (r < 92) ? 200 : (r < 96 ? 404 : (r < 99 ? 500 : 503));
            reqs.push_back({status, 4096 + static_cast<int>(st % 8192)});
        }

        DailyReport rep = buildReport(reqs);
        std::cout << "請求總數      : " << reqs.size() << "\n";
        std::cout << "總流量(bytes) : " << rep.totalBytes << "\n";
        std::cout << "是否超過 INT_MAX(" << INT_MAX << "): "
                  << std::boolalpha << (rep.totalBytes > INT_MAX) << "\n";
        std::cout << "  → 這正是 accumulate 的 init 必須傳 0LL 的原因\n";
        std::cout << "最大單筆回應  : " << rep.largestResponse << " bytes\n";
        std::cout << "成功(<400)筆數: " << rep.okCount << "\n";
        std::cout << "出現過的錯誤碼: ";
        for (int c : rep.errorCodes) std::cout << c << " ";
        std::cout << "\n";
    }

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

// 編譯: g++ -std=c++17 -Wall -Wextra summary.cpp -o summary

// === 預期輸出 ===
// ====================================================
//    第一課：STL 的歷史與設計哲學 - 總複習示範
// ====================================================
//
// --- 示範一：C 風格 vs STL 風格 ---
// [C 風格] 需要手動管理記憶體，請對比下面的 STL 風格
// [STL 風格] 排序後: 1 2 3 4 5 6 7 8 9
// 最大值: 9, 總和: 45
//
// --- 示範二：泛型排序（正交性原則）---
// 整數排序: 1 1 3 4 5
// 浮點數排序: 1.41 2.72 3.14
// 字串排序: apple banana cherry
//
// --- 示範三：元件可組合性 ---
// 從 1~10 中只複製偶數: 2 4 6 8 10
//
// --- 示範四：效率優先（測量排序時間）---
// 排序 100000 個元素：結果已排序 = true
// （耗時已輸出到 stderr——它每次執行都不同，不適合當預期輸出）
//
// ===== 重點八：迭代器分級（誰能用 std::sort）=====
// vector 用 std::sort  : 1 2 3 4 5
// list   用 list::sort : 1 2 3 4 5
// 同一個 std::find 兩邊都能用: vector 找到=true, list 找到=true
// std::distance(begin, 找到的位置): vector=3（O(1)）, list=3（O(n)）
// 介面一樣，複雜度不同 —— STL 用 tag dispatch 在編譯期選實作
//
// --- 示範五：LeetCode 349 Intersection of Two Arrays ---
// [1,2,2,1] ∩ [2,2]       = [2]
// [4,9,5] ∩ [9,4,9,8,4]   = [4,9]
// [1,2,3] ∩ [4,5,6]       = []
//
// --- 示範六：日常實務 — 伺服器日誌每日報表 ---
// 請求總數      : 300000
// 總流量(bytes) : 2457243184
// 是否超過 INT_MAX(2147483647): true
//   → 這正是 accumulate 的 init 必須傳 0LL 的原因
// 最大單筆回應  : 12287 bytes
// 成功(<400)筆數: 275775
// 出現過的錯誤碼: 404 500 503
//
// ====================================================
// 重點整理：
//   1. STL = Standard Template Library（C++ 標準函式庫核心）
//   2. 創造者：Alexander Stepanov，1998 年隨 C++98 正式發布
//   3. 核心理念：泛型編程（一套程式碼，處理多種型別）
//   4. 三大設計原則：
//      ① 正交性：容器與演算法獨立，透過迭代器連接
//      ② 效率優先：泛型不犧牲效能（編譯期展開）
//      ③ 可組合性：元件可自由組合
//   5. 相比 C 風格：更少程式碼、更安全、更易讀、更易維護
// ====================================================
