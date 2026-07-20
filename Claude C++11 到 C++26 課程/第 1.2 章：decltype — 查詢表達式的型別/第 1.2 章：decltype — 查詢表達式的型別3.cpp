// =============================================================================
// 檔案名稱: sfinae_hassize.cpp
// 主題: decltype + SFINAE — 在編譯期偵測「型別有沒有某個成員函式」
// =============================================================================
//
// 【主題資訊 Information】
//   核心語法：
//     template<class T> auto f(T& t) -> decltype(t.size(), std::true_type{});
//     std::false_type f(...);                       // 省略號 = 最低優先權備援
//     template<class T, class = void> struct HasX : std::false_type {};
//     template<class T> struct HasX<T, decltype(std::declval<T>().size(), void())>
//         : std::true_type {};                      // 偏特化 = void_t 模式的前身
//   標準版本：decltype / SFINAE / std::true_type / declval  皆為 C++11
//             （本檔嚴格維持 C++11，故刻意不使用任何 C++14 以後的語法）
//   標頭檔  ：<type_traits>
//   複雜度  ：全部發生在編譯期，執行期成本為零
//   本檔宣告的標準：C++11
//
// 【詳細解釋 Explanation】
//
// 【1. SFINAE 是什麼：替換失敗不是錯誤】
//   當編譯器為模板做參數替換時，如果替換後的「宣告」變成不合法
//   （例如 t.size() 對 int 根本不存在），標準規定：
//   把這個候選函式安靜地從重載集合移除，而不是報編譯錯誤。
//   這就是 Substitution Failure Is Not An Error（SFINAE）。
//   關鍵限制：只有「函式簽章（immediate context）」中的失敗才受保護；
//   若失敗發生在函式「本體」內，那就是真正的編譯錯誤，救不回來。
//
// 【2. 逗號運算子在 decltype 裡的妙用】
//     -> decltype(t.size(), std::true_type{})
//   逗號運算子會「求值左邊、丟棄結果，整個表達式的型別取右邊」。
//   所以：
//     * 左邊 t.size() 只用來做「這個成員存不存在」的合法性檢查（會觸發 SFINAE）
//     * 右邊 std::true_type{} 才是真正的回傳型別
//   一句話：左邊當條件，右邊當答案。
//
// 【3. 為什麼備援版本要用 f(...)】
//   C++ 重載決議的優先順序中，省略號（ellipsis）的匹配等級最低。
//   因此「精準的模板版本」只要能成立就一定勝出；
//   只有當它被 SFINAE 移除時，才會掉到 f(...) 這個備援。
//   這形成了一個乾淨的「有 → 走 A，沒有 → 走 B」二分法。
//
// 【4. 兩種寫法的差別】
//   (a) 函式重載法：用 decltype(hasSize(x))::value 取結果。
//       缺點是必須有一個「物件」可以傳進去。
//   (b) 類別模板偏特化法：HasSizeMethod<T>::value，只要有型別即可，
//       不需要物件；也更容易組合進其他 trait。這是 C++17 std::void_t 的前身。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼偏特化的第二參數要留 `typename = void`
//     主模板宣告成 template<class T, class = void> struct HasSizeMethod;
//     使用 HasSizeMethod<T> 時，第二參數被預設成 void。
//     偏特化寫成 HasSizeMethod<T, decltype(declval<T>().size(), void())>，
//     當 T 有 size() 時，decltype(...) 正好算出 void，於是「精準命中」偏特化；
//     當 T 沒有 size() 時，替換失敗 → 偏特化不成立 → 退回主模板（false_type）。
//     整個機制建立在「兩邊都是 void 才匹配」上，這就是 void_t 模式的原理。
//
// (B) std::declval 為何不可或缺
//     若寫成 decltype(T().size(), void())，就要求 T 必須可預設建構。
//     實務上大量型別只有帶參數的建構子，這個限制會讓 trait 誤判成 false。
//     std::declval<T>() 宣告回傳 T&& 但「故意不定義」，只能在不求值語境用，
//     讓我們不必真的建構物件也能查詢成員。
//
// (C) 回傳型別不同也偵測得到
//     WithIntSize::size() 回傳 int（而非 size_t），仍然會被偵測為 true —
//     因為我們只檢查「t.size() 這個表達式合不合法」，沒有限制它的型別。
//     若要求「必須回傳整數型別」，得再加一層 std::is_integral 檢查。
//
// (D) C++11 沒有 generic lambda —— 本檔的重要修正
//     本檔原本用 auto printInfo = [](auto& c){...}; 做條件式呼叫，
//     但 [](auto&) 是 C++14 的 generic lambda，在 -std=c++11 下無法編譯。
//     實測錯誤訊息：
//       error: use of 'auto' in lambda parameter declaration
//              only available with '-std=c++14' or '-std=gnu++14'
//     既然本章示範的是 C++11 的 SFINAE，正確作法是改用 C++11 真正可用的
//     手段（函式模板）來取代，而不是把整個檔案的標準往上調。
//     這正是「標準版本精確」的實際示範：先確認語法屬於哪一版，再決定怎麼寫。
//
// 【注意事項 Pay Attention】
//   1. SFINAE 只保護函式簽章；本體內的錯誤仍是硬錯誤（hard error）。
//   2. 本檔的 trait 只檢查「表達式合法」，不檢查回傳型別；需要時要另外加條件。
//   3. HasSizeMethod<T> 用 declval，因此不要求 T 可預設建構。
//   4. C++11 的執行期 if 兩個分支都必須能編譯；
//      要讓「不成立的分支」完全不被編譯，需要 C++17 的 if constexpr。
//   5. C++20 之後這整套寫法可用 concepts / requires 直接表達，可讀性大幅提升。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】decltype + SFINAE
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. -> decltype(t.size(), std::true_type{}) 中的逗號是什麼作用？
//     答：這是逗號運算子：求值左邊並丟棄，整個表達式取右邊的型別。
//         左邊 t.size() 純粹用來觸發 SFINAE 合法性檢查，
//         右邊 std::true_type{} 才是實際回傳型別。
//     追問：為什麼不直接寫 decltype(t.size())？→ 那樣回傳型別會變成 size() 的
//         回傳型別（size_t），不同型別的 size() 會給出不一致的結果，
//         也拿不到統一的 ::value。
//
// 🔥 Q2. 為什麼備援函式要寫成 f(...) 而不是 f(T)？
//     答：省略號在重載決議中優先權最低，保證只有模板版本被 SFINAE 移除時才會選中。
//         若寫成 f(T)，它會和模板版本形成真正的競爭，可能產生歧義（ambiguous）。
//     追問：省略號版本有什麼代價？→ 傳非平凡型別給 ... 是條件支援的行為，
//         但這裡它只在不求值語境被選中、不會真的被呼叫，所以安全。
//
// 🔥 Q3. 這段程式改成 C++17 可以怎麼簡化？
//     答：(a) 用 std::void_t 取代手寫的 decltype(..., void())；
//         (b) 用 if constexpr 取代執行期 if，讓不成立的分支根本不被編譯 —
//             這樣才能在分支裡安全地呼叫 size()。
//     追問：C++20 呢？→ 直接寫 concept，例如
//         template<class T> concept HasSize = requires(T t){ t.size(); };
//
// ⚠️ 陷阱. 「trait 回傳 true 就代表這個型別符合我要的介面」？
//     答：不一定。本檔的 trait 只驗證「t.size() 這個表達式合法」，
//         所以 size() 回傳 int、回傳 size_t、甚至回傳 void 都會是 true。
//     為什麼會錯：把「表達式合法」等同於「符合我心中的介面」。
//         真正要約束介面，必須把回傳型別、常量性、noexcept 等條件一併寫進 trait。
// ═══════════════════════════════════════════════════════════════════════════
//
// 編譯指令: g++ -std=c++11 -Wall -Wextra -o sfinae_hassize sfinae_hassize.cpp
// 說明: 展示使用 decltype + SFINAE 檢測成員函式
// =============================================================================

#include <iostream>
#include <vector>
#include <string>
#include <type_traits>
#include <sstream>

// ===== 方法 1：函式重載 + SFINAE =====

// 主模板：當 T 有 size() 時匹配
template<typename T>
auto hasSize(T& t) -> decltype(t.size(), std::true_type{})
{
    (void)t;  // 避免未使用警告
    return std::true_type{};
}

// 備用模板：當主模板失敗時匹配
// 使用 ... (variadic) 作為最低優先順序的匹配
std::false_type hasSize(...)
{
    return std::false_type{};
}

// ===== 方法 2：使用類別模板（更通用的寫法）=====

// 預設情況：假設沒有 size()
template<typename T, typename = void>
struct HasSizeMethod : std::false_type {};

// 特化版本：當 T 有 size() 時匹配
template<typename T>
struct HasSizeMethod<T, decltype(std::declval<T>().size(), void())> 
    : std::true_type {};

// ===== 測試用的類別 =====

// 有 size() 成員函式
class WithSize
{
public:
    std::size_t size() const { return 42; }
};

// 沒有 size() 成員函式
class WithoutSize
{
public:
    int getValue() const { return 100; }
};

// 有 size() 但回傳型別不同
class WithIntSize
{
public:
    int size() const { return 10; }
};

// =============================================================================
// 【條件式呼叫】C++11 正確寫法：函式模板（不是 generic lambda）
// =============================================================================
// 原本這裡寫的是：
//     auto printInfo = [](auto& container) { ... };
// 但 [](auto&) 是 C++14 的 generic lambda，在 -std=c++11 下會編譯失敗。
// C++11 想要「一個能吃任意型別的可呼叫物」，正解就是函式模板。
//
// 注意這裡用的是「執行期 if」：兩個分支都必須能通過編譯。
// 所以分支裡不能寫 container.size() —— 對沒有 size() 的型別會是硬錯誤。
// 想在分支裡安全呼叫 size()，需要 C++17 的 if constexpr（見下方對照說明）。
template <typename T>
void printInfo(const T& container)
{
    typedef typename std::decay<decltype(container)>::type ContainerType;

    if (HasSizeMethod<ContainerType>::value)
    {
        std::cout << "此容器有 size() 方法\n";
    }
    else
    {
        std::cout << "此容器沒有 size() 方法\n";
    }
}

// -----------------------------------------------------------------------------
// 【日常實務範例】通用除錯輸出 — 依型別是否為容器決定印法
//   情境：寫一個 log/dump 工具，對容器印「元素數」，對純量印「值本身」。
//         這是幾乎每個專案的除錯工具都會遇到的需求。
//   為什麼用到本主題：
//     必須在編譯期知道 T 有沒有 size()，否則對 int 呼叫 .size() 會編譯錯誤。
//   C++11 的做法：用「標籤分派（tag dispatch）」把兩種情況分成兩個真正的函式，
//     讓不成立的那一份根本不會被實體化 —— 這才是 C++11 的正解，
//     單純用執行期 if 是做不到的（兩個分支都會被編譯）。
// -----------------------------------------------------------------------------
// 有 size() 的版本：true_type 標籤
template <typename T>
std::string dumpImpl(const T& obj, std::true_type)
{
    std::ostringstream oss;
    oss << "[容器] 元素數 = " << obj.size();
    return oss.str();
}

// 沒有 size() 的版本：false_type 標籤
template <typename T>
std::string dumpImpl(const T& obj, std::false_type)
{
    std::ostringstream oss;
    oss << "[純量] 值 = " << obj;
    return oss.str();
}

// 對外介面：查 trait，把結果當成標籤分派給對應版本
template <typename T>
std::string dump(const T& obj)
{
    return dumpImpl(obj, std::integral_constant<bool, HasSizeMethod<T>::value>());
}

int main()
{
    std::cout << std::boolalpha;  // 輸出 true/false 而非 1/0
    
    // ===== 測試方法 1：函式重載 =====
    std::cout << "===== 方法 1：函式重載 + SFINAE =====\n";
    
    std::vector<int> vec = {1, 2, 3};
    std::string str = "Hello";
    WithSize objWith;
    WithoutSize objWithout;
    int number = 42;
    
    // decltype(hasSize(x))::value 取得回傳型別的 value 成員
    std::cout << "std::vector<int> has size(): " 
              << decltype(hasSize(vec))::value << "\n";
    
    std::cout << "std::string has size():      " 
              << decltype(hasSize(str))::value << "\n";
    
    std::cout << "WithSize has size():         " 
              << decltype(hasSize(objWith))::value << "\n";
    
    std::cout << "WithoutSize has size():      " 
              << decltype(hasSize(objWithout))::value << "\n";
    
    std::cout << "int has size():              " 
              << decltype(hasSize(number))::value << "\n";
    
    std::cout << "\n";
    
    // ===== 測試方法 2：類別模板 =====
    std::cout << "===== 方法 2：類別模板特化 =====\n";
    
    std::cout << "HasSizeMethod<std::vector<int>>: " 
              << HasSizeMethod<std::vector<int>>::value << "\n";
    
    std::cout << "HasSizeMethod<std::string>:      " 
              << HasSizeMethod<std::string>::value << "\n";
    
    std::cout << "HasSizeMethod<WithSize>:         " 
              << HasSizeMethod<WithSize>::value << "\n";
    
    std::cout << "HasSizeMethod<WithoutSize>:      " 
              << HasSizeMethod<WithoutSize>::value << "\n";
    
    std::cout << "HasSizeMethod<int>:              " 
              << HasSizeMethod<int>::value << "\n";
    
    std::cout << "HasSizeMethod<WithIntSize>:      " 
              << HasSizeMethod<WithIntSize>::value << "\n";
    
    std::cout << "\n";
    
    // ===== 實際應用：條件式呼叫 =====
    std::cout << "===== 實際應用：條件式呼叫 =====\n";
    
    // 根據是否有 size() 選擇不同行為
    // 注意：這裡呼叫的是「函式模板」printInfo，不是 generic lambda —
    //       generic lambda [](auto&){} 要 C++14 才有，本檔是 C++11。
    printInfo(vec);
    printInfo(objWithout);

    std::cout << "\n";

    // =========================================================================
    // 日常實務：通用除錯輸出（標籤分派）
    // =========================================================================
    std::cout << "===== 日常實務：通用 dump（標籤分派）=====\n";

    std::cout << "vector<int>  -> " << dump(vec) << "\n";
    std::cout << "std::string  -> " << dump(str) << "\n";
    std::cout << "int          -> " << dump(number) << "\n";
    std::cout << "WithIntSize  -> " << dump(WithIntSize()) << "\n";
    std::cout << "\n";
    std::cout << "說明：dump() 對有 size() 的型別走 true_type 版本，\n";
    std::cout << "      對沒有的走 false_type 版本。兩個版本是不同的函式，\n";
    std::cout << "      不成立的那一份根本不會被實體化 —— 這是 C++11 能做到\n";
    std::cout << "      「編譯期分支」的標準手法（C++17 可改用 if constexpr）。\n";

    return 0;
}

// 編譯: g++ -std=c++11 -Wall -Wextra "第 1.2 章：decltype — 查詢表達式的型別3.cpp" -o sfinae_hassize

// === 預期輸出 ===
// ===== 方法 1：函式重載 + SFINAE =====
// std::vector<int> has size(): true
// std::string has size():      true
// WithSize has size():         true
// WithoutSize has size():      false
// int has size():              false
//
// ===== 方法 2：類別模板特化 =====
// HasSizeMethod<std::vector<int>>: true
// HasSizeMethod<std::string>:      true
// HasSizeMethod<WithSize>:         true
// HasSizeMethod<WithoutSize>:      false
// HasSizeMethod<int>:              false
// HasSizeMethod<WithIntSize>:      true
//
// ===== 實際應用：條件式呼叫 =====
// 此容器有 size() 方法
// 此容器沒有 size() 方法
//
// ===== 日常實務：通用 dump（標籤分派）=====
// vector<int>  -> [容器] 元素數 = 3
// std::string  -> [容器] 元素數 = 5
// int          -> [純量] 值 = 42
// WithIntSize  -> [容器] 元素數 = 10
//
// 說明：dump() 對有 size() 的型別走 true_type 版本，
//       對沒有的走 false_type 版本。兩個版本是不同的函式，
//       不成立的那一份根本不會被實體化 —— 這是 C++11 能做到
//       「編譯期分支」的標準手法（C++17 可改用 if constexpr）。
