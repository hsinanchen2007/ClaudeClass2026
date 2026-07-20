// =============================================================================
//  第 18 課-10：替代方案一 —— deque<bool> 沒有被特化，行為完全正常
// =============================================================================
//
// 【主題資訊 Information】
//   std::deque<bool>::operator[]  →  bool&        （真正的引用，不是代理）
//   &db[i]                        →  bool*        （真正的指標）
//   對照：std::vector<bool> 是標準規定的「特化」，operator[] 回傳代理物件
//   標準版本：deque 自 C++98 起即有，且從未被針對 bool 特化
//   記憶體：每個 bool 佔 sizeof(bool) = 1 byte（本機實測），
//           不做位元壓縮，因此比 vector<bool> 多用約 8 倍
//   標頭檔：<deque>
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼 deque<bool> 沒有這個問題】
//   答案很簡單也很重要：標準只對 vector<bool> 做了特化，
//   對 deque、list、set、map 通通沒有。
//   所以 deque<bool> 就是「一個裝著 bool 的普通容器」——
//   每個元素是一個貨真價實的 bool 物件，佔一個 byte，有自己的位址。
//   於是 bool&、bool*、傳給吃 bool& 的函式、泛型模板，全部正常運作。
//   換句話說：vector<bool> 的五大陷阱在這裡一個都不存在。
//
// 【2. 這說明了 vector<bool> 的問題不在「bool」而在「特化」】
//   很多人以為「bool 就是不好放進容器」，這是誤解。
//   真正的問題是標準對 vector<bool> 做了一個
//   「改變介面語意」的特化，而不是「只改變實作」的特化。
//   同樣是裝 bool，deque<bool>、list<bool>、array<bool, N>
//   全部行為正常——差別只在有沒有被特化。
//
// 【3. 代價：記憶體多 8 倍，而且不連續】
//   deque<bool> 每個元素 1 byte，vector<bool> 每個 1 bit。
//   十萬個旗標的差距是約 100 KB 對上約 12.5 KB。
//   而且 deque 的元素分段配置（見同課 11.cpp），
//   所以它同樣沒有 data()——不能整塊交給 C API。
//   若你需要的是「與 C API 互通」，deque<bool> 幫不上忙，
//   該選的是 vector<uint8_t>（見 12.cpp）。
//
// 【4. 那 deque<bool> 到底什麼時候該用】
//   當你同時需要這兩件事時：
//     (a) 真正的 bool& / bool*（例如要傳給既有的 bool& 介面）；
//     (b) 頻繁在「頭尾兩端」插入刪除（這是 deque 相對 vector 的強項）。
//   如果只需要 (a)，vector<uint8_t> 或 vector<char> 更單純，
//   而且記憶體連續、有 data()、快取友善。
//   實務上 deque<bool> 是個「可用但很少是最佳解」的選項，
//   它在本課的價值主要是教學上的對照組。
//
// 【5. 還有一個常被忽略的優勢：reference 穩定性】
//   deque 在「頭尾插入」時不會使既有元素的 reference 與 pointer 失效
//   （迭代器仍會失效，這兩者在 deque 上的規則不同）。
//   vector 則是一旦重新配置，迭代器/指標/引用全數作廢。
//   若你需要長期持有指向元素的指標，deque 的這個性質很有價值。
//
// 【概念補充 Concept Deep Dive】
//   ▸ 為什麼標準當初要特化 vector<bool>
//     C++98 制定時，記憶體遠比現在昂貴，
//     「一個 bool 佔一個 byte 是八倍的浪費」是很有說服力的論點。
//     委員會希望提供一個開箱即用的位元陣列，
//     卻選擇把它塞進 vector<bool> 而不是另立名字。
//     後來公認的結論是：實作沒錯，錯在名字——
//     它應該叫 bit_vector，就不會有人期待它滿足 vector 的泛型契約。
//   ▸ std::array<bool, N> 也完全正常
//     array 同樣沒有被特化，array<bool, 100> 就是一個真正的
//     bool[100]，sizeof 是 100，data() 可用。
//     若大小固定且需要與 C 互通，它比 vector<bool> 適合得多。
//   ▸ 為什麼 list<bool> 也沒問題
//     同理，list 沒有被特化。而且 list 的節點本來就各自配置，
//     壓縮成 bit 對它毫無意義（每個節點的兩根指標就佔 16 bytes 了）。
//
// 【注意事項 Pay Attention】
//   1. deque<bool> 沒有被特化，bool& 與 bool* 都完全正常。
//   2. 但它每個元素佔 1 byte，比 vector<bool> 多用約 8 倍記憶體。
//   3. deque 的元素不連續，同樣沒有 data()，無法整塊交給 C API。
//   4. 要與 C API 互通請改用 vector<uint8_t> 或 std::array<bool, N>。
//   5. deque 頭尾插入不會使 reference/pointer 失效（迭代器仍會失效）。
//   6. deque<bool> 的中間插入/刪除是 O(n)，別把它當萬用解。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】deque<bool> 與 vector<bool> 的對比
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 deque<bool> 可以取得 bool&，vector<bool> 卻不行？
//     答：因為標準只對 vector<bool> 做了特化，deque 從未被特化。
//         deque<bool> 的每個元素就是一個真正的 bool 物件，
//         佔一個 byte、有自己的位址，operator[] 自然回傳 bool&。
//         vector<bool> 則把元素壓成 1 個 bit，
//         而單一 bit 沒有位址，只能回傳代理物件。
//     追問：那 list<bool>、array<bool, N> 呢？
//         → 同樣完全正常，它們都沒有被特化。
//           這說明問題的根源是「那個特化」，不是「bool 這個型別」。
//
// 🔥 Q2. 既然 deque<bool> 行為正常，那是不是該一律用它取代 vector<bool>？
//     答：不一定。deque<bool> 每個元素佔 1 byte，記憶體是 vector<bool>
//         的約 8 倍；而且 deque 分段配置、元素不連續，
//         同樣沒有 data()，一樣不能整塊交給 C API。
//         若只是需要真正的引用與指標，vector<uint8_t> 更單純——
//         記憶體連續、有 data()、快取更友善。
//         deque<bool> 真正適合的是「同時需要真引用 + 頻繁頭尾插刪」。
//     追問：deque 相對 vector 還有什麼獨特性質？
//         → 頭尾插入不會使既有元素的 reference 與 pointer 失效
//           （迭代器仍然會失效，這兩者的規則在 deque 上是分開的）。
//           需要長期持有元素指標時這很有價值。
//
// ⚠️ 陷阱. 「deque 沒有被特化、行為正常，那它應該也有 data() 吧？」
//     答：沒有。deque 從來就沒有 data()——但理由和 vector<bool> 完全不同。
//         vector<bool> 是因為「元素是 bit、沒有位址」；
//         deque 是因為「元素分段配置在多塊記憶體上，本來就不連續」。
//         同樣是缺 data()，背後是兩個完全不同的原因。
//     為什麼會錯：把「行為正常」誤解成「和 vector 完全一樣」。
//         deque 與 vector 的差異遠不只 bool 這一項：
//         記憶體佈局、失效規則、中間插入的成本都不同。
//         唯一能保證連續記憶體的標準容器是 vector、array 與 string。
// ═══════════════════════════════════════════════════════════════════════════

#include <deque>
#include <iostream>
#include <string>
#include <vector>

// 既有模組的介面：只接受 bool&（這種介面在真實程式碼中很常見）
void markProcessed(bool& flag) {
    flag = true;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】滑動視窗的「是否命中」紀錄
//   情境：監控系統維護最近 N 次健康檢查的結果（成功/失敗），
//         每來一筆新結果就從尾端加入、從頭端丟掉最舊的一筆，
//         同時要把某些筆標記為「已處理」——而標記函式的介面是 bool&。
//   為什麼用 deque<bool>：
//         (a) 頭尾進出正是 deque 的強項（vector 從頭端刪除是 O(n)）；
//         (b) 既有的 markProcessed(bool&) 介面需要真正的引用，
//             vector<bool> 的代理物件在這裡直接編譯失敗。
//         這兩個需求同時成立時，deque<bool> 才是恰當的選擇。
// -----------------------------------------------------------------------------
class HealthWindow {
    std::deque<bool> results_;      // true = 健康，false = 異常
    std::size_t      capacity_;
public:
    explicit HealthWindow(std::size_t cap) : capacity_(cap) {}

    void record(bool healthy) {
        results_.push_back(healthy);            // 尾端加入：O(1)
        if (results_.size() > capacity_) {
            results_.pop_front();               // 頭端丟棄：O(1)（vector 是 O(n)）
        }
    }

    // 因為 deque<bool> 給的是真正的 bool&，可以直接傳給既有介面
    void markAllProcessed() {
        for (bool& r : results_) {              // for (bool& ...) 對 vector<bool> 編不過
            markProcessed(r);
        }
    }

    std::size_t failures() const {
        std::size_t c = 0;
        for (bool r : results_) if (!r) ++c;
        return c;
    }
    std::size_t size() const { return results_.size(); }
};

int main() {
    std::cout << std::boolalpha;

    std::cout << "=== 一、deque<bool> 的引用完全正常 ===" << std::endl;
    std::deque<bool> db = {true, false, true};

    bool& ref = db[0];      // 真正的 bool& —— 對 vector<bool> 這行會編譯失敗
    ref = false;
    std::cout << "透過 bool& 修改後 db[0] = " << db[0] << std::endl;

    std::cout << "\n=== 二、deque<bool> 可以取元素位址 ===" << std::endl;
    bool* ptr = &db[1];     // 真正的 bool*
    std::cout << "*ptr = " << *ptr << std::endl;
    std::cout << "&db[1] 的型別確實是 bool*（vector<bool> 則是 reference*）" << std::endl;

    std::cout << "\n=== 三、可以直接傳給吃 bool& 的既有函式 ===" << std::endl;
    markProcessed(db[2]);   // 對 vector<bool> 這行會編譯失敗
    std::cout << "markProcessed(db[2]) 之後 db[2] = " << db[2] << std::endl;

    std::cout << "\n=== 四、範圍 for 取引用也沒問題 ===" << std::endl;
    for (bool& b : db) {    // 對 vector<bool> 要寫成 auto&&
        b = !b;
    }
    std::cout << "全部翻轉後 db = ";
    for (bool b : db) std::cout << (b ? 1 : 0);
    std::cout << std::endl;

    std::cout << "\n=== 五、代價：記憶體多約 8 倍 ===" << std::endl;
    const std::size_t N = 100000;
    std::vector<bool> vb(N, true);
    std::cout << N << " 個旗標的粗略記憶體量級：" << std::endl;
    std::cout << "  vector<bool>  約 " << (vb.capacity() / 8)
              << " bytes（1 bit/元素，位元壓縮）" << std::endl;
    std::cout << "  deque<bool>   約 " << (N * sizeof(bool))
              << " bytes（1 byte/元素，另有分段管理的額外開銷）" << std::endl;

    std::cout << "\n=== 六、但 deque 同樣沒有 data()（原因不同）===" << std::endl;
    // bool* p = db.data();   // 編譯錯誤！deque 從來就沒有 data()
    std::cout << "vector<bool> 沒有 data()：因為元素是 bit，沒有位址" << std::endl;
    std::cout << "deque<bool>  沒有 data()：因為元素分段配置，本來就不連續" << std::endl;
    std::cout << "要與 C API 互通，兩者都不行——該用 vector<uint8_t>" << std::endl;

    std::cout << "\n=== 七、日常實務：健康檢查滑動視窗 ===" << std::endl;
    HealthWindow win(5);
    bool samples[] = {true, true, false, true, false, false, true, true};
    for (bool s : samples) {
        win.record(s);
    }
    std::cout << "最近 " << win.size() << " 次檢查中，異常 "
              << win.failures() << " 次" << std::endl;
    win.markAllProcessed();
    std::cout << "markAllProcessed() 後，異常數 = " << win.failures()
              << "（全部被標記為已處理=true）" << std::endl;
    std::cout << "註：markAllProcessed 用的是 for (bool& r : results_)，"
              << "這行對 vector<bool> 會直接編譯失敗" << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 18 課：vectorbool 的特殊性與陷阱10.cpp" -o deque_bool
//
// 【關於下方預期輸出的但書】
//   記憶體量級的數字僅供比較：vector<bool> 的 capacity()/8 是約略值
//   （實際以字組為單位對齊向上取整）；deque 的 N * sizeof(bool)
//   也未計入分段配置的中央索引表與各區塊的額外開銷。
//   sizeof(bool) = 1 為本機（x86-64 / GCC 15.2）實測值，
//   標準只保證它至少為 1，屬實作定義。
//
// 【本檔未附 LeetCode 範例的理由】
//   本檔的主題是「同一個元素型別在不同容器上的介面差異」，
//   屬於標準庫設計與容器選型議題。LeetCode 不考容器選型
//   （題目通常直接給定 vector），硬套一題無法呈現重點，
//   因此改以健康檢查滑動視窗這個「同時需要真引用與頭尾進出」
//   的實務場景，說明 deque<bool> 真正合適的使用時機。

// === 預期輸出 ===
// === 一、deque<bool> 的引用完全正常 ===
// 透過 bool& 修改後 db[0] = false
//
// === 二、deque<bool> 可以取元素位址 ===
// *ptr = false
// &db[1] 的型別確實是 bool*（vector<bool> 則是 reference*）
//
// === 三、可以直接傳給吃 bool& 的既有函式 ===
// markProcessed(db[2]) 之後 db[2] = true
//
// === 四、範圍 for 取引用也沒問題 ===
// 全部翻轉後 db = 110
//
// === 五、代價：記憶體多約 8 倍 ===
// 100000 個旗標的粗略記憶體量級：
//   vector<bool>  約 12504 bytes（1 bit/元素，位元壓縮）
//   deque<bool>   約 100000 bytes（1 byte/元素，另有分段管理的額外開銷）
//
// === 六、但 deque 同樣沒有 data()（原因不同）===
// vector<bool> 沒有 data()：因為元素是 bit，沒有位址
// deque<bool>  沒有 data()：因為元素分段配置，本來就不連續
// 要與 C API 互通，兩者都不行——該用 vector<uint8_t>
//
// === 七、日常實務：健康檢查滑動視窗 ===
// 最近 5 次檢查中，異常 2 次
// markAllProcessed() 後，異常數 = 0（全部被標記為已處理=true）
// 註：markAllProcessed 用的是 for (bool& r : results_)，這行對 vector<bool> 會直接編譯失敗
