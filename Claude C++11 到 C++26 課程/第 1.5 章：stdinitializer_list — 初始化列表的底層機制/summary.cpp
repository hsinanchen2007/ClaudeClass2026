// ============================================================================
// 第 1.5 章：std::initializer_list — 初始化列表的底層機制（總複習）
// ============================================================================
//
// 【主題資訊 Information】
//   型別    ：template<class T> class std::initializer_list<T>;
//   介面    ：size() / begin() / end()   — 只有這三個成員函式，沒有 operator[]
//   標準版本：std::initializer_list          C++11
//             constexpr 化的成員函式          C++14
//             auto x{42} 改推導為 int         C++17（N3922）
//   標頭檔  ：<initializer_list>
//   複雜度  ：size()/begin()/end() 皆為 O(1)；走訪為 O(N)
//             建構本身零成本（僅記錄指標與長度），但元素複製為 O(N)
//   本檔宣告的標準：C++11
//
// 【詳細解釋 Explanation】
//
// 【1. 它到底是什麼：一個「view」，不是容器】
//   很多人以為 initializer_list 是個輕量容器，其實它「不擁有」任何資料。
//   它的內部結構實質上只有兩個欄位：
//       struct initializer_list {
//           const T* m_begin;   // 指向編譯器產生的後備陣列
//           size_t   m_size;    // 元素個數
//       };
//   真正存放資料的是編譯器在背後產生的「後備陣列（backing array）」：
//       std::initializer_list<int> il = {1, 2, 3};
//   編譯器大致會產生：
//       const int __backing[3] = {1, 2, 3};
//       initializer_list<int> il(__backing, 3);
//   所以 initializer_list 是那個陣列的「view」——這個事實決定了它所有的特性與陷阱。
//
// 【2. 為什麼元素是 const】
//   begin() 回傳的是 const T*，因此元素無法修改，也無法被移動（move）。
//   這是刻意的設計：後備陣列可能被編譯器放在唯讀記憶體區段，
//   而且同一份 {1,2,3} 在多處使用時可能被合併共用。
//   實務後果（很重要）：
//       std::vector<std::unique_ptr<int>> v{ std::make_unique<int>(1) };  // ❌ 編譯失敗
//   因為 unique_ptr 不可複製，而 initializer_list 只能複製、不能移動元素。
//   要放 move-only 型別，只能改用 push_back(std::move(...)) 或 emplace_back。
//
// 【3. 生命週期：後備陣列跟著 initializer_list 物件走】
//   後備陣列的生命週期等同於該 initializer_list 物件本身。
//   因此下面這種寫法是懸空的：
//       std::initializer_list<int> makeList() { return {1, 2, 3}; }  // ❌ 危險
//   函式回傳後，後備陣列已銷毀，回傳的 view 指向失效記憶體，
//   後續使用屬未定義行為（GCC 會給 -Winit-list-lifetime 警告）。
//   安全的作法是回傳真正擁有資料的容器（例如 std::vector<int>）。
//
// 【4. 建構子重載的絕對優先權（最常見的陷阱）】
//   只要類別有 initializer_list 建構子且「可行」，用 {} 時它就必勝：
//       std::vector<int> v1(5, 10);   // 5 個 10
//       std::vector<int> v2{5, 10};   // 2 個元素 [5, 10]
//   甚至 NumberList{5, 10} 也會選 initializer_list 而非 (size, value)。
//   想呼叫其他建構子，唯一的辦法是改用 ()。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼沒有 operator[]
//     標準只給了 size()/begin()/end() 三個成員。要隨機存取得寫 *(il.begin() + i)。
//     設計意圖是強調它「只是一段連續唯讀資料的 view」，
//     而不是鼓勵你把它當容器長期持有 —— 它本來就只該在初始化當下用完即丟。
//
// (B) 它與可變參數模板（variadic template）的分工
//     initializer_list：元素型別必須「同一種」（或能隱式轉成同一種），
//                       但語法極簡潔，適合 print({1,2,3}) 這種同質列表。
//     variadic template：元素型別可以完全不同，且能完美轉發（保留 move 語意），
//                       適合 emplace_back(args...) 這種需要原地建構的場合。
//     選擇準則：同質資料且不需 move → initializer_list；異質或需轉發 → variadic。
//
// (C) auto 與 {} 的糾纏，以及 C++17 的修正（N3922）
//       auto a = {1, 2, 3};   // C++11 起皆為 initializer_list<int>
//       auto b{42};           // C++11/14：initializer_list<int>
//                             // C++17 起：int          ← 行為改變
//       auto c = {42};        // 一律為 initializer_list<int>（含 C++17）
//     C++17 的規則是：直接列表初始化（無 =）且只有單一元素時推導為該元素型別；
//     複製列表初始化（有 =）仍推導為 initializer_list。
//     本檔宣告 C++11，故以 C++11 規則說明；此差異是面試高頻題。
//
// (D) 效能特性：不是零成本
//     initializer_list 物件本身很輕（僅指標 + 長度），
//     但把它交給容器建構子時，元素會被「逐一複製」進容器。
//     對 std::string 這類元素，vector<string> v{"a","b","c"} 會發生 3 次字串複製。
//     大量資料或 move-only 型別請改用 reserve + emplace_back。
//
// 【注意事項 Pay Attention】
//   1. 元素為 const，只能複製、不能移動；move-only 型別（unique_ptr）無法放入。
//   2. 絕不要回傳或長期儲存 initializer_list —— 後備陣列會先消失，形成懸空 view。
//      其後果是未定義行為，不會有固定的錯誤現象。
//   3. 沒有 operator[]，隨機存取要寫 *(il.begin() + i)。
//   4. 有 initializer_list 建構子時，{} 一律優先選它；要用別的建構子請改 ()。
//   5. auto x{42} 在 C++11/14 是 initializer_list<int>，C++17 起是 int（N3922）。
//   6. 元素會被逐一複製進容器，對大型元素不是零成本。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::initializer_list
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::initializer_list 內部長什麼樣子？它擁有資料嗎？
//     答：不擁有。它實質上只有 {const T* begin, size_t size} 兩個欄位，
//         是編譯器產生的「後備陣列」的唯讀 view。
//         資料真正存放在那個後備陣列，而陣列的生命週期綁在該 list 物件上。
//     追問：所以為什麼不能 return {1,2,3};（回傳型別為 initializer_list）？
//         → 因為函式返回時後備陣列已銷毀，回傳的 view 立刻懸空，使用即 UB。
//
// 🔥 Q2. 為什麼 std::vector<std::unique_ptr<int>> v{std::make_unique<int>(1)}; 編譯失敗？
//     答：initializer_list 的元素型別是 const T，只能複製、不能移動。
//         unique_ptr 不可複製，所以無法從 list 複製進 vector。
//         正確作法：v.push_back(std::make_unique<int>(1)); 或 emplace_back。
//     追問：那 vector<string> 可以嗎？→ 可以，但會發生逐一「複製」而非移動，
//         資料量大時應改用 reserve + emplace_back 以避免多餘複製。
//
// 🔥 Q3. vector<int> v(5, 10) 與 v{5, 10} 差在哪？為什麼標準要這樣設計？
//     答：前者呼叫 (count, value) → 5 個 10；後者因 initializer_list 建構子
//         擁有絕對優先權 → 2 個元素 [5, 10]。
//         設計理由是讓 {} 的「列表語意」永遠可預測：看到 {} 就是在列元素。
//     追問：有沒有辦法讓 {} 呼叫到 (count,value)？→ 沒有，只能改用 ()。
//
// ⚠️ 陷阱 1. auto x{42}; 推導出什麼型別？
//     答：要看標準版本。C++11/14 是 std::initializer_list<int>；
//         C++17 起（N3922）改為 int。而 auto y = {42}; 在所有版本都是
//         initializer_list<int>（因為那是複製列表初始化）。
//     為什麼會錯：以為 auto 的推導規則不受標準版本影響。
//         這是 C++ 少數「同一段程式碼在不同標準下型別不同」的地方，
//         也是為什麼本課程堅持用 -std=c++NN -pedantic-errors 逐版驗證。
//
// ⚠️ 陷阱 2. 把 initializer_list 存成成員變數，之後再用，可以嗎？
//     答：不可以。後備陣列的生命週期只到「該 list 物件」結束為止，
//         若 list 來自建構子的臨時引數，函式一返回資料就沒了，
//         成員裡留下的是懸空指標，之後讀取屬未定義行為。
//     為什麼會錯：把 initializer_list 當成「輕量容器」，以為它自己持有資料。
//         正確作法是在建構子裡把內容複製進真正的容器（如 std::vector）。
// ═══════════════════════════════════════════════════════════════════════════
//
// 編譯指令: g++ -std=c++11 -Wall -Wextra -o summary summary.cpp
//
// 本檔案涵蓋 std::initializer_list 的所有核心觀念：
//   1.  什麼是 std::initializer_list？基本定義與用法
//   2.  底層結構：輕量級包裝器（指標 + 大小）
//   3.  元素的 const 唯讀特性
//   4.  自訂類別搭配 initializer_list 建構子
//   5.  函式參數使用 initializer_list
//   6.  泛型（模板）版本的 initializer_list 函式
//   7.  建構子多載的優先順序規則（最重要的陷阱）
//   8.  標準容器如何使用 initializer_list
//   9.  實用類別範例：Config 類別
//  10.  空的 initializer_list 行為
//  11.  auto 推導規則與 C++17 的變化
//  12.  生命週期注意事項與常見陷阱
// ============================================================================

#include <iostream>
#include <initializer_list>  // std::initializer_list 的標頭檔
#include <vector>
#include <string>
#include <algorithm>         // std::copy, std::max_element, std::min_element
#include <numeric>           // std::accumulate
#include <cstddef>           // std::size_t
#include <typeinfo>          // typeid
#include <memory>            // std::unique_ptr

// ============================================================================
// 第一節：什麼是 std::initializer_list？
// ============================================================================
//
// std::initializer_list<T> 是 C++11 引入的一個輕量級類別模板。
// 它的作用是讓我們用大括號 {} 語法來初始化物件或傳遞一組同型別的值。
//
// 底層原理：
//   - 編譯器遇到 {1, 2, 3} 這樣的花括號列表時，
//     會在「呼叫者的堆疊空間」建立一個臨時的 const 陣列 (例如 const int[3])
//   - std::initializer_list 本身只是一個「薄包裝器 (thin wrapper)」，
//     內部僅儲存兩個東西：
//       (a) 指向底層陣列首元素的 const 指標
//       (b) 陣列的元素個數 (size)
//   - 因此複製一個 initializer_list 是極其廉價的操作：
//     只複製指標和大小，不會複製底層元素
//
// 關鍵特性：
//   - 元素是 const 的，無法透過 initializer_list 修改
//   - 支援 begin(), end(), size() 成員函式
//   - 支援 range-based for 迴圈
//   - 底層陣列的生命週期等同於 initializer_list 物件本身
// ============================================================================


// ============================================================================
// 第二節：自訂類別使用 initializer_list 建構子
// ============================================================================
//
// 要讓自己的類別支援 {} 初始化語法，
// 只需要新增一個參數型別為 std::initializer_list<T> 的建構子。
//
// 重要觀念：
//   - initializer_list 建構子在多載解析中具有「最高優先權」
//     （當使用 {} 語法時）
//   - 如果同時有 IntArray(int, int) 和 IntArray(initializer_list<int>)，
//     用 IntArray{1, 2} 會呼叫 initializer_list 版本
//   - 要呼叫其他建構子，須使用 () 而非 {}
// ============================================================================

class IntArray
{
private:
    int* data_;         // 動態分配的陣列指標
    std::size_t size_;  // 元素個數

public:
    // ----- initializer_list 建構子 -----
    // 接受花括號列表，例如 IntArray arr{1, 2, 3, 4, 5};
    // init 是一個輕量物件，內部只有指標和大小
    IntArray(std::initializer_list<int> init)
        : data_{new int[init.size()]}   // 根據列表大小分配記憶體
        , size_{init.size()}            // 記錄元素個數
    {
        std::cout << "[IntArray] initializer_list 建構子被呼叫, size = "
                  << size_ << "\n";

        // 使用 std::copy 將 initializer_list 的元素複製到動態陣列中
        // init.begin() 回傳 const int*，init.end() 回傳尾後指標
        std::copy(init.begin(), init.end(), data_);
    }

    // ----- 指定大小的建構子 -----
    // 使用 explicit 防止隱式轉換，例如 IntArray arr = 10; 會被禁止
    // 要呼叫此建構子，必須用 () 語法：IntArray arr(10);
    // 若用 IntArray arr{10}; 則會匹配 initializer_list 建構子（建立含一個元素 10 的陣列）
    explicit IntArray(std::size_t size)
        : data_{new int[size]{}}  // {} 代表零初始化（所有元素設為 0）
        , size_{size}
    {
        std::cout << "[IntArray] size 建構子被呼叫, size = " << size << "\n";
    }

    // ----- 解構子 -----
    ~IntArray()
    {
        delete[] data_;  // 釋放動態分配的記憶體
    }

    // 禁止複製（簡化範例，避免雙重釋放問題）
    IntArray(const IntArray&) = delete;
    IntArray& operator=(const IntArray&) = delete;

    // ----- 成員存取 -----
    std::size_t size() const { return size_; }
    int& operator[](std::size_t index) { return data_[index]; }
    const int& operator[](std::size_t index) const { return data_[index]; }

    // ----- 迭代器支援 -----
    // 提供 begin() 和 end() 讓此類別可用於 range-based for 迴圈
    int* begin() { return data_; }
    int* end() { return data_ + size_; }
    const int* begin() const { return data_; }
    const int* end() const { return data_ + size_; }

    // ----- 列印輔助函式 -----
    void print(const std::string& name) const
    {
        std::cout << name << ": [";
        for (std::size_t i = 0; i < size_; ++i)
        {
            if (i > 0) std::cout << ", ";
            std::cout << data_[i];
        }
        std::cout << "]\n";
    }
};


// ============================================================================
// 第三節：函式參數使用 initializer_list
// ============================================================================
//
// initializer_list 不僅可用於建構子，也可用於一般函式的參數。
// 這讓函式能接受任意數量的同型別引數，語法十分簡潔。
//
// 呼叫方式：printValues({1, 2, 3, 4, 5});
// 注意外層的 {} 是必要的，它告訴編譯器建立 initializer_list
// ============================================================================

// 列印所有值
void printValues(std::initializer_list<int> values)
{
    std::cout << "printValues: {";
    bool first = true;
    for (int v : values)  // range-based for，因為 initializer_list 提供 begin()/end()
    {
        if (!first) std::cout << ", ";
        std::cout << v;
        first = false;
    }
    std::cout << "}\n";
}

// 求和：使用 std::accumulate 搭配 initializer_list
int sum(std::initializer_list<int> values)
{
    // accumulate(begin, end, 初始值) 回傳所有元素的總和
    return std::accumulate(values.begin(), values.end(), 0);
}

// 求最大值：使用 std::max_element
int findMax(std::initializer_list<int> values)
{
    if (values.size() == 0)
    {
        throw std::invalid_argument("Empty list");  // 空列表沒有最大值
    }
    // max_element 回傳指向最大元素的迭代器，需要解引用 * 取值
    return *std::max_element(values.begin(), values.end());
}

// 求平均值：使用 double 型別的 initializer_list
double average(std::initializer_list<double> values)
{
    if (values.size() == 0) return 0.0;
    double total = std::accumulate(values.begin(), values.end(), 0.0);
    return total / values.size();  // 注意 size() 回傳 size_t，會自動轉為 double
}


// ============================================================================
// 第四節：泛型（模板）版本的 initializer_list 函式
// ============================================================================
//
// 結合 template 和 initializer_list，可以寫出適用於任何型別的函式。
// 編譯器會根據 {} 內的元素型別自動推導 T。
//
// 注意：如果元素型別不一致（例如 {1, 2.0}），
// 編譯器無法推導，需要明確指定型別，例如 findMin<double>({1, 2.0})
// 或者對於字串字面值 "hello"，T 會推導為 const char*，
// 若要使用 std::string，需要明確指定：findMin<std::string>({"hello", "world"})
// ============================================================================

template<typename T>
T findMin(std::initializer_list<T> values)
{
    if (values.size() == 0)
    {
        throw std::invalid_argument("Empty list");
    }
    return *std::min_element(values.begin(), values.end());
}

template<typename T>
void printAll(std::initializer_list<T> values)
{
    // typeid(T).name() 可以取得型別名稱（不同編譯器格式不同）
    std::cout << "printAll<" << typeid(T).name() << ">: ";
    for (const T& v : values)  // 用 const T& 避免不必要的複製
    {
        std::cout << v << " ";
    }
    std::cout << "\n";
}


// ============================================================================
// 第五節：建構子多載的優先順序規則（重點陷阱）
// ============================================================================
//
// 這是 initializer_list 最容易出錯的地方！
//
// 規則：
//   當使用 {} 初始化時，如果類別同時有：
//     (a) 一般建構子 (例如 Ambiguous(int, int))
//     (b) initializer_list 建構子 (例如 Ambiguous(initializer_list<int>))
//   編譯器「優先選擇」initializer_list 建構子。
//
// 範例：
//   Ambiguous a1(5, 10);   // --> 呼叫 Ambiguous(int, int)         用 ()
//   Ambiguous a2{5, 10};   // --> 呼叫 Ambiguous(initializer_list) 用 {}
//   Ambiguous a3{5};       // --> 呼叫 Ambiguous(initializer_list) 用 {}
//
// 實務影響（以 std::vector 為例）：
//   std::vector<int> v1(5, 10);   // 5 個元素，每個值為 10 → {10,10,10,10,10}
//   std::vector<int> v2{5, 10};   // 2 個元素，值為 5 和 10 → {5, 10}
//   這兩者結果完全不同！
// ============================================================================

class Ambiguous
{
public:
    // 一般建構子：接受兩個 int 參數
    Ambiguous(int a, int b)
    {
        std::cout << "Ambiguous(int, int): a=" << a << ", b=" << b << "\n";
    }

    // initializer_list 建構子
    Ambiguous(std::initializer_list<int> init)
    {
        std::cout << "Ambiguous(initializer_list<int>): size="
                  << init.size() << ", values = {";
        bool first = true;
        for (int v : init)
        {
            if (!first) std::cout << ", ";
            std::cout << v;
            first = false;
        }
        std::cout << "}\n";
    }
};


// ============================================================================
// 第六節：實用類別範例 — Config 類別
// ============================================================================
//
// 展示如何在實際應用中使用 initializer_list：
//   - 建構子接受 initializer_list 以便初始化
//   - 成員函式也可以接受 initializer_list
//   - 搭配 std::vector 的 assign() 和 insert() 方法
//
// 此模式在實際專案中非常常見，例如設定檔、命令列選項等。
// ============================================================================

class Config
{
private:
    std::vector<std::string> options_;  // 儲存設定選項

public:
    Config() = default;

    // 建構子：接受字串列表來初始化選項
    // 例如：Config cfg{"verbose", "debug", "color"};
    Config(std::initializer_list<std::string> opts)
        : options_(opts)  // vector 本身也支援 initializer_list 建構子
    {
    }

    // 設定方法：替換所有選項
    // assign() 會清除舊內容並填入新值
    void setOptions(std::initializer_list<std::string> opts)
    {
        options_.assign(opts.begin(), opts.end());
    }

    // 新增方法：在尾端追加多個選項
    // insert() 在指定位置插入 [begin, end) 範圍的元素
    void addOptions(std::initializer_list<std::string> opts)
    {
        options_.insert(options_.end(), opts.begin(), opts.end());
    }

    // 列印所有選項
    void print() const
    {
        std::cout << "Config options:\n";
        for (const auto& opt : options_)
        {
            std::cout << "  - " << opt << "\n";
        }
    }
};


// ============================================================================
// 【LeetCode 實戰範例】—— 本章刻意從缺，理由如下
// ============================================================================
// std::initializer_list 是「初始化語法的底層機制」，屬於語言/程式庫設計層面，
// 不是演算法。LeetCode 題目給的是已建好的 vector 參數，
// 解題過程從頭到尾不會用到 initializer_list 的任何特性
// （測資寫成 {1,2,3} 那是呼叫端的事，不是解法的一部分）。
// 硬掛一題只會讓讀者誤以為兩者有關聯，因此本章不提供 LeetCode 範例，
// 改以三個真實會用到 initializer_list 設計決策的工程情境示範。
// ============================================================================


// ----------------------------------------------------------------------------
// 【日常實務範例 1】固定維度矩陣類別 — initializer_list 讓初始化長得像數學
//   情境：影像處理／幾何運算要寫 3x3 卷積核或變換矩陣。
//   為什麼用到本主題：
//     巢狀 initializer_list 讓程式碼裡的矩陣「排版就是它的形狀」，
//     比 push_back 逐格填入好讀太多，也不容易填錯位置。
//   關鍵：在建構子裡「複製進」自己的 std::vector，
//         絕不保存 initializer_list 本身（那會懸空）。
// ----------------------------------------------------------------------------
class Matrix3x3
{
    std::vector<double> m_data;   // 真正擁有資料的是這個 vector（9 個元素）

public:
    // 接受巢狀初始化列表：{{1,0,0},{0,1,0},{0,0,1}}
    Matrix3x3(std::initializer_list<std::initializer_list<double>> rows)
        : m_data(9, 0.0)
    {
        std::size_t r = 0;
        for (std::initializer_list<std::initializer_list<double>>::const_iterator
                 it = rows.begin(); it != rows.end() && r < 3; ++it, ++r) {
            std::size_t c = 0;
            for (std::initializer_list<double>::const_iterator jt = it->begin();
                 jt != it->end() && c < 3; ++jt, ++c) {
                m_data[r * 3 + c] = *jt;   // 複製進自己的 vector
            }
        }
        // 注意：離開建構子後 rows 就失效了，
        //       所以絕不能寫 m_rows = rows; 把 list 存起來。
    }

    double at(std::size_t r, std::size_t c) const { return m_data[r * 3 + c]; }

    void print(const char* label) const
    {
        std::cout << "  " << label << ":\n";
        for (std::size_t r = 0; r < 3; ++r) {
            std::cout << "    [ ";
            for (std::size_t c = 0; c < 3; ++c)
                std::cout << at(r, c) << " ";
            std::cout << "]\n";
        }
    }
};

// ----------------------------------------------------------------------------
// 【日常實務範例 2】白名單／允許值集合的簡潔查詢介面
//   情境：驗證使用者輸入是否落在允許集合內（HTTP 方法、狀態碼、副檔名…）。
//   為什麼用到本主題：
//     用 initializer_list 當參數型別，呼叫端可以直接寫
//         isAllowed(method, {"GET", "POST", "PUT"})
//     不必先建一個 vector，語法乾淨且沒有多餘的堆積配置
//     （後備陣列通常配置在堆疊上）。
//   注意：參數只在函式執行期間有效，函式內用完即可，不可存起來。
// ----------------------------------------------------------------------------
bool isAllowed(const std::string& value, std::initializer_list<const char*> allowed)
{
    for (std::initializer_list<const char*>::const_iterator it = allowed.begin();
         it != allowed.end(); ++it) {
        if (value == *it) return true;
    }
    return false;
}

// ----------------------------------------------------------------------------
// 【日常實務範例 3】為什麼放不進 move-only 型別（實務上一定會撞到）
//   情境：想用 {} 初始化一個裝 unique_ptr 的容器（例如外掛/處理器清單）。
//   結論：做不到 —— initializer_list 的元素是 const，只能複製不能移動，
//         而 unique_ptr 不可複製。這裡示範「正確的替代寫法」。
// ----------------------------------------------------------------------------
struct Handler {
    std::string name;
    explicit Handler(const std::string& n) : name(n) {}
};

// ❌ 這樣寫編譯不過（示意，故不放進實際程式碼）：
//      std::vector<std::unique_ptr<Handler>> hs{
//          std::unique_ptr<Handler>(new Handler("auth"))
//      };
//    錯誤訊息大意：use of deleted function 'unique_ptr(const unique_ptr&)'
//
// ✅ 正確作法：先建立容器，再逐一 push_back(std::move(...))
std::vector<std::unique_ptr<Handler> > buildHandlers()
{
    std::vector<std::unique_ptr<Handler> > hs;
    hs.reserve(3);                                        // 先配置，避免多次搬移
    hs.push_back(std::unique_ptr<Handler>(new Handler("auth")));
    hs.push_back(std::unique_ptr<Handler>(new Handler("ratelimit")));
    hs.push_back(std::unique_ptr<Handler>(new Handler("router")));
    return hs;                                            // 回傳擁有資料的容器
}


// ============================================================================
// 主程式：展示所有觀念的完整範例
// ============================================================================

int main()
{
    std::cout << std::boolalpha;  // 讓 bool 值印出 true/false 而非 1/0

    // ====================================================================
    // 示範 1：std::initializer_list 基本使用
    // ====================================================================
    // 用 {} 語法建立一個 initializer_list<int>
    // 編譯器會在堆疊上建立 const int[5] = {10, 20, 30, 40, 50}
    // 然後 list1 內部只存一個指標指向這個陣列，以及 size = 5
    // ====================================================================
    std::cout << "===== 1. std::initializer_list 基本使用 =====\n";

    std::initializer_list<int> list1 = {10, 20, 30, 40, 50};

    std::cout << "list1 size: " << list1.size() << "\n";       // 輸出: 5
    std::cout << "list1 elements: ";
    for (int n : list1)  // range-based for 迴圈遍歷所有元素
    {
        std::cout << n << " ";
    }
    std::cout << "\n";

    // begin() 回傳指向第一個元素的 const 指標
    // end() 回傳指向最後一個元素「之後」的位置（尾後迭代器）
    std::cout << "first element: " << *list1.begin() << "\n";        // 10
    std::cout << "last element: " << *(list1.end() - 1) << "\n\n";  // 50

    // ====================================================================
    // 示範 2：輕量複製特性（淺複製）
    // ====================================================================
    // 複製 initializer_list 不會複製底層陣列！
    // 只是複製內部的指標和 size 值。
    // 因此 list1 和 list2 指向「相同的底層記憶體位址」。
    // 這使得傳值傳遞 initializer_list 的成本極低。
    // ====================================================================
    std::cout << "===== 2. 輕量複製特性 =====\n";

    std::initializer_list<int> list2 = list1;  // 淺複製：只複製指標和大小

    // 印出兩者的 begin() 位址來驗證它們指向同一塊記憶體
    std::cout << "list1.begin(): " << static_cast<const void*>(list1.begin()) << "\n";
    std::cout << "list2.begin(): " << static_cast<const void*>(list2.begin()) << "\n";
    std::cout << "兩者指向相同記憶體位址: "
              << (list1.begin() == list2.begin()) << "\n\n";  // true

    // ====================================================================
    // 示範 3：元素是 const 的（唯讀）
    // ====================================================================
    // initializer_list 的 begin() 回傳 const T*
    // 因此無法透過 initializer_list 修改元素值。
    // 這是設計上的限制，因為底層陣列可能存放在唯讀記憶體區段。
    // ====================================================================
    std::cout << "===== 3. 元素是 const 的 =====\n";

    std::initializer_list<int> list3 = {1, 2, 3};
    (void)list3;  // 僅為展示推導結果而宣告

    // 以下程式碼若取消註解將導致編譯錯誤：
    // *list3.begin() = 100;  // 錯誤！不能修改 const int

    std::cout << "無法修改 initializer_list 中的元素\n";
    std::cout << "begin() 回傳型別是: const int*\n\n";

    // ====================================================================
    // 示範 4：自訂類別的 initializer_list 建構子
    // ====================================================================
    // IntArray 類別有兩個建構子：
    //   (a) IntArray(initializer_list<int>)  -- 接受花括號列表
    //   (b) IntArray(size_t)                 -- 接受一個大小值
    //
    // 使用 {} 語法時，永遠優先匹配 initializer_list 建構子
    // 要呼叫 size 建構子，必須使用 () 語法
    // ====================================================================
    std::cout << "===== 4. 自訂類別的 initializer_list 建構子 =====\n";

    IntArray arr1{1, 2, 3, 4, 5};  // 呼叫 initializer_list 建構子
    arr1.print("arr1");             // arr1: [1, 2, 3, 4, 5]

    IntArray arr2{100, 200};        // 呼叫 initializer_list 建構子
    arr2.print("arr2");             // arr2: [100, 200]

    // 注意：arr3(10) 使用 () 來呼叫 size 建構子
    // 如果寫成 arr3{10}，會變成包含一個元素 10 的 initializer_list！
    IntArray arr3(10);              // 呼叫 size 建構子，建立 10 個元素（全為 0）
    arr3.print("arr3");             // arr3: [0, 0, 0, 0, 0, 0, 0, 0, 0, 0]

    std::cout << "\n";

    // ====================================================================
    // 示範 5：函式參數使用 initializer_list
    // ====================================================================
    // 函式參數型別為 initializer_list<T> 時，
    // 呼叫時需要在引數外面加上 {} 花括號。
    // 這讓函式可以接受「任意數量」的同型別引數。
    // ====================================================================
    std::cout << "===== 5. 函式參數使用 initializer_list =====\n";

    printValues({1, 2, 3, 4, 5});      // 列印: {1, 2, 3, 4, 5}

    std::cout << "sum({1, 2, 3, 4, 5}) = "
              << sum({1, 2, 3, 4, 5}) << "\n";    // 15

    std::cout << "findMax({3, 7, 2, 9, 1}) = "
              << findMax({3, 7, 2, 9, 1}) << "\n"; // 9

    std::cout << "average({1.5, 2.5, 3.5, 4.5}) = "
              << average({1.5, 2.5, 3.5, 4.5}) << "\n\n"; // 3.0

    // ====================================================================
    // 示範 6：泛型函式搭配 initializer_list
    // ====================================================================
    // 模板函式可以讓編譯器根據 {} 內的元素型別自動推導 T。
    // 但對於字串字面值 ("hello")，T 會被推導為 const char*，
    // 若需要 std::string，必須明確指定：findMin<std::string>({...})
    // ====================================================================
    std::cout << "===== 6. 泛型函式 =====\n";

    // T 自動推導為 int
    std::cout << "findMin({5, 3, 8, 1, 9}) = "
              << findMin({5, 3, 8, 1, 9}) << "\n";       // 1

    // T 自動推導為 double
    std::cout << "findMin({3.14, 1.41, 2.72}) = "
              << findMin({3.14, 1.41, 2.72}) << "\n";     // 1.41

    // T 需要明確指定為 std::string，否則會推導為 const char*
    std::cout << "findMin({\"zebra\", \"apple\", \"mango\"}) = "
              << findMin<std::string>({"zebra", "apple", "mango"}) << "\n";  // apple

    printAll({10, 20, 30});                        // T = int
    printAll({1.1, 2.2, 3.3});                     // T = double
    printAll<std::string>({"Hello", "World"});     // T = std::string（明確指定）

    std::cout << "\n";

    // ====================================================================
    // 示範 7：initializer_list 建構子的優先順序（最重要的陷阱）
    // ====================================================================
    // 當類別同時有一般建構子和 initializer_list 建構子時：
    //   - 使用 () 語法 → 呼叫一般建構子
    //   - 使用 {} 語法 → 優先呼叫 initializer_list 建構子
    //
    // 這個規則適用於所有標準容器：
    //   std::vector<int> v1(5, 10);  → 5 個 10：{10, 10, 10, 10, 10}
    //   std::vector<int> v2{5, 10};  → 2 個元素：{5, 10}
    // ====================================================================
    std::cout << "===== 7. initializer_list 建構子優先順序 =====\n";

    Ambiguous a1(5, 10);   // () 語法 → 呼叫 Ambiguous(int, int)
    Ambiguous a2{5, 10};   // {} 語法 → 呼叫 Ambiguous(initializer_list<int>)
    Ambiguous a3{5};       // {} 語法 → 呼叫 Ambiguous(initializer_list<int>)

    std::cout << "\n";

    // ====================================================================
    // 示範 8：標準容器的 initializer_list 用法
    // ====================================================================
    // 所有標準容器（vector, list, set, map 等）都支援 initializer_list：
    //   - 建構子初始化：vector<int> v{1, 2, 3};
    //   - assign() 方法：用新列表替換所有元素
    //   - insert() 方法：在指定位置插入多個元素
    //   - operator= 賦值：v = {10, 20, 30};
    // ====================================================================
    std::cout << "===== 8. 標準容器的 initializer_list =====\n";

    // 建構子初始化
    std::vector<int> v1{1, 2, 3, 4, 5};
    std::cout << "vector<int>{1,2,3,4,5}: ";
    for (int n : v1) std::cout << n << " ";
    std::cout << "\n";

    // assign() 方法：清除舊內容，設定新元素
    std::vector<int> v2;
    v2.assign({10, 20, 30});
    std::cout << "v2.assign({10,20,30}): ";
    for (int n : v2) std::cout << n << " ";
    std::cout << "\n";

    // insert() 方法：在尾端插入多個元素
    v2.insert(v2.end(), {40, 50});
    std::cout << "v2.insert(end, {40,50}): ";
    for (int n : v2) std::cout << n << " ";
    std::cout << "\n\n";

    // ====================================================================
    // 示範 9：Config 類別 — 實用範例
    // ====================================================================
    // 展示如何在實際類別中運用 initializer_list：
    //   - 建構時傳入初始選項
    //   - 透過成員函式追加或替換選項
    // ====================================================================
    std::cout << "===== 9. Config 類別示範 =====\n";

    Config cfg{"verbose", "debug", "color"};  // 建構時初始化三個選項
    cfg.print();

    cfg.addOptions({"timestamps", "logging"});  // 追加兩個選項
    cfg.print();

    cfg.setOptions({"minimal"});  // 替換為只有一個選項
    cfg.print();

    std::cout << "\n";

    // ====================================================================
    // 示範 10：空的 initializer_list
    // ====================================================================
    // 空的 {} 也是合法的 initializer_list。
    // 此時 size() 為 0，begin() == end()。
    // ====================================================================
    std::cout << "===== 10. 空的 initializer_list =====\n";

    std::initializer_list<int> emptyList{};
    std::cout << "empty list size: " << emptyList.size() << "\n";                 // 0
    std::cout << "begin() == end(): " << (emptyList.begin() == emptyList.end())   // true
              << "\n\n";

    // ====================================================================
    // 示範 11：auto 與 initializer_list 的推導規則
    // ====================================================================
    // C++11/14 規則：
    //   auto x = {1, 2, 3};   → std::initializer_list<int>
    //   auto x{1, 2, 3};      → std::initializer_list<int>
    //   auto x{42};           → std::initializer_list<int>（只有一個元素）
    //
    // C++17 變化（N3922 提案）：
    //   auto x = {1, 2, 3};   → std::initializer_list<int>（不變）
    //   auto x{42};           → int（不再是 initializer_list！）
    //   auto x{1, 2, 3};     → 編譯錯誤（direct-init 只能有一個元素）
    //
    // 結論：
    //   要安全地建立 initializer_list，使用 auto x = {1, 2, 3}; 語法
    //   單一元素用 auto x{42}; 在 C++17 後會被推導為 int
    // ====================================================================
    std::cout << "===== 11. auto 與 initializer_list =====\n";

    auto autoList = {1, 2, 3};  // 型別為 std::initializer_list<int>
    std::cout << "auto autoList = {1, 2, 3};\n";
    std::cout << "  type: std::initializer_list<int>\n";
    std::cout << "  size: " << autoList.size() << "\n";  // 3

    // C++17 之後的變化：
    // auto singleBrace{42};  // C++11/14: initializer_list<int>
    //                        // C++17+:   int

    std::cout << "\n";

    // ====================================================================
    // 示範 12：生命週期注意事項（重要陷阱）
    // ====================================================================
    // initializer_list 的底層陣列是臨時的，其生命週期有限：
    //
    // 安全的用法：
    //   (a) 直接傳給函式或建構子：sum({1, 2, 3})
    //   (b) 在同一作用域內使用：auto list = {1, 2, 3}; ... 使用 list ...
    //
    // 危險的用法：
    //   (a) 從函式回傳 initializer_list：
    //       std::initializer_list<int> getList() {
    //           return {1, 2, 3};  // 底層陣列在函式結束時銷毀！
    //       }                      // 回傳的 initializer_list 指向已釋放的記憶體！
    //
    //   (b) 儲存超過底層陣列生命週期的 initializer_list
    //
    // 最佳實踐：
    //   - 將 initializer_list 視為「臨時」物件，立即使用
    //   - 如需持久保存，複製到 std::vector 或其他容器中
    //   - 不要從函式回傳 initializer_list
    // ====================================================================
    std::cout << "===== 12. 生命週期注意事項 =====\n";

    // 安全用法：立即在 lambda 中使用
    auto safeUse = [](std::initializer_list<int> list) {
        int total = 0;
        for (int n : list) total += n;
        return total;
    };

    int result = safeUse({1, 2, 3, 4, 5});
    std::cout << "safeUse({1,2,3,4,5}) = " << result << "\n";  // 15

    // 安全用法：直接傳給建構子
    std::vector<int> safeVec({1, 2, 3, 4, 5});
    std::cout << "直接傳給建構子是安全的\n";

    // ====================================================================
    // 總結：std::initializer_list 重點整理
    // ====================================================================
    // 1. initializer_list 是 C++11 引入的輕量級包裝器
    // 2. 內部只有指標和大小，複製成本極低（淺複製）
    // 3. 元素是 const 的，不可修改
    // 4. 使用 {} 初始化時，initializer_list 建構子優先於其他建構子
    // 5. 要避免 initializer_list 優先，請使用 () 語法
    // 6. 搭配 auto 使用時注意 C++17 的規則變化
    // 7. 注意生命週期：不要從函式回傳 initializer_list
    // 8. 所有標準容器都支援 initializer_list（建構、assign、insert）
    // 9. 可用於一般函式參數，實現可變數量引數的簡潔語法
    // 10. 搭配模板可實現泛型的 initializer_list 函式
    // ====================================================================

    // ====================================================================
    // 日常實務 1：Matrix3x3（巢狀 initializer_list）
    // ====================================================================
    std::cout << "\n===== 日常實務 1：3x3 矩陣（巢狀 initializer_list）=====\n";

    Matrix3x3 identity{{1, 0, 0},
                       {0, 1, 0},
                       {0, 0, 1}};
    identity.print("單位矩陣");

    // 影像處理常見的邊緣偵測卷積核
    Matrix3x3 edgeKernel{{-1, -1, -1},
                         {-1,  8, -1},
                         {-1, -1, -1}};
    edgeKernel.print("邊緣偵測卷積核");
    std::cout << "  程式碼的排版就是矩陣的形狀 —— 這是 initializer_list 最實用的價值。\n";
    std::cout << "  建構子把內容複製進自己的 vector，沒有保存 list 本身（否則會懸空）。\n";

    // ====================================================================
    // 日常實務 2：白名單查詢
    // ====================================================================
    std::cout << "\n===== 日常實務 2：白名單查詢 =====\n";

    std::cout << "  GET    是否為允許的 HTTP 方法: "
              << isAllowed("GET",    {"GET", "POST", "PUT", "DELETE"}) << "\n";
    std::cout << "  TRACE  是否為允許的 HTTP 方法: "
              << isAllowed("TRACE",  {"GET", "POST", "PUT", "DELETE"}) << "\n";
    std::cout << "  .png   是否為允許的副檔名:     "
              << isAllowed(".png",   {".jpg", ".png", ".gif", ".webp"}) << "\n";
    std::cout << "  .exe   是否為允許的副檔名:     "
              << isAllowed(".exe",   {".jpg", ".png", ".gif", ".webp"}) << "\n";
    std::cout << "  呼叫端不必先建 vector，語法乾淨且無多餘的堆積配置。\n";

    // ====================================================================
    // 日常實務 3：move-only 型別放不進 initializer_list
    // ====================================================================
    std::cout << "\n===== 日常實務 3：move-only 型別的正確作法 =====\n";

    std::vector<std::unique_ptr<Handler> > handlers = buildHandlers();
    std::cout << "  已建立 " << handlers.size() << " 個處理器：";
    for (std::size_t i = 0; i < handlers.size(); ++i)
        std::cout << handlers[i]->name << (i + 1 < handlers.size() ? ", " : "\n");
    std::cout << "  無法用 {} 初始化，因為 initializer_list 的元素是 const，\n";
    std::cout << "  只能複製不能移動，而 unique_ptr 不可複製。\n";
    std::cout << "  正解：reserve + push_back(std::move(...))。\n";

    return 0;
}

// 編譯: g++ -std=c++11 -Wall -Wextra "summary.cpp" -o il_summary
//  ⚠️ 注意：下列輸出中的記憶體位址「每次執行都不同」（受堆積/堆疊配置與 ASLR 影響），
//     此處僅為某一次實際執行的樣本，不具重現性；其餘文字內容則是穩定可重現的。

// === 預期輸出 ===
// ===== 1. std::initializer_list 基本使用 =====
// list1 size: 5
// list1 elements: 10 20 30 40 50 
// first element: 10
// last element: 50
//
// ===== 2. 輕量複製特性 =====
// list1.begin(): 0x58498c3a8d70
// list2.begin(): 0x58498c3a8d70
// 兩者指向相同記憶體位址: true
//
// ===== 3. 元素是 const 的 =====
// 無法修改 initializer_list 中的元素
// begin() 回傳型別是: const int*
//
// ===== 4. 自訂類別的 initializer_list 建構子 =====
// [IntArray] initializer_list 建構子被呼叫, size = 5
// arr1: [1, 2, 3, 4, 5]
// [IntArray] initializer_list 建構子被呼叫, size = 2
// arr2: [100, 200]
// [IntArray] size 建構子被呼叫, size = 10
// arr3: [0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
//
// ===== 5. 函式參數使用 initializer_list =====
// printValues: {1, 2, 3, 4, 5}
// sum({1, 2, 3, 4, 5}) = 15
// findMax({3, 7, 2, 9, 1}) = 9
// average({1.5, 2.5, 3.5, 4.5}) = 3
//
// ===== 6. 泛型函式 =====
// findMin({5, 3, 8, 1, 9}) = 1
// findMin({3.14, 1.41, 2.72}) = 1.41
// findMin({"zebra", "apple", "mango"}) = apple
// printAll<i>: 10 20 30 
// printAll<d>: 1.1 2.2 3.3 
// printAll<NSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE>: Hello World 
//
// ===== 7. initializer_list 建構子優先順序 =====
// Ambiguous(int, int): a=5, b=10
// Ambiguous(initializer_list<int>): size=2, values = {5, 10}
// Ambiguous(initializer_list<int>): size=1, values = {5}
//
// ===== 8. 標準容器的 initializer_list =====
// vector<int>{1,2,3,4,5}: 1 2 3 4 5 
// v2.assign({10,20,30}): 10 20 30 
// v2.insert(end, {40,50}): 10 20 30 40 50 
//
// ===== 9. Config 類別示範 =====
// Config options:
//   - verbose
//   - debug
//   - color
// Config options:
//   - verbose
//   - debug
//   - color
//   - timestamps
//   - logging
// Config options:
//   - minimal
//
// ===== 10. 空的 initializer_list =====
// empty list size: 0
// begin() == end(): true
//
// ===== 11. auto 與 initializer_list =====
// auto autoList = {1, 2, 3};
//   type: std::initializer_list<int>
//   size: 3
//
// ===== 12. 生命週期注意事項 =====
// safeUse({1,2,3,4,5}) = 15
// 直接傳給建構子是安全的
//
// ===== 日常實務 1：3x3 矩陣（巢狀 initializer_list）=====
//   單位矩陣:
//     [ 1 0 0 ]
//     [ 0 1 0 ]
//     [ 0 0 1 ]
//   邊緣偵測卷積核:
//     [ -1 -1 -1 ]
//     [ -1 8 -1 ]
//     [ -1 -1 -1 ]
//   程式碼的排版就是矩陣的形狀 —— 這是 initializer_list 最實用的價值。
//   建構子把內容複製進自己的 vector，沒有保存 list 本身（否則會懸空）。
//
// ===== 日常實務 2：白名單查詢 =====
//   GET    是否為允許的 HTTP 方法: true
//   TRACE  是否為允許的 HTTP 方法: false
//   .png   是否為允許的副檔名:     true
//   .exe   是否為允許的副檔名:     false
//   呼叫端不必先建 vector，語法乾淨且無多餘的堆積配置。
//
// ===== 日常實務 3：move-only 型別的正確作法 =====
//   已建立 3 個處理器：auth, ratelimit, router
//   無法用 {} 初始化，因為 initializer_list 的元素是 const，
//   只能複製不能移動，而 unique_ptr 不可複製。
//   正解：reserve + push_back(std::move(...))。
