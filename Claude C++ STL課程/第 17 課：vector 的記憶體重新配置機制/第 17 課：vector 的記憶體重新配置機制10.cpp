// =============================================================================
//  第 17 課-10：shrink_to_fit() —— 把「刪掉了但沒還回去」的記憶體交出來
// =============================================================================
//
// 【主題資訊 Information】
//   void vector<T>::shrink_to_fit();
//     語意：請求把 capacity() 降到 size()。
//     注意：這是「非強制性請求（non-binding request）」——
//           標準明文允許實作完全不動作。
//   標準版本：C++11 加入
//   複雜度：最多 O(size())（可能需要配置新記憶體並搬移全部元素）
//   失效：若真的改變了容量，所有 iterator / pointer / reference 全部失效
//   標頭檔：<vector>
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼 erase / clear 不自動歸還記憶體】
//   vector 的設計假設是「容量會被重複使用」。
//   若每次刪除都縮小容量，一個「加一批、刪一批、再加一批」的
//   使用樣式就會不斷 malloc/free，而每次都要搬移全部元素。
//   保留容量等於保留一塊已經暖好的緩衝區，下次再長回來時零成本。
//   代價是：一個曾經達到過峰值的 vector，會一直佔著峰值大小的記憶體，
//   直到它被解構或你明確要求縮小。
//
// 【2. 為什麼它只是「請求」而不是命令】
//   縮小容量在物理上通常必須：配置一塊剛好大小的新記憶體 →
//   搬移全部元素 → 釋放舊的。這需要在「舊的還沒放掉」時
//   同時持有兩塊記憶體，因此可能配置失敗。
//   標準不想強制一個「可能丟例外、可能反而更耗記憶體」的操作，
//   所以定義成非強制性請求。
//   實務上 libstdc++ 與 libc++ 都會照做，但不能寫程式去依賴它。
//
// 【3. 它可能反而讓事情變糟的三種情況】
//   (a) 搬移成本：縮容要把所有元素搬到新位置，O(n) 的複製或移動；
//   (b) 峰值記憶體：搬移期間新舊兩塊同時存在，瞬間用量反而更高；
//   (c) 迭代器全部失效，和重新配置一樣危險。
//   所以不要養成「刪完就 shrink」的習慣——
//   只在「明確知道這個 vector 之後不會再長大、而且它佔的記憶體值得回收」
//   時才用。
//
// 【4. C++11 之前的做法：swap trick】
//     std::vector<T>(v).swap(v);
//   建立一個以 v 為來源的暫時 vector（其 capacity 剛好等於 v.size()），
//   再和 v 交換內部指標，暫時物件解構時就把原本的大塊記憶體帶走了。
//   要清空並徹底釋放則是 std::vector<T>().swap(v);。
//   C++11 之後直接用 shrink_to_fit() 即可，但讀舊程式碼時會看到這個寫法。
//
// 【5. clear() 之後 capacity 依然不變】
//   clear() 解構所有元素、size 歸零，但一塊記憶體都不還。
//   要真正釋放必須 clear() 之後再 shrink_to_fit()，
//   或直接用 vector<T>().swap(v)。
//   這是「記憶體用量看起來只增不減」最常見的原因之一。
//
// 【概念補充 Concept Deep Dive】
//   ▸ 為什麼 capacity 縮不回去是「設計」而非「缺陷」
//     這和 std::string 的 SSO、配置器的 free list 是同一種思路：
//     記憶體配置是昂貴操作（可能進核心、可能碰鎖、可能造成碎片），
//     所以標準庫傾向「拿到手就先留著」。
//     真正在意記憶體上限的場景，應該從一開始就控制容量（reserve 到合理值），
//     而不是事後 shrink。
//   ▸ shrink_to_fit 對 size()==0 的 vector
//     實作通常會把緩衝區完全釋放，capacity 變成 0。
//     本機（GCC 15.2 / libstdc++）實測即是如此，但同樣屬實作定義。
//   ▸ deque 的 shrink_to_fit 行為不同
//     deque 是分段配置的，縮容只會釋放完全空掉的區塊，
//     成本與效果都和 vector 不一樣。
//     這也是「不要把 vector 的直覺套到其他容器」的又一個例子。
//
// 【注意事項 Pay Attention】
//   1. shrink_to_fit 是非強制性請求，標準允許實作完全不動作。
//   2. 它若真的縮容，所有 iterator / pointer / reference 全部失效。
//   3. 縮容過程可能同時持有新舊兩塊記憶體，瞬間用量反而更高。
//   4. clear() 不會歸還記憶體，capacity 維持不變。
//   5. 不要在迴圈裡反覆 shrink——那會把倍率成長的好處全部抵銷。
//   6. 縮容後的實際 capacity 是實作定義，不保證恰好等於 size()。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】shrink_to_fit 與容量管理
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 對一個 vector 呼叫 clear() 之後，它佔用的記憶體會被釋放嗎？
//     答：不會。clear() 解構所有元素、把 size 歸零，
//         但 capacity 完全不變，那塊緩衝區仍然被持有著。
//         要真正歸還必須再呼叫 shrink_to_fit()，
//         或使用 C++11 前的慣用法 std::vector<T>().swap(v)。
//     追問：為什麼標準要這樣設計？
//         → 因為 vector 假設容量會被重複使用。若每次 clear 都釋放，
//           「清空再填滿」的樣式就會不斷 malloc/free，
//           把倍率成長換來的均攤 O(1) 全部賠掉。
//
// 🔥 Q2. shrink_to_fit() 一定會把 capacity 降到 size 嗎？
//     答：不一定。標準明文規定它是「非強制性請求」，
//         允許實作完全不動作，也不保證縮完之後 capacity 恰好等於 size。
//         原因是縮容通常要配置新記憶體並搬移全部元素，
//         這個操作可能失敗，標準不願強制。
//         實務上 libstdc++/libc++ 都會照做，但程式碼不能依賴它。
//     追問：C++11 之前怎麼做？
//         → swap trick：std::vector<T>(v).swap(v);
//           用一個容量剛好的暫時物件和 v 交換，
//           讓暫時物件在解構時把大塊記憶體帶走。
//
// ⚠️ 陷阱. 「既然 shrink_to_fit 能省記憶體，那每次 erase 完都呼叫一次
//          應該是好習慣吧？」
//     答：恰恰相反，那通常是效能災難。每次 shrink 都是一次
//         O(n) 的配置＋搬移＋釋放，而且會讓所有迭代器失效；
//         如果之後又要 push_back，還得從頭再倍率成長一次。
//         等於把 vector 精心設計的均攤 O(1) 完全破壞掉。
//     為什麼會錯：把「記憶體用量」當成唯一的最佳化目標，
//         忽略了 capacity 本身就是一種快取。
//         正確策略是：一開始就用 reserve 控制在合理值，
//         只在「確定之後不會再長大」的時間點做一次 shrink。
// ═══════════════════════════════════════════════════════════════════════════

#include <algorithm>  // std::remove_if
#include <iostream>
#include <string>
#include <vector>

// -----------------------------------------------------------------------------
// 【日常實務範例】長駐服務的資料快取：尖峰過後把記憶體交還給系統
//   情境：一個長期執行的服務，每天尖峰時段會把幾十萬筆熱資料載入
//         記憶體快取；離峰時把大部分逐出，只留少量常用資料。
//   為什麼用本主題：如果只做 erase，process 的 RSS 會一直停在
//         尖峰時的高點不下來（因為 capacity 沒還）。
//         在「確定尖峰已過、不會馬上再長回去」的時間點做一次
//         shrink_to_fit，是把記憶體真正交還給系統的唯一方式。
//         注意：這個動作只該在離峰做一次，不能每次逐出都做。
// -----------------------------------------------------------------------------
struct CacheEntry {
    int         key;
    std::string payload;
    int         hits;
};

// 逐出低命中率的項目。回傳 (逐出筆數)，並示範「只縮一次」的正確時機。
std::size_t evictColdEntries(std::vector<CacheEntry>& cache,
                             int minHits,
                             bool releaseMemory) {
    std::size_t before = cache.size();

    auto newEnd = std::remove_if(cache.begin(), cache.end(),
                                 [minHits](const CacheEntry& e) {
                                     return e.hits < minHits;
                                 });
    cache.erase(newEnd, cache.end());

    if (releaseMemory) {
        // 只有在「確定尖峰已過」時才做這一次縮容
        cache.shrink_to_fit();
    }
    return before - cache.size();
}

int main() {
    std::cout << "=== 一、大量插入後刪除，capacity 不會自己縮 ===" << std::endl;
    std::vector<int> v;

    for (int i = 0; i < 10000; ++i) {
        v.push_back(i);
    }
    std::cout << "插入後：size=" << v.size()
              << ", capacity=" << v.capacity() << std::endl;

    v.erase(v.begin() + 10, v.end());  // 只保留前 10 個
    std::cout << "刪除後：size=" << v.size()
              << ", capacity=" << v.capacity() << std::endl;
    std::cout << "  ↑ capacity 還是很大！記憶體並沒有還回去" << std::endl;

    v.shrink_to_fit();
    std::cout << "shrink 後：size=" << v.size()
              << ", capacity=" << v.capacity() << std::endl;

    std::cout << "\n=== 二、clear() 同樣不歸還記憶體 ===" << std::endl;
    std::vector<int> w(5000, 1);
    std::cout << "初始     ：size=" << w.size() << ", capacity=" << w.capacity() << std::endl;
    w.clear();
    std::cout << "clear 後 ：size=" << w.size() << ", capacity=" << w.capacity()
              << "  ← size 歸零，capacity 沒動" << std::endl;
    w.shrink_to_fit();
    std::cout << "shrink 後：size=" << w.size() << ", capacity=" << w.capacity() << std::endl;

    std::cout << "\n=== 三、C++11 之前的 swap trick（讀舊程式碼會看到）===" << std::endl;
    std::vector<int> u(5000, 2);
    std::cout << "初始      ：size=" << u.size() << ", capacity=" << u.capacity() << std::endl;
    u.erase(u.begin() + 5, u.end());
    std::vector<int>(u).swap(u);       // 等價於 u.shrink_to_fit()
    std::cout << "swap trick：size=" << u.size() << ", capacity=" << u.capacity() << std::endl;

    std::vector<int>().swap(u);        // 清空並徹底釋放
    std::cout << "清空並釋放：size=" << u.size() << ", capacity=" << u.capacity() << std::endl;

    std::cout << "\n=== 四、shrink 會使迭代器失效 ===" << std::endl;
    std::vector<int> s(1000, 9);
    s.erase(s.begin() + 3, s.end());
    const int* beforePtr = s.data();
    s.shrink_to_fit();
    std::cout << "shrink 前後 data() 位址是否改變: "
              << std::boolalpha << (s.data() != beforePtr)
              << "（改變 = 舊迭代器全部失效）" << std::endl;

    std::cout << "\n=== 五、日常實務：快取逐出後的記憶體回收 ===" << std::endl;
    std::vector<CacheEntry> cache;
    cache.reserve(100000);
    for (int i = 0; i < 100000; ++i) {
        cache.push_back({i, "payload", (i % 50 == 0) ? 100 : 1});
    }
    std::cout << "尖峰時    ：size=" << cache.size()
              << ", capacity=" << cache.capacity() << std::endl;

    std::size_t evicted = evictColdEntries(cache, 10, false);
    std::cout << "逐出 " << evicted << " 筆（不縮容）：size=" << cache.size()
              << ", capacity=" << cache.capacity()
              << "  ← RSS 仍停在高點" << std::endl;

    cache.shrink_to_fit();
    std::cout << "離峰時縮容：size=" << cache.size()
              << ", capacity=" << cache.capacity()
              << "  ← 記憶體真正交還" << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 17 課：vector 的記憶體重新配置機制10.cpp" -o shrink_demo
//
// 【關於下方預期輸出的但書】
//   ▸ 所有 capacity 數值皆為「實作定義」，本機環境是
//     x86-64 / Ubuntu / GCC 15.2 / libstdc++（2 倍成長策略）。
//     換 MSVC（1.5 倍）或其他標準庫，數字都會不同。
//   ▸ shrink_to_fit 在標準上只是「非強制性請求」，
//     允許實作完全不動作。下方輸出顯示它確實生效，
//     這是本機實作的行為，不是標準保證。
//   ▸ 第四段刻意只印出「位址有沒有改變」的布林值，
//     不印位址本身——位址每次執行都不同。

// === 預期輸出 ===
// === 一、大量插入後刪除，capacity 不會自己縮 ===
// 插入後：size=10000, capacity=16384
// 刪除後：size=10, capacity=16384
//   ↑ capacity 還是很大！記憶體並沒有還回去
// shrink 後：size=10, capacity=10
//
// === 二、clear() 同樣不歸還記憶體 ===
// 初始     ：size=5000, capacity=5000
// clear 後 ：size=0, capacity=5000  ← size 歸零，capacity 沒動
// shrink 後：size=0, capacity=0
//
// === 三、C++11 之前的 swap trick（讀舊程式碼會看到）===
// 初始      ：size=5000, capacity=5000
// swap trick：size=5, capacity=5
// 清空並釋放：size=0, capacity=0
//
// === 四、shrink 會使迭代器失效 ===
// shrink 前後 data() 位址是否改變: true（改變 = 舊迭代器全部失效）
//
// === 五、日常實務：快取逐出後的記憶體回收 ===
// 尖峰時    ：size=100000, capacity=100000
// 逐出 98000 筆（不縮容）：size=2000, capacity=100000  ← RSS 仍停在高點
// 離峰時縮容：size=2000, capacity=2000  ← 記憶體真正交還
