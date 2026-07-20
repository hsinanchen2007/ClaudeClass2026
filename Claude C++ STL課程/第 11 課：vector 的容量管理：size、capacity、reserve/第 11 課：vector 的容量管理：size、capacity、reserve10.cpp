// =============================================================================
//  第 11 課：vector 的容量管理：size、capacity、reserve10.cpp
//  —  緩衝區配置：vector<uint8_t>(n) 與 reserve(n) 該選哪一個
// =============================================================================
//
// 【主題資訊 Information】
//   explicit vector(size_type n);                  // n 個 value-initialized 元素
//   void     reserve(size_type n);                 // 只配置空間，size 仍為 0
//   pointer  data() noexcept;                      // C++11 起：取得連續記憶體位址
//
//   標頭檔：<vector>、<cstdint>（uint8_t）
//   標準版本：vector(n) 的**單參數版本**是 C++11 起（C++98 只有
//             vector(n, const T& value = T())，且該版本要求 T 可複製）；
//             data() 是 C++11 起；本檔整體需要 C++11 以上。
//   複雜度：vector(n)  → O(n)（要把 n 個元素 value-initialize）
//           reserve(n) → O(1) 的建構 + 一次 O(n) 的記憶體配置，但**不初始化**
//
//   兩者的差別（本檔核心）：
//   ┌──────────────────────┬────────┬──────────┬────────────┬──────────────┐
//   │ 寫法                  │ size   │ capacity │ 有沒有寫記憶體│ 能否 v[i] 存取│
//   ├──────────────────────┼────────┼──────────┼────────────┼──────────────┤
//   │ vector<uint8_t> b(n) │ n      │ >= n     │ 是（全歸 0）│ 可以          │
//   │ b.reserve(n)         │ 0      │ >= n     │ 否          │ **不行（UB）**│
//   └──────────────────────┴────────┴──────────┴────────────┴──────────────┘
//
// 【詳細解釋 Explanation】
//
// 【1. 緩衝區場景為什麼特別容易選錯】
// 「我要一個 1MB 的緩衝區」這句話在 C 的世界裡就是 malloc(1024*1024)：
// 拿到一塊未初始化的記憶體，馬上可以用索引存取。很多人把這個直覺帶到
// C++，寫成 `buffer.reserve(1024*1024)` 然後 `buffer[i] = x` —— 這是 UB，
// 因為 reserve 不改變 size（見 5.cpp）。
//
// 對應到 C 的 malloc 的其實是 **reserve + push_back**，
// 而對應到 calloc（配置且歸零）的才是 **vector<uint8_t> b(n)**。
//
// 【2. vector<uint8_t> b(n) 會真的寫 n 個 0】
// 單參數建構子對元素做 value-initialization，uint8_t 會歸零。所以
// `std::vector<uint8_t> buffer(1024*1024)` 不只是配置 1MB，它還會**寫入**
// 1MB 的 0（libstdc++ 對 trivial 型別會走 memset，很快，但不是免費）。
//
// 這在兩種情況下是浪費：
//   * 馬上要用 read() / memcpy 把整塊覆蓋掉 → 歸零的工白做了。
//   * 緩衝區很大且只會用到一小部分 → 歸零強迫作業系統實際配給所有實體
//     頁框（touch 過的頁不能再是 lazy allocation），記憶體用量立刻拉滿。
//
// 【3. 那 I/O 緩衝區到底該怎麼寫】
// 關鍵在於「誰來填資料」：
//   (a) 由 C API / system call 直接寫入（read、recv、fread）
//       → 需要一塊**可索引、且 size 正確**的記憶體，因為你要傳
//         buffer.data() 和 buffer.size() 進去。此時只能用 vector<uint8_t> b(n)。
//         歸零成本無法避免（除非改用 unique_ptr<uint8_t[]> 之類的工具）。
//         讀完之後記得用實際回傳的位元組數 resize 縮回來。
//   (b) 由自己的程式逐一 append
//       → reserve(n) + push_back / insert，完全不付初始化成本。
//         這是「累積輸出」「序列化」「組封包」的正確寫法。
//
// 【4. data() 與 C API 的橋接】
// vector 保證元素**連續存放**，所以 data() 可以直接傳給 C API：
//     ssize_t n = ::read(fd, buffer.data(), buffer.size());
//     buffer.resize(n > 0 ? static_cast<size_t>(n) : 0);   // 縮回實際長度
// 注意兩點：
//   * 傳的是 size() 不是 capacity()。C API 只能寫進「已建構」的範圍；
//     寫進 size() 之外的 capacity 區域是 UB。
//   * 對空 vector 呼叫 data() 是合法的（C++11 起明確允許），但回傳值
//     可能是 nullptr，且**不可解參考**。
//
// 【概念補充 Concept Deep Dive】
// (A) 為什麼不用 vector<char> 而用 vector<uint8_t>
//   char 的正負號是實作定義（x86 Linux 上是有號，ARM 上常是無號），
//   處理二進位資料時 `if (buffer[i] > 127)` 這種判斷會因平台而異。
//   uint8_t 明確無號，語意是「一個位元組」而非「一個字元」。
//   C++17 另有 std::byte（<cstddef>），語意更嚴格：不支援算術運算，
//   只能做位元操作，適合純粹的位元組緩衝區。
//
// (B) 歸零的隱藏成本：實體記憶體立刻被佔用
//   現代作業系統的 malloc 對大塊配置是 lazy 的 —— 虛擬位址先給你，
//   實體頁框等到真正寫入時才配。`vector<uint8_t> b(n)` 因為要寫 0，
//   會 touch 過每一頁，於是 1MB 的緩衝區真的立刻吃掉 1MB 實體記憶體。
//   reserve(n) 則只要到虛擬位址，實際用多少才吃多少。
//   （不過 libstdc++ 對大塊的零初始化配置在某些情況可利用 calloc 的
//     zero page 優化，實際行為取決於配置器 —— 這屬於實作細節。）
//
// (C) 為什麼 reserve 之後 size() 是 0 卻能安全地傳 data()
//   data() 本身合法（C++11 起即使空容器也可呼叫），但它指向的是一塊
//   **沒有任何已建構物件**的記憶體。你可以拿這個指標去做位址運算，
//   但不能讀寫其中的元素 —— 那些 uint8_t 物件的生命期尚未開始。
//   這就是為什麼 reserve 的緩衝區不能直接餵給 read()。
//
// 【注意事項 Pay Attention】
// 1. **reserve(n) 之後 buffer[i] 是 UB**（size 仍是 0）。要索引存取必須用
//    vector<uint8_t> b(n) 或 resize(n)。
// 2. vector<uint8_t> b(n) 會**實際寫入 n 個 0**，不是「只配置不初始化」。
//    馬上要整塊覆蓋時這是純浪費。
// 3. 傳給 C API 的長度要用 **size()** 不是 capacity()；寫進 capacity 區域是 UB。
// 4. 讀取完成後用實際位元組數 resize() 縮回來，否則 size() 會謊報長度。
// 5. 二進位資料用 uint8_t 或 std::byte（C++17），不要用 char
//    —— char 的正負號是實作定義。
// 6. 本檔 capacity 數值為 libstdc++ / GCC 15.2 實測；標準只保證 >= 請求值。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】緩衝區配置
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. `std::vector<uint8_t> buf(1024)` 和 `buf.reserve(1024)` 差在哪？
//     答：前者建立 1024 個 value-initialized 元素（**實際寫入 1024 個 0**），
//         size=1024，可以直接 buf[i] 存取；後者只配置空間，size 仍是 0，
//         沒有寫任何記憶體，buf[i] 是 UB。
//         類比：前者像 calloc（配置且歸零），後者像 malloc（只配置）
//         —— 但 reserve 之後只能 push_back，不能索引。
//     追問：要傳給 read() 當緩衝區該用哪個？
//         → 必須用 vector<uint8_t> buf(n)。read() 要寫進 buf.data()，
//           而只有已建構的 [0, size()) 範圍能被寫入。
//
// 🔥 Q2. 為什麼傳給 C API 的長度要用 size() 而不是 capacity()？
//     答：capacity 與 size 之間那段是**未建構的原始記憶體**，上面沒有任何
//         物件。讓 C API 寫進去等於在未開始生命期的物件上寫資料，是 UB，
//         而且 vector 的 size() 不會因此更新，後續走訪會漏掉那些資料。
//     追問：那讀完之後怎麼讓 size 反映實際長度？
//         → 用實際回傳的位元組數 resize()：
//           buf.resize(static_cast<size_t>(bytesRead))。
//
// ⚠️ 陷阱. 「reserve 比較快，所以大緩衝區都用 reserve」——什麼時候會出事？
//     答：只要接下來是用索引存取（buf[i] = x）或把 data() 交給會寫入的
//         C API，就是 UB。reserve 只在「接下來全部用 push_back / insert
//         填入」時才正確。
//     為什麼會錯：把 reserve 理解成 malloc。malloc 回來的記憶體可以立刻
//         索引存取，但 reserve 之後 vector 的 size 還是 0 ——
//         C++ 的物件生命期尚未開始，記憶體「可存取」不等於「物件存在」。
// ═══════════════════════════════════════════════════════════════════════════

#include <vector>
#include <cstdint>
#include <cstring>   // std::memcpy
#include <iostream>
#include <string>

int main() {
    const std::size_t kOneMB = 1024 * 1024;

    // ---- 做法一：vector<uint8_t> b(n) —— 配置且歸零，可直接索引存取 ----
    std::cout << "=== 做法一：vector<uint8_t> buffer(1MB) ===" << std::endl;
    std::vector<uint8_t> buffer(kOneMB);   // 建立 1MB 緩衝區，全部歸 0
    std::cout << "size=" << buffer.size()
              << ", capacity=" << buffer.capacity() << std::endl;
    std::cout << "可直接索引存取: buffer[0]=" << static_cast<int>(buffer[0])
              << "（value-initialize 保證歸 0）" << std::endl;
    buffer[0] = 0xFF;
    std::cout << "寫入後 buffer[0]=" << static_cast<int>(buffer[0])
              << std::endl;

    // ---- 做法二：reserve —— 只配置空間，size 仍為 0 ----
    std::cout << "\n=== 做法二：buffer2.reserve(1MB) ===" << std::endl;
    std::vector<uint8_t> buffer2;
    buffer2.reserve(kOneMB);               // 只 reserve，不建構任何元素
    std::cout << "size=" << buffer2.size()
              << ", capacity=" << buffer2.capacity() << std::endl;
    std::cout << "size 是 0 → buffer2[0] 是 UB，只能用 push_back 填入"
              << std::endl;

    std::cout << "\n=== 兩者的記憶體語意 ===" << std::endl;
    std::cout << "buffer  已建構位元組: " << buffer.size()
              << "（已寫入 " << buffer.size() << " 個 0）" << std::endl;
    std::cout << "buffer2 已建構位元組: " << buffer2.size()
              << "（一個位元組都沒寫，只保留了虛擬位址空間）" << std::endl;

    // -------------------------------------------------------------------------
    // 【日常實務範例】封包組裝：用 reserve + append，避免無謂的歸零
    //   情境：組一個簡單的長度前綴訊息 [4 bytes 長度][payload]，
    //   丟進 socket 送出。資料是自己逐段 append 的 → reserve 才是對的。
    // -------------------------------------------------------------------------
    std::cout << "\n=== 日常實務: 組裝長度前綴封包（reserve + append）==="
              << std::endl;
    {
        const std::string payload = "GET /metrics HTTP/1.1";
        std::vector<uint8_t> packet;

        // 已知最終大小 = 4 bytes 長度前綴 + payload → 一次 reserve 到位
        packet.reserve(4 + payload.size());

        // 寫入 4 bytes 長度前綴（big-endian）
        const uint32_t len = static_cast<uint32_t>(payload.size());
        packet.push_back(static_cast<uint8_t>((len >> 24) & 0xFF));
        packet.push_back(static_cast<uint8_t>((len >> 16) & 0xFF));
        packet.push_back(static_cast<uint8_t>((len >> 8) & 0xFF));
        packet.push_back(static_cast<uint8_t>(len & 0xFF));

        // 接上 payload
        packet.insert(packet.end(), payload.begin(), payload.end());

        std::cout << "payload 長度: " << payload.size() << " bytes"
                  << std::endl;
        std::cout << "封包 size=" << packet.size()
                  << ", capacity=" << packet.capacity()
                  << "（一次配置到位，過程零重新配置）" << std::endl;
        std::cout << "長度前綴: ";
        for (std::size_t i = 0; i < 4; ++i) {
            std::cout << static_cast<int>(packet[i]) << " ";
        }
        std::cout << "→ 解回長度 = " << static_cast<int>(packet[3])
                  << std::endl;
    }

    // -------------------------------------------------------------------------
    // 【日常實務範例 2】模擬 read()：C API 寫入後用實際長度 resize 縮回
    //   真實程式會是 ::read(fd, buf.data(), buf.size())；
    //   這裡用 memcpy 模擬「外部寫入了 n 個位元組」，行為可重現。
    // -------------------------------------------------------------------------
    std::cout << "\n=== 日常實務: 讀取後用實際長度 resize ===" << std::endl;
    {
        const std::size_t kChunk = 4096;
        std::vector<uint8_t> buf(kChunk);   // 要給 C API 寫 → 必須有 size

        const char fakeData[] = "HTTP/1.1 200 OK";
        const std::size_t bytesRead = sizeof(fakeData) - 1;  // 模擬 read 回傳值

        // 注意：長度要用 buf.size()，不是 buf.capacity()
        std::memcpy(buf.data(), fakeData, bytesRead);

        std::cout << "配置時 size=" << buf.size() << std::endl;
        std::cout << "實際讀到 " << bytesRead << " bytes" << std::endl;

        buf.resize(bytesRead);   // 縮回實際長度，否則 size() 會謊報
        std::cout << "resize 後 size=" << buf.size()
                  << ", capacity=" << buf.capacity()
                  << "（capacity 不變，記憶體留著下次重用）" << std::endl;

        std::cout << "內容: ";
        for (uint8_t c : buf) std::cout << static_cast<char>(c);
        std::cout << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 11 課：vector 的容量管理：size、capacity、reserve10.cpp" -o demo10

// 註：以下 capacity 數值為 libstdc++ / GCC 15.2 實測，非標準保證
//     （標準只保證 reserve(n)/vector(n) 後 capacity() >= n）。
//     resize 縮小不降 capacity 則是標準規定的行為。

// === 預期輸出 ===
// === 做法一：vector<uint8_t> buffer(1MB) ===
// size=1048576, capacity=1048576
// 可直接索引存取: buffer[0]=0（value-initialize 保證歸 0）
// 寫入後 buffer[0]=255
//
// === 做法二：buffer2.reserve(1MB) ===
// size=0, capacity=1048576
// size 是 0 → buffer2[0] 是 UB，只能用 push_back 填入
//
// === 兩者的記憶體語意 ===
// buffer  已建構位元組: 1048576（已寫入 1048576 個 0）
// buffer2 已建構位元組: 0（一個位元組都沒寫，只保留了虛擬位址空間）
//
// === 日常實務: 組裝長度前綴封包（reserve + append）===
// payload 長度: 21 bytes
// 封包 size=25, capacity=25（一次配置到位，過程零重新配置）
// 長度前綴: 0 0 0 21 → 解回長度 = 21
//
// === 日常實務: 讀取後用實際長度 resize ===
// 配置時 size=4096
// 實際讀到 15 bytes
// resize 後 size=15, capacity=4096（capacity 不變，記憶體留著下次重用）
// 內容: HTTP/1.1 200 OK
