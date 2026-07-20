// =============================================================================
//  第 15 課：vector 元素刪除 6  —  clear()：清空 size，但不還記憶體
// =============================================================================
//
// 【主題資訊 Information】
//   void clear() noexcept;
//   標頭檔：<vector>
//   標準版本：C++98（noexcept 標註自 C++11）。
//   複雜度：O(n)——要對每個元素呼叫解構子。
//           （對 trivially destructible 的型別如 int，實務上是 O(1)）
//   效果：size() 變成 0；【capacity() 不變】。
//   失效範圍：所有 iterator / reference / pointer 全部失效（包含 end()）。
//   例外：標記為 noexcept，保證不拋例外。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼 clear() 不釋放記憶體 —— 這是特性不是缺陷】
//   最常見的誤解是「clear 應該把記憶體還給系統」。標準刻意不這麼做，
//   因為 clear 最典型的用途正是「清空後馬上重新填滿」：
//       std::vector<Row> buf;
//       while (hasMoreBatches()) {
//           buf.clear();              // 清空，但保留已配置的記憶體
//           readBatch(buf);           // 重新填滿，完全不必再配置
//           process(buf);
//       }
//   若 clear 會釋放記憶體，這個迴圈每一輪都要配置+釋放一次，
//   在高頻情境下是很可觀的浪費。
//   保留 capacity 讓「記憶體高水位」被重複利用，這是 vector
//   當作可重用緩衝區時最重要的性質。
//
// 【2. 真的要釋放記憶體時怎麼做】
//   (a) C++11 起：
//         v.clear();
//         v.shrink_to_fit();
//       但 shrink_to_fit 是【非約束性請求】（non-binding request），
//       標準明文允許實作忽略它。所以不能說「呼叫它一定會縮容」。
//   (b) 保證有效的 swap 慣用法（C++11 之前的標準做法）：
//         std::vector<T>().swap(v);
//       建立一個空的臨時 vector，和 v 交換內容。
//       交換後 v 拿到臨時物件的（空）狀態，臨時物件拿到 v 的記憶體
//       並在該行結束時解構、釋放。這個做法【保證】釋放。
//   本檔的 main 兩種都實測。
//
// 【3. clear() 到底做了什麼】
//   概念上就兩件事：
//       ① 對 [begin(), end()) 內每個元素呼叫解構子
//       ② 把「結尾指標」設回「開頭指標」
//   配置的那塊記憶體完全沒有動。所以：
//     * 對 vector<int>：解構子什麼都不做，clear 幾乎等於 `size = 0`
//     * 對 vector<std::string>：每個 string 的緩衝區都會被釋放
//       （那是 string 自己的記憶體，不是 vector 的）
//   這個區別很重要：clear 不還「vector 的」記憶體，
//   但元素自己持有的資源一定會被正確釋放。
//
// 【4. clear() 與其他清空方式的對照】
//       v.clear();                    O(n) 解構，capacity 不變
//       v.erase(v.begin(), v.end());  完全等價，但意圖不如 clear 明確
//       v.resize(0);                  同樣等價
//       v = {};                       賦值一個空的 initializer_list，
//                                     capacity 可能不變也可能改變（實作定義）
//       std::vector<T>().swap(v);     保證釋放記憶體
//   意圖是「清空」就用 clear()，它最直接、而且是 noexcept。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼 clear() 可以是 noexcept
//     它只做兩件事：呼叫解構子、重設指標。
//     解構子在 C++11 起預設就是 noexcept（除非你明確標成會拋），
//     重設指標更不可能失敗。所以整個操作可以保證不拋例外。
//     對比 push_back 就不能 noexcept——它可能要配置記憶體，可能失敗。
//
// (B) clear() 之後所有迭代器都失效
//     這點常被忽略，因為「容器都空了，誰還會用舊迭代器」。
//     但在巢狀迴圈或多執行緒的情境下，持有舊迭代器是可能的。
//     標準明訂 clear 使所有 iterator/reference/pointer 失效，
//     包括 end()——即使 begin() 的位址通常不變。
//
// (C) 記憶體高水位的取捨
//     保留 capacity 的代價是：處理過一次大批次之後，
//     那塊大記憶體會一直被佔著，即使之後都是小批次。
//     長時間執行的服務若有這個模式，可能需要週期性地
//     用 swap 慣用法把它釋放掉，否則常駐記憶體會停在歷史峰值。
//     這是一個真實的取捨，不是純粹的好處。
//
// 【注意事項 Pay Attention】
//   1. clear() 不改變 capacity。這是特性，不是 bug。
//   2. shrink_to_fit() 是非約束性請求，標準允許實作忽略。
//      要保證釋放請用 std::vector<T>().swap(v)。
//   3. clear() 使所有 iterator/reference/pointer 失效（含 end()）。
//   4. 元素自己持有的資源（string 的緩衝區等）一定會被正確釋放，
//      clear 只是不還「vector 自己」那塊記憶體。
//   5. 長期執行的服務要注意記憶體高水位——處理過一次大批次後，
//      capacity 會一直停在那個峰值。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】vector::clear
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. v.clear() 之後 capacity 會變成 0 嗎？為什麼？
//     答：不會，capacity 完全不變，只有 size 變成 0。
//         因為 clear 最典型的用途是「清空後馬上重新填滿」
//         （批次處理的緩衝區），若每次都釋放記憶體，
//         下一輪又要重新配置，形成配置/釋放的抖動。
//         保留 capacity 讓記憶體被重複利用。
//     追問：那要怎麼真的把記憶體還回去？
//         → C++11 可用 clear() + shrink_to_fit()，
//           但後者是【非約束性請求】，標準允許實作忽略。
//           保證有效的做法是 swap 慣用法：std::vector<T>().swap(v);
//           它建立空的臨時 vector 與 v 交換，臨時物件在行尾解構時
//           把原本那塊記憶體釋放掉。
//
// 🔥 Q2. clear() 對 vector<std::string> 會釋放那些字串的記憶體嗎？
//     答：會。clear 對每個元素呼叫解構子，std::string 的解構子
//         會釋放它自己的字元緩衝區。
//         不被釋放的只有【vector 自己那塊存放 string 物件的陣列】。
//         換句話說：元素持有的資源一定回收，容器的容量保留。
//     追問：那 vector<int> 呢？
//         → int 是 trivially destructible，解構子什麼都不做，
//           所以 clear 實務上只是把 size 設為 0，接近 O(1)。
//
// ⚠️ 陷阱. 「處理完一批 100 萬筆的資料後我呼叫了 clear()，
//         所以那塊記憶體已經還給系統了。」——錯在哪？
//     答：完全沒有還。clear() 只把 size 設為 0，capacity 仍然是
//         100 萬筆的大小，那塊記憶體被 vector 一直佔著，
//         直到 vector 本身解構為止。
//         對長時間執行的服務而言，這代表常駐記憶體會停在
//         「歷史處理過的最大批次」，即使之後都是小批次。
//         要真的釋放必須用 std::vector<T>().swap(v)
//         （或 shrink_to_fit，但它不保證）。
//     為什麼會錯：把 clear 的語意（「清空內容」）
//         誤讀成資源管理的語意（「釋放資源」）。
//         STL 一貫把「內容」與「容量」當成兩件獨立的事：
//         size 由你放進去的東西決定，capacity 只增不減，
//         除非你明確要求。這個分離在 reserve、resize、assign、
//         clear、pop_back、erase 上全部一致。
// ═══════════════════════════════════════════════════════════════════════════

#include <vector>
#include <iostream>
#include <string>

// -----------------------------------------------------------------------------
// 【日常實務範例】批次處理的可重用緩衝區
//   情境：ETL 程式一批一批讀資料、轉換、寫出。每批筆數不定。
//   為什麼用到本主題：這正是 clear() 保留 capacity 的設計用意。
//     緩衝區宣告在迴圈【外】，每批 clear 後重新填滿；
//     只要不超過歷史最大批次，就完全不會再向堆積要記憶體。
//   下方 main 會實測印出配置次數，證明這個效果，
//   同時也展示它的代價：記憶體會停在歷史高水位。
// -----------------------------------------------------------------------------
struct BatchStats {
    size_t batches;
    size_t rows;
    size_t reallocations;    // capacity 改變的次數
    size_t peakCapacity;
};

BatchStats runEtl(const std::vector<size_t>& batchSizes) {
    BatchStats st{0, 0, 0, 0};

    std::vector<std::string> buffer;      // 宣告在迴圈外
    size_t lastCap = buffer.capacity();

    for (size_t n : batchSizes) {
        buffer.clear();                   // 清空內容，保留已配置的記憶體

        for (size_t i = 0; i < n; ++i) {
            buffer.push_back("row-" + std::to_string(i));
        }

        if (buffer.capacity() != lastCap) {
            ++st.reallocations;
            lastCap = buffer.capacity();
        }
        st.peakCapacity = (buffer.capacity() > st.peakCapacity)
                              ? buffer.capacity() : st.peakCapacity;

        // …實際的轉換與寫出…
        st.rows += buffer.size();
        ++st.batches;
    }
    return st;
}

int main() {
    std::vector<int> v = {1, 2, 3, 4, 5};
    v.reserve(100);

    std::cout << "=== 原始示範：clear 不改變 capacity ===\n";
    std::cout << "clear 前 - size: " << v.size()
              << ", capacity: " << v.capacity() << std::endl;

    v.clear();

    std::cout << "clear 後 - size: " << v.size()
              << ", capacity: " << v.capacity() << std::endl;
    // size = 0, capacity 不變
    std::cout << "→ 記憶體完全沒有還給系統，只是「內容清空了」。\n";

    std::cout << "\n=== 兩種真正釋放記憶體的方式 ===\n";
    {
        // 方法一：shrink_to_fit（非約束性請求）
        std::vector<int> a(1000, 7);
        std::cout << "a: size=" << a.size() << " capacity=" << a.capacity() << "\n";
        a.clear();
        std::cout << "a.clear() 後:            size=" << a.size()
                  << " capacity=" << a.capacity() << "\n";
        a.shrink_to_fit();
        std::cout << "a.shrink_to_fit() 後:    size=" << a.size()
                  << " capacity=" << a.capacity() << "\n";
        std::cout << "  （本機確實縮了，但標準允許實作忽略此請求）\n";

        // 方法二：swap 慣用法（保證釋放）
        std::vector<int> b(1000, 7);
        std::cout << "\nb: size=" << b.size() << " capacity=" << b.capacity() << "\n";
        std::vector<int>().swap(b);
        std::cout << "std::vector<int>().swap(b) 後: size=" << b.size()
                  << " capacity=" << b.capacity() << "\n";
        std::cout << "  （建立空的臨時 vector 與 b 交換；臨時物件在行尾解構，\n";
        std::cout << "    把 b 原本那塊記憶體釋放掉——這個做法保證有效）\n";
    }

    std::cout << "\n=== clear() 會釋放「元素自己」的資源 ===\n";
    {
        std::vector<std::string> s;
        s.reserve(4);
        s.push_back(std::string(1000, 'x'));   // 每個字串各自持有一塊堆積記憶體
        s.push_back(std::string(1000, 'y'));
        std::cout << "兩個長字串: size=" << s.size()
                  << " capacity=" << s.capacity() << "\n";
        std::cout << "  每個 string 各自持有約 1000 bytes 的字元緩衝區\n";

        s.clear();
        std::cout << "clear() 後: size=" << s.size()
                  << " capacity=" << s.capacity() << "\n";
        std::cout << "  → 兩個 string 的解構子都被呼叫了，那 2000 bytes 已釋放。\n";
        std::cout << "    但 vector 自己那塊「存放 4 個 string 物件」的陣列還在。\n";
        std::cout << "    sizeof(std::string) = " << sizeof(std::string)
                  << "，所以 vector 還佔著約 "
                  << (s.capacity() * sizeof(std::string)) << " bytes\n";
    }

    std::cout << "\n=== 各種清空方式的對照 ===\n";
    {
        auto show = [](const char* label, std::vector<int>& x) {
            std::cout << "  " << label << " size=" << x.size()
                      << " capacity=" << x.capacity() << "\n";
        };
        std::vector<int> a(100, 1), b(100, 1), c(100, 1), d(100, 1);
        a.clear();                            show("clear()                  ", a);
        b.erase(b.begin(), b.end());          show("erase(begin, end)        ", b);
        c.resize(0);                          show("resize(0)                ", c);
        std::vector<int>().swap(d);           show("vector<int>().swap(v)    ", d);
        std::cout << "  前三者等價（capacity 保留）；只有 swap 慣用法真的釋放。\n";
        std::cout << "  意圖是「清空」就用 clear()——最直接，而且是 noexcept。\n";
    }

    std::cout << "\n=== 日常實務：批次處理的可重用緩衝區 ===\n";
    {
        std::vector<size_t> batches = {100, 50, 800, 30, 200, 60};
        BatchStats st = runEtl(batches);

        std::cout << "各批筆數    : 100 50 800 30 200 60\n";
        std::cout << "批次數      : " << st.batches << "\n";
        std::cout << "總筆數      : " << st.rows << "\n";
        std::cout << "實際擴容次數: " << st.reallocations << "\n";
        std::cout << "峰值 capacity: " << st.peakCapacity << "\n";
        std::cout << "→ 只有「刷新歷史最大值」的批次才擴容（100 → 800）；\n";
        std::cout << "  50、30、200、60 都放得進既有容量，完全不碰堆積。\n";
        std::cout << "  這正是 clear() 保留 capacity 的設計用意。\n";
        std::cout << "\n代價（誠實面）：跑完那批 800 筆之後，\n";
        std::cout << "  緩衝區的 capacity 就一直停在那個峰值，\n";
        std::cout << "  即使後面都是小批次。長時間執行的服務若在意常駐記憶體，\n";
        std::cout << "  需要週期性地用 swap 慣用法把它釋放掉。\n";
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第 15 課：vector 元素刪除6.cpp -o demo6

// === 預期輸出 ===
// === 原始示範：clear 不改變 capacity ===
// clear 前 - size: 5, capacity: 100
// clear 後 - size: 0, capacity: 100
// → 記憶體完全沒有還給系統，只是「內容清空了」。
//
// === 兩種真正釋放記憶體的方式 ===
// a: size=1000 capacity=1000
// a.clear() 後:            size=0 capacity=1000
// a.shrink_to_fit() 後:    size=0 capacity=0
//   （本機確實縮了，但標準允許實作忽略此請求）
//
// b: size=1000 capacity=1000
// std::vector<int>().swap(b) 後: size=0 capacity=0
//   （建立空的臨時 vector 與 b 交換；臨時物件在行尾解構，
//     把 b 原本那塊記憶體釋放掉——這個做法保證有效）
//
// === clear() 會釋放「元素自己」的資源 ===
// 兩個長字串: size=2 capacity=4
//   每個 string 各自持有約 1000 bytes 的字元緩衝區
// clear() 後: size=0 capacity=4
//   → 兩個 string 的解構子都被呼叫了，那 2000 bytes 已釋放。
//     但 vector 自己那塊「存放 4 個 string 物件」的陣列還在。
//     sizeof(std::string) = 32，所以 vector 還佔著約 128 bytes
//
// === 各種清空方式的對照 ===
//   clear()                   size=0 capacity=100
//   erase(begin, end)         size=0 capacity=100
//   resize(0)                 size=0 capacity=100
//   vector<int>().swap(v)     size=0 capacity=0
//   前三者等價（capacity 保留）；只有 swap 慣用法真的釋放。
//   意圖是「清空」就用 clear()——最直接，而且是 noexcept。
//
// === 日常實務：批次處理的可重用緩衝區 ===
// 各批筆數    : 100 50 800 30 200 60
// 批次數      : 6
// 總筆數      : 1240
// 實際擴容次數: 2
// 峰值 capacity: 1024
// → 只有「刷新歷史最大值」的批次才擴容（100 → 800）；
//   50、30、200、60 都放得進既有容量，完全不碰堆積。
//   這正是 clear() 保留 capacity 的設計用意。
//
// 代價（誠實面）：跑完那批 800 筆之後，
//   緩衝區的 capacity 就一直停在那個峰值，
//   即使後面都是小批次。長時間執行的服務若在意常駐記憶體，
//   需要週期性地用 swap 慣用法把它釋放掉。
