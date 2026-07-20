/*
 * ================================================================
 * 【第10課：vector 的宣告與初始化方式】總複習 summary.cpp
 * ================================================================
 * 編譯方式：g++ -std=c++17 -o summary summary.cpp
 * 本課重點：
 * 1. 預設建構（空 vector）
 * 2. 指定大小建構：vector(n) 和 vector(n, val)
 * 3. 初始化串列建構：{1, 2, 3}
 * 4. 小括號 () vs 大括號 {} 的關鍵差異（陷阱！）
 * 5. 從其他容器或迭代器範圍建構
 * 6. 移動建構：std::move
 * 7. assign 重新初始化
 * 8. 自訂類別的 vector 初始化
 * 9. C++17 類別模板引數推導（CTAD）
 * ================================================================
 */

// =============================================================================
//  第 10 課 總複習  —  vector 的宣告與初始化方式
// =============================================================================
//
// 【主題資訊 Information】
//   本課涵蓋 std::vector 的建構方式（<vector>）：
//     vector()                          預設建構，size=0 capacity=0
//     vector(n)                         n 個「值初始化」的元素
//     vector(n, val)                    n 個 val
//     vector(init_list)                 C++11，逐一列舉元素
//     vector(other)                     複製建構，O(n)
//     vector(std::move(other))          移動建構，O(1)
//     vector(first, last)               迭代器範圍，半開區間
//     assign(...)                       重設既有 vector 的內容
//     std::vector v = {…}               C++17 CTAD
//   複雜度：除移動建構是 O(1) 外，其餘皆為 O(n)。
//
// 【詳細解釋 Explanation】
//
// 【1. 小括號 vs 大括號：本課唯一必須背下來的規則】
//     vector<int> a(5, 10);   // 五個 10       —— 走「數量+值」建構子
//     vector<int> b{5, 10};   // 兩個元素 5,10 —— 走 initializer_list 建構子
//   規則本身是「只要類別有 initializer_list 建構子，大括號就【優先】選它，
//   幾乎不惜代價」。這是 C++11 刻意的設計：讓 `{...}` 直覺地代表元素清單。
//   代價就是 vector 這種「建構子參數剛好也是數字」的類別會出現歧義。
//   記憶法：小括號問的是「怎麼做」，大括號問的是「裝什麼」。
//
// 【2. vector(n) 的元素是「值初始化」而不是「未初始化」】
//     vector<int> v(5);      // 五個 0，不是五個垃圾值
//   這和原生陣列 `int arr[5];`（區域變數時內容未定義）完全不同。
//   vector 一律對元素做值初始化：內建型別歸零，類別型別呼叫預設建構子。
//   代價是「明明馬上要覆寫，卻先付一次歸零成本」——
//   要避開它應該用 `vector<int> v; v.reserve(5);`，
//   那才是「配置了空間但還沒有元素」。
//
// 【3. reserve 與 resize 是兩件不同的事】
//     reserve(n) —— 只動 capacity，size 不變。之後 push_back 不會擴容。
//     resize(n)  —— 動 size，真的建構/銷毀元素。
//   本機 libstdc++ 實測（實作定義）：
//     * reserve(n) 配置的是【剛好 n】，不會多要（不做向上取整）。
//       所以在迴圈裡寫 reserve(size()+1) 會徹底破壞幾何成長，
//       讓 push_back 從攤還 O(1) 退化成每次都重新配置 → 比不寫還慢。
//     * resize(n) 走的是成長公式 max(2*size, n)。
//       所以一個 capacity=5、size=5 的 vector 呼叫 resize(8)，
//       最終 capacity 是 10（不是 8）—— 本機實測確認。
//   兩者都不會縮小容量。
//
// 【4. 移動建構為什麼是 O(1)】
//   vector 物件本身只有三個指標（begin / end / capacity_end），
//   本機實測 sizeof(std::vector<int>) == 24（64-bit，三個 8 bytes 指標）。
//   元素資料在堆積上。移動就是把這三個指標搬過去、再把來源設成空——
//   完全不碰元素，所以與元素個數無關，是 O(1)。
//   移動後來源處於「有效但未指定」狀態：標準只保證你可以安全地
//   對它賦值或解構，不保證它一定是空的（實務上 libstdc++ 會是空的，
//   但不該依賴）。
//
// 【概念補充 Concept Deep Dive】
//
// (A) vector 的成長倍率是實作定義的
//     libstdc++：2×（本機實測容量序列 1→2→4→8→16→32→64）
//     MSVC     ：1.5×
//     標準只規定 push_back 的攤還複雜度是 O(1)，沒有規定倍率。
//     所以任何「vector 一定是兩倍成長」的說法都是錯的，
//     只能說「本機這個實作是 2×」。
//     為什麼要幾何成長：若每次只 +1，插入 n 個元素的總搬移量是
//     1+2+…+n = O(n²)；幾何成長讓總搬移量收斂成 O(n)。
//
// (B) 為什麼 1.5× 有時被認為比 2× 好
//     用 2× 時，新配置的區塊永遠大於「先前所有已釋放區塊的總和」
//     （1+2+4 < 8），所以釋放出來的空間永遠無法被重複利用。
//     1.5× 則在幾次成長後，累積釋放的空間足以容納新的請求，
//     對記憶體配置器比較友善。兩者都滿足攤還 O(1)。
//
// (C) shrink_to_fit 是「非約束性請求」
//     標準的用詞是 non-binding request：實作可以完全忽略它。
//     本機 libstdc++ 確實會縮（實測 capacity 1000 → 3），
//     但這不是保證，不能寫成「呼叫它一定會縮容」。
//     真正保證釋放記憶體的做法是 swap 慣用法：
//         std::vector<int>(v).swap(v);       // C++11 前的標準技巧
//     它建立一個剛好大小的臨時複本再交換，強制丟掉多餘容量。
//
// 【注意事項 Pay Attention】
//   1. vector<int> v{5, 10} 是兩個元素，不是五個 10。這是本課第一大坑。
//   2. reserve(n) 在本機配置剛好 n；別在迴圈裡呼叫 reserve(size()+1)，
//      那會破壞幾何成長、比什麼都不做更慢。
//   3. 成長倍率、reserve 是否向上取整、shrink_to_fit 是否真的縮容，
//      全部都是實作定義，不要當成標準保證。
//   4. initializer_list 只能複製不能移動（元素型別是 const T）。
//      大量含堆積資源的元素請用 reserve + emplace_back。
//   5. 移動後的來源是「有效但未指定」狀態；除了賦值與解構之外，
//      不要對它做任何假設。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】vector 的建構與容量
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. `std::vector<int> a(5, 10);` 和 `std::vector<int> b{5, 10};`
//        分別產生什麼？為什麼？
//     答：a 是五個 10（size=5），b 是兩個元素 5 和 10（size=2）。
//         因為只要類別提供了 initializer_list 建構子，
//         大括號初始化就會優先選它；小括號則走一般的重載解析，
//         匹配到 vector(count, value)。
//     追問：那 std::vector<std::string> c{5, 10}; 呢？
//         → 編譯失敗。走 initializer_list<string> 時 5 和 10 無法
//           轉成 string；退回 (count, value) 又因為大括號不允許
//           窄化與這種轉換而失敗。
//
// 🔥 Q2. vector 的成長倍率是多少？這是標準規定的嗎？
//     答：標準只規定 push_back 的【攤還】複雜度是 O(1)，
//         完全沒有規定倍率。實際倍率是實作定義：
//         libstdc++ 是 2×（本機實測 1→2→4→8→16→32→64），
//         MSVC 是 1.5×。
//         必須用幾何成長的理由：若每次 +1，插入 n 個元素的
//         總搬移量是 O(n²)，幾何成長才能收斂成 O(n)。
//     追問：為什麼有人主張 1.5× 比 2× 好？
//         → 2× 時新請求永遠大於先前所有已釋放區塊的總和
//           （1+2+4 < 8），釋放的空間永遠用不上；
//           1.5× 幾次之後就能重複利用，對配置器較友善。
//
// ⚠️ 陷阱. 「我知道要 reserve 才有效率，所以我在 push_back 前面
//         都先呼叫 v.reserve(v.size() + 1)，確保空間一定夠。」
//         這樣寫為什麼反而更慢？
//     答：因為 libstdc++ 的 reserve(n) 配置的是【剛好 n】，不做向上取整。
//         於是每一輪迴圈 capacity 都只比 size 多 1，
//         下一次 push_back 又滿了，就再配置一次——
//         幾何成長被徹底破壞，push_back 從攤還 O(1) 退化成
//         每次都重新配置 + 搬移全部元素，總成本變 O(n²)。
//         這比完全不呼叫 reserve 還慢。
//         reserve 的正確用法是【在迴圈外呼叫一次】，
//         參數是最終總量（或其上界）。
//     為什麼會錯：把 reserve 理解成「確保夠用的保險」，
//         於是每次都保險一下。但 reserve 是「一次告訴 vector
//         你最終要多大」的最佳化提示，不是逐次的安全檢查——
//         push_back 本來就會自己確保空間夠用。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <array>
#include <string>
#include <utility>  // std::move

// ===== 重點一：預設建構 — 空的 vector =====
// 三種等效寫法，建立不含任何元素的 vector（size=0, capacity=0）

void demo_default_construction() {
    std::cout << "\n===== 重點一：預設建構（空 vector）=====\n";

    std::vector<int>         v1;        // 最常見寫法
    std::vector<double>      v2{};      // C++11 統一初始化語法
    std::vector<std::string> v3 = {};   // 明確空的初始化串列

    std::cout << "v1 size: " << v1.size() << ", capacity: " << v1.capacity() << std::endl;
    std::cout << "v2 size: " << v2.size() << ", capacity: " << v2.capacity() << std::endl;
    std::cout << "v3 size: " << v3.size() << ", capacity: " << v3.capacity() << std::endl;
    // 三者均輸出 size: 0, capacity: 0
}

// ===== 重點二：指定大小的建構 =====
// vector(n)     → 建立含 n 個「預設值」元素（int 預設為 0）
// vector(n, v)  → 建立含 n 個值均為 v 的元素
// 注意：這裡用的是小括號 ()，不是大括號 {}

void demo_size_construction() {
    std::cout << "\n===== 重點二：指定大小建構 =====\n";

    // 5 個元素，每個都是 int 的預設值（0）
    std::vector<int> v1(5);
    std::cout << "v1(5): ";
    for (int x : v1) std::cout << x << " ";  // 0 0 0 0 0
    std::cout << std::endl;

    // 5 個元素，每個值為 42
    std::vector<int> v2(5, 42);
    std::cout << "v2(5, 42): ";
    for (int x : v2) std::cout << x << " ";  // 42 42 42 42 42
    std::cout << std::endl;

    // 3 個預設建構的 string（空字串）
    std::vector<std::string> v3(3);
    std::cout << "v3(3) size: " << v3.size() << std::endl;  // 3
}

// ===== 重點三：初始化串列（Initializer List）=====
// C++11 引入的大括號初始化，可以直接列出元素值

void demo_initializer_list() {
    std::cout << "\n===== 重點三：初始化串列 =====\n";

    std::vector<int> v1 = {1, 2, 3, 4, 5};  // 明確的初始化串列
    std::vector<int> v2{1, 2, 3, 4, 5};      // 同上，省略等號
    std::vector<int> v3({1, 2, 3, 4, 5});    // 同上，較少人這樣寫

    std::cout << "v1: ";
    for (int x : v1) std::cout << x << " ";  // 1 2 3 4 5
    std::cout << std::endl;

    std::cout << "v1.size(): " << v1.size() << std::endl;  // 5
    std::cout << "v2 和 v3 效果相同\n";
}

// ===== 重點四：小括號 vs 大括號的陷阱 ★★★ =====
// 這是最容易混淆的地方！
//
// 規則：
//   () 呼叫建構子
//       vector<int>(5, 10) 意思是「5 個元素，值為 10」
//   {} 優先嘗試初始化串列
//       vector<int>{5, 10} 意思是「元素是 5 和 10」（兩個元素）
//
// 記憶訣竅：
//   () → 建構子參數（size, value）
//   {} → 元素清單（逐一列舉）

void demo_parentheses_vs_braces() {
    std::cout << "\n===== 重點四：小括號 vs 大括號（重要！）=====\n";

    std::vector<int> v1(5, 10);   // 5 個元素，每個都是 10
    std::vector<int> v2{5, 10};   // 2 個元素：5 和 10

    std::cout << "v1(5, 10) size: " << v1.size() << std::endl;  // 5
    std::cout << "v1: ";
    for (int x : v1) std::cout << x << " ";  // 10 10 10 10 10
    std::cout << std::endl;

    std::cout << "v2{5, 10} size: " << v2.size() << std::endl;  // 2
    std::cout << "v2: ";
    for (int x : v2) std::cout << x << " ";  // 5 10
    std::cout << std::endl;

    // 更多範例
    std::vector<int> a(3);    // {0, 0, 0}  3 個預設元素
    std::vector<int> b{3};    // {3}        1 個元素，值為 3
    std::vector<int> c(3, 7); // {7, 7, 7}  3 個 7
    std::vector<int> d{3, 7}; // {3, 7}     2 個元素

    std::cout << "a(3) size: " << a.size() << " 內容:";
    for (int x : a) std::cout << " " << x;
    std::cout << std::endl;

    std::cout << "b{3} size: " << b.size() << " 內容:";
    for (int x : b) std::cout << " " << x;
    std::cout << std::endl;
}

// ===== 重點五：從其他容器或迭代器範圍建構 =====
// vector 可以從任何提供迭代器的容器（或 C 風格陣列）建構

void demo_range_construction() {
    std::cout << "\n===== 重點五：從範圍/其他容器建構 =====\n";

    // 從另一個 vector 複製建構
    std::vector<int> original = {1, 2, 3, 4, 5};
    std::vector<int> copy1(original);       // 複製建構
    std::vector<int> copy2 = original;      // 同上（複製賦值初始化）

    // 從迭代器範圍建構（取部分元素）
    std::vector<int> partial(original.begin() + 1, original.begin() + 4);
    std::cout << "partial: ";
    for (int x : partial) std::cout << x << " ";  // 2 3 4
    std::cout << std::endl;

    // 從 C 風格陣列建構
    int arr[] = {10, 20, 30, 40};
    std::vector<int> from_array(std::begin(arr), std::end(arr));
    // 或者：std::vector<int> from_array(arr, arr + 4);
    std::cout << "from_array: ";
    for (int x : from_array) std::cout << x << " ";  // 10 20 30 40
    std::cout << std::endl;

    // 從 std::array 建構
    std::array<int, 3> std_arr = {100, 200, 300};
    std::vector<int> from_std_array(std_arr.begin(), std_arr.end());
    std::cout << "from_std_array: ";
    for (int x : from_std_array) std::cout << x << " ";  // 100 200 300
    std::cout << std::endl;
}

// ===== 重點六：移動建構 =====
// 當來源 vector 不再需要時，可以「移動」而非「複製」。
// 移動操作只轉移內部指標的所有權，不複製元素，效率極高（O(1)）。
// 移動後，source 處於「有效但未指定」狀態，通常是空的。

void demo_move_construction() {
    std::cout << "\n===== 重點六：移動建構 =====\n";

    std::vector<int> source = {1, 2, 3, 4, 5};
    std::cout << "移動前 source.size(): " << source.size() << std::endl;  // 5

    std::vector<int> dest = std::move(source);  // 移動建構，O(1)

    std::cout << "移動後 source.size(): " << source.size() << std::endl;  // 通常是 0
    std::cout << "移動後 dest.size():   " << dest.size()   << std::endl;  // 5

    // 警告：移動後不應再使用 source 的元素（狀態未定義）
    // 但 source 仍然是合法物件，可以重新 assign
    source = {10, 20};
    std::cout << "重新賦值後 source.size(): " << source.size() << std::endl;  // 2
}

// ===== 重點七：assign 重新初始化已存在的 vector =====
// assign 會清空現有內容，重新設定 vector 的元素

void demo_assign() {
    std::cout << "\n===== 重點七：assign 重新初始化 =====\n";

    std::vector<int> v = {1, 2, 3};

    // 方法一：指定數量和值
    v.assign(5, 100);
    std::cout << "assign(5, 100): ";
    for (int x : v) std::cout << x << " ";  // 100 100 100 100 100
    std::cout << std::endl;

    // 方法二：從初始化串列
    v.assign({10, 20, 30});
    std::cout << "assign({10, 20, 30}): ";
    for (int x : v) std::cout << x << " ";  // 10 20 30
    std::cout << std::endl;

    // 方法三：從迭代器範圍
    std::vector<int> other = {7, 8, 9, 10, 11};
    v.assign(other.begin() + 1, other.end() - 1);
    std::cout << "從迭代器範圍 assign: ";
    for (int x : v) std::cout << x << " ";  // 8 9 10
    std::cout << std::endl;
}

// ===== 重點八：自訂類別的 vector 初始化 =====
// 使用大括號初始化串列，每個元素會呼叫對應建構子

struct Person {
    std::string name;
    int age;
    Person(const std::string& n, int a) : name(n), age(a) {}
};

void demo_custom_class() {
    std::cout << "\n===== 重點八：自訂類別的初始化 =====\n";

    // 使用初始化串列（需要隱式轉換或大括號建構）
    std::vector<Person> people = {
        {"Alice",   30},
        {"Bob",     25},
        {"Charlie", 35}
    };

    for (const auto& p : people) {
        std::cout << "  " << p.name << " is " << p.age << " years old.\n";
    }
}

// ===== 重點九：C++17 類別模板引數推導（CTAD）=====
// C++17 開始，某些情況下可省略模板參數，由編譯器自動推導

void demo_ctad() {
    std::cout << "\n===== 重點九：C++17 CTAD 自動推導 =====\n";

    // C++17 之前必須寫：std::vector<int> v = {1, 2, 3};
    // C++17 可以讓編譯器推導型別：
    std::vector v1 = {1, 2, 3};         // 推導為 vector<int>
    std::vector v2 = {1.5, 2.5, 3.5};  // 推導為 vector<double>

    // 從迭代器推導
    std::vector<int> source = {10, 20, 30};
    std::vector v3(source.begin(), source.end());  // 推導為 vector<int>

    std::cout << "v1 型別推導為 vector<int>，size: " << v1.size() << std::endl;
    std::cout << "v2 型別推導為 vector<double>，size: " << v2.size() << std::endl;
    std::cout << "v3 從迭代器推導，size: " << v3.size() << std::endl;

    // 注意：型別不明顯時，建議還是明確寫出型別
}

// ===== 各種初始化方式對照表 =====
// | 語法                           | 結果              | 說明             |
// |--------------------------------|-------------------|------------------|
// | vector<int> v;                 | 空 vector         | 預設建構         |
// | vector<int> v(5);              | {0,0,0,0,0}       | 5 個預設值元素   |
// | vector<int> v(5, 42);          | {42,42,42,42,42}  | 5 個值為 42      |
// | vector<int> v{5, 42};          | {5, 42}           | 2 個元素：5 和 42|
// | vector<int> v = {1,2,3};       | {1, 2, 3}         | 初始化串列       |
// | vector<int> v(other);          | 複製 other        | 複製建構         |
// | vector<int> v(std::move(other))| 接收 other 的資源 | 移動建構         |
// | vector<int> v(it1, it2);       | 範圍 [it1, it2)   | 迭代器範圍建構   |

// ===== 重點十：容量行為實測（成長倍率 / reserve / resize）=====
// 本節的所有數值都是【實作定義】：本機是 g++ 15.2 + libstdc++。
// 換成 MSVC 或 libc++，數字會不一樣，但「幾何成長」的原則相同。
void demo_capacity_behavior() {
    std::cout << "\n===== 重點十：容量行為（實作定義，本機 libstdc++）=====\n";

    // (1) push_back 的成長序列
    std::vector<int> g;
    size_t last = g.capacity();
    std::cout << "push_back 的 capacity 序列: " << last;
    for (int i = 0; i < 70; ++i) {
        g.push_back(i);
        if (g.capacity() != last) {
            last = g.capacity();
            std::cout << " -> " << last;
        }
    }
    std::cout << "\n  → libstdc++ 是 2× 成長；MSVC 是 1.5×。\n";
    std::cout << "    標準只要求 push_back 攤還 O(1)，沒有規定倍率。\n";

    // (2) reserve 配置的是「剛好 n」，不做向上取整
    std::vector<int> r;
    r.reserve(7);
    std::cout << "reserve(7)   後 capacity=" << r.capacity() << "（剛好 7，沒有進位）\n";
    r.reserve(100);
    std::cout << "reserve(100) 後 capacity=" << r.capacity() << "\n";
    r.reserve(50);
    std::cout << "reserve(50)  後 capacity=" << r.capacity() << "（比目前小 → 不動作）\n";

    // (3) resize 走的是成長公式 max(2*size, n)
    std::vector<int> q;
    q.reserve(5);
    q.resize(5);
    std::cout << "\nresize 前: size=" << q.size() << " capacity=" << q.capacity() << "\n";
    q.resize(8);
    std::cout << "resize(8) 後: size=" << q.size() << " capacity=" << q.capacity()
              << "\n  → 不是 8！因為公式是 max(2*size, n) = max(10, 8) = 10\n";

    // (4) 反面教材：在迴圈裡 reserve(size()+1)
    std::vector<int> bad, good;
    size_t badReallocs = 0, goodReallocs = 0;
    size_t bc = bad.capacity(), gc = good.capacity();

    for (int i = 0; i < 1000; ++i) {
        bad.reserve(bad.size() + 1);      // ← 反面教材
        bad.push_back(i);
        if (bad.capacity() != bc) { bc = bad.capacity(); ++badReallocs; }
    }
    for (int i = 0; i < 1000; ++i) {
        good.push_back(i);                 // 什麼都不做，讓它自己幾何成長
        if (good.capacity() != gc) { gc = good.capacity(); ++goodReallocs; }
    }

    std::cout << "\n塞 1000 個元素的重新配置次數:\n";
    std::cout << "  每次 reserve(size()+1) : " << badReallocs << " 次  ← 幾何成長被破壞\n";
    std::cout << "  什麼都不做（純 push_back）: " << goodReallocs << " 次\n";
    std::cout << "  → reserve 該在迴圈【外】呼叫一次，參數是最終總量。\n";

    std::vector<int> best;
    best.reserve(1000);
    size_t brc = best.capacity();
    size_t bestReallocs = 0;
    for (int i = 0; i < 1000; ++i) {
        best.push_back(i);
        if (best.capacity() != brc) { brc = best.capacity(); ++bestReallocs; }
    }
    std::cout << "  迴圈外 reserve(1000)     : " << bestReallocs << " 次（配置一次就到位）\n";

    // (5) shrink_to_fit 是非約束性請求
    std::vector<int> s(1000, 1);
    s.resize(3);
    std::cout << "\nresize(3) 後 size=" << s.size() << " capacity=" << s.capacity() << "\n";
    s.shrink_to_fit();
    std::cout << "shrink_to_fit() 後 capacity=" << s.capacity() << "\n";
    std::cout << "  → 本機確實縮了，但標準說它是 non-binding request，\n";
    std::cout << "    實作可以完全忽略。不能寫成「一定會縮容」。\n";

    std::cout << "\nsizeof(std::vector<int>) = " << sizeof(std::vector<int>)
              << " bytes（三個指標：begin / end / capacity_end）\n";
    std::cout << "  → 這就是移動建構能做到 O(1) 的原因：只搬這三個指標。\n";
}

// -----------------------------------------------------------------------------
// (以下為本課的 LeetCode 範例)
// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 118. Pascal's Triangle
//   題目：給定 numRows，回傳楊輝三角形的前 numRows 列。
//         numRows=5 -> [[1],[1,1],[1,2,1],[1,3,3,1],[1,4,6,4,1]]
//   為什麼用到本主題：這題的核心就是「用正確的方式建構每一列 vector」。
//     第 i 列有 i+1 個元素、頭尾都是 1 —— 正好是
//     `std::vector<int> row(i + 1, 1);`（數量+值建構子）的教科書用法。
//     若誤寫成 `std::vector<int> row{i + 1, 1};`，得到的是
//     兩個元素 {i+1, 1}，整題直接崩掉。本課的第一大坑在這裡有實際後果。
//   複雜度：O(numRows²) 時間與空間（輸出本身就這麼大）。
// -----------------------------------------------------------------------------
std::vector<std::vector<int>> generatePascalTriangle(int numRows) {
    std::vector<std::vector<int>> tri;
    tri.reserve(static_cast<size_t>(numRows));      // 列數已知，先配置

    for (int i = 0; i < numRows; ++i) {
        // 小括號！i+1 個元素，全部初始化為 1
        std::vector<int> row(static_cast<size_t>(i) + 1, 1);
        for (int j = 1; j < i; ++j) {
            row[static_cast<size_t>(j)] =
                tri[static_cast<size_t>(i) - 1][static_cast<size_t>(j) - 1] +
                tri[static_cast<size_t>(i) - 1][static_cast<size_t>(j)];
        }
        tri.push_back(std::move(row));              // 移動，不複製整列
    }
    return tri;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】固定大小的環形取樣緩衝區（監控系統的滑動視窗）
//   情境：監控 agent 每秒取樣一次 CPU 使用率，只保留最近 N 秒，
//         用來算移動平均、判斷是否連續超標。
//   為什麼用到本主題：這裡同時用上本課三個重點——
//     ① vector<double>(N, 0.0)：一開始就配置好固定大小，
//        之後永遠不擴容（監控程式最怕執行期突然配置記憶體）。
//     ② 值初始化：明確得到 N 個 0.0，不是垃圾值。
//     ③ 完全不用 push_back，size 從頭到尾不變，只覆寫內容。
//   這是嵌入式 / 即時系統很典型的寫法：把所有配置集中在啟動階段。
// -----------------------------------------------------------------------------
class RingSampler {
public:
    explicit RingSampler(size_t capacity)
        : buf_(capacity, 0.0), cap_(capacity), count_(0), next_(0) {}
        // buf_(capacity, 0.0) 是「數量 + 值」建構子 —— 小括號語意

    void add(double v) {
        buf_[next_] = v;
        next_ = (next_ + 1) % cap_;
        if (count_ < cap_) ++count_;
    }

    double average() const {
        if (count_ == 0) return 0.0;
        double sum = 0.0;
        for (size_t i = 0; i < count_; ++i) sum += buf_[i];
        return sum / static_cast<double>(count_);
    }

    size_t size()     const { return count_; }
    size_t capacity() const { return buf_.capacity(); }

private:
    std::vector<double> buf_;
    size_t cap_;
    size_t count_;
    size_t next_;
};

int main() {
    std::cout << "====================================================\n";
    std::cout << " 第10課：vector 的宣告與初始化方式 — 總複習\n";
    std::cout << "====================================================\n";

    demo_default_construction();
    demo_size_construction();
    demo_initializer_list();
    demo_parentheses_vs_braces();
    demo_range_construction();
    demo_move_construction();
    demo_assign();
    demo_custom_class();
    demo_ctad();
    demo_capacity_behavior();

    std::cout << "\n===== LeetCode 118 Pascal's Triangle =====\n";
    {
        auto tri = generatePascalTriangle(5);
        for (const auto& row : tri) {
            std::cout << "  [";
            for (size_t i = 0; i < row.size(); ++i) std::cout << (i ? "," : "") << row[i];
            std::cout << "]\n";
        }
        std::cout << "  關鍵：每列用 vector<int> row(i+1, 1) —— 小括號。\n";
        std::cout << "  若寫成 row{i+1, 1} 會變成兩個元素，整題崩潰。\n";
    }

    std::cout << "\n===== 日常實務：固定大小的取樣緩衝區 =====\n";
    {
        RingSampler s(5);       // 只在這裡配置一次，之後永不擴容
        std::cout << "初始 size=" << s.size() << " capacity=" << s.capacity() << "\n";

        double samples[] = {10.0, 20.0, 30.0, 40.0, 50.0, 60.0, 70.0};
        for (double v : samples) {
            s.add(v);
            std::cout << "  加入 " << v << " -> size=" << s.size()
                      << " capacity=" << s.capacity()
                      << " 平均=" << s.average() << "\n";
        }
        std::cout << "  → 塞了 7 筆但容量恆為 5，capacity 全程不變：\n";
        std::cout << "    所有記憶體配置都發生在建構當下，執行期零配置。\n";
    }

    std::cout << "\n====================================================\n";
    std::cout << " 複習完畢！\n";
    std::cout << "====================================================\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra summary.cpp -o summary
// ⚠️ 需要 C++17：demo_ctad() 用到 CTAD。

// === 預期輸出 ===
// ====================================================
//  第10課：vector 的宣告與初始化方式 — 總複習
// ====================================================
//
// ===== 重點一：預設建構（空 vector）=====
// v1 size: 0, capacity: 0
// v2 size: 0, capacity: 0
// v3 size: 0, capacity: 0
//
// ===== 重點二：指定大小建構 =====
// v1(5): 0 0 0 0 0
// v2(5, 42): 42 42 42 42 42
// v3(3) size: 3
//
// ===== 重點三：初始化串列 =====
// v1: 1 2 3 4 5
// v1.size(): 5
// v2 和 v3 效果相同
//
// ===== 重點四：小括號 vs 大括號（重要！）=====
// v1(5, 10) size: 5
// v1: 10 10 10 10 10
// v2{5, 10} size: 2
// v2: 5 10
// a(3) size: 3 內容: 0 0 0
// b{3} size: 1 內容: 3
//
// ===== 重點五：從範圍/其他容器建構 =====
// partial: 2 3 4
// from_array: 10 20 30 40
// from_std_array: 100 200 300
//
// ===== 重點六：移動建構 =====
// 移動前 source.size(): 5
// 移動後 source.size(): 0
// 移動後 dest.size():   5
// 重新賦值後 source.size(): 2
//
// ===== 重點七：assign 重新初始化 =====
// assign(5, 100): 100 100 100 100 100
// assign({10, 20, 30}): 10 20 30
// 從迭代器範圍 assign: 8 9 10
//
// ===== 重點八：自訂類別的初始化 =====
//   Alice is 30 years old.
//   Bob is 25 years old.
//   Charlie is 35 years old.
//
// ===== 重點九：C++17 CTAD 自動推導 =====
// v1 型別推導為 vector<int>，size: 3
// v2 型別推導為 vector<double>，size: 3
// v3 從迭代器推導，size: 3
//
// ===== 重點十：容量行為（實作定義，本機 libstdc++）=====
// push_back 的 capacity 序列: 0 -> 1 -> 2 -> 4 -> 8 -> 16 -> 32 -> 64 -> 128
//   → libstdc++ 是 2× 成長；MSVC 是 1.5×。
//     標準只要求 push_back 攤還 O(1)，沒有規定倍率。
// reserve(7)   後 capacity=7（剛好 7，沒有進位）
// reserve(100) 後 capacity=100
// reserve(50)  後 capacity=100（比目前小 → 不動作）
//
// resize 前: size=5 capacity=5
// resize(8) 後: size=8 capacity=10
//   → 不是 8！因為公式是 max(2*size, n) = max(10, 8) = 10
//
// 塞 1000 個元素的重新配置次數:
//   每次 reserve(size()+1) : 1000 次  ← 幾何成長被破壞
//   什麼都不做（純 push_back）: 11 次
//   → reserve 該在迴圈【外】呼叫一次，參數是最終總量。
//   迴圈外 reserve(1000)     : 0 次（配置一次就到位）
//
// resize(3) 後 size=3 capacity=1000
// shrink_to_fit() 後 capacity=3
//   → 本機確實縮了，但標準說它是 non-binding request，
//     實作可以完全忽略。不能寫成「一定會縮容」。
//
// sizeof(std::vector<int>) = 24 bytes（三個指標：begin / end / capacity_end）
//   → 這就是移動建構能做到 O(1) 的原因：只搬這三個指標。
//
// ===== LeetCode 118 Pascal's Triangle =====
//   [1]
//   [1,1]
//   [1,2,1]
//   [1,3,3,1]
//   [1,4,6,4,1]
//   關鍵：每列用 vector<int> row(i+1, 1) —— 小括號。
//   若寫成 row{i+1, 1} 會變成兩個元素，整題崩潰。
//
// ===== 日常實務：固定大小的取樣緩衝區 =====
// 初始 size=0 capacity=5
//   加入 10 -> size=1 capacity=5 平均=10
//   加入 20 -> size=2 capacity=5 平均=15
//   加入 30 -> size=3 capacity=5 平均=20
//   加入 40 -> size=4 capacity=5 平均=25
//   加入 50 -> size=5 capacity=5 平均=30
//   加入 60 -> size=5 capacity=5 平均=40
//   加入 70 -> size=5 capacity=5 平均=50
//   → 塞了 7 筆但容量恆為 5，capacity 全程不變：
//     所有記憶體配置都發生在建構當下，執行期零配置。
//
// ====================================================
//  複習完畢！
// ====================================================
