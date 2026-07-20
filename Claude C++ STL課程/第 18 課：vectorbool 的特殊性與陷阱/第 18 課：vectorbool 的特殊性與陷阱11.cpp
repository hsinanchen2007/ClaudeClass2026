// =============================================================================
//  第 18 課-11：deque 的分段配置 —— 為什麼它給得起 bool&，卻給不起 data()
// =============================================================================
//
// 【主題資訊 Information】
//   std::deque<T>::operator[] → T&      （真正的引用；deque 從未被特化）
//   std::deque<T>             → 沒有 data()（元素分段配置，不連續）
//   失效規則（deque，與 vector 差異很大）：
//     頭尾 push/pop  → 所有 iterator 失效，但 reference / pointer 仍然有效
//     中間 insert/erase → iterator / reference / pointer 全部失效
//   標準版本：C++98 起
//   複雜度：頭尾插入刪除 O(1)；隨機存取 O(1)（但常數比 vector 大）
//   標頭檔：<deque>
//
// 【詳細解釋 Explanation】
//
// 【1. deque 的記憶體模型：一張索引表 + 許多固定大小的區塊】
//   vector 是「一整塊連續記憶體」，deque 則是
//   「一個指標陣列（map / 中央索引表），每根指標指向一個固定大小的區塊」。
//   本機（x86-64 / GCC 15.2 / libstdc++）實測每個區塊為 512 bytes：
//   對 deque<char> 而言，恰好每 512 個元素就換到下一塊記憶體。
//   （這個 512 是實作定義的，libc++ 與 MSVC 的策略都不同。）
//
//   概念圖：
//       中央索引表: [ ptr0 ][ ptr1 ][ ptr2 ] ...
//                      ↓        ↓        ↓
//       區塊:      [512B]   [512B]   [512B]
//
// 【2. 這個佈局同時解釋了 deque 的三個特性】
//   (a) 為什麼 operator[] 回傳真正的 T&：
//       每個元素是一個實實在在的 T 物件，住在某個區塊裡，有自己的位址。
//       這和 vector<bool> 把元素壓成 bit 是完全相反的取捨。
//   (b) 為什麼沒有 data()：
//       元素分散在多個不相鄰的區塊上，不存在一段涵蓋全部元素的連續記憶體，
//       所以回傳不了一根可以用 ptr[i] 走完全部元素的指標。
//   (c) 為什麼頭尾插入是 O(1) 且不使 reference 失效：
//       頭尾插入只需要在索引表的兩端接上新區塊，
//       既有區塊完全不動——所以既有元素的位址不變。
//
// 【3. deque 的失效規則和 vector 完全不同，這點最常被搞混】
//   vector：只要重新配置，iterator / reference / pointer 一起失效。
//   deque ：頭尾 push/pop 會使所有 iterator 失效
//           （因為迭代器內部要記住「哪一塊、塊內第幾個」，索引表可能重配），
//           但 reference 與 pointer 仍然有效（元素本身沒被搬動）。
//   這是 deque 一個很有價值卻少被利用的性質：
//   當你需要長期持有指向某個元素的指標，又要頻繁在頭尾增刪時，
//   deque 是標準容器中少數能同時滿足的。
//
// 【4. 隨機存取是 O(1)，但常數比 vector 大】
//   deque 的 operator[] 要先算出「在第幾塊」與「塊內偏移」，
//   再做兩次解參考（先取區塊指標，再取元素）。
//   vector 只要一次位址算術。
//   兩者都是 O(1)，但在密集走訪的迴圈中差距是看得出來的，
//   而且 deque 的元素不連續，對 CPU 快取預取也比較不友善。
//   所以「預設用 vector，有明確理由才換 deque」仍然是對的準則。
//
// 【5. 回到本課主題：容器選型的三個問題】
//   面對「我要存一大堆 bool」時，依序問自己：
//     (1) 需要真正的 bool& / bool* 嗎？ → 需要就別用 vector<bool>
//     (2) 需要整塊交給 C API（data()）嗎？ → 需要就只能用 vector<uint8_t>
//                                             或 std::array<bool, N>
//     (3) 記憶體是關鍵瓶頸嗎？ → 是的話才考慮 vector<bool> 或 std::bitset
//   deque<bool> 只在「(1) 需要 + 頻繁頭尾增刪」這個交集上勝出。
//
// 【概念補充 Concept Deep Dive】
//   ▸ deque 的迭代器為什麼比較「胖」
//     它必須同時記住：目前元素的位址、目前區塊的起點與終點、
//     以及自己在中央索引表中的位置——通常是四根指標。
//     vector 的迭代器只要一根。這也是 deque 迭代器的 ++
//     必須檢查「是否走到區塊邊界」而稍慢的原因。
//   ▸ 為什麼 deque 頭尾插入不使 reference 失效，但 vector 會
//     vector 擴容必須把「全部元素」搬到新的連續區域，元素位址當然改變。
//     deque 只是在索引表兩端掛上新區塊，既有區塊原地不動。
//     這正是「連續記憶體」這個承諾的代價與豁免的分界。
//   ▸ std::queue / std::stack 預設就是用 deque
//     兩者都是容器配接器，預設底層容器是 std::deque，
//     正因為它的頭尾操作是 O(1) 且不需要整塊搬移。
//
// 【注意事項 Pay Attention】
//   1. deque 沒有 data()，元素不連續，不能整塊交給 C API。
//   2. deque 的失效規則和 vector 不同：頭尾操作使 iterator 失效，
//      但 reference / pointer 仍有效。
//   3. 中間 insert/erase 會使 iterator / reference / pointer 全部失效。
//   4. 區塊大小是實作定義（本機 libstdc++ 實測為 512 bytes）。
//   5. 隨機存取雖是 O(1)，但常數比 vector 大，且對快取較不友善。
//   6. deque<bool> 每元素 1 byte，記憶體約為 vector<bool> 的 8 倍。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】deque 的記憶體模型與失效規則
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. deque 的記憶體是怎麼組織的？為什麼它沒有 data()？
//     答：deque 用「一張中央索引表（指標陣列）＋ 多個固定大小的區塊」，
//         每根指標指向一塊記憶體，元素分散在這些不相鄰的區塊裡。
//         本機（GCC 15.2 / libstdc++）實測區塊為 512 bytes。
//         因為不存在一段涵蓋全部元素的連續記憶體，
//         就回傳不了一根能用 ptr[i] 走完全部元素的指標，所以沒有 data()。
//     追問：那 deque 的 operator[] 為什麼還是 O(1)？
//         → 區塊大小固定，所以由索引可以直接算出「第幾塊、塊內第幾個」，
//           再做兩次解參考即可。是 O(1)，但常數比 vector 的
//           一次位址算術大，而且對快取較不友善。
//
// 🔥 Q2. deque 與 vector 的迭代器失效規則差在哪裡？
//     答：vector 只要觸發重新配置，iterator / reference / pointer 全部失效。
//         deque 在頭尾 push/pop 時會使所有 iterator 失效
//         （迭代器要記住「哪一塊、塊內第幾個」，索引表可能重新配置），
//         但 reference 與 pointer 仍然有效——因為既有元素根本沒被搬動。
//         中間的 insert/erase 則是三者全部失效。
//     追問：這個差異有什麼實務價值？
//         → 當你需要長期持有指向某元素的指標、又要頻繁在頭尾增刪時，
//           deque 是標準容器中少數能同時滿足的選擇。
//           用 vector 的話，任何一次擴容就會讓那根指標懸空。
//
// ⚠️ 陷阱. 「deque 頭尾插入不會讓 pointer 失效，
//          那我把 &d[0] 存起來，之後一直用它來讀第一個元素，
//          應該沒問題吧？」
//     答：不行，而且錯得很微妙。標準保證的是
//         「指向某個『元素』的 pointer 仍然有效」——
//         &d[0] 拿到的是「當時那個元素」的位址。
//         之後若 push_front 了新元素，那根指標仍然指向原本那個元素
//         （它現在是 d[1]），而不是新的 d[0]。
//         指標沒有懸空，但它指的已經不是你以為的那一個了。
//     為什麼會錯：把「pointer 仍然有效」誤讀成「d[0] 的位址不變」。
//         有效性講的是「那個物件還活著、位址沒變」，
//         不是「它在容器中的序號沒變」。
//         這和 vector 的 erase 之後「位址還在但住的是別的元素」
//         是同一類的思考陷阱。
// ═══════════════════════════════════════════════════════════════════════════

#include <deque>
#include <iostream>
#include <string>
#include <vector>

// -----------------------------------------------------------------------------
// 【日常實務範例】任務佇列：長期持有「目前處理中的任務」指標
//   情境：一個工作佇列，新任務從尾端加入、完成的從頭端移除。
//         同時有一個監控模組長期持有「目前處理中那筆任務」的指標，
//         用來即時回報進度。
//   為什麼用 deque：
//         若用 vector，任何一次 push_back 觸發擴容，
//         監控模組手上的指標就懸空了（heap-use-after-free）。
//         deque 的頭尾操作不會搬動既有元素，
//         所以那根指標始終指向同一個任務物件，一直有效。
//         這是 deque 相對 vector 最實質的優勢之一。
// -----------------------------------------------------------------------------
struct Job {
    int         id;
    std::string name;
    int         progress;   // 0..100
};

class JobQueue {
    std::deque<Job> jobs_;
public:
    void submit(int id, const std::string& name) {
        jobs_.push_back({id, name, 0});     // 尾端加入：既有元素位址不變
    }
    void finishFront() {
        if (!jobs_.empty()) jobs_.pop_front();   // 頭端移除：其餘元素位址不變
    }
    Job* current() { return jobs_.empty() ? nullptr : &jobs_.front(); }
    std::size_t size() const { return jobs_.size(); }
};

int main() {
    std::cout << std::boolalpha;

    std::cout << "=== 一、deque<bool> 的引用與指標都是真的 ===" << std::endl;
    std::deque<bool> db = {true, false, true};

    bool& ref = db[0];
    ref = false;
    std::cout << "透過 bool& 修改後 db[0] = " << db[0] << std::endl;

    bool* ptr = &db[1];
    std::cout << "*ptr = " << *ptr << "（貨真價實的 bool*）" << std::endl;

    std::cout << "\n=== 二、deque 的分段配置：實測區塊邊界 ===" << std::endl;
    std::deque<char> chunks;
    for (int i = 0; i < 2000; ++i) chunks.push_back('x');

    std::cout << "對 deque<char> 檢查「相鄰元素位址是否連續」：" << std::endl;
    int breaks = 0;
    int lastBreak = 0;
    for (int i = 1; i < 2000 && breaks < 3; ++i) {
        if (&chunks[static_cast<std::size_t>(i)] !=
            &chunks[static_cast<std::size_t>(i - 1)] + 1) {
            std::cout << "  第 " << i << " 個元素起，換到新區塊（間隔 "
                      << (i - lastBreak) << " 個元素）" << std::endl;
            lastBreak = i;
            ++breaks;
        }
    }
    std::cout << "→ 本機 libstdc++ 的區塊大小為 512 bytes（實作定義）" << std::endl;
    std::cout << "→ 正因為不連續，deque 才沒有 data()" << std::endl;

    std::cout << "\n=== 三、對照：vector 的元素永遠連續 ===" << std::endl;
    std::vector<char> flat(2000, 'y');
    bool contiguous = true;
    for (std::size_t i = 1; i < flat.size(); ++i) {
        if (&flat[i] != &flat[i - 1] + 1) { contiguous = false; break; }
    }
    std::cout << "vector<char> 全部元素連續: " << contiguous
              << "（所以它有 data()）" << std::endl;

    std::cout << "\n=== 四、失效規則的關鍵差異 ===" << std::endl;
    std::deque<int> dq = {10, 20, 30};
    int* pDeque = &dq[1];                  // 指向元素 20
    for (int i = 0; i < 1000; ++i) dq.push_back(i);   // 大量尾端插入
    std::cout << "deque  尾端插入 1000 筆後，原指標指的值 = " << *pDeque
              << "（仍然有效：元素沒被搬動）" << std::endl;

    std::vector<int> vc = {10, 20, 30};
    const int* pVecBefore = &vc[1];
    for (int i = 0; i < 1000; ++i) vc.push_back(i);   // 必定觸發多次重新配置
    std::cout << "vector 尾端插入 1000 筆後，緩衝區位址已改變: "
              << (&vc[1] != pVecBefore)
              << "（原指標已懸空，故不解參考它）" << std::endl;

    std::cout << "\n=== 五、「pointer 有效」不等於「它還是第 0 個」 ===" << std::endl;
    std::deque<int> d2 = {100, 200};
    int* pFirst = &d2[0];                  // 當時的第 0 個元素，值為 100
    d2.push_front(50);                     // 前面插入新元素
    std::cout << "push_front 之後：" << std::endl;
    std::cout << "  *pFirst = " << *pFirst << "（指標有效，仍指向原本那個元素）" << std::endl;
    std::cout << "  d2[0]   = " << d2[0]   << "（但第 0 個已經換人了）" << std::endl;

    std::cout << "\n=== 六、日常實務：任務佇列的長期指標 ===" << std::endl;
    JobQueue q;
    q.submit(1, "encode-video");
    q.submit(2, "generate-thumbnail");

    Job* watching = q.current();           // 監控模組長期持有這根指標
    std::cout << "監控中的任務: #" << watching->id << " " << watching->name << std::endl;

    for (int i = 3; i <= 200; ++i) {       // 大量新任務湧入
        q.submit(i, "job-" + std::to_string(i));
    }
    watching->progress = 42;               // 指標仍然有效，可直接更新進度
    std::cout << "湧入 198 筆新任務後，指標仍有效：#" << watching->id
              << " 進度 = " << watching->progress << "%" << std::endl;
    std::cout << "佇列長度 = " << q.size() << std::endl;
    std::cout << "（若底層改成 vector，這根指標早已在某次擴容時懸空）" << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 18 課：vectorbool 的特殊性與陷阱11.cpp" -o deque_layout
//
// 【關於下方預期輸出的但書】
//   ▸ deque 的區塊大小 512 bytes 是本機（x86-64 / GCC 15.2 / libstdc++）
//     的實測值，屬實作定義；libc++ 與 MSVC 的策略都不同。
//     因此「第 512 / 1024 / 1536 個元素起換區塊」這幾行
//     在其他標準庫上會是不同的數字。
//   ▸ 本檔刻意不印出任何記憶體位址（每次執行都不同），
//     只印出「位址是否連續」「位址是否改變」這類布林結果。
//   ▸ 第四段中 vector 的舊指標在擴容後已懸空，
//     程式只比較它與新位址是否不同（比較指標值本身是合法的），
//     絕不解參考它。
//
// 【本檔未附 LeetCode 範例的理由】
//   本檔談的是容器的底層記憶體佈局與迭代器/指標失效規則，
//   屬於實作機制層面的知識。LeetCode 的題目不會因為
//   底層是分段配置還是連續配置而有不同解法，
//   硬套一題無法呈現重點；因此改以「長期持有元素指標的任務佇列」
//   這個真正只有 deque 能滿足的實務場景呈現。

// === 預期輸出 ===
// === 一、deque<bool> 的引用與指標都是真的 ===
// 透過 bool& 修改後 db[0] = false
// *ptr = false（貨真價實的 bool*）
//
// === 二、deque 的分段配置：實測區塊邊界 ===
// 對 deque<char> 檢查「相鄰元素位址是否連續」：
//   第 512 個元素起，換到新區塊（間隔 512 個元素）
//   第 1024 個元素起，換到新區塊（間隔 512 個元素）
//   第 1536 個元素起，換到新區塊（間隔 512 個元素）
// → 本機 libstdc++ 的區塊大小為 512 bytes（實作定義）
// → 正因為不連續，deque 才沒有 data()
//
// === 三、對照：vector 的元素永遠連續 ===
// vector<char> 全部元素連續: true（所以它有 data()）
//
// === 四、失效規則的關鍵差異 ===
// deque  尾端插入 1000 筆後，原指標指的值 = 20（仍然有效：元素沒被搬動）
// vector 尾端插入 1000 筆後，緩衝區位址已改變: true（原指標已懸空，故不解參考它）
//
// === 五、「pointer 有效」不等於「它還是第 0 個」 ===
// push_front 之後：
//   *pFirst = 100（指標有效，仍指向原本那個元素）
//   d2[0]   = 50（但第 0 個已經換人了）
//
// === 六、日常實務：任務佇列的長期指標 ===
// 監控中的任務: #1 encode-video
// 湧入 198 筆新任務後，指標仍有效：#1 進度 = 42%
// 佇列長度 = 200
// （若底層改成 vector，這根指標早已在某次擴容時懸空）
