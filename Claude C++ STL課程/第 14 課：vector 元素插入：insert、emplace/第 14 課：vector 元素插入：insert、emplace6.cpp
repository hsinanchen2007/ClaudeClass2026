// =============================================================================
//  第 14 課：vector 元素插入：insert、emplace6.cpp
//    —  insert vs emplace：就地建構 (in-place construction) 到底省下什麼
// =============================================================================
//
// 【主題資訊 Information】
//   template <class... Args>
//   iterator emplace(const_iterator pos, Args&&... args);
//   標頭檔: <vector>
//   標準版本: C++11
//   回傳: 指向新建構元素的 iterator
//   複雜度: O(distance(pos, end())) + 可能的 reallocation(與 insert 相同)
//
// 【詳細解釋 Explanation】
//
// 【1. 兩者的根本差異:誰負責建構那個物件】
//   insert(pos, Person("Bob", 25)):
//       1) **呼叫端**先建構一個暫時 Person("Bob", 25)
//       2) vector 把這個暫時物件 move(或 copy)進容器
//       3) 暫時物件解構
//     → 至少 1 次建構 + 1 次 move + 1 次解構
//   emplace(pos, "David", 28):
//       1) vector 把 "David" 與 28 **完美轉發 (perfect forwarding)** 到
//          容器內部那塊生記憶體上,直接 placement new 建構
//     → 只有 1 次建構,沒有暫時物件、沒有 move、沒有多餘解構
//
// 【2. 「省一次 move」通常沒你想的值錢】
// 對 std::string、std::vector 這類 move 成本 O(1) 的型別,省下的只是幾個
// 指標賦值。真正值得用 emplace 的情境是:
//   * 元素**不可複製也不可移動**(例如內含 std::mutex、std::atomic)——
//     這種型別根本沒辦法 insert,只能 emplace。
//   * 元素的 move 很貴(內含固定大小陣列的 POD、或沒寫 move ctor 只能 copy)。
//   * 建構參數很多,寫 emplace 比寫 T{...} 乾淨。
//
// 【3. 三個容易忽略但常考的語意差異】
//   (a) emplace 用**直接初始化 (direct-initialization)** → 可以呼叫 explicit 建構子;
//       insert 需要一個現成的 T,若建構子是 explicit 就不能隱式轉換。
//           struct Celsius { explicit Celsius(double); };
//           v.emplace(v.begin(), 36.6);   // OK：直接初始化
//           v.insert(v.begin(), 36.6);    // 編譯錯誤：無法隱式轉成 Celsius
//   (b) emplace **不做 narrowing 檢查**。它是函式呼叫式的轉換,
//       double → int 會靜默截斷;而 T{...} 的大括號初始化會被編譯器擋下。
//           v.emplace(v.begin(), 3.9, 2);      // Point(int,int)：3.9 靜默變成 3
//           v.insert(v.begin(), Point{3.9, 2}); // -Wnarrowing 警告（-pedantic-errors 下為錯誤）
//   (c) emplace 若在建構過程中丟出例外,vector 仍保持有效狀態,
//       但「元素是否已被搬移」取決於實作路徑,不保證強例外保證。
//
// 【概念補充 Concept Deep Dive】
// emplace 的實作核心是完美轉發 + placement new:
//     template <class... Args>
//     void construct(T* p, Args&&... args) {
//         ::new (static_cast<void*>(p)) T(std::forward<Args>(args)...);
//     }
// `Args&&...` 是 forwarding reference(轉發參考),搭配 std::forward 才能
// 保留每個參數原本的 value category:傳 lvalue 就以 lvalue 傳到建構子、
// 傳 rvalue 就以 rvalue 傳到建構子。少了 std::forward,所有參數在函式內部
// 都會變成有名字的 lvalue,rvalue 資訊就丟失了,又會退回 copy。
//
// 注意 `T(...)` 用的是小括號 → 直接初始化,這正是 (a) 與 (b) 兩個差異的來源。
//
// 【注意事項 Pay Attention】
// 1. **在中間位置 emplace 並不會比 insert 省下搬移**。兩者都要把後段
//    往後搬一格,emplace 省的只是「暫時物件」那一次建構/move/解構。
// 2. emplace 到中間位置時,libstdc++ 的實作路徑不一定是「直接在洞裡建構」:
//    若容量足夠,它可能先在尾端 move-construct、再逐一 move-assign 搬移,
//    最後對插入點做**賦值**。所以你會看到 move 賦值的訊息,那是搬移成本,
//    不是「emplace 沒生效」。
// 3. 別為了 emplace 而 emplace。可讀性優先,效能有疑慮再實測。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】insert vs emplace
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. insert 與 emplace 差在哪?emplace 一定比較快嗎?
//     答：insert 要求呼叫端先有一個 T 物件,再複製/搬移進容器;
//         emplace 把建構參數完美轉發到容器內部的記憶體上直接建構,
//         省掉暫時物件的建構、move 與解構。
//         但不一定比較快 —— 對 move 成本 O(1) 的型別(string、vector)
//         省下的極少;而且在中間位置插入時,兩者的元素搬移成本完全相同。
//     追問：那什麼時候非用 emplace 不可?→ 元素既不可複製也不可移動時
//         (例如內含 std::mutex),insert 根本無法編譯,只能 emplace。
//
// 🔥 Q2. emplace 使用哪種初始化?這造成什麼可觀察的差異?
//     答：直接初始化 (direct-initialization),內部是 ::new (p) T(args...)。
//         因此它可以呼叫 explicit 建構子,而 insert 需要隱式轉換所以不行。
//     追問：這跟 emplace_back 有差別嗎?→ 沒有,兩者的建構語意完全一致,
//         差別只在插入位置(emplace 指定 pos、emplace_back 固定在尾端)。
//
// ⚠️ 陷阱. vector<Point> v;（Point 有 Point(int,int)）
//     v.emplace(v.begin(), 3.9, 2); 會不會編譯錯誤?結果是什麼?
//     答：不會錯,而且沒有任何警告。emplace 是函式呼叫式的直接初始化,
//         double → int 是合法的隱式轉換,3.9 被靜默截斷成 3,存進去的是 (3,2)。
//         相對地 v.insert(v.begin(), Point{3.9, 2}) 用大括號初始化,
//         會觸發 -Wnarrowing(加 -pedantic-errors 則直接是編譯錯誤)。
//     為什麼會錯：把「emplace 更現代所以更安全」當成直覺。事實相反 ——
//         emplace 繞過了大括號初始化的 narrowing 檢查,這是它的已知代價。
//         在意這點就寫 v.emplace(v.begin(), Point{3.9, 2}) 讓大括號檢查回來。
// ═══════════════════════════════════════════════════════════════════════════

#include <vector>
#include <iostream>
#include <string>

struct Person {
    std::string name;
    int age;

    Person(const std::string& n, int a) : name(n), age(a) {
        std::cout << "建構 " << name << std::endl;
    }

    Person(const Person& other) : name(other.name), age(other.age) {
        std::cout << "複製 " << name << std::endl;
    }

    Person(Person&& other) noexcept
        : name(std::move(other.name)), age(other.age) {
        std::cout << "移動 " << name << std::endl;
    }

    Person& operator=(const Person& other) {
        name = other.name;
        age = other.age;
        std::cout << "複製賦值 " << name << std::endl;
        return *this;
    }

    Person& operator=(Person&& other) noexcept {
        name = std::move(other.name);
        age = other.age;
        std::cout << "移動賦值 " << name << std::endl;
        return *this;
    }
};

// ── 示範 emplace 的兩個語意差異：explicit 建構子與 narrowing ──
struct Celsius {
    double v;
    explicit Celsius(double d) : v(d) {}   // explicit：禁止隱式轉換
};

struct Point {
    int x, y;
    Point(int a, int b) : x(a), y(b) {}
};

int main() {
    std::vector<Person> people;
    people.reserve(5);   // 先配足容量，排除 reallocation 的干擾

    people.emplace_back("Alice", 30);
    people.emplace_back("Charlie", 35);

    std::cout << "\n=== 使用 insert ===" << std::endl;
    // 呼叫端先建構暫時物件 → vector 再把它搬進來 → 暫時物件解構
    people.insert(people.begin() + 1, Person("Bob", 25));

    std::cout << "\n=== 使用 emplace ===" << std::endl;
    // 參數直接完美轉發到容器內部建構，沒有暫時物件
    people.emplace(people.begin() + 1, "David", 28);

    std::cout << "\n=== 最終結果 ===" << std::endl;
    for (const auto& p : people) {
        std::cout << p.name << " (" << p.age << ")" << std::endl;
    }

    // ── 差異 (a)：emplace 是直接初始化，可以呼叫 explicit 建構子 ──
    std::cout << "\n=== explicit 建構子 ===" << std::endl;
    std::vector<Celsius> temps;
    temps.emplace(temps.begin(), 36.6);      // OK
    // temps.insert(temps.begin(), 36.6);    // 編譯錯誤：無法隱式轉成 Celsius
    std::cout << "emplace 呼叫 explicit 建構子: " << temps[0].v << std::endl;

    // ── 差異 (b)：emplace 不做 narrowing 檢查 ──
    std::cout << "\n=== narrowing ===" << std::endl;
    std::vector<Point> pts;
    double d = 3.9;
    pts.emplace(pts.begin(), d, 2);          // 3.9 靜默截斷成 3，無警告
    std::cout << "emplace(3.9, 2) 存進去的是: ("
              << pts[0].x << "," << pts[0].y << ")" << std::endl;
    // pts.insert(pts.begin(), Point{d, 2}); // 會觸發 -Wnarrowing 警告

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 14 課：vector 元素插入：insert、emplace6.cpp" -o insert6

// === 預期輸出 ===
// 建構 Alice
// 建構 Charlie
//
// === 使用 insert ===
// 建構 Bob
// 移動 Charlie
// 移動賦值 Bob
//
// === 使用 emplace ===
// 建構 David
// 移動 Charlie
// 移動賦值 Bob
// 移動賦值 David
//
// === 最終結果 ===
// Alice (30)
// David (28)
// Bob (25)
// Charlie (35)
//
// === explicit 建構子 ===
// emplace 呼叫 explicit 建構子: 36.6
//
// === narrowing ===
// emplace(3.9, 2) 存進去的是: (3,2)
