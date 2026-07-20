// =============================================================================
//  第 18 課：vectorbool 的特殊性與陷阱2.cpp  —  bit packing：省下 8 倍記憶體的動機
// =============================================================================
//
// 【主題資訊 Information】
//   size_type vector<bool>::capacity() const noexcept;   // 回傳「可容納幾個 bool」
//   size_type vector<bool>::size()     const noexcept;
//   void      vector<bool>::reserve(size_type n);
//   標頭檔  : <vector>
//   標準版本: C++98（特化本身）；capacity/reserve 語意與主模板相同
//   複雜度  : size()/capacity() 均為 O(1)
//
//   注意：capacity() 回傳的單位是「bool 的個數」，不是 byte 數。
//         要估算實際佔用的 payload bytes 必須自己除以 8。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼標準要為 bool 做這個特化】
//   一個 bool 在語意上只需要 1 個 bit，但 C++ 規定 sizeof(bool) >= 1，而且最小
//   可定址單位就是 byte。所以 bool arr[1000] 一定佔滿 1000 bytes——有 7/8 的空間
//   純粹在浪費。
//
//   1998 年制定 C++98 時，記憶體遠比現在珍貴（一台機器常只有 32~64 MB）。委員會
//   認為「布林陣列」是夠常見的需求，值得為它做一個位元壓縮的特化，於是把
//   vector<bool> 直接改寫成 bitset 式的實作。動機本身是合理的，出問題的是「把它
//   叫做 vector」這個決定（第 14 檔會完整討論）。
//
// 【2. capacity() 的單位陷阱】
//   本檔原始碼寫的是 vb.capacity() / 8，這是把「bool 個數」換算成 bytes。
//   很多人誤以為 capacity() 回傳的是 byte 數而漏掉這個除法，於是把記憶體用量
//   高估 8 倍。正確心智模型是：
//       capacity()      → 不重新配置的前提下，最多能放幾個 bool
//       capacity() / 8  → 這些 bool 實際佔用的 payload 大約幾 bytes
//
// 【3. 為什麼實測 capacity 是 1024 而不是 1000】
//   本機實測 vector<bool>(1000).capacity() == 1024。
//   因為配置是以 word 為單位進行的：libstdc++ 的 word 型別是 unsigned long
//   （本機 64 bits，此為【實作定義】）。1000 bits 需要 ceil(1000/64) = 16 個 word，
//   16 * 64 = 1024 bits，所以 capacity 被無條件進位到 1024。
//   同理，第一次 push_back 之後 capacity 直接是 64（一整個 word），而不是 1。
//
// 【4. 別忘了容器物件本身也佔空間】
//   本機實測（皆為【實作定義】）：
//       sizeof(std::vector<char>) = 24 bytes   （3 個指標）
//       sizeof(std::vector<bool>) = 40 bytes   （2 個 bit iterator 各 16 + 1 指標 8）
//   bit iterator 必須同時記住「哪個 word」和「word 內第幾個 bit」，所以比普通
//   指標胖。資料量小的時候，這 16 bytes 的差距反而讓 vector<bool> 更耗記憶體；
//   要到幾千個元素以上，bit packing 的優勢才真正壓過來。
//
// 【5. 省記憶體 = 省 cache = 有時候真的更快】
//   壓成 bit 之後，讀單一元素要多做 shift 與 mask，單次存取比讀一個 byte 貴。
//   但在資料量大到塞不進 cache 時，決定效能的是 cache miss 次數而非指令數。
//   同樣的 100 萬個旗標，vector<char> 要 1 MB（超過典型 L2），vector<bool> 只要
//   125 KB（可能整個塞進 L2）。埃氏篩法（Sieve of Eratosthenes）正是典型受益者。
//   但這是「有時候」——是否真的變快必須實測，不能想當然。
//
// 【概念補充 Concept Deep Dive】
//   記憶體佈局對照（示意；bit 在 word 內的實際排列順序是【實作定義】）：
//
//     std::vector<char> vc(16);      每個元素獨立佔一個 byte
//       ┌──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┐
//       │0 │1 │2 │3 │4 │5 │6 │7 │8 │9 │10│11│12│13│14│15│  = 16 bytes
//       └──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┘
//
//     std::vector<bool> vb(16);      16 個 bool 擠在同一個 64-bit word 裡
//       ┌─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬────────────────┐
//       │0│1│2│3│4│5│6│7│8│9│…│ │ │ │ │15│  剩下 48 bits 未用 │  = 8 bytes
//       └─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴────────────────┘
//        └────────────── 一個 word（本機 64 bits，實作定義）──────────────┘
//
//   存取第 i 個元素在編譯後大致是：
//       word_index = i / 64          → 實際是 i >> 6
//       bit_offset = i % 64          → 實際是 i & 63
//       讀: (words[word_index] >> bit_offset) & 1ul
//       寫: 設為 1 → words[word_index] |=  (1ul << bit_offset)
//           設為 0 → words[word_index] &= ~(1ul << bit_offset)
//   除法與取餘都會被編譯器換成位移與遮罩，成本很低。
//
// 【注意事項 Pay Attention】
//   1. capacity() 的單位是「元素個數」不是 bytes，估算記憶體要自己 / 8。
//   2. capacity 會被無條件進位到 word 邊界；1024、64 這些數字是【實作定義】，
//      換一個 STL 實作（例如 word 用 unsigned int）就會變成別的值。
//   3. sizeof(std::vector<bool>) 比 sizeof(std::vector<char>) 大（本機 40 vs 24），
//      這也是【實作定義】。小容器不見得比較省。
//   4. 「壓縮就一定更快」是錯的。單次存取變慢、整體 cache 變好，淨效果要實測。
//   5. 多執行緒下要特別小心：相鄰的元素共用同一個 word，兩個執行緒寫入
//      vb[0] 與 vb[1] 會形成 data race（vector<char> 則不會）。詳見下方面試題。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】bit packing 與記憶體效率
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. vector<bool>(1000) 大約佔多少記憶體？capacity() 回傳什麼？
//     答：payload 約 128 bytes。本機實測 capacity() 回傳 1024（不是 1000，也不是
//         byte 數）——配置以 word 為單位，1000 bits 需 16 個 64-bit word = 1024 bits。
//         換算 bytes 要寫 capacity() / 8。
//     追問：那 sizeof(std::vector<bool>) 是多少？→ 本機 40 bytes（實作定義），
//         比 vector<char> 的 24 還大，因為 bit iterator 要存「word 指標 + bit 位移」。
//
// 🔥 Q2. 為什麼壓成 1 bit 反而可能讓程式更快？
//     答：單次存取確實變慢（多了 shift 和 mask）。但資料量大時瓶頸是 cache miss，
//         不是指令數。100 萬個旗標從 1 MB 縮到 125 KB，可能從「塞不進 L2」變成
//         「塞得進 L2」，整體反而快。埃氏篩法是經典案例。
//     追問：那什麼時候反而更慢？→ 資料量小（壓縮省不到 cache，還多付指令成本）、
//         或需要頻繁隨機單點寫入時。是否更快一定要實測。
//
// ⚠️ 陷阱. 「多執行緒各寫各的元素不用加鎖，反正沒有寫到同一個位置。」
//     答：對 vector<char> 成立，對 vector<bool> 不成立。vb[0] 與 vb[1] 住在同一個
//         word 裡，寫入是「read-modify-write 整個 word」，兩執行緒同時寫就是
//         data race，是 undefined behavior。
//     為什麼會錯：C++ 記憶體模型保證「不同記憶體位置」可以並行寫入而不需同步，
//         多數人記得這條規則，卻忘了 bit packing 讓不同的『元素』落在同一個
//         『記憶體位置』上——標準對此有特別註記，vector<bool> 不適用該保證。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 204. Count Primes
//   題目：給定整數 n，回傳所有小於 n 的質數個數。
//   為什麼用到本主題：埃氏篩法需要一個長度 n 的布林標記陣列。n 可以到 5 * 10^6，
//     用 vector<char> 要 5 MB，用 vector<bool> 只要約 625 KB——正是 bit packing
//     的最佳使用情境：純粹讀寫布林值、不需要取址或綁引用。
//   複雜度：時間 O(n log log n)，空間 O(n) bits。
// -----------------------------------------------------------------------------
int countPrimes(int n) {
    if (n < 3) return 0;                       // 小於 3 時沒有質數（2 不算「小於 2」）

    std::vector<bool> is_composite(n, false);  // n bits，不是 n bytes
    int count = 0;

    for (int i = 2; i < n; ++i) {
        if (is_composite[i]) continue;         // 已被標記為合數
        ++count;
        // 從 i*i 開始標記（比 i*2 起跳少做很多重複工作）
        // 用 long long 避免 i*i 在 int 上溢位
        for (long long j = 1LL * i * i; j < n; j += i) {
            is_composite[static_cast<std::size_t>(j)] = true;
        }
    }
    return count;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】Bloom filter 的 bitmap 位元陣列
//   情境：爬蟲要判斷某個 URL「是否可能已經抓過」。真的存下上億條 URL 太耗記憶體，
//         改用 Bloom filter：開一個大 bitmap，用 k 個 hash 把 URL 映射到 k 個 bit。
//         查詢時若 k 個 bit 全為 1 → 可能看過（有偽陽性）；只要有一個是 0 → 一定沒看過。
//   為什麼用 vector<bool>：bitmap 需要的正是「大量布林值 + 只做單點讀寫」，
//         而且省記憶體是這個資料結構存在的全部理由。1000 萬 bits 只要 1.25 MB。
//   注意：這裡刻意用 vector<bool>，因為完全不需要取址、綁引用或傳給泛型模板。
// -----------------------------------------------------------------------------
class BloomFilter {
public:
    explicit BloomFilter(std::size_t bits) : bits_(bits, false), nbits_(bits) {}

    void add(const std::string& key) {
        for (int k = 0; k < kNumHash; ++k) {
            bits_[hash(key, k)] = true;
        }
    }

    // 回傳 false = 一定沒看過；回傳 true = 可能看過（Bloom filter 沒有偽陰性）
    bool mightContain(const std::string& key) const {
        for (int k = 0; k < kNumHash; ++k) {
            if (!bits_[hash(key, k)]) return false;
        }
        return true;
    }

    // payload 佔用的位元組數（capacity 單位是「bool 個數」，要除以 8）
    std::size_t approxBytes() const { return bits_.capacity() / 8; }

private:
    static constexpr int kNumHash = 3;

    std::size_t hash(const std::string& key, int seed) const {
        // FNV-1a 變形；seed 讓同一個 key 得到 k 個不同位置
        std::size_t h = 1469598103934665603ULL + static_cast<std::size_t>(seed) * 0x9e3779b9ULL;
        for (unsigned char c : key) {
            h ^= c;
            h *= 1099511628211ULL;
        }
        return h % nbits_;
    }

    std::vector<bool> bits_;
    std::size_t nbits_;
};

int main() {
    std::cout << "=== 原始範例：三種存法的記憶體對比 ===" << std::endl;
    const int N = 1000;

    // 普通的 bool 陣列：每個 bool 佔 1 byte
    bool arr[N];
    std::cout << "bool[1000] 大小：" << sizeof(arr) << " bytes" << std::endl;

    // vector<char> 模擬存 bool：每個也佔 1 byte
    std::vector<char> vc(N);
    std::cout << "vector<char>(1000) 容量佔用：約 "
              << vc.capacity() * sizeof(char) << " bytes" << std::endl;

    // vector<bool>：每個 bool 只佔 1 bit
    std::vector<bool> vb(N);
    std::cout << "vector<bool>(1000) 容量佔用：約 "
              << vb.capacity() / 8 << " bytes" << std::endl;

    std::cout << "\n=== capacity() 的單位是「元素個數」不是 bytes ===" << std::endl;
    std::cout << "vb.size()     = " << vb.size()     << " 個 bool" << std::endl;
    std::cout << "vb.capacity() = " << vb.capacity() << " 個 bool（被進位到 word 邊界）"
              << std::endl;
    std::cout << "換算 payload  = " << vb.capacity() / 8 << " bytes" << std::endl;

    std::cout << "\n=== 容器物件本身的大小（實作定義）===" << std::endl;
    std::cout << "sizeof(std::vector<char>) = " << sizeof(std::vector<char>)
              << " bytes（3 個指標）" << std::endl;
    std::cout << "sizeof(std::vector<bool>) = " << sizeof(std::vector<bool>)
              << " bytes（bit iterator 比較胖）" << std::endl;

    std::cout << "\n=== capacity 成長：以 word 為單位進位 ===" << std::endl;
    std::vector<bool> grow;
    for (int i = 0; i < 3; ++i) {
        grow.push_back(true);
        std::cout << "push_back 第 " << grow.size() << " 個後 capacity = "
                  << grow.capacity() << "（一整個 word）" << std::endl;
    }

    std::cout << "\n=== LeetCode 204. Count Primes ===" << std::endl;
    std::cout << "countPrimes(10)      = " << countPrimes(10)      << "（2,3,5,7）" << std::endl;
    std::cout << "countPrimes(0)       = " << countPrimes(0)       << std::endl;
    std::cout << "countPrimes(1)       = " << countPrimes(1)       << std::endl;
    std::cout << "countPrimes(100)     = " << countPrimes(100)     << std::endl;
    std::cout << "countPrimes(1000000) = " << countPrimes(1000000) << std::endl;
    std::cout << "篩到 100 萬只用約 " << (1000000 / 8 / 1024)
              << " KB，換成 vector<char> 要約 " << (1000000 / 1024) << " KB" << std::endl;

    std::cout << "\n=== 日常實務：Bloom filter bitmap ===" << std::endl;
    BloomFilter bf(1 << 20);                   // 100 萬個 bit
    bf.add("https://example.com/page/1");
    bf.add("https://example.com/page/2");

    std::cout << "bitmap 佔用約 " << bf.approxBytes() << " bytes（100 萬 bits）"
              << std::endl;
    std::cout << "page/1 已抓過？ "
              << (bf.mightContain("https://example.com/page/1") ? "可能" : "一定沒有")
              << std::endl;
    std::cout << "page/2 已抓過？ "
              << (bf.mightContain("https://example.com/page/2") ? "可能" : "一定沒有")
              << std::endl;
    std::cout << "page/999 已抓過？ "
              << (bf.mightContain("https://example.com/page/999") ? "可能" : "一定沒有")
              << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 18 課：vectorbool 的特殊性與陷阱2.cpp" -o vb2

// === 預期輸出 ===
// === 原始範例：三種存法的記憶體對比 ===
// bool[1000] 大小：1000 bytes
// vector<char>(1000) 容量佔用：約 1000 bytes
// vector<bool>(1000) 容量佔用：約 128 bytes
//
// === capacity() 的單位是「元素個數」不是 bytes ===
// vb.size()     = 1000 個 bool
// vb.capacity() = 1024 個 bool（被進位到 word 邊界）
// 換算 payload  = 128 bytes
//
// === 容器物件本身的大小（實作定義）===
// sizeof(std::vector<char>) = 24 bytes（3 個指標）
// sizeof(std::vector<bool>) = 40 bytes（bit iterator 比較胖）
//
// === capacity 成長：以 word 為單位進位 ===
// push_back 第 1 個後 capacity = 64（一整個 word）
// push_back 第 2 個後 capacity = 64（一整個 word）
// push_back 第 3 個後 capacity = 64（一整個 word）
//
// === LeetCode 204. Count Primes ===
// countPrimes(10)      = 4（2,3,5,7）
// countPrimes(0)       = 0
// countPrimes(1)       = 0
// countPrimes(100)     = 25
// countPrimes(1000000) = 78498
// 篩到 100 萬只用約 122 KB，換成 vector<char> 要約 976 KB
//
// === 日常實務：Bloom filter bitmap ===
// bitmap 佔用約 131072 bytes（100 萬 bits）
// page/1 已抓過？ 可能
// page/2 已抓過？ 可能
// page/999 已抓過？ 一定沒有
