// =============================================================================
//  第 10 課：vector 的宣告與初始化方式 7  —  assign：重設既有 vector 的內容
// =============================================================================
//
// 【主題資訊 Information】
//   void assign(size_type count, const T& value);                    // (1) 數量+值
//   template<class InputIt> void assign(InputIt first, InputIt last); // (2) 迭代器範圍
//   void assign(std::initializer_list<T> ilist);                     // (3) 初始化串列 C++11
//   標頭檔：<vector>
//   標準版本：(1)(2) C++98；(3) C++11。
//   複雜度：O(舊 size + 新 size) —— 先銷毀舊元素，再建構新元素。
//   回傳：void（不是回傳 *this，所以不能鏈式呼叫）。
//   關鍵性質：assign 會改變 size，但【不會縮小 capacity】。
//
// 【詳細解釋 Explanation】
//
// 【1. assign 存在的意義：重用已配置的記憶體】
//   `v = {10, 20, 30};` 和 `v.assign({10, 20, 30});` 結果看起來一樣，
//   但 assign 傳達的意圖更明確：「這個 vector 我要繼續用，只是換內容」。
//   最大的實務價值在迴圈裡：
//       std::vector<char> buf;
//       while (readChunk(...)) {
//           buf.assign(chunk.begin(), chunk.end());   // 重用既有容量
//           process(buf);
//       }
//   只要新內容不超過現有 capacity，assign 就完全不需要向作業系統
//   要新記憶體，也不必釋放舊的。這在高頻迴圈（解析器、封包處理、
//   影像逐幀處理）是很有感的最佳化。
//
// 【2. assign 為什麼不縮小 capacity】
//   這和 clear() 的設計理由一致：縮小容量意味著「釋放記憶體再重新配置」，
//   而使用者很可能馬上又要把它填回去。STL 的一貫策略是
//   「size 由內容決定，capacity 只增不減，除非你明確要求」。
//   要真的把記憶體還回去，得呼叫 shrink_to_fit()——
//   而它是【非約束性請求】，標準允許實作忽略它。
//
// 【3. 三個重載的取捨】
//   (1) assign(count, value)
//       填 count 個相同的值。注意這裡是「小括號語意」——
//       assign(5, 100) 是「五個 100」，不是「5 和 100」。
//   (2) assign(first, last)
//       半開區間 [first, last)。可以來自任何容器，甚至是原生陣列、
//       istream_iterator。這是三者中最泛用的。
//   (3) assign(initializer_list)
//       直接列舉元素。注意 initializer_list 的元素是 const 的，
//       所以這個版本只能複製、不能移動——見【注意事項】第 3 點。
//
// 【4. 自我賦值的陷阱：來源不能是自己的元素】
//   下面這行是未定義行為：
//       v.assign(v.begin() + 1, v.end() - 1);      // 危險！
//   因為 assign 會先清掉 v 的舊內容，而 first/last 正指向那些舊內容。
//   一旦舊元素被銷毀，兩個迭代器就變成懸空迭代器。
//   標準明訂 assign 的來源範圍不得指向 *this 的元素。
//   要做這件事，必須先複製到暫存容器，或改用 erase：
//       v.erase(v.begin());  v.pop_back();          // 安全的等價操作
//
// 【概念補充 Concept Deep Dive】
//
// (A) assign 內部做了什麼（libstdc++ 的策略）
//     大致分三種情況：
//       新 size <= 舊 size ：就地賦值前 n 個，再銷毀多餘的尾巴
//       新 size <= capacity：就地賦值舊有的，其餘用 placement 建構
//       新 size >  capacity：只能重新配置（此時所有 iterator/reference 失效）
//     前兩種完全不碰堆積，這就是 assign 相對「先 clear 再 push_back」
//     的優勢——後者雖然也不縮容量，但少了「就地賦值」這條路。
//
// (B) 為什麼 assign 用「賦值」而不是「銷毀+建構」
//     對 std::string 這種元素，賦值可以重用它自己內部已配置的緩衝區：
//       s = "hi";   // 若 s 原本容量夠，直接覆寫字元，不重新配置
//     若先銷毀再建構，那塊字串緩衝區就白白還回去又要再要一次。
//     這是 STL 一層層「重用勝於重配」思路的縮影。
//
// (C) assign 與 operator= 的差別
//     v = other;                 // 複製賦值，內容變成 other 的完整複本
//     v.assign(o.begin(), o.end());  // 效果相同，但不需要 other 是同型別容器
//     assign 的迭代器版本可以跨容器型別：
//       std::list<int> l = {...};
//       v.assign(l.begin(), l.end());   // list -> vector，operator= 做不到
//
// 【注意事項 Pay Attention】
//   1. assign 的來源範圍不可指向 *this 自己的元素（未定義行為）。
//   2. assign 一定會使原有的 iterator / reference / pointer 全部失效
//      ——即使沒有重新配置，元素的值也已經被換掉，語意上不該再用。
//   3. initializer_list 版本只能複製、不能移動：ilist 的元素是 const T，
//      無法從中移動走資源。若元素是 std::string 且很大，
//      用 assign(first, last) 搭配 move_iterator 才能避免深複製。
//   4. assign 不縮小 capacity。若剛處理完一個超大批次、想把記憶體還回去，
//      要另外呼叫 shrink_to_fit()，而且它是非約束性請求，
//      標準允許實作不理會它——不能假設它「一定會」縮容。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】vector::assign
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. v.assign(...) 之後，v 的 capacity 會怎麼變？
//     答：只會增加，不會減少。若新內容放得進現有 capacity，
//         capacity 完全不變（這正是 assign 的效能價值——重用既有記憶體）；
//         放不下才重新配置、capacity 變大。
//         要縮回去只能呼叫 shrink_to_fit()，而它是非約束性請求，
//         標準允許實作忽略，不保證一定縮容。
//     追問：那 assign 之後原本的 iterator 還能用嗎？
//         → 不能。即使沒有重新配置，元素的值已被替換，
//           標準也明訂 assign 會使所有 iterator/reference/pointer 失效。
//
// 🔥 Q2. `v.assign(5, 100)` 和 `v = {5, 100}` 分別得到什麼？
//     答：assign(5, 100) 走的是「數量 + 值」的重載 → 五個 100，size 是 5。
//         v = {5, 100} 走 initializer_list → 兩個元素 5 和 100，size 是 2。
//         這正是本課核心的「小括號 vs 大括號」差異，在 assign 上同樣成立。
//     追問：那 v.assign({5, 100}) 呢？
//         → 大括號 → initializer_list 版本 → 兩個元素，和 v = {5,100} 相同。
//
// ⚠️ 陷阱. `v.assign(v.begin() + 1, v.end() - 1);` 想「掐頭去尾」，
//         編譯得過、小資料測起來好像也對，為什麼是錯的？
//     答：這是未定義行為。assign 會先銷毀 v 現有的元素，
//         但傳進去的 first/last 正指向那些即將被銷毀的元素——
//         一旦銷毀，它們就成了懸空迭代器，後續的讀取全部無效。
//         標準明文要求 assign 的來源區間不得指向 *this 的元素。
//         正確做法：先複製到暫存容器再 assign，
//         或直接用 v.erase(v.begin()); v.pop_back();
//     為什麼會錯：以為 assign 是「先把新內容算出來，再整批換掉」。
//         實際順序相反——它是「邊清舊的、邊放新的」，
//         來源與目的地重疊時就出事。這類「來源與目的地重疊」的
//         未定義行為在 STL 裡很常見（std::copy 也有同樣限制）。
// ═══════════════════════════════════════════════════════════════════════════

#include <vector>
#include <iostream>
#include <string>
#include <list>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】—— 本檔刻意不放
//   理由：assign 是「容器生命週期管理」的工具，關心的是記憶體重用與
//   capacity 行為。LeetCode 的題目只在意「輸入 → 輸出」的演算法正確性，
//   沒有任何一題的難點會落在「怎麼重設一個既有 vector 的內容」上。
//   硬找一題來套，只會變成「在解法中間莫名其妙呼叫一次 assign」，
//   對理解 assign 毫無幫助。本主題的價值在下方的實務範例裡。
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 【日常實務範例】固定緩衝區的批次匯入（assign 重用記憶體）
//   情境：從資料來源一批一批讀取記錄（每批大小不定），逐批處理後寫入資料庫。
//         這是 ETL / 批次匯入程式最典型的骨架。
//   為什麼用到本主題：緩衝區宣告在迴圈【外面】，每批用 assign 換內容。
//     只要某一批的大小沒超過歷史最大值，就完全不會再向堆積要記憶體——
//     capacity 停留在歷史高水位，之後每批都是零配置。
//     若把 vector 宣告在迴圈裡面，每批都要配置+釋放一次。
//   下方 main 會實測印出 capacity 的變化，證明這個效果。
// -----------------------------------------------------------------------------
struct ImportStats {
    size_t batches;
    size_t totalRows;
    size_t reallocations;   // capacity 改變的次數
};

ImportStats importBatches(const std::vector<std::vector<int>>& source) {
    ImportStats st{0, 0, 0};

    std::vector<int> buffer;          // 宣告在迴圈外 —— 這是重點
    size_t lastCap = buffer.capacity();

    for (const auto& batch : source) {
        // 用 assign 換內容：能重用就重用，放不下才擴容
        buffer.assign(batch.begin(), batch.end());

        if (buffer.capacity() != lastCap) {
            ++st.reallocations;
            lastCap = buffer.capacity();
        }

        // …這裡才是真正的處理（寫 DB、轉檔、校驗）…
        st.totalRows += buffer.size();
        ++st.batches;
    }
    return st;
}

int main() {
    std::vector<int> v = {1, 2, 3};

    std::cout << "=== 原始示範：assign 的三種重載 ===\n";

    // 方法一：指定數量和值
    v.assign(5, 100);
    std::cout << "assign(5, 100): ";
    for (int x : v) std::cout << x << " ";  // 輸出：100 100 100 100 100
    std::cout << std::endl;

    // 方法二：從初始化串列
    v.assign({10, 20, 30});
    std::cout << "assign({10, 20, 30}): ";
    for (int x : v) std::cout << x << " ";  // 輸出：10 20 30
    std::cout << std::endl;

    // 方法三：從迭代器範圍
    std::vector<int> other = {7, 8, 9, 10, 11};
    v.assign(other.begin() + 1, other.end() - 1);
    std::cout << "從迭代器範圍: ";
    for (int x : v) std::cout << x << " ";  // 輸出：8 9 10
    std::cout << std::endl;

    std::cout << "\n=== assign 只增不減 capacity ===\n";
    {
        std::vector<int> w;
        w.assign(1000, 7);
        std::cout << "assign(1000, 7) 後  size=" << w.size()
                  << " capacity=" << w.capacity() << "\n";
        w.assign(3, 1);
        std::cout << "assign(3, 1)    後  size=" << w.size()
                  << " capacity=" << w.capacity() << "  ← capacity 沒有跟著縮\n";
        w.shrink_to_fit();
        std::cout << "shrink_to_fit() 後  size=" << w.size()
                  << " capacity=" << w.capacity() << "\n";
        std::cout << "（shrink_to_fit 是「非約束性請求」，標準允許實作忽略，\n";
        std::cout << "  上面這個結果是本機 libstdc++ 的行為，不是標準保證）\n";
    }

    std::cout << "\n=== assign 的迭代器版本可以跨容器型別 ===\n";
    {
        std::list<int> l = {100, 200, 300};   // list 不是 vector
        std::vector<int> fromList;
        fromList.assign(l.begin(), l.end());  // operator= 做不到這件事
        std::cout << "從 std::list assign 到 vector: ";
        for (int x : fromList) std::cout << x << " ";
        std::cout << "\n";

        int raw[] = {11, 22, 33};             // 原生陣列也可以
        fromList.assign(std::begin(raw), std::end(raw));
        std::cout << "從原生陣列 assign:            ";
        for (int x : fromList) std::cout << x << " ";
        std::cout << "\n";
    }

    std::cout << "\n=== assign(count,value) vs assign({...}) 的大小括號差異 ===\n";
    {
        std::vector<int> a, b;
        a.assign(5, 100);       // 小括號 → 五個 100
        b.assign({5, 100});     // 大括號 → 兩個元素 5 和 100
        std::cout << "a.assign(5, 100)  -> size=" << a.size() << " 內容:";
        for (int x : a) std::cout << " " << x;
        std::cout << "\n";
        std::cout << "b.assign({5, 100})-> size=" << b.size() << " 內容:";
        for (int x : b) std::cout << " " << x;
        std::cout << "\n";
    }

    std::cout << "\n=== 日常實務：批次匯入時用 assign 重用緩衝區 ===\n";
    {
        // 模擬六個批次，大小起伏不定
        std::vector<std::vector<int>> batches;
        for (size_t n : {10u, 50u, 30u, 200u, 80u, 150u}) {
            batches.emplace_back(n, 1);
        }

        ImportStats st = importBatches(batches);
        std::cout << "批次數        : " << st.batches << "\n";
        std::cout << "總筆數        : " << st.totalRows << "\n";
        std::cout << "實際擴容次數  : " << st.reallocations << "\n";
        std::cout << "各批大小      : 10 50 30 200 80 150\n";
        std::cout << "→ 只有「刷新歷史最大值」的那幾批才擴容（10/50/200）；\n";
        std::cout << "  30、80、150 都放得進既有容量，完全不碰堆積。\n";
        std::cout << "  若把 vector 宣告在迴圈裡，這裡會是 6 次配置 + 6 次釋放。\n";
    }

    std::cout << "\n=== 危險示範（僅說明，不執行）===\n";
    std::cout << "v.assign(v.begin()+1, v.end()-1);  // 未定義行為！\n";
    std::cout << "來源區間指向 v 自己的元素，assign 會先銷毀它們 → 懸空迭代器。\n";
    std::cout << "等價的安全寫法: v.erase(v.begin()); v.pop_back();\n";
    {
        std::vector<int> safe = {1, 2, 3, 4, 5};
        safe.erase(safe.begin());
        safe.pop_back();
        std::cout << "安全版結果: ";
        for (int x : safe) std::cout << x << " ";
        std::cout << "\n";
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第 10 課：vector 的宣告與初始化方式7.cpp -o demo7

// === 預期輸出 ===
// === 原始示範：assign 的三種重載 ===
// assign(5, 100): 100 100 100 100 100
// assign({10, 20, 30}): 10 20 30
// 從迭代器範圍: 8 9 10
//
// === assign 只增不減 capacity ===
// assign(1000, 7) 後  size=1000 capacity=1000
// assign(3, 1)    後  size=3 capacity=1000  ← capacity 沒有跟著縮
// shrink_to_fit() 後  size=3 capacity=3
// （shrink_to_fit 是「非約束性請求」，標準允許實作忽略，
//   上面這個結果是本機 libstdc++ 的行為，不是標準保證）
//
// === assign 的迭代器版本可以跨容器型別 ===
// 從 std::list assign 到 vector: 100 200 300
// 從原生陣列 assign:            11 22 33
//
// === assign(count,value) vs assign({...}) 的大小括號差異 ===
// a.assign(5, 100)  -> size=5 內容: 100 100 100 100 100
// b.assign({5, 100})-> size=2 內容: 5 100
//
// === 日常實務：批次匯入時用 assign 重用緩衝區 ===
// 批次數        : 6
// 總筆數        : 520
// 實際擴容次數  : 3
// 各批大小      : 10 50 30 200 80 150
// → 只有「刷新歷史最大值」的那幾批才擴容（10/50/200）；
//   30、80、150 都放得進既有容量，完全不碰堆積。
//   若把 vector 宣告在迴圈裡，這裡會是 6 次配置 + 6 次釋放。
//
// === 危險示範（僅說明，不執行）===
// v.assign(v.begin()+1, v.end()-1);  // 未定義行為！
// 來源區間指向 v 自己的元素，assign 會先銷毀它們 → 懸空迭代器。
// 等價的安全寫法: v.erase(v.begin()); v.pop_back();
// 安全版結果: 2 3 4
