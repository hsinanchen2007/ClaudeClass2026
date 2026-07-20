// =============================================================================
//  第八課 12  —  手寫函數物件 vs Lambda：證明它們是同一件事
// =============================================================================
//
// 【主題資訊 Information】
//   函數物件 : class F { bool operator()(T) const; };      [C++98]
//   Lambda   : [capture](params) { body }                   [C++11]
//   演算法   : std::count_if(first, last, pred)             [C++98, <algorithm>]
//              回傳 pred 為 true 的元素個數，複雜度 O(n)，pred 需可複製
//
// 【詳細解釋 Explanation】
//
// 【1. 本檔要證明的一件事】
//   GreaterThanFunctor(threshold) 與 [threshold](int n){ return n > threshold; }
//   不只是「結果一樣」，而是「編譯出來的東西本質相同」。編譯器看到 lambda 時，
//   做的事情就是自動幫你生成上面那個 class：
//       捕獲清單 → 成員變數（threshold）
//       函數主體 → operator() 的內容
//       預設加 const
//   所以本檔兩種寫法算出 count1 == count2 並非巧合，而是必然。
//
// 【2. 那為什麼還要保留手寫 functor？】
//   lambda 贏在就地、簡短；手寫 class 贏在以下場合：
//     * 需要具名型別：要放進 std::set<int, MyCompare> 的比較器必須是具名型別，
//       lambda 的 closure type 無名，C++17 以前連預設建構都不行。
//     * 需要多個成員函式：例如 functor 同時提供 reset()、get_count()。
//     * 需要被重複使用、單元測試、繼承或特化。
//     * 需要文件化的公開 API：型別有名字才好寫在標頭檔裡。
//   反過來，只用一次的謂詞硬要寫成 class 就是雜訊，該用 lambda。
//
// 【3. 為什麼 count_if 的謂詞是「傳值」】
//   std::count_if 的簽名是 count_if(It, It, Predicate pred)——pred 是傳值。
//   標準要求謂詞可複製（CopyConstructible），這樣演算法內部可以自由搬動它。
//   代價是：帶狀態的 functor 在演算法內累積的狀態，不會回到你手上的那個物件。
//   要拿回狀態得靠 std::for_each 的回傳值，或用 std::ref 包一層。
//
// 【概念補充 Concept Deep Dive】
//   count_if 是模板，Predicate 是模板參數，所以「呼叫哪段程式碼」在編譯期
//   就由型別決定了。無論你傳 GreaterThanFunctor 還是 lambda，實例化後謂詞
//   都能被 inline 進迴圈，比較動作直接變成一條指令；換成函數指標
//   （bool(*)(int)）則只能做 indirect call，無法 inline。這就是 STL 演算法
//   普遍比 C 的 qsort/bsearch 快的結構性原因。
//   附帶一提：GreaterThanFunctor 只有一個 int 成員，sizeof 為 4；
//   對應的 lambda 也是 4（本機實測），連記憶體佈局都一致。
//
// 【注意事項 Pay Attention】
// 1. 謂詞應該是「純函式」——不改變元素、對同樣輸入永遠回傳同樣結果。
//    count_if 未規定訪問順序與呼叫次數，帶副作用的謂詞行為難以預測。
// 2. 謂詞的 operator() 建議加 const（本檔即是）。忘記加 const 時，
//    某些需要 const 謂詞的情境會編不過。
// 3. GreaterThanFunctor 的建構子只有一個參數卻沒有 explicit，
//    會允許隱式轉換（如 GreaterThanFunctor f = 5;）。單參數建構子
//    養成加 explicit 的習慣比較安全。
// 4. count_if 回傳型別是 iterator 的 difference_type（通常是 ptrdiff_t），
//    本檔存進 int 在小資料量沒問題，處理大容器時應改用 auto。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】函數物件 vs Lambda
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. Lambda 和手寫的函數物件，編譯後有效能差異嗎？
//     答：沒有。編譯器把 lambda 轉成一個等價的匿名 class（closure type），
//         捕獲變成成員、主體變成 operator()。兩者實例化出的程式碼一致，
//         連 sizeof 都相同（本檔的 int 捕獲皆為 4 bytes）。
//     追問：那什麼情況下還是要手寫 class？→ 需要具名型別（例如當作
//         std::set 的比較器模板參數）、需要額外成員函式、需要重複使用或
//         單元測試時。
//
// 🔥 Q2. 為什麼 std::count_if 的謂詞用傳值而不是傳參考？
//     答：標準要求謂詞可複製，好讓演算法內部自由複製／搬移它，也讓
//         實作有最大最佳化空間。後果是帶狀態的 functor 在演算法內累積
//         的狀態不會寫回原物件。
//     追問：那我真的需要拿回狀態怎麼辦？→ 用 std::for_each 接回傳值，
//         或用 std::ref(f) 傳入 reference_wrapper。
//
// ⚠️ 陷阱. 「謂詞傳函數指標也一樣，反正都能呼叫」——差在哪？
//     答：差在能不能 inline。函數物件／lambda 的呼叫目標由「型別」決定，
//         編譯期就固定，可完全 inline；函數指標的目標是執行期的「值」，
//         只能 indirect call，還會擋住迴圈向量化。
//     為什麼會錯：因為在原始碼層級三者寫起來幾乎一樣，看不出差別；
//         但編譯器眼中「型別攜帶的資訊」和「值攜帶的資訊」是天差地別的兩件事。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <algorithm>
#include <string>

// 手寫的函數物件
class GreaterThanFunctor {
private:
    int threshold;
public:
    GreaterThanFunctor(int t) : threshold(t) {}
    bool operator()(int n) const {
        return n > threshold;
    }
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 27. Remove Element
//   題目：就地移除陣列中所有等於 val 的元素，回傳剩餘長度；
//         前 k 個位置需為保留下來的元素，順序不限。
//   為什麼用到本主題：這題的核心正是「把一個謂詞交給演算法」。std::remove_if
//     需要一個 pred 決定誰該被移除，而 pred 可以是手寫 functor 也可以是
//     lambda——與本檔示範的 count_if 完全同一個模式。這裡刻意兩種寫法都給，
//     讓「functor 與 lambda 等價」不只是口號。
//   複雜度：O(n)，就地操作。
//   注意：remove_if 採用 erase-remove idiom 的前半段——它把要保留的元素往前
//     搬，回傳新的邏輯結尾；[new_end, end) 區間內是「有效但未指定」的值，
//     不可讀取其內容，只能拿長度。
// -----------------------------------------------------------------------------
class EqualsVal {
private:
    int val_;
public:
    explicit EqualsVal(int v) : val_(v) {}
    bool operator()(int n) const { return n == val_; }
};

// 版本 A：手寫 functor
int removeElementFunctor(std::vector<int>& nums, int val) {
    auto new_end = std::remove_if(nums.begin(), nums.end(), EqualsVal(val));
    return static_cast<int>(new_end - nums.begin());
}

// 版本 B：lambda（編譯器自動生成等價的 class）
int removeElementLambda(std::vector<int>& nums, int val) {
    auto new_end = std::remove_if(nums.begin(), nums.end(),
                                  [val](int n) { return n == val; });
    return static_cast<int>(new_end - nums.begin());
}

// -----------------------------------------------------------------------------
// 【日常實務範例】伺服器監控：統計超過 SLA 門檻的請求數
//   場景：一批 API 回應時間（毫秒），要統計超過 SLA（例如 200ms）的筆數，
//         而且門檻要能依不同端點動態調整。
//   為什麼用 functor：門檻是設定值，不同端點不同。用帶狀態的 functor
//     （或捕獲門檻的 lambda）就能同時持有多個不同門檻的判定器，
//     這是普通函數做不到的。
// -----------------------------------------------------------------------------
class SlaViolation {
private:
    int limit_ms_;
public:
    explicit SlaViolation(int limit_ms) : limit_ms_(limit_ms) {}
    bool operator()(int latency_ms) const { return latency_ms > limit_ms_; }
};

int main() {
    std::vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    int threshold = 5;

    std::cout << "=== functor 與 lambda 等價驗證 ===" << std::endl;

    // 方法一：手寫函數物件
    int count1 = std::count_if(vec.begin(), vec.end(), 
                               GreaterThanFunctor(threshold));
    
    // 方法二：Lambda（編譯器自動生成類似的類別）
    int count2 = std::count_if(vec.begin(), vec.end(),
        [threshold](int n) { return n > threshold; });
    
    std::cout << "函數物件結果: " << count1 << std::endl;
    std::cout << "Lambda 結果: " << count2 << std::endl;
    
    // 它們是等價的！
    std::cout << "兩者相等: " << std::boolalpha << (count1 == count2) << std::endl;
    std::cout << "sizeof(GreaterThanFunctor) = " << sizeof(GreaterThanFunctor)
              << " bytes（本機實測，實作定義）" << std::endl;
    auto equiv_lambda = [threshold](int n) { return n > threshold; };
    std::cout << "sizeof(對應的 lambda)      = " << sizeof(equiv_lambda)
              << " bytes（本機實測，實作定義）" << std::endl;

    std::cout << "\n=== LeetCode 27. Remove Element ===" << std::endl;
    std::vector<int> a1 = {3, 2, 2, 3};
    int k1 = removeElementFunctor(a1, 3);
    std::cout << "functor 版 [3,2,2,3] val=3 -> k=" << k1 << ", 前 k 個: ";
    for (int i = 0; i < k1; ++i) std::cout << a1[i] << " ";
    std::cout << std::endl;
    // 注意：只印前 k 個。[k, size) 是「有效但未指定」的殘值，不可依賴其內容。

    std::vector<int> a2 = {0, 1, 2, 2, 3, 0, 4, 2};
    int k2 = removeElementLambda(a2, 2);
    std::cout << "lambda 版 [0,1,2,2,3,0,4,2] val=2 -> k=" << k2 << ", 前 k 個: ";
    for (int i = 0; i < k2; ++i) std::cout << a2[i] << " ";
    std::cout << std::endl;

    std::cout << "\n=== 日常實務: SLA 違規統計 ===" << std::endl;
    std::vector<int> latencies = {45, 320, 88, 210, 150, 505, 199, 201};
    // 同時存在兩個不同門檻的判定器——普通函數做不到這件事
    int over200 = std::count_if(latencies.begin(), latencies.end(), SlaViolation(200));
    int over500 = std::count_if(latencies.begin(), latencies.end(), SlaViolation(500));
    std::cout << "超過 200ms: " << over200 << " 筆" << std::endl;
    std::cout << "超過 500ms: " << over500 << " 筆" << std::endl;
    std::cout << "總筆數: " << latencies.size() << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第八課：函數物件（Function Object）初探12.cpp -o functor_vs_lambda

// === 預期輸出 ===
// === functor 與 lambda 等價驗證 ===
// 函數物件結果: 5
// Lambda 結果: 5
// 兩者相等: true
// sizeof(GreaterThanFunctor) = 4 bytes（本機實測，實作定義）
// sizeof(對應的 lambda)      = 4 bytes（本機實測，實作定義）
//
// === LeetCode 27. Remove Element ===
// functor 版 [3,2,2,3] val=3 -> k=2, 前 k 個: 2 2 
// lambda 版 [0,1,2,2,3,0,4,2] val=2 -> k=5, 前 k 個: 0 1 3 0 4 
//
// === 日常實務: SLA 違規統計 ===
// 超過 200ms: 4 筆
// 超過 500ms: 1 筆
// 總筆數: 8
