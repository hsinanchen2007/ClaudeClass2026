// =====================================================================
// 主題 02: std::shared_ptr  (共享擁有權的智慧指標)
// =====================================================================
// 參考來源:
//   https://en.cppreference.com/w/cpp/memory/shared_ptr
//   https://cplusplus.com/reference/memory/shared_ptr/
//
// ---------------------------------------------------------------------
// 一、什麼是 shared_ptr?
// ---------------------------------------------------------------------
// std::shared_ptr 是 C++11 引入的智慧指標, 定義於 <memory>。
// 它的核心語意是「共享擁有權 (shared ownership)」:
//   - 多個 shared_ptr 可以「同時擁有」同一個物件。
//   - 內部以「引用計數 (reference count)」追蹤目前有多少 shared_ptr
//     指向該物件。
//   - 當「最後一個」shared_ptr 被銷毀或重設時, 引用計數歸零, 物件
//     被自動 delete。
//
// ---------------------------------------------------------------------
// 二、Control Block (控制區塊)
// ---------------------------------------------------------------------
// shared_ptr 內部其實是「兩個指標」:
//   1. 指向「被管理物件」的指標
//   2. 指向「control block」的指標
//      control block 至少包含:
//         - shared count   (有幾個 shared_ptr 指向)
//         - weak count     (有幾個 weak_ptr 指向)
//         - 物件指標 / deleter / allocator (依建構方式而定)
//
// 重點: 使用 std::make_shared<T>(...) 建立時, 物件本身與 control block
// 會配置在「同一塊記憶體」(只一次 malloc), 比 shared_ptr<T>(new T(...))
// 更有效率, 也更例外安全。
//
// ---------------------------------------------------------------------
// 三、為什麼 / 何時要用 shared_ptr?
// ---------------------------------------------------------------------
// 當「擁有者不只一個」、且「不確定誰會最後釋放」時:
//   - 圖、樹、DAG 中, 某些節點被多個父節點共享
//   - 觀察者模式中, 多個 subject 持有同一個 observer
//   - 快取系統中, 多個用戶共享同一個資源句柄
//
// 反之, 若擁有權「明確獨占」, 應用 unique_ptr 而非 shared_ptr,
// 因為 shared_ptr 有引用計數的時間/空間額外成本。
//
// ---------------------------------------------------------------------
// 四、基本用法
// ---------------------------------------------------------------------
//   auto p = std::make_shared<T>(args...);   // 推薦寫法
//   std::shared_ptr<T> q(new T(args...));    // 也可以, 但較不建議
//
//   p.use_count()   -> 目前的引用計數
//   p.reset()       -> 放棄擁有權; 計數 -1
//   p.reset(new T)  -> 放棄舊物件, 改持有新物件
//   *p, p->x        -> 解參照語法
//   p1 == p2        -> 比較底層指標是否相同
//   if (p) {...}    -> bool 轉換: 是否持有物件
//
// ---------------------------------------------------------------------
// 五、執行緒安全 (Thread Safety)
// ---------------------------------------------------------------------
// - 「control block 內的引用計數」操作是 atomic, 安全。
// - 但「指向同一個 shared_ptr 物件本身的多執行緒讀寫」並非執行緒安全
//   (寫 vs 寫、寫 vs 讀仍需同步)。
// - 「指向的物件」本身的內容操作, 仍需使用者自行保護 (例如 mutex)。
//
// ---------------------------------------------------------------------
// 六、常見陷阱
// ---------------------------------------------------------------------
// 1. 不要由「同一個 raw pointer」建立多個獨立的 shared_ptr:
//      Foo* raw = new Foo();
//      std::shared_ptr<Foo> a(raw);
//      std::shared_ptr<Foo> b(raw);  // ❌ 兩個獨立 control block, double free
//    正確做法: a 用 raw 建立, b 由 a 複製而來。
//
// 2. 循環引用 (cycle): 兩個物件互持 shared_ptr 會永遠不歸零。
//    解法: 其中一邊改用 weak_ptr (見下一章)。
//
// 3. 效能成本: 引用計數需要 atomic 操作, 高頻路徑要注意。
//
// =====================================================================

/*
補充筆記：std::shared_ptr
  - shared_ptr 表示共享所有權；每次拷貝都會影響引用計數，這不是免費操作。
  - make_shared 通常配置較少，但大型物件和 weak_ptr 共存時要理解控制區塊生命週期。
  - 循環引用不會自動釋放，雙向關係通常要有一邊改成 weak_ptr。
  - std::shared_ptr 屬於智慧指標；先判斷所有權是獨占、共享，還是只是觀察。
  - unique_ptr 表達獨占所有權，不可複製但可 move；shared_ptr 表達共享所有權，weak_ptr 打破循環引用。
  - make_unique/make_shared 通常比裸 new 更安全；它們把配置和建構包在一起，降低例外中途洩漏風險。
  - get() 只取得借用指標，不轉移所有權；不要 delete get() 回傳值。
  - shared_ptr 的 control block 很重要；不要用同一裸指標建立多個 shared_ptr。
  - custom deleter 用於 FILE*、C API handle、特殊釋放函式；deleter 型別也會影響 unique_ptr 型別大小。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::shared_ptr
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. control block 裡面有什麼？shared_ptr 本身多大？
//     答：cppreference 明列 control block 含：指向被管理物件的指標（或物件本身）、
//         型別擦除的 deleter、型別擦除的 allocator、strong count、weak count。
//         shared_ptr 物件本身則存兩個指標（物件指標 + control block 指標），所以
//         sizeof(shared_ptr) 通常是裸指標的兩倍 —— 這是常見實作，並非標準保證。
//     追問：為什麼要存兩個指標，不從 control block 查？（為了支援 aliasing
//         constructor，以及解引用時少一次間接定址）
//
// 🔥 Q2. shared_ptr 是 thread-safe 的嗎？
//     答：必須分三層講。① control block 的引用計數操作是 atomic，多執行緒同時複製或
//         銷毀「不同的 shared_ptr 實例」（即使指向同一物件）是安全的，物件保證只被
//         刪一次。② 被管理的物件本身完全不受保護，讀寫它仍要自行同步。③ 多執行緒
//         存取「同一個 shared_ptr 實例」且其中含非 const 操作（reset、指派）就是
//         data race；C++20 起應改用 std::atomic<std::shared_ptr<T>>。
//     追問：原子計數的代價？（比非原子遞增顯著昂貴，多核競爭時尤其明顯；這正是
//         「別到處亂傳 shared_ptr，優先傳 const T& 或裸指標」的理由）
//
// 🔥 Q3. make_shared 與 shared_ptr<T>(new T) 差在哪？什麼時候「不該」用 make_shared？
//     答：make_shared 一次配置就同時容納 control block 與物件（少一次 allocation、
//         cache locality 較好），也避開建構過程中的洩漏窗口。不該用它的情況：
//         ① 需要自訂 deleter ② 需要接管既有的裸指標 ③ 物件很大又會有長壽 weak_ptr
//         —— 單次配置下，物件那塊記憶體要等最後一個 weak_ptr 消失才歸還
//         ④ 建構子是 private/protected（make_shared 不是 friend）。
//     追問：物件與 control block 各自何時被釋放？（strong count 歸零 → 解構物件；
//         control block 要等 weak count 也歸零才釋放）
//
// Q4. aliasing constructor 是什麼？
//     答：shared_ptr<U>(sp, ptr) —— 共用 sp 的 control block（因此維持擁有者的生命
//         週期），但 get() 回傳你指定的 ptr。用來安全地持有某個大物件的成員或子物件，
//         同時保證整個母物件不被銷毀。
//
// ⚠️ 陷阱. 用同一個裸指標建立兩個 shared_ptr 會怎樣？
//     答：會產生「兩個獨立的 control block」，各自計數歸零、各刪一次 → double free
//         （UB）。shared_ptr<T> b(a.get()); 是同一個 bug 的另一種寫法。
//     為什麼會錯：多數人腦中以為 shared_ptr 會「認得」這個指標已經被管理了。但引用
//         計數在 control block 裡、不在物件裡 —— 裸指標只能交給 smart pointer 一次。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

// =====================================================================
// 範例 (1): 基本用法 - 觀察引用計數的變化
// =====================================================================
void example_basic() {
    std::cout << "--- 範例 1: 基本用法 ---\n";
    auto p1 = std::make_shared<std::string>("hello");
    std::cout << "p1.use_count() = " << p1.use_count() << "\n"; // 1

    {
        auto p2 = p1;                                // 複製 -> 計數 +1
        std::cout << "after copy, use_count = " << p1.use_count() << "\n"; // 2
        std::cout << "*p2 = " << *p2 << "\n";
    } // p2 離開 scope, 計數 -1

    std::cout << "after scope, use_count = " << p1.use_count() << "\n"; // 1
    // 函式結束, p1 釋放, 計數歸零, 字串被自動 delete
}

// =====================================================================
// 範例 (2): 共享資源 - 多個擁有者場景
// =====================================================================
// 觀念: 同一份資料被多個物件共用時, shared_ptr 確保最後一個釋放者
//       才真正 delete 資料, 避免任何一方提早釋放。
struct Image {
    std::string name;
    explicit Image(std::string n) : name(std::move(n)) {
        std::cout << "  [load image: " << name << "]\n";
    }
    ~Image() { std::cout << "  [free image: " << name << "]\n"; }
};

struct Window {
    std::string title;
    std::shared_ptr<Image> bg;                       // 共享背景圖
    Window(std::string t, std::shared_ptr<Image> img)
        : title(std::move(t)), bg(std::move(img)) {}
};

void example_shared_resource() {
    std::cout << "--- 範例 2: 共享資源 ---\n";
    auto bg = std::make_shared<Image>("wallpaper.png");
    Window w1("Editor",   bg);
    Window w2("Terminal", bg);
    std::cout << "image use_count = " << bg.use_count() << "\n"; // 3
    // 三者 (bg, w1.bg, w2.bg) 都離開後, Image 才會被釋放
}

// =====================================================================
// 範例 (3): 簡易快取 - 日常工作可能用到
// =====================================================================
// 觀念: 用 shared_ptr 做快取, 多個 caller 共用同一份載入結果。
//       這比 raw pointer 安全, 且不需手動管理生命週期。
class TextureCache {
    std::unordered_map<std::string, std::shared_ptr<std::string>> cache_;
public:
    std::shared_ptr<std::string> get(const std::string& key) {
        auto it = cache_.find(key);
        if (it != cache_.end()) return it->second;
        auto data = std::make_shared<std::string>("data_of_" + key);
        cache_[key] = data;
        return data;
    }
};

void example_cache() {
    std::cout << "--- 範例 3: 簡易快取 ---\n";
    TextureCache cache;
    auto a = cache.get("logo");
    auto b = cache.get("logo");                // 同一份, 不重新建立
    std::cout << "a == b ? " << (a.get() == b.get()) << "\n";
    std::cout << "use_count = " << a.use_count() << "\n";  // 3 (cache 自己也持有)
}

// =====================================================================
// 範例 (4): Custom Deleter - 進階但實用
// =====================================================================
// 觀念: shared_ptr 可以指定自訂釋放函式, 適合管理非 new 出來的資源
//       (例如 C API 傳回的 handle、檔案、連線池物件等)。
void example_custom_deleter() {
    std::cout << "--- 範例 4: Custom Deleter (FILE*) ---\n";
    // fopen 失敗會回傳 nullptr, 此處為示範用 stderr 表示成功 case
    // 實務上會 fopen("/path", "r")
    FILE* fp = stderr;                          // 不要 fclose stderr; 此處純示範
    std::shared_ptr<FILE> fh(fp, [](FILE* f) {
        // 在實際情境是 fclose(f);
        std::cout << "  [custom deleter called for FILE*]\n";
        (void)f;
    });
    std::cout << "fh.use_count() = " << fh.use_count() << "\n";
}

// =====================================================================
// Leetcode 138. Copy List with Random Pointer  (Medium, 但概念簡單實用)
// =====================================================================
// 題目:
//   每個節點除了 next 指標外, 還有一個 random 指標, 可指向串列中
//   任意節點 (包括自己) 或 nullptr。請深拷貝 (deep copy) 整條串列。
//
// 為什麼這題適合 shared_ptr?
//   - random 指標讓「同一個節點可被多個指標指到」, 完全是「共享」語意。
//   - 用 shared_ptr 後, 我們不需要手動管理任何節點的生命週期, 也
//     不會像裸指標解法一樣容易遺漏釋放。
//   - 解法只需要一個 hashmap: 原節點 -> 新節點, 兩次走訪即可完成。
//
// 解題思路:
//   Pass 1: 走過原串列, 為每個原節點建立對應的新節點 (尚未連線),
//           並把映射存到 unordered_map。
//   Pass 2: 再走一次, 利用 map 查到對應新節點, 連好 next 與 random。
//
// 時間複雜度 O(n), 空間複雜度 O(n)。
// ---------------------------------------------------------------------
struct Node {
    int val;
    std::shared_ptr<Node> next;
    std::shared_ptr<Node> random;
    explicit Node(int v) : val(v) {}
};

std::shared_ptr<Node> copy_random_list(const std::shared_ptr<Node>& head) {
    if (!head) return nullptr;

    // map: 原節點裸指標 -> 新節點 shared_ptr
    // 用 raw pointer 當 key 不會影響原節點生命週期 (只是觀察)
    std::unordered_map<const Node*, std::shared_ptr<Node>> mp;

    // Pass 1: 建立每個新節點
    for (auto p = head; p; p = p->next) {
        mp[p.get()] = std::make_shared<Node>(p->val);
    }

    // Pass 2: 連好 next 與 random
    for (auto p = head; p; p = p->next) {
        auto& copy = mp[p.get()];
        copy->next   = p->next   ? mp[p->next.get()]   : nullptr;
        copy->random = p->random ? mp[p->random.get()] : nullptr;
    }

    return mp[head.get()];
}

// 印出 (val, random->val) 對, 方便比對深拷貝結果
void print_with_random(const std::shared_ptr<Node>& head) {
    for (auto p = head; p; p = p->next) {
        std::cout << "(" << p->val << ", random=";
        if (p->random) std::cout << p->random->val; else std::cout << "null";
        std::cout << ")  ";
    }
    std::cout << "\n";
}

// =====================================================================
// 實用範例: 共享設定物件 (Config) 給多個 service
// =====================================================================
// 觀念: 應用啟動時 load 一份 Config, 多個 service 模組共享 read-only 指標。
//       任何一個 service 釋放 (例如關閉) 都不會影響其他 service;
//       全部關閉後 Config 自動釋放。
// =====================================================================
struct AppConfig {
    std::string host;
    int port;
    AppConfig(std::string h, int p) : host(std::move(h)), port(p) {
        std::cout << "  [load AppConfig " << host << ":" << port << "]\n";
    }
    ~AppConfig() { std::cout << "  [destroy AppConfig]\n"; }
};

class ApiService {
public:
    explicit ApiService(std::shared_ptr<AppConfig> cfg) : cfg_(std::move(cfg)) {}
    void start() { std::cout << "  ApiService starting on " << cfg_->host << ":" << cfg_->port << "\n"; }
private:
    std::shared_ptr<AppConfig> cfg_;
};

class LogService {
public:
    explicit LogService(std::shared_ptr<AppConfig> cfg) : cfg_(std::move(cfg)) {}
    void start() { std::cout << "  LogService for " << cfg_->host << "\n"; }
private:
    std::shared_ptr<AppConfig> cfg_;
};

void example_shared_config() {
    std::cout << "--- 實用範例: 多 service 共享 Config ---\n";
    auto cfg = std::make_shared<AppConfig>("api.example.com", 443);
    ApiService api(cfg);
    LogService log(cfg);
    api.start();
    log.start();
    std::cout << "  use_count=" << cfg.use_count() << "\n";  // 3
    // 函式結束時三者 (cfg / api.cfg_ / log.cfg_) 依序釋放, 最後 AppConfig destruct
}

void example_leetcode_138() {
    std::cout << "--- Leetcode 138: Copy List with Random Pointer ---\n";
    // 建立原串列: 1 -> 2 -> 3, random: 1->3, 2->1, 3->null
    auto n1 = std::make_shared<Node>(1);
    auto n2 = std::make_shared<Node>(2);
    auto n3 = std::make_shared<Node>(3);
    n1->next = n2; n2->next = n3;
    n1->random = n3;
    n2->random = n1;
    n3->random = nullptr;

    auto cloned = copy_random_list(n1);
    std::cout << "原串列  : "; print_with_random(n1);
    std::cout << "深拷貝  : "; print_with_random(cloned);

    // 驗證: 新節點與舊節點位址不同 (真的是 deep copy)
    std::cout << "n1.get() == cloned.get() ? "
              << (n1.get() == cloned.get()) << " (應為 0)\n";
}

// =====================================================================
// main
// =====================================================================
int main() {
    example_basic();
    example_shared_resource();
    example_cache();
    example_custom_deleter();
    example_shared_config();
    example_leetcode_138();

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：make_shared 跟 shared_ptr<T>(new T(...)) 真正的差別?
    //    A：make_shared 把 control block 與物件配置在同一塊記憶體,
    //       少一次 allocation, 較快, 也避免「new 成功但 shared_ptr ctor
    //       throw」的洩漏窗口。但缺點是「物件記憶體要等所有 weak_ptr
    //       釋放才能歸還」, 大物件 + 多 weak_ptr 的場景反而吃空間。
    //
    //  Q2：use_count() 在多執行緒下可信嗎?
    //    A：引用計數的更新是 atomic 沒錯, 但 use_count() 回傳的瞬間值
    //       下一行就可能變動, 所以它只能拿來除錯/測試, 不能用來做執行
    //       時的決策 (例如「if (uc==1) 就改寫」── 競態)。判斷單一持有
    //       者請用 unique() (C++20 移除, 改用 use_count()==1 也是同樣
    //       不可靠)。
    //
    //  Q3：shared_ptr 的兩個指標 (object ptr / control block ptr) 為什麼
    //       要分開?
    //    A：分開才能支援 aliasing constructor (一個 shared_ptr 共享別人
    //       的 control block 卻指向子物件) 與 cast 後仍共享計數
    //       (static_pointer_cast / dynamic_pointer_cast)。這也是 sizeof
    //       (shared_ptr) 通常 = 2 個指標 (16 bytes on 64-bit) 的原因。
    //
    return 0;
}
