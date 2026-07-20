// =============================================================================
//  第 18 課：對象的生命週期 9  —  野指標（Dangling Pointer）與擁有權
// =============================================================================
//
// 【主題資訊 Information】
//   主題：回傳區域物件位址造成的野指標，以及「誰擁有這塊記憶體」的設計問題。
//   狀態：本檔可以編譯、可以執行。
//   ★ 但編譯時 gcc/clang 會發出警告：
//        warning: address of local variable 'local' returned [-Wreturn-local-addr]
//     這個警告是刻意保留的教學重點。main() 中「使用」該野指標的程式碼
//     已被註解掉，因為那是未定義行為，不該被寫成預期輸出。
//   標頭檔：<iostream>、<memory>、<vector>
//
// 【詳細解釋 Explanation】
//
// 【1. 野指標與懸空引用是同一件事】
//   8.cpp 回傳的是參考，本檔回傳的是位址，底層完全相同——
//   都是「指向已結束生命週期的記憶體」。
//   差別只在寫法：指標版本比較容易被看出來（有明顯的 & 和 *），
//   參考版本則偽裝成普通變數，更危險。
//
// 【2. 三種常見的野指標來源】
//   (a) 回傳區域變數的位址（本檔的 dangerousPointer()）
//   (b) delete 之後繼續使用該指標
//         Item* p = new Item(1); delete p; p->id;      // 野指標
//   (c) 容器重新配置後，舊的元素指標/迭代器失效
//         int* p = &v[0]; v.push_back(x); *p;          // v 可能已搬家
//   ★ (c) 最容易被忽略：vector 一旦成長超過 capacity，就會配置新記憶體、
//     搬移全部元素、釋放舊記憶體，所有指向舊元素的指標/參考/迭代器全部失效。
//     本檔有可觀察的示範（只印「容量是否改變」這種確定性資訊，
//     不印任何位址——位址每次執行都不同）。
//
// 【3. 真正的問題不是指標，而是「擁有權」沒有被表達】
//   裸指標 Item* 完全沒有說明：
//       這塊記憶體是誰配置的？誰該負責釋放？可以釋放幾次？
//       函式回傳它，是把擁有權交給我，還是只借我看？
//   這些資訊只存在於文件與註解裡，編譯器無法檢查，人也會忘記。
//   現代 C++ 的解法是「用型別把擁有權寫出來」：
//       std::unique_ptr<T>   我獨佔擁有，離開作用域就釋放
//       std::shared_ptr<T>   多方共享，最後一個離開的負責釋放
//       std::weak_ptr<T>     我只觀察，不延長生命，用前必須 lock()
//       T*  /  T&            我只是借用，絕不釋放（non-owning）
//   一旦用型別表達，「該不該 delete」就不再是靠記憶，而是靠編譯器。
//
// 【4. 本檔為什麼把 new/delete 換成 unique_ptr 作為示範】
//   原始程式的「正確做法」是 new + delete。它確實正確，但仍有風險：
//   中間若有 return 或例外，delete 就不會執行（見第 19 課的洩漏四場景）。
//   unique_ptr 讓釋放綁定在作用域上，任何離開路徑都會自動釋放，
//   而且不需要額外成本（unique_ptr 的大小與裸指標相同，本檔會實測印出）。
//
// 【概念補充 Concept Deep Dive】
//   ● 為什麼「delete 之後把指標設為 nullptr」是好習慣？
//     因為 delete nullptr 是明確定義的安全操作（什麼都不做），
//     所以設成 nullptr 可以讓「不小心 double delete」變成無害。
//     但它救不了「別的變數也指向同一塊記憶體」的情況——
//     那些副本仍然是野指標。這正是裸指標最難管的地方。
//   ● 位址被重用：釋放的記憶體可能立刻被下一次配置拿去用，
//     於是野指標可能指向一個「完全合法但屬於別人」的物件，
//     寫入它會安靜地破壞不相干的資料。這比崩潰難查得多。
//   ● 本檔刻意不印任何原始位址：位址每次執行都不同，
//     把它寫成預期輸出既無意義也無法驗證。
//     要說明「東西有沒有搬家」，正確做法是印出「是否相同」的布林值。
//
// 【注意事項 Pay Attention】
//   1. 本檔會產生 -Wreturn-local-addr 警告，這是刻意保留的教學重點。
//   2. 使用野指標是未定義行為——不可寫成「一定崩潰」或「一定印出某值」。
//      它可能安靜地讀到看似正常的資料，那才是最危險的。
//   3. vector 成長會使所有既有的指標/參考/迭代器失效；
//      reserve() 可以避免中途重新配置，但一旦超過 capacity 仍會失效。
//   4. delete 後設 nullptr 只保護「這一個」指標變數，不保護它的副本。
//   5. sizeof(unique_ptr<T>) 與 sizeof(T*) 相同（本機 g++ 15.2 實測皆為 8），
//      屬實作定義；有自訂 deleter 時可能更大。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】野指標與擁有權
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 野指標有哪些常見來源？
//     答：三種。(1) 回傳區域變數的位址；(2) delete 之後繼續使用該指標；
//         (3) 容器重新配置後，舊的元素指標/迭代器失效——例如
//         int* p = &v[0]; 之後 v.push_back() 觸發擴容，p 立刻失效。
//         第三種最常被忽略，因為程式碼看起來完全無關。
//     追問：怎麼避免第三種？
//         → 不要長期保存指向容器元素的指標；改存「索引」，
//           或先 reserve() 足夠容量。但索引仍會被 erase 打亂，
//           最穩的是需要時才取。
//
// 🔥 Q2. 為什麼現代 C++ 建議用智慧指標取代裸指標？
//     答：因為裸指標沒有表達「擁有權」。看到 Item* 完全不知道
//         該不該 delete、能不能 delete 兩次、生命週期歸誰管。
//         智慧指標把這件事寫進型別：unique_ptr 表示獨佔、
//         shared_ptr 表示共享、weak_ptr 表示只觀察、
//         裸指標/參考則約定為「只借用，絕不釋放」。
//         如此一來，記憶體管理從「靠記憶」變成「靠編譯器」。
//     追問：unique_ptr 有額外成本嗎？
//         → 沒有。它的大小與裸指標相同（本機實測皆為 8 bytes），
//           解參考也會被完全內聯，是零成本抽象。
//
// ⚠️ 陷阱. delete p; 之後把 p 設成 nullptr，就安全了嗎？
//     答：只保護了 p 這一個變數。如果之前有 Item* q = p; 這樣的副本，
//         q 仍然指向已釋放的記憶體，而且它不會因為 p 被設為 nullptr
//         而跟著改變。此時透過 q 存取仍是未定義行為。
//     為什麼會錯：把「指標」和「它指向的記憶體」混為一談。
//         設 nullptr 改的是「指標變數的值」，
//         而記憶體是否有效是另一件獨立的事。一塊記憶體可能被
//         任意多個指標指著，delete 一次就讓「全部」失效，
//         你卻只能改到手上這一個。這正是為什麼擁有權必須由型別
//         （unique_ptr / shared_ptr）來管理，而不是靠人工約定。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <memory>
#include <vector>
using namespace std;

class Item {
public:
    int id;
    Item(int i) : id(i) {
        cout << "  [+] Item #" << id << endl;
    }
    ~Item() {
        cout << "  [-] Item #" << id << endl;
    }
};

// ★ 編譯時會出現：warning: address of local variable 'local' returned
//   這個警告是本課的重點，刻意保留。
Item* dangerousPointer() {
    Item local(99);
    return &local;    // 返回局部對象的地址——危險！
}   // local 死亡，返回的指標成為野指標, 使用它會導致未定義行為！
// 這段程式碼展示了 C++ 中對象生命週期的陷阱：
// - dangerousPointer() 函數返回了一個局部對象 local 的地址，但 local 在函數結束時就被解構了，
// 因此返回的指標成為野指標，使用它會導致未定義行為！


// -----------------------------------------------------------------------------
// 【日常實務範例】用型別表達擁有權：工廠回傳 unique_ptr
//   情境：一個任務工廠依型別建立任務物件。
//   ● 若回傳裸指標 Task*，呼叫端必須自己記得 delete——
//     一旦中途 return 或拋例外就洩漏（第 19 課會詳談）。
//   ● 回傳 unique_ptr<Task> 則把「你擁有它、離開作用域自動釋放」
//     寫進型別，編譯器與讀者都一目了然。
// -----------------------------------------------------------------------------
class Task {
    string name;
    int    priority;
public:
    Task(const string& n, int p) : name(n), priority(p) {
        cout << "    [建立] 任務 " << name << "（優先度 " << priority << "）" << endl;
    }
    ~Task() { cout << "    [銷毀] 任務 " << name << endl; }
    void run() const { cout << "    [執行] " << name << endl; }
    int  getPriority() const { return priority; }
};

// 回傳型別就說明了擁有權轉移：呼叫端拿到後自動負責釋放
unique_ptr<Task> makeTask(const string& name, int priority) {
    return make_unique<Task>(name, priority);
}

// 參數用「裸指標/參考」表示 non-owning：我只是借來看，絕不釋放
void inspectTask(const Task* t) {
    if (t == nullptr) {                 // 借用的指標可能是 null，要檢查
        cout << "    (沒有任務)" << endl;
        return;
    }
    cout << "    [檢視] 優先度 = " << t->getPriority() << endl;
}

// 註：本檔不加 LeetCode 範例。
//     野指標與擁有權是記憶體安全與 API 設計的議題，屬未定義行為範疇，
//     無法寫出可驗證的預期輸出；LeetCode 也不考這類錯誤模式，故從缺。

int main() {
    cout << "=== 陷阱：野指標 ===" << endl;
    
    // Item* ptr = dangerousPointer();
    // cout << ptr->id << endl;  // 未定義行為！
    cout << "  dangerousPointer() 的呼叫已刻意註解掉" << endl;
    cout << "  原因：使用野指標是未定義行為，沒有「預期輸出」可言" << endl;
    cout << "  請注意編譯時的 -Wreturn-local-addr 警告，那就是本課重點" << endl;
    
    // 正確做法：用 new 分配
    cout << "\n=== 正確做法 ===" << endl;
    Item* safePtr = new Item(99);
    cout << "  safePtr->id = " << safePtr->id << endl;
    delete safePtr;
    safePtr = nullptr;        // 好習慣：避免誤用已釋放的指標

    // ====== 更好的做法：讓型別管理生命週期 ======
    cout << "\n=== 更好的做法：unique_ptr（離開作用域自動釋放）===" << endl;
    {
        unique_ptr<Item> owned = make_unique<Item>(100);
        cout << "  owned->id = " << owned->id << endl;
        cout << "  sizeof(unique_ptr<Item>) = " << sizeof(unique_ptr<Item>)
             << "，sizeof(Item*) = " << sizeof(Item*)
             << "（本機 g++ 15.2 實測，屬實作定義）" << endl;
    }   // 不需要 delete，離開作用域自動釋放

    // ====== 容器擴容導致既有指標失效 ======
    cout << "\n=== 常被忽略的野指標來源：容器重新配置 ===" << endl;
    {
        vector<int> v;
        v.reserve(2);                      // 先只保留 2 格
        v.push_back(10);
        v.push_back(20);

        int* p = &v[0];                    // 指向第一個元素
        size_t capBefore = v.capacity();
        bool sameBefore = (p == &v[0]);

        v.push_back(30);                   // 超過 capacity → 重新配置 + 搬移

        size_t capAfter = v.capacity();
        bool stillSame = (p == &v[0]);     // 只比較「是否相同」，不印位址

        cout << "  push_back 前 capacity = " << capBefore
             << "，p 是否指向 v[0]: " << (sameBefore ? "是" : "否") << endl;
        cout << "  push_back 後 capacity = " << capAfter
             << "，p 是否仍指向 v[0]: " << (stillSame ? "是" : "否") << endl;
        cout << "  → capacity 改變代表 vector 已搬家，p 成為野指標" << endl;
        cout << "  → 此後解參考 p 是未定義行為，故本檔不讀取它" << endl;
        cout << "  註：capacity 的成長倍率是實作定義（libstdc++ 為 2 倍）" << endl;
    }

    cout << "\n=== 日常實務：用型別表達擁有權 ===" << endl;
    {
        unique_ptr<Task> task = makeTask("匯出報表", 5);
        inspectTask(task.get());          // 借用：傳裸指標，不轉移擁有權
        task->run();

        inspectTask(nullptr);             // 借用的指標可以是 null
        cout << "  離開作用域，unique_ptr 自動釋放：" << endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 18 課：對象的生命週期（Object Lifetime）9.cpp" -o life9
//   ★ 編譯時會出現一個刻意保留的警告（這正是本課重點）：
//       warning: address of local variable 'local' returned [-Wreturn-local-addr]

// 【輸出說明】
//   1. dangerousPointer() 的呼叫已被註解，執行期不會觸發未定義行為。
//   2. 本檔刻意不印任何原始位址（位址每次執行都不同、無法驗證），
//      而是印出「指標是否仍指向 v[0]」這種確定性的布林結果。
//   3. vector 的 capacity 成長倍率為實作定義；下列 2 → 4 是本機
//      g++ 15.2 / libstdc++ 的實測值，不是標準保證。

// === 預期輸出 ===
// === 陷阱：野指標 ===
//   dangerousPointer() 的呼叫已刻意註解掉
//   原因：使用野指標是未定義行為，沒有「預期輸出」可言
//   請注意編譯時的 -Wreturn-local-addr 警告，那就是本課重點
//
// === 正確做法 ===
//   [+] Item #99
//   safePtr->id = 99
//   [-] Item #99
//
// === 更好的做法：unique_ptr（離開作用域自動釋放）===
//   [+] Item #100
//   owned->id = 100
//   sizeof(unique_ptr<Item>) = 8，sizeof(Item*) = 8（本機 g++ 15.2 實測，屬實作定義）
//   [-] Item #100
//
// === 常被忽略的野指標來源：容器重新配置 ===
//   push_back 前 capacity = 2，p 是否指向 v[0]: 是
//   push_back 後 capacity = 4，p 是否仍指向 v[0]: 否
//   → capacity 改變代表 vector 已搬家，p 成為野指標
//   → 此後解參考 p 是未定義行為，故本檔不讀取它
//   註：capacity 的成長倍率是實作定義（libstdc++ 為 2 倍）
//
// === 日常實務：用型別表達擁有權 ===
//     [建立] 任務 匯出報表（優先度 5）
//     [檢視] 優先度 = 5
//     [執行] 匯出報表
//     (沒有任務)
//   離開作用域，unique_ptr 自動釋放：
//     [銷毀] 任務 匯出報表
