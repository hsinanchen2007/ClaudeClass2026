// =============================================================================
//  第 2.8 章 範例 3  —  Rule of Zero：讓成員自己管理資源
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<string>、<vector>、<memory>（unique_ptr）、<utility>（move）
//   Rule of Zero 的內容：
//     不要自己寫任何一個特殊成員函式。改用已經正確實作 RAII 的成員型別
//     （std::string / std::vector / std::unique_ptr / std::shared_ptr），
//     讓編譯器逐成員生成解構、複製、移動——它生成的版本必定正確。
//   這個名詞由 R. Martinho Fernandes 於 2012 年提出，是 Rule of Five 的
//   現代結論：最好的資源管理程式碼，是你根本不用寫的那些。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼「不寫」會比「寫對」更好】
//   手寫 Rule of Five 的五個函式，每一個都有出錯空間：
//     * 複製賦值忘了處理自我賦值 → 先 delete 再讀取已釋放的來源
//     * 移動後忘了把來源指標設 nullptr → double free
//     * 忘了標 noexcept → vector 擴容靜默退化成複製
//     * 新增一個成員後忘了同步更新五個函式 → 該成員在複製時被遺漏
//   最後一項最危險：它是「維護期」才會出現的 bug，
//   而且新增成員的人通常不會想到要去改複製建構子。
//   Rule of Zero 讓編譯器逐成員生成——新增成員時，五個函式自動跟著更新。
//
// 【2. 所有權語意如何從成員「傳播」到整個類別】
//   隱式生成的複製建構子做的是逐成員複製。若任何一個成員不可複製，
//   整個類別的複製建構子就會被隱式 delete。
//   本檔的 User 有一個 std::unique_ptr<int[]> 成員：
//     * unique_ptr 的複製建構子是 = delete → User 的複製也被 delete
//     * unique_ptr 可以移動 → User 的移動正常生成
//   結果：User 自動變成「只能移動、不能複製」，而且這個限制寫在型別裡，
//   誤用時是編譯期錯誤，不是執行期崩潰。
//   把 unique_ptr 換成 shared_ptr，整個類別就自動變成可複製（共用所有權）。
//   **選擇成員的智慧指標型別 = 選擇整個類別的所有權語意。**
//
// 【3. 「不寫任何特殊函式」的例外：虛擬解構子】
//   若這個類別要被當成多型基底（有人會透過基底指標 delete 衍生物件），
//   就必須寫 virtual ~Base() = default;。
//   但注意：宣告了解構子（即使 = default）會抑制移動操作的自動生成！
//   此時要補回來：
//       virtual ~Base() = default;
//       Base(Base&&) = default;
//       Base& operator=(Base&&) = default;
//       Base(const Base&) = default;
//       Base& operator=(const Base&) = default;
//   這叫 Rule of Five 的 "= default" 版本。多型基底是 Rule of Zero 的
//   主要例外情境。
//
// 【4. 什麼時候才真的需要手寫 Rule of Five】
//   只有在「包裝一個沒有現成 RAII 型別可用的低階資源」時：
//     檔案描述符（int fd）、OS handle、C 函式庫的 context 指標、
//     需要自訂釋放函式且不適合用 unique_ptr 自訂 deleter 的資源。
//   即使如此，優先考慮 std::unique_ptr<T, CustomDeleter>——
//   它能吃下大部分「需要自訂釋放方式」的情況，讓你回到 Rule of Zero。
//
// 【概念補充 Concept Deep Dive】
//   (A) std::unique_ptr<int[]> 是陣列特化版本，它的解構子呼叫 delete[]
//       而非 delete。用錯（對陣列用 unique_ptr<int>）是未定義行為。
//       實務上多數情況該用 std::vector<int> 而不是 unique_ptr<int[]>——
//       vector 額外提供大小資訊、可調整大小、可複製。
//   (B) 隱式生成的移動操作是否為 noexcept，取決於所有成員的移動是否都 noexcept。
//       string、vector、unique_ptr 的移動都是 noexcept，
//       所以 User 的隱式移動建構子自動是 noexcept——這是 Rule of Zero
//       的另一個免費好處：連 noexcept 都不會忘記標。
//   (C) 本檔的 User 沒有任何 public 成員或建構子，所有成員都是 private，
//       所以它只能預設建構。這是為了把焦點放在「特殊成員函式的生成規則」上。
//
// 【注意事項 Pay Attention】
//   1. 多型基底類別需要 virtual 解構子，這會抑制移動生成 → 需用 = default 補回。
//   2. unique_ptr<T[]> 與 unique_ptr<T> 不可混用（delete[] vs delete）。
//   3. 成員含 unique_ptr 時整個類別不可複製；需要複製語意請改用 shared_ptr
//      或自己實作深層複製。
//   4. 被移動後的物件處於 valid but unspecified 狀態，可安全解構與重新賦值，
//      但不可假設其內容。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】Rule of Zero
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 什麼是 Rule of Zero？它和 Rule of Five 是什麼關係？
//     答：Rule of Zero 是「不要自己寫任何特殊成員函式」——改用已正確實作
//         RAII 的成員型別，讓編譯器逐成員生成。它是 Rule of Five 的現代結論：
//         Rule of Five 告訴你「有所有權就得寫五個」，
//         Rule of Zero 則說「那就別讓這個類別直接持有所有權」。
//     追問：那還有需要手寫 Rule of Five 的時候嗎？
//         → 有，包裝沒有現成 RAII 型別的低階資源（fd、OS handle）時。
//           但先考慮 unique_ptr 搭配自訂 deleter，多數情況能回到 Rule of Zero。
//
// 🔥 Q2. 為什麼一個 unique_ptr 成員就讓整個 User 不可複製？
//     答：隱式複製建構子是逐成員複製；unique_ptr 的複製建構子是 = delete，
//         導致 User 的複製建構子被隱式 delete。移動則因 unique_ptr 可移動
//         而正常生成。換成 shared_ptr，User 就自動變回可複製——
//         成員的所有權語意會自動傳播成整個類別的語意。
//
// ⚠️ 陷阱. 「我的基底類別只加了 virtual ~Base() = default; 而已，
//          = default 就是編譯器版本，所以什麼都沒改變」——錯在哪？
//     答：只要「宣告」了解構子——即使是 = default——
//         移動建構子與移動賦值就不再自動生成。
//         於是所有 std::move 都靜默退回複製，vector 擴容也退回複製。
//         正確做法是把五個都用 = default 明確寫出來。
//     為什麼會錯：大家把 = default 理解成「等於什麼都沒寫」，
//         但標準區分的是「有沒有被使用者宣告（user-declared）」，
//         而不是「函式本體是不是編譯器產生的」。
//         = default 仍然是一次使用者宣告，一樣會觸發抑制規則。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <utility>
#include <type_traits>

// Rule of Zero：讓成員自己管理資源
class User {
    std::string name_;                    // 自己知道怎麼複製和移動
    std::vector<int> scores_;             // 自己知道怎麼複製和移動
    std::unique_ptr<int[]> buffer_;       // 自己知道怎麼移動（不可複製）

    // 不需要寫任何一個！編譯器全部自動生成
    // 解構子：自動呼叫每個成員的解構子
    // 移動：自動逐成員移動
    // 複製：因為 unique_ptr 不可複製，所以整個類別不可複製
};

// 對照組：把 unique_ptr 換成 shared_ptr，整個類別就自動變成「可複製」
class SharedUser {
    std::string name_;
    std::vector<int> scores_;
    std::shared_ptr<int[]> buffer_;       // 共用所有權 → 可複製
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 705. Design HashSet
//   題目：不使用內建雜湊表，自行設計 HashSet，支援 add / remove / contains。
//   為什麼用到本主題：這題的標準解法是「固定桶數的陣列 + 每桶一條鏈」。
//     用 C 的思維會寫成 Node** buckets 手動 new/delete，
//     那就必須完整手寫 Rule of Five，還要自己處理 double free。
//     改用 std::vector<std::vector<int>> 之後——
//     複製、移動、解構全部由編譯器正確生成，一行特殊函式都不用寫。
//     這正是 Rule of Zero 在演算法題中的實際價值：
//     把心力放在演算法本身，而不是記憶體管理。
//   複雜度：平均 O(1)（負載平均時），最壞 O(n/桶數)；空間 O(桶數 + 元素數)。
// -----------------------------------------------------------------------------
class MyHashSet {
    // ★ Rule of Zero：唯一的成員是 vector，所有特殊函式自動正確生成
    static constexpr int kBuckets = 769;          // 質數可減少雜湊碰撞
    std::vector<std::vector<int>> table_;

    static int hash(int key) { return key % kBuckets; }

public:
    MyHashSet() : table_(kBuckets) {}
    // 沒有解構子、沒有複製建構子、沒有移動建構子——全部不需要寫

    void add(int key) {
        auto& bucket = table_[hash(key)];
        for (int v : bucket) if (v == key) return;   // 已存在
        bucket.push_back(key);
    }

    void remove(int key) {
        auto& bucket = table_[hash(key)];
        for (std::size_t i = 0; i < bucket.size(); ++i) {
            if (bucket[i] == key) {
                bucket[i] = bucket.back();           // 與最後一個交換再彈出，O(1)
                bucket.pop_back();
                return;
            }
        }
    }

    bool contains(int key) const {
        const auto& bucket = table_[hash(key)];
        for (int v : bucket) if (v == key) return true;
        return false;
    }

    std::size_t size() const {
        std::size_t n = 0;
        for (const auto& b : table_) n += b.size();
        return n;
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】不可複製的資料庫連線 vs 可複製的查詢結果
//   情境：真實系統中，「所有權語意」是設計決策，不是實作細節。
//     * 連線（Connection）代表一個獨佔的作業系統資源 → 不該被複製，
//       複製一個連線物件在語意上根本沒有意義。
//     * 查詢結果（ResultSet）是純資料 → 本來就該能自由複製傳遞。
//   用 Rule of Zero 表達這兩種語意，完全不用寫任何特殊函式：
//   只要選對成員型別，編譯器就會把正確的規則強制執行在型別系統裡。
// -----------------------------------------------------------------------------
struct ConnectionHandle {                 // 模擬一個獨佔的 OS 資源
    int id;
    explicit ConnectionHandle(int i) : id(i) {}
};

class DbConnection {                      // 不可複製、可移動
    std::string dsn_;
    std::unique_ptr<ConnectionHandle> handle_;   // ← 獨佔所有權
public:
    DbConnection(std::string dsn, int id)
        : dsn_(std::move(dsn))
        , handle_(std::make_unique<ConnectionHandle>(id)) {}
    // 五個特殊函式全部不用寫
    int id() const { return handle_ ? handle_->id : -1; }
    const std::string& dsn() const { return dsn_; }
};

class QueryResult {                       // 可複製（純資料）
    std::vector<std::string> rows_;
public:
    void addRow(std::string r) { rows_.push_back(std::move(r)); }
    std::size_t rowCount() const { return rows_.size(); }
};

int main() {
    std::cout << "=== 1. Rule of Zero：所有權語意自動傳播 ===\n";
    User a;
    // User b = a;           // 編譯錯誤：unique_ptr 不可複製
    User c = std::move(a);   // OK：逐成員移動
    (void)c;

    // 用 type traits 確定性地驗證編譯器生成了什麼（純編譯期資訊，輸出穩定）
    std::cout << "  User（含 unique_ptr 成員）:\n";
    std::cout << "    可複製建構? " << std::is_copy_constructible<User>::value << "\n";
    std::cout << "    可移動建構? " << std::is_move_constructible<User>::value << "\n";
    std::cout << "    移動是 noexcept? "
              << std::is_nothrow_move_constructible<User>::value
              << "（成員都 noexcept → 自動也是）\n";

    std::cout << "  SharedUser（改用 shared_ptr 成員）:\n";
    std::cout << "    可複製建構? " << std::is_copy_constructible<SharedUser>::value
              << "（換個成員型別，語意就變了）\n";

    std::cout << "\n=== 2. LeetCode 705. Design HashSet ===\n";
    {
        MyHashSet set;
        set.add(1);
        set.add(2);
        std::cout << "  contains(1) = " << set.contains(1) << "\n";   // 1 (true)
        std::cout << "  contains(3) = " << set.contains(3) << "\n";   // 0 (false)
        set.add(2);                       // 重複加入不應增加元素
        std::cout << "  重複 add(2) 後 size = " << set.size() << "\n";
        set.remove(2);
        std::cout << "  remove(2) 後 contains(2) = " << set.contains(2) << "\n";

        // ★ Rule of Zero 的好處：複製這個 HashSet 完全不用寫任何程式碼
        MyHashSet copy = set;
        copy.add(100);
        std::cout << "  複製一份並加入 100:\n";
        std::cout << "    原 set 的 size = " << set.size()
                  << "，contains(100) = " << set.contains(100) << "\n";
        std::cout << "    複製品 size = " << copy.size()
                  << "，contains(100) = " << copy.contains(100) << "\n";
        std::cout << "  → 深層複製自動完成，沒有共用、沒有 double free\n";
    }

    std::cout << "\n=== 3. 日常實務：用成員型別表達所有權語意 ===\n";
    {
        DbConnection conn("postgres://db:5432/orders", 42);
        std::cout << "  DbConnection（unique_ptr 成員）:\n";
        std::cout << "    可複製? " << std::is_copy_constructible<DbConnection>::value
                  << "（連線不該被複製，編譯期就擋住）\n";
        std::cout << "    可移動? " << std::is_move_constructible<DbConnection>::value
                  << "（可以轉移給別人管理）\n";

        DbConnection moved = std::move(conn);
        std::cout << "    移動後 moved.id = " << moved.id()
                  << "，來源 conn.id = " << conn.id() << "（-1 表示已交出）\n";

        QueryResult r;
        r.addRow("1001,alice,paid");
        r.addRow("1002,bob,pending");
        QueryResult r2 = r;              // 純資料，複製完全合理
        std::cout << "  QueryResult（vector 成員）:\n";
        std::cout << "    可複製? " << std::is_copy_constructible<QueryResult>::value
                  << "（純資料本來就該能複製）\n";
        std::cout << "    複製後兩者 rowCount = " << r.rowCount()
                  << " / " << r2.rowCount() << "\n";
    }

    std::cout << "\n=== 重點 ===\n";
    std::cout << "  選對成員型別 = 選對整個類別的所有權語意\n";
    std::cout << "  unique_ptr 成員 → 只能移動；shared_ptr / vector → 可複製\n";
    std::cout << "  五個特殊函式一行都不用寫，而且新增成員時自動保持正確\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 2.8 章：Rule of Five — 現代資源管理規則3.cpp" -o rule_of_zero

// 註：is_copy_constructible 等 type traits 印出的 1/0 是編譯期常數，
//     每次執行完全相同。
//
// 註：DbConnection 被 std::move 後，其 unique_ptr 成員依標準保證變為 nullptr
//     （unique_ptr 的移動建構子明確規定來源會被設為 nullptr），
//     因此 id() 回傳 -1 是可靠的行為，而非未指定值。
//     這與 std::string 的「valid but unspecified」不同——
//     unique_ptr 是少數對移動後狀態有明確保證的標準型別。

// === 預期輸出 ===
// === 1. Rule of Zero：所有權語意自動傳播 ===
//   User（含 unique_ptr 成員）:
//     可複製建構? 0
//     可移動建構? 1
//     移動是 noexcept? 1（成員都 noexcept → 自動也是）
//   SharedUser（改用 shared_ptr 成員）:
//     可複製建構? 1（換個成員型別，語意就變了）
//
// === 2. LeetCode 705. Design HashSet ===
//   contains(1) = 1
//   contains(3) = 0
//   重複 add(2) 後 size = 2
//   remove(2) 後 contains(2) = 0
//   複製一份並加入 100:
//     原 set 的 size = 1，contains(100) = 0
//     複製品 size = 2，contains(100) = 1
//   → 深層複製自動完成，沒有共用、沒有 double free
//
// === 3. 日常實務：用成員型別表達所有權語意 ===
//   DbConnection（unique_ptr 成員）:
//     可複製? 0（連線不該被複製，編譯期就擋住）
//     可移動? 1（可以轉移給別人管理）
//     移動後 moved.id = 42，來源 conn.id = -1（-1 表示已交出）
//   QueryResult（vector 成員）:
//     可複製? 1（純資料本來就該能複製）
//     複製後兩者 rowCount = 2 / 2
//
// === 重點 ===
//   選對成員型別 = 選對整個類別的所有權語意
//   unique_ptr 成員 → 只能移動；shared_ptr / vector → 可複製
//   五個特殊函式一行都不用寫，而且新增成員時自動保持正確
