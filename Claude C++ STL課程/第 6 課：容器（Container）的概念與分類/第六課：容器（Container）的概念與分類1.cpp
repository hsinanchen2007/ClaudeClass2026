// =============================================================================
//  第六課 1 — std::array：編譯期固定大小的「零開銷」陣列
// =============================================================================
//
// 【主題資訊 Information】
//   template<class T, std::size_t N> struct array;      // <array>,C++11 起
//
//   常用介面（全部 constexpr-friendly）：
//     constexpr size_type size()      const noexcept;   // 恆等於 N,O(1)
//     constexpr bool      empty()     const noexcept;   // 恆等於 (N == 0)
//     constexpr reference operator[](size_type n);      // 不做邊界檢查
//     constexpr reference at(size_type n);              // 越界丟 std::out_of_range
//     constexpr reference front();                      // == (*this)[0]
//     constexpr reference back();                       // == (*this)[N-1]
//     constexpr T*        data() noexcept;              // 指向連續記憶體
//
//   複雜度：所有存取與 size() 皆為 O(1),且多半在編譯期就決定。
//   標頭檔：#include <array>
//
// 【詳細解釋 Explanation】
//
// 【1. std::array 要解決什麼問題:C 陣列的三宗罪】
// C 原生陣列 int arr[5] 用起來很快,但它有三個結構性缺陷,而 std::array 正是
// 為了逐一補掉這三點而生的:
//
//   (a) 會 decay 成指標。把 int arr[5] 傳進函式,參數實際型別是 int*,
//       長度資訊在傳參的瞬間就永久遺失了。函式裡再寫 sizeof(arr) 得到的是
//       指標大小(本機 8),不是 20。這是 C 語言最經典的 bug 來源之一。
//   (b) 不是一等公民。C 陣列不能整體指派(a = b 不合法)、不能當回傳值、
//       不能直接放進 std::vector,因為它沒有 value semantics。
//   (c) 沒有介面。沒有 size()、沒有 begin()/end()、沒有邊界檢查版本的存取,
//       所以無法直接餵給 STL 演算法,也無法用 range-based for 以外的泛型寫法。
//
// std::array 的做法非常聰明:它不是「重新發明陣列」,而是把 C 陣列**原封不動
// 包進一個 struct**,再補上成員函式。因為包了一層 class,它自然就有了
// value semantics(可指派、可回傳、可放進其他容器)、不會 decay、而且有完整
// 的容器介面 —— 但底層存的還是那塊連續的 C 陣列,一個 byte 都沒多。
//
// 【2. 「零開銷抽象」是什麼意思:用 sizeof 驗證】
// C++ 的核心信條之一是 zero-overhead abstraction:你沒用到的東西不該付出代價,
// 你用到的東西不會比手寫更慢。std::array 是這條原則最乾淨的示範。
//
//   本機實測(GCC 15.2.0 / x86-64):
//     sizeof(std::array<int,5>) == 20 == sizeof(int[5])
//
// 完全相等,沒有多存一個 size 欄位、沒有多一個指標、沒有 vtable。為什麼不需要
// 存 size?因為 N 是**模板參數**,它是型別的一部分,在編譯期就已知,size() 只要
// 直接 return N 即可,連讀記憶體都不必。這也代表 std::array 的物件**完全放在
// stack 上(或所在物件內部)**,建構時沒有任何 heap 配置 —— 這點和 vector 是
// 根本性的差異。
//
// 【3. N 是型別的一部分:威力與代價】
// std::array<int,5> 和 std::array<int,6> 是**兩個毫不相干的型別**,不能互相指派,
// 也不能放進同一個 std::vector。這帶來:
//   * 威力:大小錯誤在編譯期就被抓到,而不是執行期才炸。size() 是 constexpr,
//           可以拿去當另一個模板的參數、可以放進 static_assert。
//   * 代價:大小必須在編譯期就確定,執行期才知道長度的資料**不能用 array**,
//           那是 vector 的地盤。且若你寫一個吃 array 的函式,換個長度就得再
//           實例化一份程式碼(template bloat)。
//
// 【4. at() 與 operator[]:兩種錯誤哲學】
// 這兩個介面刻意提供了不同的安全/效能取捨,不是二選一的「好壞」關係:
//   * operator[]:不做任何邊界檢查。越界是 undefined behavior。它的存在是為了
//     「我已經用迴圈保證 index 合法了,不要再幫我檢查一次」的熱路徑。
//   * at():每次都比對 n < N,越界則丟出 std::out_of_range。適合 index 來自
//     外部輸入(使用者、設定檔、網路封包)而你無法事先保證其合法性的場合。
// 慣例是:index 由你自己的迴圈產生就用 [],index 來自不可信來源就用 at()。
//
// 【5. 聚合初始化與那組「雙層大括號」】
// std::array 是 aggregate(沒有使用者自訂建構子),所以用 aggregate initialization
// 初始化。因為它內部包的是一個 C 陣列成員,嚴格來說要寫成:
//     std::array<int,5> a = {{10,20,30,40,50}};   // 內層對應內部的 C 陣列
// 但標準允許 brace elision(省略內層大括號),所以日常寫單層即可:
//     std::array<int,5> a = {10,20,30,40,50};     // 實務上都這樣寫
// 另外要記得:std::array **沒有** initializer_list 建構子(它根本沒有建構子),
// 這也是為什麼 std::array<int,5> a(5, 0) 這種 vector 式的寫法不存在。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 記憶體佈局:它真的就是一個 struct 包 C 陣列
//     libstdc++ 的定義簡化後長這樣:
//         template<typename T, size_t N>
//         struct array {
//             T _M_elems[N];          // 唯一的資料成員
//             constexpr size_t size() const noexcept { return N; }
//             constexpr T* data() noexcept { return _M_elems; }
//             // ...
//         };
//     沒有 private、沒有建構子、沒有解構子 —— 所以它是 aggregate、是
//     trivially copyable、可以放進 constexpr、可以 memcpy。這也解釋了為什麼
//     sizeof 剛好等於 N*sizeof(T)(在 T 無額外對齊需求時;對齊規則由 T 決定,
//     屬實作定義)。
//
// (B) N == 0 的特例
//     std::array<int,0> 是合法的,empty() 回傳 true,size() 回傳 0。標準要求
//     它仍然可以被建立,且 begin() == end()。但 sizeof 不可能是 0(C++ 規定
//     完整型別的 sizeof 至少為 1),實作會塞一個 dummy 成員,實際大小屬實作定義。
//     重點是:對 size 為 0 的 array 呼叫 front()、back() 或 data()[0] 是
//     undefined behavior,絕不能寫。
//
// (C) 為什麼 std::array 沒有 allocator 參數
//     vector、list、map 都有 Allocator 模板參數,array 沒有。因為 array 從不
//     配置記憶體 —— 元素就內嵌在物件本身裡。物件放哪(stack、全域區、還是
//     某個 heap 物件的成員),由使用它的人決定,和 array 自己無關。
//
// (D) 編譯器實際做了什麼
//     由於 size() 回傳的是編譯期常數,for (size_t i=0;i<a.size();++i) 這種迴圈
//     的邊界在最佳化後是立即數,編譯器可以直接展開迴圈或向量化。用 -O2 編譯時,
//     走 std::array 和走原生 C 陣列產生的機器碼通常**完全一樣**。這就是
//     「零開銷」的實證含義:抽象只存在於原始碼層,不存在於機器碼層。
//
// 【注意事項 Pay Attention】
//  1. operator[] 越界是 undefined behavior。它**可能**讀到旁邊的變數、可能觸發
//     segfault、也可能看起來完全正常地跑完 —— 行為不保證、也不可預測,絕不能
//     拿「跑起來沒事」當作程式正確的證據。需要保證檢查請改用 at()。
//  2. 大型 array 放在函式內是 stack 物件。std::array<int, 1000000> 約 4 MB,
//     多數平台預設 stack 只有 1~8 MB,很可能直接 stack overflow。大量資料請用
//     vector(資料在 heap)。
//  3. sizeof(std::array<int,5>) == 20 是本機實測值,對齊與 padding 規則屬
//     **實作定義**;在 T 有特殊對齊需求時不保證等於 N*sizeof(T)。
//  4. std::array 不會 decay,所以傳進函式後 size() 仍然正確 —— 但也因此
//     不能隱式轉成 T*。需要 C API 相容時請明確呼叫 .data()。
//  5. 別把 array 的 size() 當成「有幾個有效元素」。array 永遠有 N 個元素,
//     它們在你寫入前是**預設初始化**的;對 int 這類 trivial 型別,若寫成
//     std::array<int,5> a;(無初始器)其值是不確定的,讀取未初始化的值是 UB。
//     寫成 std::array<int,5> a{}; 才會全部零初始化。
//  6. std::array<int,5> 與 std::array<int,6> 是不同型別,無法互相指派,
//     也無法放進同一個容器 —— 這是設計,不是缺陷。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::array
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::array 和 C 原生陣列有什麼差別?為什麼說它是「零開銷」?
//     答：介面上,array 有 size()/begin()/end()/at(),是一等公民(可指派、
//         可回傳、可放進其他容器),而且傳參時**不會 decay 成指標**,長度資訊
//         得以保留。記憶體上兩者完全相同 —— 本機實測 sizeof(std::array<int,5>)
//         == 20 == sizeof(int[5]),因為 N 是模板參數、編譯期已知,不需要額外
//         存一個 size 欄位。所以是「介面全拿、成本不付」的零開銷抽象。
//     追問：那為什麼還需要 std::vector?
//         → 因為 array 的長度必須編譯期確定且資料在 stack 上;
//           執行期才知道長度、或資料量大到不適合放 stack 時,只能用 vector。
//
// 🔥 Q2. std::array<int,5> 和 std::array<int,6> 是同一個型別嗎?這帶來什麼影響?
//     答：不是,是兩個完全不同的型別。N 是模板參數,參與型別的組成。
//         影響有兩面:好處是大小不符在編譯期就被擋下,且 size() 是 constexpr,
//         可用於 static_assert 或當作別的模板參數;壞處是無法把不同長度的
//         array 放進同一個 vector,而且每種長度都會實例化一份程式碼。
//     追問：那要寫一個能吃任意長度 array 的函式怎麼辦?
//         → 把 N 做成模板參數 template<size_t N> void f(const std::array<int,N>&),
//           或更泛型地改吃 std::span(C++20)/一對 iterator。
//
// ⚠️ 陷阱. std::array<int,5> a; 之後直接讀 a[0],值是多少?
//     答：不確定。這種寫法是 default-initialization,對 int 這類 trivial 型別
//         等於**不初始化**,元素值是不確定的,讀取它是 undefined behavior。
//         要全部歸零必須寫 std::array<int,5> a{};(value-initialization)。
//     為什麼會錯：多數人把 std::array 想成「像 vector 一樣會自己初始化」。
//         但 vector 會零初始化是因為它有建構子;std::array 是 aggregate、
//         **沒有任何建構子**,行為完全比照內嵌的 C 陣列 —— 而 int arr[5];
//         在函式內本來就是未初始化的。
//
// ⚠️ 陷阱. 既然 at() 比較安全,是不是應該全部改用 at() 就好?
//     答：不是。at() 每次存取都要多做一次比較與潛在的例外路徑,在已能保證
//         index 合法的熱迴圈裡是白付成本;而且用例外處理「本來就不該發生」的
//         程式邏輯錯誤,會把 bug 變成可被吞掉的執行期狀態,反而更難除錯。
//         慣例:index 由自己的迴圈產生 → operator[];index 來自外部不可信輸入
//         → at() 或事先手動驗證。
//     為什麼會錯：把「有檢查」直接等同於「比較好」,忽略了 C++ 讓你自己選擇
//         安全與效能取捨的設計哲學 —— 兩個介面並存正是因為兩種需求都合理。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <array>
#include <string>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 242. Valid Anagram
//   題目：判斷字串 t 是不是 s 的字母重排(anagram),兩字串僅含小寫英文字母。
//   為什麼用到本主題：字母只有 26 個 —— 這是典型「大小在編譯期就固定」的計數
//     需求,正是 std::array 的主場。用 std::array<int,26> 完全不碰 heap,
//     比 unordered_map<char,int> 快上一個數量級,而且 cache 極友善。
//   複雜度：時間 O(n),空間 O(1)(26 是常數)。
// -----------------------------------------------------------------------------
bool isAnagram(const std::string& s, const std::string& t) {
    if (s.size() != t.size()) return false;

    std::array<int, 26> count{};          // {} 很重要:value-initialization → 全部歸零
    for (char c : s) ++count[static_cast<size_t>(c - 'a')];
    for (char c : t) {
        size_t idx = static_cast<size_t>(c - 'a');
        if (--count[idx] < 0) return false;   // 出現次數比 s 多 → 不是 anagram
    }
    return true;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】把 HTTP 狀態碼歸類成 1xx~5xx 的語意分類
//   情境：寫存取日誌(access log)或監控儀表板時,常要把 200/301/404/500 這種
//     原始狀態碼收斂成「成功 / 重導 / 用戶端錯誤 / 伺服器錯誤」再做統計。
//   為什麼用到本主題：分類只有固定 6 種(index 0~5),表格內容編譯期就完全確定
//     —— 用 std::array 當查表是最省的做法:沒有 heap 配置、查表 O(1)、
//     而且 constexpr 讓整張表可以直接躺在唯讀區段。
// -----------------------------------------------------------------------------
const char* httpStatusClass(int code) {
    // index 0 保留給「非法狀態碼」,1~5 對應 1xx~5xx
    static constexpr std::array<const char*, 6> kNames = {
        "INVALID", "Informational", "Success", "Redirection",
        "ClientError", "ServerError"
    };
    int cls = code / 100;
    if (cls < 1 || cls > 5) cls = 0;      // 邊界防護:不合法就落到 index 0
    return kNames[static_cast<size_t>(cls)];
}

int main() {
    // ── 原始課堂示範:std::array 的基本操作 ────────────────────────────────
    // array：大小在編譯期固定
    std::array<int, 5> arr = {10, 20, 30, 40, 50};

    std::cout << "=== std::array ===" << std::endl;
    std::cout << "大小: " << arr.size() << std::endl;
    std::cout << "第一個: " << arr.front() << std::endl;
    std::cout << "最後一個: " << arr.back() << std::endl;
    std::cout << "arr[2]: " << arr[2] << std::endl;
    std::cout << "arr.at(2): " << arr.at(2) << std::endl;  // 有邊界檢查

    // 遍歷
    std::cout << "所有元素: ";
    for (int n : arr) {
        std::cout << n << " ";
    }
    std::cout << std::endl;

    // ── 零開銷抽象的實證 ──────────────────────────────────────────────────
    std::cout << "\n=== 零開銷抽象實證 ===" << std::endl;
    std::cout << "sizeof(std::array<int,5>) = " << sizeof(std::array<int, 5>) << std::endl;
    std::cout << "sizeof(int[5])            = " << sizeof(int[5]) << std::endl;
    std::cout << "兩者相等 → array 沒有任何額外開銷" << std::endl;
    // size() 是編譯期常數,可以拿去做 static_assert
    static_assert(arr.size() == 5, "array 的 size() 在編譯期就已知");

    // ── at() 的邊界檢查 ───────────────────────────────────────────────────
    std::cout << "\n=== at() 邊界檢查 ===" << std::endl;
    try {
        std::cout << "arr.at(10) → ";
        std::cout << arr.at(10) << std::endl;   // 越界 → 丟例外
    } catch (const std::out_of_range& e) {
        std::cout << "捕捉到 std::out_of_range: " << e.what() << std::endl;
    }
    // 注意:arr[10] 是 undefined behavior,不保證會丟例外或崩潰,故此處不示範。

    // ── 初始化差異 ────────────────────────────────────────────────────────
    std::cout << "\n=== 初始化差異 ===" << std::endl;
    std::array<int, 5> zeroed{};            // value-initialization → 全部為 0
    std::cout << "std::array<int,5> a{}; 的內容: ";
    for (int n : zeroed) std::cout << n << " ";
    std::cout << "(保證全 0)" << std::endl;
    std::cout << "std::array<int,5> a;  的內容: 不確定 —— 讀取未初始化的值是 UB,故不示範" << std::endl;

    // ── LeetCode 242 ──────────────────────────────────────────────────────
    std::cout << "\n=== LeetCode 242. Valid Anagram ===" << std::endl;
    std::cout << "anagram(\"anagram\", \"nagaram\") = "
              << std::boolalpha << isAnagram("anagram", "nagaram") << std::endl;
    std::cout << "anagram(\"rat\", \"car\")         = "
              << isAnagram("rat", "car") << std::endl;

    // ── 日常實務:HTTP 狀態碼分類 ─────────────────────────────────────────
    std::cout << "\n=== 日常實務: HTTP 狀態碼分類 ===" << std::endl;
    for (int code : {200, 301, 404, 500, 999}) {
        std::cout << "  " << code << " → " << httpStatusClass(code) << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第六課：容器（Container）的概念與分類1.cpp" -o array_demo

// === 預期輸出 ===
// === std::array ===
// 大小: 5
// 第一個: 10
// 最後一個: 50
// arr[2]: 30
// arr.at(2): 30
// 所有元素: 10 20 30 40 50
//
// === 零開銷抽象實證 ===
// sizeof(std::array<int,5>) = 20
// sizeof(int[5])            = 20
// 兩者相等 → array 沒有任何額外開銷
//
// === at() 邊界檢查 ===
// arr.at(10) → 捕捉到 std::out_of_range: array::at: __n (which is 10) >= _Nm (which is 5)
//
// === 初始化差異 ===
// std::array<int,5> a{}; 的內容: 0 0 0 0 0 (保證全 0)
// std::array<int,5> a;  的內容: 不確定 —— 讀取未初始化的值是 UB,故不示範
//
// === LeetCode 242. Valid Anagram ===
// anagram("anagram", "nagaram") = true
// anagram("rat", "car")         = false
//
// === 日常實務: HTTP 狀態碼分類 ===
//   200 → Success
//   301 → Redirection
//   404 → ClientError
//   500 → ServerError
//   999 → INVALID
