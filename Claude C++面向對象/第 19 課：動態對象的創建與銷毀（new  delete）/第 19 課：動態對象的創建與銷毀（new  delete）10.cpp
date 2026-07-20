// =============================================================================
//  第 19 課 範例 10  —  new/delete 綜合實戰：武器工廠 WeaponFactory
// =============================================================================
//
//  ※ 本檔結構說明：
//    開頭一大段以 `//` 寫成的內容是課程講義（Markdown 格式，含表格與
//    示範用的程式片段，那些片段「不會被編譯」）。真正會被編譯執行的程式碼，
//    從下方 `#include <iostream>` 那一行開始。閱讀時請注意區分。
//
// 【主題資訊 Information】
//   語法    ：T* p = new T(args);  delete p;
//             T** a = new T*[n];   delete[] a;   // 指標陣列
//   標準版本：C++98；現代替代方案 unique_ptr 為 C++11、make_unique 為 C++14
//   標頭檔  ：<iostream>、<string>（改用智能指標則需 <memory>）
//
// 【詳細解釋 Explanation】
//
// 【1. 本範例的兩層動態配置】
//   這是實務上最容易漏掉東西的結構，值得看清楚：
//     * 每個 Weapon 物件本身是 new 出來的（第一層）；
//     * 每個 Weapon 內部又有一個 new int 配置的 durability（第二層）。
//   後半段的「批量創建」更疊了一層：
//       Weapon** armory = new Weapon*[n];   // 指標陣列本身
//       armory[i] = new Weapon(...);        // 每個元素指向的物件
//   釋放時必須反過來、由內而外：
//       先 delete armory[i]（逐一釋放武器，這會連帶釋放各自的 durability），
//       最後才 delete[] armory（釋放指標陣列本身）。
//   順序反過來就會失去所有武器的位址，造成無法挽回的洩漏。
//
// 【2. 為什麼 delete armory[i] 能連帶釋放 durability】
//   因為 delete 會先呼叫 ~Weapon()，而 ~Weapon() 裡有 delete durability。
//   這就是 RAII 的層層委派：外層物件的解構函數負責釋放它自己持有的資源。
//   反過來說，如果忘記 delete armory[i] 而只 delete[] armory，
//   不只是武器物件漏掉，連它們內部的 durability 也一起漏掉 ——
//   一次疏忽，兩層洩漏。
//
// 【3. 指標陣列 vs 物件陣列】
//   `new Weapon*[3]` 配置的是「3 個指標」，不是 3 個 Weapon，
//   因此它不會呼叫任何建構函數，元素初值是不確定的（本檔隨即逐一賦值）。
//   對照 `new Weapon[3]` 才會呼叫 3 次預設建構函數 ——
//   但那要求 Weapon 有預設建構函數，而且無法讓每個元素帶不同參數。
//   本檔用指標陣列，正是為了讓三把武器各有各的名稱與數值。
//
// 【概念補充 Concept Deep Dive】
//   ⚠️ 本檔的 Weapon 違反 Rule of Three，值得特別留意：
//   它以裸指標（int* durability）持有資源並自訂了解構函數，
//   卻沒有自訂複製建構函數與複製賦值運算子。
//   編譯器產生的預設版本只做淺複製（把 durability 指標抄一份），
//   於是兩個 Weapon 會共用同一塊 int，各自解構時各 delete 一次
//   —— double free，未定義行為。
//   本檔之所以安全，純粹是因為程式碼從頭到尾「沒有複製過任何 Weapon」
//   （全部透過指標操作）。真要當成可用的類別，必須補齊 Rule of Three／Five，
//   或改用 RAII 成員走 Rule of Zero。
//
//   另外，程式碼註解稱 Weapon 為「武器基類」，但本檔並沒有任何衍生類別，
//   而 ~Weapon() 也不是 virtual。在目前的用法下這完全正確且省成本
//   （非 virtual 表示物件不需要 vptr）。但若日後真的衍生出 Sword、Bow
//   並以 Weapon* 持有再 delete，就「必須」把 ~Weapon() 改成 virtual，
//   否則衍生類別的解構函數不會被呼叫，屬未定義行為。
//   這是「基類」這個命名帶來的隱藏地雷，改動繼承結構時務必一併處理。
//
// 【注意事項 Pay Attention】
// 1. 兩層配置必須由內而外釋放：先 delete 每個元素，再 delete[] 陣列本身。
// 2. new 配 delete、new[] 配 delete[]，混用是未定義行為。
//    本檔的 armory 是 new Weapon*[n]，因此用 delete[]；
//    armory[i] 是 new Weapon，因此用 delete。
// 3. Weapon 違反 Rule of Three（見上），僅因程式中未發生複製而安全。
// 4. ~Weapon() 目前非 virtual 是正確的（無繼承）；
//    一旦引入多型繼承並以基底指標 delete，就必須改為 virtual。
// 5. 這整段手動管理，用 std::vector<std::unique_ptr<Weapon>> 可以
//    完全取代：不必寫任何 delete，例外安全也自動成立。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】多層動態配置與所有權
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. `Weapon** armory = new Weapon*[3];` 之後要怎麼正確釋放？
//     答：必須兩步，而且順序不能顛倒：
//         (1) 先逐一 `delete armory[i];` —— 釋放每個 Weapon 物件，
//             這會呼叫 ~Weapon()，連帶釋放它內部 new 出來的 durability；
//         (2) 再 `delete[] armory;` —— 釋放那個指標陣列本身。
//         若先 delete[] armory，所有武器的位址就此遺失，永遠無法釋放。
//     追問：`new Weapon*[3]` 會呼叫幾次 Weapon 的建構函數？
//         → 零次。它配置的是 3 個「指標」，不是 3 個 Weapon 物件，
//           元素初值不確定，必須自行逐一賦值。
//           要呼叫建構函數得寫 `new Weapon[3]`，但那需要預設建構函數，
//           且無法讓每個元素帶不同的參數。
//
// 🔥 Q2. 這個 Weapon 類別可以安全地複製嗎？
//     答：不行。它以裸指標持有資源又自訂了解構函數，卻沒有處理複製語意，
//         違反 Rule of Three。`Weapon a = b;` 會做淺複製，
//         兩個物件共用同一塊 durability，各自解構時各 delete 一次
//         —— double free，未定義行為。
//         本檔安全只是因為全程透過指標操作、從未複製過 Weapon。
//     追問：那要怎麼修？
//         → 最好的做法是 Rule of Zero：把 `int* durability` 直接改成
//           `int durability;`（這裡根本不需要動態配置），
//           或改用 std::unique_ptr<int>。這樣所有特殊成員都不必自己寫，
//           複製／移動語意也自動正確。
//
// ⚠️ 陷阱. 「這個類別叫做『武器基類』，但解構函數沒有寫 virtual，
//           這一定是個 bug，應該馬上加上 virtual。」
//     答：以「目前的程式碼」而言不是 bug —— 本檔沒有任何衍生類別，
//         也從未透過基底指標 delete 衍生物件，
//         此時非 virtual 是正確且較省成本的選擇（物件不需要 vptr，
//         sizeof 較小，也不會妨礙某些最佳化）。
//         但它確實是一顆待爆的地雷：一旦有人依照「基類」這個命名
//         衍生出 Sword 並寫 `Weapon* w = new Sword(); delete w;`，
//         就會變成未定義行為，衍生類別的解構函數不會被呼叫。
//     為什麼會錯（兩個方向都會錯）：一種人看到「基類」就無腦加 virtual，
//         替不需要多型的類別平白加上 vptr；另一種人看到「現在沒問題」
//         就完全不管，等到有人繼承時才炸。
//         正確的判準只有一條：這個類別「會不會被當成多型基底使用」？
//         會 → virtual 解構函數；不會 → 維持非 virtual，
//         並且最好用 final 或 protected 非 virtual 解構函數把意圖寫清楚。
// ═══════════════════════════════════════════════════════════════════════════
//
// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】—— 本檔不附
//   理由：本檔主題是「多層動態配置的釋放順序與所有權管理」，
//   屬於資源管理機制。LeetCode 評測演算法的輸入輸出，
//   單次執行結束即由 OS 回收，不會因為釋放順序而有不同答案。
//   依規格「寧缺勿濫」從缺——本檔的武器工廠／軍械庫本身
//   就是遊戲開發中物件所有權管理的真實情境。
// -----------------------------------------------------------------------------

// 好的，信安！第三階段的最後一課。

// ---

// # 第 19 課：動態對象的創建與銷毀（new / delete）

// ---

// ## 19.1 回顧 C 語言的動態記憶體

// 在 C 語言中，你用 `malloc` / `free` 來管理動態記憶體：

// ```c
// // C 語言
// int* arr = (int*)malloc(10 * sizeof(int));  // 分配記憶體
// if (arr == NULL) { /* 處理錯誤 */ }
// // ... 使用 ...
// free(arr);                                    // 釋放記憶體
// ```

// C 語言的 `malloc` 有幾個問題：

// | 問題 | 說明 |
// |------|------|
// | 需要手動計算大小 | `sizeof(int) * 10`，容易算錯 |
// | 需要強制轉型 | `(int*)malloc(...)`，C++ 中必須轉型 |
// | 返回 `void*` | 沒有類型安全 |
// | 不調用建構函數 | 對於 C++ 類別，`malloc` 只分配記憶體，不初始化對象 |
// | `free` 不調用解構函數 | 對於 C++ 類別，資源不會被清理 |

// C++ 的 `new` / `delete` 解決了所有這些問題。

// ---

// ## 19.2 new 與 delete 的基本語法

// ```cpp
// #include <iostream>
// #include <string>
// using namespace std;

// class Hero {
// private:
//     string name;
//     int level;

// public:
//     Hero(const string& n, int lv) : name(n), level(lv) {
//         cout << "  [建構] " << name << " Lv." << level << endl;
//     }
    
//     ~Hero() {
//         cout << "  [解構] " << name << " Lv." << level << endl;
//     }
    
//     void print() const {
//         cout << "  " << name << " (Lv." << level << ")" << endl;
//     }
// };

// int main() {
//     cout << "=== new 與 delete 基本用法 ===" << endl;
    
//     // ====== 基本型別 ======
//     cout << "\n--- 基本型別 ---" << endl;
//     int* p1 = new int;          // 分配一個 int（未初始化）
//     int* p2 = new int(42);      // 分配一個 int 並初始化為 42
//     int* p3 = new int{100};     // C++11 大括號初始化
    
//     cout << "  *p1 = " << *p1 << " (垃圾值)" << endl;
//     cout << "  *p2 = " << *p2 << endl;
//     cout << "  *p3 = " << *p3 << endl;
    
//     delete p1;
//     delete p2;
//     delete p3;
    
//     // ====== 類別物件 ======
//     cout << "\n--- 類別物件 ---" << endl;
//     Hero* hero = new Hero("勇者", 10);   // new = 分配記憶體 + 調用建構函數
//     hero->print();
//     delete hero;                          // delete = 調用解構函數 + 釋放記憶體
    
//     cout << "\n--- 完成 ---" << endl;
//     return 0;
// }
// ```

// ### 預期輸出

// ```
// === new 與 delete 基本用法 ===

// --- 基本型別 ---
//   *p1 = 0 (垃圾值)
//   *p2 = 42
//   *p3 = 100

// --- 類別物件 ---
//   [建構] 勇者 Lv.10
//   勇者 (Lv.10)
//   [解構] 勇者 Lv.10

// --- 完成 ---
// ```

// ---

// ## 19.3 new 與 malloc 的本質差異

// `new` 做了**兩件事**，而 `malloc` 只做一件：

// ```
// new 的過程：
//   ┌──────────────────┐     ┌──────────────────┐
//   │ 1. 分配記憶體     │ ──→ │ 2. 調用建構函數   │
//   │ (operator new)   │     │ (Constructor)     │
//   └──────────────────┘     └──────────────────┘

// delete 的過程：
//   ┌──────────────────┐     ┌──────────────────┐
//   │ 1. 調用解構函數   │ ──→ │ 2. 釋放記憶體     │
//   │ (Destructor)     │     │ (operator delete) │
//   └──────────────────┘     └──────────────────┘

// malloc 的過程：
//   ┌──────────────────┐
//   │ 只分配記憶體      │     ← 不調用建構函數！
//   └──────────────────┘

// free 的過程：
//   ┌──────────────────┐
//   │ 只釋放記憶體      │     ← 不調用解構函數！
//   └──────────────────┘
// ```

// 用程式碼來驗證：

// ```cpp
// #include <iostream>
// #include <string>
// #include <cstdlib>
// using namespace std;

// class Widget {
// private:
//     string name;
//     int* data;

// public:
//     Widget(const string& n) : name(n) {
//         data = new int[10];
//         cout << "  [建構] " << name << "：分配了內部資源" << endl;
//     }
    
//     ~Widget() {
//         delete[] data;
//         cout << "  [解構] " << name << "：釋放了內部資源" << endl;
//     }
// };

// int main() {
//     cout << "=== new vs malloc ===" << endl;
    
//     // 正確：使用 new
//     cout << "\n--- 使用 new ---" << endl;
//     Widget* w1 = new Widget("正確的 Widget");
//     delete w1;   // 解構函數被調用，內部資源被釋放
    
//     // 錯誤示範（概念上）：如果用 malloc
//     cout << "\n--- 如果用 malloc（概念說明）---" << endl;
//     cout << "  Widget* w2 = (Widget*)malloc(sizeof(Widget));" << endl;
//     cout << "  // 記憶體分配了，但建構函數沒被調用！" << endl;
//     cout << "  // name 沒被初始化，data 沒被分配" << endl;
//     cout << "  // 使用 w2 會導致未定義行為！" << endl;
//     cout << "  // free(w2) 也不會調用解構函數，data 洩漏！" << endl;
    
//     // 我們不實際執行 malloc，因為會崩潰
    
//     return 0;
// }
// ```

// ### 預期輸出

// ```
// === new vs malloc ===

// --- 使用 new ---
//   [建構] 正確的 Widget：分配了內部資源
//   [解構] 正確的 Widget：釋放了內部資源

// --- 如果用 malloc（概念說明）---
//   Widget* w2 = (Widget*)malloc(sizeof(Widget));
//   // 記憶體分配了，但建構函數沒被調用！
//   // name 沒被初始化，data 沒被分配
//   // 使用 w2 會導致未定義行為！
//   // free(w2) 也不會調用解構函數，data 洩漏！
// ```

// **結論**：在 C++ 中，**永遠不要用 `malloc`/`free` 來管理類別物件**。只使用 `new`/`delete`。

// ---

// ## 19.4 new[] 與 delete[]：動態陣列

// ```cpp
// #include <iostream>
// #include <string>
// using namespace std;

// class Soldier {
// private:
//     int id;
//     static int nextId;

// public:
//     Soldier() : id(nextId++) {
//         cout << "  [建構] 士兵 #" << id << endl;
//     }
    
//     ~Soldier() {
//         cout << "  [解構] 士兵 #" << id << endl;
//     }
    
//     void report() const {
//         cout << "  士兵 #" << id << " 報到！" << endl;
//     }
// };

// int Soldier::nextId = 1;

// int main() {
//     cout << "=== new[] 與 delete[] ===" << endl;
    
//     // ====== 基本型別陣列 ======
//     cout << "\n--- 基本型別陣列 ---" << endl;
//     int* nums = new int[5];           // 5 個 int，未初始化
//     int* zeros = new int[5]();        // 5 個 int，全部初始化為 0
//     int* init = new int[5]{10, 20, 30, 40, 50};  // C++11 初始化列表
    
//     cout << "  nums:  ";
//     for (int i = 0; i < 5; i++) cout << nums[i] << " ";
//     cout << "(可能是垃圾值)" << endl;
    
//     cout << "  zeros: ";
//     for (int i = 0; i < 5; i++) cout << zeros[i] << " ";
//     cout << endl;
    
//     cout << "  init:  ";
//     for (int i = 0; i < 5; i++) cout << init[i] << " ";
//     cout << endl;
    
//     delete[] nums;
//     delete[] zeros;
//     delete[] init;
    
//     // ====== 類別物件陣列 ======
//     cout << "\n--- 類別物件陣列 ---" << endl;
//     cout << "  創建 3 個士兵：" << endl;
//     Soldier* squad = new Soldier[3];   // 調用 3 次預設建構函數
    
//     cout << "\n  點名：" << endl;
//     for (int i = 0; i < 3; i++) {
//         squad[i].report();
//     }
    
//     cout << "\n  解散：" << endl;
//     delete[] squad;   // 調用 3 次解構函數，然後釋放記憶體
    
//     return 0;
// }
// ```

// ### 預期輸出

// ```
// === new[] 與 delete[] ===

// --- 基本型別陣列 ---
//   nums:  0 0 0 0 0 (可能是垃圾值)
//   zeros: 0 0 0 0 0
//   init:  10 20 30 40 50

// --- 類別物件陣列 ---
//   創建 3 個士兵：
//   [建構] 士兵 #1
//   [建構] 士兵 #2
//   [建構] 士兵 #3

//   點名：
//   士兵 #1 報到！
//   士兵 #2 報到！
//   士兵 #3 報到！

//   解散：
//   [解構] 士兵 #3
//   [解構] 士兵 #2
//   [解構] 士兵 #1
// ```

// ---

// ## 19.5 new/delete 與 new[]/delete[] 不能混用

// 這是一個**非常危險**的錯誤：

// ```cpp
// #include <iostream>
// using namespace std;

// class Item {
// private:
//     int id;
//     static int nextId;
// public:
//     Item() : id(nextId++) {
//         cout << "  [+] Item #" << id << endl;
//     }
//     ~Item() {
//         cout << "  [-] Item #" << id << endl;
//     }
// };

// int Item::nextId = 1;

// int main() {
//     cout << "=== 正確配對 ===" << endl;
    
//     // 正確：new 配 delete
//     Item* single = new Item;
//     delete single;
    
//     // 正確：new[] 配 delete[]
//     Item* array = new Item[3];
//     delete[] array;
    
//     cout << "\n=== 錯誤配對（千萬不要這樣做！）===" << endl;
//     cout << "  // Item* p = new Item[3];" << endl;
//     cout << "  // delete p;      ← 錯誤！應該用 delete[]" << endl;
//     cout << "  // 後果：只解構第一個元素，其餘洩漏" << endl;
//     cout << "  //        或者直接崩潰（未定義行為）" << endl;
    
//     cout << "\n  // Item* q = new Item;" << endl;
//     cout << "  // delete[] q;    ← 錯誤！應該用 delete" << endl;
//     cout << "  // 後果：未定義行為，可能崩潰" << endl;
    
//     return 0;
// }
// ```

// ### 預期輸出

// ```
// === 正確配對 ===
//   [+] Item #1
//   [-] Item #1
//   [+] Item #2
//   [+] Item #3
//   [+] Item #4
//   [-] Item #4
//   [-] Item #3
//   [-] Item #2

// === 錯誤配對（千萬不要這樣做！）===
//   // Item* p = new Item[3];
//   // delete p;      ← 錯誤！應該用 delete[]
//   // 後果：只解構第一個元素，其餘洩漏
//   //        或者直接崩潰（未定義行為）

//   // Item* q = new Item;
//   // delete[] q;    ← 錯誤！應該用 delete
//   // 後果：未定義行為，可能崩潰
// ```

// ### 配對規則

// | 分配方式 | 釋放方式 | 混用後果 |
// |----------|----------|----------|
// | `new T` | `delete p` | 正確 |
// | `new T[n]` | `delete[] p` | 正確 |
// | `new T[n]` | `delete p` | **未定義行為！** |
// | `new T` | `delete[] p` | **未定義行為！** |

// ---

// ## 19.6 new 失敗時的處理

// 當系統沒有足夠的記憶體時，`new` 會拋出 `std::bad_alloc` 異常（和 C 語言 `malloc` 返回 `NULL` 不同）：

// ```cpp
// #include <iostream>
// #include <new>       // bad_alloc
// using namespace std;

// int main() {
//     cout << "=== new 的錯誤處理 ===" << endl;
    
//     // ====== 方式 1：用 try-catch 捕獲異常（標準方式）======
//     cout << "\n--- 方式 1：try-catch ---" << endl;
//     try {
//         // 嘗試分配巨大的記憶體（可能失敗）
//         // 這裡用一個合理大小來示範語法
//         int* p = new int[100];
//         cout << "  分配成功！" << endl;
//         delete[] p;
//     } catch (const bad_alloc& e) {
//         cout << "  記憶體分配失敗: " << e.what() << endl;
//     }
    
//     // ====== 方式 2：使用 nothrow 版本（返回 nullptr）======
//     cout << "\n--- 方式 2：nothrow ---" << endl;
//     int* p2 = new(nothrow) int[100];  // 失敗時返回 nullptr，不拋異常
//     if (p2 == nullptr) {
//         cout << "  記憶體分配失敗（返回 nullptr）" << endl;
//     } else {
//         cout << "  分配成功！" << endl;
//         delete[] p2;
//     }
    
//     // ====== 實際的分配失敗示範 ======
//     cout << "\n--- 嘗試分配超大記憶體 ---" << endl;
//     try {
//         // 嘗試分配大約 8TB 的記憶體（一定會失敗）
//         size_t hugeSize = 1000000000000ULL;
//         int* huge = new int[hugeSize];
//         delete[] huge;  // 不會執行到這裡
//     } catch (const bad_alloc& e) {
//         cout << "  預期中的失敗: " << e.what() << endl;
//     }
    
//     return 0;
// }
// ```

// ### 預期輸出

// ```
// === new 的錯誤處理 ===

// --- 方式 1：try-catch ---
//   分配成功！

// --- 方式 2：nothrow ---
//   分配成功！

// --- 嘗試分配超大記憶體 ---
//   預期中的失敗: std::bad_alloc
// ```

// ### new 的錯誤處理對比

// | 方式 | 語法 | 失敗行為 | 適用場景 |
// |------|------|----------|----------|
// | 標準 `new` | `new T` | 拋出 `bad_alloc` | 大多數情況（推薦） |
// | `nothrow` | `new(nothrow) T` | 返回 `nullptr` | 類似 C 風格的檢查 |

// ---

// ## 19.7 delete nullptr 是安全的

// ```cpp
// #include <iostream>
// using namespace std;

// int main() {
//     cout << "=== delete nullptr ===" << endl;
    
//     int* p = nullptr;
//     delete p;       // 完全安全！不會崩潰
//     cout << "  delete nullptr 是安全的" << endl;
    
//     int* arr = nullptr;
//     delete[] arr;   // 也是安全的
//     cout << "  delete[] nullptr 也是安全的" << endl;
    
//     // 但是 delete 同一個指標兩次是未定義行為！
//     cout << "\n  // int* q = new int(42);" << endl;
//     cout << "  // delete q;   ← 第一次 OK" << endl;
//     cout << "  // delete q;   ← 第二次：未定義行為！可能崩潰！" << endl;
    
//     // 好習慣：delete 後把指標設為 nullptr
//     int* safe = new int(42);
//     delete safe;
//     safe = nullptr;    // 設為 nullptr
//     delete safe;       // 再次 delete 也安全（因為是 nullptr）
//     cout << "\n  好習慣：delete 後設為 nullptr" << endl;
    
//     return 0;
// }
// ```

// ### 預期輸出

// ```
// === delete nullptr ===
//   delete nullptr 是安全的
//   delete[] nullptr 也是安全的

//   // int* q = new int(42);
//   // delete q;   ← 第一次 OK
//   // delete q;   ← 第二次：未定義行為！可能崩潰！

//   好習慣：delete 後設為 nullptr
// ```

// ---

// ## 19.8 記憶體洩漏的各種場景

// ```cpp
// #include <iostream>
// #include <string>
// using namespace std;

// class Resource {
// private:
//     string name;
// public:
//     Resource(const string& n) : name(n) {
//         cout << "  [+] " << name << endl;
//     }
//     ~Resource() {
//         cout << "  [-] " << name << endl;
//     }
// };

// void leak1_forget_delete() {
//     cout << "\n--- 洩漏 1：忘記 delete ---" << endl;
//     Resource* r = new Resource("被遺忘的資源");
//     // 忘記 delete r;
//     // r 指向的記憶體永遠無法釋放
// }

// void leak2_overwrite_pointer() {
//     cout << "\n--- 洩漏 2：覆蓋指標 ---" << endl;
//     Resource* r = new Resource("第一個資源");
//     r = new Resource("第二個資源");  // r 指向新對象
//     // 第一個資源的地址丟失了，永遠無法 delete！
//     delete r;  // 只釋放了第二個
// }

// void leak3_early_return() {
//     cout << "\n--- 洩漏 3：提前返回 ---" << endl;
//     Resource* r = new Resource("可能洩漏的資源");
    
//     bool error = true;  // 模擬錯誤
//     if (error) {
//         cout << "  發生錯誤，提前返回！" << endl;
//         return;         // 直接返回，忘記 delete！
//     }
    
//     delete r;   // 這行永遠不會執行
// }

// void leak4_exception() {
//     cout << "\n--- 洩漏 4：異常中斷 ---" << endl;
//     Resource* r = new Resource("異常中洩漏的資源");
    
//     // 如果這裡拋出異常...
//     throw runtime_error("模擬異常");
    
//     delete r;   // 這行永遠不會執行
// }

// int main() {
//     cout << "=== 記憶體洩漏的常見場景 ===" << endl;
    
//     leak1_forget_delete();
//     leak2_overwrite_pointer();
//     leak3_early_return();
    
//     try {
//         leak4_exception();
//     } catch (const exception& e) {
//         cout << "  捕獲異常: " << e.what() << endl;
//     }
    
//     cout << "\n=== 注意：以上有多個資源沒被解構 ===" << endl;
//     return 0;
// }
// ```

// ### 預期輸出

// ```
// === 記憶體洩漏的常見場景 ===

// --- 洩漏 1：忘記 delete ---
//   [+] 被遺忘的資源

// --- 洩漏 2：覆蓋指標 ---
//   [+] 第一個資源
//   [+] 第二個資源
//   [-] 第二個資源

// --- 洩漏 3：提前返回 ---
//   [+] 可能洩漏的資源
//   發生錯誤，提前返回！

// --- 洩漏 4：異常中斷 ---
//   [+] 異常中洩漏的資源
//   捕獲異常: 模擬異常

// === 注意：以上有多個資源沒被解構 ===
// ```

// 有三個資源（被遺忘的、第一個、可能洩漏的、異常中的）從來沒有看到 `[-]` 的解構訊息——它們洩漏了。

// ---

// ## 19.9 解決方案：用局部對象管理動態記憶體

// 最簡單的解決方案是**讓類別自己管理記憶體**，利用解構函數自動清理：

// ```cpp
// #include <iostream>
// #include <string>
// using namespace std;

// // ============================================================
// // 一個簡單的動態字串類別（模擬 std::string 的核心概念）
// // ============================================================
// class MyString {
// private:
//     char* data;
//     int length;

// public:
//     // 建構：分配記憶體
//     MyString(const char* str = "") {
//         length = 0;
//         while (str[length] != '\0') length++;
        
//         data = new char[length + 1];   // +1 給 '\0'
//         for (int i = 0; i <= length; i++) {
//             data[i] = str[i];
//         }
        
//         cout << "  [建構] MyString: \"" << data << "\" (長度: " 
//              << length << ")" << endl;
//     }
    
//     // 解構：自動釋放記憶體
//     ~MyString() {
//         cout << "  [解構] MyString: \"" << data << "\"" << endl;
//         delete[] data;     // 自動清理！
//         data = nullptr;
//     }
    
//     void print() const {
//         cout << "  \"" << data << "\"" << endl;
//     }
    
//     int getLength() const { return length; }
// };

// // 不管怎麼離開這個函數，MyString 都會自動清理
// void safeFunction(bool earlyReturn) {
//     MyString greeting("Hello, C++!");
    
//     if (earlyReturn) {
//         cout << "  提前返回..." << endl;
//         return;   // greeting 自動解構，記憶體自動釋放
//     }
    
//     greeting.print();
//     // greeting 在函數結束時自動解構
// }

// int main() {
//     cout << "=== 用局部對象管理記憶體 ===" << endl;
    
//     cout << "\n--- 正常流程 ---" << endl;
//     safeFunction(false);
    
//     cout << "\n--- 提前返回 ---" << endl;
//     safeFunction(true);
    
//     cout << "\n--- 異常安全 ---" << endl;
//     try {
//         MyString msg("即將拋出異常");
//         throw runtime_error("boom!");
//         // msg 在異常傳播過程中自動解構（堆疊展開）
//     } catch (...) {
//         cout << "  異常已捕獲，記憶體已自動清理" << endl;
//     }
    
//     cout << "\n=== 完成 ===" << endl;
//     return 0;
// }
// ```

// ### 預期輸出

// ```
// === 用局部對象管理記憶體 ===

// --- 正常流程 ---
//   [建構] MyString: "Hello, C++!" (長度: 11)
//   "Hello, C++!"
//   [解構] MyString: "Hello, C++!"

// --- 提前返回 ---
//   [建構] MyString: "Hello, C++!" (長度: 11)
//   提前返回...
//   [解構] MyString: "Hello, C++!"

// --- 異常安全 ---
//   [建構] MyString: "即將拋出異常" (長度: 18)
//   [解構] MyString: "即將拋出異常"
//   異常已捕獲，記憶體已自動清理

// === 完成 ===
// ```

// 不管是正常返回、提前返回還是異常——記憶體都被正確釋放了。這就是 **RAII 的威力**。

// ---

// ## 19.10 現代 C++ 的建議：避免裸 new/delete

// C++11 引入了智能指標（第 94-96 課會詳細學），它們把 RAII 的概念進一步封裝，讓你幾乎不需要手動寫 `new` / `delete`：

// ```cpp
// #include <iostream>
// #include <string>
// #include <memory>    // 智能指標
// using namespace std;

// class Monster {
// private:
//     string name;
// public:
//     Monster(const string& n) : name(n) {
//         cout << "  [+] " << name << " 出現" << endl;
//     }
//     ~Monster() {
//         cout << "  [-] " << name << " 消失" << endl;
//     }
//     void roar() const {
//         cout << "  " << name << " 吼叫！" << endl;
//     }
// };

// int main() {
//     cout << "=== 裸 new/delete vs 智能指標 ===" << endl;
    
//     // ====== 舊方式：裸 new/delete ======
//     cout << "\n--- 舊方式（裸指標）---" << endl;
//     {
//         Monster* m = new Monster("哥布林");
//         m->roar();
//         delete m;       // 必須手動 delete！
//     }
    
//     // ====== 新方式：unique_ptr ======
//     cout << "\n--- 新方式（unique_ptr）---" << endl;
//     {
//         // make_unique 會自動調用 new，unique_ptr 離開作用域時自動 delete
//         unique_ptr<Monster> m = make_unique<Monster>("史萊姆");
//         m->roar();
//         // 不需要 delete！unique_ptr 自動處理！
//     }
    
//     // ====== 異常安全 ======
//     cout << "\n--- 異常安全示範 ---" << endl;
//     try {
//         unique_ptr<Monster> m = make_unique<Monster>("龍");
//         m->roar();
//         throw runtime_error("勇者逃跑了！");
//         // m 在堆疊展開時自動解構，不會洩漏
//     } catch (const exception& e) {
//         cout << "  " << e.what() << endl;
//     }
    
//     // ====== 動態陣列 ======
//     cout << "\n--- 動態陣列（unique_ptr）---" << endl;
//     {
//         unique_ptr<int[]> arr = make_unique<int[]>(5);
//         for (int i = 0; i < 5; i++) {
//             arr[i] = (i + 1) * 10;
//         }
        
//         cout << "  ";
//         for (int i = 0; i < 5; i++) {
//             cout << arr[i] << " ";
//         }
//         cout << endl;
//         // 不需要 delete[]！
//     }
    
//     cout << "\n=== 所有記憶體已自動管理 ===" << endl;
//     return 0;
// }
// ```

// ### 預期輸出

// ```
// === 裸 new/delete vs 智能指標 ===

// --- 舊方式（裸指標）---
//   [+] 哥布林 出現
//   哥布林 吼叫！
//   [-] 哥布林 消失

// --- 新方式（unique_ptr）---
//   [+] 史萊姆 出現
//   史萊姆 吼叫！
//   [-] 史萊姆 消失

// --- 異常安全示範 ---
//   [+] 龍 出現
//   龍 吼叫！
//   [-] 龍 消失
//   勇者逃跑了！

// --- 動態陣列（unique_ptr）---
//   10 20 30 40 50

// === 所有記憶體已自動管理 ===
// ```

// ---

// ## 19.11 new/delete 完整使用指南

// ### 什麼時候用 new？

// | 場景 | 是否需要 new | 推薦替代方案 |
// |------|-------------|-------------|
// | 臨時的小對象 | **不需要** | 用局部對象（棧） |
// | 大型陣列 | 看情況 | 用 `std::vector` |
// | 多型（基類指標指向派生類） | 需要動態分配 | 用 `unique_ptr` / `shared_ptr` |
// | 對象的生命週期超越作用域 | 需要動態分配 | 用 `unique_ptr` / `shared_ptr` |
// | 工廠函數返回對象 | 需要動態分配 | 用 `unique_ptr` 返回 |
// | 學習底層原理 | 直接使用 | 學完後改用智能指標 |

// ### 現代 C++ 的準則

// ```
// 1. 能用棧上對象就用棧上對象
// 2. 需要動態分配時，用 make_unique / make_shared
// 3. 只在極少數底層場景才用裸 new/delete
// 4. 永遠不要在現代 C++ 中用 malloc/free
// ```

// ---

// ## 19.12 完整綜合範例：對象工廠

// ```cpp
// #include <iostream>
// #include <string>
// using namespace std;

// // ============================================================
// // 武器基類
// // ============================================================
// class Weapon {
// protected:
//     string name;
//     int damage;
//     int* durability;   // 動態分配的耐久度

// public:
//     Weapon(const string& n, int dmg, int dur) 
//         : name(n), damage(dmg) 
//     {
//         durability = new int(dur);   // 動態分配
//         cout << "  [鍛造] " << name 
//              << " (攻擊:" << damage 
//              << ", 耐久:" << *durability << ")" << endl;
//     }
    
//     ~Weapon() {
//         cout << "  [銷毀] " << name << endl;
//         delete durability;           // 記得釋放！
//     }
    
//     void use() {
//         if (*durability > 0) {
//             (*durability) -= 10;
//             cout << "  使用 " << name 
//                  << " 攻擊！(耐久: " << *durability << ")" << endl;
//         } else {
//             cout << "  " << name << " 已損壞！" << endl;
//         }
//     }
    
//     void print() const {
//         cout << "  " << name 
//              << " [攻擊:" << damage 
//              << " 耐久:" << *durability << "]" << endl;
//     }
// };

// // ============================================================
// // 武器工廠（使用裸 new，展示手動管理）
// // ============================================================
// class WeaponFactory {
// public:
//     // 工廠方法：根據類型創建武器
//     static Weapon* create(const string& type) {
//         if (type == "劍") {
//             return new Weapon("鐵劍", 25, 100);
//         } else if (type == "弓") {
//             return new Weapon("長弓", 20, 80);
//         } else if (type == "杖") {
//             return new Weapon("法杖", 35, 60);
//         } else {
//             return new Weapon("木棍", 5, 200);
//         }
//     }
// };

// int main() {
//     cout << "============================================" << endl;
//     cout << "   第 19 課：new/delete 綜合範例" << endl;
//     cout << "============================================" << endl;
    
//     // 創建武器
//     cout << "\n=== 武器鍛造 ===" << endl;
//     Weapon* sword = WeaponFactory::create("劍");
//     Weapon* bow = WeaponFactory::create("弓");
//     Weapon* staff = WeaponFactory::create("杖");
    
//     // 使用武器
//     cout << "\n=== 戰鬥 ===" << endl;
//     sword->use();
//     sword->use();
//     bow->use();
//     staff->use();
    
//     // 查看狀態
//     cout << "\n=== 武器狀態 ===" << endl;
//     sword->print();
//     bow->print();
//     staff->print();
    
//     // 清理——必須手動 delete 每一個！
//     cout << "\n=== 清理武器 ===" << endl;
//     delete sword;
//     delete bow;
//     delete staff;
    
//     // 動態陣列版本
//     cout << "\n=== 批量創建（陣列）===" << endl;
//     const int BATCH_SIZE = 3;
//     Weapon** armory = new Weapon*[BATCH_SIZE];  // 指標陣列
    
//     armory[0] = WeaponFactory::create("劍");
//     armory[1] = WeaponFactory::create("弓");
//     armory[2] = WeaponFactory::create("杖");
    
//     cout << "\n  武器庫清單：" << endl;
//     for (int i = 0; i < BATCH_SIZE; i++) {
//         armory[i]->print();
//     }
    
//     // 必須先 delete 每個 Weapon，再 delete 指標陣列
//     cout << "\n=== 清空武器庫 ===" << endl;
//     for (int i = 0; i < BATCH_SIZE; i++) {
//         delete armory[i];    // 釋放每把武器
//     }
//     delete[] armory;          // 釋放指標陣列本身
    
//     cout << "\n=== 所有資源已清理 ===" << endl;
//     return 0;
// }
// ```

// ### 編譯與執行

// ```bash
// g++ -std=c++17 -o lesson19 lesson19.cpp
// ./lesson19
// ```

// ### 預期輸出

// ```
// ============================================
//    第 19 課：new/delete 綜合範例
// ============================================

// === 武器鍛造 ===
//   [鍛造] 鐵劍 (攻擊:25, 耐久:100)
//   [鍛造] 長弓 (攻擊:20, 耐久:80)
//   [鍛造] 法杖 (攻擊:35, 耐久:60)

// === 戰鬥 ===
//   使用 鐵劍 攻擊！(耐久: 90)
//   使用 鐵劍 攻擊！(耐久: 80)
//   使用 長弓 攻擊！(耐久: 70)
//   使用 法杖 攻擊！(耐久: 50)

// === 武器狀態 ===
//   鐵劍 [攻擊:25 耐久:80]
//   長弓 [攻擊:20 耐久:70]
//   法杖 [攻擊:35 耐久:50]

// === 清理武器 ===
//   [銷毀] 鐵劍
//   [銷毀] 長弓
//   [銷毀] 法杖

// === 批量創建（陣列）===
//   [鍛造] 鐵劍 (攻擊:25, 耐久:100)
//   [鍛造] 長弓 (攻擊:20, 耐久:80)
//   [鍛造] 法杖 (攻擊:35, 耐久:60)

//   武器庫清單：
//   鐵劍 [攻擊:25 耐久:100]
//   長弓 [攻擊:20 耐久:80]
//   法杖 [攻擊:35 耐久:60]

// === 清空武器庫 ===
//   [銷毀] 鐵劍
//   [銷毀] 長弓
//   [銷毀] 法杖

// === 所有資源已清理 ===
// ```

// ---

// ## 19.13 本課重點回顧

// | 概念 | 說明 |
// |------|------|
// | `new` 做兩件事 | 分配記憶體 + 調用建構函數 |
// | `delete` 做兩件事 | 調用解構函數 + 釋放記憶體 |
// | `new[]` / `delete[]` | 陣列版本，**必須配對使用** |
// | 不能混用 | `new` 配 `delete`，`new[]` 配 `delete[]`，否則未定義行為 |
// | `new` 失敗 | 拋出 `bad_alloc`，或用 `new(nothrow)` 返回 `nullptr` |
// | `delete nullptr` | 安全，不會崩潰 |
// | 記憶體洩漏 | 忘記 delete、覆蓋指標、提前返回、異常中斷 |
// | RAII 解決方案 | 用類別的解構函數自動管理動態記憶體 |
// | 現代 C++ 建議 | 用 `unique_ptr` / `shared_ptr` 取代裸 `new`/`delete` |
// | 永遠不用 malloc | C++ 類別必須用 `new`/`delete`，不要用 `malloc`/`free` |

// ---

// ## 19.14 第三階段總結

// 恭喜你完成了**第三階段：建構與解構**！讓我們回顧整個階段：

// | 課次 | 主題 | 核心概念 |
// |------|------|----------|
// | 第 13 課 | 建構函數基礎 | 對象創建時自動初始化 |
// | 第 14 課 | 預設建構函數 | `= default`、`= delete`、編譯器自動生成規則 |
// | 第 15 課 | 帶參數的建構函數 | const 引用、`explicit`、窄化防護 |
// | 第 16 課 | 初始化列表 | 效能優勢、四種必須使用的情況 |
// | 第 17 課 | 解構函數 | 自動資源釋放、RAII 概念 |
// | 第 18 課 | 對象生命週期 | 四種存儲期、常見陷阱 |
// | 第 19 課 | new / delete | 動態對象管理、現代替代方案 |

// 這七課建立了一個完整的概念鏈：

// ```
// 對象誕生 → 初始化（建構函數 + 初始化列表）
//    ↓
// 對象存活 → 使用（成員函數）
//    ↓
// 對象死亡 → 清理（解構函數）
//    ↓
// 資源管理 → RAII（建構獲取 + 解構釋放）
// ```

// ---

// ## 19.15 下一課預告

// 下一課進入 **第四階段：封裝深入**，從 **第 20 課：封裝（Encapsulation）的意義** 開始。我們將深入理解為什麼要封裝、好的封裝設計長什麼樣、以及封裝在大型專案中的實際價值。

// 準備好進入 **第 20 課：封裝的意義** 了嗎？



#include <iostream>
#include <string>
using namespace std;

// ============================================================
// 武器基類
// ============================================================
class Weapon {
protected:
    string name;
    int damage;
    int* durability;   // 動態分配的耐久度

public:
    Weapon(const string& n, int dmg, int dur) 
        : name(n), damage(dmg) 
    {
        durability = new int(dur);   // 動態分配
        cout << "  [鍛造] " << name 
             << " (攻擊:" << damage 
             << ", 耐久:" << *durability << ")" << endl;
    }
    
    ~Weapon() {
        cout << "  [銷毀] " << name << endl;
        delete durability;           // 記得釋放！
    }
    
    void use() {
        if (*durability > 0) {
            (*durability) -= 10;
            cout << "  使用 " << name 
                 << " 攻擊！(耐久: " << *durability << ")" << endl;
        } else {
            cout << "  " << name << " 已損壞！" << endl;
        }
    }
    
    void print() const {
        cout << "  " << name 
             << " [攻擊:" << damage 
             << " 耐久:" << *durability << "]" << endl;
    }
};

// ============================================================
// 武器工廠（使用裸 new，展示手動管理）
// ============================================================
class WeaponFactory {
public:
    // 工廠方法：根據類型創建武器
    static Weapon* create(const string& type) {
        if (type == "劍") {
            return new Weapon("鐵劍", 25, 100);
        } else if (type == "弓") {
            return new Weapon("長弓", 20, 80);
        } else if (type == "杖") {
            return new Weapon("法杖", 35, 60);
        } else {
            return new Weapon("木棍", 5, 200);
        }
    }
};

int main() {
    cout << "============================================" << endl;
    cout << "   第 19 課：new/delete 綜合範例" << endl;
    cout << "============================================" << endl;
    
    // 創建武器
    cout << "\n=== 武器鍛造 ===" << endl;
    Weapon* sword = WeaponFactory::create("劍");
    Weapon* bow = WeaponFactory::create("弓");
    Weapon* staff = WeaponFactory::create("杖");
    
    // 使用武器
    cout << "\n=== 戰鬥 ===" << endl;
    sword->use();
    sword->use();
    bow->use();
    staff->use();
    
    // 查看狀態
    cout << "\n=== 武器狀態 ===" << endl;
    sword->print();
    bow->print();
    staff->print();
    
    // 清理——必須手動 delete 每一個！
    cout << "\n=== 清理武器 ===" << endl;
    delete sword;
    delete bow;
    delete staff;
    
    // 動態陣列版本
    cout << "\n=== 批量創建（陣列）===" << endl;
    const int BATCH_SIZE = 3;
    Weapon** armory = new Weapon*[BATCH_SIZE];  // 指標陣列
    
    armory[0] = WeaponFactory::create("劍");
    armory[1] = WeaponFactory::create("弓");
    armory[2] = WeaponFactory::create("杖");
    
    cout << "\n  武器庫清單：" << endl;
    for (int i = 0; i < BATCH_SIZE; i++) {
        armory[i]->print();
    }
    
    // 必須先 delete 每個 Weapon，再 delete 指標陣列
    cout << "\n=== 清空武器庫 ===" << endl;
    for (int i = 0; i < BATCH_SIZE; i++) {
        delete armory[i];    // 釋放每把武器
    }
    delete[] armory;          // 釋放指標陣列本身
    
    cout << "\n=== 所有資源已清理 ===" << endl;
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 19 課：動態對象的創建與銷毀（new  delete）10.cpp" -o weaponfactory

// ── 輸出但書 ────────────────────────────────────────────────────────────
// 1. 本檔輸出逐位元組可重現（實測連跑 5 次 md5 相同）。
// 2. 判讀重點：每一個 [鍛造] 都有對應的 [銷毀]，前後兩批共 6 把武器
//    全部釋放乾淨，沒有洩漏。特別注意「清空武器庫」那一段 ——
//    三個 [銷毀] 來自逐一 delete armory[i]，
//    之後才 delete[] armory 釋放指標陣列本身（那一步沒有輸出，
//    因為釋放指標不會呼叫任何解構函數）。
// 3. 耐久度的變化（100→80、80→70、60→50）來自戰鬥階段的扣減，
//    是程式邏輯的結果，不涉及任何不確定值。
// 4. 本檔的 Weapon 違反 Rule of Three（裸指標 + 自訂解構函數，
//    未處理複製語意），僅因全程透過指標操作、從未複製而安全；
//    直接複製會造成 double free（未定義行為），本檔未執行。
// 5. ~Weapon() 目前非 virtual，在「無衍生類別」的現況下正確；
//    若日後引入多型繼承並以 Weapon* delete 衍生物件，必須改為 virtual。
// 6. 本機環境：GCC 15.2.0 (Ubuntu 15.2.0-16ubuntu1) / libstdc++ / x86-64。

// === 預期輸出 ===
// ============================================
//    第 19 課：new/delete 綜合範例
// ============================================
//
// === 武器鍛造 ===
//   [鍛造] 鐵劍 (攻擊:25, 耐久:100)
//   [鍛造] 長弓 (攻擊:20, 耐久:80)
//   [鍛造] 法杖 (攻擊:35, 耐久:60)
//
// === 戰鬥 ===
//   使用 鐵劍 攻擊！(耐久: 90)
//   使用 鐵劍 攻擊！(耐久: 80)
//   使用 長弓 攻擊！(耐久: 70)
//   使用 法杖 攻擊！(耐久: 50)
//
// === 武器狀態 ===
//   鐵劍 [攻擊:25 耐久:80]
//   長弓 [攻擊:20 耐久:70]
//   法杖 [攻擊:35 耐久:50]
//
// === 清理武器 ===
//   [銷毀] 鐵劍
//   [銷毀] 長弓
//   [銷毀] 法杖
//
// === 批量創建（陣列）===
//   [鍛造] 鐵劍 (攻擊:25, 耐久:100)
//   [鍛造] 長弓 (攻擊:20, 耐久:80)
//   [鍛造] 法杖 (攻擊:35, 耐久:60)
//
//   武器庫清單：
//   鐵劍 [攻擊:25 耐久:100]
//   長弓 [攻擊:20 耐久:80]
//   法杖 [攻擊:35 耐久:60]
//
// === 清空武器庫 ===
//   [銷毀] 鐵劍
//   [銷毀] 長弓
//   [銷毀] 法杖
//
// === 所有資源已清理 ===
