// =============================================================================
//  第 11 課：vector 的容量管理：size、capacity、reserve8.cpp
//  —  清空 vector 的三種方式：clear、swap trick、clear + shrink_to_fit
// =============================================================================
//
// 【主題資訊 Information】
//   void clear() noexcept;                  // C++11 起 noexcept；C++20 constexpr
//   std::vector<T>(v).swap(v);              // swap trick（C++98 唯一的縮容手段）
//   v.clear(); v.shrink_to_fit();           // C++11 之後的標準做法
//
//   標頭檔：<vector>
//   複雜度：clear() → O(size())（要逐一呼叫解構子；對 trivially destructible
//                    的型別如 int，實務上是 O(1)）
//           swap()  → O(1)（只交換三根內部指標）
//           swap trick 整體 → O(size())（複製建構那一步）
//           shrink_to_fit() → 最壞 O(size())
//
//   三者的效果對照（capacity 一欄為 libstdc++ 實測）：
//   ┌────────────────────────┬────────┬─────────────┬──────────────────┐
//   │ 做法                    │ size   │ capacity    │ 記憶體還給系統？  │
//   ├────────────────────────┼────────┼─────────────┼──────────────────┤
//   │ clear()                │ 0      │ **不變**    │ 否                │
//   │ vector<T>().swap(v)    │ 0      │ 0           │ 是（保證重新配置）│
//   │ clear() + shrink_to_fit│ 0      │ 0（實測）   │ 是（但是非強制請求）│
//   └────────────────────────┴────────┴─────────────┴──────────────────┘
//
// 【詳細解釋 Explanation】
//
// 【1. clear() 為什麼不釋放記憶體 —— 這是特性不是缺陷】
// clear() 只做兩件事：對每個元素呼叫解構子、把 size 設為 0。capacity 完全
// 不動，那塊 buffer 仍然握在手上。很多人第一次看到會覺得「這不是記憶體
// 洩漏嗎」，但這其實是刻意設計：
//   * 清空往往是為了**重複使用**。典型如「每輪讀一批資料 → 處理 → 清空 →
//     再讀下一批」的迴圈。保留 capacity 讓第二輪之後完全免於重新配置。
//   * 若 clear() 自動釋放，上述迴圈每輪都要重新配置 + 重新成長，
//     那是效能災難。
// 所以正確的心智模型是：**clear() 清的是「元素」，不是「記憶體」。**
//
// 【2. swap trick：C++11 之前唯一的縮容手段】
//     std::vector<int>().swap(v);
// 拆成三步看：
//   ① `std::vector<int>()` 建立一個空的暫時 vector（capacity 0）。
//      注意這裡若寫成 `std::vector<int>(v)` 則是**複製** v，
//      得到 capacity 剛好等於 size 的緊湊副本 —— 那是「縮容」版本。
//      本檔用的 `std::vector<int>()` 是空的 —— 那是「完全釋放」版本。
//   ② `.swap(v)` 交換兩者的內部三根指標，O(1)，不搬移任何元素。
//   ③ 敘述結束，暫時物件解構，帶走**原本 v 那塊過大的 buffer**。
// 結果：v 拿到空的（或緊湊的）buffer，舊記憶體被釋放。
//
// 兩個變體要分清楚：
//     std::vector<T>().swap(v);   // 完全清空 + 釋放  → size 0, capacity 0
//     std::vector<T>(v).swap(v);  // 保留內容、只縮容 → size 不變, capacity == size
//
// 【3. 為什麼 C++11 之後仍值得知道 swap trick】
// 因為 shrink_to_fit() 是 **non-binding request**，標準允許實作忽略它
// （見 6.cpp）。而 swap trick **一定會**重新配置 —— 它不是請求，是實實在在
// 的建構 + 交換 + 解構。所以在「必須保證釋放」的場合，swap trick 反而更可靠。
// 此外維護 C++98 舊碼時，它是唯一選項。
//
// 【4. 該選哪一個】
//   * 之後還要重新填入資料 → **clear()**。保留 capacity 就是最大的優點。
//   * 確定不再需要這些記憶體、且希望意圖清楚 → clear() + shrink_to_fit()。
//   * 需要**保證**釋放（不接受實作忽略） → std::vector<T>().swap(v)。
//   * 只想縮掉多餘容量但保留內容 → shrink_to_fit() 或
//     std::vector<T>(v).swap(v)。
//
// 【概念補充 Concept Deep Dive】
// (A) 為什麼 swap 是 O(1)
//   vector 的全部狀態就是三根指標（_M_start / _M_finish / _M_end_of_storage）。
//   swap 只是交換這三根指標（以及可能的 allocator），完全不碰元素。
//   這也是為什麼 std::swap 對 vector 有特化 —— 泛型版本的
//   「複製到暫存 → 互相賦值」對 vector 是 O(n) 的災難。
//
// (B) clear() 是 noexcept，但 swap trick 不是
//   clear() 只解構元素、不配置記憶體，所以標了 noexcept。
//   swap trick 中的複製/預設建構可能配置記憶體 → 可能丟 bad_alloc。
//   在 noexcept 函式裡想清空容器，只能用 clear()。
//
// (C) clear() 之後 data() 指標還有效嗎？
//   clear() 不釋放 buffer，所以 data() 通常仍指向同一塊記憶體。但標準規定
//   clear() 會使所有 iterator/pointer/reference **失效**（因為元素已被解構，
//   那些指標指向的物件生命期已結束）。「位址沒變」不等於「可以用」——
//   讀取已解構物件是 UB。本檔第二段用「位址是否改變」示範這個區別。
//
// 【注意事項 Pay Attention】
// 1. **clear() 不釋放記憶體**，capacity 維持原值。想釋放要額外動作。
// 2. clear() 會使所有 iterator/pointer/reference 失效，即使 data() 位址不變。
// 3. swap trick 的兩個變體差很多：`vector<T>()` 是清空、`vector<T>(v)` 是縮容。
//    寫錯會把資料整個丟掉。
// 4. shrink_to_fit() 是 **non-binding request**，標準不保證 capacity 變 0；
//    本檔輸出的 0 是 libstdc++ 實測。需要保證時用 swap trick。
// 5. 本檔所有 capacity 數值皆為 libstdc++ / GCC 15.2 實測，非標準保證。
// 6. C++11 起 vector 有 move assignment，`v = std::vector<T>();` 也能達到
//    類似 swap trick 的效果，但涉及 allocator 傳播規則，語意比 swap 微妙，
//    要求明確保證時仍以 swap trick 最直白。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】clear / swap trick / shrink_to_fit
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. clear() 之後 capacity 會變成 0 嗎？
//     答：不會。clear() 只解構元素並把 size 設為 0，capacity 完全不變，
//         那塊 buffer 仍然握在手上。這是刻意設計 —— 清空多半是為了重複
//         使用，保留 capacity 讓下一輪填入完全免於重新配置。
//     追問：那要怎麼真的釋放？
//         → clear() + shrink_to_fit()（C++11，但是非強制請求），
//           或 std::vector<T>().swap(v)（保證釋放）。
//
// 🔥 Q2. 解釋 `std::vector<int>().swap(v);` 這一行做了什麼、為什麼有效。
//     答：① 建立一個空的暫時 vector（capacity 0）；② swap 交換兩者的內部
//         三根指標，O(1)；③ 敘述結束時暫時物件解構，帶走原本 v 那塊過大的
//         buffer。結果 v 的 size 與 capacity 都是 0，記憶體真的還給系統。
//     追問：C++11 有 shrink_to_fit 了，為什麼還要知道這招？
//         → 因為 shrink_to_fit 是 non-binding request，實作可以忽略；
//           swap trick 一定會重新配置，需要**保證**釋放時更可靠。
//           另外維護 C++98 舊碼時它是唯一選項。
//
// 🔥 Q3. `std::vector<T>().swap(v)` 和 `std::vector<T>(v).swap(v)` 差在哪？
//     答：前者用**空的**暫時物件 → v 變成 size 0、capacity 0（清空 + 釋放）。
//         後者用 v **複製建構**暫時物件（capacity 剛好等於 size）→ 交換後
//         v 內容不變但 capacity 縮到剛好（純縮容）。
//     追問：後者為什麼能得到剛好的容量？
//         → 因為複製建構只需容納來源的 size() 個元素，實作沒有理由多配。
//           不過嚴格說這也是實作行為，標準未規定複製後的 capacity。
//
// ⚠️ 陷阱 1. 「clear() 之後 data() 位址沒變，所以舊指標還能用」——對嗎？
//     答：不對。clear() 確實不釋放 buffer，位址通常不變，但標準規定 clear()
//         會使所有 iterator/pointer/reference 失效 —— 因為元素已經被解構，
//         那些指標指向的物件生命期已結束。讀取已解構的物件是 UB。
//     為什麼會錯：把「記憶體位址仍可存取」誤當成「物件仍然存在」。
//         C++ 的物件生命期和記憶體是否已配置是兩回事。
//
// ⚠️ 陷阱 2. 迴圈裡每輪處理完都寫 `v.clear(); v.shrink_to_fit();` —— 好嗎？
//     答：這是效能反模式。若下一輪還要填回差不多的資料量，shrink_to_fit
//         會把 buffer 還掉，下一輪又得從頭幾何成長回去 —— 每輪都白付一次
//         縮容 O(n) 加一整串重新配置。這種情境**只用 clear()** 才對。
//     為什麼會錯：以為「盡快釋放記憶體」永遠是美德，忽略了 capacity 被
//         保留正是為了讓重複使用免於重新配置。
// ═══════════════════════════════════════════════════════════════════════════

#include <vector>
#include <iostream>

// 印出目前的 size / capacity
static void report(const char* label, const std::vector<int>& v) {
    std::cout << label << " size=" << v.size()
              << ", capacity=" << v.capacity() << std::endl;
}

int main() {
    std::vector<int> v = {1, 2, 3, 4, 5};
    v.reserve(100);

    std::cout << "=== 起點 ===" << std::endl;
    report("初始:          ", v);

    // ---- 方法一：clear（保留 capacity，供重複使用）----
    std::cout << "\n=== 方法一：clear()（保留 capacity）===" << std::endl;
    const int* beforeClear = v.data();
    v.clear();
    report("clear():       ", v);
    std::cout << "資料位址是否改變: "
              << (v.data() != beforeClear ? "是" : "否")
              << "（但所有 iterator/pointer 仍然失效：元素已被解構）"
              << std::endl;

    // ---- 方法二：swap trick（保證釋放記憶體）----
    std::cout << "\n=== 方法二：swap trick（C++11 前唯一手段）===" << std::endl;
    v = {1, 2, 3, 4, 5};
    v.reserve(100);
    report("swap 前:       ", v);
    std::vector<int>().swap(v);   // 空的暫時物件 → 清空 + 釋放
    report("swap 後:       ", v);

    // ---- 方法三：clear + shrink_to_fit（C++11 之後的標準做法）----
    std::cout << "\n=== 方法三：clear() + shrink_to_fit()（C++11 起）==="
              << std::endl;
    v = {1, 2, 3, 4, 5};
    v.reserve(100);
    report("處理前:        ", v);
    v.clear();
    report("clear() 後:    ", v);
    v.shrink_to_fit();
    report("再 shrink 後:  ", v);
    std::cout << "註: capacity 降到 0 是 libstdc++ 實測；"
              << "shrink_to_fit 是 non-binding request，標準允許實作忽略。"
              << std::endl;

    // ---- 變體：只縮容、保留內容 ----
    std::cout << "\n=== 變體：vector<T>(v).swap(v) 只縮容、保留內容 ==="
              << std::endl;
    v = {1, 2, 3, 4, 5};
    v.reserve(100);
    report("縮容前:        ", v);
    std::vector<int>(v).swap(v);  // 用 v 複製建構 → capacity 剛好等於 size
    report("縮容後:        ", v);
    std::cout << "內容仍在: [";
    for (std::size_t i = 0; i < v.size(); ++i) {
        if (i != 0) std::cout << ' ';
        std::cout << v[i];
    }
    std::cout << "]" << std::endl;

    // -------------------------------------------------------------------------
    // 【日常實務範例】log 批次寫入緩衝區：每輪 clear() 重複使用同一塊記憶體
    //   真實情境：日誌收集器每滿 N 筆就 flush 一次，然後清空緩衝區繼續收。
    //   這裡刻意**只用 clear()、不用 shrink_to_fit()** —— 保留 capacity
    //   讓後續每一輪都免於重新配置，是這個模式的整個重點。
    // -------------------------------------------------------------------------
    std::cout << "\n=== 日常實務: log buffer 重複使用 ===" << std::endl;
    {
        std::vector<int> logBuffer;   // 存 log 的序號，簡化示範
        const int batchSize  = 500;
        const int batchCount = 4;
        int reallocations = 0;
        std::size_t lastCap = logBuffer.capacity();

        for (int batch = 0; batch < batchCount; ++batch) {
            for (int i = 0; i < batchSize; ++i) {
                logBuffer.push_back(batch * batchSize + i);
                if (logBuffer.capacity() != lastCap) {
                    ++reallocations;
                    lastCap = logBuffer.capacity();
                }
            }
            // flush 到磁碟（此處省略），然後清空但**保留 capacity**
            logBuffer.clear();
        }

        std::cout << "處理 " << batchCount << " 批 x " << batchSize
                  << " 筆，全程只重新配置 " << reallocations << " 次"
                  << std::endl;
        std::cout << "結束時 size=" << logBuffer.size()
                  << ", capacity=" << logBuffer.capacity()
                  << "（capacity 留著給下一輪用）" << std::endl;
        std::cout << "若每輪都 shrink_to_fit()，第 2 批之後每批都要"
                  << "重新從頭成長一次。" << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 11 課：vector 的容量管理：size、capacity、reserve8.cpp" -o demo8

// 註：以下 capacity 數值為 libstdc++ / GCC 15.2 實測，非標準保證。
//     其中 clear()+shrink_to_fit() 後的 capacity=0 尤其是實作行為 ——
//     標準規定 shrink_to_fit 是 non-binding request，允許實作完全忽略。
//     只有 swap trick 是「保證會重新配置」的做法。

// === 預期輸出 ===
// === 起點 ===
// 初始:           size=5, capacity=100
//
// === 方法一：clear()（保留 capacity）===
// clear():        size=0, capacity=100
// 資料位址是否改變: 否（但所有 iterator/pointer 仍然失效：元素已被解構）
//
// === 方法二：swap trick（C++11 前唯一手段）===
// swap 前:        size=5, capacity=100
// swap 後:        size=0, capacity=0
//
// === 方法三：clear() + shrink_to_fit()（C++11 起）===
// 處理前:         size=5, capacity=100
// clear() 後:     size=0, capacity=100
// 再 shrink 後:   size=0, capacity=0
// 註: capacity 降到 0 是 libstdc++ 實測；shrink_to_fit 是 non-binding request，標準允許實作忽略。
//
// === 變體：vector<T>(v).swap(v) 只縮容、保留內容 ===
// 縮容前:         size=5, capacity=100
// 縮容後:         size=5, capacity=5
// 內容仍在: [1 2 3 4 5]
//
// === 日常實務: log buffer 重複使用 ===
// 處理 4 批 x 500 筆，全程只重新配置 10 次
// 結束時 size=0, capacity=512（capacity 留著給下一輪用）
// 若每輪都 shrink_to_fit()，第 2 批之後每批都要重新從頭成長一次。
