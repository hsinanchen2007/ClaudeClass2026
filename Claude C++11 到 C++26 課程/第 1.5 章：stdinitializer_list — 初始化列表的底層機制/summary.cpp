// ============================================================================
// 第 1.5 章：std::initializer_list — 初始化列表的底層機制（總複習）
// ============================================================================
// 編譯指令: g++ -std=c++11 -Wall -o summary summary.cpp
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

    return 0;
}
