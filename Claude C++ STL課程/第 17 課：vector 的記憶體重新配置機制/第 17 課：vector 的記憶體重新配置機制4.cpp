// =============================================================================
//  第 17 課：vector 的記憶體重新配置機制 4  —  重新配置造成的 iterator 失效
// =============================================================================
//
// 【主題資訊 Information】
//   void reserve(size_type new_cap);   // C++98；new_cap > max_size() 丟 std::length_error
//   void push_back(const T& value);    // C++98
//
//   標頭檔  ：<vector>
//   複雜度  ：reserve 為 O(n)（要搬移現有元素）；push_back 均攤 O(1)
//   失效規則（標準保證）：
//     * 若 push_back / insert / reserve / resize 造成【重新配置】
//       → 該 vector 的【所有】iterator、pointer、reference 全部失效（含 end()）
//     * 若沒有重新配置（capacity 夠）
//       → 只有「插入點（含）之後」的 iterator 失效；end() 一律失效
//
// 【詳細解釋 Explanation】
//
// 【1. 「失效」到底是什麼意思】
//   vector 的 iterator 在多數實作裡就是一根裸指標（或包了一層的指標）。
//   重新配置會做兩件事：把元素搬到新緩衝區、然後把舊緩衝區還給 allocator。
//   舊緩衝區一旦歸還，先前取得的 iterator 就指向一塊「已經不屬於你」的記憶體，
//   它變成一根 dangling pointer。
//   對這種 iterator 做解參考（*it）、遞增（++it）、甚至只是拿它跟別人比較，
//   都是【未定義行為 UB】。
//   UB 的意思是「標準不對結果作任何規定」——它可能印出舊值、印出垃圾、
//   當場崩潰，也可能被最佳化器假設「不會發生」而讓周圍程式碼行為改變。
//   絕對不能寫成「它一定會印出 X」或「它一定會 crash」。
//
// 【2. 為什麼「reserve(3) + 已有 3 個元素」保證下一次 push_back 會重新配置】
//   本檔先用 {10,20,30} 建構（size==3），再 reserve(3)。
//   reserve(n) 的規則是「若 n <= capacity()，什麼都不做」，
//   所以這行實際上沒有擴大容量，只是讓我們確定 capacity 至少是 3。
//   由於本機 libstdc++ 對 initializer_list 建構會配置剛好 3 格，
//   此時 size()==capacity()==3 → 下一次 push_back 必然重新配置。
//   （容量是否恰好等於 3 屬【實作定義】；本檔用輸出把它印出來驗證，
//     不靠假設。）
//
// 【3. 正確的因應方式：不要保存 iterator，改保存「索引」】
//   若一段程式碼中間可能插入元素，最穩健的做法是保存 index 而非 iterator：
//       size_t pos = 1;            // 索引不會因重新配置而失效
//       v.push_back(40);
//       std::cout << v[pos];       // 安全
//   索引之所以安全，是因為它描述的是「第幾個元素」這個邏輯位置，
//   與底層緩衝區的實體位址無關。iterator 描述的則是實體位址。
//   若真的要用 iterator，就在每次可能重新配置的操作後【重新取得】：
//       it = v.begin() + 1;
//
// 【4. insert 的回傳值：實作已經幫你把有效 iterator 交回來了】
//   insert / erase 都會回傳一個「重新取得後的有效 iterator」，
//   這是為了讓你在失效之後還能接著走。不接住回傳值而繼續用舊的，
//   就是第 6 檔那個經典錯誤。
//
// 【概念補充 Concept Deep Dive】
//   重新配置前後的記憶體示意（位址為示意值）：
//
//     重新配置前                    重新配置後
//     old buf @0x1000               new buf @0x2000
//     ┌────┬────┬────┐              ┌────┬────┬────┬────┬────┬────┐
//     │ 10 │ 20 │ 30 │              │ 10 │ 20 │ 30 │ 40 │ ?  │ ?  │
//     └────┴─▲──┴────┘              └────┴────┴────┴────┴────┴────┘
//            │                       (舊區塊 0x1000 已 operator delete)
//        it ─┘  仍然指著 0x1008
//               ↑ 這根指標現在指向「已歸還的記憶體」= dangling
//
//   關鍵：it 是一根「記住了舊位址」的指標，vector 沒有辦法通知它。
//   C++ 標準庫刻意不做「iterator 註冊 / 通知」機制，因為那需要為每個
//   iterator 維護連結串列，會讓 iterator 的成本從「一根指標」變成
//   「一個要維護的物件」，違背 STL「不為你沒用到的東西付費」的原則。
//   （libstdc++ 的 _GLIBCXX_DEBUG 模式正是用這種註冊機制來抓失效，
//     代價是效能大幅下降，所以只在除錯時開。）
//
// 【注意事項 Pay Attention】
//   1. 使用失效的 iterator 是【UB】。不要預期任何特定結果——
//      不會保證印出舊值，也不會保證崩潰。本檔因此【不示範】去解參考它，
//      只用註解標示危險位置。
//   2. end() 特別容易被忽略：任何插入操作之後 end() 都會失效，
//      所以「先存 auto e = v.end(); 然後在迴圈裡插入再跟 e 比較」是錯的。
//   3. reference 與 pointer 跟 iterator 一樣會失效。
//      `int& r = v[0];` 之後 push_back 造成重新配置，r 就懸空了。
//   4. 除錯時可用 g++ -D_GLIBCXX_DEBUG（libstdc++ 除錯模式）或
//      -fsanitize=address 讓失效存取比較容易被抓到；但「沒被抓到」
//      不代表程式正確——UB 可以毫無症狀。
//   5. 下方輸出中的 capacity 數字是本機 libstdc++ 的【實作定義】行為。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】iterator 失效（重新配置）
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. push_back 之後，先前取得的 iterator 還能用嗎？
//     答：分兩種情況。若這次 push_back 觸發了重新配置（size==capacity），
//         該 vector 的所有 iterator、pointer、reference 全部失效，
//         包含 end()；若沒有重新配置，只有 end() 失效，其餘仍有效。
//         由於呼叫端通常無法預知會不會重新配置，實務上一律視為「全部失效」。
//     追問：那要怎麼寫才安全？→ 保存「索引」而不是 iterator；
//         或在每次插入後用 v.begin() + pos 重新取得。
//
// 🔥 Q2. 如何「保證」接下來的 push_back 不會使 iterator 失效？
//     答：事先 reserve 足夠容量，讓 size() < capacity() 恆成立。
//         只要不重新配置，指向既有元素的 iterator 就不會因尾端插入而失效
//         （但 end() 仍會失效）。
//     追問：reserve 本身會不會使 iterator 失效？
//         → 會，如果它真的擴大了容量（發生重新配置）。
//           若 n <= capacity()，reserve 什麼都不做，就不會失效。
//
// ⚠️ 陷阱. 「用了失效的 iterator，程式一定會 segfault，所以測試跑得過就沒問題」——錯在哪？
//     答：那是【未定義行為】，標準不保證任何結果。舊記憶體可能還沒被
//         作業系統收回，讀起來「看似正常」；也可能被 allocator 配給別人
//         而讀到別的資料。測試通過完全不能證明程式正確。
//     為什麼會錯：把 UB 想成「一種會被抓到的錯誤」。UB 的本質是
//         「編譯器可以假設它不發生」，最危險的情況正是它安靜地不出事，
//         直到換編譯器、換最佳化等級或上了正式環境才爆。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <string>

// -----------------------------------------------------------------------------
// 【日常實務範例】訂單處理：為什麼快取「指向 vector 元素的指標」是個地雷
//   情境：訂單系統把每筆訂單放進 vector，另外用一個「熱門訂單」清單
//         保存指標以便快速存取。只要主清單後來又 push_back 觸發擴容，
//         那些指標就全部懸空——而且通常不會當場出事，
//         而是幾天後在正式環境讀到亂資料。
//   正解：改存「索引」，或改用 std::deque（尾端插入不會使既有元素失效）、
//         或存 std::shared_ptr<Order> 讓物件本身不隨 vector 搬家。
// -----------------------------------------------------------------------------
struct Order {
    std::string id;
    long        amount;
};

// 安全版本：回傳索引，不回傳指標／iterator
std::vector<size_t> find_big_order_indices(const std::vector<Order>& orders, long threshold) {
    std::vector<size_t> hits;
    for (size_t i = 0; i < orders.size(); ++i) {
        if (orders[i].amount >= threshold) hits.push_back(i);
    }
    return hits;
}

int main() {
    std::cout << "=== 一、重新配置使 iterator 失效 ===" << std::endl;

    std::vector<int> v = {10, 20, 30};
    v.reserve(3);  // n <= capacity()，實際上不做事；只是宣告我們要的下限

    std::cout << "size=" << v.size() << ", capacity=" << v.capacity()
              << "（size==capacity ⇒ 下次 push_back 必定重新配置）" << std::endl;

    auto it = v.begin() + 1;  // 指向 20
    std::cout << "push_back 前： *it = " << *it << std::endl;

    const int* before = v.data();
    v.push_back(40);          // 觸發重新配置
    const int* after  = v.data();

    std::cout << "push_back 後： capacity=" << v.capacity()
              << "，緩衝區位址是否改變 = " << (before != after ? "是" : "否")
              << std::endl;

    // ⚠️ 絕對不可以在這裡寫 std::cout << *it;
    //    it 指向已歸還的舊緩衝區，那是未定義行為（UB）。
    //    UB 沒有「保證的結果」：可能印出舊值、可能印出垃圾、可能崩潰，
    //    也可能被最佳化器假設不會發生而讓周圍程式碼行為改變。
    //    本檔刻意不示範去讀它。

    // 正確做法 A：重新取得 iterator
    it = v.begin() + 1;
    std::cout << "重新取得 iterator 後： *it = " << *it << std::endl;

    std::cout << "\n=== 二、改用「索引」就完全不受重新配置影響 ===" << std::endl;
    std::vector<int> w = {10, 20, 30};
    const size_t pos = 1;                       // 索引描述的是邏輯位置
    std::cout << "push_back 前： w[" << pos << "] = " << w[pos] << std::endl;
    for (int i = 0; i < 20; ++i) w.push_back(100 + i);   // 期間必然多次重新配置
    std::cout << "大量 push_back 後： w[" << pos << "] = " << w[pos]
              << "（索引全程有效）, capacity=" << w.capacity() << std::endl;

    std::cout << "\n=== 三、事先 reserve 就不會重新配置 ===" << std::endl;
    std::vector<int> r = {1, 2, 3};
    r.reserve(100);                              // 這一次 reserve 真的擴容了
    const int* base = r.data();
    auto it2 = r.begin() + 1;
    std::cout << "reserve(100) 後 capacity=" << r.capacity() << std::endl;
    for (int i = 0; i < 50; ++i) r.push_back(i); // 容量夠，不會重新配置
    std::cout << "push_back 50 次後：緩衝區位址是否改變 = "
              << (base != r.data() ? "是" : "否")
              << "，*it2 = " << *it2
              << "（未重新配置 ⇒ 指向既有元素的 iterator 仍有效）" << std::endl;
    std::cout << "注意：即使沒有重新配置，end() 仍然會失效，不可事先存起來重複用。"
              << std::endl;

    std::cout << "\n=== 四、日常實務：訂單清單用「索引」而非指標 ===" << std::endl;
    std::vector<Order> orders = {
        {"A-1001", 1200}, {"A-1002", 80}, {"A-1003", 5400}
    };
    std::vector<size_t> big = find_big_order_indices(orders, 1000);

    std::cout << "擴容前的大額訂單：";
    for (size_t i : big) std::cout << orders[i].id << "(" << orders[i].amount << ") ";
    std::cout << std::endl;

    // 之後又進來很多訂單，必然觸發重新配置
    for (int i = 0; i < 50; ++i) orders.push_back({"B-" + std::to_string(i), 10});

    std::cout << "擴容後（capacity=" << orders.capacity() << "）用同一組索引查詢：";
    for (size_t i : big) std::cout << orders[i].id << "(" << orders[i].amount << ") ";
    std::cout << std::endl;
    std::cout << "→ 索引不受重新配置影響；若當初存的是 Order*，此刻已全部懸空。"
              << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 17 課：vector 的記憶體重新配置機制4.cpp" -o iter_invalidate

// 註：下方 capacity 數字為本機 GCC 15.2 / libstdc++ 的【實作定義】結果；
//     「緩衝區位址是否改變」只印是／否，不印位址本身（位址每次執行都不同）。

// === 預期輸出 ===
// === 一、重新配置使 iterator 失效 ===
// size=3, capacity=3（size==capacity ⇒ 下次 push_back 必定重新配置）
// push_back 前： *it = 20
// push_back 後： capacity=6，緩衝區位址是否改變 = 是
// 重新取得 iterator 後： *it = 20
//
// === 二、改用「索引」就完全不受重新配置影響 ===
// push_back 前： w[1] = 20
// 大量 push_back 後： w[1] = 20（索引全程有效）, capacity=24
//
// === 三、事先 reserve 就不會重新配置 ===
// reserve(100) 後 capacity=100
// push_back 50 次後：緩衝區位址是否改變 = 否，*it2 = 2（未重新配置 ⇒ 指向既有元素的 iterator 仍有效）
// 注意：即使沒有重新配置，end() 仍然會失效，不可事先存起來重複用。
//
// === 四、日常實務：訂單清單用「索引」而非指標 ===
// 擴容前的大額訂單：A-1001(1200) A-1003(5400)
// 擴容後（capacity=96）用同一組索引查詢：A-1001(1200) A-1003(5400)
// → 索引不受重新配置影響；若當初存的是 Order*，此刻已全部懸空。
