// =============================================================================
//  第六課 8 — std::map：有序關聯容器,以及 operator[] 這個經典陷阱
// =============================================================================
//
// 【主題資訊 Information】
//   template<class Key, class T,
//            class Compare = std::less<Key>,
//            class Allocator = std::allocator<std::pair<const Key, T>>>
//   class map;                                        // <map>,C++98 起
//
//   核心型別:
//     key_type    = Key
//     mapped_type = T
//     value_type  = std::pair<const Key, T>           // 注意 Key 上的 const
//
//   常用介面與複雜度(n = 元素個數):
//     T&        operator[](const Key& k);             // O(log n),找不到就「插入」
//     T&        at(const Key& k);                     // O(log n),找不到丟 out_of_range
//     iterator  find(const Key& k);                   // O(log n),找不到回 end()
//     size_type count(const Key& k) const;            // O(log n),map 只會是 0 或 1
//     pair<iterator,bool> insert(value_type);         // O(log n),已存在則不覆蓋
//     pair<iterator,bool> try_emplace(k, args...);    // O(log n),C++17
//     pair<iterator,bool> insert_or_assign(k, v);     // O(log n),C++17
//     iterator  lower_bound(const Key& k);            // O(log n),第一個 >= k
//     iterator  upper_bound(const Key& k);            // O(log n),第一個 >  k
//     bool      contains(const Key& k) const;         // O(log n),C++20 才有
//
//   標頭檔：#include <map>
//   本機實測(GCC 15.2.0 / libstdc++ / x86-64)：sizeof(std::map<int,int>) == 48,
//   屬**實作定義**(那是紅黑樹的 header node + size 欄位,不隨元素數量改變)。
//
// 【詳細解釋 Explanation】
//
// 【1. map 解決什麼問題:從「位置」定址改成從「意義」定址】
// vector/array 用整數 index 定址,前提是資料天生就有序號。但真實世界的資料多半
// 不是這樣:使用者是用帳號查的、設定值是用鍵名查的、股票是用代號查的。這些鍵
// 稀疏、不連續、甚至根本不是整數,你不可能開一個「以帳號當 index」的陣列。
//
// map 提供的是**鍵值對映**:給我 Key,還你 T&。而且它額外保證一件 unordered_map
// 給不了的事 —— 走訪時**永遠按 Key 由小到大排序**。這個「順序」不是附贈的裝飾,
// 它是 map 存在的主要理由之一(見第 4 點的 lower_bound)。
//
// 【2. 為什麼是 O(log n) 而不是 O(1):平衡二元搜尋樹】
// 標準只規定 map 的查找/插入/刪除是 O(log n),並且走訪為有序 —— 它**沒有規定
// 必須用紅黑樹**。但實務上主流實作(libstdc++、libc++、MSVC)都用紅黑樹,因為
// 紅黑樹正好能同時滿足「最壞情況 O(log n)」與「插入刪除時旋轉次數有上界」。
//
//   紅黑樹是**實作定義**;標準保證的只有複雜度與排序這兩件事。
//
// 關鍵在於這個 O(log n) 是**最壞情況保證**,不是平均。相對地 unordered_map 平均
// O(1) 但最壞是 O(n)。當你面對的是不可信的外部輸入(見【概念補充 C】的
// hash-flooding),這個「最壞情況保證」本身就是一種安全屬性。
//
// 【3. operator[] 的陷阱:一個看起來像「讀」的「寫」】
// 這是 map 最有名、也最常在 code review 被抓到的問題:
//
//     std::map<std::string,int> ages;
//     if (ages["Eve"] == 0) { ... }        // 你以為只是查詢
//
// 但 operator[] 的語意是:**鍵不存在時,以 mapped_type 的 value-initialization
// 建立一個元素並插入,再回傳它的參考**。所以上面這行讓 map 多了一個 {"Eve",0},
// size() 加一 —— 一個外觀是唯讀的運算式,改動了容器。
//
// 這帶來三個實際後果:
//   (a) 迴圈中誤用會讓 map 無聲膨脹。用 map 統計「哪些鍵出現過」時,若在條件式裡
//       寫 m[k],不管命中與否都會塞一筆進去,統計結果直接失真。
//   (b) **operator[] 不能用在 const map 上**。因為它可能修改容器,所以它根本沒有
//       const 版本 —— 在 const map 上寫 m[k] 是編譯錯誤,不是執行期問題。
//       這是初學者把 map 傳成 const& 之後最常撞到的牆。
//   (c) **mapped_type 必須是 default-constructible**。因為 operator[] 必須有能力
//       憑空生一個值出來。所以 map<string, SomeTypeWithoutDefaultCtor> 這個型別
//       本身合法,但你一旦寫 m[k] 就編譯不過。
//
// 正確的替代品要看意圖:
//     只想查、不想改  → find()(回 end() 表示不存在)、C++20 的 contains()
//     查不到算錯誤    → at()(丟 std::out_of_range)
//     不存在才插入    → try_emplace() / insert()
//     不管有沒有都覆蓋 → insert_or_assign()
//
// 【4. 為什麼要用 map 而不是 unordered_map:順序與 lower_bound】
// 如果只比較「單點查找誰快」,unordered_map 幾乎總是贏。map 值得存在,是因為
// 三件 unordered_map 做不到的事:
//   (a) **有序走訪**。直接 for 一遍就是排序結果,不必額外 sort。
//   (b) **範圍查詢**。lower_bound(k) 回傳「第一個 >= k 的元素」,這讓你能問
//       「離 k 最近的下一筆是誰」「所有落在 [lo, hi) 的資料」。雜湊表把鍵打散了,
//       這類問題它一題都答不了 —— 本檔的 LeetCode 729 正是靠這點解的。
//   (c) **最壞情況 O(log n) 保證**,不會因為鍵的分佈而退化。
//
// 【5. try_emplace 修好了 emplace 的一個真實缺陷(C++17)】
// emplace(k, v) 在**鍵已存在**時的行為長期是個坑:標準允許它「為了嘗試插入而先
// 建構出節點」,而若 v 是以 std::move 傳入的,參數就已經**被搬走了**,即使最後
// 什麼都沒插入。
//
//   本機實測(GCC 15.2.0):對已存在的鍵呼叫 emplace(k, std::move(s)) 之後,
//   s 已被搬走。標準只保證搬移後的物件處於 **valid but unspecified** 狀態
//   (libstdc++ 實測為空字串,但不可依賴這個值)。
//
// try_emplace 的規定就是為了修這個洞:**先查鍵,鍵已存在就直接回傳,絕不碰你的
// 參數**。所以在「有就沿用、沒有才建」的場景,一律用 try_emplace 而非 emplace。
// 順帶一提,try_emplace 對 map<string, vector<int>> 這種昂貴的 mapped_type 也更省
// —— 它不會先做一個節點再丟掉。
//
// 【概念補充 Concept Deep Dive】
//
// (A) value_type 是 pair<const Key, T> —— 那個 const 影響很大
//     map 的元素不是 pair<Key,T>,而是 pair<**const** Key, T>。原因很直接:
//     Key 決定了元素在樹中的位置,一旦允許就地改鍵,樹的排序不變式立刻被破壞。
//     所以標準把 Key 直接鎖成 const。實際影響:
//       * 不能寫 it->first = newKey;(編譯錯誤),要改鍵只能 erase 再 insert
//         (C++17 起可用 extract() 取出節點、改鍵、再 insert,免去重新配置)。
//       * structured binding 拿到的 first 是 const 的:
//             for (auto& [name, age] : m) { age += 1; }   // age 可改
//             for (auto& [name, age] : m) { name = "x"; } // 編譯錯誤
//       * 因為 value_type 帶 const,std::sort 這類需要搬移元素的演算法無法作用在
//         map 上 —— 這也合理,它本來就已經是排序的了。
//
// (B) 節點式結構:迭代器與參考的穩定性
//     map 是節點式(node-based)容器,每個元素獨立配置在自己的節點裡。這帶來一個
//     和 vector 完全相反的性質:**插入新元素不會使任何既有的迭代器/參考失效**;
//     erase 也只讓「被刪掉的那一個」失效,其他全部安好。vector 一次 reallocation
//     就讓全部迭代器失效,對比非常明顯。代價是每個元素都要一次獨立的 heap 配置,
//     且節點散落在記憶體各處,cache locality 遠差於 vector —— 這是 map 在小資料量
//     時常常輸給「vector + sort + binary_search」的主因。
//
// (C) Compare 決定的是「等價」而非「相等」
//     map 從不呼叫 operator==。它判斷兩個鍵是不是同一個,用的是
//     `!comp(a,b) && !comp(b,a)`,稱為 **equivalence(等價)**。這代表:
//       * 自訂比較器只要提供嚴格弱序(strict weak ordering)即可,不需要 operator==。
//       * 若你的比較器故意忽略某些差異(例如不分大小寫比較),那 "Alice" 與 "ALICE"
//         就會被視為同一個鍵。這常被拿來做大小寫不敏感的查表。
//       * 比較器若不滿足嚴格弱序(例如寫成 <=),行為是 undefined,不保證會報錯,
//         可能只是安靜地產生錯誤結果。
//
// (D) 編譯器實際做了什麼:operator[] 展開後的樣子
//     m[k] 大致等價於:
//         auto it = m.lower_bound(k);
//         if (it == m.end() || m.key_comp()(k, it->first))
//             it = m.emplace_hint(it, k, T{});     // 沒找到 → 就地插入預設值
//         return it->second;
//     由此可以直接看出兩件事:為什麼它需要 T 是 default-constructible(那個 T{}),
//     以及為什麼它不可能有 const 版本(它會寫入容器)。
//
// 【注意事項 Pay Attention】
//  1. operator[] 在鍵不存在時**會插入**。若只是想查詢,請用 find()/at()/
//     (C++20) contains()。這不是效能問題,是正確性問題 —— 它會改變 size()。
//  2. operator[] 沒有 const 版本,無法用於 const map&。傳 const map& 進函式後
//     只能用 find()/at()。
//  3. at() 找不到鍵會丟 std::out_of_range;operator[] 永遠不丟例外(它改成插入)。
//     兩者的錯誤哲學完全不同,不要混用。
//  4. value_type 是 pair<const Key, T>。不能透過迭代器改鍵;structured binding
//     取得的第一個名字是 const 的。
//  5. 紅黑樹、sizeof(std::map<int,int>) == 48 都屬**實作定義**。標準只保證
//     O(log n) 與有序走訪,不要在程式裡依賴任何特定的樹結構或記憶體大小。
//  6. 對已存在的鍵呼叫 emplace(k, std::move(v)),v **可能已被搬走**,且搬移後的
//     狀態是 valid but unspecified —— 不保證是空字串,也不保證維持原值。要避免
//     這件事請用 try_emplace(C++17)。
//  7. 自訂 Compare 必須是嚴格弱序。用 <= 之類的非嚴格比較是 undefined behavior,
//     不保證會被偵測到。
//  8. contains() 是 C++20 才加入的;在 C++17 只能用 count(k) > 0 或
//     find(k) != end()。std::erase_if(map, pred) 同樣是 C++20。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::map
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::map 的 operator[] 和 at() 有什麼差別?什麼時候該用哪一個?
//     答：operator[] 在鍵不存在時會**以預設值建立並插入**該元素,然後回傳參考;
//         它永遠不丟例外,但會改變容器(size() 增加)。at() 則完全不插入,
//         找不到就丟 std::out_of_range。所以:要寫入或「沒有就給預設值」用 [],
//         純查詢且找不到算錯誤用 at(),純查詢且找不到是正常情況用 find()。
//     追問：那為什麼 operator[] 不能用在 const map 上?
//         → 因為它可能插入元素、修改容器,語意上就不是 const 操作,
//           所以標準根本沒有為它定義 const 多載;在 const map 上呼叫是編譯錯誤。
//
// 🔥 Q2. map 的 value_type 是什麼?為什麼要那樣設計?
//     答：是 std::pair<**const** Key, T>,Key 帶 const。因為 Key 決定元素在
//         紅黑樹中的位置,若允許就地改鍵,排序不變式立刻被破壞、後續查找全錯。
//         把 Key 鎖成 const 就在型別層面杜絕了這件事。要改鍵只能 erase 後重新
//         insert,或用 C++17 的 extract() 取出節點改完再放回。
//     追問：那 structured binding 寫 auto& [k, v] 時,k 和 v 分別能不能改?
//         → v 可以改;k 是 const 的,改它編譯不過。
//
// 🔥 Q3. 什麼時候該用 map 而不是 unordered_map?
//     答：三種情況:(1) 需要**有序走訪**,直接 for 一遍就是排序好的;
//         (2) 需要**範圍查詢**,例如 lower_bound 找「第一個 >= k 的元素」、
//         找前驅後繼、取出某區間的所有資料 —— 雜湊表把鍵打散了,做不到這些;
//         (3) 需要**最壞情況 O(log n) 保證**,不會因鍵的分佈而退化成 O(n)。
//         若只做單點查找、又不在意順序,unordered_map 通常明顯較快。
//     追問：unordered_map 最壞情況為什麼會退化成 O(n)?
//         → 所有鍵雜湊到同一個 bucket 時,該 bucket 的鏈結串列退化成線性搜尋。
//           惡意輸入可以刻意構造這種碰撞(hash-flooding),形成 DoS 攻擊面。
//
// ⚠️ 陷阱. 下面這段程式想統計「哪些使用者曾經出現」,它有什麼問題?
//         for (const auto& name : names)
//             if (counter[name] > 0) ++dup;
//     答：counter[name] 在 name 不存在時會**插入一筆 {name, 0}**。所以迴圈跑完
//         後,counter 裡會有每一個查詢過的鍵,size() 完全失真,後續若拿
//         counter.size() 當「出現過的使用者數」就直接錯了。應改用
//         counter.find(name) != counter.end(),或 C++20 的 counter.contains(name)。
//     為什麼會錯：多數人把 operator[] 理解成「像陣列一樣的讀取」。但 map 的
//         operator[] 回傳的是 T&(可寫的參考),它必須先讓那個元素**存在**才能
//         回傳參考 —— 所以「讀取不存在的鍵」這個動作在 map 裡邏輯上不成立,
//         標準選擇的解法就是「那就順手建一個」。
//
// ⚠️ 陷阱. m.emplace(key, std::move(bigString)) 在 key 已存在時,bigString 還在嗎?
//     答：不保證還在。標準允許實作為了嘗試插入而先建構節點,參數因此被搬走,
//         即使最後因鍵重複而沒有插入任何東西。搬移後的物件處於 valid but
//         unspecified 狀態 —— 不保證是空的,也不保證維持原值。C++17 的
//         try_emplace 就是為了修這個缺陷:它保證先查鍵,已存在就直接返回,
//         完全不碰你的參數。
//     為什麼會錯：直覺會認為「沒插入成功,參數當然原封不動」。但 emplace 的
//         語意是「就地建構」,建構動作可能發生在「發現重複」之前 —— 是否如此
//         屬實作細節,所以標準乾脆規定你不能依賴參數還活著。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <sstream>
#include <type_traits>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 729. My Calendar I
//   題目：實作一個行事曆,每次 book(start, end) 要求登記半開區間 [start, end)。
//     若與任何已登記的區間重疊則拒絕(回傳 false),否則登記成功(回傳 true)。
//   為什麼用到本主題：這題是 std::map 的招牌題 —— 它需要的不是「查某個鍵在不在」,
//     而是「離我最近的那筆預約是誰」。map 把 start 當鍵、自動保持有序,
//     lower_bound(start) 一步就找到「第一個開始時間 >= start 的預約」,
//     再往前退一格就是前一筆。unordered_map 把鍵打散了,這題它完全無法處理。
//   複雜度：每次 book 為 O(log n)。
// -----------------------------------------------------------------------------
class MyCalendar {
    std::map<int, int> booked_;      // start -> end,半開區間 [start, end)

public:
    bool book(int start, int end) {
        // 第一個 start_i >= start 的預約
        auto next = booked_.lower_bound(start);

        // 後方衝突:下一筆的開始時間落在我的區間內
        if (next != booked_.end() && next->first < end) return false;

        // 前方衝突:前一筆的結束時間越過了我的開始時間
        if (next != booked_.begin()) {
            auto prev = std::prev(next);
            if (prev->second > start) return false;
        }

        booked_.emplace(start, end);
        return true;
    }

    std::size_t size() const { return booked_.size(); }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】載入 INI 風格的設定檔,並補上預設值
//   情境：幾乎所有服務啟動時都要做這件事 —— 讀一份設定檔,後出現的同名鍵覆蓋
//     先前的值(方便使用者在檔尾臨時覆寫),讀完再把使用者沒寫到的項目補上預設值。
//   為什麼用到本主題：這正好對應 C++17 新增的兩個介面,而且語意剛好相反:
//     * insert_or_assign：有就覆蓋、沒有就插入 → 用來套用設定檔的每一行。
//     * try_emplace     ：有就**原封不動**、沒有才建立 → 用來補預設值,
//                         絕不會蓋掉使用者已經指定的設定。
//     用 map 而非 unordered_map 的理由:設定項目列印時按鍵排序,診斷輸出穩定可讀。
// -----------------------------------------------------------------------------
std::map<std::string, std::string> loadConfig(const std::vector<std::string>& lines) {
    std::map<std::string, std::string> cfg;

    for (const std::string& raw : lines) {
        // 去掉註解:# 之後全部忽略
        std::string line = raw.substr(0, raw.find('#'));

        // 去掉頭尾空白
        const std::size_t b = line.find_first_not_of(" \t");
        if (b == std::string::npos) continue;                 // 空行或純註解
        const std::size_t e = line.find_last_not_of(" \t");
        line = line.substr(b, e - b + 1);

        const std::size_t eq = line.find('=');
        if (eq == std::string::npos) continue;                // 不是 key=value,略過

        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);
        // 個別再修一次尾/頭空白(key= value 這種寫法很常見)
        if (const std::size_t ke = key.find_last_not_of(" \t"); ke != std::string::npos)
            key = key.substr(0, ke + 1);
        if (const std::size_t vb = val.find_first_not_of(" \t"); vb != std::string::npos)
            val = val.substr(vb);
        else
            val.clear();

        // 後出現的覆蓋先出現的 → insert_or_assign
        cfg.insert_or_assign(std::move(key), std::move(val));
    }
    return cfg;
}

void applyDefaults(std::map<std::string, std::string>& cfg) {
    // try_emplace:鍵已存在就完全不動作,不會蓋掉使用者的設定
    cfg.try_emplace("host", "127.0.0.1");
    cfg.try_emplace("port", "8080");
    cfg.try_emplace("timeout_ms", "3000");
    cfg.try_emplace("log_level", "info");
}

int main() {
    // ── 原始課堂示範:std::map 的基本操作 ────────────────────────────────────
    std::cout << "=== std::map ===" << std::endl;

    std::map<std::string, int> ages;

    // 插入方式一：operator[]
    ages["Alice"] = 25;
    ages["Bob"] = 30;

    // 插入方式二：insert
    ages.insert({"Charlie", 35});
    ages.insert(std::make_pair("Diana", 28));

    // 按鍵排序
    std::cout << "所有人（按名字排序）:" << std::endl;
    for (const auto& pair : ages) {
        std::cout << "  " << pair.first << ": " << pair.second << std::endl;
    }

    // 查找
    auto it = ages.find("Bob");
    if (it != ages.end()) {
        std::cout << "Bob 的年齡: " << it->second << std::endl;
    }

    // operator[] 的陷阱：會插入預設值！
    // 如果查找不存在的鍵，會插入一個預設值（對於 int 是 0）
    // 這裡會插入 Eve = 0，因為 Eve 不存在
    // 注意：這會改變 map 的內容，因為 operator[] 會插入預設值
    std::cout << "Eve 的年齡: " << ages["Eve"] << std::endl;  // 插入 Eve = 0
    std::cout << "現在的大小: " << ages.size() << std::endl;

    // ── 四種「查詢」介面的差異 ───────────────────────────────────────────────
    std::cout << "\n=== 查詢介面比較: [] / at() / find() / count() ===" << std::endl;
    std::cout << "查詢前 size = " << ages.size() << std::endl;

    // find():找不到回 end(),絕不插入
    if (ages.find("Frank") == ages.end())
        std::cout << "  find(\"Frank\")  → 找不到,且**沒有**插入" << std::endl;

    // count():map 的 count 只會是 0 或 1(C++20 可改用 contains())
    std::cout << "  count(\"Frank\") → " << ages.count("Frank")
              << " (map 沒有重複鍵,只會是 0 或 1)" << std::endl;

    // at():找不到丟例外,絕不插入
    try {
        std::cout << "  at(\"Frank\")    → ";
        std::cout << ages.at("Frank") << std::endl;
    } catch (const std::out_of_range& e) {
        std::cout << "丟出 std::out_of_range: " << e.what() << std::endl;
    }
    std::cout << "以上三種查詢後 size = " << ages.size() << " (完全沒變)" << std::endl;

    // operator[]:找不到就插入
    const int frank = ages["Frank"];
    std::cout << "  ages[\"Frank\"]  → " << frank
              << ",但 size 變成 " << ages.size() << " ← 這就是那個陷阱" << std::endl;

    // ── value_type 的 const Key ──────────────────────────────────────────────
    std::cout << "\n=== value_type 是 pair<const Key, T> ===" << std::endl;
    static_assert(std::is_same_v<std::map<std::string, int>::value_type,
                                 std::pair<const std::string, int>>,
                  "map 的 value_type 第一個型別帶 const");
    std::cout << "static_assert 通過:value_type == pair<const string, int>" << std::endl;

    // structured binding：age 可改,name 是 const 改不了(改了會編譯錯誤)
    for (auto& [name, age] : ages) {
        if (name == "Alice") age += 1;        // 合法:mapped 可改
        // name = "X";                        // 編譯錯誤:key 是 const
    }
    std::cout << "把 Alice 的年齡 +1 後: " << ages.at("Alice") << std::endl;

    // ── try_emplace 修好了 emplace 的缺陷 ────────────────────────────────────
    std::cout << "\n=== emplace vs try_emplace (C++17) ===" << std::endl;
    std::map<std::string, std::string> notes;
    notes.emplace("bug-1", "原始內容");

    std::string arg1 = "要塞進去的長字串-A";
    notes.emplace("bug-1", std::move(arg1));      // 鍵已存在 → 不會插入
    std::cout << "  emplace 對已存在的鍵: 插入失敗, 但參數長度變成 "
              << arg1.size()
              << " (被搬走了; 搬移後狀態是 valid but unspecified)" << std::endl;

    std::string arg2 = "要塞進去的長字串-B";
    notes.try_emplace("bug-1", std::move(arg2));  // 鍵已存在 → 直接返回,不碰參數
    std::cout << "  try_emplace 對已存在的鍵: 插入失敗, 參數長度仍是 "
              << arg2.size() << " (保證原封不動)" << std::endl;
    std::cout << "  notes[\"bug-1\"] = " << notes.at("bug-1") << " (兩次都沒被覆蓋)" << std::endl;

    // insert_or_assign:有就覆蓋
    notes.insert_or_assign("bug-1", "被 insert_or_assign 覆蓋了");
    std::cout << "  insert_or_assign 後 = " << notes.at("bug-1") << std::endl;

    // ── lower_bound / upper_bound:map 才有的範圍查詢 ────────────────────────
    std::cout << "\n=== lower_bound / upper_bound (unordered_map 沒有) ===" << std::endl;
    std::map<int, std::string> events{
        {100, "服務啟動"}, {250, "設定重載"}, {400, "健康檢查"}, {780, "服務關閉"}
    };
    const int t = 300;
    auto lb = events.lower_bound(t);              // 第一個 >= 300
    std::cout << "  時間 " << t << " 之後的第一個事件: "
              << lb->first << " (" << lb->second << ")" << std::endl;
    auto pv = std::prev(lb);                      // 往前退一格就是前一筆
    std::cout << "  時間 " << t << " 之前的最後一個事件: "
              << pv->first << " (" << pv->second << ")" << std::endl;
    std::cout << "  區間 [200, 500) 內的事件: ";
    for (auto i = events.lower_bound(200); i != events.lower_bound(500); ++i)
        std::cout << i->first << " ";
    std::cout << std::endl;

    // ── LeetCode 729 ─────────────────────────────────────────────────────────
    std::cout << "\n=== LeetCode 729. My Calendar I ===" << std::endl;
    MyCalendar cal;
    const std::vector<std::pair<int, int>> requests{
        {10, 20}, {15, 25}, {20, 30}, {5, 10}, {8, 12}
    };
    for (const auto& [s, e] : requests) {
        const bool ok = cal.book(s, e);
        std::cout << "  book(" << s << ", " << e << ") = "
                  << std::boolalpha << ok
                  << (ok ? "" : "  ← 與既有預約重疊") << std::endl;
    }
    std::cout << "  最後成功登記 " << cal.size() << " 筆" << std::endl;

    // ── 日常實務:載入設定檔 + 補預設值 ──────────────────────────────────────
    std::cout << "\n=== 日常實務: 設定檔載入 (insert_or_assign + try_emplace) ===" << std::endl;
    const std::vector<std::string> configFile{
        "# 服務設定檔",
        "host = 10.0.0.7",
        "port = 9090",
        "",
        "log_level = debug      # 上線前記得改回 info",
        "port = 9443            # 檔尾臨時覆寫,後者生效"
    };

    std::map<std::string, std::string> cfg = loadConfig(configFile);
    std::cout << "  套用設定檔後(注意 port 被後面那行覆蓋):" << std::endl;
    for (const auto& [k, v] : cfg)
        std::cout << "    " << k << " = " << v << std::endl;

    applyDefaults(cfg);
    std::cout << "  補上預設值後(timeout_ms 是補的,其餘維持使用者設定):" << std::endl;
    for (const auto& [k, v] : cfg)
        std::cout << "    " << k << " = " << v << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第六課：容器（Container）的概念與分類8.cpp" -o map_demo

// === 預期輸出 ===
// === std::map ===
// 所有人（按名字排序）:
//   Alice: 25
//   Bob: 30
//   Charlie: 35
//   Diana: 28
// Bob 的年齡: 30
// Eve 的年齡: 0
// 現在的大小: 5
//
// === 查詢介面比較: [] / at() / find() / count() ===
// 查詢前 size = 5
//   find("Frank")  → 找不到,且**沒有**插入
//   count("Frank") → 0 (map 沒有重複鍵,只會是 0 或 1)
//   at("Frank")    → 丟出 std::out_of_range: map::at
// 以上三種查詢後 size = 5 (完全沒變)
//   ages["Frank"]  → 0,但 size 變成 6 ← 這就是那個陷阱
//
// === value_type 是 pair<const Key, T> ===
// static_assert 通過:value_type == pair<const string, int>
// 把 Alice 的年齡 +1 後: 26
//
// === emplace vs try_emplace (C++17) ===
//   emplace 對已存在的鍵: 插入失敗, 但參數長度變成 0 (被搬走了; 搬移後狀態是 valid but unspecified)
//   try_emplace 對已存在的鍵: 插入失敗, 參數長度仍是 26 (保證原封不動)
//   notes["bug-1"] = 原始內容 (兩次都沒被覆蓋)
//   insert_or_assign 後 = 被 insert_or_assign 覆蓋了
//
// === lower_bound / upper_bound (unordered_map 沒有) ===
//   時間 300 之後的第一個事件: 400 (健康檢查)
//   時間 300 之前的最後一個事件: 250 (設定重載)
//   區間 [200, 500) 內的事件: 250 400
//
// === LeetCode 729. My Calendar I ===
//   book(10, 20) = true
//   book(15, 25) = false  ← 與既有預約重疊
//   book(20, 30) = true
//   book(5, 10) = true
//   book(8, 12) = false  ← 與既有預約重疊
//   最後成功登記 3 筆
//
// === 日常實務: 設定檔載入 (insert_or_assign + try_emplace) ===
//   套用設定檔後(注意 port 被後面那行覆蓋):
//     host = 10.0.0.7
//     log_level = debug
//     port = 9443
//   補上預設值後(timeout_ms 是補的,其餘維持使用者設定):
//     host = 10.0.0.7
//     log_level = debug
//     port = 9443
//     timeout_ms = 3000
