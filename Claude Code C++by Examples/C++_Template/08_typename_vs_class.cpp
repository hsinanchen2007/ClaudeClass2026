// ============================================================================
//  08_typename_vs_class.cpp  ──  typename / class 關鍵字與 Dependent Name
// ============================================================================
//
//  【本篇目標】
//    把 C++ template 中「最容易讓初學者迷惑」的 typename 關鍵字徹底講清
//    楚。重點：
//      ① 在「宣告 template 參數」時，typename 與 class 等價。
//      ② 在「函式體內存取 dependent name」時，必須用 typename 告訴編譯器
//         那是個「型別」。
//      ③ 同樣的場景下，存取 dependent template 名稱時要寫 .template 或
//         ::template ── 稱為 「template disambiguator」。
//
// ----------------------------------------------------------------------------
//  【① typename vs class】
//    在 template 參數位置：
//        template<typename T>     ←┐ 兩者完全等價
//        template<class    T>     ←┘ 不會因為寫 class 就變得只能傳類別
//
//    建議：統一用 typename，因為 T 不一定是類別，也可以是 int、char、自訂
//    enum 等。寫 class 容易讓讀者誤會。
//
//    例外：在「template template parameter」(file 10) 位置上，C++17 之前必
//    須寫 class；C++17 起 typename 也可以。詳見 file 10。
//
// ----------------------------------------------------------------------------
//  【② Dependent Name】
//    所謂 dependent name = 它的意義「依賴 template parameter」。
//
//        template<typename T>
//        void f(T x) {
//            T::iterator it;        // ← T::iterator 是 dependent name
//        }
//
//    上面這行其實是「歧義」：T::iterator 可能是
//        (a) T 內的某個 nested type，例如 std::vector<int>::iterator
//        (b) T 內的某個 static member，例如 T 的某個 static int 叫 iterator
//
//    編譯器在「未實例化前」無法知道 T 是什麼，所以 C++ 規定：
//        ▶ 預設假設 dependent name 「不是」型別 (是值或函式)。
//        ▶ 想當作型別用，必須在前面加 typename：
//
//            typename T::iterator it;   // 告訴編譯器「這是型別」
//
//    若忘了加 typename，編譯器會報「expected ';' before 'it'」之類詭異
//    錯誤 ── 是 C++ template 的經典菜雞踩坑。
//
//    C++20 起，在某些上下文 (例如函式回傳型別、using 宣告右側) 編譯器可以
//    自己推 typename，但保險起見，所有 dependent name 加上 typename 都不
//    會錯。
//
// ----------------------------------------------------------------------------
//  【③ template disambiguator】
//    類比上面的問題，dependent name 如果是 「template」 也要消歧：
//
//        template<typename T>
//        void g() {
//            T::template foo<int>();   // T::foo 是 template，要寫 .template
//        }
//
//    這也是經典考題：在 STL 的某些 metaprogramming code 中常見。
//
//  參考：https://en.cppreference.com/cpp/language/dependent_name
// ============================================================================

/*
補充筆記：typename_vs_class
  - typename_vs_class 涉及模板實例化；請先判斷哪些型別由呼叫端推導，哪些型別由程式指定。
  - 泛型程式要把型別需求寫清楚，例如可比較、可移動、可呼叫或符合某個 concept。
  - 模板技巧的價值在減少重複且保留型別安全，不是讓錯誤訊息變得更長。
  - typename_vs_class 是 template 主題；template 的重點是讓型別或值在編譯期決定，產生對應的具體程式碼。
  - template 定義通常需要放在 header 或使用點可見的位置，否則編譯器無法實例化需要的版本。
  - 錯誤訊息常出現在實例化深處；閱讀時先找第一個 substitution 或 constraint 不成立的位置。
  - type trait、SFINAE、concepts 都是在表達「這個型別必須具備什麼能力」；C++20 後 concepts 通常更清楚。
  - perfect forwarding 需要 T&& 搭配 std::forward<T>，不要把所有 && 都誤認為 move。
  - template 可提升零成本抽象，但也可能造成編譯時間上升和二進位膨脹；共通實作可用非 template helper 收斂。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】typename / class 與 Dependent Name
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. typename 與 class 在模板參數位置真的完全等價嗎？
//     答：宣告型別參數時 100% 等價，純粹是風格與歷史包袱。差別在其他位置：
//         typename 另有消歧義用途（見 Q2），class 沒有；而在 template template
//         parameter 的位置，C++17 起才允許用 typename 取代 class，C++14 以前只能寫 class。
//     追問：那為什麼歷史上要引入 typename？（class 讓人誤以為只能傳 class type）
//
// 🔥 Q2. 為什麼要寫 typename Container::const_iterator？什麼是 two-phase lookup？
//     答：模板分兩階段查名：非待決名稱在「定義點」解析，待決（dependent）名稱要到
//         「實例化點」才解析。編譯器在第一階段無法判斷 T::const_iterator 是型別還是
//         靜態成員，標準規定「預設當成非型別」，所以想當型別用就必須加 typename。
//     追問：什麼時候可以省？（base class list；C++20 起在只可能是型別的位置）
//
// ⚠️ 陷阱. 加了 typename 就萬無一失嗎？t.get<int>() 為什麼編不過？
//     答：不是。待決名稱如果是「成員模板」，要用的是 template 消歧義字而不是
//         typename ── 必須寫 t.template get<int>()（或 T::template rebind<U>）。
//         否則編譯器會把 get 後面的 < 當成小於號解析，噴出看似毫不相干的語法錯誤。
//     為什麼會錯：以為 typename 管所有 dependent name。typename 只負責宣告
//         「這是型別」；「這是模板」是另一個獨立的歧義，得用 template 關鍵字解。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <list>
#include <map>
#include <unordered_map>
#include <vector>

// ─── 1. typename：走訪任意 STL 容器 ───────────────────────────────────────
//   container::iterator 是 dependent name → 必須用 typename。
template <typename Container>
void print_all(const Container& c) {
    typename Container::const_iterator it;          // ← 這裡 typename 不能省
    for (it = c.begin(); it != c.end(); ++it) {
        std::cout << *it << ' ';
    }
    std::cout << "\n";
}

// 也可以用 auto 省掉 typename (C++11 起最簡潔)：
template <typename Container>
void print_all_auto(const Container& c) {
    for (auto it = c.begin(); it != c.end(); ++it) {
        std::cout << *it << ' ';
    }
    std::cout << "\n";
}

// ─── 2. typename 用在「巢狀別名」型別 ────────────────────────────────────
//   常見於 STL allocator / iterator_traits 模式。
template <typename Container>
typename Container::value_type sum_container(const Container& c) {
    typename Container::value_type s{};   // T{} 值初始化
    for (const auto& x : c) s += x;
    return s;
}

// ─── 3. Leetcode 1 ── Two Sum (用 typename Container::iterator 風格) ─────
//   題目：給 nums 與 target，找 i, j 使得 nums[i]+nums[j]==target，回傳 [i,j]。
//   假設恰有一解、不能用同一個元素兩次、回傳順序不限。
//
//   經典 hash 解法 O(n)：邊掃邊查，看「target - x」是否已經出現過。
//
//   為什麼放這裡？
//     展示「用 typename Container::const_iterator 走訪通用容器」+ 不依賴
//     特定容器型別。雖然 LC 寫 vector<int>，我們做成 template 練習。
//
//   時間：O(n)
//   空間：O(n)
template <typename Container>
std::vector<int> two_sum(const Container& nums, int target) {
    std::unordered_map<int, int> seen;            // value -> index
    int i = 0;
    for (typename Container::const_iterator it = nums.begin(); it != nums.end(); ++it, ++i) {
        int need = target - *it;
        auto found = seen.find(need);
        if (found != seen.end()) return { found->second, i };
        seen[*it] = i;
    }
    return {};                                     // 題目保證一定有解，這行為防禦
}

// ─── 4. 工作實用範例：通用 find_first ────────────────────────────────────
//   類似 std::find，只是回傳 index (找不到 -1) 而非 iterator，使用更直白。
template <typename Container, typename T>
int find_first(const Container& c, const T& key) {
    int i = 0;
    for (typename Container::const_iterator it = c.begin(); it != c.end(); ++it, ++i) {
        if (*it == key) return i;
    }
    return -1;
}

// ─── 5. template disambiguator (.template) 的範例 ───────────────────────
//   想像一個型別 T 有自己的 template member function get<U>()。
//   通用程式呼叫時要寫 t.template get<int>() 才能編譯通過。
struct Holder {
    template <typename U>
    U get() const { return U{}; }
};

template <typename T>
void demo_template_disambig(const T& t) {
    auto v = t.template get<int>();        // ← 必須的 .template
    std::cout << "demo_template_disambig got " << v << "\n";
}

// ─── 6. Leetcode 1431 ── Kids With the Greatest Number of Candies ───────
//   難度: easy
//   題目：candies[i] 是第 i 個小孩擁有的糖數；extra 是要分配的糖數。
//        對每個小孩判斷：「把 extra 全給他後是否就是糖最多的？」回傳 bool 陣列。
//   範例：candies=[2,3,5,1,3], extra=3 → [T,T,T,F,T]
//
//   解法：先找最大值 max；對每個 c 判斷 c + extra >= max。
//   時間：O(n)；空間：O(n)。
//
//   為什麼放在這裡？
//     用 typename Container::value_type 取出元素型別並作為運算對象，演示
//     dependent type alias 的實用感。
template <typename Container>
std::vector<bool> kids_with_candies(const Container& candies,
                                    typename Container::value_type extra) {
    using T = typename Container::value_type;
    T max_v = T{};
    for (const T& c : candies) if (c > max_v) max_v = c;
    std::vector<bool> result;
    result.reserve(candies.size());
    for (const T& c : candies) result.push_back(c + extra >= max_v);
    return result;
}

// ─── 7. 工作實用：count_if_t，用 typename Container::value_type 做謂詞參數 ─
//   類似 std::count_if，但用 typename 寫法明確展示 dependent name。
template <typename Container, typename Pred>
std::size_t count_if_t(const Container& c, Pred p) {
    std::size_t count = 0;
    for (typename Container::const_iterator it = c.begin(); it != c.end(); ++it) {
        if (p(*it)) ++count;
    }
    return count;
}

// ─── main ────────────────────────────────────────────────────────────────
int main() {
    // (1) print_all：對 vector / list / map 都行
    std::vector<int> v{1, 2, 3, 4};
    std::list<int>   l{10, 20, 30};
    print_all(v);
    print_all(l);
    print_all_auto(v);

    // (2) sum_container
    std::cout << "sum vector = " << sum_container(v) << "\n";
    std::cout << "sum list   = " << sum_container(l) << "\n";

    // (3) Leetcode 1 Two Sum
    std::vector<int> nums{2, 7, 11, 15};
    auto idx = two_sum(nums, 9);
    std::cout << "two_sum -> [" << idx[0] << ", " << idx[1] << "]\n";

    // (4) find_first
    std::cout << "find 11 in nums = " << find_first(nums, 11) << "\n";
    std::cout << "find 99 in nums = " << find_first(nums, 99) << "\n";

    // (5) template disambig
    Holder h;
    demo_template_disambig(h);

    // (6) Leetcode 1431 Kids With Candies
    std::vector<int> candies{2, 3, 5, 1, 3};
    auto kids = kids_with_candies(candies, 3);
    std::cout << "kids_with_candies: ";
    for (bool b : kids) std::cout << (b ? "T" : "F") << ' ';
    std::cout << "\n";

    // (7) count_if_t
    std::cout << "count_if_t (>= 3): "
              << count_if_t(candies, [](int x) { return x >= 3; }) << "\n";

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：typename 跟 class 在 template 參數位置真的「完全等價」嗎？
    //    A：是的，宣告 type parameter 時 100% 等價，純粹是歷史包袱。
    //       小差異只在 template template parameter：C++14 之前該位置必須
    //       寫 class，C++17 起也接受 typename。建議全程統一用 typename，
    //       因為 T 不一定是 class type，也可能是 int / enum / void，寫
    //       class 容易誤導讀者。
    //
    //  Q2：為什麼編譯器在沒 typename 時，會把 T::xxx 預設當成「值」？
    //    A：因為 template 在還沒實例化前，編譯器無法得知 T::xxx 究竟是
    //       nested type、static member 還是 member function。為了讓語
    //       法分析能繼續走，標準規定預設視為「非型別」(value 或 function
    //       name)，要當型別必須加 typename。這就是「dependent name 二
    //       階段查找 (two-phase lookup)」的核心規則之一。
    //
    //  Q3：什麼情況「必須」加 typename？什麼情況可以省？
    //    A：只要寫的位置是「dependent qualified name 且想當型別用」就
    //       必須加，例如函式內 `typename T::iterator it;`。可以省的情況：
    //       (a) 在 base class list (因為那位置只能是 type)；(b) using
    //       宣告的左側；(c) C++20 起在「明確只能是 type」的上下文 (函
    //       式回傳型別、template default argument 右側) 編譯器會自己推。
    //       保險原則：寫了不會錯，遇到該加沒加會編譯失敗，多寫無害。
    //
    return 0;
}

// ============================================================================
//  【小結 & 經驗法則】
//    1. 看到「巢狀型別」(T::xxx 或 typename C::iterator)，加 typename 不會錯。
//    2. 看到「巢狀 template」(T::template foo<...>)，加 .template 不會錯。
//    3. 用 auto / range-for 可以幫你避開大半 dependent name 的麻煩。
//    4. typename 與 class 在 template 參數位置等價，建議統一用 typename。
//
//  【下一篇】
//    09_non_type_template_parameter.cpp ── NTTP 深入。
// ============================================================================

// 編譯: g++ -std=c++20 -Wall -Wextra 08_typename_vs_class.cpp -o 08_typename_vs_class

// === 預期輸出 ===
// 1 2 3 4
// 10 20 30
// 1 2 3 4
// sum vector = 10
// sum list   = 60
// two_sum -> [0, 1]
// find 11 in nums = 2
// find 99 in nums = -1
// demo_template_disambig got 0
// kids_with_candies: T T T F T
// count_if_t (>= 3): 3
