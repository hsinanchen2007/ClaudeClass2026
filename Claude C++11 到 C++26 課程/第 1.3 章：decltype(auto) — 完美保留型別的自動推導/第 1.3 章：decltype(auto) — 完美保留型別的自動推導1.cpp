// =============================================================================
// 檔案名稱: decltype_auto_demo.cpp
// 主題: decltype(auto) — 讓「是值還是參考」由表達式自己決定
// =============================================================================
//
// 【主題資訊 Information】
//   語法    ：decltype(auto) 變數 = 表達式;
//             decltype(auto) 函式(...) { return 表達式; }
//   標準版本：decltype(auto)  C++14（本檔核心，-std=c++11 下編譯失敗）
//             decltype        C++11
//   標頭檔  ：<type_traits>（驗證推導結果用）
//   複雜度  ：純編譯期推導，執行期零成本
//   本檔宣告的標準：C++14
//
// 【詳細解釋 Explanation】
//
// 【1. 本檔要證明的一件事】
//   auto 與 decltype(auto) 在「同一個初始化表達式」下會推出不同型別，
//   而這個差別在執行期會造成「改得到 vs 改不到」的實際行為差異。
//   本檔用可執行的方式證明它，而不是只用文字描述：
//       auto           elem1 = vec[0];   // int   → 改 elem1，vec[0] 不變
//       decltype(auto) elem2 = vec[1];   // int&  → 改 elem2，vec[1] 跟著變
//
// 【2. 為什麼 vec[0] 會推出 int&】
//   std::vector<int>::operator[] 的回傳型別是 int&。
//   函式回傳左值參考時，該呼叫表達式的 value category 是 lvalue。
//   decltype 規則 2：lvalue → T&，於是得到 int&。
//   auto 則走模板 by-value 推導，把參考剝掉，得到 int。
//
// 【3. 賦值表達式 (a = b) 為什麼是左值】
//   C++ 的內建賦值運算子回傳「被賦值物件的左值參考」，
//   這是為了支援 (a = b) = c 這種連鎖寫法（雖然實務上不建議）。
//   因此 decltype(auto) assign = (a = b); 推出 int&，assign 成為 a 的別名。
//
// 【4. 泛型轉發：本檔最實用的部分】
//   callWithAuto 與 callWithDecltypeAuto 包裝同一個回傳 int& 的 lambda：
//     * auto 版本把 int& 剝成 int → 呼叫端拿到複本，寫入丟失。
//     * decltype(auto) 版本原樣保留 int& → 呼叫端寫得回去。
//   這正是中介層／裝飾器（decorator）必須用 decltype(auto) 的原因。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 「改不到」比「編譯錯誤」更危險
//     auto 版本會安靜地給你一個複本。程式編得過、跑得動、不丟例外，
//     只是你的修改沒有生效。這類 bug 在測試環境常因資料量小而看不出來，
//     上線後才以「設定沒生效」「快取沒更新」的形式浮現。
//     編譯錯誤反而是好事 —— 它逼你當場面對問題。
//
// (B) decltype(auto) 變數宣告的實務準則
//     一般變數請優先用 auto（要值）或 auto&（明確要參考）。
//     decltype(auto) 的價值在「我不知道也不想知道它是值還是參考，
//     我只要原樣保留」—— 這只在泛型轉發時成立。
//     在具體型別已知的地方用 decltype(auto)，只是把意圖藏起來。
//
// (C) 本檔的 (void)x; 是什麼
//     部分變數只為展示推導結果而宣告，不會被讀取。
//     為了在 -Wall -Wextra 下保持零警告，用 (void)x; 明確標示「刻意不使用」。
//     本檔是 C++14，不能用 C++17 才有的 [[maybe_unused]]
//     （已用 -pedantic-errors 驗證此差異）。
//
// 【注意事項 Pay Attention】
//   1. decltype(auto) 是 C++14；-std=c++11 會編譯失敗。
//   2. decltype(auto) x = (y); 與 = y; 型別不同（T& vs T），括號有語意。
//   3. 用 decltype(auto) 綁定容器元素時，若容器隨後擴容（如 push_back 觸發
//      重新配置），該參考會失效，再使用就是未定義行為。
//   4. 回傳區域變數的參考是 UB，其後果不固定，不可依賴任何一次觀察到的輸出。
//   5. std::vector<bool> 的 operator[] 回傳 proxy 而非 bool&，
//      decltype(auto) 會得到那個 proxy 型別，行為與其他 vector 不同。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】decltype(auto) 實作與轉發
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. auto elem = vec[0]; 與 decltype(auto) elem = vec[0]; 有什麼實際差別？
//     答：operator[] 回傳 int&，該呼叫是 lvalue。
//         auto 走 by-value 推導剝掉參考 → int（複本），改它不影響容器。
//         decltype(auto) 依規則 2 保留 → int&（別名），改它就是改容器元素。
//     追問：想明確要參考但不想寫 decltype(auto) 呢？→ 寫 auto&，
//         意圖更清楚，而且若表達式不是 lvalue 會直接編譯失敗（更安全）。
//
// 🔥 Q2. 為什麼包裝函式（wrapper）非用 decltype(auto) 不可？
//     答：被包裝的函式可能回傳值、左值參考或 const 參考。
//         用 auto 會一律剝成值，呼叫端對回傳值的寫入就失效了。
//         decltype(auto) 才能把原本的型別原封不動轉發出去。
//     追問：參數也要一起完美轉發嗎？→ 要，用 Args&&... + std::forward，
//         否則參數的值類別（lvalue/rvalue）同樣會在包裝層被抹平。
//
// ⚠️ 陷阱. decltype(auto) 綁定容器元素之後，對容器 push_back 還安全嗎？
//     答：不安全。push_back 可能觸發重新配置，把整塊元素搬到新位址，
//         原本綁定的參考就指向已釋放的記憶體，再使用是未定義行為。
//     為什麼會錯：把「參考」想成永遠有效的別名；實際上參考的有效期
//         完全取決於被綁定物件的生命週期與位址穩定性。
//         vector 只保證「未重新配置時」迭代器與參考有效。
// ═══════════════════════════════════════════════════════════════════════════
//
// 編譯指令: g++ -std=c++14 -Wall -Wextra -o decltype_auto_demo decltype_auto_demo.cpp
// 說明: 展示 C++14 decltype(auto) 的用法與特性
// =============================================================================

#include <iostream>
#include <vector>
#include <string>
#include <type_traits>
#include <utility>

// ===== 輔助函式：印出型別特性 =====
template<typename T>
void printTypeInfo(const char* name)
{
    std::cout << name << ":\n";
    std::cout << "  is_reference:     " << std::is_reference<T>::value << "\n";
    std::cout << "  is_const:         " 
              << std::is_const<typename std::remove_reference<T>::type>::value << "\n";
    std::cout << "  is_lvalue_ref:    " << std::is_lvalue_reference<T>::value << "\n";
    std::cout << "  is_rvalue_ref:    " << std::is_rvalue_reference<T>::value << "\n";
    std::cout << "\n";
}

// ===== 展示回傳型別的函式 =====

// 全域變數，用於回傳參考
int globalValue = 100;
std::vector<int> globalVec = {10, 20, 30, 40, 50};

// 回傳值
int getValue()
{
    return 42;
}

// 回傳左值參考
int& getReference()
{
    return globalValue;
}

// 回傳 const 左值參考
const int& getConstReference()
{
    static int value = 200;
    return value;
}

// ===== 使用 auto 作為回傳型別 =====
auto autoReturn()
{
    return globalValue;  // 回傳 int (複製)
}

auto autoReturnRef()
{
    return getReference();  // 回傳 int (仍是複製！auto 忽略參考)
}

// ===== 使用 decltype(auto) 作為回傳型別 =====
decltype(auto) decltypeAutoReturn()
{
    return globalValue;  // 回傳 int (globalValue 是識別符號)
}

decltype(auto) decltypeAutoReturnRef()
{
    return getReference();  // 回傳 int& (保留參考！)
}

decltype(auto) decltypeAutoReturnConstRef()
{
    return getConstReference();  // 回傳 const int&
}

// ===== 泛型轉發函式 =====

// 使用 auto：會丟失參考
template<typename F>
auto callWithAuto(F&& f)
{
    return f();  // 回傳值型別，即使 f() 回傳參考
}

// 使用 decltype(auto)：完美保留回傳型別
template<typename F>
decltype(auto) callWithDecltypeAuto(F&& f)
{
    return f();  // 完整保留 f() 的回傳型別
}

// -----------------------------------------------------------------------------
// 【日常實務範例】指標統計登錄表（metrics registry）
//   情境：服務要累計各種計數（HTTP 狀態碼、錯誤類型、快取命中）。
//         呼叫端希望寫成 registry.counter("http_200")++; 這樣直覺的形式。
//   為什麼用到本主題：
//     counter() 必須回傳 long&（參考），++ 才會累加到表裡。
//     若回傳型別被 auto 剝成 long（複本），++ 就加在暫時物件上、隨即丟棄，
//     計數永遠是 0 —— 而且完全不會報錯，是典型的靜默失效。
//   為何用 decltype(auto) 而非直接寫 long&：
//     這裡兩種寫法都對。示範重點是：當回傳型別來自
//     m_counters[name] 這個表達式時，decltype(auto) 會自動跟著它走；
//     日後若把 long 換成 std::atomic<long>，回傳型別不必手動改。
// -----------------------------------------------------------------------------
class MetricsRegistry
{
    std::vector<std::pair<std::string, long>> m_counters;

    // 找不到就新增一筆，回傳該筆的參考
    long& slotFor(const std::string& name)
    {
        for (std::size_t i = 0; i < m_counters.size(); ++i)
            if (m_counters[i].first == name)
                return m_counters[i].second;
        m_counters.push_back(std::make_pair(name, 0L));
        return m_counters.back().second;
    }

public:
    // decltype(auto)：回傳型別完全跟隨 slotFor()，此處推導為 long&
    decltype(auto) counter(const std::string& name)
    {
        return slotFor(name);          // 沒有括號 → 轉發 slotFor 的 long&
    }

    // 對照組：auto 會把 long& 剝成 long（複本）
    auto counter_WRONG(const std::string& name)
    {
        return slotFor(name);
    }

    void report() const
    {
        for (std::size_t i = 0; i < m_counters.size(); ++i)
            std::cout << "    " << m_counters[i].first
                      << " = " << m_counters[i].second << "\n";
    }
};


int main()
{
    std::cout << std::boolalpha;
    
    // ===== 1. 基本變數推導比較 =====
    std::cout << "===== 1. 基本變數推導比較 =====\n";
    
    int x = 10;
    const int cx = 20;
    int& rx = x;
    const int& crx = x;
    
    // auto 推導
    auto a1 = x;     // int
    (void)a1;  // 僅為展示推導結果而宣告
    auto a2 = cx;    // int (忽略 const)
    auto a3 = rx;    // int (忽略參考)
    auto a4 = crx;   // int (忽略 const 和參考)
    (void)a4;  // 僅為展示推導結果而宣告
    
    // decltype(auto) 推導
    decltype(auto) d1 = x;     // int
    (void)d1;  // 僅為展示推導結果而宣告
    decltype(auto) d2 = cx;    // const int
    decltype(auto) d3 = rx;    // int&
    decltype(auto) d4 = crx;   // const int&
    (void)d4;  // 僅為展示推導結果而宣告
    
    std::cout << "auto a2 = cx (const int):\n";
    std::cout << "  a2 is const: " << std::is_const<decltype(a2)>::value << "\n";
    
    std::cout << "decltype(auto) d2 = cx:\n";
    std::cout << "  d2 is const: " << std::is_const<decltype(d2)>::value << "\n";
    
    std::cout << "\nauto a3 = rx (int&):\n";
    std::cout << "  a3 is reference: " << std::is_reference<decltype(a3)>::value << "\n";
    
    std::cout << "decltype(auto) d3 = rx:\n";
    std::cout << "  d3 is reference: " << std::is_reference<decltype(d3)>::value << "\n";
    std::cout << "\n";
    
    // ===== 2. 驗證參考保留 =====
    std::cout << "===== 2. 驗證參考保留 =====\n";
    
    int value = 100;
    int& refValue = value;
    
    auto autoVar = refValue;
    (void)autoVar;  // 僅為展示推導結果而宣告
    decltype(auto) decltypeAutoVar = refValue;
    
    // 修改看看是否影響原值
    autoVar = 999;
    std::cout << "修改 autoVar = 999 後:\n";
    std::cout << "  value = " << value << " (不變，因為是複製)\n";
    
    decltypeAutoVar = 888;
    std::cout << "修改 decltypeAutoVar = 888 後:\n";
    std::cout << "  value = " << value << " (改變，因為是參考)\n\n";
    
    // ===== 3. 括號的影響 (極重要！) =====
    std::cout << "===== 3. 括號的影響 (極重要！) =====\n";
    
    int num = 42;
    
    decltype(auto) da1 = num;    // int   (識別符號)
    decltype(auto) da2 = (num);  // int&  (左值表達式！)
    
    std::cout << "decltype(auto) da1 = num:\n";
    std::cout << "  is_reference: " << std::is_reference<decltype(da1)>::value << "\n";
    
    std::cout << "decltype(auto) da2 = (num):\n";
    std::cout << "  is_reference: " << std::is_reference<decltype(da2)>::value << "\n";
    
    da2 = 123;
    std::cout << "修改 da2 = 123 後, num = " << num << "\n\n";
    
    // ===== 4. 函式回傳型別比較 =====
    std::cout << "===== 4. 函式回傳型別比較 =====\n";
    
    // 測試 auto 回傳
    std::cout << "autoReturn() 回傳型別:\n";
    printTypeInfo<decltype(autoReturn())>("  autoReturn()");
    
    std::cout << "autoReturnRef() 回傳型別:\n";
    printTypeInfo<decltype(autoReturnRef())>("  autoReturnRef()");
    
    // 測試 decltype(auto) 回傳
    std::cout << "decltypeAutoReturn() 回傳型別:\n";
    printTypeInfo<decltype(decltypeAutoReturn())>("  decltypeAutoReturn()");
    
    std::cout << "decltypeAutoReturnRef() 回傳型別:\n";
    printTypeInfo<decltype(decltypeAutoReturnRef())>("  decltypeAutoReturnRef()");
    
    // ===== 5. 透過回傳的參考修改全域變數 =====
    std::cout << "===== 5. 透過回傳的參考修改全域變數 =====\n";
    
    std::cout << "globalValue 原始值: " << globalValue << "\n";
    
    decltypeAutoReturnRef() = 500;  // 直接修改全域變數！
    
    std::cout << "執行 decltypeAutoReturnRef() = 500 後:\n";
    std::cout << "globalValue 新值: " << globalValue << "\n\n";
    
    // ===== 6. 泛型轉發比較 =====
    std::cout << "===== 6. 泛型轉發比較 =====\n";
    
    globalValue = 100;  // 重設
    
    // Lambda 回傳參考
    auto refLambda = []() -> int& { return globalValue; };
    
    // 使用 auto 轉發（會丟失參考）
    auto result1 = callWithAuto(refLambda);
    (void)result1;  // 僅為展示推導結果而宣告
    result1 = 777;
    std::cout << "callWithAuto 後修改 result1 = 777:\n";
    std::cout << "  globalValue = " << globalValue << " (不變)\n";
    
    // 使用 decltype(auto) 轉發（保留參考）
    decltype(auto) result2 = callWithDecltypeAuto(refLambda);
    result2 = 888;
    std::cout << "callWithDecltypeAuto 後修改 result2 = 888:\n";
    std::cout << "  globalValue = " << globalValue << " (改變！)\n\n";
    
    // ===== 7. 容器元素存取 =====
    std::cout << "===== 7. 容器元素存取 =====\n";
    
    std::vector<int> vec = {1, 2, 3, 4, 5};
    
    auto elem1 = vec[0];             // int (複製)
    (void)elem1;  // 僅為展示推導結果而宣告
    decltype(auto) elem2 = vec[1];   // int& (參考！)
    
    elem1 = 100;
    std::cout << "修改 elem1 = 100 後, vec[0] = " << vec[0] << " (不變)\n";
    
    elem2 = 200;
    std::cout << "修改 elem2 = 200 後, vec[1] = " << vec[1] << " (改變！)\n\n";
    
    // ===== 8. 表達式的推導 =====
    std::cout << "===== 8. 表達式的推導 =====\n";
    
    int a = 5, b = 10;
    
    decltype(auto) sum = a + b;       // int (prvalue)
    decltype(auto) assign = (a = b);  // int& (賦值回傳左值參考)
    
    std::cout << "decltype(auto) sum = a + b:\n";
    std::cout << "  is_reference: " << std::is_reference<decltype(sum)>::value << "\n";
    
    std::cout << "decltype(auto) assign = (a = b):\n";
    std::cout << "  is_reference: " << std::is_reference<decltype(assign)>::value << "\n";
    
    assign = 999;
    std::cout << "修改 assign = 999 後, a = " << a << "\n\n";

    // ===== 9. 日常實務：指標統計登錄表 =====
    std::cout << "===== 9. 日常實務：指標統計登錄表 =====\n";

    MetricsRegistry metrics;

    // decltype(auto) 版本 → 回傳 long&，++ 直接累加進表裡
    metrics.counter("http_200")++;
    metrics.counter("http_200")++;
    metrics.counter("http_200")++;
    metrics.counter("http_500")++;

    // auto 版本 → 回傳 long 複本，++ 加在暫時物件上，隨即丟棄
    metrics.counter_WRONG("cache_hit");   // 這一筆會被建立，但值永遠停在 0
    metrics.counter_WRONG("cache_hit");

    std::cout << "  目前計數：\n";
    metrics.report();
    std::cout << "  http_200 累加了 3 次 → 正確記錄（decltype(auto) 回傳 long&）\n";
    std::cout << "  cache_hit 呼叫了 2 次 → 仍是 0（auto 回傳複本，累加被丟棄）\n";
    std::cout << "  注意：auto 版本連編譯警告都沒有，是完全靜默的失效。\n";

    return 0;
}

// 編譯: g++ -std=c++14 -Wall -Wextra "第 1.3 章：decltype(auto) — 完美保留型別的自動推導1.cpp" -o decltype_auto_demo

// === 預期輸出 ===
// ===== 1. 基本變數推導比較 =====
// auto a2 = cx (const int):
//   a2 is const: false
// decltype(auto) d2 = cx:
//   d2 is const: true
//
// auto a3 = rx (int&):
//   a3 is reference: false
// decltype(auto) d3 = rx:
//   d3 is reference: true
//
// ===== 2. 驗證參考保留 =====
// 修改 autoVar = 999 後:
//   value = 100 (不變，因為是複製)
// 修改 decltypeAutoVar = 888 後:
//   value = 888 (改變，因為是參考)
//
// ===== 3. 括號的影響 (極重要！) =====
// decltype(auto) da1 = num:
//   is_reference: false
// decltype(auto) da2 = (num):
//   is_reference: true
// 修改 da2 = 123 後, num = 123
//
// ===== 4. 函式回傳型別比較 =====
// autoReturn() 回傳型別:
//   autoReturn():
//   is_reference:     false
//   is_const:         false
//   is_lvalue_ref:    false
//   is_rvalue_ref:    false
//
// autoReturnRef() 回傳型別:
//   autoReturnRef():
//   is_reference:     false
//   is_const:         false
//   is_lvalue_ref:    false
//   is_rvalue_ref:    false
//
// decltypeAutoReturn() 回傳型別:
//   decltypeAutoReturn():
//   is_reference:     false
//   is_const:         false
//   is_lvalue_ref:    false
//   is_rvalue_ref:    false
//
// decltypeAutoReturnRef() 回傳型別:
//   decltypeAutoReturnRef():
//   is_reference:     true
//   is_const:         false
//   is_lvalue_ref:    true
//   is_rvalue_ref:    false
//
// ===== 5. 透過回傳的參考修改全域變數 =====
// globalValue 原始值: 100
// 執行 decltypeAutoReturnRef() = 500 後:
// globalValue 新值: 500
//
// ===== 6. 泛型轉發比較 =====
// callWithAuto 後修改 result1 = 777:
//   globalValue = 100 (不變)
// callWithDecltypeAuto 後修改 result2 = 888:
//   globalValue = 888 (改變！)
//
// ===== 7. 容器元素存取 =====
// 修改 elem1 = 100 後, vec[0] = 1 (不變)
// 修改 elem2 = 200 後, vec[1] = 200 (改變！)
//
// ===== 8. 表達式的推導 =====
// decltype(auto) sum = a + b:
//   is_reference: false
// decltype(auto) assign = (a = b):
//   is_reference: true
// 修改 assign = 999 後, a = 999
//
// ===== 9. 日常實務：指標統計登錄表 =====
//   目前計數：
//     http_200 = 3
//     http_500 = 1
//     cache_hit = 0
//   http_200 累加了 3 次 → 正確記錄（decltype(auto) 回傳 long&）
//   cache_hit 呼叫了 2 次 → 仍是 0（auto 回傳複本，累加被丟棄）
//   注意：auto 版本連編譯警告都沒有，是完全靜默的失效。
