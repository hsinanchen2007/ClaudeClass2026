// =============================================================================
// 檔案名稱: initializer_list_demo.cpp
// 主題: std::initializer_list — 一個「不擁有資料」的唯讀 view
// =============================================================================
//
// 【主題資訊 Information】
//   型別    ：template<class T> class std::initializer_list<T>;
//   介面    ：size() / begin() / end()（沒有 operator[]、沒有 push_back）
//   標準版本：std::initializer_list   C++11
//             auto x{42} 改推導為 int  C++17（N3922，本檔以 C++11 規則說明）
//   標頭檔  ：<initializer_list>
//   複雜度  ：size()/begin()/end() 為 O(1)；走訪 O(N)；元素複製進容器為 O(N)
//   本檔宣告的標準：C++11
//
// 【詳細解釋 Explanation】
//
// 【1. 一句話：它是編譯器生成陣列的 view】
//   看到 {1, 2, 3}，編譯器會產生一個後備陣列（backing array）：
//       const int __backing[3] = {1, 2, 3};
//   而 initializer_list 內部只記兩件事：指向該陣列的 const 指標、元素個數。
//   它不配置記憶體、不擁有資料 —— 這解釋了它全部的特性與陷阱。
//
// 【2. 元素為 const，所以只能複製、不能移動】
//   begin() 回傳 const T*，因此：
//     * 不能透過 list 修改元素
//     * 不能把元素 move 出來（move 需要非 const 的來源）
//   實務衝擊：任何 move-only 型別（unique_ptr、thread…）
//   都無法用 {} 放進容器，必須改用 push_back(std::move(...))。
//
// 【3. 生命週期：本檔最重要的一課】
//   後備陣列的壽命 = 該 initializer_list 物件的壽命。
//   所以「回傳 initializer_list」必然懸空：
//       std::initializer_list<int> bad() { return {1,2,3}; }  // ❌
//   函式一返回，後備陣列就沒了，呼叫端拿到指向失效記憶體的 view。
//   本檔把這個危險寫法保留為註解（刻意不編譯），並示範兩種安全用法：
//     (a) 立即使用：safeUse({1,2,3,4,5}) —— list 在整個呼叫期間都有效
//     (b) 同一表達式內交給擁有資料的容器：std::vector<int> v({1,2,3,4,5});
//
// 【4. 什麼時候該用 initializer_list 當參數】
//   適合：同質、少量、唯讀、用完即丟的資料（白名單、預設值、測試資料）。
//   不適合：需要保存、需要修改、需要 move、或元素型別不一致的場合 ——
//         那些情況該用 std::vector 或 variadic template。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼「立即使用」是安全的
//     把 {1,2,3,4,5} 當引數傳給函式時，後備陣列的生命週期涵蓋整個函式呼叫
//     （直到該完整表達式結束）。所以在函式內部走訪它完全安全。
//     危險的只有「讓 view 活得比後備陣列久」—— 回傳它、或存成成員。
//
// (B) 沒有 operator[] 的設計意圖
//     標準只提供 size()/begin()/end()。要隨機存取得寫 *(il.begin() + i)。
//     這個「不方便」是刻意的：它在提醒你 initializer_list 不是容器，
//     不該被當成長期持有的資料結構使用。
//
// (C) 複製 initializer_list 很便宜，但複製「它的內容」不便宜
//     複製 list 物件本身只是複製指標與長度（淺複製，O(1)）。
//     但把它交給容器建構子時，元素會被逐一複製進容器（O(N) 次複製建構）。
//     對 std::string 這類元素，vector<string> v{"a","b","c"} 是 3 次字串複製。
//
// 【注意事項 Pay Attention】
//   1. 絕不要回傳 initializer_list，也不要把它存成類別成員 —— 會懸空，
//      之後使用屬未定義行為，不會有固定的錯誤表現。
//   2. 元素是 const：不能修改，也不能 move（move-only 型別放不進去）。
//   3. 沒有 operator[]；隨機存取須用 *(il.begin() + i)。
//   4. 類別若有 initializer_list 建構子，{} 會優先選它；要用別的建構子請改 ()。
//   5. auto x{42} 在 C++11/14 是 initializer_list<int>，C++17 起是 int（N3922）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::initializer_list 的生命週期與限制
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼函式不能回傳 std::initializer_list？
//     答：因為它只是編譯器產生之後備陣列的 view，不擁有資料。
//         後備陣列的壽命綁在該 list 物件上，函式返回時陣列即銷毀，
//         回傳的 view 立刻指向失效記憶體，使用它是未定義行為。
//     追問：那要回傳一組值該用什麼？→ 回傳擁有資料的容器，例如 std::vector<int>，
//         寫 return {1,2,3}; 讓 vector 從 list 複製一份自己的資料。
//
// 🔥 Q2. 把 {1,2,3} 當引數傳給函式，在函式內走訪它安全嗎？
//     答：安全。引數的後備陣列生命週期涵蓋整個函式呼叫（到完整表達式結束為止）。
//         危險的只有「讓 view 活得比後備陣列久」—— 也就是回傳它或存成成員。
//     追問：那在函式裡把它存進 static 變數呢？→ 不安全，那等同讓 view 活得更久，
//         函式返回後就懸空了。正確作法是複製內容到自己的容器。
//
// ⚠️ 陷阱. initializer_list 的元素可以修改嗎？可以 move 嗎？
//     答：都不行。begin() 回傳 const T*，元素是唯讀的；
//         而 move 需要非 const 的來源，所以 move-only 型別根本放不進去。
//     為什麼會錯：把它想成「輕量的 vector」，以為只是容器的簡寫。
//         實際上它是唯讀 view，const 是型別的一部分，不是可以繞過的限制。
// ═══════════════════════════════════════════════════════════════════════════
//
// 編譯指令: g++ -std=c++11 -Wall -Wextra -o initializer_list_demo initializer_list_demo.cpp
// 說明: 展示 std::initializer_list 的用法與特性
// =============================================================================

#include <iostream>
#include <initializer_list>
#include <vector>
#include <string>
#include <algorithm>
#include <numeric>
#include <cstddef>

// ===== 1. 自訂類別使用 initializer_list 建構子 =====

class IntArray
{
private:
    int* data_;
    std::size_t size_;
    
public:
    // initializer_list 建構子
    IntArray(std::initializer_list<int> init)
        : data_{new int[init.size()]}
        , size_{init.size()}
    {
        std::cout << "[IntArray] initializer_list 建構子, size = " 
                  << size_ << "\n";
        
        // 從 initializer_list 複製元素
        std::copy(init.begin(), init.end(), data_);
    }
    
    // 指定大小的建構子
    explicit IntArray(std::size_t size)
        : data_{new int[size]{}}  // 零初始化
        , size_{size}
    {
        std::cout << "[IntArray] size 建構子, size = " << size << "\n";
    }
    
    // 解構子
    ~IntArray()
    {
        delete[] data_;
    }
    
    // 禁止複製（簡化範例）
    IntArray(const IntArray&) = delete;
    IntArray& operator=(const IntArray&) = delete;
    
    // 成員存取
    std::size_t size() const { return size_; }
    
    int& operator[](std::size_t index) { return data_[index]; }
    const int& operator[](std::size_t index) const { return data_[index]; }
    
    // 迭代器支援
    int* begin() { return data_; }
    int* end() { return data_ + size_; }
    const int* begin() const { return data_; }
    const int* end() const { return data_ + size_; }
    
    // 印出內容
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

// ===== 2. 使用 initializer_list 作為函式參數 =====

void printValues(std::initializer_list<int> values)
{
    std::cout << "printValues: {";
    bool first = true;
    for (int v : values)
    {
        if (!first) std::cout << ", ";
        std::cout << v;
        first = false;
    }
    std::cout << "}\n";
}

int sum(std::initializer_list<int> values)
{
    return std::accumulate(values.begin(), values.end(), 0);
}

int findMax(std::initializer_list<int> values)
{
    if (values.size() == 0)
    {
        throw std::invalid_argument("Empty list");
    }
    return *std::max_element(values.begin(), values.end());
}

double average(std::initializer_list<double> values)
{
    if (values.size() == 0) return 0.0;
    double total = std::accumulate(values.begin(), values.end(), 0.0);
    return total / values.size();
}

// ===== 3. 泛型版本 =====

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
    std::cout << "printAll<" << typeid(T).name() << ">: ";
    for (const T& v : values)
    {
        std::cout << v << " ";
    }
    std::cout << "\n";
}

// ===== 4. 結合多載：展示優先順序 =====

class Ambiguous
{
public:
    Ambiguous(int a, int b)
    {
        std::cout << "Ambiguous(int, int): a=" << a << ", b=" << b << "\n";
    }
    
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

// ===== 5. 類別內使用 initializer_list 設定資料 =====

class Config
{
private:
    std::vector<std::string> options_;
    
public:
    Config() = default;
    
    // 接受字串列表
    Config(std::initializer_list<std::string> opts)
        : options_(opts)
    {
    }
    
    // 設定方法也接受 initializer_list
    void setOptions(std::initializer_list<std::string> opts)
    {
        options_.assign(opts.begin(), opts.end());
    }
    
    // 新增多個選項
    void addOptions(std::initializer_list<std::string> opts)
    {
        options_.insert(options_.end(), opts.begin(), opts.end());
    }
    
    void print() const
    {
        std::cout << "Config options:\n";
        for (const auto& opt : options_)
        {
            std::cout << "  - " << opt << "\n";
        }
    }
};

int main()
{
    std::cout << std::boolalpha;
    
    // ===== 1. 基本使用 =====
    std::cout << "===== 1. std::initializer_list 基本使用 =====\n";
    
    std::initializer_list<int> list1 = {10, 20, 30, 40, 50};
    
    std::cout << "list1 size: " << list1.size() << "\n";
    std::cout << "list1 elements: ";
    for (int n : list1)
    {
        std::cout << n << " ";
    }
    std::cout << "\n";
    
    // 使用 begin() 和 end()
    std::cout << "first element: " << *list1.begin() << "\n";
    std::cout << "last element: " << *(list1.end() - 1) << "\n\n";
    
    // ===== 2. 輕量複製特性 =====
    std::cout << "===== 2. 輕量複製特性 =====\n";
    
    std::initializer_list<int> list2 = list1;  // 只複製指標和大小
    
    std::cout << "list1.begin(): " << static_cast<const void*>(list1.begin()) << "\n";
    std::cout << "list2.begin(): " << static_cast<const void*>(list2.begin()) << "\n";
    std::cout << "兩者指向相同記憶體位址: " 
              << (list1.begin() == list2.begin()) << "\n\n";
    
    // ===== 3. 元素是 const 的 =====
    std::cout << "===== 3. 元素是 const 的 =====\n";
    
    std::initializer_list<int> list3 = {1, 2, 3};
    (void)list3;  // 僅為展示推導結果而宣告
    
    // *list3.begin() = 100;  // 編譯錯誤！元素是 const
    
    std::cout << "無法修改 initializer_list 中的元素\n";
    std::cout << "型別是: const int*\n\n";
    
    // ===== 4. 自訂類別的 initializer_list 建構子 =====
    std::cout << "===== 4. 自訂類別的 initializer_list 建構子 =====\n";
    
    IntArray arr1{1, 2, 3, 4, 5};
    arr1.print("arr1");
    
    IntArray arr2{100, 200};
    arr2.print("arr2");
    
    // 明確使用 size 建構子
    IntArray arr3(10);  // 使用 () 避免匹配 initializer_list
    arr3.print("arr3");
    
    std::cout << "\n";
    
    // ===== 5. 函式參數使用 initializer_list =====
    std::cout << "===== 5. 函式參數使用 initializer_list =====\n";
    
    printValues({1, 2, 3, 4, 5});
    
    std::cout << "sum({1, 2, 3, 4, 5}) = " << sum({1, 2, 3, 4, 5}) << "\n";
    std::cout << "findMax({3, 7, 2, 9, 1}) = " << findMax({3, 7, 2, 9, 1}) << "\n";
    std::cout << "average({1.5, 2.5, 3.5, 4.5}) = " 
              << average({1.5, 2.5, 3.5, 4.5}) << "\n\n";
    
    // ===== 6. 泛型函式 =====
    std::cout << "===== 6. 泛型函式 =====\n";
    
    std::cout << "findMin({5, 3, 8, 1, 9}) = " 
              << findMin({5, 3, 8, 1, 9}) << "\n";
    std::cout << "findMin({3.14, 1.41, 2.72}) = " 
              << findMin({3.14, 1.41, 2.72}) << "\n";
    std::cout << "findMin({\"zebra\", \"apple\", \"mango\"}) = " 
              << findMin<std::string>({"zebra", "apple", "mango"}) << "\n";
    
    printAll({10, 20, 30});
    printAll({1.1, 2.2, 3.3});
    printAll<std::string>({"Hello", "World"});
    
    std::cout << "\n";
    
    // ===== 7. initializer_list 建構子優先順序 =====
    std::cout << "===== 7. initializer_list 建構子優先順序 =====\n";
    
    Ambiguous a1(5, 10);     // 一般建構子
    Ambiguous a2{5, 10};     // initializer_list 建構子！
    Ambiguous a3{5};         // initializer_list 建構子！
    // Ambiguous a4{};       // 預設建構子（如果有）
    
    std::cout << "\n";
    
    // ===== 8. 標準容器的 initializer_list =====
    std::cout << "===== 8. 標準容器的 initializer_list =====\n";
    
    // vector
    std::vector<int> v1{1, 2, 3, 4, 5};
    std::cout << "vector<int>{1,2,3,4,5}: ";
    for (int n : v1) std::cout << n << " ";
    std::cout << "\n";
    
    // assign 方法
    std::vector<int> v2;
    v2.assign({10, 20, 30});
    std::cout << "v2.assign({10,20,30}): ";
    for (int n : v2) std::cout << n << " ";
    std::cout << "\n";
    
    // insert 方法
    v2.insert(v2.end(), {40, 50});
    std::cout << "v2.insert(end, {40,50}): ";
    for (int n : v2) std::cout << n << " ";
    std::cout << "\n\n";
    
    // ===== 9. Config 類別示範 =====
    std::cout << "===== 9. Config 類別示範 =====\n";
    
    Config cfg{"verbose", "debug", "color"};
    cfg.print();
    
    cfg.addOptions({"timestamps", "logging"});
    cfg.print();
    
    cfg.setOptions({"minimal"});
    cfg.print();
    
    std::cout << "\n";
    
    // ===== 10. 空的 initializer_list =====
    std::cout << "===== 10. 空的 initializer_list =====\n";
    
    std::initializer_list<int> emptyList{};
    std::cout << "empty list size: " << emptyList.size() << "\n";
    std::cout << "begin() == end(): " << (emptyList.begin() == emptyList.end()) << "\n\n";
    
    // ===== 11. auto 與 initializer_list =====
    std::cout << "===== 11. auto 與 initializer_list =====\n";
    
    auto autoList = {1, 2, 3};  // std::initializer_list<int>
    std::cout << "auto autoList = {1, 2, 3};\n";
    std::cout << "  type: std::initializer_list<int>\n";
    std::cout << "  size: " << autoList.size() << "\n";
    
    // C++17 之後，單一元素有變化
    // auto singleBrace{42};  // C++11/14: initializer_list<int>
                               // C++17+: int
    
    std::cout << "\n";
    
    // ===== 12. 生命週期注意事項 =====
    std::cout << "===== 12. 生命週期注意事項 =====\n";
    
    // 危險！底層陣列在表達式結束後銷毀
    // std::initializer_list<int> getList()
    // {
    //     return {1, 2, 3};  // 回傳後底層陣列已銷毀！
    // }
    
    // 安全：立即使用
    auto safeUse = [](std::initializer_list<int> list) {
        int total = 0;
        for (int n : list) total += n;
        return total;
    };
    
    int result = safeUse({1, 2, 3, 4, 5});
    std::cout << "safeUse({1,2,3,4,5}) = " << result << "\n";
    
    // 安全：在同一表達式內使用
    std::vector<int> safeVec({1, 2, 3, 4, 5});
    std::cout << "直接傳給建構子是安全的（vector 會複製一份自己的資料）\n";
    std::cout << "safeVec 目前有 " << safeVec.size() << " 個元素，"
              << "它擁有資料，可安全回傳、保存\n\n";

    // =========================================================================
    // 【日常實務範例】批次日誌等級過濾器
    //   情境：日誌系統要判斷「這筆訊息的等級是否在使用者選擇的顯示清單中」。
    //         呼叫端希望能直接寫 shouldLog("WARN", {"WARN", "ERROR", "FATAL"})。
    //   為什麼用 initializer_list 當參數：
    //     (a) 呼叫端不必先建一個 vector，語法乾淨；
    //     (b) 資料同質、少量、唯讀、用完即丟 —— 完全命中它的適用情境；
    //     (c) 後備陣列通常配置在堆疊上，沒有堆積配置成本。
    //   安全性：參數只在函式執行期間使用，不保存、不回傳 —— 符合生命週期規則。
    // =========================================================================
    std::cout << "===== 日常實務：日誌等級過濾 =====\n";

    // 注意這個 lambda 只「使用」list，不保存它 —— 這是安全的用法
    auto shouldLog = [](const std::string& level,
                        std::initializer_list<const char*> enabled) {
        for (std::initializer_list<const char*>::const_iterator it = enabled.begin();
             it != enabled.end(); ++it) {
            if (level == *it) return true;
        }
        return false;
    };

    // 模擬一批待輸出的日誌
    const char* incoming[] = {"DEBUG", "INFO", "WARN", "ERROR", "FATAL"};

    std::cout << "生產環境設定（只顯示 WARN 以上）：\n";
    for (std::size_t i = 0; i < 5; ++i) {
        bool pass = shouldLog(incoming[i], {"WARN", "ERROR", "FATAL"});
        std::cout << "  " << incoming[i] << " → "
                  << (pass ? "輸出" : "略過") << "\n";
    }

    std::cout << "除錯模式設定（全部顯示）：\n";
    for (std::size_t i = 0; i < 5; ++i) {
        bool pass = shouldLog(incoming[i],
                              {"DEBUG", "INFO", "WARN", "ERROR", "FATAL"});
        std::cout << "  " << incoming[i] << " → "
                  << (pass ? "輸出" : "略過") << "\n";
    }

    std::cout << "\n重點：白名單直接寫在呼叫處，不必先建容器；\n";
    std::cout << "      而且我們只在函式內走訪它，沒有保存或回傳 —— 生命週期安全。\n";

    return 0;
}

// 編譯: g++ -std=c++11 -Wall -Wextra "第 1.5 章：stdinitializer_list — 初始化列表的底層機制1.cpp" -o initializer_list_demo
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
// list1.begin(): 0x55dc90651890
// list2.begin(): 0x55dc90651890
// 兩者指向相同記憶體位址: true
//
// ===== 3. 元素是 const 的 =====
// 無法修改 initializer_list 中的元素
// 型別是: const int*
//
// ===== 4. 自訂類別的 initializer_list 建構子 =====
// [IntArray] initializer_list 建構子, size = 5
// arr1: [1, 2, 3, 4, 5]
// [IntArray] initializer_list 建構子, size = 2
// arr2: [100, 200]
// [IntArray] size 建構子, size = 10
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
// 直接傳給建構子是安全的（vector 會複製一份自己的資料）
// safeVec 目前有 5 個元素，它擁有資料，可安全回傳、保存
//
// ===== 日常實務：日誌等級過濾 =====
// 生產環境設定（只顯示 WARN 以上）：
//   DEBUG → 略過
//   INFO → 略過
//   WARN → 輸出
//   ERROR → 輸出
//   FATAL → 輸出
// 除錯模式設定（全部顯示）：
//   DEBUG → 輸出
//   INFO → 輸出
//   WARN → 輸出
//   ERROR → 輸出
//   FATAL → 輸出
//
// 重點：白名單直接寫在呼叫處，不必先建容器；
//       而且我們只在函式內走訪它，沒有保存或回傳 —— 生命週期安全。
