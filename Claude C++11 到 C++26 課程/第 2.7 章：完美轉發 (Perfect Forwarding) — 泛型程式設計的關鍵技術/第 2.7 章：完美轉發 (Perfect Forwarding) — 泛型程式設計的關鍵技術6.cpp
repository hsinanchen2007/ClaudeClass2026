// =============================================================================
//  第 2.7 章 範例 6  —  成員函式轉發器：連「物件本身」都要轉發
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<utility>（forward）、<functional>（std::invoke，C++17）
//   本檔樣式：
//     template<class Obj, class MemFunc, class... Args>
//     auto invoke_member(Obj&& obj, MemFunc func, Args&&... args)
//         -> decltype((std::forward<Obj>(obj).*func)(std::forward<Args>(args)...));
//   .* 是「成員指標存取運算子」；obj.*func 取出成員函式後才加 (...) 呼叫，
//   所以整段必須用括號包起來：(obj.*func)(args...)。
//   C++17 起標準提供 std::invoke，可統一處理成員函式指標／成員資料／一般可呼叫物件。
//
// 【詳細解釋 Explanation】
//
// 【1. 這裡有「兩組」東西要轉發，不是一組】
//   一般的轉發只煩惱引數；成員函式呼叫多了一個主角——物件本身。
//     * Args&&... args  → 引數的值類別要保留（decltype 的老問題）
//     * Obj&& obj       → 物件的值類別也要保留
//   為什麼物件也重要？因為成員函式可以用 & 或 && 修飾自己：
//       struct S {
//           std::string take() &  { return data_; }              // 左值物件：複製
//           std::string take() && { return std::move(data_); }   // 右值物件：搬走
//       };
//   若轉發器把 obj 一律當左值，右值物件版本永遠選不到，
//   使用者寫 invoke_member(S{...}, &S::take) 也拿不到搬移優化。
//
// 【2. 為什麼 MemFunc 是傳值而不是 MemFunc&&】
//   成員函式指標是一個小小的純量值（本機上通常 8 或 16 bytes），
//   複製成本可以忽略，而且它沒有「左值／右值語意」可言——
//   轉發它得不到任何好處。傳值反而讓簽名更單純。
//   這是一個很好的判準示範：不是所有參數都該寫成轉發參考。
//
// 【3. 逐行對照本檔的三個呼叫】
//   invoke_member(db, &Database::query, sql)
//       sql 是左值 → query(const string&) → 內部複製，sql 保持完好
//   invoke_member(db, &Database::insert, std::string("new record"))
//       臨時字串是右值 → insert(string&&) → 可以搬走，零複製
//   invoke_member(db, &Database::update, key, std::string("updated value"))
//       混合：key 是左值走 const string&，value 是右值走 string&&
//       ——同一次呼叫中兩個參數各自保持自己的值類別，這正是參數包轉發的重點。
//
// 【4. 現代寫法：std::invoke（C++17）】
//   本檔的 (obj.*func)(args...) 語法只處理得了「成員函式指標」這一種情況。
//   std::invoke 統一了全部五種可呼叫形式（一般函式、函式物件、成員函式指標、
//   成員資料指標、以及透過 reference_wrapper／指標存取的物件），寫法一致：
//       std::invoke(func, std::forward<Obj>(obj), std::forward<Args>(args)...);
//   實務上新程式碼應優先使用 std::invoke；本檔保留手寫 .* 是為了讓你看見
//   std::invoke 內部到底在做什麼。
//
// 【概念補充 Concept Deep Dive】
//   (A) 成員函式指標不是一般指標。它可能需要儲存虛擬函式表的偏移量，
//       所以 sizeof 通常大於一般函式指標，而且不能轉型成 void*。
//   (B) 運算子優先權陷阱：obj.*func(args) 會被解析成 obj.*(func(args))，
//       完全不是你要的意思。括號 (obj.*func)(args) 是必要的，不是風格問題。
//   (C) 若 obj 是指標，要用 ->* 而不是 .*：(ptr->*func)(args)。
//       std::invoke 兩種都自動處理，這也是它更好用的原因之一。
//
// 【注意事項 Pay Attention】
//   1. (obj.*func)(args) 的括號不可省略，這是運算子優先權的硬性要求。
//   2. 物件本身也要轉發，否則 &/&& 修飾的成員函式多載會選錯。
//   3. 成員函式指標傳值即可，不需要（也無益於）轉發。
//   4. C++17 起優先用 std::invoke；它還能處理成員資料指標與指標形式的物件。
//   5. 兩個陷阱會疊加：一旦成員函式有 & / && 多載，&Buffer::release 就是
//      「多載集合」，MemFunc 同樣推導不出來（本機實測：couldn't deduce
//      template parameter 'MemFunc'）。此時必須用 static_cast 指定，
//      而型別末尾的 & 與 && 也是成員函式指標型別的一部分：
//          std::string (Buffer::*)() &     // 左值物件版本
//          std::string (Buffer::*)() &&    // 右值物件版本
//      本檔 main() 中的 Buffer 示範即是如此處理。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】成員函式的完美轉發
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 成員函式轉發器為什麼連「物件」也要用 std::forward？
//     答：因為成員函式可以用 & / && 修飾自己（ref-qualifier）。
//         右值物件上的 && 版本通常會把成員資源搬走，是重要的優化。
//         若轉發器把 obj 一律當左值傳遞，&& 版本永遠選不到，
//         呼叫端寫 invoke_member(S{}, &S::take) 也拿不到搬移。
//     追問：那成員函式指標 MemFunc 要不要轉發？
//         → 不用。它是個小純量值、沒有值類別語意，傳值最單純。
//
// 🔥 Q2. 為什麼 (obj.*func)(args...) 的括號不能省略？
//     答：運算子優先權。函式呼叫 () 的優先權高於 .*，
//         寫成 obj.*func(args) 會被解析成 obj.*(func(args))——
//         先呼叫 func 再對結果做成員存取，語意完全不同，通常直接編譯錯誤。
//
// ⚠️ 陷阱. 「成員函式指標就是個指標，應該可以轉成 void* 存起來」——錯在哪？
//     答：成員函式指標不是普通位址。對虛擬函式而言，它可能需要記錄
//         虛擬表偏移量與 this 調整值，因此 sizeof 通常大於一般函式指標
//         （常見實作為 16 bytes），而且標準明確禁止與 void* 互轉。
//     為什麼會錯：C 的心智模型是「函式名就是位址、指標都能塞進 void*」。
//         但 C++ 的成員函式呼叫需要 this 與可能的動態分派資訊，
//         這些無法壓縮成單一位址；它比較像「一個描述如何呼叫的小結構」。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <utility>
#include <functional>

class Database {
public:
    std::string query(const std::string& sql) {
        std::cout << "  DB::query(const string&)\n";
        return "result of: " + sql;
    }

    void insert(std::string&& data) {
        std::cout << "  DB::insert(string&&): " << data << "\n";
    }

    void update(const std::string& key, std::string&& value) {
        std::cout << "  DB::update(\"" << key << "\", \"" << value << "\")\n";
    }
};

// 泛型的成員函式呼叫轉發器
template<typename Obj, typename MemFunc, typename... Args>
auto invoke_member(Obj&& obj, MemFunc func, Args&&... args)
    -> decltype((std::forward<Obj>(obj).*func)(std::forward<Args>(args)...))
{
    return (std::forward<Obj>(obj).*func)(std::forward<Args>(args)...);
}

// -----------------------------------------------------------------------------
// 示範：物件本身的值類別為什麼重要（ref-qualifier 多載）
//   Buffer::release() 有 & 與 && 兩個版本：
//     左值物件 → 必須複製（呼叫端還要繼續用這個 buffer）
//     右值物件 → 可以直接搬走（呼叫端不再需要它了）
//   只有「連物件都轉發」的轉發器，才選得到右值版本。
// -----------------------------------------------------------------------------
class Buffer {
    std::string data_;
public:
    explicit Buffer(std::string d) : data_(std::move(d)) {}

    std::string release() &  {                       // 左值物件版本
        std::cout << "    Buffer::release() &  → 複製一份給你\n";
        return data_;
    }
    std::string release() && {                       // 右值物件版本
        std::cout << "    Buffer::release() && → 直接把內容搬給你\n";
        return std::move(data_);
    }
    std::size_t size() const { return data_.size(); }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】以成員函式轉發器實作「統一的重試 + 記錄」DAO 呼叫層
//   情境：資料存取層有一堆成員函式（query / insert / update / delete）。
//     若要為每一支都加上「記錄呼叫、統計耗次」的橫切邏輯，
//     逐一改每個函式會造成大量重複程式碼。
//     用成員函式轉發器可以寫一次就套用到全部——這正是 AOP（切面）
//     在 C++ 裡最樸素的實作方式。
// -----------------------------------------------------------------------------
int g_dbCallCount = 0;

template<typename Obj, typename MemFunc, typename... Args>
auto tracedCall(const char* opName, Obj&& obj, MemFunc func, Args&&... args)
    -> decltype((std::forward<Obj>(obj).*func)(std::forward<Args>(args)...))
{
    ++g_dbCallCount;
    std::cout << "  [trace] 第 " << g_dbCallCount << " 次呼叫: " << opName << "\n";
    // 只呼叫一次 → 可以安全轉發
    return (std::forward<Obj>(obj).*func)(std::forward<Args>(args)...);
}

int main() {
    Database db;
    std::string sql = "SELECT * FROM users";

    auto result = invoke_member(db, &Database::query, sql);
    std::cout << "  " << result << "\n\n";

    invoke_member(db, &Database::insert, std::string("new record"));

    std::cout << "\n";
    std::string key = "user_1";
    invoke_member(db, &Database::update, key, std::string("updated value"));

    std::cout << "\n=== 物件的值類別也會影響多載選擇 ===\n";
    {
        // ⚠️ 這裡同時踩到範例 10 的陷阱：release 有 & 與 && 兩個多載，
        //    直接寫 &Buffer::release 是「多載集合」，MemFunc 推導不出來
        //    （本機實測錯誤訊息：couldn't deduce template parameter 'MemFunc'）。
        //    必須用 static_cast 指定要哪一個——注意型別末尾的 & 與 &&
        //    就是 ref-qualifier，它是成員函式指標型別的一部分。
        using LvalueRelease = std::string (Buffer::*)() &;
        using RvalueRelease = std::string (Buffer::*)() &&;

        Buffer buf("payload-content");
        std::cout << "  傳左值物件:\n";
        auto a = invoke_member(buf, static_cast<LvalueRelease>(&Buffer::release));
        std::cout << "    取得 " << a.size() << " bytes，buf 仍有 "
                  << buf.size() << " bytes\n";

        std::cout << "  傳右值物件:\n";
        auto b = invoke_member(std::move(buf),
                               static_cast<RvalueRelease>(&Buffer::release));
        std::cout << "    取得 " << b.size() << " bytes（走的是 && 版本）\n";
    }

    std::cout << "\n=== 日常實務：統一加上追蹤的 DAO 呼叫層 ===\n";
    {
        Database dao;
        std::string q = "SELECT id FROM orders WHERE status = 'new'";
        auto r1 = tracedCall("query", dao, &Database::query, q);
        std::cout << "  " << r1 << "\n";

        tracedCall("insert", dao, &Database::insert, std::string("order-9001"));

        std::string k = "order-9001";
        tracedCall("update", dao, &Database::update, k, std::string("shipped"));

        std::cout << "  本次共呼叫資料庫 " << g_dbCallCount << " 次\n";
        std::cout << "  查詢字串未被破壞: " << q << "\n";
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 2.7 章：完美轉發 (Perfect Forwarding) — 泛型程式設計的關鍵技術6.cpp" -o member_fwd

// 註：本檔未附 LeetCode 範例。成員函式指標與 .* 運算子屬於泛型程式庫的
//     實作技術，LeetCode 題目的解法不會用到；硬套一題只會失真。
//
// 註：Buffer 在被 std::move 後透過 && 版本 release()，其內部字串處於
//     valid but unspecified 狀態。下方輸出中 buf 於 move 後不再被讀取，
//     符合標準對移動來源物件的使用規範。

// === 預期輸出 ===
//   DB::query(const string&)
//   result of: SELECT * FROM users
//
//   DB::insert(string&&): new record
//
//   DB::update("user_1", "updated value")
//
// === 物件的值類別也會影響多載選擇 ===
//   傳左值物件:
//     Buffer::release() &  → 複製一份給你
//     取得 15 bytes，buf 仍有 15 bytes
//   傳右值物件:
//     Buffer::release() && → 直接把內容搬給你
//     取得 15 bytes（走的是 && 版本）
//
// === 日常實務：統一加上追蹤的 DAO 呼叫層 ===
//   [trace] 第 1 次呼叫: query
//   DB::query(const string&)
//   result of: SELECT id FROM orders WHERE status = 'new'
//   [trace] 第 2 次呼叫: insert
//   DB::insert(string&&): order-9001
//   [trace] 第 3 次呼叫: update
//   DB::update("order-9001", "shipped")
//   本次共呼叫資料庫 3 次
//   查詢字串未被破壞: SELECT id FROM orders WHERE status = 'new'
