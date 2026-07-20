// =============================================================================
//  第 21 課：deque 的內部結構（分段連續空間）1.cpp
//    —  用可執行的程式驗證 deque 的分段結構與失效規則
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔: #include <deque>
//   push_front / push_back  攤銷 O(1)（既有元素從不搬移）
//   pop_front  / pop_back   O(1)
//   operator[]              O(1)（兩次記憶體存取,常數大於 vector）
//   ★ 沒有 data()：元素不保證連續
//
// 【詳細解釋 Explanation】
//
// 【1. 分段連續 = map 指標陣列 + 固定大小 buffer】
// deque 不是一整塊連續記憶體,而是「一個指標陣列(map/中控器)指向多個
// 固定大小 buffer」的兩層結構。map 從中間開始使用,兩端都留有空槽,
// 因此往前長或往後長都只是「填一個 buffer 內的格子」或「配一個新 buffer
// 並填一個 map 槽」——**既有元素永遠不搬移**,這就是雙端 O(1) 的來源。
//
// 【2. 這個設計換來什麼、付出什麼】
//   得到：push_front 從 vector 的 O(n) 變成攤銷 O(1);
//        擴容時不搬移元素,不呼叫 move/copy 建構子,元素位址穩定。
//   付出：隨機存取要先讀 map 再讀元素(兩次記憶體存取);
//        元素分段 → cache locality 不如 vector;
//        沒有 data(),不能整段餵給 C API。
//
// 【概念補充 Concept Deep Dive】
// libstdc++ 的每段大小為 (sizeof(T) < 512) ? 512/sizeof(T) : 1 個元素,
// 也就是「約 512 bytes 一段,但至少放得下一個元素」。本檔會實際量測它。
// ★ 標準完全沒有規定這個數字,MSVC 用另一套規則 —— 不可依賴。
//
// 【注意事項 Pay Attention】
//  1. 頭尾插入 → **所有 iterator 失效**,但 **reference/pointer 仍有效**。
//  2. 中間 insert/erase → iterator 與 reference **全部失效**。
//  3. operator[] 不做邊界檢查;需要檢查用 at()（丟 std::out_of_range）。
//  4. 元素不連續,沒有 data();要給 C API 必須先複製成 vector。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】deque 的分段連續結構
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. vector 的 push_front 是 O(n),deque 為什麼能做到攤銷 O(1)?
//     答：vector 是單一連續區塊,前端插入必須把 n 個元素整批往後搬。
//         deque 把元素切成多段,只要在「前端那個 buffer」的空位寫入即可;
//         該 buffer 滿了就配一個新的,並把指標填進 map 左邊的空槽,
//         **既有元素完全不動**。
//     追問：map 的槽用完了呢?→ 重配一個更大的 map,但只複製
//         n/buffer_size 個指標,不搬元素、不呼叫建構子,攤還後仍是 O(1)。
//
// ⚠️ 陷阱. 「push_front 之後,先前取得的 iterator 和 pointer 都不能再用了」
//         —— 這句話錯在哪?
//     答：只對一半。標準規定 deque 在頭尾插入時**所有 iterator 失效**,
//         但**reference 與 pointer 仍然有效**,因為元素根本沒有被搬移。
//     為什麼會錯：把 vector 的心智模型套過來 —— vector 擴容會整批搬移元素,
//         所以 iterator 和 reference 必然同時失效。deque 不搬元素,
//         兩者因此脫鉤。注意豁免**只限頭尾**,中間 insert/erase 仍全部失效。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <deque>
#include <vector>
#include <algorithm>

// 量測「單段最多能連續放幾個元素」:連續 push_back,
// 觀察相鄰元素位址何時不再相差 sizeof(T) —— 那就是跨過了 buffer 邊界。
// 只回傳個數(穩定值),不印位址(位址每次執行都不同)。
template <typename T>
std::size_t probeBufferElems() {
    std::deque<T> dq;
    dq.push_back(T{});
    const char* prev = reinterpret_cast<const char*>(&dq.back());
    std::size_t run = 1, best = 1;
    for (int i = 0; i < 2000; ++i) {
        dq.push_back(T{});
        const char* cur = reinterpret_cast<const char*>(&dq.back());
        if (cur == prev + sizeof(T)) { ++run; best = std::max(best, run); }
        else                          { run = 1; }
        prev = cur;
    }
    return best;
}

int main() {
    std::cout << "=== 1. 雙端插入 ===\n";
    std::deque<int> dq;
    dq.push_back(10);
    dq.push_back(20);
    dq.push_back(30);
    dq.push_front(5);       // ← vector 做不到 O(1)
    dq.push_front(1);
    std::cout << "  內容:";
    for (int v : dq) std::cout << " " << v;
    std::cout << "\n";

    std::cout << "\n=== 2. 隨機存取與雙端刪除 ===\n";
    std::cout << "  dq[0]=" << dq[0] << " dq[2]=" << dq[2] << "\n";
    dq.pop_front();
    dq.pop_back();
    std::cout << "  pop 首尾後:";
    for (int v : dq) std::cout << " " << v;
    std::cout << "  size=" << dq.size() << "\n";

    std::cout << "\n=== 3. 實測分段大小(libstdc++,非標準保證) ===\n";
    std::size_t ni = probeBufferElems<int>();
    std::size_t nd = probeBufferElems<double>();
    std::cout << "  int:    單段 " << ni << " 個 → " << ni * sizeof(int)    << " bytes\n";
    std::cout << "  double: 單段 " << nd << " 個 → " << nd * sizeof(double) << " bytes\n";
    std::cout << "  → 規則 = (sizeof(T) < 512) ? 512/sizeof(T) : 1\n";

    std::cout << "\n=== 4. 頭尾插入:iterator 失效但 reference 有效 ===\n";
    std::deque<int> d2 = {100, 200, 300};
    int* p = &d2[1];                    // 指向值 200 的那個元素
    std::cout << "  插入前 *p=" << *p << "\n";
    for (int i = 0; i < 1000; ++i) d2.push_front(i);   // 必然跨越多個 buffer
    std::cout << "  push_front 1000 次後 *p=" << *p << "（reference 仍有效）\n";
    std::cout << "  該元素現在的索引是 1001,d2[1001]=" << d2[1001] << "\n";
    std::cout << "  ★ 但所有 iterator 已失效,不可再使用先前取得的 iterator\n";

    std::cout << "\n=== 5. 沒有 data():要餵 C API 得先攤平 ===\n";
    std::vector<int> flat(dq.begin(), dq.end());
    std::cout << "  複製成 vector 後 flat.data()[0]=" << flat.data()[0]
              << " size=" << flat.size() << "\n";
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 21 課：deque 的內部結構（分段連續空間）1.cpp" -o deque_internal

// === 3. 實測分段大小(libstdc++,非標準保證) ===

// === 預期輸出 ===
// === 1. 雙端插入 ===
//   內容: 1 5 10 20 30
//
// === 2. 隨機存取與雙端刪除 ===
//   dq[0]=1 dq[2]=10
//   pop 首尾後: 5 10 20  size=3
//
//   int:    單段 128 個 → 512 bytes
//   double: 單段 64 個 → 512 bytes
//   → 規則 = (sizeof(T) < 512) ? 512/sizeof(T) : 1
//
// === 4. 頭尾插入:iterator 失效但 reference 有效 ===
//   插入前 *p=200
//   push_front 1000 次後 *p=200（reference 仍有效）
//   該元素現在的索引是 1001,d2[1001]=200
//   ★ 但所有 iterator 已失效,不可再使用先前取得的 iterator
//
// === 5. 沒有 data():要餵 C API 得先攤平 ===
//   複製成 vector 後 flat.data()[0]=5 size=3
