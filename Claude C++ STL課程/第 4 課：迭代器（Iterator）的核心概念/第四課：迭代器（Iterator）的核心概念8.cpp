// =============================================================================
//  第四課：迭代器的核心概念 8  —  迭代器失效：邊走邊刪的正確寫法
// =============================================================================
//
// 【主題資訊 Information】
//   簽名：iterator vector<T>::erase(const_iterator pos);
//         iterator vector<T>::erase(const_iterator first, const_iterator last);
//   回傳：**被刪元素之後那個元素**的迭代器（若刪的是最後一個，則回傳 end()）。
//   標準版本：erase 自 C++98；C++11 起參數型別由 iterator 改為 const_iterator。
//   複雜度：vector::erase 為 O(N)（要把後面的元素往前搬）；
//           list::erase 為 O(1)（只重接指標）。
//   標頭檔：<vector>
//
// 【詳細解釋 Explanation】
//
// 【1. 「迭代器失效」到底發生了什麼】
//   vector 的迭代器本質上是指向陣列元素的指標。
//   erase(it) 之後，libstdc++ 會用 memmove 把 it 之後的元素整段往前搬一格，
//   並讓 size 減一。於是：
//     - it 所指的位址現在裝的是「原本的下一個元素」（值變了！）
//     - 最後一個位置變成「已解構、不再屬於容器」的區域
//     - end() 往前挪了一格
//   標準因此規定：**erase 之後，被刪位置及其後的所有迭代器、指標、參考全部失效**。
//   注意「失效」不等於「一定會崩潰」。實務上 it 通常還指著合法記憶體，
//   讀起來甚至像是「跳過了一個元素」—— 這才是危險之處：
//   它是未定義行為，但症狀可能只是安靜的錯誤結果。
//
// 【2. 為什麼 for(...; ++it) { if(...) v.erase(it); } 一定錯】
//   兩個獨立的錯誤疊在一起：
//     (a) erase(it) 之後 it 已失效，接著 ++it 是對失效迭代器操作 → UB。
//     (b) 就算僥倖沒崩潰，元素往前搬 + 再 ++it，等於**跳過一個元素**：
//         連續兩個 3 只會刪掉第一個。
//   後者非常常被誤診成「erase 有 bug」，實際上是使用方式錯了。
//
// 【3. 正解：讓 erase 的回傳值推進迴圈】
//   erase 回傳「被刪元素之後那個元素」的有效迭代器，正好可以直接接手：
//       for (auto it = v.begin(); it != v.end(); /* 這裡不 ++ */) {
//           if (要刪) it = v.erase(it);   // erase 已經幫你前進了
//           else      ++it;               // 沒刪才自己前進
//       }
//   關鍵是「刪了就不要再 ++」。這個模式對 vector / list / deque / set / map
//   全部適用，是最通用的邊走邊刪寫法。
//
// 【4. vector 專用的更好選擇：erase-remove 慣用法】
//   上面的迴圈對 vector 有個效能問題：每次 erase 都是 O(N) 的搬移，
//   刪 K 個元素就是 O(N×K)。改用：
//       v.erase(std::remove(v.begin(), v.end(), val), v.end());
//   std::remove 一次走訪就把要保留的元素往前壓實（O(N)），
//   回傳新的邏輯尾端；再用一次 erase 砍掉尾巴（O(1) 的 size 調整 + 解構）。
//   總複雜度 O(N)。C++20 更進一步提供 std::erase(v, val) 一行完成。
//
// 【概念補充 Concept Deep Dive】
//   各容器的失效規則差異極大，這往往是選容器的決定性因素：
//     vector   push_back 若觸發重新配置 → **全部**迭代器/指標/參考失效；
//              未觸發則只有 end() 失效。
//              erase → 被刪位置及其後全部失效。
//     deque    插入到中間 → 全部迭代器失效（但參考可能仍有效）；
//              插入到兩端 → 迭代器失效，但**參考與指標仍有效**（很特別）。
//     list     插入 → 不影響任何既有迭代器；
//              erase → 只有被刪的那個失效，其餘全部有效。
//     set/map  規則同 list（節點式容器，元素不會被搬動）。
//   這就是為什麼「需要長期持有元素位置」的設計（例如 LRU cache）
//   幾乎一定選 list 或 map，而不是 vector。
//
// 【注意事項 Pay Attention】
//   1. erase 之後絕不可再使用原本那個迭代器 —— 要用它的回傳值。
//   2. 「刪了就別再 ++」：寫成 it = v.erase(it) 之後若又 ++it 會跳過元素。
//   3. 範圍 for 內修改容器同樣危險：它內部持有 __begin/__end，
//      容器一變就可能失效。要邊走邊刪請用明確的迭代器迴圈。
//   4. std::remove **不會**改變 size；它只是搬移並回傳新邏輯尾端，
//      必須配合 erase 才真的刪除（見同課第 9 個範例）。
//   5. 失效的迭代器「看起來還能用」不代表沒事 —— 那是未定義行為，
//      不同編譯器、最佳化層級、容器大小都可能給出不同結果。
//      要驗證請用 -D_GLIBCXX_DEBUG 或 AddressSanitizer。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】迭代器失效與安全刪除
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 在 for 迴圈中呼叫 vector::erase 為什麼危險？正確寫法是什麼？
//     答：erase 之後，被刪位置及其後的所有迭代器全部失效，
//         再對它做 ++ 是未定義行為；即使沒崩潰也會因為元素前移而**跳過一個元素**
//         （連續兩個相同值只會刪掉第一個）。
//         正確寫法是用 erase 的回傳值推進：
//             for (auto it = v.begin(); it != v.end(); ) {
//                 if (cond) it = v.erase(it); else ++it;
//             }
//     追問：對 vector 有沒有更有效率的做法？
//           → erase-remove 慣用法：v.erase(std::remove(b, e, val), v.end())。
//             逐一 erase 是 O(N×K)，erase-remove 只要 O(N)。
//             C++20 可直接寫 std::erase(v, val)。
//
// 🔥 Q2. vector 的 push_back 會讓哪些迭代器失效？
//     答：看有沒有觸發重新配置。若 size == capacity，push_back 會配置一塊更大的
//         記憶體並搬移全部元素 → **所有**迭代器、指標、參考全部失效。
//         若容量還夠，則只有 end() 失效（它往後移了一格）。
//         因為呼叫端通常無法預知是哪種情況，安全的假設是「都可能失效」。
//     追問：list 呢？
//           → list 的插入**不會**使任何既有迭代器失效，
//             erase 也只讓被刪的那一個失效。這是節點式容器的關鍵優勢，
//             也是 LRU cache 這類需要長期持有位置的設計選 list 的原因。
//
// ⚠️ 陷阱. 用「erase 之後印出來看起來是對的」來證明程式沒問題，錯在哪？
//     答：迭代器失效是**未定義行為**，不是「一定崩潰」。
//         失效的 vector 迭代器通常還指著已配置的合法記憶體，
//         讀起來是舊資料或已被搬移的資料 —— 程式照跑、結果可能剛好對，
//         但換個編譯器、換個最佳化等級、或資料量變大觸發重新配置時就會壞。
//     為什麼會錯：把「沒有立即崩潰」當成「正確」。
//         UB 最惡劣的性質就是它可以表現得完全正常。
//         要驗證這類問題必須用工具：-D_GLIBCXX_DEBUG（libstdc++ 的檢查模式）
//         會在失效迭代器被使用時直接中止並指出問題，
//         或用 -fsanitize=address 抓越界存取。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 27. Remove Element
//   題目：就地移除陣列中所有等於 val 的元素，回傳剩餘長度 k。
//   為什麼用到本主題：這一題就是「邊走邊刪」的最小化版本。
//         下面刻意給出兩種正確解法並對照它們的複雜度差異：
//           (a) erase 回傳值迴圈：通用於所有容器，但對 vector 是 O(N×K)
//           (b) erase-remove 慣用法：只適用支援搬移的序列容器，O(N)
//         這正是本課要傳達的重點 —— 正確之外還要選對工具。
//   複雜度：(a) O(N×K)、(b) O(N)；空間皆 O(1)。
// -----------------------------------------------------------------------------

// (a) 通用寫法：用 erase 的回傳值推進
int removeElementByEraseLoop(std::vector<int>& nums, int val) {
    for (auto it = nums.begin(); it != nums.end(); /* 不在這裡 ++ */) {
        if (*it == val) {
            it = nums.erase(it);   // erase 回傳下一個有效迭代器
        } else {
            ++it;                  // 沒刪才自己前進
        }
    }
    return static_cast<int>(nums.size());
}

// (b) vector 的正解：erase-remove 慣用法，只走一趟
int removeElementByEraseRemove(std::vector<int>& nums, int val) {
    nums.erase(std::remove(nums.begin(), nums.end(), val), nums.end());
    return static_cast<int>(nums.size());
}

// -----------------------------------------------------------------------------
// 【日常實務範例】連線池清理：移除已斷線的 session
//   情境：伺服器維護一份線上連線清單，心跳檢查後要把逾時的連線移除，
//         同時統計被清掉幾條、剩下幾條。
//   為什麼用到本主題：這是生產環境最常見的「邊走邊刪」場景，
//         而且條件不是單一值而是述詞（逾時），所以要用 remove_if。
//         這裡也示範為什麼不能在範圍 for 裡直接刪 —— 註解中有說明。
// -----------------------------------------------------------------------------
struct Session {
    std::string id;
    int         idle_seconds;
};

int pruneIdleSessions(std::vector<Session>& pool, int timeout_seconds) {
    std::size_t before = pool.size();

    // 錯誤示範（已註解）：範圍 for 內修改容器 → 內部 __begin/__end 失效，UB
    // for (const auto& s : pool) {
    //     if (s.idle_seconds > timeout_seconds) pool.erase(...);   // 絕對不可以
    // }

    // 正解：remove_if 把要保留的往前壓實，再一次 erase 砍掉尾巴（O(N)）
    pool.erase(std::remove_if(pool.begin(), pool.end(),
                              [timeout_seconds](const Session& s) {
                                  return s.idle_seconds > timeout_seconds;
                              }),
               pool.end());

    return static_cast<int>(before - pool.size());
}

int main() {
    std::vector<int> vec = {1, 2, 3, 4, 5};

    // 危險！在遍歷時修改容器
    /*
    for (auto it = vec.begin(); it != vec.end(); ++it) {
        if (*it == 3) {
            vec.erase(it);  // 刪除後，it 失效！
            // 繼續使用 it 是未定義行為
        }
    }
    */

    // 正確做法：使用 erase 的回傳值
    std::cout << "原始: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;

    for (auto it = vec.begin(); it != vec.end(); /* 不在這裡 ++ */) {
        if (*it == 3) {
            it = vec.erase(it);  // erase 回傳下一個有效迭代器
        } else {
            ++it;
        }
    }

    std::cout << "刪除 3 後: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;

    // 「刪了又 ++」會跳過元素 —— 用連續重複值演示（安全版，不觸發 UB）
    std::cout << "\n=== 為什麼「刪了又 ++」會漏掉元素 ===" << std::endl;
    std::cout << "  資料: 1 3 3 3 5，目標：刪掉全部的 3" << std::endl;

    // 正確：刪了就不 ++
    std::vector<int> good = {1, 3, 3, 3, 5};
    for (auto it = good.begin(); it != good.end(); ) {
        if (*it == 3) it = good.erase(it);
        else          ++it;
    }
    std::cout << "  正確寫法（刪了不 ++）: ";
    for (int n : good) std::cout << n << " ";
    std::cout << "  ← 3 全部刪光" << std::endl;

    // 模擬「刪了又 ++」的效果（用索引重現，避免真的觸發 UB）
    std::vector<int> bad = {1, 3, 3, 3, 5};
    for (std::size_t i = 0; i < bad.size(); ++i) {
        if (bad[i] == 3) {
            bad.erase(bad.begin() + static_cast<std::ptrdiff_t>(i));
            // 這裡「沒有」i--，等同於刪完又前進一格
        }
    }
    std::cout << "  錯誤寫法（刪了又前進）: ";
    for (int n : bad) std::cout << n << " ";
    std::cout << "  ← 中間那個 3 被跳過了！" << std::endl;

    // 重新配置會讓「全部」迭代器失效（觀察 capacity 變化，不解參考舊迭代器）
    std::cout << "\n=== push_back 觸發重新配置 → 全部失效 ===" << std::endl;
    std::vector<int> g;
    g.reserve(2);
    g.push_back(1);
    g.push_back(2);
    const int* before_ptr = g.data();
    std::cout << "  push_back 前 capacity=" << g.capacity() << std::endl;
    g.push_back(3);                                   // 容量不足 → 重新配置
    const int* after_ptr = g.data();
    std::cout << "  push_back 後 capacity=" << g.capacity() << std::endl;
    std::cout << "  底層緩衝區位址是否改變: "
              << (before_ptr != after_ptr ? "已改變 → 舊迭代器全部失效"
                                          : "未改變 → 只有 end() 失效")
              << std::endl;
    std::cout << "  （位址值本身每次執行都不同，這裡只比較「有沒有變」）" << std::endl;

    std::cout << "\n=== LeetCode 27. Remove Element ===" << std::endl;
    std::vector<int> a = {3, 2, 2, 3};
    int ka = removeElementByEraseLoop(a, 3);
    std::cout << "  (a) erase 迴圈    [3,2,2,3] 移除 3 → k=" << ka << "，內容 = ";
    for (int n : a) std::cout << n << " ";
    std::cout << std::endl;

    std::vector<int> b = {0, 1, 2, 2, 3, 0, 4, 2};
    int kb = removeElementByEraseRemove(b, 2);
    std::cout << "  (b) erase-remove  [0,1,2,2,3,0,4,2] 移除 2 → k=" << kb << "，內容 = ";
    for (int n : b) std::cout << n << " ";
    std::cout << std::endl;
    std::cout << "  兩者結果相同，但 (b) 是 O(N)、(a) 是 O(N×K)" << std::endl;

    std::cout << "\n=== 日常實務：清理逾時連線 ===" << std::endl;
    std::vector<Session> pool = {
        {"sess-1001",   12}, {"sess-1002",  305}, {"sess-1003",   45},
        {"sess-1004",  980}, {"sess-1005",    3}, {"sess-1006",  421},
    };
    std::cout << "  清理前 " << pool.size() << " 條連線" << std::endl;
    int pruned = pruneIdleSessions(pool, 300);
    std::cout << "  逾時門檻 300 秒，清掉 " << pruned << " 條" << std::endl;
    std::cout << "  剩餘連線: ";
    for (const Session& s : pool) std::cout << s.id << "(" << s.idle_seconds << "s) ";
    std::cout << std::endl;

    return 0;
}

// 注意：上面「底層緩衝區位址」的比較刻意只印出「有沒有改變」而不印位址本身 ——
//       實際位址由配置器決定，每次執行都不同，不適合寫成固定的預期輸出。
//       本機 g++ 15.2 / libstdc++ 的 vector 成長倍率為 2×（實作定義），
//       因此 reserve(2) 後第三次 push_back 必定觸發重新配置。

// 編譯: g++ -std=c++17 -Wall -Wextra 第四課：迭代器（Iterator）的核心概念8.cpp -o demo8

// === 預期輸出 ===
// 原始: 1 2 3 4 5
// 刪除 3 後: 1 2 4 5
//
// === 為什麼「刪了又 ++」會漏掉元素 ===
//   資料: 1 3 3 3 5，目標：刪掉全部的 3
//   正確寫法（刪了不 ++）: 1 5   ← 3 全部刪光
//   錯誤寫法（刪了又前進）: 1 3 5   ← 中間那個 3 被跳過了！
//
// === push_back 觸發重新配置 → 全部失效 ===
//   push_back 前 capacity=2
//   push_back 後 capacity=4
//   底層緩衝區位址是否改變: 已改變 → 舊迭代器全部失效
//   （位址值本身每次執行都不同，這裡只比較「有沒有變」）
//
// === LeetCode 27. Remove Element ===
//   (a) erase 迴圈    [3,2,2,3] 移除 3 → k=2，內容 = 2 2
//   (b) erase-remove  [0,1,2,2,3,0,4,2] 移除 2 → k=5，內容 = 0 1 3 0 4
//   兩者結果相同，但 (b) 是 O(N)、(a) 是 O(N×K)
//
// === 日常實務：清理逾時連線 ===
//   清理前 6 條連線
//   逾時門檻 300 秒，清掉 3 條
//   剩餘連線: sess-1001(12s) sess-1003(45s) sess-1005(3s)
