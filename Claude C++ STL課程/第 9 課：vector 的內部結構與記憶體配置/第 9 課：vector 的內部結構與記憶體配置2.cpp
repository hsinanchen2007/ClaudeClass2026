// =============================================================================
//  第 9 課 2  —  vector 的擴容策略：capacity 如何成長
// =============================================================================
//
// 【主題資訊 Information】
//   size_type size()     const noexcept;   // 實際元素數 = _end - _begin
//   size_type capacity() const noexcept;   // 已配置容量 = _cap - _begin
//   void      reserve(size_type n);        // 預先配置至少 n 的容量
//   標頭檔  : <vector>
//   複雜度  : push_back 為「攤銷 O(1)」（amortized constant），單次最壞 O(n)
//   成長倍率: 實作定義。libstdc++／libc++ 為 2 倍，MSVC 約 1.5 倍。
//             本機（g++ 15.2 / libstdc++）實測序列為 1,2,4,8,16,32,64,128。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼要有 capacity 這個概念】
//   如果 vector 每次 push_back 都剛好配置 size+1 的空間，那加入 n 個元素
//   就要複製 0+1+2+...+(n-1) = n(n-1)/2 次，也就是 O(n²)。
//   加入 100 萬個元素會是約 5×10¹¹ 次複製——完全不可用。
//   解法是「每次不夠時就多要一些」，用空間換時間。
//
// 【2. 為什麼倍率成長能得到「攤銷 O(1)」】
//   關鍵在於等比級數。以 2 倍成長為例，把 n 個元素推進去，
//   總搬移次數是 1 + 2 + 4 + ... + n/2 + n < 2n。
//   也就是說 n 次 push_back 的總成本是 O(n)，平均下來每次 O(1)。
//   注意「攤銷 O(1)」不等於「每次都 O(1)」——觸發擴容的那一次是 O(n)。
//   即時性要求高的系統（音訊、遊戲主迴圈）必須先 reserve，
//   避免某一幀突然被一次 O(n) 的搬移拖住。
//
// 【3. 為什麼是 2 倍？1.5 倍又有什麼好處】
//   這是記憶體重用與擴容次數的權衡：
//     * 2 倍：擴容次數最少，但新區塊永遠大於「先前所有已釋放區塊的總和」
//       （1+2+4+...+2^(k-1) = 2^k - 1 < 2^k），所以釋放的舊空間永遠拼不出
//       下一次需要的大小，配置器較難就地重用。
//     * 1.5 倍：成長較慢、擴容次數較多，但因為 1+1.5+2.25 > 3.375，
//       釋放過的空間累積後有機會滿足後續需求，對某些配置器更友善。
//   標準只要求「攤銷 O(1)」，並未規定倍率，所以這是實作定義，
//   絕對不可以把 2 倍寫死進程式邏輯。
//
// 【4. reserve 的正確用法與唯一注意點】
//   已知大概要放多少元素時，先 reserve 可以把擴容次數降到 0～1 次
//   （本檔會用「實際計數」證明這件事）。
//   注意 reserve 只改 capacity、不改 size：
//       std::vector<int> v; v.reserve(10);
//       v[0] = 1;      // UB！size() 還是 0，v[0] 是越界
//   要能直接索引請用 resize()；只是要避免擴容則用 reserve()。
//
// 【概念補充 Concept Deep Dive】
//   本檔刻意用「重新配置次數」而不是「執行耗時」來衡量成本，理由有二：
//     (a) 可重現：計數在同一份程式碼上永遠一致；耗時受 CPU 頻率、
//         快取狀態、系統負載影響，每次都不同，寫進預期輸出就是假資料。
//     (b) 證據更強：計數直接對應「發生了幾次配置 + 搬移」這個因果機制，
//         而耗時只是這個機制的間接後果。
//   本機實測：推入 1,000,000 個 int，不 reserve 需要 21 次重新配置；
//   先 reserve(1000000) 則是 0 次。
//   另外，capacity 只增不減：erase、clear、pop_back 都不會釋放記憶體。
//   要真的還記憶體給系統得用 shrink_to_fit()（C++11，且僅為非約束性請求），
//   或經典的 swap trick：std::vector<int>(v).swap(v)。
//
// 【注意事項 Pay Attention】
// 1. 成長倍率是實作定義（libstdc++ 2 倍、MSVC 約 1.5 倍）。
//    本檔輸出的 1,2,4,8,... 序列在其他標準庫上會不同。
// 2. 任何造成重新配置的操作（push_back 擴容、reserve、resize、insert、
//    shrink_to_fit）都會使既有的迭代器／指標／參考全部失效，再用即 UB。
// 3. reserve 只改 capacity 不改 size；reserve 後直接索引是越界（UB）。
// 4. capacity 只增不減；clear() 之後 capacity 仍然保持原樣。
// 5. reserve 給的是「至少」n，實作可能給更多；不要假設
//    reserve(n) 之後 capacity() 一定精確等於 n（實務上多半相等，但非保證）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】vector 擴容策略
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. push_back 的複雜度是 O(1) 嗎？請說精確一點。
//     答：是「攤銷 O(1)」（amortized constant）。多數呼叫只是寫一個元素、
//         O(1)；但容量滿時要配置新空間並搬移全部 n 個元素，那一次是 O(n)。
//         因為採倍率成長，n 次 push_back 的總搬移量小於 2n，
//         平均下來每次仍是常數。
//     追問：那什麼場景不能接受「攤銷」？→ 即時系統（音訊回呼、遊戲每幀、
//         控制迴路）。平均值再漂亮，某一次突然 O(n) 就會造成掉幀或爆音，
//         這種場合必須事先 reserve。
//
// 🔥 Q2. vector 的成長倍率是 2 倍嗎？
//     答：標準沒有規定，只要求 push_back 攤銷 O(1)。libstdc++ 與 libc++
//         用 2 倍，MSVC 約 1.5 倍。本機實測序列是 1,2,4,8,16,32,64,128。
//         這是實作定義，不能寫死在程式邏輯裡。
//     追問：為什麼有人主張 1.5 倍比較好？→ 2 倍時新區塊必定大於先前所有
//         已釋放區塊的總和，舊空間永遠拼不出下一次的需求；1.5 倍則讓
//         釋放過的記憶體有機會被重複利用。
//
// ⚠️ 陷阱. std::vector<int> v; v.reserve(10); v[0] = 42;  錯在哪？
//     答：reserve 只配置容量，size() 依然是 0。v[0] 是對「不存在的元素」
//         做存取，屬於越界，是 UB。要能直接索引必須用 resize(10)。
//     為什麼會錯：因為 reserve 之後記憶體「確實已經配置好了」，
//         寫進去通常不會崩潰，看起來完全正常——甚至印出來就是 42。
//         但 size() 仍是 0，走訪、size()、迭代器全都看不到這個元素，
//         而且下一次 push_back 會直接覆蓋它。這是典型「不崩潰的 UB」，
//         比 segfault 難查得多。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】從缺。
//   理由：本檔主題是 vector 的記憶體配置策略，屬於效能與實作機制層面。
//   LeetCode 題目不會測量 capacity，也不因擴容次數而判定對錯
//   （最多影響執行時間）。清單中沒有任何一題的核心是「容量成長」，
//   硬套只會模糊焦點，故從缺，改以實務範例呈現 reserve 的實際效益。

#include <vector>
#include <iostream>
#include <string>

// -----------------------------------------------------------------------------
// 【日常實務範例】批次載入資料時先 reserve
//   場景：從資料庫／CSV 讀進固定筆數的記錄，或把一批 log 行解析成結構。
//         筆數在讀取前就已知（SELECT COUNT(*)、檔案行數、封包標頭的長度欄位）。
//   為什麼要 reserve：不 reserve 時 vector 會反覆配置與搬移；已知筆數卻
//     不告訴它，等於白白付出多次記憶體配置與元素搬移的成本。
//   下面用「實際計數的重新配置次數」來量化，而不是量時間——
//     計數可重現，而且直接對應成本發生的機制。
// -----------------------------------------------------------------------------
// 回傳：推入 n 個元素過程中「重新配置」發生的次數
static int countReallocations(int n, bool use_reserve) {
    std::vector<int> v;
    if (use_reserve) v.reserve(static_cast<size_t>(n));

    size_t cap = v.capacity();
    int reallocs = 0;
    for (int i = 0; i < n; ++i) {
        v.push_back(i);
        if (v.capacity() != cap) {   // capacity 改變 ⇒ 剛剛發生一次重新配置
            ++reallocs;
            cap = v.capacity();
        }
    }
    return reallocs;
}

int main() {
    std::cout << "=== capacity 成長序列（前 100 次 push_back）===" << std::endl;
    std::vector<int> v;

    size_t prev_cap = 0;
    for (int i = 0; i < 100; ++i) {
        v.push_back(i);
        if (v.capacity() != prev_cap) {
            std::cout << "size: " << v.size()
                      << ", capacity: " << v.capacity() << std::endl;
            prev_cap = v.capacity();
        }
    }
    std::cout << "（本機 libstdc++ 為 2 倍成長；MSVC 約 1.5 倍，屬實作定義）"
              << std::endl;

    // ---- capacity 只增不減 ----
    std::cout << "\n=== capacity 只增不減 ===" << std::endl;
    std::cout << "clear() 前: size=" << v.size()
              << ", capacity=" << v.capacity() << std::endl;
    v.clear();
    std::cout << "clear() 後: size=" << v.size()
              << ", capacity=" << v.capacity() << "（記憶體並未歸還）" << std::endl;
    v.shrink_to_fit();
    std::cout << "shrink_to_fit() 後: size=" << v.size()
              << ", capacity=" << v.capacity()
              << "（非約束性請求，實作可以不理會）" << std::endl;

    // ---- 日常實務：用「計數」量化 reserve 的效益 ----
    std::cout << "\n=== 日常實務: reserve 的效益（以重新配置次數衡量）===" << std::endl;
    const int N = 1000000;
    int without = countReallocations(N, false);
    int with    = countReallocations(N, true);
    std::cout << "推入 " << N << " 個 int：" << std::endl;
    std::cout << "  不 reserve       -> 重新配置 " << without << " 次" << std::endl;
    std::cout << "  先 reserve(" << N << ") -> 重新配置 " << with << " 次" << std::endl;
    std::cout << "（刻意計算次數而非量時間：次數可重現，且直接對應"
                 "「配置 + 搬移」實際發生的機制）" << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第 9 課：vector 的內部結構與記憶體配置2.cpp -o vector_growth

// 【但書】capacity 的成長序列（1,2,4,8,...）與「重新配置 21 次」都是
//   本機 g++ 15.2 / libstdc++ 的實作定義結果。MSVC 約 1.5 倍成長，
//   序列與次數都會不同。本檔刻意輸出「次數」而非「耗時」，
//   因為次數可重現、也直接對應成本發生的機制。

// === 預期輸出 ===
// === capacity 成長序列（前 100 次 push_back）===
// size: 1, capacity: 1
// size: 2, capacity: 2
// size: 3, capacity: 4
// size: 5, capacity: 8
// size: 9, capacity: 16
// size: 17, capacity: 32
// size: 33, capacity: 64
// size: 65, capacity: 128
// （本機 libstdc++ 為 2 倍成長；MSVC 約 1.5 倍，屬實作定義）
//
// === capacity 只增不減 ===
// clear() 前: size=100, capacity=128
// clear() 後: size=0, capacity=128（記憶體並未歸還）
// shrink_to_fit() 後: size=0, capacity=0（非約束性請求，實作可以不理會）
//
// === 日常實務: reserve 的效益（以重新配置次數衡量）===
// 推入 1000000 個 int：
//   不 reserve       -> 重新配置 21 次
//   先 reserve(1000000) -> 重新配置 0 次
// （刻意計算次數而非量時間：次數可重現，且直接對應「配置 + 搬移」實際發生的機制）
