/*
================================================================================
【C++_SmartPointers/summary.cpp】

本目錄主題：智慧指標（Smart Pointers）— 用「所有權」思維寫出更安全的 C++

本檔目標（課件式）：
  - 把智慧指標當成「所有權模型」來教：誰負責 delete？生命週期由誰決定？
  - 逐一示範最常用的 member functions（每個都用最小、清楚的例子）
  - 附錄提供 cppreference 風格速查表（冷門/進階只提示，不硬塞主線）

你要背的總結（工作上超重要）：
  - ✅ 預設選 unique_ptr：單一擁有、最便宜、語意最清楚
  - ✅ 只有「真的需要共享生命週期」才用 shared_ptr
  - ✅ shared_ptr 要避免循環引用：用 weak_ptr 打破環
  - ✅ 幾乎不要手寫 new/delete：用 make_unique/make_shared

編譯：
  g++ -std=c++17 -Wall -Wextra summary.cpp -o summary && ./summary
================================================================================
*/

/*
補充筆記：C++_SmartPointers/C++_SmartPointers summary
  - 如果兩個範例看起來都能完成同一件事，優先比較它們是否擁有資料、是否配置記憶體、是否改變輸入。
  - C++_SmartPointers/C++_SmartPointers summary 屬於智慧指標；先判斷所有權是獨占、共享，還是只是觀察。
  - unique_ptr 表達獨占所有權，不可複製但可 move；shared_ptr 表達共享所有權，weak_ptr 打破循環引用。
  - make_unique/make_shared 通常比裸 new 更安全；它們把配置和建構包在一起，降低例外中途洩漏風險。
  - get() 只取得借用指標，不轉移所有權；不要 delete get() 回傳值。
  - shared_ptr 的 control block 很重要；不要用同一裸指標建立多個 shared_ptr。
  - custom deleter 用於 FILE*、C API handle、特殊釋放函式；deleter 型別也會影響 unique_ptr 型別大小。
  - 這個 summary.cpp 只做章節整理，不新增題庫題解；需要實作練習時回到各主題檔。
  - C++_SmartPointers/C++_SmartPointers summary 的複習方式是把 API 依用途分組，再比較輸入條件、輸出語意、失敗狀態和複雜度。
  - 初學複習 summary 時，不要只背函式名稱；要能說出何時該用、何時不該用、和相近工具差在哪裡。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】智慧指標總覽（unique_ptr / shared_ptr / weak_ptr）
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. unique_ptr、shared_ptr、weak_ptr 的差別與適用場景？
//     答：unique_ptr = 獨佔所有權，不可 copy 只可 move，沒有自訂 deleter 時大小等同
//         裸指標，是預設首選。shared_ptr = 共享所有權，以引用計數管理，成本是原子計數
//         加上通常兩個指標的大小（常見實作）。weak_ptr = 不擁有的觀察者，不影響物件
//         存活，用來打破循環引用與做會自動失效的快取。
//     追問：為什麼「預設用 unique_ptr」？（大多數場景的所有權是單一且明確的；
//         unique_ptr 可以【單向】轉成 shared_ptr，反之不行——先選嚴格的那個，
//         之後需要共享再放寬。⚠️ 但這個轉換不是零成本：本機以自訂 operator new
//         計數實測，轉換當下會配置 1 次記憶體來建 control block。）
//
// 🔥 Q2. 智慧指標該怎麼當函式參數傳遞？
//     答：① 函式只是「使用」物件、不涉及所有權 → 傳 T& 或 const T&（或裸 T*）
//         ② 函式要「接管」所有權 → 傳 unique_ptr<T> by value（呼叫端須 std::move）
//         ③ 函式要「共享」所有權、會把它存起來 → 傳 shared_ptr<T> by value
//         ④ 「可能存、也可能不存」（函式自己決定要不要多留一份 owner）
//            → 才傳 const shared_ptr<T>&（Core Guidelines R.30、F.7）
//         ⚠️ 注意「只讀不存」不是 ④ 而是 ①：不涉及所有權就【根本不要收
//            shared_ptr】，傳 const T& 或 T*。用 const shared_ptr<T>& 來
//            「避免原子遞增」是常見的錯誤推理 —— 真正的修法是把 shared_ptr
//            從介面上拿掉，而不是改成傳它的 const 參考（那還是把呼叫端綁死
//            在「必須持有 shared_ptr」上，stack 物件、成員物件都傳不進來）。
//         反模式：無腦到處傳 shared_ptr by value。
//
// Q3. 為什麼幾乎不該手寫 new / delete？
//     答：make_unique（C++14）與 make_shared 把「配置」與「建構」包在一起，關掉了例外
//         中途離開造成的洩漏窗口，也讓型別名只需要寫一次。更重要的是：裸 new 的結果
//         最好連「離開建構表達式」的機會都不要有 —— 一旦裸指標流出去，就有人可能把
//         它交給兩個 smart pointer，變成 double free。
//
// ⚠️ 陷阱. release() 與 reset() 哪一個會刪除物件？
//     答：reset() 會刪除；release() 不會 —— 它只是放棄所有權並把裸指標交還給你，忘了
//         delete 就是洩漏（本檔 demo_unique_ptr 中的 leaked 就必須手動 delete）。
//     為什麼會錯：名字誤導 —— 「release」聽起來像「釋放記憶體」，但它實際的意思是
//         「釋放所有權」。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <memory>
#include <string>
#include <utility>

struct Trace {
    explicit Trace(std::string name) : name(std::move(name)) {
        std::cout << "  Trace(" << this->name << ") ctor\n";
    }
    ~Trace() { std::cout << "  Trace(" << name << ") dtor\n"; }
    std::string name;
};

static void header(const char* name) { std::cout << "\n[" << name << "]\n"; }

// -----------------------------------------------------------------------------
// 【重點 1】unique_ptr：唯一擁有（move-only）
// -----------------------------------------------------------------------------
// 你可以把 unique_ptr 想成「會自動 delete 的指標」+「所有權不可複製」。
// 它的設計目的是：
//   - 讓 ownership（誰負責釋放）在型別層級就清楚
//   - 避免忘記 delete 或例外早退導致洩漏
//
// 常用 member functions（工作上常用）：
//   - get()      ：拿出裸指標（不移交所有權）
//   - operator* / operator->：像指標一樣用
//   - release()  ：放棄所有權，回傳裸指標（⚠️ 你接手 delete）
//   - reset(p)   ：釋放舊物件並改管理新物件（或 reset() 釋放變成空）
//   - swap()     ：交換所有權
static void demo_unique_ptr() {
    header("demo_unique_ptr");

    auto p = std::make_unique<Trace>("unique");
    std::cout << "  p owns? " << static_cast<bool>(p) << "\n";

    // operator->：像指標一樣用
    std::cout << "  p->name=" << p->name << "\n";

    // get()：拿裸指標（不轉移所有權）
    Trace* raw = p.get();
    std::cout << "  raw==p.get()? " << (raw == p.get()) << "\n";

    // unique_ptr 不能拷貝，只能移動
    auto q = std::move(p);
    std::cout << "  after move: p? " << static_cast<bool>(p) << ", q? " << static_cast<bool>(q) << "\n";

    // reset()：釋放目前物件（q 變空）
    q.reset();
    std::cout << "  after reset(): q? " << static_cast<bool>(q) << "\n";

    // reset(new T)：改管理另一個物件（同時會釋放舊物件）
    q.reset(new Trace("unique2")); // 教學示範；實務更建議 make_unique
    std::cout << "  after reset(new): q->name=" << q->name << "\n";

    // release()：放棄所有權（⚠️ 你要負責 delete）
    Trace* leaked = q.release();
    std::cout << "  after release(): q? " << static_cast<bool>(q) << ", leaked=" << leaked->name << "\n";
    delete leaked; // ⚠️ 沒刪會漏
}

// -----------------------------------------------------------------------------
// 【重點 2】shared_ptr：共享擁有（引用計數）
// -----------------------------------------------------------------------------
// shared_ptr 的成本與風險：
//   - 成本：引用計數（通常是 atomic）+ control block 配置
//   - 風險：循環引用（A 持有 B，B 持有 A）會讓引用計數永遠不為 0 → 永不釋放
//
// 常用 member functions：
//   - use_count()：引用計數（僅供觀察/除錯，不要拿來寫同步邏輯）
//   - reset()：釋放一個引用
//   - get()：拿裸指標（不影響引用計數）
//   - operator* / operator->：像指標一樣用
//   - unique()：是否只有一個 owner（注意：這也只建議除錯/觀察）
static void demo_shared_ptr_and_weak_ptr() {
    header("demo_shared_ptr_and_weak_ptr");

    std::shared_ptr<Trace> sp = std::make_shared<Trace>("shared");
    std::weak_ptr<Trace> wp = sp;

    std::cout << "  use_count=" << sp.use_count() << "\n";
    {
        auto sp2 = sp;
        std::cout << "  after copy: use_count=" << sp.use_count() << "\n";
        std::cout << "  sp2.get()==sp.get()? " << (sp2.get() == sp.get()) << "\n";
    }
    std::cout << "  after scope: use_count=" << sp.use_count() << "\n";
    std::cout << "  unique()? " << sp.unique() << "\n";

    // weak_ptr.lock()：若物件還活著，取得 shared_ptr；否則得到空
    if (auto locked = wp.lock()) {
        std::cout << "  weak_ptr lock ok: " << locked->name << "\n";
    }

    sp.reset(); // 釋放最後一個 shared_ptr（物件會被銷毀）
    if (wp.expired()) std::cout << "  weak_ptr expired\n";

    // expired 後 lock() 會得到空 shared_ptr
    if (auto locked = wp.lock()) {
        std::cout << "  should not happen: " << locked->name << "\n";
    } else {
        std::cout << "  lock() after expired => empty\n";
    }
}

// enable_shared_from_this：避免「把 this 包成新的 shared_ptr」造成 double delete
struct Node : std::enable_shared_from_this<Node> {
    explicit Node(int id) : id(id) {}
    int id{};

    std::shared_ptr<Node> self() { return shared_from_this(); }
};

static void demo_enable_shared_from_this() {
    header("demo_enable_shared_from_this");

    auto n = std::make_shared<Node>(7);
    auto me = n->self();
    std::cout << "  n.use_count=" << n.use_count() << " (>=2)\n";
    std::cout << "  me->id=" << me->id << "\n";
}

static void demo_common_pitfalls_note() {
    header("demo_common_pitfalls_note");
    std::cout << "  1) 不要用 shared_ptr 管理同一個裸指標兩次（會 double delete）。\n";
    std::cout << "  2) shared_ptr 之間互相持有會循環引用 → 用 weak_ptr 打破環。\n";
    std::cout << "  3) 介面回傳所有權：unique_ptr 表示 transfer ownership。\n";
    std::cout << "  4) shared_ptr 的 use_count 不是同步工具；不要拿它當『鎖』或『狀態』判斷。\n";
}

int main() {
    demo_unique_ptr();
    demo_shared_ptr_and_weak_ptr();
    demo_enable_shared_from_this();
    demo_common_pitfalls_note();

    std::cout << "\n[done]\n";
    return 0;
}

/*
================================================================================
【附錄：cppreference 風格速查（常用 member functions）】

unique_ptr<T, Deleter>
  - Observers: get, operator*, operator->, operator bool
  - Modifiers: release, reset, swap
  - (非 member) make_unique（C++14 起，建議用）

shared_ptr<T>
  - Observers: get, operator*, operator->, operator bool, use_count, unique
  - Modifiers: reset, swap
  - (非 member) make_shared（建議用）
  - 進階：aliasing constructor、owner_before（工作上較少用，需時再查）

weak_ptr<T>
  - Observers: expired, lock, use_count
  - Modifiers: reset, swap

enable_shared_from_this<T>
  - shared_from_this / weak_from_this（C++17 有 weak_from_this，但看需求再用）
================================================================================
*/

// 編譯: g++ -std=c++20 -Wall -Wextra summary.cpp -o summary

// === 預期輸出 (節錄) ===
//
// [demo_unique_ptr]
//   Trace(unique) ctor
//   p owns? 1
//   p->name=unique
//   raw==p.get()? 1
//   after move: p? 0, q? 1
//   Trace(unique) dtor
//   after reset(): q? 0
//   Trace(unique2) ctor
//   after reset(new): q->name=unique2
//   after release(): q? 0, leaked=unique2
//   Trace(unique2) dtor
//
// [demo_shared_ptr_and_weak_ptr]
//   Trace(shared) ctor
//   use_count=1
//   after copy: use_count=2
//   sp2.get()==sp.get()? 1
//   after scope: use_count=1
// …（後略，完整輸出共 37 行）
