// =============================================================================
//  第 17 課 總複習：vector 的記憶體重新配置機制
//                 —— 「連續記憶體」這個承諾，代價是什麼
// =============================================================================
//
// 【主題資訊 Information】
//   容量相關成員：
//     size_type size()      const noexcept;   // 目前有幾個已建構的元素
//     size_type capacity()  const noexcept;   // 不必重新配置就能容納的上限
//     void      reserve(size_type n);         // 抬高 capacity（不改 size）
//     void      resize(size_type n);          // 改變 size（會建構/解構元素）
//     void      shrink_to_fit();              // 非強制性請求：capacity → size
//   標準保證：push_back 的均攤複雜度為 O(1)（[vector.modifiers]）
//   標準版本：shrink_to_fit 是 C++11 加入；其餘 C++98 起即有
//   標頭檔：<vector>
//
// 【詳細解釋 Explanation】
//
// 【1. 一切的起點：vector 必須維持記憶體連續】
//   標準要求 vector 的元素在記憶體中連續存放（C++11 起明文保證），
//   這讓 &v[0] 可以直接餵給 C API、讓索引存取是一次位址算術、
//   也讓 CPU 的預取器與快取發揮到極致。
//   但連續是有代價的：當現有那塊記憶體用完，不可能「在後面接一段」——
//   後面可能已經住了別人。唯一的辦法是另外配置一塊更大的、
//   把全部元素搬過去、再把舊的還掉。
//   本課所有的現象——擴容、迭代器失效、reserve、shrink_to_fit——
//   全部是這一個約束的下游後果。
//
// 【2. 倍率成長：把 O(n²) 換成均攤 O(1)】
//   若每次只多要固定 k 格，插入 n 個元素要重新配置 n/k 次，
//   總搬移量 k+2k+3k+… ≈ n²/(2k)，是 O(n²)。
//   改成每次乘上固定倍率後，容量序列是 1,2,4,8,…，
//   只需 log₂(n) 次重新配置，總搬移量 1+2+4+…+n ≈ 2n，
//   平均分攤到每次 push_back 就是常數。
//   本機（GCC 15.2 / libstdc++）實測倍率為 2，MSVC 為 1.5——
//   標準從未規定倍率，這是實作定義。
//
// 【3. 均攤 O(1) 不等於最壞 O(1)】
//   絕大多數 push_back 是純粹的「寫一格 + size 加一」，
//   但剛好觸發重新配置的那一次是 O(n)。
//   對吞吐量導向的程式這無所謂；對遊戲主迴圈、交易系統、
//   即時控制這類延遲敏感的場景，這個偶發的長尾延遲就是事故。
//   消除它的方法只有一個：事先 reserve。
//
// 【4. 搬移時用 move 還是 copy —— 由 noexcept 決定】
//   vector 的重新配置提供「強例外保證」：中途若丟出例外，
//   原容器必須維持不變。
//   問題來了：移動會掏空來源物件，搬到一半失敗就再也還原不了；
//   複製則不會破壞來源，隨時可以放棄新緩衝區退回原狀。
//   所以標準庫用 std::move_if_noexcept：
//   只有當移動建構子標了 noexcept（保證不丟例外）時才敢用移動，
//   否則保守地退回複製。
//   實務結論：自訂類別的移動建構子一定要寫 noexcept，
//   否則放進 vector 擴容時會靜默退化成深拷貝，效能可能差好幾個數量級。
//
// 【5. 迭代器失效的完整規則（vector）】
//     push_back / emplace_back
//       觸發重新配置 → 全部失效；未觸發 → 只有 end() 失效
//     insert
//       觸發重新配置 → 全部失效；未觸發 → 插入點之後失效
//     erase
//       刪除點及其之後失效；之前仍有效（erase 不重新配置）
//     reserve / resize / shrink_to_fit
//       容量若改變 → 全部失效
//   最危險的是 push_back 的「可能失效」：同一行程式碼，
//   容量夠時沒事、剛好用滿時全毀。正確心態是一律當作已失效。
//
// 【6. size 與 capacity 是兩件完全不同的事】
//   capacity 是「這塊緩衝區能放幾個」，size 是「目前真的建構了幾個」。
//     v.reserve(10); v[3] = 5;   // UB！size 還是 0，元素根本不存在
//     v.resize(10);  v[3] = 5;   // OK，10 個元素都已被 value-initialize
//   而刪除只降低 size、從不歸還記憶體——這是「記憶體用量只增不減」
//   最常見的原因。要真的還回去必須 shrink_to_fit()。
//
// 【概念補充 Concept Deep Dive】
//   ▸ 重新配置的五個步驟
//       1) 配置更大的新記憶體
//       2) 把舊元素搬過去（move_if_noexcept 決定 move 或 copy）
//       3) 解構舊記憶體中的元素
//       4) 釋放舊記憶體
//       5) 更新內部三根指標 begin_ / end_ / cap_end_
//     只要走到第 4 步，所有指向舊緩衝區的迭代器就成了懸空指標。
//   ▸ 為什麼 2 倍不見得是最佳倍率
//     用 2 倍時，新容量恆大於先前所有已釋放區塊的總和（1+2+4 < 8），
//     因此永遠無法重複利用前面釋放的記憶體。
//     小於黃金比例 φ≈1.618 的倍率（如 MSVC 的 1.5）理論上有機會回收再利用。
//     實務差異取決於配置器，兩派都有支持者；重點是不要寫死假設。
//   ▸ 為什麼 shrink_to_fit 只是「請求」
//     縮容通常要配置新記憶體再搬移，這個動作可能失敗，
//     而且過程中新舊兩塊同時存在，瞬間記憶體用量反而更高。
//     標準不願強制一個「可能丟例外、可能更耗記憶體」的操作。
//
// 【注意事項 Pay Attention】
//   1. 成長倍率是實作定義（本機 libstdc++ 實測 2 倍，MSVC 為 1.5 倍）。
//   2. 均攤 O(1) 不代表每次都 O(1)；延遲敏感場景必須 reserve。
//   3. reserve 只改 capacity，不改 size；之後索引存取仍可能越界。
//   4. 自訂類別的移動建構子務必標 noexcept，否則擴容會退化為複製。
//   5. erase / clear 都不歸還記憶體；需要時明確呼叫 shrink_to_fit()。
//   6. shrink_to_fit 是非強制性請求，標準允許實作完全不動作。
//   7. reference 與 pointer 的失效規則和迭代器完全相同。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】vector 的記憶體重新配置機制
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 請說明 vector 的擴容過程，以及為什麼 push_back 是均攤 O(1)。
//     答：size == capacity 時再 push_back，會配置一塊更大的記憶體、
//         把全部元素搬過去、解構並釋放舊的，最後在尾端建構新元素。
//         由於容量按固定倍率成長，插入 n 個元素只需 log₂(n) 次重新配置，
//         總搬移量約 2n（等比級數和小於首項兩倍），
//         平均到每次 push_back 就是常數。
//     追問：為什麼不用「每次多加固定格數」的策略？
//         → 那會變成 O(n²)：重新配置次數與 n 成正比，
//           每次又要搬移目前所有元素。倍率成長才能把總量壓到線性。
//
// 🔥 Q2. vector 擴容搬移元素時，什麼時候用 move、什麼時候用 copy？為什麼？
//     答：取決於元素的移動建構子有沒有標 noexcept。
//         vector 必須提供強例外保證——搬移中途丟例外時原容器要維持不變。
//         移動會掏空來源，失敗就無法還原；複製則可以隨時放棄新緩衝區。
//         所以標準庫用 std::move_if_noexcept，只在移動保證不丟例外時才用移動。
//     追問：這對實務有什麼直接影響？
//         → 自訂類別的移動建構子必須寫 noexcept。
//           少了它，把物件放進 vector 時每次擴容都是深拷貝，
//           對持有堆積資源的型別可能慢好幾個數量級——而且完全沒有警告。
//
// 🔥 Q3. reserve 和 resize 差在哪裡？
//     答：reserve(n) 只抬高 capacity，size 完全不變，
//         不建構任何元素；resize(n) 改變 size，會實際建構
//         （或解構）元素使其數量變成 n。
//         所以 reserve(10) 之後 v[3] 仍是 UB，resize(10) 之後才合法。
//     追問：resize 會不會縮小 capacity？
//         → 不會。resize 變小只解構多餘元素、降低 size，
//           capacity 維持原樣。要縮容仍需 shrink_to_fit()。
//
// ⚠️ 陷阱. std::vector<int> v = {1,2,3};
//          int* p = &v[0];
//          v.push_back(4);
//          std::cout << *p;          // ← 為什麼這是 undefined behavior？
//     答：push_back 若觸發重新配置，整塊緩衝區搬到新位址、舊的已被釋放，
//         p 就成了懸空指標，讀它是 heap-use-after-free。
//         關鍵在於「若觸發」——容量還夠時它其實還有效，
//         所以這個 bug 在小資料量測試下經常不會顯現。
//     為什麼會錯：只把「迭代器失效」記成迭代器的問題，
//         沒意識到 pointer 與 reference 本質上完全相同——
//         三者都只是「記住了一個位址」。
//         口訣：凡是從容器取出位址的東西，失效規則一模一樣。
// ═══════════════════════════════════════════════════════════════════════════

/*
 * ================================================================
 * 【第17課：vector 的記憶體重新配置機制】總複習 summary.cpp
 * ================================================================
 * 編譯方式：g++ -std=c++17 -o summary summary.cpp
 * 本課重點：
 * 1. 擴容策略：size 滿時配置更大的新記憶體，複製/移動元素，釋放舊記憶體
 * 2. 成長倍率：通常是 1.5 倍（MSVC）或 2 倍（GCC/Clang），均攤 O(1) 的保證
 * 3. push_back 觸發重新配置時的物件搬移：移動建構子（若有 noexcept）優先
 * 4. 迭代器失效：重新配置後所有迭代器/指標/引用全部失效
 * 5. insert/erase 也可能造成迭代器失效（元素搬移即使不重新配置）
 * 6. 在迴圈中安全刪除元素的正確寫法（利用 erase 的回傳值）
 * 7. erase-remove 慣用法（std::remove_if + erase）
 * 8. reserve() 避免不必要的重新配置
 * 9. shrink_to_fit() 釋放多餘的容量
 * 10. 使用 chrono 量測 reserve 對效能的影響
 * ================================================================
 */

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>  // std::remove_if
#include <chrono>

// ===== 重點一：擴容策略（Reallocation Strategy）=====
// 當 vector 的 size == capacity 時，再呼叫 push_back 就會觸發「重新配置」：
//   1. 配置一塊「更大」的新記憶體（通常 1.5 或 2 倍）
//   2. 把所有現有元素從舊記憶體複製（或移動）到新記憶體
//   3. 銷毀舊記憶體中的元素
//   4. 釋放舊記憶體
//   5. 在新記憶體尾端建構新元素
//
// 這個過程是「透明的」——使用者感覺 push_back 就是在尾端加一個元素。

// ===== 重點二：均攤 O(1) 的數學原理 =====
// 倍率成長策略保證 n 次 push_back 的總拷貝次數是 O(n)，
// 因此「每次 push_back 的均攤成本」是 O(1)。
//
// 以倍率 2 為例（0→1→2→4→8→16→...）：
//   加入第 1 個：拷貝 0 次
//   加入第 2 個：拷貝 1 次（1→2，搬 1 個舊元素）
//   加入第 3 個：拷貝 2 次（2→4，搬 2 個舊元素）
//   加入第 5 個：拷貝 4 次（4→8，搬 4 個舊元素）
//   ...
//   加入 n 個元素，總拷貝次數 ≤ 2n，均攤每次 O(1)

// ===== 重點三：noexcept 移動建構子的重要性 =====
// 重新配置時，vector 需要把舊元素搬到新位址。
//   - 若元素型別有「標記 noexcept 的移動建構子」→ 使用移動（高效）
//   - 若沒有 noexcept 的移動建構子                → 使用拷貝（保守，以防例外安全）
// 結論：自訂類別的移動建構子務必標記 noexcept。

// ===== 重點四：迭代器失效的根本原因 =====
// 重新配置後，舊記憶體被釋放，指向舊記憶體的迭代器就變成「懸空迭代器」。
// 繼續使用懸空迭代器 = 未定義行為（UB），可能崩潰或出現奇怪數值。
//
// insert 和 erase 也會使「插入/刪除點之後」的迭代器失效，
// 因為後方元素被搬移了，原本的地址存的是不同的值。

// ===== 重點五：在迴圈中安全刪除元素 =====
// 錯誤寫法：
//   for (auto it = v.begin(); it != v.end(); ++it) {
//       if (條件) v.erase(it);  // erase 後 it 失效，++it 是 UB！
//   }
//
// 正確寫法：利用 erase() 的回傳值（下一個有效迭代器）
//   for (auto it = v.begin(); it != v.end(); /* 不在此處 ++it */) {
//       if (條件) it = v.erase(it);  // 取回有效的下一個迭代器
//       else      ++it;
//   }

// ===== 重點六：erase-remove 慣用法（推薦）=====
// std::remove_if 把不滿足條件的元素「移到前面」，回傳邏輯新尾端。
// v.erase(new_end, v.end()) 才真正刪除尾部的無用元素。
// 這個「先 remove_if 再 erase」的模式叫做「erase-remove idiom」。

// ===== 重點七：reserve() 與 shrink_to_fit() =====
// reserve(n)：至少保留 n 個元素的容量，避免後續重新配置
//   - 只影響 capacity，不影響 size
//   - 若 n <= 目前 capacity，無任何效果
//   - 若 n > 目前 capacity，觸發一次重新配置（現有迭代器失效！）
//
// shrink_to_fit()：請求把 capacity 縮減到 size 大小
//   - 是「非綁定性請求」（hint），編譯器/STL 實作可忽略，但通常會執行
//   - 使用時機：大量刪除元素後，想釋放多餘記憶體

// 用於觀察物件搬移行為的追蹤類別
class Tracker {
    std::string name_;
public:
    explicit Tracker(const std::string& name) : name_(name) {
        std::cout << "  建構: " << name_ << std::endl;
    }
    Tracker(const Tracker& other) : name_(other.name_) {
        std::cout << "  拷貝: " << name_ << std::endl;
    }
    // noexcept 讓 vector 在重新配置時優先使用移動而非拷貝
    Tracker(Tracker&& other) noexcept : name_(std::move(other.name_)) {
        std::cout << "  移動: " << name_ << std::endl;
    }
    ~Tracker() {
        if (!name_.empty())
            std::cout << "  銷毀: " << name_ << std::endl;
    }
    const std::string& name() const { return name_; }
};

void demo_reallocation_observation() {
    std::cout << "\n--- 一、觀察重新配置的時機與位址變化 ---" << std::endl;
    std::vector<int> v;

    std::cout << "初始：size=" << v.size()
              << ", capacity=" << v.capacity() << std::endl;

    for (int i = 1; i <= 10; ++i) {
        size_t old_cap = v.capacity();
        const int* old_data = v.data();

        v.push_back(i);

        if (v.capacity() != old_cap) {
            // 刻意不印位址本身——位址每次執行都不同，無法作為預期輸出。
            // 改印「位址有沒有改變」這個布林結果，才是可重現的觀察。
            std::cout << "  *** 重新配置！舊容量=" << old_cap
                      << " → 新容量=" << v.capacity()
                      << "（緩衝區位址已改變："
                      << std::boolalpha << (v.data() != old_data) << "）" << std::endl;
        }
    }
}

void demo_growth_ratio() {
    std::cout << "\n--- 二、成長倍率觀察（push_back 100 次）---" << std::endl;
    std::vector<int> v;
    size_t prev_cap = 0;

    for (int i = 0; i < 100; ++i) {
        v.push_back(i);
        if (v.capacity() != prev_cap) {
            if (prev_cap > 0) {
                double ratio = static_cast<double>(v.capacity()) / prev_cap;
                std::cout << "  " << prev_cap << " → " << v.capacity()
                          << "（倍率約 " << ratio << "）" << std::endl;
            }
            prev_cap = v.capacity();
        }
    }
    // 通常看到的倍率：GCC/Clang 約 2 倍，MSVC 約 1.5 倍
}

void demo_move_vs_copy_on_realloc() {
    std::cout << "\n--- 三、重新配置時：移動 vs 拷貝（需要 noexcept）---" << std::endl;
    std::vector<Tracker> v;
    v.reserve(2);  // 故意只留 2 個位置

    std::cout << "push_back A：" << std::endl;
    v.push_back(Tracker("A"));

    std::cout << "push_back B：" << std::endl;
    v.push_back(Tracker("B"));

    std::cout << "push_back C（觸發重新配置）：" << std::endl;
    v.push_back(Tracker("C"));
    // 由於 Tracker 的移動建構子有 noexcept，vector 會使用移動而非拷貝
}

void demo_iterator_invalidation_by_push_back() {
    std::cout << "\n--- 四、push_back 觸發重新配置使迭代器失效 ---" << std::endl;
    std::vector<int> v = {10, 20, 30};
    v.reserve(3);  // 確保容量剛好是 3，下一次 push_back 必定重新配置

    auto it = v.begin() + 1;  // 指向 20
    std::cout << "push_back 前 *it = " << *it << std::endl;  // 20

    v.push_back(40);  // 觸發重新配置，it 失效

    // 絕對不可以：std::cout << *it;  // 未定義行為！

    // 安全做法：重新取得迭代器
    it = v.begin() + 1;
    std::cout << "重新取得後 *it = " << *it << std::endl;  // 20
}

void demo_iterator_invalidation_by_insert_erase() {
    std::cout << "\n--- 五、insert/erase 也會使插入/刪除點之後的迭代器失效 ---" << std::endl;
    std::vector<int> v = {10, 20, 30, 40, 50};
    v.reserve(100);  // 預留足夠容量，排除重新配置的干擾

    auto it_10 = v.begin();      // 指向 10（插入點之前）
    int  before_at_index2 = v[2];  // 先記下索引 2 目前的值（是 30）

    v.insert(v.begin() + 1, 15);  // 在索引 1 插入 15 → {10, 15, 20, 30, 40, 50}

    // 插入點之前的迭代器仍然有效
    std::cout << "*it_10（插入點之前）= " << *it_10 << "（仍有效）" << std::endl;

    // 插入點之後的迭代器已失效：位址還在，但上面住的已經換人了。
    // 這裡不去解參考那個失效迭代器（那是 UB），改用索引呈現同一個事實：
    std::cout << "插入前 v[2] = " << before_at_index2
              << "，插入後 v[2] = " << v[2]
              << "  ← 同一個位置，元素已不同" << std::endl;
    std::cout << "  所以指向插入點之後的舊迭代器即使「還能讀」，讀到的也是錯的元素" << std::endl;

    std::cout << "目前 v = ";
    for (int x : v) std::cout << x << " ";
    std::cout << std::endl;
}

void demo_safe_erase_in_loop() {
    std::cout << "\n--- 六、迴圈中安全刪除元素（正確 vs 錯誤）---" << std::endl;
    std::vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8};

    std::cout << "刪除前：";
    for (int x : v) std::cout << x << " ";
    std::cout << std::endl;

    // 正確做法：利用 erase() 回傳值取得下一個有效迭代器
    for (auto it = v.begin(); it != v.end(); /* 不在此 ++it */) {
        if (*it % 2 == 0) {
            it = v.erase(it);  // erase 回傳指向下一個元素的迭代器
        } else {
            ++it;              // 只在不刪除時才手動前進
        }
    }

    std::cout << "刪除偶數後：";
    for (int x : v) std::cout << x << " ";
    std::cout << std::endl;
}

void demo_erase_remove_idiom() {
    std::cout << "\n--- 七、erase-remove 慣用法（推薦方式）---" << std::endl;
    std::vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8};

    std::cout << "刪除前：";
    for (int x : v) std::cout << x << " ";
    std::cout << std::endl;

    // std::remove_if：把不滿足條件的元素移到前面，回傳邏輯新尾端
    // v.erase(new_end, v.end())：真正刪除「尾部的無用元素」
    auto new_end = std::remove_if(v.begin(), v.end(),
                                  [](int x) { return x % 2 == 0; });
    v.erase(new_end, v.end());

    std::cout << "刪除偶數後（erase-remove）：";
    for (int x : v) std::cout << x << " ";
    std::cout << std::endl;
}

void demo_reserve_performance() {
    std::cout << "\n--- 八、reserve 對效能的影響 ---" << std::endl;

    {
        std::vector<int> v;
        int realloc_count = 0;
        size_t prev_cap = 0;
        for (int i = 0; i < 10000; ++i) {
            v.push_back(i);
            if (v.capacity() != prev_cap) {
                ++realloc_count;
                prev_cap = v.capacity();
            }
        }
        std::cout << "不用 reserve：重新配置 " << realloc_count
                  << " 次，最終容量 " << v.capacity() << std::endl;
    }

    {
        std::vector<int> v;
        v.reserve(10000);  // 預先配置，避免後續重新配置
        int realloc_count = 0;
        size_t prev_cap = v.capacity();
        for (int i = 0; i < 10000; ++i) {
            v.push_back(i);
            if (v.capacity() != prev_cap) {
                ++realloc_count;
                prev_cap = v.capacity();
            }
        }
        std::cout << "使用 reserve：重新配置 " << realloc_count
                  << " 次，最終容量 " << v.capacity() << std::endl;
    }
}

void demo_shrink_to_fit() {
    std::cout << "\n--- 九、shrink_to_fit：釋放多餘容量 ---" << std::endl;
    std::vector<int> v;

    for (int i = 0; i < 10000; ++i) v.push_back(i);
    std::cout << "插入 10000 個後：size=" << v.size()
              << ", capacity=" << v.capacity() << std::endl;

    // 刪除大部分，只保留前 10 個
    v.erase(v.begin() + 10, v.end());
    std::cout << "刪除後（未 shrink）：size=" << v.size()
              << ", capacity=" << v.capacity() << std::endl;

    // 要求把多餘容量還給系統
    v.shrink_to_fit();
    std::cout << "shrink_to_fit 後：size=" << v.size()
              << ", capacity=" << v.capacity() << std::endl;
}

void demo_timing_reserve() {
    std::cout << "\n--- 十、計時對比：使用/不使用 reserve ---" << std::endl;
    const int N = 5'000'000;

    auto time_it = [&](const std::string& label, auto fn) {
        auto start = std::chrono::high_resolution_clock::now();
        fn();
        auto end = std::chrono::high_resolution_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << label << ": " << ms << " ms" << std::endl;
    };

    time_it("不用 reserve", [&]() {
        std::vector<int> v;
        for (int i = 0; i < N; ++i) v.push_back(i);
    });

    time_it("使用 reserve", [&]() {
        std::vector<int> v;
        v.reserve(N);
        for (int i = 0; i < N; ++i) v.push_back(i);
    });
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 155. Min Stack
//   題目：設計一個 stack，push / pop / top / getMin 全部都要 O(1)。
//   為什麼用到本主題：這題底層就是兩個動態成長的 vector，
//         而「push 是 O(1)」這句話能成立，靠的正是本課的均攤分析——
//         單次 push 偶爾會觸發 O(n) 的重新配置，但均攤下來是 O(1)。
//         題目所謂的 O(1) 指的就是均攤 O(1)。
//         這裡刻意示範 reserve 的用途：若事先知道操作次數上限，
//         reserve 可以把那個「偶發的 O(n) 長尾」完全消除，
//         讓 push 變成真正的最壞 O(1)。
// -----------------------------------------------------------------------------
class MinStack {
    std::vector<int> data_;    // 實際資料
    std::vector<int> mins_;    // 與 data_ 同步的「到目前為止的最小值」堆疊
public:
    MinStack() = default;

    // 已知操作次數上限時，先把容量開好，消除擴容造成的長尾延遲
    explicit MinStack(std::size_t expectedOps) {
        data_.reserve(expectedOps);
        mins_.reserve(expectedOps);
    }

    void push(int val) {
        data_.push_back(val);
        // 新的最小值 = min(舊的最小值, val)；空堆疊時就是 val 本身
        mins_.push_back(mins_.empty() ? val : (val < mins_.back() ? val : mins_.back()));
    }
    void pop() {
        data_.pop_back();      // pop_back 從不重新配置，capacity 不變
        mins_.pop_back();
    }
    int top()    const { return data_.back(); }
    int getMin() const { return mins_.back(); }

    std::size_t capacity() const { return data_.capacity(); }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】封包組裝緩衝區：用 reserve 消除傳輸途中的重新配置
//   情境：網路層收到分片的封包，要依序組回一個完整訊息。
//         標頭已經告訴我們總長度，但資料是一片一片到的。
//   為什麼用本主題：這是 reserve 最典型的正當用途——
//         總長度已知，卻只能逐片附加。
//         若不 reserve，組裝一個 1 MB 的訊息會經歷約 20 次
//         重新配置與 memcpy；更麻煩的是，任何事先取得的
//         指標或迭代器都會在組裝途中失效。
// -----------------------------------------------------------------------------
std::vector<unsigned char> assemblePacket(
        const std::vector<std::vector<unsigned char>>& fragments,
        std::size_t totalLen,
        bool useReserve,
        int& reallocCount) {
    std::vector<unsigned char> buf;
    if (useReserve) buf.reserve(totalLen);

    reallocCount = 0;
    const unsigned char* prev = buf.data();

    for (const auto& frag : fragments) {
        buf.insert(buf.end(), frag.begin(), frag.end());
        if (buf.data() != prev) {          // 位址改變 = 發生過重新配置
            ++reallocCount;
            prev = buf.data();
        }
    }
    return buf;
}

void demo_leetcode_and_practice() {
    std::cout << "\n--- 十一、LeetCode 155. Min Stack ---" << std::endl;
    MinStack st;
    st.push(-2);
    st.push(0);
    st.push(-3);
    std::cout << "push(-2), push(0), push(-3)" << std::endl;
    std::cout << "  getMin() = " << st.getMin() << std::endl;   // -3
    st.pop();
    std::cout << "pop() 之後" << std::endl;
    std::cout << "  top()    = " << st.top()    << std::endl;   // 0
    std::cout << "  getMin() = " << st.getMin() << std::endl;   // -2

    // 對照：有沒有 reserve，對「單次 push 的最壞成本」的影響
    MinStack grow;                 // 不 reserve：容量一路倍增
    MinStack pre(1000);            // 先 reserve 1000
    for (int i = 0; i < 1000; ++i) { grow.push(i); pre.push(i); }
    std::cout << "push 1000 次後 capacity：不 reserve = " << grow.capacity()
              << "，有 reserve = " << pre.capacity() << std::endl;
    std::cout << "  → 兩者都是均攤 O(1)，但只有後者的「每一次」push 都是 O(1)" << std::endl;

    std::cout << "\n--- 十二、日常實務：封包組裝緩衝區 ---" << std::endl;
    // 造 200 個分片、每片 512 bytes，總長 102400 bytes
    std::vector<std::vector<unsigned char>> fragments(200,
                                                      std::vector<unsigned char>(512, 0xAB));
    const std::size_t totalLen = 200 * 512;

    int reallocs = 0;
    std::vector<unsigned char> a = assemblePacket(fragments, totalLen, false, reallocs);
    std::cout << "不用 reserve：組出 " << a.size() << " bytes，重新配置 "
              << reallocs << " 次，capacity=" << a.capacity() << std::endl;

    std::vector<unsigned char> b = assemblePacket(fragments, totalLen, true, reallocs);
    std::cout << "使用 reserve：組出 " << b.size() << " bytes，重新配置 "
              << reallocs << " 次，capacity=" << b.capacity() << std::endl;
    std::cout << "兩者內容相同: " << std::boolalpha << (a == b) << std::endl;
}

int main() {
    std::cout << "================================================================" << std::endl;
    std::cout << "   第 17 課：vector 的記憶體重新配置機制 總複習" << std::endl;
    std::cout << "================================================================" << std::endl;

    demo_reallocation_observation();
    demo_growth_ratio();
    demo_move_vs_copy_on_realloc();
    demo_iterator_invalidation_by_push_back();
    demo_iterator_invalidation_by_insert_erase();
    demo_safe_erase_in_loop();
    demo_erase_remove_idiom();
    demo_reserve_performance();
    demo_shrink_to_fit();
    demo_leetcode_and_practice();
    demo_timing_reserve();

    std::cout << "\n================================================================" << std::endl;
    std::cout << "課程重點回顧：" << std::endl;
    std::cout << "  1. size == capacity 時 push_back 觸發重新配置" << std::endl;
    std::cout << "  2. 成長倍率（約 1.5~2 倍）保證均攤 O(1)" << std::endl;
    std::cout << "  3. 移動建構子加 noexcept → 重新配置時使用移動，效率更高" << std::endl;
    std::cout << "  4. 重新配置後所有迭代器/指標/引用全部失效" << std::endl;
    std::cout << "  5. insert/erase 使插入/刪除點之後的迭代器失效" << std::endl;
    std::cout << "  6. 迴圈中安全刪除：用 erase() 的回傳值取回有效迭代器" << std::endl;
    std::cout << "  7. erase-remove 慣用法：O(n) 批量刪除，推薦使用" << std::endl;
    std::cout << "  8. reserve() 預先配置，避免重新配置" << std::endl;
    std::cout << "  9. shrink_to_fit() 釋放多餘容量" << std::endl;
    std::cout << "================================================================" << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra summary.cpp -o summary17
//
// 【關於下方預期輸出的但書】
//   ▸ 最後一段「十、計時對比」的兩個毫秒數「每次執行都不同」——
//     它們取決於 CPU 型號、當下系統負載、頻率調節與最佳化等級。
//     下方數值僅為本機某一次 -O0 建置的結果，不具重現性、
//     不可作為驗收依據；請以「reserve 版本較快」這個相對關係為準。
//   ▸ 其餘所有內容都是確定性的，但其中的 capacity 數值
//     （1, 2, 4, 8, 16 …、16384、1024 等）屬「實作定義」：
//     本機 libstdc++ 採 2 倍成長策略，MSVC 採 1.5 倍，數字會不同。
//   ▸ 第一段刻意不印記憶體位址（每次執行都不同），
//     改印「緩衝區位址已改變」這個布林結果。

// === 預期輸出 ===
// ================================================================
//    第 17 課：vector 的記憶體重新配置機制 總複習
// ================================================================
//
// --- 一、觀察重新配置的時機與位址變化 ---
// 初始：size=0, capacity=0
//   *** 重新配置！舊容量=0 → 新容量=1（緩衝區位址已改變：true）
//   *** 重新配置！舊容量=1 → 新容量=2（緩衝區位址已改變：true）
//   *** 重新配置！舊容量=2 → 新容量=4（緩衝區位址已改變：true）
//   *** 重新配置！舊容量=4 → 新容量=8（緩衝區位址已改變：true）
//   *** 重新配置！舊容量=8 → 新容量=16（緩衝區位址已改變：true）
//
// --- 二、成長倍率觀察（push_back 100 次）---
//   1 → 2（倍率約 2）
//   2 → 4（倍率約 2）
//   4 → 8（倍率約 2）
//   8 → 16（倍率約 2）
//   16 → 32（倍率約 2）
//   32 → 64（倍率約 2）
//   64 → 128（倍率約 2）
//
// --- 三、重新配置時：移動 vs 拷貝（需要 noexcept）---
// push_back A：
//   建構: A
//   移動: A
// push_back B：
//   建構: B
//   移動: B
// push_back C（觸發重新配置）：
//   建構: C
//   移動: C
//   移動: A
//   移動: B
//   銷毀: A
//   銷毀: B
//   銷毀: C
//
// --- 四、push_back 觸發重新配置使迭代器失效 ---
// push_back 前 *it = 20
// 重新取得後 *it = 20
//
// --- 五、insert/erase 也會使插入/刪除點之後的迭代器失效 ---
// *it_10（插入點之前）= 10（仍有效）
// 插入前 v[2] = 30，插入後 v[2] = 20  ← 同一個位置，元素已不同
//   所以指向插入點之後的舊迭代器即使「還能讀」，讀到的也是錯的元素
// 目前 v = 10 15 20 30 40 50
//
// --- 六、迴圈中安全刪除元素（正確 vs 錯誤）---
// 刪除前：1 2 3 4 5 6 7 8
// 刪除偶數後：1 3 5 7
//
// --- 七、erase-remove 慣用法（推薦方式）---
// 刪除前：1 2 3 4 5 6 7 8
// 刪除偶數後（erase-remove）：1 3 5 7
//
// --- 八、reserve 對效能的影響 ---
// 不用 reserve：重新配置 15 次，最終容量 16384
// 使用 reserve：重新配置 0 次，最終容量 10000
//
// --- 九、shrink_to_fit：釋放多餘容量 ---
// 插入 10000 個後：size=10000, capacity=16384
// 刪除後（未 shrink）：size=10, capacity=16384
// shrink_to_fit 後：size=10, capacity=10
//
// --- 十一、LeetCode 155. Min Stack ---
// push(-2), push(0), push(-3)
//   getMin() = -3
// pop() 之後
//   top()    = 0
//   getMin() = -2
// push 1000 次後 capacity：不 reserve = 1024，有 reserve = 1000
//   → 兩者都是均攤 O(1)，但只有後者的「每一次」push 都是 O(1)
//
// --- 十二、日常實務：封包組裝緩衝區 ---
// 不用 reserve：組出 102400 bytes，重新配置 9 次，capacity=131072
// 使用 reserve：組出 102400 bytes，重新配置 0 次，capacity=102400
// 兩者內容相同: true
//
// --- 十、計時對比：使用/不使用 reserve ---
// 不用 reserve: 46 ms
// 使用 reserve: 36 ms
//
// ================================================================
// 課程重點回顧：
//   1. size == capacity 時 push_back 觸發重新配置
//   2. 成長倍率（約 1.5~2 倍）保證均攤 O(1)
//   3. 移動建構子加 noexcept → 重新配置時使用移動，效率更高
//   4. 重新配置後所有迭代器/指標/引用全部失效
//   5. insert/erase 使插入/刪除點之後的迭代器失效
//   6. 迴圈中安全刪除：用 erase() 的回傳值取回有效迭代器
//   7. erase-remove 慣用法：O(n) 批量刪除，推薦使用
//   8. reserve() 預先配置，避免重新配置
//   9. shrink_to_fit() 釋放多餘容量
// ================================================================
