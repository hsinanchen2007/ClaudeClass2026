// =============================================================================
//  第 14 課：預設建構函數 7  —  物件陣列為什麼一定要 default constructor
// =============================================================================
//
// 【主題資訊 Information】
//   規則      : T arr[N]; 會對每個元素做 default-initialization → 需要 default constructor
//   標準版本  : C++98 起；本檔用到的 std::array 與 NSDMI 是 C++11
//   標頭檔    : <iostream>、<string>、<array>、<vector>
//   相關      : std::vector<T> v(n) 同樣需要；v.reserve(n) 與 emplace_back 則不需要
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼陣列非要 default constructor 不可】
//   T arr[5]; 這行的語意是「一次生出 5 個完整的 T 物件」。
//   C++ 沒有「半建構的物件」這種狀態——每個元素從那一刻起就必須是合法物件。
//   但這個語法沒有地方讓你為第 0 個、第 1 個…分別指定引數，
//   所以編譯器只有一條路：對每個元素呼叫 default constructor。
//   類別若沒有 default constructor，這行就無法成立，直接編譯失敗。
//
// 【2. 建構順序與解構順序】
//   建構：索引 0 → 1 → 2 → …（由低到高）
//   解構：… → 2 → 1 → 0（由高到低，與建構相反）
//   這個順序是標準規定的，本檔以實際輸出驗證。
//   若某個元素的 constructor 中途拋出例外，**已建構完成的元素會被逆序解構**，
//   尚未建構的則不會——這保證了陣列建構的例外安全性。
//
// 【3. 沒有 default constructor 時的四條出路】
//   (a) 補一個 default constructor（本檔 Enemy 的作法）
//   (b) 在宣告時就用初始化列表逐一給引數：
//           NoDefault arr[3] = { NoDefault(1), NoDefault(2), NoDefault(3) };
//       （這是 8.cpp 的主題；缺點是大小必須在編譯期寫死）
//   (c) 改用 std::vector + reserve + emplace_back —— 最實用的作法，
//       完全不需要 default constructor，且大小可以在執行期決定
//   (d) 改用 std::optional<T> 陣列，表達「這一格還沒有東西」
//
// 【4. 「補一個 default constructor」不一定是好事】
//   很多人一遇到編譯錯誤就反射性地補一個 default constructor，
//   但如果這個類別**在語意上本來就不該有預設狀態**（例如
//   DatabaseConnection 沒有連線字串就不該存在），補它等於製造一個
//   「合法但無意義」的物件，把錯誤延後到執行期。
//   此時正確的作法是 (c) 或 (d)，而不是硬補一個假的預設值。
//   判準很簡單：**這個型別存在「零值 / 空白狀態」這個合理概念嗎？**
//   Enemy（小怪，HP 100）有；DatabaseConnection 沒有。
//
// 【概念補充 Concept Deep Dive】
//   ▍T arr[N]; 與 T arr[N]{}; 的差別
//     前者 default-initialization，後者 aggregate/value-initialization。
//     對含內建型別成員且無 NSDMI 的類別，後者會歸零、前者不會——
//     和 5.cpp 講的是同一套規則。
//
//   ▍原生陣列 vs std::array vs std::vector
//     原生陣列會退化成指標（傳進函式就失去長度資訊），沒有 size()、
//     不能整體賦值。std::array<T,N> 保留大小資訊、支援迭代器與 STL 演算法，
//     且與原生陣列同樣配置在 stack 上，沒有額外成本——現代 C++ 應優先用它。
//     大小需要在執行期決定時才用 std::vector。
//
//   ▍vector<T> v(n) 與 v.reserve(n) 的關鍵差別
//     v(n) 會真的值初始化 n 個元素 → 需要 default constructor；
//     v.reserve(n) 只配置原始記憶體、不建立任何元素 → 不需要。
//     這是「為什麼有時編得過、有時編不過」最常見的答案。
//
// 【注意事項 Pay Attention】
//   1. T arr[N]; 需要 default constructor；沒有就是編譯錯誤，不是警告。
//   2. 不要為了通過編譯而硬補一個語意上不成立的預設狀態。
//   3. 建構是索引由小到大，解構是由大到小——依賴順序的資源管理要注意。
//   4. 原生陣列傳進函式會退化成指標；優先考慮 std::array / std::vector。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】物件陣列與 default constructor
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 T arr[5]; 需要 default constructor，而 std::vector<T> v; 不用？
//     答：T arr[5]; 必須立刻產生 5 個完整物件，而語法上沒有地方為每個元素
//         指定引數，所以只能呼叫 default constructor。
//         std::vector<T> v; 建立的是空容器，一個元素都沒有，自然不需要。
//         但 std::vector<T> v(5); 會值初始化 5 個元素 → 又需要了。
//     追問：那 v.reserve(5) 呢？→ 只配置記憶體、不建立元素，不需要
//         default constructor；之後用 emplace_back 直接就地建構即可。
//
// 🔥 Q2. 類別沒有 default constructor 又想要一個容器裝它，有哪些作法？
//         答：(1) vector + reserve + emplace_back（最實用，大小可執行期決定）；
//         (2) 宣告時用初始化列表逐一給引數（大小須編譯期寫死）；
//         (3) 改存 std::optional<T> 或 std::unique_ptr<T>，
//             明確表達「這一格可能還沒有東西」。
//         最不該做的是「為了編譯通過而硬補一個沒有意義的預設狀態」。
//     追問：怎麼判斷該不該補？→ 問這個型別有沒有「零值／空白狀態」這個
//         合理概念。Enemy 有（小怪），DatabaseConnection 沒有（沒連線字串就不該存在）。
//
// ⚠️ 陷阱. Enemy enemies[5]; 編譯失敗，於是加了一個空的 Enemy() { } 就好了？
//     答：編譯確實會過，但每個元素的 type 是空字串、health 是不確定值，
//         你只是把「編譯期錯誤」換成了「執行期的不確定行為」。
//         要補就要補成有意義的預設狀態（本檔的 type="小怪", health=100），
//         或用 NSDMI 保證每個欄位都有值。
//     為什麼會錯：把編譯器的錯誤訊息當成「要我加個函式」的機械指令，
//         而不是「這裡有個設計問題」的提示。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <array>
#include <vector>
using namespace std;

class Enemy {
private:
    string type;
    int    health;

public:
    // 必須有預設建構函數，否則無法用 Enemy enemies[5]; 建立陣列。
    // 注意：補的是「有意義的預設狀態」，不是空的 { }。
    Enemy() {
        type = "小怪";
        health = 100;
    }

    Enemy(string t, int hp) {
        type = t;
        health = hp;
    }

    void print() const {
        cout << "  " << type << " (HP: " << health << ")" << endl;
    }

    void setType(string t) { type = t; }
    void setHealth(int hp) { health = hp; }
};

// 觀察建構／解構順序用
class Tracer {
private:
    int id_;
public:
    Tracer() : id_(-1) { cout << "    [建構] Tracer(預設)" << endl; }
    explicit Tracer(int i) : id_(i) { cout << "    [建構] Tracer(" << i << ")" << endl; }
    ~Tracer() { cout << "    [解構] Tracer(" << id_ << ")" << endl; }
};

// 語意上「不該有預設狀態」的型別：不要硬補 default constructor
class DbConnection {
private:
    string dsn_;
public:
    explicit DbConnection(const string& dsn) : dsn_(dsn) {}
    void print() const { cout << "  連線: " << dsn_ << endl; }
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 705. Design HashSet
//   題目：不使用內建雜湊表，設計 MyHashSet，支援 add / remove / contains。
//   為什麼用到本主題：這題最自然的解法是「固定數量的 bucket，每個 bucket
//         是一條鏈」。constructor 一開始就要生出 N 個空的 bucket——
//         這正是「陣列/容器需要元素能被預設建構」的實際應用：
//         std::vector<std::vector<int>> buckets_(N) 會值初始化 N 個空 vector，
//         靠的就是 std::vector 自己的 default constructor。
//   複雜度：平均 O(1)（元素分布均勻時），最壞 O(n / bucketCount)。
// -----------------------------------------------------------------------------
class MyHashSet {
private:
    static const int kBuckets = 769;          // 質數可減少碰撞
    vector<vector<int>> buckets_;

    int hash(int key) const { return key % kBuckets; }

public:
    // 這一行就是本課主題：一次生出 769 個 vector，
    // 每一個都由 std::vector 自己的 default constructor 建構成空容器
    MyHashSet() : buckets_(kBuckets) {}

    void add(int key) {
        auto& b = buckets_[hash(key)];
        for (int v : b) if (v == key) return;   // 已存在
        b.push_back(key);
    }

    void remove(int key) {
        auto& b = buckets_[hash(key)];
        for (size_t i = 0; i < b.size(); ++i) {
            if (b[i] == key) { b.erase(b.begin() + static_cast<long>(i)); return; }
        }
    }

    bool contains(int key) const {
        const auto& b = buckets_[hash(key)];
        for (int v : b) if (v == key) return true;
        return false;
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】遊戲的物件池（Object Pool）
//   情境：遊戲中每一幀可能生成／回收大量子彈。若每次都 new/delete，
//         會造成記憶體碎片與不可預測的延遲尖峰（對 60 FPS 是致命的）。
//   作法：啟動時一次配置固定數量的物件，之後只「借出／歸還」，不再配置。
//         這要求元素型別能被預設建構——池子一開始就要生出 N 個待命物件。
//   這正是「陣列需要 default constructor」在真實系統中最典型的用途。
// -----------------------------------------------------------------------------
class Bullet {
private:
    double x_ = 0.0, y_ = 0.0;
    bool   active_ = false;      // NSDMI：保證每顆子彈一開始是「未使用」

public:
    Bullet() = default;          // 池子需要它；NSDMI 已保證狀態合法

    void fire(double x, double y) { x_ = x; y_ = y; active_ = true; }
    void recycle()                { active_ = false; }
    bool active() const           { return active_; }

    void print() const {
        cout << "    子彈(" << x_ << ", " << y_ << ") "
             << (active_ ? "使用中" : "待命") << endl;
    }
};

class BulletPool {
private:
    std::array<Bullet, 8> pool_;   // 一次生出 8 顆，全部預設建構

public:
    Bullet* acquire(double x, double y) {
        for (auto& b : pool_) {
            if (!b.active()) { b.fire(x, y); return &b; }
        }
        return nullptr;            // 池子滿了，這一幀就不再生成
    }

    int activeCount() const {
        int n = 0;
        for (const auto& b : pool_) if (b.active()) ++n;
        return n;
    }

    void dump() const {
        for (const auto& b : pool_) b.print();
    }
};

int main() {
    cout << "=== 對象陣列（需要預設建構函數）===" << endl;
    Enemy enemies[5];              // 5 個元素，各呼叫一次 default constructor
    for (int i = 0; i < 5; i++) enemies[i].print();

    cout << "\n=== 修改其中幾個 ===" << endl;
    enemies[0].setType("Boss");
    enemies[0].setHealth(5000);
    enemies[2].setType("精英怪");
    enemies[2].setHealth(500);
    for (int i = 0; i < 5; i++) enemies[i].print();

    cout << "\n=== 建構順序 0→N，解構順序 N→0 ===" << endl;
    {
        Tracer t[3];               // 觀察建構順序
        cout << "    （即將離開 scope）" << endl;
    }                              // 觀察解構順序

    cout << "\n=== 沒有 default constructor 時，不要硬補 ===" << endl;
    cout << "  DbConnection arr[3];  → 編譯失敗（它本來就不該有預設狀態）" << endl;
    cout << "  正解是 vector + reserve + emplace_back：" << endl;
    vector<DbConnection> conns;
    conns.reserve(2);              // 只配置記憶體，不需要 default constructor
    conns.emplace_back("host=db1;port=5432");
    conns.emplace_back("host=db2;port=5432");
    for (const auto& c : conns) c.print();

    cout << "\n=== LeetCode 705. Design HashSet ===" << endl;
    MyHashSet hs;
    hs.add(1);
    hs.add(2);
    cout << "  contains(1) = " << (hs.contains(1) ? "true" : "false") << endl;
    cout << "  contains(3) = " << (hs.contains(3) ? "true" : "false") << endl;
    hs.add(2);                     // 重複加入不應有效果
    cout << "  add(2) 兩次後 contains(2) = " << (hs.contains(2) ? "true" : "false") << endl;
    hs.remove(2);
    cout << "  remove(2) 後 contains(2) = " << (hs.contains(2) ? "true" : "false") << endl;
    hs.add(770);                   // 770 % 769 == 1，與 key=1 同一個 bucket
    cout << "  加入 770（與 1 碰撞同一 bucket）後：" << endl;
    cout << "    contains(1)   = " << (hs.contains(1) ? "true" : "false") << endl;
    cout << "    contains(770) = " << (hs.contains(770) ? "true" : "false") << endl;

    cout << "\n=== 日常實務：子彈物件池 ===" << endl;
    BulletPool pool;
    cout << "  啟動時 8 顆子彈全部預設建構完成，使用中 = "
         << pool.activeCount() << endl;
    pool.acquire(10, 20);
    pool.acquire(30, 40);
    pool.acquire(50, 60);
    cout << "  發射 3 顆後，使用中 = " << pool.activeCount() << endl;
    pool.dump();
    cout << "  整個過程沒有任何一次 new／delete——" << endl;
    cout << "  這正是「陣列需要 default constructor」在真實系統中的價值。" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 14 課：預設建構函數（Default Constructor）7.cpp" -o def7

// === 預期輸出 ===
// === 對象陣列（需要預設建構函數）===
//   小怪 (HP: 100)
//   小怪 (HP: 100)
//   小怪 (HP: 100)
//   小怪 (HP: 100)
//   小怪 (HP: 100)
//
// === 修改其中幾個 ===
//   Boss (HP: 5000)
//   小怪 (HP: 100)
//   精英怪 (HP: 500)
//   小怪 (HP: 100)
//   小怪 (HP: 100)
//
// === 建構順序 0→N，解構順序 N→0 ===
//     [建構] Tracer(預設)
//     [建構] Tracer(預設)
//     [建構] Tracer(預設)
//     （即將離開 scope）
//     [解構] Tracer(-1)
//     [解構] Tracer(-1)
//     [解構] Tracer(-1)
//
// === 沒有 default constructor 時，不要硬補 ===
//   DbConnection arr[3];  → 編譯失敗（它本來就不該有預設狀態）
//   正解是 vector + reserve + emplace_back：
//   連線: host=db1;port=5432
//   連線: host=db2;port=5432
//
// === LeetCode 705. Design HashSet ===
//   contains(1) = true
//   contains(3) = false
//   add(2) 兩次後 contains(2) = true
//   remove(2) 後 contains(2) = false
//   加入 770（與 1 碰撞同一 bucket）後：
//     contains(1)   = true
//     contains(770) = true
//
// === 日常實務：子彈物件池 ===
//   啟動時 8 顆子彈全部預設建構完成，使用中 = 0
//   發射 3 顆後，使用中 = 3
//     子彈(10, 20) 使用中
//     子彈(30, 40) 使用中
//     子彈(50, 60) 使用中
//     子彈(0, 0) 待命
//     子彈(0, 0) 待命
//     子彈(0, 0) 待命
//     子彈(0, 0) 待命
//     子彈(0, 0) 待命
//   整個過程沒有任何一次 new／delete——
//   這正是「陣列需要 default constructor」在真實系統中的價值。
