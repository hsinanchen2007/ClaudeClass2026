// =============================================================================
//  第 9 課 1  —  vector 的連續記憶體保證與 data()
// =============================================================================
//
// 【主題資訊 Information】
//   T* data() noexcept;  const T* data() const noexcept;   [C++11 起有 data()]
//   標頭檔  : <vector>
//   複雜度  : O(1)
//   標準保證: [vector.data] 明文規定「元素連續儲存」——對任何 0 <= n < size()，
//             &v[n] == v.data() + n 恆成立。這不是實作巧合，是標準要求。
//             （C++03 起就已用 DR 69 把這個保證寫進標準。）
//
// 【詳細解釋 Explanation】
//
// 【1. 「連續」為什麼是 vector 最重要的性質】
//   連續帶來三件別的容器做不到的事：
//     (a) O(1) 隨機存取：v[i] 就是一次位址計算 base + i*sizeof(T)，
//         不需要走訪節點。
//     (b) 快取友善：現代 CPU 一次抓一整條 cache line（本機為 64 bytes，
//         可容納 16 個 int）。連續走訪 vector 幾乎每次都命中；
//         list 的節點散落各處，幾乎每次都 miss。這就是為什麼即使
//         list 的中間插入是理論 O(1)，實測上 vector 常常還是贏。
//     (c) 可直接餵給 C API：read()、memcpy()、OpenGL、繪圖或
//         硬體驅動的介面都收 T*，vector 可以無縫接軌。
//
// 【2. data() vs &v[0]：差在空容器】
//   兩者對非空 vector 完全等價。但當 v 為空時：
//     * v.data()  合法，回傳值未指定（可能是 nullptr），不可解參考但可安全呼叫。
//     * &v[0]     是 UB——v[0] 對空容器是越界存取，取位址前就已經錯了。
//   所以一律用 data()，不要用 &v[0]。
//
// 【3. 這個保證的邊界：vector<bool> 是例外】
//   std::vector<bool> 是特化版本，內部把每個 bool 壓成一個 bit。
//   它「沒有」連續的 bool 陣列，因此不提供 data()，也不能餵給 C API。
//   需要真正的 bool 陣列請用 std::vector<char> 或 std::array<bool, N>。
//
// 【概念補充 Concept Deep Dive】
//   vector 本身是一個「三指標」的小物件（本機 sizeof(std::vector<int>) 為
//   24 bytes = 3 × 8，屬實作定義）：_begin、_end、_cap。
//   元素資料在堆積上，vector 物件本身可能在堆疊上。所以：
//       sizeof(v)      永遠是 24，與元素數量無關
//       v.size()       = _end - _begin
//       v.capacity()   = _cap - _begin
//   本檔以「相對 data() 的 byte 位移」呈現連續性，而不是印出原始位址——
//   因為原始位址每次執行都不同（ASLR，本機實測兩次執行結果不同），
//   把它寫進預期輸出是沒有意義的。位移則是確定性的證據，
//   而且更能直接看出「每個 int 相差 4 bytes」。
//
// 【注意事項 Pay Attention】
// 1. data() 回傳的指標會在「重新配置」後失效：push_back 造成擴容、
//    resize、reserve、insert、shrink_to_fit 之後，先前存下的指標／迭代器／
//    參考全部懸空，再使用就是 UB。存指標前務必確認之後不會改動容量。
// 2. 空 vector 用 &v[0] 是 UB；用 data() 才安全（但仍不可解參考）。
// 3. std::vector<bool> 沒有 data()，不適用本檔所有結論。
// 4. 指標算術只在 [data(), data() + size()] 範圍內合法；
//    data() + size() 可以計算（尾後指標）但不可解參考。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】vector 的連續記憶體
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. vector 元素連續是標準保證，還是各家實作剛好都這樣做？
//     答：是標準明文保證的。標準規定對 0 <= n < size() 恆有
//         &v[n] == v.data() + n。正因為有這個保證，vector 才能安全地
//         把 data() 交給 C API。
//     追問：那 std::vector<bool> 呢？→ 它是特化版本，用 bit 打包，
//         不保證連續、也沒有 data()。需要真正的位元組陣列要改用
//         std::vector<char>。
//
// 🔥 Q2. 為什麼實測時 vector 走訪常常比 list 快，即使 list 的插入是 O(1)？
//     答：因為 cache locality。vector 連續存放，一次 cache line（本機
//         64 bytes = 16 個 int）就載入多個元素，走訪幾乎全命中；
//         list 節點各自 new 出來、散落在堆積各處，幾乎每步都 cache miss，
//         而一次 miss 的代價可達上百個 cycle，遠大於省下的搬移成本。
//     追問：那什麼時候 list 才真的贏？→ 元素非常大、搬移成本高，
//         且需要在已知位置頻繁插入刪除、又不需要隨機存取時。
//
// ⚠️ 陷阱. int* p = v.data(); v.push_back(x); 然後用 p——為什麼危險？
//     答：push_back 若造成擴容，vector 會配置新記憶體、搬移元素、
//         釋放舊區塊。此時 p 指向已釋放的記憶體，是懸空指標，
//         再使用屬於 UB。注意這與「p 一定會壞掉」不同——
//         若當時 capacity 還夠，就不會重新配置，p 依然有效。
//     為什麼會錯：正因為「不一定會壞」，這個 bug 極難重現。
//         開發時容量剛好夠用、測試全過；上線後資料量一大觸發擴容，
//         才開始隨機崩潰或讀到垃圾。要靠推理（會不會擴容？）而不是
//         靠測試來判斷安全性。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】從缺。
//   理由：本檔主題是「vector 的記憶體佈局與 data() 指標」，這是語言／
//   標準庫層面的性質。LeetCode 評測環境不涉及 C API 互通或原始指標操作，
//   沒有任何一題的核心是「元素是否連續」。強加一題會把焦點從記憶體模型
//   拉到演算法上，故從缺，改以實務範例呈現 data() 真正無可取代的用途。

#include <vector>
#include <iostream>
#include <cstring>
#include <cstddef>

// -----------------------------------------------------------------------------
// 【日常實務範例】把 vector 當作緩衝區餵給 C API
//   場景：讀取二進位封包／影像資料時，底層 API（read()、memcpy()、
//         第三方 C 函式庫）幾乎都收「指標 + 長度」。
//   為什麼用 data()：vector 保證連續，data() 直接就是那個指標，
//     於是我們同時得到「C 相容的緩衝區」與「自動釋放的 RAII 生命週期」，
//     不需要手動 malloc/free，例外發生時也不會漏記憶體。
//   注意：呼叫 C API 前必須先 resize()（不是 reserve()）——
//     reserve 只配空間但 size() 仍是 0，之後讀 v[i] 就是越界。
// -----------------------------------------------------------------------------

// 模擬一個典型的 C 風格 API：把資料填進呼叫端提供的緩衝區，回傳寫入位元組數
static size_t c_style_fill(unsigned char* buf, size_t buf_len) {
    const unsigned char payload[] = {0xDE, 0xAD, 0xBE, 0xEF, 0x42};
    size_t n = sizeof(payload);
    if (n > buf_len) n = buf_len;      // 永遠不要寫超過呼叫端給的長度
    std::memcpy(buf, payload, n);
    return n;
}

int main() {
    std::vector<int> v = {10, 20, 30, 40, 50};

    // ---- 連續性的「確定性」證據：相對 data() 的 byte 位移 ----
    // 不印原始位址：位址受 ASLR 影響，每次執行都不同（本機實測確認），
    // 寫進預期輸出毫無意義。位移才是可重現、也更能說明問題的證據。
    std::cout << "=== 連續記憶體：相對 data() 的 byte 位移 ===" << std::endl;
    const int* base = v.data();
    for (size_t i = 0; i < v.size(); ++i) {
        std::cout << "v[" << i << "] 位移 = "
                  << reinterpret_cast<const char*>(&v[i]) -
                     reinterpret_cast<const char*>(base)
                  << " bytes, 值 = " << v[i] << std::endl;
    }

    // 標準保證 &v[n] == v.data() + n，直接驗證它
    bool contiguous = true;
    for (size_t i = 0; i < v.size(); ++i) {
        if (&v[i] != base + i) contiguous = false;
    }
    std::cout << "所有 &v[n] == data() + n ? " << std::boolalpha
              << contiguous << std::noboolalpha << std::endl;
    std::cout << "sizeof(int) = " << sizeof(int)
              << ", sizeof(std::vector<int>) = " << sizeof(std::vector<int>)
              << " bytes（實作定義，本機實測）" << std::endl;

    // 原始位址只供人眼觀察，送到 stderr，讓 stdout 保持逐位元組穩定
    std::cerr << "[stderr] v.data() 原始位址（每次執行都不同）: "
              << static_cast<const void*>(base) << std::endl;

    // 因為是連續記憶體，可以取得原始指標
    std::cout << "\n=== 用 data() 取得原始指標 ===" << std::endl;
    int* ptr = v.data();  // 或 &v[0]（但 &v[0] 對空 vector 是 UB）
    std::cout << "第三個元素: " << ptr[2] << std::endl;  // 輸出 30
    ptr[0] = 100;                                         // 透過指標寫回
    std::cout << "透過指標改寫後 v[0] = " << v[0] << std::endl;

    // ---- 日常實務：當作 C API 的緩衝區 ----
    std::cout << "\n=== 日常實務: vector 當 C API 緩衝區 ===" << std::endl;
    std::vector<unsigned char> buf;
    buf.resize(16);      // 必須 resize，不能只 reserve
    size_t written = c_style_fill(buf.data(), buf.size());
    buf.resize(written); // 依實際寫入量縮回真實長度

    std::cout << "C API 寫入 " << written << " bytes: ";
    for (unsigned char b : buf) {
        // 以十六進位印出；轉成 unsigned 才不會被當成字元輸出
        std::cout << std::hex << std::uppercase
                  << (b < 16 ? "0" : "") << static_cast<unsigned>(b) << " ";
    }
    std::cout << std::dec << std::nouppercase << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第 9 課：vector 的內部結構與記憶體配置1.cpp -o vector_contiguous

// 【但書】
//   1. 原始位址（v.data() 的十六進位值）刻意輸出到 stderr 而非 stdout，
//      因為它受 ASLR 影響、每次執行都不同（本機實測確認）。
//      下方預期輸出只涵蓋 stdout，是逐位元組穩定的。
//   2. sizeof(std::vector<int>) = 24 bytes 是實作定義（本機 g++ 15.2 /
//      libstdc++ 的三指標佈局），libc++ 與 MSVC 可能不同。

// === 預期輸出 ===
// === 連續記憶體：相對 data() 的 byte 位移 ===
// v[0] 位移 = 0 bytes, 值 = 10
// v[1] 位移 = 4 bytes, 值 = 20
// v[2] 位移 = 8 bytes, 值 = 30
// v[3] 位移 = 12 bytes, 值 = 40
// v[4] 位移 = 16 bytes, 值 = 50
// 所有 &v[n] == data() + n ? true
// sizeof(int) = 4, sizeof(std::vector<int>) = 24 bytes（實作定義，本機實測）
//
// === 用 data() 取得原始指標 ===
// 第三個元素: 30
// 透過指標改寫後 v[0] = 100
//
// === 日常實務: vector 當 C API 緩衝區 ===
// C API 寫入 5 bytes: DE AD BE EF 42 
