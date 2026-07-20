// =============================================================================
//  04_sync_with_stdio.cpp  —  加速 IO 的兩個關鍵設定
// =============================================================================
//  參考：
//    https://en.cppreference.com/w/cpp/io/ios_base/sync_with_stdio
//    https://en.cppreference.com/w/cpp/io/basic_ios/tie
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、為什麼 cin / cout 預設那麼慢？                         │
//  └────────────────────────────────────────────────────────────┘
//
//  C++ 的 iostream 預設兩個「為了相容 C 與互動體驗」的設定：
//
//   (1) sync_with_stdio(true)：cin/cout 的 buffer 與 C 的 stdin/stdout
//       同步。這樣 printf 跟 cout 混用不會錯亂，但代價是每次操作都要
//       「兩邊同步」，慢一個量級。
//
//   (2) cin.tie(&cout)：每次 cin >> 之前自動 flush cout。確保你寫
//       cout << "Enter: "; cin >> x; 時 prompt 字串先出現。但若你「不
//       搭配 prompt 互動」，這個 flush 是多餘成本。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、競賽 / 大量 IO 的標準姿勢                              │
//  └────────────────────────────────────────────────────────────┘
//
//      #include <iostream>
//      int main() {
//          std::ios::sync_with_stdio(false);
//          std::cin.tie(nullptr);
//          /* 之後 cin/cout 速度跟 scanf/printf 差不多 */
//      }
//
//  注意事項：
//   * 一旦 sync_with_stdio(false) 就不要再混用 printf / scanf —— 否則輸
//     出順序錯亂。
//   * 這兩行放在 main() 開頭、其它任何 IO 之前，效果最好。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、簡單 benchmark                                         │
//  └────────────────────────────────────────────────────────────┘
//
//  本檔模擬：寫 100 萬個整數到 ostringstream 兩種方式比較：
//   * 純 cout 方式（這裡我們改用 ostringstream，避免影響螢幕）
//   * 不需要 sync 的版本
//
//  也做「讀」端的對比：用 istringstream 讀。
// =============================================================================


/*
補充筆記：std::ios::sync_with_stdio / cin.tie
  - std::ios::sync_with_stdio(false) 解除 C++ iostream 與 C stdio 的同步，常用於大量輸入輸出加速。
  - 解除同步後，不要混用 printf/scanf 與 cin/cout；即使都能編譯，輸出順序也可能因兩套 buffer 不同步而錯亂。
  - std::cin.tie(nullptr) 解除「每次讀 cin 前自動 flush cout」的關係，可減少互動以外場景的多餘 flush。
  - 這兩行應放在 main() 開頭、任何 cin/cout/printf/scanf 使用之前；太晚設定時，實作可能已建立狀態，效果不如預期。
  - 互動式程式若解除 tie，印提示文字後要手動 std::flush 或使用 std::endl，否則使用者可能還沒看到 prompt 程式就等輸入。
  - 這個設定只影響標準串流和 stdio 的同步，不會讓演算法變快；真正瓶頸若是解析、格式化或磁碟 I/O，仍要另外處理。
  - std::endl 會輸出換行並 flush，速度敏感處應優先使用 '\n'；flush 是 I/O 效能常見隱形成本。
  - 競賽程式常加這兩行是合理慣例，長期服務程式則要先確認沒有混用 C I/O 或依賴自動 flush 的互動行為。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】sync_with_stdio 與 cin.tie
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. ios_base::sync_with_stdio(false) 做了什麼？什麼時候用是安全的？
//     答：預設情況下 C++ streams 與 C stdio 共用緩衝、保持同步，因此可以自由交錯使用，
//     代價是 cout 幾乎無法有效緩衝。解除同步後 iostream 使用自己獨立的緩衝，大量 I/O
//     時常有數倍加速。安全條件是：解除同步後，同一個程式就不要再混用 C stdio 與
//     iostream。呼叫時機是 main 一開始、任何 I/O 之前。
//     追問：競賽常見的組合是什麼？（sync_with_stdio(false); 再加 cin.tie(nullptr);）
//
// 🔥 Q2. cin.tie(nullptr) 做了什麼？有什麼副作用？
//     答：cin 預設 tie 到 cout，每次從 cin 讀取前會先 flush cout——這就是
//     cout << "請輸入："; cin >> x; 提示字能正常顯示的原因。解除 tie 之後，提示文字可能
//     還停在緩衝區裡就開始等輸入，使用者看到的是一片空白。所以互動式程式不該解除 tie。
//
// ⚠️ 陷阱. 關了同步之後，printf 和 cout 的輸出順序為什麼亂掉？這算 UB 嗎？
//     答：解除同步後兩邊各自維護獨立緩衝區，內容依各自的 flush 時機寫出，順序不再由
//     程式中的書寫順序決定。這不是 bug，而是 sync_with_stdio(false) 的定義後果——規則
//     就是「關掉同步就只能擇一使用」。另外要注意：標準對「同時寫入標準流物件不產生
//     data race」的那層保證，前提是未解除同步。
//     為什麼會錯：以為 sync 只影響效能不影響語意；它影響的正是「兩套 API 共用同一個
//     底層緩衝」這件事。
// ═══════════════════════════════════════════════════════════════════════════

#include <chrono>
#include <iostream>
#include <sstream>

using namespace std::chrono;

template <class F>
static long long measureUs(F&& fn) {
    auto t1 = steady_clock::now();
    fn();
    auto t2 = steady_clock::now();
    return duration_cast<microseconds>(t2 - t1).count();
}

int main() {
    constexpr int N = 100'000;

    // ─────────────────────────────────────────────────────────
    // Demo 1：寫端 — 同樣是 ostringstream，本身不受 sync_with_stdio 影響
    //   這邊主要展示 ostringstream 寫入速度（已經很快）
    // ─────────────────────────────────────────────────────────
    auto write_us = measureUs([] {
        std::ostringstream os;
        for (int i = 0; i < N; ++i) os << i << ' ';
        volatile auto sink = os.str().size();
        (void)sink;
    });
    std::cout << "[Demo1] write " << N << " ints to ostringstream: "
              << write_us << " us\n";

    // ─────────────────────────────────────────────────────────
    // Demo 2：讀端 — 用 istringstream 讀大量整數
    // ─────────────────────────────────────────────────────────
    std::ostringstream src;
    for (int i = 0; i < N; ++i) src << i << ' ';
    std::string text = src.str();

    auto read_us = measureUs([&] {
        std::istringstream is{text};
        long long sum = 0;
        int x;
        while (is >> x) sum += x;
        volatile long long sink = sum;
        (void)sink;
    });
    std::cout << "[Demo2] read " << N << " ints from istringstream: "
              << read_us << " us\n";

    // ─────────────────────────────────────────────────────────
    // Demo 3：示範「用」這兩個設定（實際對 cin/cout 才有差異）
    //   這裡不真的讀 stdin，只展示寫法
    // ─────────────────────────────────────────────────────────
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    std::cout << "[Demo3] sync_with_stdio(false) + cin.tie(nullptr) — 標準姿勢\n";

    // ─────────────────────────────────────────────────────────
    // 實用範例 A：endl vs '\n' 的成本對照
    //   endl 會 flush；'\n' 只是換行字元。
    //   在熱迴圈裡用 endl 會明顯變慢，這個 demo 量化兩者差距。
    // ─────────────────────────────────────────────────────────
    {
        // 用 ostringstream 量測（不汙染螢幕），但行為差異概念跟 cout 一致
        // (注意：ostringstream 的 endl 也會 flush，雖然這個 sink 不真的寫 IO)
        auto t_n = measureUs([] {
            std::ostringstream o;
            for (int i = 0; i < N; ++i) o << i << '\n';
            volatile auto s = o.str().size(); (void)s;
        });
        auto t_endl = measureUs([] {
            std::ostringstream o;
            for (int i = 0; i < N; ++i) o << i << std::endl;
            volatile auto s = o.str().size(); (void)s;
        });
        std::cout << "[endl vs \\n] '\\n' = " << t_n
                  << " us, endl = " << t_endl << " us\n";
    }

    // ─────────────────────────────────────────────────────────
    // 實用範例 B：用 ostringstream 攢一整塊再一次 cout 寫出 — 減少 IO 次數
    //   工作上輸出大量資料時，多次 cout << 可能 N 次 IO；先攢成一個 string
    //   再一次寫，能大幅降低 syscall 成本（也減少多 thread 交錯）。
    // ─────────────────────────────────────────────────────────
    {
        auto t_many = measureUs([] {
            std::ostringstream o;
            for (int i = 0; i < 1000; ++i) o << "x=" << i << '\n';
            // 模擬把字串「一次寫」到 sink（這裡就是讓 sink 摸一下）
            volatile auto s = o.str().size(); (void)s;
        });
        std::cout << "[batch-out] one big string for 1000 lines: "
                  << t_many << " us\n";
    }

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：sync_with_stdio(false) 之後可以用 printf 嗎？
    //    A：可以呼叫，但「順序」不再保證跟 cin/cout 同步 — 容易輸出錯亂。
    //       要嘛全 C 風格、要嘛全 C++ 風格，別混用。
    //
    //  Q2：沒做這兩件事，cin/cout 會比 scanf/printf 慢多少？
    //    A：典型 5~20 倍。LeetCode 競賽級的「百萬筆數」題通常會 TLE。一加
    //       上去就跟 scanf 同等級。
    //
    //  Q3：cin.tie(nullptr) 後互動式 prompt 會壞掉嗎？
    //    A：可能 — prompt 字串可能在你「等使用者輸入」時還沒被 flush。
    //       你要在 cout << prompt 後手動 << std::flush 或 endl。
    //
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra 04_sync_with_stdio.cpp -o 04_sync_with_stdio

// === 預期輸出 ===
// [Demo1] write 100000 ints to ostringstream: 8124 us
// [Demo2] read 100000 ints from istringstream: 9937 us
// [Demo3] sync_with_stdio(false) + cin.tie(nullptr) — 標準姿勢
// [endl vs \n] '\n' = 8970 us, endl = 9062 us
// [batch-out] one big string for 1000 lines: 115 us
// ⚠️ 上面的位址／執行緒 id／耗時每次執行都不同，數值僅供對照，不是固定結果。
