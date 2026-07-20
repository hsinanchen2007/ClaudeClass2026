// =============================================================================
//  第 19 課：vector 與原始陣列的互操作 15  —  重新配置使 data() 指標失效
// =============================================================================
//
// 【主題資訊 Information】
//   T* vector<T>::data() noexcept;                       // C++11 起
//   void push_back(const T& value);  void push_back(T&& value);
//   size_type capacity() const noexcept;
//
//   標頭檔：<vector>
//   複雜度：push_back 攤銷 O(1)；發生重新配置的那一次是 O(n)
//   本檔標準：C++17
//   本檔性質：**危險示範**。使用失效指標的那一行「刻意保持註解狀態」，
//             檔案本身可正常編譯執行。
//
// 【詳細解釋 Explanation】
//
// 【1. 重新配置（reallocation）到底發生了什麼】
//   vector 必須維持「所有元素在記憶體中連續」這個保證。當 size 即將超過
//   capacity 時，原地擴張通常不可行（後面那塊記憶體可能已被別人占用），
//   所以 vector 只能：
//       ① 向 allocator 要一塊更大的新記憶體
//       ② 把既有元素逐一搬到新位置（可移動就 move，否則 copy）
//       ③ 銷毀舊元素、釋放舊記憶體
//   第 ③ 步一結束，所有指向舊緩衝區的指標／iterator／reference 全部失效。
//   注意這不是「值被改掉」，而是「那塊記憶體已經還給配置器了」。
//
// 【2. 成長倍率：為什麼是 2 倍（實作定義）】
//   標準只要求 push_back 的**攤銷**複雜度是 O(1)，沒有規定倍率。
//   實務上要達到攤銷 O(1)，成長必須是「乘法」而非「加法」：
//     * 每次 +1 → 插入 n 個元素要搬 1+2+...+n = O(n²)
//     * 每次 ×k → 搬移總量是等比級數，收斂到 O(n)
//   本機 g++ 15.2 / libstdc++ 實測為 **2 倍**（1→2→4→8→16→…），
//   MSVC 的 STL 則是 **1.5 倍**。這兩個都是實作選擇，不是標準保證。
//   1.5 倍的理論優勢是：釋放出來的舊區塊有機會被後續配置重複利用
//   （因為 1+1.5+2.25… 的前綴和會超過下一次的需求，2 倍則永遠不會）。
//
// 【3. 為什麼 reserve 能解決問題】
//   reserve(n) 一次把 capacity 拉到至少 n，之後只要 size 不超過 n，
//   就保證不會重新配置，指標與 iterator 也就保持有效。
//   這是「先知道總量再批次填入」時的標準做法，同時省下多次搬移成本。
//   ⚠️ 但 reserve **不改變 size**，只改變 capacity。
//
// 【4. 用「索引」取代「指標」】
//   如果一定要在容器可能變動的期間記住某個位置，正解是記**索引**而不是指標：
//   索引是「第幾個元素」的邏輯位置，重新配置後依然正確；
//   指標是「哪個位址」的實體位置，重新配置後就沒有意義了。
//   代價是每次存取多一次 v[i] 的位址計算——對 vector 而言只是一次加法。
//
// 【概念補充 Concept Deep Dive】
//   ● 移動 vs 複製：noexcept 的關鍵作用
//     重新配置時，vector 只有在元素的 move constructor 標了 noexcept
//     （或型別不可複製）時才會使用移動；否則為了維持強例外保證會退回複製。
//     這就是「自訂型別的 move constructor 一定要加 noexcept」這條建議的由來——
//     忘了加，你的 vector 擴容會默默從 move 退化成 copy，效能差距可能很大。
//
//   ● 攤銷 O(1) 的直覺
//     以 2 倍成長插入 n 個元素，搬移總次數約為 1+2+4+…+n < 2n，
//     平均每次插入搬移不到 2 個元素 → 攤銷 O(1)。
//     但「攤銷 O(1)」不代表「每次都快」：觸發擴容的那一次是實打實的 O(n)，
//     這對即時系統（音訊回呼、遊戲主迴圈）是不可接受的延遲尖峰，
//     所以那類程式一律事先 reserve。
//
//   ● 為什麼 pop_back 不會讓指標失效
//     pop_back 只銷毀最後一個元素並把 size 減一，緩衝區完全不動，
//     所以指向其他元素的指標仍然有效（指向被刪那一格的則不再有效）。
//     capacity 也不會因此縮小。
//
// 【注意事項 Pay Attention】
//   1. 使用失效指標是 **未定義行為**，不保證崩潰、不保證印出任何特定值，
//      也不保證兩次執行結果相同。**不可以說「它會印出舊值」。**
//   2. 「容量還夠，所以這次 push_back 不會失效」在單一程式裡也許成立，
//      但這是實作細節。安全的紀律是：**任何可能改變容器的操作之後，重新取得指標**。
//   3. reserve 只保證「size 不超過 reserve 的量」這段期間不重新配置；
//      一旦超過就照樣失效。
//   4. 迴圈中一邊迭代一邊 push_back 是經典錯誤——range-for 內部保存的
//      iterator 會在擴容後失效，而且這種程式常常「小資料量測不出來」。
//   5. 本檔第 3 節示範的 capacity 成長序列（1,2,4,8,…）是本機 libstdc++
//      的實測結果，**不是標準保證**，換編譯器可能不同。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】vector 重新配置與指標失效
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 push_back 之後，先前 v.data() 取得的指標可能失效？
//     答：vector 要維持元素連續。size 超過 capacity 時只能配置一塊更大的記憶體、
//         把元素搬過去、釋放舊的。舊緩衝區被歸還後，所有指向它的指標／iterator
//         全部失效。這與 iterator 失效是同一件事。
//     追問：那 capacity 沒滿的 push_back 呢？→ 不會重新配置，指標仍有效。
//         但這是實作細節，不該依賴；正確紀律是每次改動容器後重新取得指標。
//
// 🔥 Q2. vector 為什麼採「倍數成長」而不是「每次加固定量」？
//     答：為了讓 push_back 達到攤銷 O(1)。每次加固定量的話，
//         插入 n 個元素的搬移總量是 O(n²)；倍數成長則是等比級數，總量 O(n)。
//         libstdc++ 用 2 倍、MSVC 用 1.5 倍，都是實作選擇，標準沒有規定。
//     追問：1.5 倍有什麼好處？→ 釋放出的舊區塊有機會被後續配置重用
//         （前綴和會超過下次需求），2 倍則永遠不會，記憶體局部性略差。
//
// ⚠️ 陷阱. 「我 push_back 的元素不多，vector 應該不會搬家，指標留著沒關係」
//     答：這個推論在單次執行、單一編譯器下也許碰巧成立，但它依賴的是
//         capacity 的初始值與成長策略——兩者都是實作定義。
//         更糟的是，就算它「這次沒事」，程式碼一改、資料量一變就爆炸，
//         而且是在遠離現場的地方爆炸。
//     為什麼會錯：把「攤銷 O(1)」誤解成「大部分時候不會重新配置，所以可以賭」。
//         正確的心智模型是：**push_back 之後，就當作所有指標都失效了**。
//         這條紀律的成本只是多呼叫一次 data()，收益是徹底消滅一整類 bug。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <string>

// -----------------------------------------------------------------------------
// 【日常實務範例】封包組裝器：為什麼一定要先 reserve，或改用索引
//   情境：網路程式常把多個欄位序列化進同一個 vector<unsigned char>，
//         並且需要「先佔位、稍後回填長度」（長度要等內容組完才知道）。
//         新手會先取 header 的指標再繼續 append，結果 append 觸發擴容，
//         回填時就寫到已釋放的記憶體上——這是真實世界會出現的資安等級 bug。
//   正解有兩個：① 先 reserve 足夠空間；② 記「位移量（索引）」而不是指標。
//   本函式示範 ②，因為它在任何情況下都正確，不需要事先猜測總長度。
// -----------------------------------------------------------------------------
std::vector<unsigned char> build_packet(const std::string& payload) {
    std::vector<unsigned char> pkt;

    pkt.push_back(0xAB);                  // magic
    pkt.push_back(0x00);                  // 長度欄位佔位（稍後回填）
    const std::size_t lengthFieldIndex = pkt.size() - 1;   // 記「索引」，不是指標

    for (char c : payload) {
        pkt.push_back(static_cast<unsigned char>(c));      // 這裡可能觸發擴容
    }

    // 回填長度：用索引存取，重新配置過也依然正確
    pkt[lengthFieldIndex] = static_cast<unsigned char>(payload.size());
    return pkt;
}

// 註：本檔不附 LeetCode 範例。「指標／iterator 失效」是容器實作與記憶體管理議題，
//     LeetCode 判題只看回傳值，不會考這種生命週期問題；
//     真正相關的 in-place 陣列題型已在同課 8.cpp（LeetCode 26）示範過。

// -----------------------------------------------------------------------------
// 用來「數搬移次數」的儀器型別。
// 刻意只提供 copy constructor（不提供 move constructor），
// 這樣 vector 重新配置時一定走複製路徑，計數完全可重現。
// 注意：C++ 不允許 local class 擁有 static data member，故必須放在檔案範圍。
// -----------------------------------------------------------------------------
struct Counted {
    static long relocations;          // 累計被搬移（複製建構）的次數
    int v;
    explicit Counted(int x = 0) : v(x) {}
    Counted(const Counted& o) : v(o.v) { ++relocations; }
    Counted& operator=(const Counted& o) { v = o.v; return *this; }
};
long Counted::relocations = 0;

int main() {
    std::cout << "=== 1. push_back 可能讓 data() 指標失效 ===\n";
    std::vector<int> v = {10, 20, 30};
    int* ptr = v.data();

    std::cout << "push_back 前：ptr[0] = " << ptr[0] << std::endl;
    std::cout << "push_back 前：size=" << v.size()
              << ", capacity=" << v.capacity() << "\n";

    // push_back 可能觸發重新配置
    for (int i = 0; i < 100; ++i) {
        v.push_back(i);
    }

    std::cout << "push_back 100 次後：size=" << v.size()
              << ", capacity=" << v.capacity() << "\n";

    // ptr 可能已經失效！（和迭代器失效同理）
    // ⚠️ 下面這行是未定義行為，故意保持註解狀態。
    //    不保證印出舊值、不保證崩潰、不保證每次結果相同。
    // std::cout << "push_back 後：ptr[0] = " << ptr[0] << std::endl;  // 危險！

    // 正確：重新取得指標
    ptr = v.data();
    std::cout << "重新取得後：ptr[0] = " << ptr[0] << std::endl;

    std::cout << "\n=== 2. reserve 之後就不會重新配置 ===\n";
    std::vector<int> r;
    r.reserve(200);                        // 一次拉到足夠容量
    std::cout << "reserve(200)：size=" << r.size()
              << ", capacity=" << r.capacity()
              << "（注意 size 仍是 0！）\n";

    for (int i = 0; i < 100; ++i) r.push_back(i);
    std::cout << "填入 100 個後：size=" << r.size()
              << ", capacity=" << r.capacity()
              << " → capacity 完全沒變，代表沒有發生重新配置\n";

    std::cout << "\n=== 3. 實測 capacity 成長序列（本機 libstdc++）===\n";
    std::vector<int> g;
    std::size_t lastCap = g.capacity();
    std::cout << "起始 capacity = " << lastCap << "\n";
    std::cout << "成長序列：";
    int reallocCount = 0;
    for (int i = 0; i < 1000; ++i) {
        g.push_back(i);
        if (g.capacity() != lastCap) {
            std::cout << g.capacity() << " ";
            lastCap = g.capacity();
            ++reallocCount;
        }
    }
    std::cout << "\n插入 1000 個元素，共重新配置 " << reallocCount << " 次\n";
    std::cout << "（本機 g++ 15.2 / libstdc++ 為 2 倍成長；MSVC 是 1.5 倍。"
                 "兩者皆為實作定義，非標準保證）\n";

    std::cout << "\n=== 4. 用「搬移次數」證明倍數成長的必要性 ===\n";
    // 不用計時（每次執行都不同），改用「可重現的計數」當證據
    const int N = 4000;

    // (a) 倍數成長：讓 vector 自己決定何時擴容
    Counted::relocations = 0;
    {
        std::vector<Counted> doubling;
        for (int i = 0; i < N; ++i) doubling.push_back(Counted(i));
    }
    long relocDoubling = Counted::relocations;

    // (b) 模擬「每次只加 1 格」：每次都 reserve(size + 1) 強迫重新配置
    Counted::relocations = 0;
    {
        std::vector<Counted> linear;
        for (int i = 0; i < N; ++i) {
            linear.reserve(linear.size() + 1);   // 強迫每次都重新配置
            linear.push_back(Counted(i));
        }
    }
    long relocLinear = Counted::relocations;

    // (c) 事先 reserve：完全不需要搬移
    Counted::relocations = 0;
    {
        std::vector<Counted> reserved;
        reserved.reserve(N);
        for (int i = 0; i < N; ++i) reserved.push_back(Counted(i));
    }
    long relocReserved = Counted::relocations;

    // 說明：計數器數的是「複製建構的總次數」＝ N 次插入本身 + 擴容時的搬移次數。
    //       所以「純搬移次數」要扣掉 N。事先 reserve 的情況剛好是 N，代表零搬移。
    std::cout << "插入 " << N << " 個元素時，複製建構總次數（＝ N 次插入 + 搬移）：\n";
    std::cout << "  倍數成長（vector 預設）：" << relocDoubling
              << "  → 其中搬移 " << (relocDoubling - N) << " 次\n";
    std::cout << "  每次加 1 格（模擬）    ：" << relocLinear
              << "  → 其中搬移 " << (relocLinear - N) << " 次\n";
    std::cout << "  事先 reserve(" << N << ")     ：" << relocReserved
              << "  → 其中搬移 " << (relocReserved - N) << " 次（完全不搬）\n";
    if (relocDoubling - N > 0) {
        std::cout << "  搬移次數比：每次加 1 格 是 倍數成長的 "
                  << ((relocLinear - N) / (relocDoubling - N))
                  << " 倍 → O(n²) 與 O(n) 的實證\n";
    }
    std::cout << "  對照理論值：倍數成長搬移 = 1+2+4+...+2048 = "
              << (relocDoubling - N)
              << "；每次加 1 格 = 1+2+...+" << (N - 1) << " = "
              << (relocLinear - N) << "\n";
    std::cout << "（用「複製次數」而非耗時：結果每次執行完全相同，可重現、可驗證）\n";

    std::cout << "\n=== 日常實務：封包組裝器用「索引」回填長度欄位 ===\n";
    auto pkt = build_packet("HELLO");
    std::cout << "封包位元組：";
    for (unsigned char b : pkt) {
        std::cout << std::hex << std::uppercase << static_cast<int>(b) << " ";
    }
    std::cout << std::dec << "\n";
    std::cout << "magic=0xAB, 長度欄位=" << static_cast<int>(pkt[1])
              << ", payload 共 " << pkt.size() - 2 << " 位元組\n";

    auto big = build_packet("A rather longer payload that forces reallocation");
    std::cout << "較長封包：長度欄位=" << static_cast<int>(big[1])
              << ", 總長=" << big.size() << " 位元組（中途擴容過，回填依然正確）\n";
    std::cout << "若當初存的是指標而非索引，這裡就會寫進已釋放的記憶體。\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 19 課：vector 與原始陣列的互操作15.cpp" -o demo15

// === 預期輸出 ===
// === 1. push_back 可能讓 data() 指標失效 ===
// push_back 前：ptr[0] = 10
// push_back 前：size=3, capacity=3
// push_back 100 次後：size=103, capacity=192
// 重新取得後：ptr[0] = 10
//
// === 2. reserve 之後就不會重新配置 ===
// reserve(200)：size=0, capacity=200（注意 size 仍是 0！）
// 填入 100 個後：size=100, capacity=200 → capacity 完全沒變，代表沒有發生重新配置
//
// === 3. 實測 capacity 成長序列（本機 libstdc++）===
// 起始 capacity = 0
// 成長序列：1 2 4 8 16 32 64 128 256 512 1024
// 插入 1000 個元素，共重新配置 11 次
// （本機 g++ 15.2 / libstdc++ 為 2 倍成長；MSVC 是 1.5 倍。兩者皆為實作定義，非標準保證）
//
// === 4. 用「搬移次數」證明倍數成長的必要性 ===
// 插入 4000 個元素時，複製建構總次數（＝ N 次插入 + 搬移）：
//   倍數成長（vector 預設）：8095  → 其中搬移 4095 次
//   每次加 1 格（模擬）    ：8002000  → 其中搬移 7998000 次
//   事先 reserve(4000)     ：4000  → 其中搬移 0 次（完全不搬）
//   搬移次數比：每次加 1 格 是 倍數成長的 1953 倍 → O(n²) 與 O(n) 的實證
//   對照理論值：倍數成長搬移 = 1+2+4+...+2048 = 4095；每次加 1 格 = 1+2+...+3999 = 7998000
// （用「複製次數」而非耗時：結果每次執行完全相同，可重現、可驗證）
//
// === 日常實務：封包組裝器用「索引」回填長度欄位 ===
// 封包位元組：AB 5 48 45 4C 4C 4F
// magic=0xAB, 長度欄位=5, payload 共 5 位元組
// 較長封包：長度欄位=48, 總長=50 位元組（中途擴容過，回填依然正確）
// 若當初存的是指標而非索引，這裡就會寫進已釋放的記憶體。
