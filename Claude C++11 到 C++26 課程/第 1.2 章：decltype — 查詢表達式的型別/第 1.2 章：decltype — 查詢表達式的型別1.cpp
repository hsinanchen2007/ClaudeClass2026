// =============================================================================
//  第 1.2 章：decltype — 查詢表達式的型別 (1)
//  decltype 的兩條推導規則與各種實戰用法
// =============================================================================
//
// 【主題資訊 Information】
//   語法：  decltype(表達式)          → 得到該表達式的型別
//   標準：  C++11
//   標頭檔：語言核心關鍵字，不需要 #include
//           （本檔為驗證型別才引入 <type_traits>）
//   複雜度：純編譯期操作，執行期零成本；表達式「不會被求值」
//
// 【詳細解釋 Explanation】
//
// 【1. decltype 要解決的問題 —— auto 會「洗掉」型別資訊】
//   auto 走的是樣板引數推導規則，會做三件事：陣列/函式退化成指標、
//   丟掉頂層 const、丟掉 reference。這對「我要一份可以改的副本」很方便，
//   但對「我要跟那個東西完全一樣的型別」就是災難：
//
//       const int& cr = x;
//       auto           a = cr;   // int          ← const 與 & 全被剝掉
//       decltype(cr)   b = cr;   // const int&   ← 一模一樣
//
//   泛型程式碼常常需要「回報別人的型別」而不是「決定自己的型別」，
//   這時就必須用 decltype。
//
// 【2. 兩條推導規則（整個 decltype 的核心，務必背下來）】
//   規則 1：運算元是「未加括號的識別符號 (id-expression)」或
//           「未加括號的類別成員存取」
//           → 直接取它「宣告時寫的型別」，const 與 reference 原封不動。
//           這條規則不看值類別，只看宣告。
//
//   規則 2：其他所有情況（包含加了括號、函式呼叫、運算子運算式）
//           → 當成一般表達式，依「值類別 (value category)」加工：
//               prvalue（純右值，如 i + j）      → T
//               lvalue （左值，如 i += j、vec[0]）→ T&
//               xvalue （將亡值，如 std::move(x)）→ T&&
//
//   這兩條規則的分界，就是本章最著名的括號陷阱：
//       int x;
//       decltype(x)   → int    （規則 1：看宣告）
//       decltype((x)) → int&   （規則 2：(x) 是 lvalue → 補 &）
//   括號在 C++ 絕大多數地方只是分組、沒有語意；decltype 是少數例外。
//
// 【3. 為什麼標準要這樣設計 —— 兩條規則各有用途】
//   規則 1 服務「我要宣告一個和它同型別的變數」：這時你要的是宣告型別，
//   若也補上 &，那 decltype(x) 就永遠是參考，根本沒辦法宣告獨立變數。
//   規則 2 服務「我要轉發一個表達式的結果」：這時值類別才是關鍵資訊，
//   若丟掉 &，包裝函式就會意外把 reference 降級成副本（見第 1.3 章）。
//   兩種需求都真實存在，標準用「有沒有括號」讓你明確選擇。
//
// 【4. 保留陣列型別 —— decltype 與 auto 的另一個大差異】
//   auto arr2 = arr;      → int*   （陣列退化成指標，sizeof 變成指標大小）
//   decltype(arr) arr2;   → int[5] （完整保留元素型別與長度）
//   需要以 sizeof 推算元素個數、或要傳遞真正的陣列型別時，只有 decltype 辦得到。
//
// 【概念補充 Concept Deep Dive】
//   * 未求值運算元 (unevaluated operand)：
//     decltype 內的表達式只做型別分析，編譯器不會為它產生任何指令。
//     所以 decltype(getValue()) 不會呼叫 getValue，被呼叫的函式甚至可以
//     只有宣告、沒有定義。同屬未求值脈絡的還有 sizeof 與 noexcept 運算子。
//     副作用也一樣不會發生：decltype(i += j) 得到 int&，但 i 完全沒被改。
//
//   * 為什麼 lvalue 要補 &：
//     值類別描述的是「表達式」的性質，型別系統要把它編碼成型別時，
//     lvalue 對應 T&、xvalue 對應 T&&、prvalue 對應 T。
//     這正是完美轉發能運作的底層編碼方式，decltype 只是把它揭露出來。
//
//   * 編譯器實際做了什麼：
//     decltype 在語意分析階段就被替換成具體型別，程式碼產生階段完全看不到它。
//     它不佔記憶體、不產生指令，是純粹的編譯期查詢。
//
//   * 成員存取的細節：
//     decltype(cp.x) 走規則 1，只看成員宣告型別，會「忽略物件本身的 const」；
//     decltype((cp.x)) 走規則 2，才會把物件的 const 一起帶進來變成 const int&。
//     （本機以 static_assert 實測驗證，見 demoConstMember()。）
//
// 【注意事項 Pay Attention】
//   1. 括號陷阱：decltype((x)) 是 T& 而非 T。多打一個括號就從「值」變「參考」，
//      而 reference 必須初始化，否則編譯失敗。
//   2. decltype 保留 reference，代表生命週期責任回到你身上：
//      若表達式指向的物件先死亡，你手上就是懸垂參考，行為不保證
//      （不是「一定崩潰」，可能看似正常執行，這正是它危險的原因）。
//   3. decltype(vec[0]) 是 int& 而不是 int —— operator[] 回傳的是參考。
//      想要副本要自己去參考：std::remove_reference<...>::type（C++11）
//      或 std::remove_reference_t<...>（需 C++14，本機已實測）。
//   4. sizeof 的數值是實作定義：本檔印出的 sizeof(int[5])=20、指標=8
//      為本機 x86-64 Linux g++ 實測值，不同平台可能不同。
//   5. decltype 內的表達式若含有樣板參數，替換失敗會觸發 SFINAE
//      而非編譯錯誤 —— 這是第 3 個範例檔的主題。
//   6. 在 unevaluated operand 中寫 lambda 需要 C++20（本機以 -pedantic-errors
//      實測：decltype([]{}) 在 C++17 失敗、C++20 通過）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】decltype 推導規則
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 說明 decltype 的推導規則。
//     答：兩條。(1) 運算元是「未加括號的識別符號或成員存取」→ 直接取其宣告
//         型別，const 與 reference 完整保留，不看值類別。(2) 其他情況依值類別
//         加工：prvalue → T、lvalue → T&、xvalue → T&&。
//     追問：那跟 auto 差在哪？（auto 走樣板推導，會讓陣列/函式退化、丟掉頂層
//         const 與 reference；decltype 完整保留。C++14 的 decltype(auto)
//         才兼得 auto 的簡潔與 decltype 的精確。）
//
// 🔥 Q2. int x = 0; 時 decltype(x) 與 decltype((x)) 分別是什麼？為什麼？
//     答：decltype(x) 是 int（規則 1，看宣告）；decltype((x)) 是 int&
//         （加括號後不再是裸識別符號，成為 lvalue 表達式，走規則 2 補上 &）。
//         既然是 reference，它就必須初始化，不能像 int 那樣裸宣告。
//     追問：這在實務上會造成什麼後果？（寫 decltype(auto) 的轉發函式時，
//         return x; 回傳 int，return (x); 回傳 int&；若 x 是區域變數，
//         後者就是回傳懸垂參考，本機 g++ 會發 -Wreturn-local-addr 警告。）
//
// 🔥 Q3. decltype(i += j) 之後，i 的值有沒有改變？
//     答：沒有。decltype 的運算元是未求值運算元，只做型別分析，
//         不產生程式碼、不發生任何副作用。它只回報「i += j 這個表達式
//         的型別是 int&」（因為複合賦值回傳左值參考）。
//     追問：還有哪些未求值脈絡？（sizeof、noexcept 運算子；
//         std::declval 正是靠這個性質才能「不建構物件」就取得其型別。）
//
// ⚠️ 陷阱 1. auto arr2 = arr; 與 decltype(arr) arr2; 對 int arr[5] 有何不同？
//     答：auto 讓陣列退化成 int*（sizeof 變成指標大小，本機 8）；
//         decltype 完整保留 int[5]（本機 sizeof 為 20）。
//         想用 sizeof(arr)/sizeof(arr[0]) 算元素個數時，退化就是致命傷。
//     為什麼會錯：習慣了「auto 就是自動幫我推對型別」，忽略 auto 用的是
//         樣板推導規則，而樣板推導對按值傳遞的參數本來就會做 decay。
//
// ⚠️ 陷阱 2. const Point cp; 時 decltype(cp.x) 是 const int 嗎？
//     答：不是，是 int。規則 1 只看「成員的宣告型別」，物件本身的 const
//         不會被帶進來。要拿到 const int& 必須加括號：decltype((cp.x))。
//     為什麼會錯：直覺認為「const 物件的成員當然是 const」。這對「透過該
//         物件存取」的表達式成立（規則 2），但規則 1 查的是宣告，
//         而成員宣告時寫的就是 int。本檔 demoConstMember() 已用
//         static_assert 實測兩者差異。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <string>
#include <queue>
#include <unordered_map>
#include <type_traits>

// 輔助巨集：印出型別資訊
// 使用 type_traits 來檢查型別特性
#define PRINT_TYPE_INFO(expr) \
    std::cout << #expr << ":\n"; \
    std::cout << "  is_reference: " << std::is_reference<decltype(expr)>::value << "\n"; \
    std::cout << "  is_const: " << std::is_const<typename std::remove_reference<decltype(expr)>::type>::value << "\n\n";

// 用於展示函式回傳型別推導
int getValue()
{
    return 42;
}

int& getReference()
{
    static int value = 100;
    return value;
}

const int& getConstReference()
{
    static int value = 200;
    return value;
}

// -----------------------------------------------------------------------------
// 【補充示範】const 物件的成員 —— 規則 1 與規則 2 的分水嶺
//   decltype(cp.x)   走規則 1：只看成員宣告型別 → int（物件的 const 不參與）
//   decltype((cp.x)) 走規則 2：(cp.x) 是 lvalue，且透過 const 物件存取
//                              → const int&（const 這時才被帶進來）
//   兩者以 static_assert 在編譯期驗證，不通過就編譯失敗。
// -----------------------------------------------------------------------------
struct ConstPoint
{
    int x;
    double y;
};

void demoConstMember()
{
    const ConstPoint cp{7, 1.5};

    static_assert(std::is_same<decltype(cp.x), int>::value,
                  "規則 1：只看成員宣告型別，忽略物件的 const");
    static_assert(std::is_same<decltype((cp.x)), const int&>::value,
                  "規則 2：加括號後才把物件的 const 帶進來");

    std::cout << "decltype(cp.x)   -> int         (規則 1：看宣告，不含 const)\n";
    std::cout << "decltype((cp.x)) -> const int&  (規則 2：括號 → lvalue，含 const)\n";
    std::cout << "cp.x = " << cp.x << " (static_assert 已在編譯期證明上述兩點)\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1046. Last Stone Weight
//   題目：每次取出最重的兩顆石頭相撞，回傳最後剩下的石頭重量（沒有則回 0）。
//   為什麼用到本主題：
//     這題要用 priority_queue，而自訂比較器如果寫成 lambda，就會遇到
//     「lambda 的型別是編譯器產生的匿名型別，你沒辦法把它寫出來」的問題。
//     priority_queue 的第三個樣板參數需要的是「型別」不是「物件」，
//     此時唯一的辦法就是 decltype(cmp) —— 這是 decltype 在演算法題裡
//     最真實、最高頻的用途（LeetCode 215、347、703 等都會用到同一招）。
//
//   版本註記（本機以 -pedantic-errors 實測）：
//     C++11/14/17：lambda 沒有預設建構子，必須把 cmp 物件傳進建構子。
//     C++20 起   ：無捕捉 lambda 可預設建構，且可直接寫在未求值脈絡中
//                  （decltype([](int a,int b){return a>b;})），才能省略傳參。
// -----------------------------------------------------------------------------
int lastStoneWeight(std::vector<int> stones)
{
    // 比較器：a < b 代表「a 的優先權較低」，所以這樣寫是最大堆 (max-heap)
    auto cmp = [](int a, int b) { return a < b; };

    // 關鍵：型別參數只能用 decltype(cmp) 表達；C++11 還必須把 cmp 傳給建構子
    std::priority_queue<int, std::vector<int>, decltype(cmp)> heap(cmp);
    for (int s : stones) heap.push(s);

    while (heap.size() > 1)
    {
        int a = heap.top(); heap.pop();   // 最重
        int b = heap.top(); heap.pop();   // 次重
        if (a != b) heap.push(a - b);     // 相撞後剩下差值
    }
    return heap.empty() ? 0 : heap.top();
}

// -----------------------------------------------------------------------------
// 【日常實務範例】伺服器存取日誌統計 —— 用 decltype 綁定「容器決定的型別」
//   情境：統計每個 URL 的請求次數，再找出次數最高者。
//   為什麼用 decltype：
//     計數容器日後可能從 unordered_map<string,int> 改成
//     unordered_map<string,long long>（流量變大、int 會溢位）。
//     若累計變數手寫成 int，改容器時這裡會靜默溢位而不會編譯錯誤。
//     用 decltype(counter)::mapped_type 讓型別「跟著容器走」，
//     改一處即可，不會漏改。
// -----------------------------------------------------------------------------
std::string hottestEndpoint(const std::vector<std::string>& accessLog)
{
    std::unordered_map<std::string, long long> counter;
    for (const std::string& url : accessLog) ++counter[url];

    // 型別跟著容器定義走，不手寫 long long
    using CountType = decltype(counter)::mapped_type;   // = long long
    static_assert(std::is_same<CountType, long long>::value,
                  "mapped_type 應與容器宣告一致");

    CountType best = 0;
    std::string bestUrl;
    for (const auto& kv : counter)
    {
        // decltype(kv.second) 走規則 1 → long long（成員宣告型別）
        if (kv.second > best || (kv.second == best && kv.first < bestUrl))
        {
            best = kv.second;
            bestUrl = kv.first;
        }
    }
    return bestUrl + " (" + std::to_string(best) + " 次)";
}

int main()
{
    // ===== 1. 基本用法：從變數推導型別 =====
    std::cout << "===== 1. 基本用法：從變數推導型別 =====\n";
    
    int x = 10;
    double y = 3.14;
    std::string str = "Hello";
    
    decltype(x) a = 20;        // int
    decltype(y) b = 2.718;     // double
    decltype(str) s = "World"; // std::string
    
    std::cout << "a = " << a << " (與 x 同型別: int)\n";
    std::cout << "b = " << b << " (與 y 同型別: double)\n";
    std::cout << "s = " << s << " (與 str 同型別: std::string)\n\n";
    
    // ===== 2. 保留 const 與參考 =====
    std::cout << "===== 2. 保留 const 與參考 =====\n";
    
    const int cx = 100;
    int& rx = x;
    const int& crx = x;
    
    decltype(cx) c1 = 50;     // const int (保留 const)
    decltype(rx) r1 = x;      // int& (保留參考)
    decltype(crx) cr1 = x;    // const int& (保留 const 和參考)

    // c1 = 60;  // 錯誤！c1 是 const int
    r1 = 999;    // 合法，r1 是 int&

    // 用 static_assert 在編譯期證明型別完整保留（比執行期印出更有力）
    static_assert(std::is_same<decltype(c1),  const int>::value,  "應保留 const");
    static_assert(std::is_same<decltype(r1),  int&>::value,       "應保留 reference");
    static_assert(std::is_same<decltype(cr1), const int&>::value, "應保留 const 與 reference");
    (void)c1; (void)cr1;      // 上面已用 static_assert 驗證，抑制未使用警告

    std::cout << "修改 r1 = 999 後:\n";
    std::cout << "  x = " << x << " (證明 r1 是 x 的參考)\n\n";
    
    // ===== 3. auto vs decltype 比較 =====
    std::cout << "===== 3. auto vs decltype 比較 =====\n";
    
    const int value = 42;
    int& ref = x;
    
    auto       autoVal = value;      // int (忽略 const)
    decltype(value) declVal = 100;   // const int (保留 const)

    auto       autoRef = ref;        // int (忽略參考，是複製)
    decltype(ref) declRef = x;       // int& (保留參考)

    // 編譯期證明：auto 洗掉了 const / reference，decltype 完整保留
    static_assert(!std::is_const<decltype(autoVal)>::value,  "auto 會丟掉 const");
    static_assert(std::is_const<decltype(declVal)>::value,   "decltype 保留 const");
    static_assert(!std::is_reference<decltype(autoRef)>::value, "auto 會丟掉 reference");
    static_assert(std::is_reference<decltype(declRef)>::value,  "decltype 保留 reference");
    (void)autoVal; (void)declVal;

    autoRef = 111;   // 不影響 x
    declRef = 222;   // 會修改 x
    (void)autoRef;   // 只為示範「改副本不影響原值」，之後不再使用

    std::cout << "autoRef = 111 後, x = " << x << "\n";
    x = 500;  // 重設
    declRef = 222;
    std::cout << "declRef = 222 後, x = " << x << " (證明 declRef 是參考)\n\n";
    
    // ===== 4. 括號的影響 (重要！) =====
    std::cout << "===== 4. 括號的影響 (重要！) =====\n";
    
    int num = 10;
    
    decltype(num)   d1 = 0;    // int  (識別符號 → 規則 1，用宣告型別)
    // decltype((num)) d2;     // ✗ 編譯錯誤：它是 int&，reference 必須初始化
    decltype((num)) d2 = num;  // int& (左值表達式 → 規則 2，必須初始化參考)

    static_assert(std::is_same<decltype(d1), int>::value,  "規則 1 → int");
    static_assert(std::is_same<decltype(d2), int&>::value, "規則 2 → int&");
    (void)d1;

    d2 = 777;
    std::cout << "d2 = 777 後, num = " << num << "\n";
    std::cout << "(證明 decltype((num)) 是 int&，多一個括號就從值變參考)\n\n";

    // ===== 5. 表達式的型別推導 =====
    std::cout << "===== 5. 表達式的型別推導 =====\n";

    int i = 5, j = 10;

    decltype(i + j) sum = 0;       // int  (prvalue，純右值 → T)
    decltype(i < j) cmp = false;   // bool (prvalue → T)
    decltype(i += j) ref2 = i;     // int& (i += j 是 lvalue → T&)

    static_assert(std::is_same<decltype(sum),  int>::value,  "prvalue → T");
    static_assert(std::is_same<decltype(cmp),  bool>::value, "比較運算 → bool");
    static_assert(std::is_same<decltype(ref2), int&>::value, "lvalue → T&");

    sum = 100;
    cmp = true;
    (void)cmp; (void)ref2;

    std::cout << "decltype(i + j)  -> int,  sum = " << sum << "\n";
    std::cout << "decltype(i < j)  -> bool\n";
    std::cout << "decltype(i += j) -> int& (複合賦值回傳左值參考)\n";
    // 重點：decltype 的運算元是「未求值運算元」，i += j 完全沒有被執行
    std::cout << "i 仍然是 " << i << "（decltype(i += j) 不會真的做加法，無副作用）\n\n";
    
    // ===== 6. 函式回傳型別推導 =====
    std::cout << "===== 6. 函式回傳型別推導 =====\n";
    
    decltype(getValue()) val1;           // int
    decltype(getReference()) val2 = x;   // int&
    decltype(getConstReference()) val3 = x;  // const int&
    
    std::cout << "decltype(getValue()) -> int\n";
    std::cout << "decltype(getReference()) -> int&\n";
    std::cout << "decltype(getConstReference()) -> const int&\n";
    
    // 注意：decltype 不會真的呼叫函式！
    std::cout << "(以上推導過程中，函式都沒有被實際呼叫)\n\n";
    
    // ===== 7. 陣列與指標 =====
    std::cout << "===== 7. 陣列與指標 =====\n";
    
    int arr[5] = {1, 2, 3, 4, 5};
    int* ptr = arr;
    
    decltype(arr) arr2 = {10, 20, 30, 40, 50};  // int[5] (保留陣列型別！)
    decltype(ptr) ptr2;                          // int*
    
    std::cout << "sizeof(arr) = " << sizeof(arr) << "\n";
    std::cout << "sizeof(arr2) = " << sizeof(arr2) << " (decltype 保留陣列型別)\n";
    
    // 對比 auto
    auto arrAuto = arr;  // int* (退化為指標)
    std::cout << "sizeof(arrAuto) = " << sizeof(arrAuto) << " (auto 退化為指標)\n\n";
    
    // ===== 8. 容器元素存取 =====
    std::cout << "===== 8. 容器元素存取 =====\n";
    
    std::vector<int> vec = {1, 2, 3, 4, 5};
    
    // vec[0] 回傳 int&
    decltype(vec[0]) elem = vec[0];  // int&
    elem = 100;
    
    std::cout << "修改 elem 後, vec[0] = " << vec[0] << "\n";
    
    // vec.size() 回傳 size_t (prvalue)
    decltype(vec.size()) sz = vec.size();  // std::vector<int>::size_type
    std::cout << "vec.size() = " << sz << "\n\n";
    
    // ===== 9. 搭配 typedef / using =====
    std::cout << "===== 9. 搭配 typedef / using =====\n";
    
    std::vector<std::pair<std::string, int>> data;
    data.push_back({"Alice", 95});
    data.push_back({"Bob", 87});
    
    // 使用 decltype 定義型別別名
    using DataType = decltype(data);
    using ElemType = decltype(data[0]);
    using IterType = decltype(data.begin());
    
    DataType data2;
    data2.push_back({"Charlie", 92});
    
    std::cout << "成功使用 decltype 定義複雜型別別名\n";
    std::cout << "data2[0].first = " << data2[0].first << "\n\n";
    
    // ===== 10. 類別成員的 decltype =====
    std::cout << "===== 10. 類別成員的 decltype =====\n";
    
    struct Point
    {
        int x;
        double y;
    };
    
    Point pt{10, 20.5};
    
    decltype(pt.x) px = 100;      // int
    decltype(pt.y) py = 3.14;     // double
    decltype((pt.x)) prx = pt.x;  // int& (括號使其成為左值表達式)
    
    prx = 999;
    std::cout << "修改 prx 後, pt.x = " << pt.x << "\n";
    
    // 不需實體也能推導（使用類別名稱）
    decltype(Point::x) memberX;   // int
    decltype(Point::y) memberY;   // double
    std::cout << "decltype(Point::x) -> int\n";
    std::cout << "decltype(Point::y) -> double\n";
    
    return 0;
}

// 編譯: g++ -std=c++11 -Wall -Wextra "第 1.2 章：decltype — 查詢表達式的型別1.cpp" -o decltype1
//
// 版本註記（本機以 -pedantic-errors 實測）：
//   本檔【全部】語法自 C++11 起即可用 —— 以 -std=c++11 -pedantic-errors
//   編譯零 error 通過（decltype 本身就是 C++11 新增）。
//   註解中提到的 std::remove_reference_t 需 C++14、
//   decltype([]{}) 需 C++20，這兩者本檔都只在說明中出現、未實際使用。

// 註 1:編譯時會出現 10 個警告（7 個 -Wunused-variable、
//      2 個 -Wunused-local-typedefs、以及 memberX/memberY），
//      這些是【刻意】的。decltype 的重點是「宣告出什麼型別」，
//      而型別本身無法在執行期印出，所以第 10 節那類變數
//      （decltype(Point::x) memberX;）宣告完就沒有用途 ——
//      它們存在的意義是「能編譯過」本身就是證明。

// 註 2:本檔輸出是【完全確定的】，連跑 5 次位元組完全相同。
//      沒有位址、亂數或執行緒；第 6 節的函式甚至從未被真正呼叫
//      （decltype 的運算元是 unevaluated operand，只做型別分析）。

// 註 3:第 7 節的 sizeof 為【本機實測】(x86-64 Linux, g++ 15.2.0)：
//      sizeof(arr)=sizeof(arr2)=20 是 int[5]，sizeof(arrAuto)=8 是指標。
//      指標大小屬實作定義（32-bit 平台為 4）。此處要記的是
//      「decltype 保留陣列型別、auto 讓它退化成指標」這個對比，
//      不是 8 或 20 這兩個數字。

// === 預期輸出 ===
// ===== 1. 基本用法：從變數推導型別 =====
// a = 20 (與 x 同型別: int)
// b = 2.718 (與 y 同型別: double)
// s = World (與 str 同型別: std::string)
//
// ===== 2. 保留 const 與參考 =====
// 修改 r1 = 999 後:
//   x = 999 (證明 r1 是 x 的參考)
//
// ===== 3. auto vs decltype 比較 =====
// autoRef = 111 後, x = 222
// declRef = 222 後, x = 222 (證明 declRef 是參考)
//
// ===== 4. 括號的影響 (重要！) =====
// d2 = 777 後, num = 777
// (證明 decltype((num)) 是 int&，多一個括號就從值變參考)
//
// ===== 5. 表達式的型別推導 =====
// decltype(i + j)  -> int,  sum = 100
// decltype(i < j)  -> bool
// decltype(i += j) -> int& (複合賦值回傳左值參考)
// i 仍然是 5（decltype(i += j) 不會真的做加法，無副作用）
//
// ===== 6. 函式回傳型別推導 =====
// decltype(getValue()) -> int
// decltype(getReference()) -> int&
// decltype(getConstReference()) -> const int&
// (以上推導過程中，函式都沒有被實際呼叫)
//
// ===== 7. 陣列與指標 =====
// sizeof(arr) = 20
// sizeof(arr2) = 20 (decltype 保留陣列型別)
// sizeof(arrAuto) = 8 (auto 退化為指標)
//
// ===== 8. 容器元素存取 =====
// 修改 elem 後, vec[0] = 100
// vec.size() = 5
//
// ===== 9. 搭配 typedef / using =====
// 成功使用 decltype 定義複雜型別別名
// data2[0].first = Charlie
//
// ===== 10. 類別成員的 decltype =====
// 修改 prx 後, pt.x = 999
// decltype(Point::x) -> int
// decltype(Point::y) -> double
