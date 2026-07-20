// =============================================================================
//  第六課 11 — std::unordered_map：平均 O(1) 的鍵值對映表
// =============================================================================
//
// 【主題資訊 Information】
//   template<class Key, class T,
//            class Hash     = std::hash<Key>,
//            class KeyEqual = std::equal_to<Key>,
//            class Allocator = std::allocator<std::pair<const Key, T>>>
//   class unordered_map;                              // <unordered_map>,C++11 起
//
//   常用介面：
//     T& operator[](const Key& k);                    // 平均 O(1),**不存在會自動插入**
//     T& at(const Key& k);                            // 平均 O(1),不存在丟 out_of_range
//     iterator find(const Key& k);                    // 平均 O(1),最壞 O(n)
//     std::pair<iterator,bool> insert(const value_type& v);       // 已存在則不覆蓋
//     std::pair<iterator,bool> insert_or_assign(k, v);            // C++17,已存在就覆蓋
//     std::pair<iterator,bool> try_emplace(k, args...);           // C++17,已存在則不建構
//     bool contains(const Key& k) const;              // C++20
//
//   雜湊表狀態：bucket_count() / load_factor() / max_load_factor() / reserve() / rehash()
//
//   **沒有** lower_bound / upper_bound / 有序走訪。需要這些能力請用 std::map。
//
//   複雜度：查找、插入、刪除平均 O(1),**最壞 O(n)**(全部碰撞時)。
//   標頭檔：#include <unordered_map>
//
// 【詳細解釋 Explanation】
//
// 【1. unordered_map 與 map:先比能力,再比速度】
// 這兩者常被當成「慢的版本」與「快的版本」,但那是危險的簡化。它們的底層資料
// 結構根本不同,能力也不同:
//
//   ┌──────────────┬────────────────────────┬────────────────────────────┐
//   │              │ std::map               │ std::unordered_map         │
//   ├──────────────┼────────────────────────┼────────────────────────────┤
//   │ 底層         │ 紅黑樹(平衡 BST)     │ 雜湊表(separate chaining)│
//   │ 查找複雜度   │ O(log n) **最壞保證**  │ 平均 O(1),**最壞 O(n)**   │
//   │ 走訪順序     │ 依 key 排序            │ **未由標準規定**           │
//   │ 範圍查詢     │ lower_bound/upper_bound│ 無(只能 O(n) 全掃)       │
//   │ 最小/最大    │ begin() / rbegin()     │ 無(只能 O(n) 全掃)       │
//   │ key 的需求   │ 可比較大小(operator<)│ 可雜湊 + 可判相等          │
//   └──────────────┴────────────────────────┴────────────────────────────┘
//
// 所以選擇的正確順序是 **先看能力,再看速度**:
//   * 需要「照 key 排序輸出」、「找出 100~200 之間的所有紀錄」、「取最小的 key」
//     → 只能用 map。unordered_map 不是比較慢,是**根本做不到**。
//   * 需要**最壞情況**的時間保證(即時系統;或 key 直接來自外部輸入的服務)
//     → 用 map。雜湊表可能被惡意輸入誘導退化成 O(n)(hash flooding)。
//   * key 型別只有 operator< 沒有 std::hash → 用 map 省事。
//   * 以上皆非,只是要「用 key 查 value」 → unordered_map 通常明顯較快。
//
// 【2. operator[] 的陷阱:它會「偷偷插入」,而且不能用在 const 物件上】
// 這是 unordered_map / map 共通、實務上最常釀成 bug 的一點。
//
//     std::unordered_map<std::string, int> ages;
//     int n = ages["Nobody"];      // ← 這行不只是「查詢」!
//
// operator[] 的語意是「回傳 k 對應值的 reference,**若 k 不存在就先預設建構
// 一個插入進去**再回傳」。所以上面這行的副作用是:容器裡多了一筆
// {"Nobody", 0}。在「只是想查一下」的情境下,這會讓容器不斷長大,
// 也會讓後續的 size()、走訪結果全部不符預期。
//
// 三種正確的「純查詢」寫法:
//     if (auto it = ages.find(k); it != ages.end()) use(it->second);  // C++17 init-if
//     int v = ages.at(k);           // 不存在丟 std::out_of_range,不會插入
//     if (ages.contains(k)) ...     // C++20,語意最清楚
//
// 還有一個直接的後果:**operator[] 不是 const 成員函式**(它可能修改容器),
// 所以你不能對 const unordered_map& 使用 [] —— 那是編譯錯誤。寫工具函式時
// 若參數是 const reference,就只能用 at() 或 find()。本檔的實務範例遵循這一點。
//
// 反過來說,「不存在就自動建立預設值」在**計數**情境下非常好用:
//     ++wordCount[word];      // 第一次出現時自動建立 0,再 ++ 變成 1
// 這是 operator[] 最經典、也最恰當的用法。
//
// 【3. insert / operator[] / insert_or_assign / try_emplace 的分工】
// 四個介面看似重疊,實際上各自對應不同的意圖,C++17 補上後兩個正是為了填補
// 前兩個的語意空缺:
//
//   ┌──────────────────┬──────────────┬────────────────┬────────────────────┐
//   │                  │ key 已存在   │ key 不存在     │ 備註               │
//   ├──────────────────┼──────────────┼────────────────┼────────────────────┤
//   │ insert(k, v)     │ **不覆蓋**   │ 插入           │ 回傳 bool 告知結果 │
//   │ m[k] = v         │ 覆蓋         │ 先預設建構再賦值│ 需 T 可預設建構    │
//   │ insert_or_assign │ **覆蓋**     │ 插入           │ C++17,意圖最明確  │
//   │ try_emplace      │ **不覆蓋**   │ 就地建構       │ C++17,不浪費建構  │
//   └──────────────────┴──────────────┴────────────────┴────────────────────┘
//
// 兩個 C++17 新介面各自解決一個具體痛點:
//
//   * insert_or_assign：想「有就更新、沒有就新增」時,以前只能寫 m[k] = v,
//     但那要求 T **必須可預設建構**,而且會白白建構一次再賦值。
//     insert_or_assign 沒有這兩個限制,而且回傳的 bool 明確告訴你是新增還是更新。
//
//   * try_emplace：emplace 有一個惡名昭彰的問題 —— 即使 key 已存在(最後不會
//     插入),它仍然**可能已經先建構好了那個元素**,然後再丟掉。對於持有資源的
//     型別(std::string、std::vector、unique_ptr)這是白費的配置;
//     若參數是 move 進來的,更糟的是**來源物件可能已經被搬空**,而元素卻沒插入。
//     try_emplace 保證「key 已存在就完全不建構、不碰你的參數」。
//
// 【4. 雜湊表機制:與 unordered_set 完全相同】
// unordered_map 與 unordered_set 在 libstdc++ 底下共用同一份 _Hashtable 實作,
// 差別只在元素型別是 std::pair<const Key, T> 而不是 Key。所以下列性質完全一致:
//
//   * separate chaining:同桶元素串成單向鏈結。查找 = 算桶 O(1) + 走鏈 O(鏈長)。
//   * load_factor() = size() / bucket_count();max_load_factor() 預設 1.0,
//     超過就 rehash(重建桶陣列並重新分配所有元素,O(n))。
//   * 「平均 O(1)」是**攤提**結果 —— 偶爾一次插入要付 O(n) 的 rehash。
//     已知資料量時 reserve(n) 可完全避開。
//   * **最壞 O(n)**:所有 key 碰撞到同一桶時,退化成線性掃描。hash flooding
//     就是刻意誘發這件事的攻擊手法。
//
// 【5. rehash 的失效規則:iterator 失效,references 不失效】
// 這點與 unordered_set 相同,但在 map 上更有實務價值,值得再強調一次:
//
//     * rehash 後 **所有 iterator 失效**(桶陣列重建、走訪順序全變)
//     * 但 **指向 value 的 reference / pointer 仍然有效**(節點沒有搬家)
//
// 因為每個 (key, value) 住在自己獨立配置的節點裡,桶陣列只存指標。這代表:
//
//     auto& slot = cache["session-42"];    // 取得 value 的 reference
//     for (int i = 0; i < 100000; ++i)     // 大量插入 → 必然多次 rehash
//         cache["k" + std::to_string(i)] = i;
//     slot = 999;                          // ✓ 仍然合法,slot 依然指向同一個 value
//
// 同樣的程式碼換成 std::vector 就是 undefined behavior(擴容搬移元素)。
// 本檔的 main() 用實際位址驗證了這個差異。
//
// 【6. key 的型別要求:std::hash 有沒有,決定你能不能直接用】
// map 只要求 key 有 operator<(或自訂比較器)。unordered_map 則要求**兩樣**:
//     (1) std::hash<Key> 的特化(或自訂 Hash functor)
//     (2) operator==(或自訂 KeyEqual)—— 碰撞後沿鏈比對時使用
//
// 標準只為內建型別、std::string、std::string_view 等提供 std::hash 特化,
// **沒有**為 std::pair、std::tuple、std::vector 或自訂 struct 提供。所以:
//
//     std::unordered_map<std::pair<int,int>, int> m;   // ✗ 編譯錯誤
//     std::map<std::pair<int,int>, int> m2;            // ✓ pair 有 operator<
//
// 自備 hash 時務必守住一致性:**a == b ⇒ h(a) == h(b)**。違反這條會讓兩個
// 相等的 key 落在不同桶,容器裡同時存在兩筆同 key 的資料,唯一性保證瓦解。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 記憶體佈局:元素型別是 pair<const Key, T>,注意那個 const
//     本機實測 sizeof(std::unordered_map<int,int>) == 56(**實作定義**),
//     與 sizeof(std::unordered_set<int>) 相同 —— 因為兩者共用同一份
//     _Hashtable 骨架,元素本身都在 heap 節點裡,不佔容器本體。
//
//     每個節點大致是:
//         [next 指標][(可選)快取的雜湊值][ std::pair<const Key, T> ]
//
//     value_type 是 std::pair<**const** Key, T>,key 帶 const。這是刻意的:
//     key 決定元素落在哪個桶,若允許就地修改 key,元素就會待在錯誤的桶裡,
//     從此再也找不到。所以走訪時只能改 second,不能改 first:
//         for (auto& [k, v] : m) { v *= 2; }        // ✓ 合法
//         for (auto& [k, v] : m) { k = "new"; }     // ✗ 編譯錯誤,k 是 const
//     真的要換 key,只能 erase 再 insert(或 C++17 的 extract() 取出節點、
//     改 key 後再 insert 回去 —— 那是唯一不需重新配置節點的做法)。
//
// (B) structured bindings 讓走訪清爽很多(C++17)
//         for (const auto& [name, age] : ages)      // C++17
//         for (const auto& p : ages) { p.first; p.second; }   // C++11 寫法
//     兩者產生的機器碼相同,但前者可讀性明顯較好,也避免了 .first/.second
//     這種毫無語意的名字。注意 structured bindings 是 **C++17** 特性。
//
// (C) 為什麼走訪順序「看起來像亂數」卻又完全確定
//     順序由「桶的走訪次序」與「每桶內鏈的次序」共同決定,而這兩者又取決於
//     雜湊函式與插入歷史。所以它**不是亂數**(同一支程式同樣的輸入,每次執行
//     結果通常相同),但**標準完全沒有規定**它 —— 換編譯器、換標準庫版本、
//     甚至只是改變插入順序,結果都可能不同。
//     結論不變:需要穩定輸出時,複製到 vector 再排序。
//
// (D) 為什麼 map 有時反而比 unordered_map 快
//     大 O 只描述成長趨勢,不描述常數。實務上:
//       * n 很小(幾十筆)時,紅黑樹的幾次比較可能比「算雜湊 + 追指標」還快,
//         尤其 key 是 int 這種比較極廉價的型別。
//       * 雜湊 std::string 要走過整個字串,是 O(字串長度) 的實際成本,
//         而樹的比較通常在前幾個字元就分出勝負。
//       * 兩者都是節點式容器、cache 都不友善,但雜湊表的存取更跳躍。
//     所以「unordered_map 一定比較快」是錯的。效能問題請實測,不要背結論。
//
// 【注意事項 Pay Attention】
//  1. **operator[] 會插入**。純查詢請用 find() / at() / contains(),
//     否則容器會被查詢行為悄悄撐大。
//  2. operator[] **不是 const 成員函式**,不能用在 const unordered_map& 上 ——
//     那是編譯錯誤。const 情境只能用 at() 或 find()。
//  3. **走訪順序未由標準規定**。不同實作、不同插入歷史、不同版本都可能不同,
//     絕不可寫進測試斷言或程式邏輯。要穩定輸出就複製到 vector 再排序。
//  4. 平均 O(1) 是**平均**;最壞 O(n)。需要最壞情況保證(或 key 來自不可信
//     外部輸入)時,請考慮 std::map。
//  5. rehash 後 **iterator 全部失效**,但 **reference / pointer 仍有效**
//     (節點不搬家)。這與 vector 恰好相反。
//  6. value_type 是 std::pair<**const** Key, T>。走訪時可以改 second,
//     **不能**改 first;要換 key 只能 erase + insert,或用 C++17 的 extract()。
//  7. std::hash **沒有** std::pair / std::tuple / 自訂 struct 的特化。
//     自訂 key 需自備 Hash **與** operator==,且必須滿足 a == b ⇒ h(a) == h(b)。
//  8. bucket_count() 的數值是**實作定義**(libstdc++ 用質數,MSVC 用 2 的冪),
//     不可跨平台假設。已知資料量時請 reserve(n)。
//  9. C++ 標準版本:unordered_map 是 **C++11**;structured bindings、
//     try_emplace、insert_or_assign、extract/merge 是 **C++17**;
//     contains() 與 std::erase_if 是 **C++20**。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::unordered_map
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::map 和 std::unordered_map 該怎麼選?
//     答：**先看能力,再看速度**。map 是紅黑樹:有序走訪、支援 lower_bound
//         範圍查詢、可取最小/最大 key,而且 O(log n) 是**最壞情況保證**。
//         unordered_map 是雜湊表:平均 O(1) 但最壞 O(n),完全沒有順序可言,
//         也沒有範圍查詢 —— 那不是「比較慢」,是**沒有這個能力**。
//         若只是要「用 key 查 value」,unordered_map 通常較快;
//         但只要牽涉排序、範圍、或最壞情況保證,就只能用 map。
//     追問：那 unordered_map 是不是永遠比較快?
//         → 不是。大 O 不描述常數。n 很小時樹的幾次比較可能更快;
//           key 是長字串時,算雜湊要走過整個字串 O(len),而樹比較常在前幾個
//           字元就分出勝負。實務上請實測,不要背「雜湊一定快」。
//
// 🔥 Q2. C++17 的 try_emplace 解決了 emplace 的什麼問題?
//     答：emplace 在 key **已存在**時最終不會插入,但它**可能已經先建構好
//         元素才發現重複**,然後把它丟掉。對持有資源的型別(string、vector、
//         unique_ptr)這是白費的記憶體配置;更嚴重的是若參數是 std::move
//         進來的,來源物件**可能已經被搬空**,而元素卻沒有插入 —— 資料就這樣
//         不見了。try_emplace 保證 key 已存在時**完全不建構、不碰你的參數**。
//     追問：那 insert_or_assign 又解決什麼?
//         → 解決 m[k] = v 的兩個限制:(1) operator[] 要求 T **必須可預設建構**;
//           (2) 它會先預設建構一個再賦值,白做一次工。insert_or_assign 兩者
//           都沒有,而且回傳的 bool 會明確告訴你這次是「新增」還是「更新」。
//
// ⚠️ 陷阱. 一個 const std::unordered_map<std::string,int>& 參數,
//         在函式裡寫 m["key"] 查詢,會發生什麼事?
//     答：**編譯錯誤**。operator[] 不是 const 成員函式 —— 因為 key 不存在時
//         它會**插入**一筆預設值,那是修改容器的操作。const 情境下只能用
//         at()(不存在丟 std::out_of_range)、find(),或 C++20 的 contains()。
//     為什麼會錯：多數人把 operator[] 當成「陣列式的唯讀查詢」,
//         忘了它帶有插入的副作用。這個誤解在非 const 的情境下更危險:
//         那時它**編譯得過**,只是每查一次不存在的 key 就悄悄多一筆資料,
//         容器不斷長大而沒有任何警告 —— 是很典型的記憶體與邏輯雙重 bug。
//
// ⚠️ 陷阱. 對 unordered_map 大量插入觸發 rehash 後,
//         先前取得的 value reference(auto& v = m[k])還能用嗎?
//     答：**可以,reference 仍然有效**。失效的只有 iterator。
//         因為 separate chaining 下每個 (key,value) 住在自己獨立配置的節點裡,
//         桶陣列存的只是指標;rehash 重建桶陣列與鏈結關係,**節點本身沒有
//         搬家**,位址不變。所以 reference / pointer 有效,但走訪順序整個
//         重來,iterator 因此失效。
//     為什麼會錯：直覺來自 std::vector —— 擴容會搬移元素,iterator、
//         reference、pointer 一起失效,所以「持有 reference 再插入」是
//         undefined behavior。unordered 容器的答案**恰好相反**。
//         記憶關鍵是問「元素本身有沒有被搬動」,而不是「容器有沒有重新配置」。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <unordered_map>
#include <map>
#include <string>
#include <vector>
#include <algorithm>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 1】LeetCode 1. Two Sum
//   題目：給一個整數陣列與 target,找出兩個數字相加等於 target 的「索引」。
//   為什麼用到本主題：暴力雙迴圈是 O(n^2)。關鍵洞察是 —— 掃到 nums[i] 時,
//     我們要找的是「值為 target - nums[i] 的那個數在不在前面出現過」。
//     這正是「用值查索引」的查表需求,而且完全不需要順序 → unordered_map。
//     把「值 → 索引」邊掃邊建表,一次走訪就能得到答案。
//   複雜度：時間平均 O(n)(最壞 O(n^2),全部碰撞時),空間 O(n)。
// -----------------------------------------------------------------------------
std::vector<int> twoSum(const std::vector<int>& nums, int target) {
    std::unordered_map<int, int> seen;      // 值 → 索引
    seen.reserve(nums.size());              // 預先配置,避開中途 rehash

    for (int i = 0; i < static_cast<int>(nums.size()); ++i) {
        int need = target - nums[i];
        // 注意用 find 而非 seen[need] —— 後者會把不存在的 key 插進去
        auto it = seen.find(need);
        if (it != seen.end()) return {it->second, i};
        seen[nums[i]] = i;                  // 這裡的 [] 是「賦值」,插入是本意
    }
    return {};                              // 題目保證有解,實務上仍要處理無解
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 2】LeetCode 49. Group Anagrams
//   題目：把字串陣列中互為 anagram(字母重排)的字串分到同一組。
//   為什麼用到本主題：需要一個「正規化後的簽章 → 該組所有字串」的分組表。
//     把每個字串排序後的結果當作簽章("eat"/"tea"/"ate" 都變成 "aet"),
//     用 unordered_map<string, vector<string>> 一次走訪即可完成分組。
//     這裡完美體現 operator[] 的正當用法:key 不存在時自動建立一個空 vector,
//     直接 push_back 即可,不必先判斷存在性。
//   複雜度：時間平均 O(n * k log k)(n 個字串,每個長度 k,排序簽章),空間 O(n*k)。
//   注意：unordered_map 的走訪順序未由標準規定,所以最後輸出前會排序,
//     確保教學輸出可重現。
// -----------------------------------------------------------------------------
std::vector<std::vector<std::string>> groupAnagrams(const std::vector<std::string>& strs) {
    std::unordered_map<std::string, std::vector<std::string>> groups;

    for (const std::string& s : strs) {
        std::string key = s;
        std::sort(key.begin(), key.end());   // 排序後的字串就是 anagram 的簽章
        groups[key].push_back(s);            // key 不存在 → 自動建立空 vector
    }

    std::vector<std::vector<std::string>> out;
    out.reserve(groups.size());
    for (auto& [key, members] : groups) {    // structured bindings,C++17
        (void)key;                           // key 本身不需輸出
        std::sort(members.begin(), members.end());
        out.push_back(members);
    }
    // unordered_map 走訪順序未由標準規定 → 排序後再回傳,確保輸出穩定
    std::sort(out.begin(), out.end());
    return out;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 1】伺服器 log 的錯誤碼統計與端點分析
//   情境：從 access log 統計「各 HTTP 狀態碼出現幾次」、「哪個端點最常出錯」。
//     這是維運最日常的工作,資料量動輒數百萬行,查表效率直接決定跑多久。
//   為什麼用到本主題：
//     (1) 計數是 operator[] 最正當的用法:++counter[code] 在 key 不存在時
//         自動建立 0 再遞增,一行搞定,不必先 find 再判斷。
//     (2) 統計完全不需要順序,只要能快速「用 key 找到累加器」→ unordered_map。
//     (3) 但**報表要給人看**,所以輸出前一律複製到 vector 再排序 ——
//         unordered_map 的走訪順序未由標準規定,不能直接拿來輸出。
//   注意：參數是 const reference,所以函式內部**不能用 operator[]**
//     (它不是 const 成員函式),只能用 at() / find()。
// -----------------------------------------------------------------------------
struct LogEntry {
    std::string endpoint;
    int         status;
};

std::unordered_map<int, std::size_t> countByStatus(const std::vector<LogEntry>& entries) {
    std::unordered_map<int, std::size_t> counter;
    for (const LogEntry& e : entries) {
        ++counter[e.status];        // key 不存在 → 自動建立 0 → 再 ++ 變 1
    }
    return counter;
}

// 找出錯誤(status >= 400)最多的端點。注意參數是 const &,故只能用 find/at。
std::string worstEndpoint(const std::vector<LogEntry>& entries) {
    std::unordered_map<std::string, std::size_t> errors;
    for (const LogEntry& e : entries) {
        if (e.status >= 400) ++errors[e.endpoint];
    }
    if (errors.empty()) return "(no errors)";

    // 走訪順序未由標準規定 → 用「次數大者優先,同次數則字典序小者優先」
    // 這個確定性規則挑選,結果才可重現
    std::vector<std::pair<std::string, std::size_t>> v(errors.begin(), errors.end());
    std::sort(v.begin(), v.end(),
              [](const auto& a, const auto& b) {
                  if (a.second != b.second) return a.second > b.second;
                  return a.first < b.first;
              });
    return v.front().first + " (" + std::to_string(v.front().second) + " 次錯誤)";
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】設定檔載入 —— 示範 C++17 的 try_emplace / insert_or_assign
//   情境：設定值有優先順序:預設值 < 設定檔 < 環境變數。
//     「環境變數要覆蓋既有值」→ insert_or_assign
//     「預設值只在還沒設定時才補上,不可覆蓋使用者的設定」→ try_emplace
//   為什麼用到本主題：這兩個 C++17 介面正是為了讓「覆蓋 / 不覆蓋」的意圖
//     直接寫在函式名稱上,不必再靠 count() 先查一次或倚賴 operator[] 的副作用。
// -----------------------------------------------------------------------------
using Config = std::unordered_map<std::string, std::string>;

void applyDefaults(Config& cfg, const std::vector<std::pair<std::string, std::string>>& defs) {
    for (const auto& [k, v] : defs) {
        // try_emplace：已存在就完全不動(也不會建構那個 string),不覆蓋使用者設定
        cfg.try_emplace(k, v);
    }
}

void applyOverrides(Config& cfg, const std::vector<std::pair<std::string, std::string>>& envs) {
    for (const auto& [k, v] : envs) {
        // insert_or_assign：有就覆蓋、沒有就新增,並回報是新增還是更新
        auto [it, inserted] = cfg.insert_or_assign(k, v);
        std::cout << "  " << (inserted ? "新增" : "覆蓋") << " "
                  << it->first << " = " << it->second << std::endl;
    }
}

// 穩定輸出：unordered_map 走訪順序未由標準規定,先複製到 vector 再排序
void dumpConfig(const Config& cfg) {
    std::vector<std::pair<std::string, std::string>> v(cfg.begin(), cfg.end());
    std::sort(v.begin(), v.end());
    for (const auto& [k, val] : v)
        std::cout << "    " << k << " = " << val << std::endl;
}

int main() {
    // ── 原始課堂示範:unordered_map 的基本操作 ────────────────────────────
    std::cout << "=== std::unordered_map ===" << std::endl;

    std::unordered_map<std::string, int> ages;

    ages["Alice"] = 25;
    ages["Bob"] = 30;
    ages["Charlie"] = 35;
    ages["Diana"] = 28;

    // 順序不固定
    std::cout << "所有人:" << std::endl;
    for (const auto& pair : ages) {
        std::cout << "  " << pair.first << ": " << pair.second << std::endl;
    }

    // 查找：平均 O(1)
    auto it = ages.find("Bob");
    if (it != ages.end()) {
        std::cout << "Bob 的年齡: " << it->second << std::endl;
    }

    // ── 穩定輸出的正確做法:複製到 vector 再排序 ──────────────────────────
    std::cout << "\n=== 要穩定輸出就先排序 ===" << std::endl;
    {
        std::vector<std::pair<std::string, int>> sorted(ages.begin(), ages.end());
        std::sort(sorted.begin(), sorted.end());
        for (const auto& [name, age] : sorted)      // structured bindings,C++17
            std::cout << "  " << name << ": " << age << std::endl;
        std::cout << "→ 上面「所有人」那段的順序未由標準規定,不可寫進測試斷言"
                  << std::endl;
    }

    // ── 觀察 1:operator[] 會「偷偷插入」 ─────────────────────────────────
    std::cout << "\n=== operator[] 的插入副作用 ===" << std::endl;
    std::cout << "查詢前 size() = " << ages.size() << std::endl;
    int nobody = ages["Nobody"];        // ← 只是想「查」,卻插入了一筆
    std::cout << "ages[\"Nobody\"] 回傳 " << nobody
              << ",但查詢後 size() = " << ages.size()
              << "  ← 容器被查詢行為撐大了!" << std::endl;
    ages.erase("Nobody");               // 清掉,免得影響後面的示範

    std::cout << "正確的純查詢寫法:" << std::endl;
    std::cout << "  find():     "
              << (ages.find("Nobody") == ages.end() ? "不存在" : "存在")
              << ",size() 仍為 " << ages.size() << std::endl;
    try {
        (void)ages.at("Nobody");
    } catch (const std::out_of_range& e) {
        std::cout << "  at():       捕捉到 std::out_of_range,size() 仍為 "
                  << ages.size() << std::endl;
    }
    std::cout << "  contains(): C++20 起可用,語意最清楚(本檔以 C++17 編譯)"
              << std::endl;

    // ── 觀察 2:四種插入介面的差異 ────────────────────────────────────────
    std::cout << "\n=== insert / [] / insert_or_assign / try_emplace ===" << std::endl;
    {
        std::unordered_map<std::string, int> m;
        m["x"] = 1;
        std::cout << "  起始: x = " << m.at("x") << std::endl;

        auto r1 = m.insert({"x", 100});
        std::cout << "  insert({x,100})          → inserted=" << std::boolalpha
                  << r1.second << ", x = " << m.at("x") << "  (已存在,不覆蓋)"
                  << std::endl;

        auto r2 = m.try_emplace("x", 200);
        std::cout << "  try_emplace(x, 200)      → inserted=" << r2.second
                  << ", x = " << m.at("x") << "  (已存在,不建構也不覆蓋)"
                  << std::endl;

        auto r3 = m.insert_or_assign("x", 300);
        std::cout << "  insert_or_assign(x, 300) → inserted=" << r3.second
                  << ", x = " << m.at("x") << "  (已存在 → 覆蓋,inserted=false)"
                  << std::endl;

        auto r4 = m.insert_or_assign("y", 400);
        std::cout << "  insert_or_assign(y, 400) → inserted=" << r4.second
                  << ", y = " << m.at("y") << "  (不存在 → 新增,inserted=true)"
                  << std::endl;
    }

    // ── 觀察 3:rehash 後 iterator 失效,但 reference 仍有效 ───────────────
    std::cout << "\n=== rehash 的失效規則(與 vector 相反) ===" << std::endl;
    {
        std::unordered_map<std::string, int> cache;
        cache["session-42"] = 7;
        int* addrBefore = &cache["session-42"];      // 指向 value 的指標
        std::size_t bucketsBefore = cache.bucket_count();

        for (int i = 0; i < 500; ++i)                // 大量插入 → 多次 rehash
            cache["k" + std::to_string(i)] = i;

        int* addrAfter = &cache["session-42"];
        std::cout << "  rehash 前 bucket_count = " << bucketsBefore
                  << ",rehash 後 = " << cache.bucket_count() << std::endl;
        std::cout << "  session-42 的 value 位址前後相同嗎? "
                  << (addrBefore == addrAfter) << std::endl;
        *addrBefore = 999;                           // 透過舊指標寫入,合法
        std::cout << "  透過 rehash 前取得的指標寫入 999 → 讀回 "
                  << cache.at("session-42") << " (仍然有效)" << std::endl;
        std::cout << "  → 節點沒有搬家,reference/pointer 有效;iterator 則已失效。"
                  << std::endl;
        std::cout << "    換成 vector 的話,擴容後三者全部失效 —— 方向剛好相反。"
                  << std::endl;
    }

    // ── 觀察 4:key 是 const,不能就地修改 ────────────────────────────────
    std::cout << "\n=== value_type 是 pair<const Key, T> ===" << std::endl;
    {
        std::unordered_map<std::string, int> m = {{"a", 1}, {"b", 2}};
        for (auto& [k, v] : m) {
            (void)k;        // k 的型別是 const std::string&,不能賦值
            v *= 10;        // second 可以改
        }
        std::vector<std::pair<std::string, int>> sorted(m.begin(), m.end());
        std::sort(sorted.begin(), sorted.end());
        std::cout << "  把所有 value 乘 10: ";
        for (const auto& [k, v] : sorted) std::cout << k << "=" << v << " ";
        std::cout << std::endl;
        std::cout << "  key 帶 const 是刻意設計:key 決定桶位置,"
                  << "就地改 key 會讓元素留在錯誤的桶而永遠找不到。" << std::endl;
    }

    // ── 觀察 5:記憶體佈局 ────────────────────────────────────────────────
    std::cout << "\n=== 記憶體佈局(實作定義) ===" << std::endl;
    std::cout << "sizeof(std::unordered_map<int,int>) = "
              << sizeof(std::unordered_map<int, int>) << std::endl;
    std::cout << "sizeof(std::map<int,int>)           = "
              << sizeof(std::map<int, int>) << std::endl;
    std::cout << "→ 兩者都只存骨架,元素在各自的 heap 節點裡;"
              << "數值屬實作定義" << std::endl;

    // ── LeetCode 1 ────────────────────────────────────────────────────────
    std::cout << "\n=== LeetCode 1. Two Sum ===" << std::endl;
    {
        std::vector<int> nums = {2, 7, 11, 15};
        std::vector<int> ans = twoSum(nums, 9);
        std::cout << "twoSum({2,7,11,15}, 9)   = [" << ans[0] << ", " << ans[1]
                  << "]  (nums[0]+nums[1] = 2+7 = 9)" << std::endl;

        std::vector<int> nums2 = {3, 2, 4};
        std::vector<int> ans2 = twoSum(nums2, 6);
        std::cout << "twoSum({3,2,4}, 6)       = [" << ans2[0] << ", " << ans2[1]
                  << "]  (nums[1]+nums[2] = 2+4 = 6)" << std::endl;
    }

    // ── LeetCode 49 ───────────────────────────────────────────────────────
    std::cout << "\n=== LeetCode 49. Group Anagrams ===" << std::endl;
    {
        std::vector<std::string> strs = {"eat", "tea", "tan", "ate", "nat", "bat"};
        auto groups = groupAnagrams(strs);
        std::cout << "輸入: eat tea tan ate nat bat" << std::endl;
        std::cout << "分成 " << groups.size() << " 組(已排序,確保輸出可重現):"
                  << std::endl;
        for (const auto& g : groups) {
            std::cout << "  [ ";
            for (const std::string& s : g) std::cout << s << " ";
            std::cout << "]" << std::endl;
        }
    }

    // ── 日常實務 1:log 統計 ──────────────────────────────────────────────
    std::cout << "\n=== 日常實務 1: 伺服器 log 統計 ===" << std::endl;
    {
        std::vector<LogEntry> log = {
            {"/api/v1/users",  200}, {"/api/v1/users",  200},
            {"/api/v1/orders", 200}, {"/login",         401},
            {"/login",         401}, {"/login",         401},
            {"/api/v1/orders", 500}, {"/admin",         403},
            {"/api/v1/users",  200}, {"/api/v1/orders", 500}
        };

        auto counter = countByStatus(log);

        // 報表要給人看 → 排序後輸出(unordered_map 走訪順序未由標準規定)
        std::vector<std::pair<int, std::size_t>> rows(counter.begin(), counter.end());
        std::sort(rows.begin(), rows.end());
        std::cout << "各狀態碼次數(依狀態碼排序):" << std::endl;
        for (const auto& [code, n] : rows)
            std::cout << "  " << code << " → " << n << " 次" << std::endl;

        std::cout << "錯誤最多的端點: " << worstEndpoint(log) << std::endl;
    }

    // ── 日常實務 2:設定檔優先序 ──────────────────────────────────────────
    std::cout << "\n=== 日常實務 2: 設定檔優先序(預設 < 檔案 < 環境變數) ===" << std::endl;
    {
        Config cfg;

        // (1) 設定檔讀進來的值
        cfg["log.level"] = "info";
        cfg["db.host"]   = "db.internal";
        std::cout << "  讀入設定檔後:" << std::endl;
        dumpConfig(cfg);

        // (2) 補上預設值 —— try_emplace:已存在就不動,不覆蓋使用者的設定
        std::cout << "  套用預設值(try_emplace,不覆蓋既有值):" << std::endl;
        applyDefaults(cfg, {
            {"log.level", "warn"},      // 已存在 → 保持 info
            {"db.port",   "5432"},      // 不存在 → 補上
            {"db.pool",   "10"}         // 不存在 → 補上
        });
        dumpConfig(cfg);

        // (3) 環境變數覆蓋 —— insert_or_assign:有就蓋、沒有就加
        std::cout << "  套用環境變數(insert_or_assign,覆蓋):" << std::endl;
        applyOverrides(cfg, {
            {"log.level", "debug"},     // 已存在 → 覆蓋成 debug
            {"db.ssl",    "true"}       // 不存在 → 新增
        });
        std::cout << "  最終設定:" << std::endl;
        dumpConfig(cfg);
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第六課：容器（Container）的概念與分類11.cpp" -o unordered_map_demo

// 注意：unordered 容器的走訪順序未由標準規定,不同實作／不同執行可能不同。
//       下方「所有人」那段的順序、以及各項 bucket_count 數值,皆為本機
//       (GCC 15.2.0 / libstdc++ / x86-64)的實測結果,屬實作定義,
//       不可作為跨平台的保證。其餘段落已刻意排序,故輸出穩定可重現。

// === 預期輸出 ===
// === std::unordered_map ===
// 所有人:
//   Diana: 28
//   Charlie: 35
//   Bob: 30
//   Alice: 25
// Bob 的年齡: 30
//
// === 要穩定輸出就先排序 ===
//   Alice: 25
//   Bob: 30
//   Charlie: 35
//   Diana: 28
// → 上面「所有人」那段的順序未由標準規定,不可寫進測試斷言
//
// === operator[] 的插入副作用 ===
// 查詢前 size() = 4
// ages["Nobody"] 回傳 0,但查詢後 size() = 5  ← 容器被查詢行為撐大了!
// 正確的純查詢寫法:
//   find():     不存在,size() 仍為 4
//   at():       捕捉到 std::out_of_range,size() 仍為 4
//   contains(): C++20 起可用,語意最清楚(本檔以 C++17 編譯)
//
// === insert / [] / insert_or_assign / try_emplace ===
//   起始: x = 1
//   insert({x,100})          → inserted=false, x = 1  (已存在,不覆蓋)
//   try_emplace(x, 200)      → inserted=false, x = 1  (已存在,不建構也不覆蓋)
//   insert_or_assign(x, 300) → inserted=false, x = 300  (已存在 → 覆蓋,inserted=false)
//   insert_or_assign(y, 400) → inserted=true, y = 400  (不存在 → 新增,inserted=true)
//
// === rehash 的失效規則(與 vector 相反) ===
//   rehash 前 bucket_count = 13,rehash 後 = 541
//   session-42 的 value 位址前後相同嗎? true
//   透過 rehash 前取得的指標寫入 999 → 讀回 999 (仍然有效)
//   → 節點沒有搬家,reference/pointer 有效;iterator 則已失效。
//     換成 vector 的話,擴容後三者全部失效 —— 方向剛好相反。
//
// === value_type 是 pair<const Key, T> ===
//   把所有 value 乘 10: a=10 b=20
//   key 帶 const 是刻意設計:key 決定桶位置,就地改 key 會讓元素留在錯誤的桶而永遠找不到。
//
// === 記憶體佈局(實作定義) ===
// sizeof(std::unordered_map<int,int>) = 56
// sizeof(std::map<int,int>)           = 48
// → 兩者都只存骨架,元素在各自的 heap 節點裡;數值屬實作定義
//
// === LeetCode 1. Two Sum ===
// twoSum({2,7,11,15}, 9)   = [0, 1]  (nums[0]+nums[1] = 2+7 = 9)
// twoSum({3,2,4}, 6)       = [1, 2]  (nums[1]+nums[2] = 2+4 = 6)
//
// === LeetCode 49. Group Anagrams ===
// 輸入: eat tea tan ate nat bat
// 分成 3 組(已排序,確保輸出可重現):
//   [ ate eat tea ]
//   [ bat ]
//   [ nat tan ]
//
// === 日常實務 1: 伺服器 log 統計 ===
// 各狀態碼次數(依狀態碼排序):
//   200 → 4 次
//   401 → 3 次
//   403 → 1 次
//   500 → 2 次
// 錯誤最多的端點: /login (3 次錯誤)
//
// === 日常實務 2: 設定檔優先序(預設 < 檔案 < 環境變數) ===
//   讀入設定檔後:
//     db.host = db.internal
//     log.level = info
//   套用預設值(try_emplace,不覆蓋既有值):
//     db.host = db.internal
//     db.pool = 10
//     db.port = 5432
//     log.level = info
//   套用環境變數(insert_or_assign,覆蓋):
//   覆蓋 log.level = debug
//   新增 db.ssl = true
//   最終設定:
//     db.host = db.internal
//     db.pool = 10
//     db.port = 5432
//     db.ssl = true
//     log.level = debug
