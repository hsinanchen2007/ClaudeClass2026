// =============================================================================
//  第 8 課：對象（object）的創建與使用2.cpp  —  在堆積上建立物件（new / delete）
// =============================================================================
//
// 【主題資訊 Information】
//   T* p = new T();      // 配置記憶體 + 呼叫建構子，回傳指標
//   delete p;            // 呼叫解構子 + 釋放記憶體
//   p->member            // 等價於 (*p).member
//   儲存期：動態儲存期（dynamic storage duration）—— 由程式設計者決定何時結束
//   標準版本：C++98 起（nullptr 為 C++11；智慧指標亦為 C++11）
//   複雜度：new/delete 通常 O(1) 均攤，但常數遠大於堆疊配置
//   標頭檔：語言核心特性；智慧指標在 <memory>
//
// 【詳細解釋 Explanation】
//
// 【1. new 做了兩件事，delete 也做了兩件事】
//       new T()  = ① operator new 配置 sizeof(T) 的原始記憶體
//                  ② 在那塊記憶體上呼叫 T 的建構子
//       delete p = ① 呼叫 *p 的解構子
//                  ② operator delete 把記憶體還給 allocator
//   把這兩步分開理解很重要，因為它們可以被拆開使用
//   （placement new 就是「只做第 ②步」），
//   也因為【忘記 delete 只漏掉第二步的記憶體，但解構子也沒跑】——
//   如果那個物件持有檔案 handle 或鎖，後果比單純漏記憶體嚴重得多。
//
// 【2. new T() 與 new T 的差別（很多人不知道）】
//     • new T   —— 【預設初始化】。內建型別成員不會被清零，值不確定。
//     • new T() —— 【值初始化】。對沒有自訂建構子的類別，
//                   內建型別成員會被【零初始化】。
//   本檔寫的是 new Dog()，加了括號。
//   對本檔的 Dog 而言差別不大（age 有類內初始值 0），
//   但對一個 struct Point { int x; int y; }：
//       new Point   → x、y 值不確定
//       new Point() → x、y 都是 0
//   這對括號經常決定了「有沒有 UB」。
//
// 【3. 為什麼要用 -> 而不是 .】
//   ptr 是一個【指標】，它本身只是一個位址（本機 8 bytes），
//   沒有 name 這個成員。要先解參考才能拿到物件：
//       (*ptr).name        // 明確的兩步：先解參考，再取成員
//       ptr->name          // 完全等價的簡寫
//   -> 存在的理由純粹是可讀性 —— (*a).b 在鏈式存取時
//   會變成 (*(*(*a).b).c).d，而用 -> 則是 a->b->c->d。
//
// 【4. 「delete 後設為 nullptr」是好習慣，但不是萬靈丹】
//   本檔的 ptr = nullptr; 有兩個實際好處：
//     ① 避免懸空指標（dangling pointer）被誤用
//     ② 對 nullptr 呼叫 delete 是【安全且無作用】的，
//        所以能防止重複 delete（重複 delete 非 null 指標是 UB）
//   但它救不了【其他指向同一塊記憶體的指標】：
//       Dog* a = new Dog();
//       Dog* b = a;
//       delete a; a = nullptr;     // b 仍然指向已釋放的記憶體
//   b 現在是懸空的，而且無法從 b 本身判斷 ——
//   這正是為什麼現代 C++ 用智慧指標表達所有權，而不是靠紀律。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 本檔正確，但現代 C++ 不該這樣寫
//   本檔的 new/delete 是配對正確的，但它【不是例外安全】的：
//       Dog* ptr = new Dog();
//       doSomething();          // 若這裡丟出例外
//       delete ptr;             // 這一行永遠不會執行 → 洩漏
//   正解是 RAII：
//       auto ptr = std::make_unique<Dog>();   // C++14
//   離開作用域時（無論正常結束或例外）都保證釋放，而且沒有額外成本。
//   C++ Core Guidelines 的建議是：一般程式碼中【不應出現裸露的 new/delete】。
//
// (B) new 失敗會怎樣
//   預設的 operator new 在配置失敗時【丟出 std::bad_alloc】，
//   不會回傳 nullptr（這與 C 的 malloc 不同）。
//   所以 if (ptr == nullptr) 這種檢查對預設的 new 是無效的 ——
//   要拿到 nullptr 必須用 new (std::nothrow) T()。
//   這是從 C 轉過來的人最常見的錯誤觀念之一。
//
// (C) delete 與 delete[] 不可混用
//       T* p = new T[10];
//       delete p;            // 未定義行為！必須用 delete[]
//   兩者不對稱使用是 UB，即使 T 是內建型別（實作上可能「看似正常」）。
//   這也是 std::vector 存在的理由之一 —— 完全不需要記這條規則。
//
// 【注意事項 Pay Attention】
//   1. new 與 delete 必須配對；new[] 必須配 delete[]，混用是 UB。
//   2. 預設的 new 失敗時【丟 std::bad_alloc】，不回傳 nullptr。
//   3. new T 與 new T() 不同：後者會值初始化（內建型別成員清零）。
//   4. delete 後設為 nullptr 能防止重複 delete，
//      但救不了其他指向同一塊記憶體的指標。
//   5. 現代 C++ 應改用 std::unique_ptr / std::make_unique（C++14），
//      裸 new/delete 無法在例外發生時保證釋放。
//   6. 忘記 delete 不只漏記憶體 —— 解構子也不會執行，
//      持有的檔案、鎖、連線都不會被釋放。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】new / delete 與動態物件
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. new 和 malloc 有什麼差別？
//     答：至少四點：
//         ① new 會【呼叫建構子】，malloc 只給你一塊原始記憶體。
//         ② new 失敗時【丟出 std::bad_alloc】，malloc 回傳 NULL。
//         ③ new 是運算子（可被多載、可 placement），malloc 是函式。
//         ④ new 知道型別，會自動算大小；malloc 要自己寫 sizeof。
//         對應地，delete 會呼叫解構子而 free 不會 ——
//         所以兩者【絕對不能混用】（malloc 的記憶體用 delete 釋放是 UB）。
//     追問：那 new 底層有用到 malloc 嗎？
//           → 多數實作的 operator new 確實是包裝 malloc，
//             但這是實作細節、標準不保證，不能依賴它來混用。
//
// 🔥 Q2. delete 之後把指標設為 nullptr，有什麼好處？能解決懸空指標嗎？
//     答：兩個好處：避免自己誤用已釋放的指標；
//         以及 delete nullptr 是安全無作用的，可防止重複 delete
//         （重複 delete 一個非 null 指標是 UB）。
//         但它【不能】解決懸空指標的根本問題 ——
//         若還有別的指標指向同一塊記憶體，那些指標依然是懸空的，
//         而且無法從指標本身判斷它是否有效。
//     追問：那正確做法是什麼？
//           → 用所有權明確的智慧指標：
//             std::unique_ptr（獨佔）或 std::shared_ptr（共享）。
//             需要「可能已失效」的觀察者時用 std::weak_ptr，
//             它可以透過 lock() 安全地檢查對象是否還活著。
//
// ⚠️ 陷阱. 「這段程式碼有 delete，所以沒有記憶體洩漏」—— 什麼情況下這是錯的？
//     答：只要 new 與 delete 之間可能丟出例外（或有 early return），
//         delete 就會被跳過：
//             Dog* ptr = new Dog();
//             doSomething();      // 丟例外 → 堆疊展開
//             delete ptr;         // 永遠不會執行
//         此時 ptr 這個【區域變數】被清掉了，
//         但它指向的堆積記憶體沒有任何人記得，形成洩漏。
//     為什麼會錯：把「程式碼裡有寫 delete」等同於「delete 一定會執行」。
//         C++ 的控制流不是只有循序執行 —— 例外會讓中間的程式碼被整段跳過。
//         這正是 RAII 存在的理由：把釋放動作交給【解構子】，
//         而解構子在堆疊展開時是【保證】會被呼叫的。
//         所以現代 C++ 的答案不是「小心記得 delete」，
//         而是「用 unique_ptr，讓編譯器保證它一定發生」。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
using namespace std;

class Dog {
public:
    string name;
    int age = 0;

    void bark() {
        cout << name << " 汪汪！" << endl;
    }
};

int main() {
    Dog* ptr = new Dog();    // 在堆上建立，返回指標
    ptr->name = "阿花";      // 用 -> 存取成員
    ptr->age = 2;
    ptr->bark();

    delete ptr;              // 必須手動釋放！否則會造成記憶體洩漏
    ptr = nullptr;           // 好習慣：釋放後設為 nullptr, 避免野指標

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 8 課：對象（object）的創建與使用2.cpp" -o obj2
// 檢查: g++ -std=c++17 -fsanitize=address -g "第 8 課：對象（object）的創建與使用2.cpp" -o obj2_asan
//       （AddressSanitizer 能偵測洩漏、重複 delete 與釋放後使用）

// 註 1:本檔輸出是【完全確定的】—— 沒有輸入、亂數、執行緒或位址。
//      特別注意本檔【不會】印出任何指標位址；
//      位址每次執行都不同（ASLR），不適合寫進預期輸出。
//
// 註 2:本檔的 new/delete 配對正確、沒有洩漏，
//      但它【不是例外安全】的 —— 若在 delete 之前丟出例外，
//      那一行就永遠不會執行。現代 C++ 應改用
//      auto ptr = std::make_unique<Dog>();（C++14）。
//
// 註 3:ptr = nullptr; 是好習慣（可防止重複 delete，
//      因為 delete nullptr 是安全無作用的），
//      但它救不了其他指向同一塊記憶體的指標。
//
// 註 4:本檔寫的是 new Dog()【有括號】——
//      對沒有自訂建構子的類別，加括號是值初始化（內建成員清零），
//      不加則是預設初始化（值不確定）。這對括號經常決定了有沒有 UB。
//
// 【LeetCode 實戰範例】從缺 —— 理由：
//      本檔主題是「動態記憶體的手動管理」。
//      LeetCode 的設計題（146、155、705、707、1603、1656）
//      在 C++ 中一律用 STL 容器實作，
//      評測系統也不檢查記憶體管理方式；
//      唯一沾邊的 707 Design Linked List 若用裸指標實作，
//      重點會整個偏移到節點的增刪演算法上，反而模糊了本檔的焦點。

// === 預期輸出 ===
// 阿花 汪汪！
