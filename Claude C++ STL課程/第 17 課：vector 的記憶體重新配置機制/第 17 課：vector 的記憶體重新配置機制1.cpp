// =============================================================================
//  第 17 課：vector 的記憶體重新配置機制 1  —  觀察重新配置的時機與位址搬移
// =============================================================================
//
// 【主題資訊 Information】
//   size_type size()     const noexcept;   // 目前有幾個「已建構」的元素
//   size_type capacity() const noexcept;   // 目前這塊緩衝區「最多」能放幾個
//   T*        data()           noexcept;   // C++11 起：取得底層連續陣列的首位址
//
//   標頭檔    ：<vector>
//   標準版本  ：size()/capacity() 為 C++98；data() 為 C++11
//   複雜度    ：三者皆為 O(1)，且都是 noexcept
//   關鍵不變式：0 <= size() <= capacity()
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼 vector 需要「size 與 capacity 兩個數字」】
//   vector 對外承諾兩件事，而這兩件事本質上是衝突的：
//     (a) 元素在記憶體中「連續」（才能支援 O(1) 隨機存取、才能把 data() 交給
//         C API、才能讓 CPU 的 prefetcher 與 SIMD 發揮效果）。
//     (b) 尾端插入的均攤成本是 O(1)。
//   連續代表「這塊記憶體一旦配置就固定大小」，作業系統不保證你能就地
//   長大——你的緩衝區後面很可能已經住了別人。既然不能就地長大，唯一
//   的辦法就是「另外配一塊更大的、把舊資料搬過去」。
//   如果每次 push_back 都剛好搬一次，n 次插入就是 O(n^2)，(b) 就破產了。
//   解法是：一次就多要一些，把多要的部分先「空著」。
//     capacity = 這塊緩衝區能放的上限
//     size     = 這塊緩衝區裡真的已經建構好的元素個數
//   [size, capacity) 這段是「已配置、但尚未建構任何物件」的原始記憶體。
//
// 【2. 重新配置（reallocation）到底做了哪五件事】
//   當 size() == capacity() 時再 push_back，實作必須：
//     1) 向 allocator 要一塊新的、更大的記憶體（不保證與舊區塊相鄰）
//     2) 把舊區塊的元素逐一「搬」到新區塊（移動或拷貝，見第 3 檔）
//     3) 對舊區塊的每個元素呼叫解構子
//     4) 把舊區塊還給 allocator
//     5) 在新區塊的尾端建構這個新元素
//   注意順序：新元素其實是「先在新緩衝區建構」才拆舊的，這是為了萬一
//   建構過程丟出例外時，vector 還能維持原狀（strong exception guarantee）。
//
// 【3. 為什麼要用 data() 而不是 &v[0] 來觀察位址】
//   兩者在 v 非空時等價，但 v 為空時 &v[0] 會對「不存在的元素」取 operator[]，
//   那是 UB。data() 對空 vector 是合法呼叫（可能回傳 nullptr 或某個
//   不可解參考的值），所以「觀察緩衝區位址」一律用 data()。
//   本檔在 push_back 前就呼叫 data()，第一輪 v 還是空的——正因為用了
//   data() 而不是 &v[0]，這段程式碼才是合法的。
//
// 【4. 怎麼判斷「這次 push_back 有沒有重新配置」】
//   最可靠的觀測量是 capacity() 是否改變（本檔的做法）。
//   也可以比對 data() 前後是否改變，但要小心：allocator 完全有可能把
//   剛釋放的舊區塊回收後又配回同一個位址給你——「位址相同」不代表
//   「沒有重新配置」。以 capacity 為準，位址只當輔助觀察。
//
// 【概念補充 Concept Deep Dive】
//   本機（GCC 15.2 / libstdc++，x86-64）實測 sizeof(std::vector<int>) == 24，
//   也就是三個 8 byte 的指標，典型佈局是：
//
//       vector 物件本身（24 bytes，可能在 stack 上）
//       ┌───────────┬───────────────┬─────────────────────┐
//       │ _M_start  │ _M_finish     │ _M_end_of_storage   │
//       └─────┬─────┴───────┬───────┴──────────┬──────────┘
//             │             │                  │
//             ▼             ▼                  ▼
//       heap: [ 1 ][ 2 ][ 3 ][ 4 ][ ? ][ ? ][ ? ][ ? ]
//             ^^^^^^^^^^^^^^^^^^^^  ^^^^^^^^^^^^^^^^^^
//             size() == 4           未建構的原始記憶體
//             capacity() == 8
//
//   由此可見 size() 與 capacity() 都不是「存起來的整數」，而是指標相減：
//       size()     == _M_finish        - _M_start
//       capacity() == _M_end_of_storage - _M_start
//   這正是它們保證 O(1) 且 noexcept 的原因。
//   注意 vector 物件本身很小（24 bytes），元素永遠在 heap 上；把 vector
//   按值傳參的成本不是「複製 24 bytes」，而是「複製整個 heap 緩衝區」。
//
// 【注意事項 Pay Attention】
//   1. 成長倍率是【實作定義】。C++ 標準只要求 push_back 的均攤複雜度是
//      O(1)，從未規定倍率。本機 libstdc++ 實測為 2 倍，MSVC 慣用 1.5 倍。
//      下方預期輸出中的所有 capacity 數字都來自本機 libstdc++，換平台
//      或換標準函式庫版本就可能不同，不可寫進測試斷言。
//   2. 一旦發生重新配置，所有指向舊緩衝區的 iterator、pointer、reference
//      （包含 end()）全部失效；之後再使用它們是 UB。
//   3. 位址（0x... 那串）每次執行都不同（ASLR），不要拿來比對。
//   4. 空 vector 的 capacity() 不保證是 0，只是本機實測為 0。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】size / capacity / 重新配置的觀察
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. size() 和 capacity() 差在哪？各自的複雜度是多少？
//     答：size() 是「已建構的元素個數」，capacity() 是「不重新配置的前提下
//         最多能放幾個」，恆有 size() <= capacity()。兩者都是 O(1) 且
//         noexcept，因為 libstdc++ 內部就是三個指標相減得到的。
//         [size, capacity) 之間是已配置但尚未建構物件的原始記憶體。
//     追問：那 capacity 那段沒建構的記憶體，能不能用 v[size()] 去讀？
//         → 不行，那裡沒有物件，operator[] 越界即 UB。
//
// 🔥 Q2. 怎麼可靠地偵測「這次 push_back 有沒有觸發重新配置」？
//     答：比較 push_back 前後的 capacity() 是否改變。
//         不要只靠 data() 位址是否改變——allocator 可能把剛釋放的區塊
//         再配回同一個位址，位址沒變也可能真的重新配置過了。
//     追問：那反過來，位址變了一定是重新配置嗎？→ 對 vector 而言是的，
//         但這個方向的判斷仍不如 capacity 直接。
//
// ⚠️ 陷阱. 「capacity 是 8、size 是 4，所以中間那 4 格是值為 0 的元素」——錯在哪？
//     答：[size, capacity) 是「原始記憶體」，上面根本沒有建構任何物件，
//         連「值」這個概念都不存在。要讓它們變成真的元素只能透過
//         push_back / emplace_back / resize / insert。
//     為什麼會錯：把 vector 想成「一個長度 8 的 C 陣列，只是我只用了 4 格」。
//         C 陣列的每一格都是已存在的物件，但 vector 的尾巴是未建構空間——
//         這正是 vector 能容納「沒有預設建構子的型別」的原因。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <string>

// -----------------------------------------------------------------------------
// 【日常實務範例】伺服器 log 緩衝區：量化「沒有預先 reserve」的代價
//   情境：服務把每筆 log 先收進 vector，累積到一定量才批次寫檔。
//         如果不 reserve，收 N 筆的過程中會反覆重新配置，每次都要把
//         先前所有 log 搬一次；本函式把「搬了幾次、總共搬了幾個元素」
//         算出來，讓成本看得見。
//   關聯：這就是重新配置的直接成本，也是第 9、11 檔要優化的目標。
// -----------------------------------------------------------------------------
struct ReallocStat {
    int    realloc_times = 0;   // 重新配置發生幾次
    long   moved_elems   = 0;   // 累計搬移了幾個元素
    size_t final_cap     = 0;   // 最終容量
};

ReallocStat measure_log_buffer(int n_lines) {
    ReallocStat st;
    std::vector<std::string> buf;
    size_t prev_cap = buf.capacity();

    for (int i = 0; i < n_lines; ++i) {
        buf.push_back("2026-07-20 10:00:00 [INFO] request handled");
        if (buf.capacity() != prev_cap) {
            ++st.realloc_times;
            // 重新配置當下，舊緩衝區裡的 prev_cap 個元素都要被搬到新區
            st.moved_elems += static_cast<long>(prev_cap);
            prev_cap = buf.capacity();
        }
    }
    st.final_cap = buf.capacity();
    return st;
}

int main() {
    std::cout << "=== 觀察 push_back 造成的重新配置 ===" << std::endl;

    std::vector<int> v;

    std::cout << "初始狀態：size=" << v.size()
              << ", capacity=" << v.capacity() << std::endl;

    for (int i = 1; i <= 10; ++i) {
        // 記錄 push_back 之前的狀態
        size_t old_cap = v.capacity();
        const int* old_data = v.data();  // 底層陣列的位址（空 vector 也可安全呼叫）

        v.push_back(i);

        if (v.capacity() != old_cap) {
            std::cout << "--- 重新配置！---" << std::endl;
            std::cout << "  舊容量: " << old_cap
                      << ", 新容量: " << v.capacity() << std::endl;
            std::cout << "  舊位址: " << old_data
                      << ", 新位址: " << v.data() << std::endl;
        }

        std::cout << "push_back(" << i << "): size=" << v.size()
                  << ", capacity=" << v.capacity() << std::endl;
    }

    std::cout << "\n=== 容量利用率（本機 libstdc++：capacity 為 2 的冪次）===" << std::endl;
    std::cout << "最終 size=" << v.size()
              << ", capacity=" << v.capacity()
              << "，浪費 " << (v.capacity() - v.size()) << " 格未使用空間" << std::endl;

    std::cout << "\n=== 日常實務：log 緩衝區的重新配置成本 ===" << std::endl;
    for (int n : {100, 1000, 10000}) {
        ReallocStat st = measure_log_buffer(n);
        std::cout << "收 " << n << " 筆 log："
                  << "重新配置 " << st.realloc_times << " 次，"
                  << "累計搬移 " << st.moved_elems << " 個元素，"
                  << "最終容量 " << st.final_cap << std::endl;
    }
    std::cout << "觀察：搬移總數大約是 n 的常數倍（< 2n），這正是均攤 O(1) 的證據；"
              << std::endl;
    std::cout << "      但常數不是 0——所以能事先估算大小時，仍應 reserve。"
              << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 17 課：vector 的記憶體重新配置機制1.cpp" -o realloc_observe

// 註：輸出中的「舊位址 / 新位址」每次執行都不同（ASLR + heap 配置結果），
//     以下數值僅為本機某一次實際執行的結果；capacity 數列則是本機
//     libstdc++ 的實作定義行為（2 倍成長），換平台可能不同。

// === 預期輸出 ===
// === 觀察 push_back 造成的重新配置 ===
// 初始狀態：size=0, capacity=0
// --- 重新配置！---
//   舊容量: 0, 新容量: 1
//   舊位址: 0, 新位址: 0x5b9f1c1da020
// push_back(1): size=1, capacity=1
// --- 重新配置！---
//   舊容量: 1, 新容量: 2
//   舊位址: 0x5b9f1c1da020, 新位址: 0x5b9f1c1da340
// push_back(2): size=2, capacity=2
// --- 重新配置！---
//   舊容量: 2, 新容量: 4
//   舊位址: 0x5b9f1c1da340, 新位址: 0x5b9f1c1da020
// push_back(3): size=3, capacity=4
// push_back(4): size=4, capacity=4
// --- 重新配置！---
//   舊容量: 4, 新容量: 8
//   舊位址: 0x5b9f1c1da020, 新位址: 0x5b9f1c1da360
// push_back(5): size=5, capacity=8
// push_back(6): size=6, capacity=8
// push_back(7): size=7, capacity=8
// push_back(8): size=8, capacity=8
// --- 重新配置！---
//   舊容量: 8, 新容量: 16
//   舊位址: 0x5b9f1c1da360, 新位址: 0x5b9f1c1da390
// push_back(9): size=9, capacity=16
// push_back(10): size=10, capacity=16
//
// === 容量利用率（本機 libstdc++：capacity 為 2 的冪次）===
// 最終 size=10, capacity=16，浪費 6 格未使用空間
//
// === 日常實務：log 緩衝區的重新配置成本 ===
// 收 100 筆 log：重新配置 8 次，累計搬移 127 個元素，最終容量 128
// 收 1000 筆 log：重新配置 11 次，累計搬移 1023 個元素，最終容量 1024
// 收 10000 筆 log：重新配置 15 次，累計搬移 16383 個元素，最終容量 16384
// 觀察：搬移總數大約是 n 的常數倍（< 2n），這正是均攤 O(1) 的證據；
//       但常數不是 0——所以能事先估算大小時，仍應 reserve。
