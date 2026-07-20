// =============================================================================
//  第 19 課：vector 與原始陣列的互操作 1  —  v.data()：取得底層原始指標
// =============================================================================
//
// 【主題資訊 Information】
//   T*       data()       noexcept;   // 非 const vector          (C++11 起)
//   const T* data() const noexcept;   // const vector             (C++11 起)
//
//   標頭檔  : <vector>
//   回傳值  : 指向底層連續儲存區第一個元素的指標。
//             保證 [data(), data() + size()) 是一段合法範圍。
//   複雜度  : O(1)（就是回傳內部那根指標，不做任何搬移或計算）
//   例外    : noexcept，絕不丟例外
//   連續性  : vector 元素「連續存放」的保證由 C++03 的缺陷報告 LWG 69 補進標準，
//             C++11 再把 data() 成員函式與正式措辭一起寫定。
//             （C++98 原文並未明講連續，但當時所有實作事實上都連續。）
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼需要 data()：兩個世界的接縫】
//   C++ 的世界用 vector 管理生命週期；C 的世界（POSIX、OpenGL、SQLite、
//   FFmpeg、各種硬體 SDK）只認得「指標 + 長度」這組介面。
//   如果沒有 data()，你每次呼叫 C 函式都得先把 vector 複製到一塊 new[] 的
//   陣列，用完再 delete[]。那是額外的一次 O(N) 複製 + 一次記憶體配置，
//   而且中間任何 return / throw 都會漏掉 delete[]。
//
//   data() 的意義是：vector 底層本來就是一塊 C 陣列，直接把那根指標給你，
//   零複製、零配置。vector 依然握有所有權，你只是「借看」。
//
// 【2. data() 到底回傳什麼】
//   vector 的內部結構在 libstdc++ 上就是三根指標：
//       _M_start（頭）、_M_finish（size 的尾）、_M_end_of_storage（capacity 的尾）
//   data() 只是回傳 _M_start，編譯器最佳化後通常直接內聯成一次載入指令。
//   所以 data() 不是「產生」一個陣列，而是「揭露」既有的那塊。
//
// 【3. const 正確性是自動的】
//   data() 有兩個重載，靠物件本身的 const 決定選哪個：
//       vector<int>       v;  int*       p = v.data();   // 可寫
//       const vector<int> c;  const int* q = c.data();   // 唯讀
//   非 const vector 呼叫 data() 得到 T*，而 T* 可以隱式轉成 const T*，
//   所以把 v.data() 傳給吃 const int* 的函式完全合法、不需要 const_cast。
//   反過來不行：const T* 不能隱式轉回 T*，這正是我們要的保護。
//
// 【4. data() 與 &v[0] 的差別（本課最常考的一點）】
//   非空 vector 兩者結果相同；空 vector 完全不同：
//       v.data()  → 合法呼叫，回傳某個不可解參考的指標（可能是 nullptr）
//       &v[0]     → v[0] 的前置條件是 size() > 0，空 vector 上就是 UB
//   所以規則很簡單：一律用 data()。詳見本課第 2、3 個檔案。
//
// 【5. 指標與長度必須成對傳遞】
//   data() 只給你起點，長度資訊在 size() 裡。C API 沒有「一個參數同時帶長度」
//   的概念，所以慣例是 f(v.data(), v.size())，兩者一定要來自同一個 vector、
//   同一個時間點。C++20 的 std::span 就是為了把這兩個值綁成一個型別而生。
//
// 【概念補充 Concept Deep Dive】
//
// (A) size_t → int 的隱式窄化
//   size() 回傳 size_type（在本機 64-bit Linux 上是 8 bytes 的 size_t），
//   而大量 C API 的長度參數是 int（4 bytes）。直接傳會被隱式截斷；
//   -Wall -Wextra 預設「不會」警告這件事（要 -Wconversion 才會叫）。
//   正式程式碼請寫 static_cast<int>(v.size())，讓「我知道我在窄化」被記錄下來，
//   並在資料可能超過 INT_MAX 時自己先擋。
//
// (B) 為什麼位址每次執行都不同
//   現代 Linux 預設開啟 ASLR（Address Space Layout Randomization），
//   heap 基底每次執行都會被隨機化，所以 data() 印出來的絕對位址每跑一次都不一樣。
//   本檔因此刻意只印「位址之間的關係」（是否相等、相差幾個 byte），
//   這些是決定性的、可以被驗證的，也才是真正該教的東西。
//
// (C) data() 不會使任何東西失效
//   data() 是 const 操作（不改變 vector 狀態），呼叫它本身永遠安全。
//   危險的從來不是取指標，而是「取完之後又動了 vector」——見本課第 15 個檔案。
//
// 【注意事項 Pay Attention】
//   1. data() 是 C++11 才有的成員函式；C++03 只能寫 &v[0]（且空時是 UB）。
//   2. 拿到的指標不擁有記憶體。絕對不可以對它 delete[] / free()——
//      那會造成 double free，屬於未定義行為。
//   3. 指標的壽命被 vector 綁住：vector 一旦解構、或發生重新配置（reallocation），
//      舊指標就成為懸空指標，再解參考是未定義行為。
//   4. std::vector<bool> 是位元壓縮的特化，它的 data() 在標準中不存在；
//      libstdc++ 上是「已刪除的函式（deleted function）」，寫了會編譯失敗。
//   5. 空 vector 的 data() 回傳值由實作決定，標準只保證它不可被解參考。
//      本機 libstdc++ 實測為 nullptr，但不可以把這當成跨平台保證。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】vector::data()
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. v.data() 和 &v[0] 有什麼差別？該用哪一個？
//     答：vector 非空時兩者回傳同一個位址，完全等價。差別只在空 vector：
//         data() 是合法呼叫，回傳一個不可解參考的指標；
//         &v[0] 會先執行 v[0]，而 operator[] 的前置條件是 size() > 0，
//         空 vector 上就是未定義行為。所以一律用 data()。
//     追問：那 &v[0] 為什麼曾經是主流寫法？
//         → 因為 data() 是 C++11 才加入的成員函式，C++03 時代沒得選。
//
// 🔥 Q2. vector 保證元素連續存放嗎？從哪個標準版本開始？
//     答：保證。C++98 原文沒有明講，是 C++03 透過缺陷報告 LWG 69 補上連續性要求，
//         C++11 再把 data() 與正式措辭一併寫定。
//         正因為連續，vector 才能無縫替換 C 陣列，也才能安全地用在 memcpy、
//         fread/fwrite、socket send/recv 這些吃「指標 + 位元組數」的介面上。
//     追問：deque 也連續嗎？→ 不。deque 是分段陣列，所以它根本沒有 data()。
//
// 🔥 Q3. 為什麼 vector<bool> 不能呼叫 data()？
//     答：vector<bool> 是標準指定的特化，內部把每個 bool 壓成 1 個 bit 存放，
//         底層根本不存在一塊「bool 陣列」可以給出指標，所以標準沒有為它定義
//         data()。本機 libstdc++ 直接把它宣告成 deleted function，
//         誤用會在編譯期就被擋下來（這是好事，總比執行期爆炸好）。
//     追問：那要 bool 的連續緩衝區怎麼辦？
//         → 改用 vector<char> / vector<uint8_t> / std::bitset，不要用 vector<bool>。
//
// ⚠️ 陷阱. 「data() 回傳指標，用完是不是該 delete[] 釋放？」
//     答：絕對不可以。data() 只是「借看」vector 內部那塊記憶體，所有權從頭到尾
//         都在 vector 手上。你 delete[] 一次，vector 解構時還會再釋放一次，
//         這是 double free，屬於未定義行為——可能立刻 abort、可能悄悄污染
//         heap 之後在不相干的地方爆炸，標準不保證任何特定結果。
//     為什麼會錯：腦中把 data() 想成「工廠函式，回傳一塊剛配好的記憶體」，
//         但它其實是「觀察者，回傳既有物件的內部位址」。
//         判斷準則：函式名稱是取得既有狀態（data/c_str/get），就沒有所有權轉移；
//         真正轉移所有權的介面會叫 release()（如 unique_ptr::release）。
// ═══════════════════════════════════════════════════════════════════════════

#include <cstddef>
#include <iostream>
#include <vector>

// -----------------------------------------------------------------------------
// 模擬一個 C 風格的函式庫函數（唯讀）：只認得 const int* + 長度
// -----------------------------------------------------------------------------
void c_library_function(const int* arr, int size) {
    std::cout << "C 函數收到：";
    for (int i = 0; i < size; ++i) {
        std::cout << (i ? " " : "") << arr[i];
    }
    std::cout << "\n";
}

// -----------------------------------------------------------------------------
// 【日常實務範例】把感測器讀數餵給既有的 C 語言 DSP 函式庫
//   情境：溫度感測器以 vector<int> 累積毫度數（milli-degree）讀數，
//         但公司十年前寫的 dsp_moving_average() 是 C 函式，只吃 int* + 長度。
//   為何用 data()：不必為了呼叫舊函式而把整個 vector 複製成 new int[N]，
//                  零複製直接把底層陣列交出去，回傳值再交給 C++ 端使用。
// -----------------------------------------------------------------------------
int dsp_moving_average(const int* samples, int n, int window) {  // 假裝這是 C 函式庫
    if (n <= 0 || window <= 0 || window > n) return 0;
    long long sum = 0;
    for (int i = n - window; i < n; ++i) sum += samples[i];
    return static_cast<int>(sum / window);
}

int reportLatestAverage(const std::vector<int>& milliDegrees, int window) {
    // size_t → int 是窄化，正式程式碼要顯式 static_cast 並自行把關上限
    return dsp_moving_average(milliDegrees.data(),
                              static_cast<int>(milliDegrees.size()),
                              window);
}

int main() {
    std::vector<int> v = {10, 20, 30, 40, 50};

    // -------------------------------------------------------------------------
    std::cout << "=== data() 的基本性質 ===\n";
    // -------------------------------------------------------------------------
    int* ptr = v.data();                 // 非 const vector → int*
    const int* cptr = v.data();          // int* 可隱式轉成 const int*（合法）

    // 只印「關係」不印絕對位址：位址受 ASLR 影響，每次執行都不同，無法驗證
    std::cout << "data() 是否等於 &v[0]      : " << (ptr == &v[0] ? "是" : "否") << "\n";
    std::cout << "data() 是否等於 &*v.begin(): " << (ptr == &*v.begin() ? "是" : "否") << "\n";
    std::cout << "const 版本是否指向同一處   : " << (cptr == ptr ? "是" : "否") << "\n";
    std::cout << "ptr[0] = " << ptr[0] << ", ptr[4] = " << ptr[4] << "\n";

    // -------------------------------------------------------------------------
    std::cout << "\n=== 連續性：位址是等距排列的 ===\n";
    // -------------------------------------------------------------------------
    // 元素連續，所以 &v[i] - &v[0] 恰好等於 i，位元組差恰好是 i * sizeof(int)
    std::cout << "sizeof(int) = " << sizeof(int) << " bytes（實作定義，本機 x86-64 為 4）\n";
    for (std::size_t i = 0; i < v.size(); ++i) {
        std::ptrdiff_t elemGap = &v[i] - ptr;                       // 以元素為單位
        std::ptrdiff_t byteGap = reinterpret_cast<const char*>(&v[i])
                               - reinterpret_cast<const char*>(ptr);  // 以 byte 為單位
        std::cout << "  v[" << i << "]：元素距離 " << elemGap
                  << "，位元組距離 " << byteGap << "\n";
    }

    // -------------------------------------------------------------------------
    std::cout << "\n=== 透過指標讀寫，改的就是 vector 本體 ===\n";
    // -------------------------------------------------------------------------
    ptr[1] = 200;   // 不是改副本，是直接改 vector 的第 2 個元素
    std::cout << "ptr[1] = 200 之後，v[1] = " << v[1] << "\n";
    std::cout << "v.size() 完全沒變 = " << v.size() << "（寫值不會改變長度）\n";

    // -------------------------------------------------------------------------
    std::cout << "\n=== 傳給 C 函式庫 ===\n";
    // -------------------------------------------------------------------------
    c_library_function(v.data(), static_cast<int>(v.size()));

    // const vector：data() 自動變成 const int*，寫入會編譯失敗
    const std::vector<int> readOnly = {7, 8, 9};
    const int* roPtr = readOnly.data();
    // roPtr[0] = 99;   // 編譯錯誤：assignment of read-only location
    std::cout << "const vector 的 data()[0] = " << roPtr[0] << "（唯讀）\n";

    // -------------------------------------------------------------------------
    std::cout << "\n=== 日常實務：感測器讀數交給舊 C DSP 函式 ===\n";
    // -------------------------------------------------------------------------
    std::vector<int> milliDegrees = {23100, 23250, 23400, 23380, 23500, 23620};
    std::cout << "最近 3 筆平均 = " << reportLatestAverage(milliDegrees, 3)
              << " m°C\n";
    std::cout << "全部 6 筆平均 = " << reportLatestAverage(milliDegrees, 6)
              << " m°C\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第\ 19\ 課：vector\ 與原始陣列的互操作1.cpp -o interop1

// === 預期輸出 ===
// === data() 的基本性質 ===
// data() 是否等於 &v[0]      : 是
// data() 是否等於 &*v.begin(): 是
// const 版本是否指向同一處   : 是
// ptr[0] = 10, ptr[4] = 50
//
// === 連續性：位址是等距排列的 ===
// sizeof(int) = 4 bytes（實作定義，本機 x86-64 為 4）
//   v[0]：元素距離 0，位元組距離 0
//   v[1]：元素距離 1，位元組距離 4
//   v[2]：元素距離 2，位元組距離 8
//   v[3]：元素距離 3，位元組距離 12
//   v[4]：元素距離 4，位元組距離 16
//
// === 透過指標讀寫，改的就是 vector 本體 ===
// ptr[1] = 200 之後，v[1] = 200
// v.size() 完全沒變 = 5（寫值不會改變長度）
//
// === 傳給 C 函式庫 ===
// C 函數收到：10 200 30 40 50
// const vector 的 data()[0] = 7（唯讀）
//
// === 日常實務：感測器讀數交給舊 C DSP 函式 ===
// 最近 3 筆平均 = 23500 m°C
// 全部 6 筆平均 = 23375 m°C
