// =============================================================================
//  第 19 課-11：memcpy / memset / qsort —— 把 C 的工具用在 vector 上
// =============================================================================
//
// 【主題資訊 Information】
//   void* memcpy(void* dst, const void* src, size_t n);        // <cstring>
//   void* memset(void* s, int c, size_t n);                     // <cstring>
//   void  qsort(void* base, size_t nmemb, size_t size,
//               int (*compar)(const void*, const void*));       // <cstdlib>
//   前提：vector 的元素連續（C++11 明文保證），故 v.data() 可直接餵給它們
//   安全前提：只有 trivially copyable 型別才可以用 memcpy / memset
//   標準版本：連續性保證自 C++11；data() 自 C++11
//   標頭檔：<cstring>、<cstdlib>、<vector>、<type_traits>
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼這些 C 函式能用在 vector 上】
//   因為 C++11 起標準明文保證 vector 的元素在記憶體中連續存放，
//   而且 data() 回傳的指標讓 [data(), data()+size()) 是一段合法範圍。
//   對 C 函式而言，這和一個真正的陣列毫無區別。
//   這是 vector 相對於 list/deque/map 的最大優勢之一——
//   它是唯一能無縫接上整個 C 生態系的動態容器。
//
// 【2. memcpy 的前提：trivially copyable】
//   memcpy 做的是「原封不動地複製位元組」。
//   對 int、double、POD struct 完全正確；
//   但對 std::string 就是災難——
//   string 內部有一根指向堆積的指標，
//   memcpy 之後兩個 string 會指向同一塊記憶體，
//   解構時會被 delete 兩次（double free）。
//   判斷方式：std::is_trivially_copyable_v<T>。
//   非 trivially copyable 的型別一律用 std::copy 或區間建構。
//
// 【3. memset 的兩個陷阱】
//   (a) 第二個參數是「一個 byte 的值」，不是「元素的值」。
//       memset(v.data(), 1, n * sizeof(int)) 不會把每個 int 設成 1，
//       而是把每個 byte 設成 0x01，於是每個 int 變成 0x01010101 = 16843009。
//       唯一「湊巧正確」的值是 0——因為全零的位元樣式對整數就是 0。
//   (b) 對非 trivially copyable 型別使用是 undefined behavior。
//   結論：要填值請用 std::fill 或 v.assign(n, value)，
//   它們型別安全、意圖清楚，而且對 int 這類型別
//   編譯器同樣會最佳化成 memset。
//
// 【4. qsort vs std::sort：為什麼後者反而更快】
//   直覺上 C 的 qsort 應該比較「輕」，實測往往慢 2–3 倍，原因是：
//     ▸ qsort 透過「函式指標」呼叫比較函式，無法內聯（inline）；
//       std::sort 接受的是函式物件或 lambda，比較邏輯可以完全內聯。
//     ▸ qsort 的介面是 void*，每次比較都要解參考與轉型；
//       std::sort 是型別安全的模板，編譯器知道確切型別。
//     ▸ std::sort 的複雜度有標準保證（O(n log n) 最壞，
//       C++11 起要求；實作通常是 introsort = quicksort + heapsort + insertion sort），
//       而 qsort 的標準只要求「平均 O(n log n)」。
//   結論：在 C++ 中沒有理由用 qsort，除非你正在維護 C 介面。
//
// 【5. 一個 qsort 比較函式的經典 bug】
//   int cmp(const void* a, const void* b) {
//       return *(const int*)a - *(const int*)b;     // ← 相減可能溢位！
//   }
//   當兩數相差超過 INT_MAX（例如 INT_MIN 與 INT_MAX 比較）時，
//   相減會發生有號整數溢位——那是 undefined behavior，排序結果可能完全錯亂。
//   正確寫法是用比較而非相減：
//       return (x > y) - (x < y);
//   本檔的 compare_int 已採用這個安全寫法。
//
// 【概念補充 Concept Deep Dive】
//   ▸ 為什麼 memcpy 對 vector<string> 是 double free
//     std::string 內部持有一根指向字元緩衝區的指標。
//     memcpy 只複製那根指標的位元，不會複製它指向的內容。
//     於是來源與目的兩個 string 指向同一塊堆積記憶體，
//     兩者解構時各 delete 一次 → double free，通常直接崩潰。
//   ▸ std::copy 為什麼對 POD 一樣快
//     libstdc++ 的 std::copy 對 trivially copyable 型別
//     有特化路徑，最終呼叫 memmove。
//     所以「用 std::copy 比較安全」不需要付出效能代價——
//     它在該用 memmove 的時候就會用 memmove。
//   ▸ memcpy 與 memmove 的差別
//     memcpy 要求來源與目的「不重疊」，重疊時是 undefined behavior；
//     memmove 允許重疊，會正確處理方向。
//     在同一個 vector 內搬移資料時務必用 memmove（或 std::copy_backward）。
//
// 【注意事項 Pay Attention】
//   1. memcpy / memset 只能用於 trivially copyable 型別；
//      對 std::string、含虛擬函式的類別使用是 undefined behavior。
//   2. memset 的第二個參數是「byte 值」，只有填 0 時才符合直覺。
//   3. 來源與目的重疊時要用 memmove，不是 memcpy。
//   4. 大小一律用 n * sizeof(T)，別忘了乘 sizeof。
//   5. 在 C++ 中請優先用 std::copy / std::fill / std::sort——
//      型別安全、意圖清楚，而且對 POD 型別效能相同或更好。
//   6. qsort 的比較函式不要用「相減」，可能有號溢位（UB）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】vector 與 C 記憶體函式
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼可以對 vector<int> 用 memcpy，卻不能對 vector<std::string> 用？
//     答：memcpy 做的是逐位元組複製，只對 trivially copyable 型別正確。
//         std::string 內部持有一根指向堆積的指標，
//         memcpy 只複製那根指標的位元、不複製它指向的內容，
//         於是兩個 string 指向同一塊記憶體，解構時各 delete 一次
//         → double free。判斷依據是 std::is_trivially_copyable_v<T>。
//     追問：那該用什麼？效能會不會比較差？
//         → 用 std::copy。而且不會比較慢——libstdc++ 對
//           trivially copyable 型別有特化路徑，最終同樣呼叫 memmove。
//
// 🔥 Q2. memset(v.data(), 1, v.size() * sizeof(int)); 之後 v[0] 是多少？
//     答：不是 1，而是 16843009（0x01010101）。
//         memset 的第二個參數是「要填進每一個 byte 的值」，
//         不是「要填進每個元素的值」。
//         每個 int 有 4 個 byte，全部填 0x01 就得到 0x01010101。
//         唯一湊巧正確的填充值是 0，因為全零位元樣式對整數就是 0。
//     追問：那要把 vector<int> 每個元素設成 1 該怎麼寫？
//         → std::fill(v.begin(), v.end(), 1); 或 v.assign(n, 1);
//           型別安全、意圖清楚，而且編譯器一樣會最佳化。
//
// ⚠️ 陷阱. qsort 的比較函式寫成
//          int cmp(const void* a, const void* b) {
//              return *(const int*)a - *(const int*)b;
//          }
//          為什麼在某些輸入下排序結果會完全錯亂？
//     答：相減可能發生有號整數溢位。
//         例如 a = INT_MIN、b = INT_MAX 時，兩者相減的數學結果
//         遠超出 int 的表示範圍，這是 undefined behavior，
//         實際得到的符號可能與正確答案相反，導致排序邏輯崩壞。
//     為什麼會錯：以為「回傳負/零/正」最省事的方式就是相減。
//         在數值範圍小時確實不會出事，所以這個 bug 極難在測試中發現。
//         正確寫法是用比較而非相減：return (x > y) - (x < y);
//         這也是 std::sort 要求傳入「嚴格弱序」述詞而非「差值」的原因之一。
// ═══════════════════════════════════════════════════════════════════════════

#include <algorithm>   // std::sort、std::copy、std::fill（對比用）
#include <cstdlib>     // qsort
#include <cstring>     // memcpy, memset, memmove
#include <iostream>
#include <string>
#include <type_traits> // std::is_trivially_copyable_v
#include <vector>

// qsort 的比較函式
// 注意：刻意「不」寫成 (*a - *b)——那樣在極端值下會有號溢位（UB）
int compare_int(const void* a, const void* b) {
    int x = *static_cast<const int*>(a);
    int y = *static_cast<const int*>(b);
    return (x > y) - (x < y);      // 安全：只做比較，不做減法
}

// -----------------------------------------------------------------------------
// 【日常實務範例】影像像素緩衝區：與 C 影像函式庫互通
//   情境：讀進一張灰階影像（寬 × 高 個 byte），要做三件事——
//         (1) 清空成黑色；(2) 複製一份做備份；(3) 交給 C 函式庫做直方圖統計。
//   為什麼用本主題：影像處理是 memcpy/memset 最典型的正當用途——
//         資料量大、元素是 POD、而且幾乎一定要與 C 函式庫互通。
//         這裡同時示範「memset 填 0 沒問題、填其他值就要用 std::fill」
//         這個關鍵分界。
// -----------------------------------------------------------------------------
using Pixel = unsigned char;

// 模擬 C 影像函式庫：統計每個灰階值出現次數
extern "C" void c_histogram(const Pixel* data, std::size_t n, unsigned* histOut256) {
    for (std::size_t i = 0; i < 256; ++i) histOut256[i] = 0;
    for (std::size_t i = 0; i < n; ++i) ++histOut256[data[i]];
}

class GrayImage {
    std::vector<Pixel> buf_;
    std::size_t        w_;
    std::size_t        h_;
public:
    GrayImage(std::size_t w, std::size_t h) : buf_(w * h, 0), w_(w), h_(h) {}

    // 清成黑色：填 0 是 memset 唯一符合直覺的用法
    void clearToBlack() {
        std::memset(buf_.data(), 0, buf_.size() * sizeof(Pixel));
    }
    // 填成某個灰階值：不能用 memset 的直覺寫法思考，改用 std::fill 最清楚
    // （對 1-byte 的 Pixel 來說 memset 湊巧也對，但換成 int 就會出錯，
    //   所以養成用 std::fill 的習慣）
    void fillGray(Pixel value) {
        std::fill(buf_.begin(), buf_.end(), value);
    }
    void setPixel(std::size_t x, std::size_t y, Pixel v) { buf_[y * w_ + x] = v; }

    // 備份：Pixel 是 trivially copyable，memcpy 合法且最快
    std::vector<Pixel> snapshot() const {
        std::vector<Pixel> copy(buf_.size());
        std::memcpy(copy.data(), buf_.data(), buf_.size() * sizeof(Pixel));
        return copy;
    }
    // 直接交給 C 函式庫，零複製
    void histogram(unsigned* out256) const {
        c_histogram(buf_.data(), buf_.size(), out256);
    }
    std::size_t pixels() const { return buf_.size(); }
    std::size_t height() const { return h_; }
};

int main() {
    std::cout << std::boolalpha;

    std::cout << "=== 一、memcpy：在 vector 之間複製原始記憶體 ===" << std::endl;
    std::vector<int> src = {50, 30, 10, 40, 20};
    std::vector<int> dst(src.size());

    std::memcpy(dst.data(), src.data(), src.size() * sizeof(int));

    std::cout << "memcpy 結果：";
    for (int x : dst) std::cout << x << " ";
    std::cout << std::endl;

    std::cout << "\n=== 二、memset 的陷阱：第二個參數是「byte 值」===" << std::endl;
    std::vector<int> zeros(5);
    std::memset(zeros.data(), 0, zeros.size() * sizeof(int));
    std::cout << "memset 填 0（正確）：";
    for (int x : zeros) std::cout << x << " ";
    std::cout << std::endl;

    std::vector<int> ones(5);
    std::memset(ones.data(), 1, ones.size() * sizeof(int));
    std::cout << "memset 填 1（不是你要的！）：";
    for (int x : ones) std::cout << x << " ";
    std::cout << "  ← 每個 byte 都是 0x01，組成 0x01010101" << std::endl;

    std::vector<int> properOnes(5);
    std::fill(properOnes.begin(), properOnes.end(), 1);
    std::cout << "std::fill 填 1（正確做法）：";
    for (int x : properOnes) std::cout << x << " ";
    std::cout << std::endl;

    std::cout << "\n=== 三、什麼型別可以 memcpy ===" << std::endl;
    std::cout << "is_trivially_copyable<int>         : "
              << std::is_trivially_copyable_v<int> << std::endl;
    std::cout << "is_trivially_copyable<double>      : "
              << std::is_trivially_copyable_v<double> << std::endl;
    std::cout << "is_trivially_copyable<std::string> : "
              << std::is_trivially_copyable_v<std::string>
              << "  ← 對它 memcpy 會 double free！" << std::endl;

    std::cout << "\n=== 四、非 POD 型別請用 std::copy ===" << std::endl;
    std::vector<std::string> words_src = {"hello", "world", "cpp"};
    std::vector<std::string> words_dst(words_src.size());
    std::copy(words_src.begin(), words_src.end(), words_dst.begin());
    std::cout << "std::copy 結果：";
    for (const std::string& s : words_dst) std::cout << s << " ";
    std::cout << std::endl;
    std::cout << "（對 POD 型別 std::copy 內部同樣會走 memmove，不會比較慢）" << std::endl;

    std::cout << "\n=== 五、qsort：C 的排序函式 ===" << std::endl;
    std::vector<int> to_sort = {50, 30, 10, 40, 20};
    std::qsort(to_sort.data(), to_sort.size(), sizeof(int), compare_int);
    std::cout << "qsort 結果：";
    for (int x : to_sort) std::cout << x << " ";
    std::cout << std::endl;

    std::vector<int> to_sort2 = {50, 30, 10, 40, 20};
    std::sort(to_sort2.begin(), to_sort2.end());
    std::cout << "std::sort 結果：";
    for (int x : to_sort2) std::cout << x << " ";
    std::cout << "  ← 比較邏輯可內聯，通常比 qsort 快 2-3 倍" << std::endl;

    std::cout << "\n=== 六、重疊區間要用 memmove，不是 memcpy ===" << std::endl;
    std::vector<int> ov = {1, 2, 3, 4, 5};
    // 把 [0,3) 往後搬到 [1,4) —— 來源與目的重疊
    std::memmove(ov.data() + 1, ov.data(), 3 * sizeof(int));
    std::cout << "memmove 重疊搬移後：";
    for (int x : ov) std::cout << x << " ";
    std::cout << "（用 memcpy 在此是 UB）" << std::endl;

    std::cout << "\n=== 七、日常實務：灰階影像緩衝區 ===" << std::endl;
    GrayImage img(64, 48);
    std::cout << "影像大小：" << img.pixels() << " pixels" << std::endl;

    img.fillGray(128);                       // 填中灰
    img.setPixel(10, 10, 255);               // 一個白點
    img.setPixel(20, 20, 255);

    std::vector<Pixel> backup = img.snapshot();   // memcpy 備份
    std::cout << "備份大小：" << backup.size() << " bytes" << std::endl;

    unsigned hist[256];
    img.histogram(hist);                     // 直接把 data() 交給 C 函式庫
    std::cout << "直方圖：灰階 128 有 " << hist[128] << " 個像素，"
              << "灰階 255 有 " << hist[255] << " 個像素" << std::endl;

    img.clearToBlack();                      // memset 填 0（唯一直覺正確的用法）
    img.histogram(hist);
    std::cout << "清空後：灰階 0 有 " << hist[0] << " 個像素" << std::endl;
    std::cout << "備份未受影響（獨立記憶體）：backup[10*64+10] = "
              << static_cast<int>(backup[10 * 64 + 10]) << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 19 課：vector 與原始陣列的互操作11.cpp" -o c_interop
//
// 【關於下方預期輸出的但書】
//   ▸ memset 填 1 得到的 16843009（0x01010101）依賴
//     sizeof(int) == 4，本機（x86-64 / GCC 15.2）成立，屬實作定義。
//     在 sizeof(int) 為 2 的平台上會是 257（0x0101）。
//   ▸ 「std::sort 比 qsort 快 2-3 倍」是一般性的經驗結論，
//     本檔並未在此執行計時量測，該行僅為說明文字。
//     實際差距取決於資料量、型別與最佳化等級。
//
// 【本檔未附 LeetCode 範例的理由】
//   本檔的主題是「與 C 記憶體函式互通的安全前提」——
//   trivially copyable、byte 語意、重疊區間、函式指標無法內聯。
//   LeetCode 的執行環境不涉及 C 函式庫互通，
//   也不會考 memcpy 對非 POD 型別的 double free，
//   硬套一題無法呈現重點；因此改以灰階影像緩衝區
//   這個 memcpy/memset 最典型的正當用途呈現。

// === 預期輸出 ===
// === 一、memcpy：在 vector 之間複製原始記憶體 ===
// memcpy 結果：50 30 10 40 20
//
// === 二、memset 的陷阱：第二個參數是「byte 值」===
// memset 填 0（正確）：0 0 0 0 0
// memset 填 1（不是你要的！）：16843009 16843009 16843009 16843009 16843009   ← 每個 byte 都是 0x01，組成 0x01010101
// std::fill 填 1（正確做法）：1 1 1 1 1
//
// === 三、什麼型別可以 memcpy ===
// is_trivially_copyable<int>         : true
// is_trivially_copyable<double>      : true
// is_trivially_copyable<std::string> : false  ← 對它 memcpy 會 double free！
//
// === 四、非 POD 型別請用 std::copy ===
// std::copy 結果：hello world cpp
// （對 POD 型別 std::copy 內部同樣會走 memmove，不會比較慢）
//
// === 五、qsort：C 的排序函式 ===
// qsort 結果：10 20 30 40 50
// std::sort 結果：10 20 30 40 50   ← 比較邏輯可內聯，通常比 qsort 快 2-3 倍
//
// === 六、重疊區間要用 memmove，不是 memcpy ===
// memmove 重疊搬移後：1 1 2 3 5 （用 memcpy 在此是 UB）
//
// === 七、日常實務：灰階影像緩衝區 ===
// 影像大小：3072 pixels
// 備份大小：3072 bytes
// 直方圖：灰階 128 有 3070 個像素，灰階 255 有 2 個像素
// 清空後：灰階 0 有 3072 個像素
// 備份未受影響（獨立記憶體）：backup[10*64+10] = 255
