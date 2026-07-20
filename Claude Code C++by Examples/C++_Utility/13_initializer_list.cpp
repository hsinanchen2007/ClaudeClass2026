/*
================================================================================
主題:std::initializer_list —— 接住「{1, 2, 3}」這種大括號初始列
標準:C++11 起
標頭:<initializer_list>(常常自動由其他標頭間接含入)
參考:https://en.cppreference.com/w/cpp/utility/initializer_list
================================================================================

【一、課題介紹】
  你一定寫過:
      std::vector<int> v = {1, 2, 3, 4, 5};
  那「{1,2,3,4,5}」是什麼型別?答案:**std::initializer_list<int>**。

  std::initializer_list<T> 是一個輕量、唯讀、暫時性的「T 的序列」,
  讓函式或建構子能接住「使用者直接寫的大括號清單」。

  為什麼需要它?
    - C++98 沒辦法做「vector<int> v = {1,2,3};」這種事,只能 push_back 五次。
    - 自訂容器 / 工廠類別也應該支援這種「寫起來像內建陣列」的初始化風格,
      讓使用者體驗一致。

【二、觀念解釋】
  1. 標頭:<initializer_list>(<vector>、<map> 等容器標頭通常已含入)。
  2. 唯讀:它的迭代器是 const,不能改值。
  3. 介面只有:size()、begin()、end();可用 range-for 走訪。
  4. 它指向「短暫」的陣列(可能是堆疊上的編譯器產生陣列),
     **不要把它存起來給後續使用**;一旦語句結束,內容可能不再有效。
  5. 在「建構子」中接受 std::initializer_list<T>,呼叫端就能寫:
       MyContainer c{1, 2, 3, 4};
  6. 函式可同時有「initializer_list 版」與「varargs / 個別參數版」,
     遇到 {} 大括號呼叫時 initializer_list 優先。

【三、常見陷阱】
  - 不要把 initializer_list 存成成員 / 全域變數 —— 內容是暫時的。
  - 與 std::vector 建構子的「含糊」例子:
       std::vector<int> v(10, 5);     // 10 個 5
       std::vector<int> v{10, 5};     // 兩個元素 10、5
    很多新手會誤以為大括號版做了同樣的事。
  - operator= 接 initializer_list 時要仔細:
       v = {7, 8, 9};
    這會替換整個容器內容(不是 push_back)。

【四、與其他 utility 的比較】
  - vs std::array<T, N>:array 大小編譯期固定且元素可寫;init_list 唯讀短暫。
  - vs 可變參數樣板(template<class...>):variadic 允許不同型別、可移動;
    init_list 強制元素同型別、不可移動(只能拷貝)。

【五、Leetcode 對應題目】
  題號:1929. Concatenation of Array
  難度:Easy
  連結:https://leetcode.com/problems/concatenation-of-array/
  題目大意:給 nums,回傳 nums 的兩份串接 [nums | nums]。
  選用理由:題目本身與大括號初始化關係不大,但解法可示範用 initializer_list
            風格的構造方式;也展示 vector 的「{}」初始化在實務上多麼常用。

【六、日常工作實用範例】
  情境:寫一個自製的 IntList 容器(類似 mini vector),要支援:
        IntList nums = {1, 2, 3, 4, 5};
        這正是 initializer_list 建構子的用武之地。
================================================================================
*/

/*
補充筆記：std::initializer_list
  - std::initializer_list 屬於 utility 類工具；這些型別與函式常用來表達小型資料組合、可選值、型別安全聯集或值類別轉換。
  - pair/tuple 適合簡短聚合結果，但欄位語意複雜時應定義具名 struct，避免 first/second 或 get<0> 難讀。
  - optional 表示可能沒有值，使用前要檢查 has_value 或使用 value_or；value() 在無值時會丟例外。
  - variant 表示多選一型別，應用 visit 或 holds_alternative/get_if 安全存取目前替代項。
  - any 提供執行期任意型別保存，但取回需要知道正確型別；過度使用會失去靜態型別檢查優勢。
  - std::move/std::forward/std::exchange/as_const 都是表達意圖的工具；它們本身不一定搬移或複製資料。
  - std::initializer_list 只是指向一段由編譯器管理的 const 陣列視圖，元素不可修改。
  - initializer_list 建構子在 overload resolution 中優先權高，可能讓大括號初始化選到非預期版本。
  - 不要保存 initializer_list 的 begin 指標到物件長期狀態；底層陣列生命週期通常只到完整表達式結束。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::initializer_list
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::initializer_list 的本質是什麼？有什麼生命週期陷阱？
//     答：它是一個輕量的視圖，內部指向一個由編譯器生成的暫存陣列，該陣列的生命週期
//     與 initializer_list 物件相同。所以把 initializer_list 存進成員、或從函式回傳它，
//     都會懸垂。它應該只在「接住 { ... } 然後立刻用掉」的場合出現。
//     追問：它的元素可以被移動嗎？（不行，元素是 const，所以
//     std::vector<std::unique_ptr<T>> v{ std::make_unique<T>() }; 編譯失敗）
//
// 🔥 Q2. std::vector<int> v(3, 0); 和 std::vector<int> v{3, 0}; 差在哪？
//     答：前者呼叫 (count, value) 建構子，得到 {0, 0, 0}；後者被 initializer_list
//     建構子接走，得到 {3, 0}。因為只要有 initializer_list 建構子，大括號初始化就會
//     優先選它——優先權高到會壓過其他建構子。這是最經典的 {} 陷阱。
//     追問：那 {} 到底該不該用？（大多數情況用 {} 較好，可防窄化；但對「有
//     initializer_list 建構子且大小語意重要」的容器，要刻意用 () ）
//
// ⚠️ 陷阱. auto x = {1, 2, 3}; 的型別是什麼？
//     答：std::initializer_list<int>，不是陣列也不是 vector。而 auto x{1}; 在 C++17
//     起推導為 int（單一元素的直接列表初始化被特別處理）。這兩條規則不一致，正是
//     auto 搭配大括號最容易記錯的地方。
//     為什麼會錯：以為 auto 一律「推成裡面元素的集合型別」，忽略了 auto 對大括號初始
//     列有專門的特殊規則。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <initializer_list>
#include <vector>
#include <string>

// ---------------------------------------------------------------------------
// 範例 1:函式接 initializer_list
// ---------------------------------------------------------------------------
int sum(std::initializer_list<int> xs) {
    int s = 0;
    for (int x : xs) s += x;                      // range-for 走訪
    return s;
}

void demo_basic() {
    std::cout << "[demo_basic]\n";
    std::cout << "  sum({1,2,3,4,5}) = " << sum({1,2,3,4,5}) << "\n";
    std::cout << "  sum({})          = " << sum({}) << "\n";
    std::cout << "  sum({10,20})     = " << sum({10,20}) << "\n";
}

// ---------------------------------------------------------------------------
// 範例 2:同名函式有「個別參數版」與「initializer_list 版」
//
// 大括號 {} 呼叫會優先選 initializer_list 版本。
// ---------------------------------------------------------------------------
void greet(int n, const std::string& name) {
    std::cout << "  greet(int,string): n=" << n << ", name=" << name << "\n";
}

void greet(std::initializer_list<int> xs) {
    std::cout << "  greet(init_list):  size=" << xs.size() << "\n";
}

void demo_overload() {
    std::cout << "[demo_overload]\n";
    greet(3, "Alice");                            // 走第一版
    greet({1, 2, 3});                             // 走第二版
}

// ---------------------------------------------------------------------------
// 範例 2.5:initializer_list 的全部成員 + 非成員 begin/end + 賦值用法
//
// std::initializer_list<T> 介面非常單純,但「完整」一次列出仍有意義:
//
//   成員方法(全部就這幾個):
//     - size()  : 元素數量
//     - begin() : 指向第一個元素的「const T*」
//     - end()   : 指向尾後位置
//
//   非成員(在 namespace std 內,屬 <initializer_list>):
//     - std::begin(il) / std::end(il)  也能用,在泛型程式碼中很方便。
//
//   常見用法:
//     - 容器的 operator=({...}):整批替換內容(注意:是替換,不是 push_back!)
//     - 範圍轉成 std::vector:vector v(il.begin(), il.end()) 或直接傳入。
//
// 還要提醒:initializer_list 內含的陣列「壽命短暫」,千萬不要把 il 存下來
// 之後再用;範例最後給一個小提示函式說明這點(只儲存它指到的副本,而非
// initializer_list 本身)。
// ---------------------------------------------------------------------------
void demo_init_list_helpers() {
    std::cout << "[demo_init_list_helpers]\n";

    std::initializer_list<int> il = {10, 20, 30, 40};

    // (a) size()、begin()、end():三個成員的明確使用
    std::cout << "  size() = " << il.size() << "\n";
    std::cout << "  *begin() = " << *il.begin()
              << ", *(end()-1) = " << *(il.end() - 1) << "\n";

    // (b) 用 std::begin / std::end(非成員)走訪
    int total = 0;
    for (auto it = std::begin(il); it != std::end(il); ++it) total += *it;
    std::cout << "  total via std::begin/end = " << total << "\n";

    // (c) 把 initializer_list 的內容拷貝進 vector(常見模式)
    std::vector<int> v(il.begin(), il.end());
    std::cout << "  v.size = " << v.size() << ", v.front=" << v.front() << "\n";

    // (d) operator=({...}):整批替換現有 vector 內容
    v = {7, 8, 9};                                  // 直接從 init_list 賦值
    std::cout << "  after v={7,8,9}: size=" << v.size()
              << ", front=" << v.front() << ", back=" << v.back() << "\n";

    // (e) 提醒:initializer_list 不可儲存 —— 它指向的暫時陣列在語句結束就無效
    //         若要保存,務必把資料拷貝進 vector / array 等具備所有權的容器。
}

// ---------------------------------------------------------------------------
// 範例 3:Leetcode #1929 Concatenation of Array
//
// 解題思路:
//   1. 配置 2*n 大小的結果陣列。
//   2. 走訪 nums,把每個元素寫入 result[i] 與 result[i+n]。
//
// 此處示範用「{}」風格的 vector 初始化把幾個小範例串起來,
// 順便提醒呼叫端最舒服的呼叫方式就是 initializer_list。
//
// 時間複雜度:O(n),空間複雜度:O(n)。
// ---------------------------------------------------------------------------
std::vector<int> getConcatenation(const std::vector<int>& nums) {
    int n = static_cast<int>(nums.size());
    std::vector<int> result(2 * n);
    for (int i = 0; i < n; ++i) {
        result[i]     = nums[i];
        result[i + n] = nums[i];
    }
    return result;
}

void demo_leetcode_concat() {
    std::cout << "[demo_leetcode_concat]\n";
    auto out = getConcatenation({1, 2, 1});       // 大括號就是 initializer_list
    std::cout << "  result = ";
    for (int x : out) std::cout << x << " ";
    std::cout << "\n";
}

// ---------------------------------------------------------------------------
// 範例 4:日常工作實用範例 —— 自製 IntList 支援 {} 初始化
//
// 情境:寫一個極簡的 IntList(只有 size 與 print),要讓使用者能寫:
//         IntList nums = {1, 2, 3};
//       關鍵就是提供 std::initializer_list<int> 建構子。
// ---------------------------------------------------------------------------
class IntList {
public:
    IntList(std::initializer_list<int> xs) : data_(xs) {}   // 直接拷貝到 vector
    std::size_t size() const { return data_.size(); }
    void print() const {
        std::cout << "  [";
        for (std::size_t i = 0; i < data_.size(); ++i) {
            if (i) std::cout << ", ";
            std::cout << data_[i];
        }
        std::cout << "]\n";
    }
private:
    std::vector<int> data_;
};

void demo_practical_intlist() {
    std::cout << "[demo_practical_intlist]\n";
    IntList a = {1, 2, 3};                        // 走 init_list 建構子
    IntList b = {};                               // 空清單也可
    a.print();
    std::cout << "  b.size = " << b.size() << "\n";
}

// ---------------------------------------------------------------------------
// 實用範例 (額外):log_any —— 一行同時記錄任意數量訊息
//
// 工作中常見:log("a", "b", "c") 這種「不定個數但同型別」的呼叫,
// 用 initializer_list<string_view> 接最方便, 不需要 variadic template。
// ---------------------------------------------------------------------------
#include <string_view>

void log_any(std::initializer_list<std::string_view> parts) {
    std::cout << "  LOG:";
    for (auto p : parts) std::cout << ' ' << p;
    std::cout << "\n";
}

void demo_practical_log() {
    std::cout << "[demo_practical_log]\n";
    log_any({"user", "login", "success"});
    log_any({"db", "query", "took", "12ms"});
    log_any({"shutdown"});
}

int main() {
    demo_basic();
    demo_overload();
    demo_init_list_helpers();
    demo_leetcode_concat();
    demo_practical_intlist();
    demo_practical_log();
    return 0;
}

/*
================================================================================
編譯與執行:
    g++ -std=c++17 -Wall -Wextra 13_initializer_list.cpp -o 13_initializer_list \
        && ./13_initializer_list
================================================================================
*/
