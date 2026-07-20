// =============================================================================
//  summary.cpp  —  第 13 課總複習：vector 元素新增 push_back / emplace_back
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<vector>
//
//   void push_back(const T& value);                    // (1) C++03 起，複製
//   void push_back(T&& value);                         // (2) C++11 起，移動
//
//   template<class... Args>
//   void      emplace_back(Args&&... args);            // C++11 / C++14：回傳 void
//   template<class... Args>
//   reference emplace_back(Args&&... args);            // C++17 起：回傳新元素參考
//
//   複雜度：兩者皆為「攤銷常數時間」(amortized constant / amortized O(1))。
//           單次呼叫最壞是 O(n)（發生 reallocation，要搬走全部既有元素），
//           但連續 n 次 push_back 的總成本是 O(n)，故平均每次是 O(1)。
//
//   例外保證：strong exception guarantee（強例外保證）——
//             若拋出例外，vector 保持呼叫前的狀態，無副作用。
//             但這個保證有前提，見【5.】與【概念補充 (C)】。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼「攤銷 O(1)」不是在騙人】
// vector 保證元素連續存放，所以空間滿了就沒有「原地長大」這種事，只能：
//     配置更大的一塊 → 把舊元素全部搬過去 → 釋放舊記憶體。
// 這一步是 O(n)。若成長策略是「每次 +1」，插入 n 個元素的總搬移量是
//     1 + 2 + 3 + ... + n = O(n^2)，平均每次 O(n)，這就毫無用處。
// 但若成長策略是「每次乘上固定倍率 k > 1」，總搬移量是等比級數：
//     1 + k + k^2 + ... + n  ≈  n * k/(k-1)  = O(n)
// 除以 n 次操作，平均每次是 O(1)。這就是「攤銷」的精確意義：
//     * 單次呼叫可能很貴（O(n)）——不可以說 push_back 一定是 O(1)。
//     * 但貴的次數呈指數級稀疏，總成本被攤平成常數。
// 面試若問「push_back 是 O(1) 嗎」，正確答案是「攤銷 O(1)，單次最壞 O(n)」。
//
// 【2. 成長倍率是實作定義，標準沒有規定】
// 標準只規定「攤銷常數複雜度」，**沒有**規定倍率是多少。實際上：
//     * libstdc++ (GCC)        ：2 倍
//     * libc++   (Clang)       ：2 倍
//     * MSVC STL               ：1.5 倍
// 本檔在 GCC/libstdc++ 上實測，capacity 序列是 1,2,4,8,16,32,...（2 倍）。
// 這個數字**不是標準保證**，換編譯器就會不同，絕不可以寫進商業邏輯。
// （1.5 倍的理論優點：釋放出的舊記憶體有機會被後續配置重複使用；
//   2 倍則永遠不夠用前面所有已釋放區塊的總和。實務上差異取決於 allocator。）
//
// 【3. push_back vs emplace_back：真正的差別在哪】
// push_back 的參數型別是 T：呼叫端必須先有一個 T 物件，才能複製或移動進去。
// emplace_back 的參數是 Args&&...：把參數「完美轉發」(perfect forwarding)
// 到 vector 內部那塊未初始化記憶體上，直接呼叫 T 的建構子，就地建構。
//
//   vector<Person> v;
//   v.push_back(Person("Alice", 30));   // ① 建構臨時 Person ② 移動進 vector ③ 解構臨時
//   v.emplace_back("Alice", 30);        // ① 就地建構，完畢
//
// 省下來的是「一次臨時物件的建構＋一次 move＋一次解構」。
//
// 但**最重要、也最多人搞錯的一點**：
//   當你傳進去的已經是一個現成的 T 物件時，兩者幾乎完全等價。
//       string s = "Hello";
//       v.push_back(s);        // 複製
//       v.emplace_back(s);     // 也是複製（轉發一個 lvalue → 呼叫複製建構子）
//       v.push_back(move(s));       // 移動
//       v.emplace_back(move(s));    // 也是移動
//   「emplace_back 永遠比較快」是錯的。它只在「你本來需要先造一個臨時物件」
//   的情況下才贏。這是本課最高頻的面試陷阱題。
//
// 【4. emplace_back 用「直接初始化」，這是彈性也是風險】
// push_back 走的是複製初始化(copy-initialization)語意（參數是 T，要能隱式轉換）；
// emplace_back 在內部是：
//     ::new (ptr) T(std::forward<Args>(args)...);       // 直接初始化 direct-init
// 直接初始化的三個後果：
//   (a) 可以呼叫 explicit 建構子：
//         struct E { explicit E(int); };
//         v.push_back(5);      // ✗ 編譯失敗（explicit 不允許隱式轉換）
//         v.emplace_back(5);   // ✓ 通過
//   (b) **不做 narrowing（窄化）檢查**：
//         vector<int> n;
//         n.emplace_back(3.9); // ✓ 編過，實測存進去的是 3（截斷，本機驗證）
//         n.push_back({3.9});  // ✗ list-init 會做窄化檢查，編譯失敗
//   (c) 參數打錯型別時可能「悄悄編過」，錯誤延後到執行期才發現。
// 所以 emplace_back 不是無腦的升級版；意圖是「我已經有物件了」時，
// push_back 反而更能讓編譯器幫你擋錯。
//
// 【5. 回傳值：C++17 是分水嶺】
//   * C++11 / C++14：emplace_back 回傳 void。
//   * C++17 起      ：回傳 reference（指向剛建構好的那個元素）。
//   * push_back     ：從頭到尾都回傳 void，沒有變過。
// 本機以 g++ -std=c++14 -pedantic-errors 實測，
// 寫 `auto& r = v.emplace_back(x);` 會得到 "forming reference to void" 編譯錯誤；
// 換 -std=c++17 即通過。（用 -fsyntax-only 驗證會被 GCC 當擴充放行，結論會錯。）
//
// 【6. 迭代器／指標／參考失效規則】
//   * 只要發生 reallocation：指向該 vector 的**所有** iterator / pointer /
//     reference 全部失效（包含 end()）。
//   * 未發生 reallocation（size() < capacity()）：既有元素的 iterator/pointer/
//     reference 仍有效，但 end() 一定失效。
//   * 所以 reserve() 不只是效能手段，也是「維持參考有效」的正確性手段。
//
// 【概念補充 Concept Deep Dive】
//
// (A) reallocation 到底做了什麼
//     1) allocator 配置一塊新的、更大的原始記憶體（尚未建構任何物件）
//     2) 把舊元素逐一「移動或複製」建構到新記憶體上
//     3) 解構舊元素
//     4) 釋放舊記憶體
//     注意第 2 步是「建構」不是 memcpy——除非型別是 trivially copyable，
//     編譯器才可能最佳化成 memmove。有自訂建構子的型別必須逐一呼叫。
//
// (B) 自我參照：v.push_back(v[0]) 為什麼是安全的
//     直覺會擔心：v[0] 是 vector 內部的參考，一旦 reallocation 把舊記憶體釋放掉，
//     那個參考不就懸空了嗎？
//     標準明文要求實作必須正確處理這種自我參照(self-referencing)的情形。
//     libstdc++ 的作法是：**先**在新配置的記憶體上把新元素建構好，**再**搬移
//     舊元素、最後才釋放舊記憶體。所以讀取 v[0] 時舊記憶體仍然活著。
//     本檔 demoSelfReference() 實測 {1,2,3} 在 capacity 剛好用完時
//     push_back(v[0])，結果正確得到 1 2 3 1。
//     （但 v.push_back(v[0]) 之後，先前保存的 iterator 仍然全部失效——
//       「這個操作安全」跟「舊 iterator 還能用」是兩件事。）
//
// (C) 強例外保證的隱藏前提：move constructor 必須是 noexcept
//     reallocation 搬移元素到一半若拋出例外，就回不去了——舊記憶體已經被
//     搬掉一部分，vector 會處於半殘狀態，強例外保證破功。
//     標準的解法是 std::move_if_noexcept：
//         * T 的 move constructor 是 noexcept  → 用 move（快）
//         * T 的 move constructor 可能拋例外   → **退化成 copy**（慢但可回復，
//           因為 copy 不會破壞來源，失敗時舊資料完好如初）
//         * T 不可複製（如 unique_ptr）        → 就算 move 可能拋，也只能用 move
//     實務結論：**一定要把你的 move constructor 標成 noexcept**。
//     忘了標，vector 擴容時會默默從 move 退化成 copy，效能斷崖式下滑而且
//     完全不會有任何警告。本檔 demoNoexceptMatters() 用兩個只差 noexcept 的
//     類別實測，輸出一個印 "移動"、一個印 "複製"，差別一目了然。
//
// (D) emplace_back 內部長什麼樣（概念示意）
//     template<class... Args>
//     reference emplace_back(Args&&... args) {
//         if (size_ == cap_) reallocate(grow());          // 可能觸發擴容
//         alloc_traits::construct(alloc_, data_ + size_,   // 就地建構
//                                 std::forward<Args>(args)...);
//         ++size_;
//         return data_[size_ - 1];                        // C++17 起才有這行
//     }
//     std::forward 保持了參數的值類別(value category)：傳 lvalue 進來就轉發
//     lvalue（→ 呼叫複製建構子），傳 rvalue 進來就轉發 rvalue（→ 移動建構子）。
//     這就是為什麼 emplace_back(s) 對 lvalue s 依然是複製。
//
// 【注意事項 Pay Attention】
// 1. push_back 是**攤銷** O(1)，不是每次都 O(1)。即時性要求高的系統
//    （交易、音訊 callback）要先 reserve，避免不可預期的 O(n) 尖峰。
// 2. capacity 的實際數值是實作定義。本檔所有 capacity 輸出皆為
//    **libstdc++ 實測值，非標準保證**。
// 3. 「emplace_back 永遠比較快」是錯的。傳現成物件時兩者等價。
// 4. emplace_back 走 direct-init：能呼叫 explicit 建構子、**不做窄化檢查**，
//    參數打錯可能悄悄編過。
// 5. emplace_back 的回傳值 C++17 起才是 reference，C++11/14 是 void。
// 6. 任何一次 reallocation 都會讓所有 iterator / pointer / reference 失效。
//    在 range-for 迴圈中對同一個 vector push_back 是未定義行為。
// 7. move constructor 沒標 noexcept，擴容會退化成 copy（見概念補充 C）。
// 8. push_back 的回傳值是 void，不能鏈式呼叫；要鏈式請用 C++17 的 emplace_back。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】push_back / emplace_back
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. push_back 的時間複雜度是多少？
//     答：攤銷 O(1)（amortized constant）。單次呼叫在觸發 reallocation 時是
//         O(n)，因為要把既有元素全部搬到新記憶體。但因為成長是等比級數，
//         連續 n 次的總成本是 O(n)，攤平後每次是常數。
//     追問：為什麼成長要用「乘上倍率」而不是「每次加固定值」？
//         → 加固定值會讓總搬移量變成 O(n^2)，攤銷後每次仍是 O(n)，
//           等比成長才能讓總量收斂成 O(n)。
//
// 🔥 Q2. vector 的成長倍率是 2 倍嗎？
//     答：標準**沒有規定**倍率，只規定攤銷常數複雜度。libstdc++ 與 libc++ 用
//         2 倍，MSVC STL 用 1.5 倍。本機實測 capacity 序列 1,2,4,8,16,32
//         （libstdc++ 實測值，非標準保證）。任何依賴具體 capacity 數值的
//         程式碼都是不可攜的。
//     追問：1.5 倍的理由是什麼？
//         → 2 倍時新配置量永遠大於先前所有已釋放區塊的總和，舊空間無法被
//           重複利用；1.5 倍則有機會，理論上對記憶體碎片較友善。
//
// 🔥 Q3. push_back 跟 emplace_back 差在哪？什麼時候該用哪一個？
//     答：push_back 參數是 T，呼叫端得先有物件，然後複製或移動進容器；
//         emplace_back 參數是 Args&&...，完美轉發到容器內部**就地建構**，
//         省掉一次臨時物件的建構＋移動＋解構。
//         「要從零件造一個新元素」用 emplace_back；
//         「已經有現成物件」兩者等價，用 push_back 意圖更清楚。
//     追問：emplace_back 為什麼能接受任意數量參數？
//         → 它是 variadic template，用 std::forward 把整包參數原封不動
//           轉發給 T 的建構子。
//
// ⚠️ 陷阱 Q4. 「emplace_back 永遠比 push_back 快，所以應該全面改用」——對嗎？
//     答：錯。當傳入的已經是現成的同型別物件時，兩者**完全等價**：
//             string s = "Hello";
//             v.push_back(s);      // 複製
//             v.emplace_back(s);   // 也是複製！s 是 lvalue，轉發後呼叫複製建構子
//         emplace_back 只有在「原本需要先建構一個臨時物件」時才有優勢。
//     為什麼會錯：多數人腦中的模型是「emplace = 就地建構 = 不會複製」，
//         漏掉了 std::forward 會**保留參數的值類別**這件事。傳 lvalue 進去，
//         轉發出來還是 lvalue，最後呼叫的依然是複製建構子。就地建構省的是
//         「臨時物件」，不是「複製」本身。
//
// ⚠️ 陷阱 Q5. emplace_back 比 push_back 更安全嗎？
//     答：反過來，在型別檢查上 emplace_back **比較危險**。它用 direct-init：
//         (a) 可以呼叫 explicit 建構子——繞過了作者「禁止隱式轉換」的意圖；
//         (b) **不做 narrowing 檢查**——v.emplace_back(3.9) 存進 vector<int>
//             會靜默截斷成 3（本機實測），push_back({3.9}) 則會編譯失敗。
//         參數打錯時 emplace_back 更容易悄悄編過，把錯誤推遲到執行期。
//     為什麼會錯：把「功能更強」直接等同於「更好」。直接初始化的彈性
//         同時也拆掉了編譯器替你架設的護欄。
//
// ⚠️ 陷阱 Q6. v.push_back(v[0]) 會不會因為 reallocation 而讀到懸空參考？
//     答：不會。標準明文要求實作必須正確處理自我參照。libstdc++ 的順序是
//         「先在新記憶體建構新元素 → 再搬移舊元素 → 最後釋放舊記憶體」，
//         讀 v[0] 時舊記憶體還活著。本檔實測 {1,2,3} 滿載時 push_back(v[0])
//         正確得到 1 2 3 1。
//     為什麼會錯：直覺推理「先擴容再插入」，就得出「舊記憶體已釋放」的結論。
//         但這個順序是實作可以自己安排的，而標準要求的結果是正確的。
//         注意：操作本身安全 ≠ 你先前存下的 iterator 還有效，那些一律失效。
//
// 🔥 Q7. 為什麼一定要把 move constructor 標成 noexcept？
//     答：vector 擴容時用 std::move_if_noexcept 決定搬移策略。move 若不是
//         noexcept，搬到一半拋例外就無法回復，會破壞強例外保證，所以標準庫
//         寧可**退化成 copy**（copy 不破壞來源，失敗可回滾）。
//         結果是：忘了標 noexcept → 每次擴容都在做深複製，效能斷崖下滑，
//         而且編譯器不會給任何警告。本檔實測兩個只差 noexcept 的類別，
//         一個印「移動」、一個印「複製」。
//     追問：那 unique_ptr 這種不可複製的型別呢？
//         → 沒有 copy 可退化，只能用 move，此時強例外保證降級為基本保證。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>

using namespace std;

// 用於觀察建構/複製/移動行為的測試類別
struct Tracker {
    string name;

    Tracker(const string& n) : name(n) {
        cout << "  [建構] " << name << endl;
    }
    Tracker(const Tracker& other) : name(other.name) {
        cout << "  [複製] " << name << endl;
    }
    Tracker(Tracker&& other) noexcept : name(move(other.name)) {
        cout << "  [移動] " << name << endl;
    }
    ~Tracker() {
        // 解構時不印出，避免干擾輸出
    }
};

// ================================================================
// 重點一：push_back() 的基本用法
// ================================================================
void demoPushBack() {
    cout << "\n=== 重點一：push_back 基本用法 ===" << endl;

    vector<int> v;
    v.push_back(10);
    v.push_back(20);
    v.push_back(30);

    cout << "元素: ";
    for (int n : v) cout << n << " ";
    cout << endl;

    vector<string> words;
    string hello = "Hello";

    words.push_back(hello);             // 複製 hello 進入 vector
    words.push_back("World");           // 由字面量造臨時 string，再移動進入
    words.push_back(move(hello));       // 移動 hello 進入

    // 被 move 走的 string 處於「valid but unspecified」狀態：
    // 標準只保證它仍可安全解構與賦值，**不保證**內容是什麼。
    // libstdc++ 實測為空字串，這是實作行為，不可依賴。
    cout << "被 move 後 hello.empty() = " << boolalpha << hello.empty()
         << "（libstdc++ 實測；標準只保證 valid but unspecified）" << endl;
    cout << "vector 內容: ";
    for (const string& w : words) cout << "\"" << w << "\" ";
    cout << endl;
}

// ================================================================
// 重點二：push_back 的複製 vs 移動行為
// ================================================================
void demoPushBackCopyMove() {
    cout << "\n=== 重點二：push_back 複製 vs 移動 ===" << endl;

    vector<Tracker> v;
    v.reserve(3);  // 預留空間，避免重分配干擾觀察

    cout << "--- push_back(lvalue) → 複製 ---" << endl;
    Tracker t1("T1");
    v.push_back(t1);      // 複製：t1 仍然有效

    cout << "--- push_back(rvalue) → 移動 ---" << endl;
    v.push_back(Tracker("T2"));  // 臨時物件：移動

    cout << "--- push_back(move(lvalue)) → 移動 ---" << endl;
    Tracker t3("T3");
    v.push_back(move(t3));  // 明確移動：t3 之後不應再讀其值
}

// ================================================================
// 重點三：emplace_back() —— 就地建構（C++11）
// ================================================================
void demoEmplaceBack() {
    cout << "\n=== 重點三：emplace_back 就地建構 ===" << endl;

    vector<Tracker> v;
    v.reserve(2);

    cout << "--- emplace_back（直接傳建構參數）---" << endl;
    v.emplace_back("E1");  // 只有一次建構，沒有臨時物件
    v.emplace_back("E2");
    cout << "（注意：完全沒有出現 [移動] 或 [複製]）" << endl;
}

// ================================================================
// 重點四：兩者「等價」與「有差」的分界線
// ================================================================
struct Point {
    int x, y;
    Point(int px, int py) : x(px), y(py) {}
};

void demoCompare() {
    cout << "\n=== 重點四：push_back vs emplace_back 的分界線 ===" << endl;

    vector<Tracker> v;
    v.reserve(4);

    cout << "--- 情境 A：從零件建構新元素（emplace_back 有優勢）---" << endl;
    cout << "push_back(Tracker(\"A1\")):" << endl;
    v.push_back(Tracker("A1"));     // 建構臨時 + 移動
    cout << "emplace_back(\"A2\"):" << endl;
    v.emplace_back("A2");           // 只有建構

    cout << "--- 情境 B：已有現成物件（兩者等價，都是複製）---" << endl;
    Tracker existing("B1");
    cout << "push_back(existing):" << endl;
    v.push_back(existing);          // 複製
    cout << "emplace_back(existing):" << endl;
    v.emplace_back(existing);       // 也是複製！

    // 多參數建構才是 emplace_back 的主場
    vector<Point> pts;
    pts.reserve(4);
    pts.push_back(Point(1, 2));     // 建構臨時物件再移動
    pts.push_back({3, 4});          // list-init，同樣先造臨時物件
    pts.emplace_back(5, 6);         // 直接傳 (5,6)，就地建構
    pts.emplace_back(7, 8);

    cout << "Points: ";
    for (const Point& p : pts) cout << "(" << p.x << "," << p.y << ") ";
    cout << endl;
}

// ================================================================
// 重點五：攤銷 O(1) 與成長倍率（實測）
// ================================================================
void demoAmortizedGrowth() {
    cout << "\n=== 重點五：攤銷 O(1) 與成長倍率 ===" << endl;

    vector<int> v;
    size_t lastCap = v.capacity();
    int reallocCount = 0;

    cout << "capacity 變化序列: " << lastCap;
    for (int i = 0; i < 20; ++i) {
        v.push_back(i);
        if (v.capacity() != lastCap) {
            ++reallocCount;
            lastCap = v.capacity();
            cout << " -> " << lastCap;
        }
    }
    cout << endl;
    cout << "插入 20 個元素，reallocation 次數: " << reallocCount << endl;
    cout << "（以上為 libstdc++ 2 倍成長的實測值，非標準保證；MSVC 是 1.5 倍）"
         << endl;

    // 先 reserve → 零次 reallocation
    vector<int> v2;
    v2.reserve(20);
    size_t cap2 = v2.capacity();
    int realloc2 = 0;
    for (int i = 0; i < 20; ++i) {
        v2.push_back(i);
        if (v2.capacity() != cap2) { ++realloc2; cap2 = v2.capacity(); }
    }
    cout << "先 reserve(20) 後插入 20 個，reallocation 次數: " << realloc2 << endl;
}

// ================================================================
// 重點六：迭代器失效
// ================================================================
void demoInvalidation() {
    cout << "\n=== 重點六：迭代器 / 參考失效 ===" << endl;

    // 情況 A：capacity 不足 → reallocation → 全部失效
    vector<int> v;
    v.reserve(2);
    v.push_back(1);
    v.push_back(2);
    const int* before = v.data();
    v.push_back(3);                      // capacity 用盡，觸發 reallocation
    const int* after = v.data();
    cout << "capacity 用盡後 push_back，資料位址是否改變: "
         << boolalpha << (before != after)
         << "（改變即代表舊 iterator/pointer/reference 全部失效）" << endl;

    // 情況 B：capacity 足夠 → 既有元素的 iterator 不失效
    vector<int> v2;
    v2.reserve(10);
    v2.push_back(100);
    auto it2 = v2.begin();               // 指向第一個元素
    const int* base2 = v2.data();
    for (int i = 1; i < 5; ++i) {
        v2.push_back(i * 100);           // 未超過 capacity，不會 reallocation
    }
    cout << "reserve(10) 內連續 push_back，資料位址是否保持不變: "
         << (base2 == v2.data()) << endl;
    cout << "  → 先前取得的 it2 仍然有效，*it2 = " << *it2 << endl;
    cout << "  （但 end() 一定失效，任何情況都不能沿用舊的 end()）" << endl;
}

// ================================================================
// 重點七：自我參照 v.push_back(v[0])
// ================================================================
void demoSelfReference() {
    cout << "\n=== 重點七：自我參照 push_back(v[0]) ===" << endl;

    vector<int> v{1, 2, 3};
    v.shrink_to_fit();                   // 讓 size == capacity，確保下一次會擴容
    cout << "push_back 前 size=" << v.size()
         << " capacity=" << v.capacity()
         << "（capacity 為 libstdc++ 實測值）" << endl;

    v.push_back(v[0]);                   // 自我參照：標準要求實作必須正確處理

    cout << "push_back(v[0]) 後: ";
    for (int x : v) cout << x << " ";
    cout << endl;
    cout << "（安全，因為實作先在新記憶體建構新元素、才釋放舊記憶體）" << endl;
}

// ================================================================
// 重點八：noexcept move 決定擴容是「移動」還是「複製」
// ================================================================
struct FastMove {          // move constructor 標了 noexcept
    int id;
    explicit FastMove(int i) : id(i) {}
    FastMove(const FastMove& o) : id(o.id) { cout << "  [複製] FastMove" << id << endl; }
    FastMove(FastMove&& o) noexcept : id(o.id) { cout << "  [移動] FastMove" << id << endl; }
};

struct SlowMove {          // move constructor 忘了標 noexcept
    int id;
    explicit SlowMove(int i) : id(i) {}
    SlowMove(const SlowMove& o) : id(o.id) { cout << "  [複製] SlowMove" << id << endl; }
    SlowMove(SlowMove&& o) : id(o.id) { cout << "  [移動] SlowMove" << id << endl; }
};

void demoNoexceptMatters() {
    cout << "\n=== 重點八：noexcept move 對擴容的影響 ===" << endl;

    cout << "FastMove（move 是 noexcept），擴容時：" << endl;
    vector<FastMove> a;
    a.reserve(1);
    a.emplace_back(1);
    a.emplace_back(2);                   // 觸發 reallocation，搬移既有的 1

    cout << "SlowMove（move 未標 noexcept），擴容時：" << endl;
    vector<SlowMove> b;
    b.reserve(1);
    b.emplace_back(1);
    b.emplace_back(2);                   // 觸發 reallocation，搬移既有的 1

    cout << "→ 同樣的操作，只因少了 noexcept 就從移動退化成複製。" << endl;
    cout << "  這是 std::move_if_noexcept 為了維持強例外保證所做的取捨。" << endl;
}

// ================================================================
// 重點九：emplace_back 的 direct-init 風險
// ================================================================
struct Meters {
    explicit Meters(int v) : value(v) {}   // explicit：禁止隱式轉換
    int value;
};

void demoDirectInitRisk() {
    cout << "\n=== 重點九：emplace_back 的 direct-init 風險 ===" << endl;

    vector<Meters> m;
    // m.push_back(5);        // ✗ 編譯失敗：explicit 建構子不允許隱式轉換
    m.emplace_back(5);        // ✓ 通過：direct-init 可以呼叫 explicit 建構子
    cout << "emplace_back(5) 成功呼叫了 explicit 建構子，value = "
         << m[0].value << endl;

    vector<int> n;
    n.emplace_back(3.9);      // ✓ 編過！direct-init 不做 narrowing 檢查
    // n.push_back({3.9});    // ✗ 編譯失敗：list-init 會檢查窄化
    cout << "emplace_back(3.9) 存進 vector<int> 的結果 = " << n[0]
         << "（靜默截斷，本機實測；push_back({3.9}) 則會編譯失敗）" << endl;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 1】LeetCode 54. Spiral Matrix
//   題目：以螺旋順序回傳矩陣中所有元素。
//   為什麼用到本主題：結果長度事先已知（rows * cols），是 reserve + push_back
//                     的教科書場景——先 reserve 就完全不會發生 reallocation。
// -----------------------------------------------------------------------------
vector<int> spiralOrder(const vector<vector<int>>& matrix) {
    vector<int> result;
    if (matrix.empty() || matrix[0].empty()) return result;

    int rows = static_cast<int>(matrix.size());
    int cols = static_cast<int>(matrix[0].size());
    result.reserve(static_cast<size_t>(rows) * static_cast<size_t>(cols));

    int top = 0, bottom = rows - 1, left = 0, right = cols - 1;
    while (top <= bottom && left <= right) {
        for (int c = left; c <= right; ++c) result.push_back(matrix[top][c]);
        ++top;
        for (int r = top; r <= bottom; ++r) result.push_back(matrix[r][right]);
        --right;
        if (top <= bottom) {
            for (int c = right; c >= left; --c) result.push_back(matrix[bottom][c]);
            --bottom;
        }
        if (left <= right) {
            for (int r = bottom; r >= top; --r) result.push_back(matrix[r][left]);
            ++left;
        }
    }
    return result;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 2】LeetCode 46. Permutations
//   題目：回傳陣列所有排列。
//   為什麼用到本主題：backtracking 的標準骨架就是
//                     push_back(選擇) → 遞迴 → pop_back(回溯)。
//                     找到完整解時再 push_back 進結果集，是「累積答案」的典型。
// -----------------------------------------------------------------------------
static void permuteHelper(vector<int>& nums, vector<bool>& used,
                          vector<int>& current, vector<vector<int>>& out) {
    if (current.size() == nums.size()) {
        out.push_back(current);          // 複製當前這條完整路徑進結果
        return;
    }
    for (size_t i = 0; i < nums.size(); ++i) {
        if (used[i]) continue;
        used[i] = true;
        current.push_back(nums[i]);      // 做選擇
        permuteHelper(nums, used, current, out);
        current.pop_back();              // 撤銷選擇（回溯）
        used[i] = false;
    }
}

vector<vector<int>> permute(vector<int> nums) {
    vector<vector<int>> out;
    vector<int> current;
    vector<bool> used(nums.size(), false);
    current.reserve(nums.size());
    permuteHelper(nums, used, current, out);
    return out;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 1】把 CSV 文字解析成結構陣列
//   情境：讀取伺服器匯出的 CSV（使用者 ID, 名稱, 登入次數），轉成物件陣列。
//   為什麼用 emplace_back：每一列都要「從三個欄位造一個新物件」，
//                          正是 emplace_back 唯一真正勝出的場景——
//                          省掉臨時 UserRecord 的建構、移動與解構。
// -----------------------------------------------------------------------------
struct UserRecord {
    int id;
    string name;
    int loginCount;

    UserRecord(int i, string n, int c)
        : id(i), name(move(n)), loginCount(c) {}
};

vector<UserRecord> parseCsv(const string& csv) {
    vector<UserRecord> records;
    istringstream stream(csv);
    string line;

    // 先數行數再 reserve，避免解析過程反覆 reallocation
    records.reserve(static_cast<size_t>(
        count(csv.begin(), csv.end(), '\n') + 1));

    while (getline(stream, line)) {
        if (line.empty()) continue;
        size_t c1 = line.find(',');
        if (c1 == string::npos) continue;
        size_t c2 = line.find(',', c1 + 1);
        if (c2 == string::npos) continue;

        int id = stoi(line.substr(0, c1));
        string name = line.substr(c1 + 1, c2 - c1 - 1);
        int cnt = stoi(line.substr(c2 + 1));

        // 直接把三個零件交給 vector 就地建構
        records.emplace_back(id, move(name), cnt);
    }
    return records;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】從 log 收集指定等級的訊息
//   情境：監控系統掃描 log，把 [ERROR] 的行收集起來準備發警報。
//   為什麼用 push_back：這裡是「把已經存在的字串放進容器」，
//                       push_back 與 emplace_back 完全等價，
//                       用 push_back 意圖更清楚（我在放一個現成的東西進去）。
// -----------------------------------------------------------------------------
vector<string> collectErrorLines(const vector<string>& logLines) {
    vector<string> errors;
    for (const string& line : logLines) {
        if (line.find("[ERROR]") != string::npos) {
            errors.push_back(line);      // 現成物件 → push_back 語意最清楚
        }
    }
    return errors;
}

int main() {
    cout << "=============================================" << endl;
    cout << "   第 13 課：push_back / emplace_back 總複習" << endl;
    cout << "=============================================" << endl;

    demoPushBack();
    demoPushBackCopyMove();
    demoEmplaceBack();
    demoCompare();
    demoAmortizedGrowth();
    demoInvalidation();
    demoSelfReference();
    demoNoexceptMatters();
    demoDirectInitRisk();

    cout << "\n=== LeetCode 54. Spiral Matrix ===" << endl;
    vector<vector<int>> mat{{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};
    for (int x : spiralOrder(mat)) cout << x << " ";
    cout << endl;

    cout << "\n=== LeetCode 46. Permutations ===" << endl;
    for (const auto& p : permute({1, 2, 3})) {
        cout << "[";
        for (size_t i = 0; i < p.size(); ++i) {
            cout << p[i] << (i + 1 < p.size() ? "," : "");
        }
        cout << "] ";
    }
    cout << endl;

    cout << "\n=== 日常實務 1：CSV 解析 ===" << endl;
    string csv = "101,Alice,42\n102,Bob,7\n103,Charlie,128\n";
    for (const UserRecord& r : parseCsv(csv)) {
        cout << "  id=" << r.id << " name=" << r.name
             << " logins=" << r.loginCount << endl;
    }

    cout << "\n=== 日常實務 2：收集 ERROR log ===" << endl;
    vector<string> logs{
        "2026-07-19 10:00:01 [INFO]  service started",
        "2026-07-19 10:00:05 [ERROR] db connection refused",
        "2026-07-19 10:00:06 [WARN]  retrying in 3s",
        "2026-07-19 10:00:09 [ERROR] db connection refused (retry 1)"
    };
    for (const string& e : collectErrorLines(logs)) cout << "  " << e << endl;

    cout << "\n==============================================" << endl;
    cout << " 選擇建議：" << endl;
    cout << " - 已有現成物件 → push_back（語意清晰，兩者效能相同）" << endl;
    cout << " - 從零件建構新元素 → emplace_back（省一次臨時物件）" << endl;
    cout << " - 批量新增前先 reserve()（避免 reallocation 與失效）" << endl;
    cout << " - move constructor 一定要標 noexcept" << endl;
    cout << "==============================================" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra summary.cpp -o summary

// （以上為 libstdc++ 2 倍成長的實測值，非標準保證；MSVC 是 1.5 倍）
// push_back 前 size=3 capacity=3（capacity 為 libstdc++ 實測值）

// === 預期輸出 ===
// =============================================
//    第 13 課：push_back / emplace_back 總複習
// =============================================
// 
// === 重點一：push_back 基本用法 ===
// 元素: 10 20 30 
// 被 move 後 hello.empty() = true（libstdc++ 實測；標準只保證 valid but unspecified）
// vector 內容: "Hello" "World" "Hello" 
// 
// === 重點二：push_back 複製 vs 移動 ===
// --- push_back(lvalue) → 複製 ---
//   [建構] T1
//   [複製] T1
// --- push_back(rvalue) → 移動 ---
//   [建構] T2
//   [移動] T2
// --- push_back(move(lvalue)) → 移動 ---
//   [建構] T3
//   [移動] T3
// 
// === 重點三：emplace_back 就地建構 ===
// --- emplace_back（直接傳建構參數）---
//   [建構] E1
//   [建構] E2
// （注意：完全沒有出現 [移動] 或 [複製]）
// 
// === 重點四：push_back vs emplace_back 的分界線 ===
// --- 情境 A：從零件建構新元素（emplace_back 有優勢）---
// push_back(Tracker("A1")):
//   [建構] A1
//   [移動] A1
// emplace_back("A2"):
//   [建構] A2
// --- 情境 B：已有現成物件（兩者等價，都是複製）---
//   [建構] B1
// push_back(existing):
//   [複製] B1
// emplace_back(existing):
//   [複製] B1
// Points: (1,2) (3,4) (5,6) (7,8) 
// 
// === 重點五：攤銷 O(1) 與成長倍率 ===
// capacity 變化序列: 0 -> 1 -> 2 -> 4 -> 8 -> 16 -> 32
// 插入 20 個元素，reallocation 次數: 6
// 先 reserve(20) 後插入 20 個，reallocation 次數: 0
// 
// === 重點六：迭代器 / 參考失效 ===
// capacity 用盡後 push_back，資料位址是否改變: true（改變即代表舊 iterator/pointer/reference 全部失效）
// reserve(10) 內連續 push_back，資料位址是否保持不變: true
//   → 先前取得的 it2 仍然有效，*it2 = 100
//   （但 end() 一定失效，任何情況都不能沿用舊的 end()）
// 
// === 重點七：自我參照 push_back(v[0]) ===
// push_back(v[0]) 後: 1 2 3 1 
// （安全，因為實作先在新記憶體建構新元素、才釋放舊記憶體）
// 
// === 重點八：noexcept move 對擴容的影響 ===
// FastMove（move 是 noexcept），擴容時：
//   [移動] FastMove1
// SlowMove（move 未標 noexcept），擴容時：
//   [複製] SlowMove1
// → 同樣的操作，只因少了 noexcept 就從移動退化成複製。
//   這是 std::move_if_noexcept 為了維持強例外保證所做的取捨。
// 
// === 重點九：emplace_back 的 direct-init 風險 ===
// emplace_back(5) 成功呼叫了 explicit 建構子，value = 5
// emplace_back(3.9) 存進 vector<int> 的結果 = 3（靜默截斷，本機實測；push_back({3.9}) 則會編譯失敗）
// 
// === LeetCode 54. Spiral Matrix ===
// 1 2 3 6 9 8 7 4 5 
// 
// === LeetCode 46. Permutations ===
// [1,2,3] [1,3,2] [2,1,3] [2,3,1] [3,1,2] [3,2,1] 
// 
// === 日常實務 1：CSV 解析 ===
//   id=101 name=Alice logins=42
//   id=102 name=Bob logins=7
//   id=103 name=Charlie logins=128
// 
// === 日常實務 2：收集 ERROR log ===
//   2026-07-19 10:00:05 [ERROR] db connection refused
//   2026-07-19 10:00:09 [ERROR] db connection refused (retry 1)
// 
// ==============================================
//  選擇建議：
//  - 已有現成物件 → push_back（語意清晰，兩者效能相同）
//  - 從零件建構新元素 → emplace_back（省一次臨時物件）
//  - 批量新增前先 reserve()（避免 reallocation 與失效）
//  - move constructor 一定要標 noexcept
// ==============================================
