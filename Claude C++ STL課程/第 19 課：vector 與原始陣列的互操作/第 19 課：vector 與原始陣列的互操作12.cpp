// =============================================================================
//  第 19 課-12：vector 與二進位檔 I/O —— data() 最實用的一個場景
// =============================================================================
//
// 【主題資訊 Information】
//   std::ostream& write(const char* s, std::streamsize n);   // <ostream>
//   std::istream& read (char* s,       std::streamsize n);   // <istream>
//   std::streamsize gcount() const;      // 上一次 read 實際讀到幾個字元
//   開檔模式：std::ios::binary（不可省略，見下）
//   前提：vector 元素連續（C++11 保證），故 data() 可直接當緩衝區
//   標準版本：C++98 起即有；連續性保證與 data() 自 C++11
//   標頭檔：<fstream>、<vector>
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼 vector 特別適合做二進位 I/O 的緩衝區】
//   read/write 需要的是「一根字元指標 + 一個長度」，
//   而 vector 的元素連續、data() 直接給得出那根指標。
//   相較之下：
//     ▸ 原生陣列：長度必須編譯期固定，而檔案大小是執行期才知道的
//     ▸ new[]：要自己管生命週期，例外路徑容易洩漏
//     ▸ deque/list：元素不連續，根本給不出那根指標
//   vector 是「大小動態 + 記憶體連續」的唯一組合，
//   這正是二進位 I/O 需要的。
//
// 【2. std::ios::binary 為什麼不能省】
//   在 Windows 上，文字模式會做換行轉換：
//   寫出時把 '\n'（0x0A）轉成 "\r\n"（0x0D 0x0A），讀入時反向轉換。
//   對二進位資料這是災難——任何值剛好是 0x0A 的位元組都會被
//   悄悄改成兩個位元組，檔案長度與內容全部錯亂。
//   在 Linux/macOS 上文字與二進位模式沒有差別，
//   所以這個 bug 只會在移植到 Windows 時爆發，
//   屬於最典型的「在我機器上沒問題」類錯誤。
//   結論：處理二進位一律加 std::ios::binary。
//
// 【3. reinterpret_cast 在這裡是必要且合法的】
//   write/read 的參數型別是 char*，而我們手上是 int*，
//   所以必須轉型。這裡用 reinterpret_cast 是標準允許的：
//   透過 char* / unsigned char* / std::byte* 檢視任何物件的
//   位元組表示，是 strict aliasing 規則的明文豁免。
//   反過來把任意 char* 轉成 int* 再解參考則不一定合法
//   （對齊與物件生命週期問題）。
//
// 【4. 取得檔案大小的標準手法與其限制】
//     ifs.seekg(0, std::ios::end);
//     std::streamoff size = ifs.tellg();
//     ifs.seekg(0, std::ios::beg);
//   注意 tellg() 回傳的是 std::streampos，不是整數型別，
//   要先轉成 std::streamoff 再轉 size_t，否則某些編譯器會給窄化警告。
//   更穩健的做法是用 std::filesystem::file_size()（C++17）。
//   另外：對「非 seekable」的串流（管線、socket）這招完全無效，
//   那時只能分塊讀到 EOF。
//
// 【5. 必須檢查 read 實際讀到多少】
//   read() 不保證讀滿你要求的長度——檔案被截斷、
//   讀到 EOF、I/O 錯誤都會讓它讀不足。
//   read() 本身不會丟例外（除非你設了 exceptions()），
//   必須用 gcount() 檢查實際讀到的位元組數。
//   忽略這一步的程式，在檔案損毀時會安靜地處理到未初始化的資料。
//
// 【6. 可攜性：這樣寫出來的檔案不是跨平台格式】
//   直接把 int 的記憶體位元組倒進檔案，隱含了三個平台相依假設：
//     ▸ 位元組序（endianness）：x86 是小端，某些平台是大端
//     ▸ 型別大小：sizeof(int) 不保證是 4
//     ▸ 結構的填充（padding）：struct 的位元組佈局因編譯器而異
//   同一台機器上自用（快取檔、暫存檔）完全沒問題；
//   要跨機器交換就必須定義明確的序列化格式
//   （固定寬度型別 + 明確位元組序，或用 Protocol Buffers / MessagePack 這類工具）。
//
// 【概念補充 Concept Deep Dive】
//   ▸ 為什麼不能直接寫出 vector<std::string>
//     string 內部是一根指向堆積的指標，把它的位元組寫進檔案
//     存下來的是「那次執行的記憶體位址」，毫無意義。
//     非 trivially copyable 的型別一律需要真正的序列化：
//     先寫長度、再寫字元內容。
//   ▸ resize 與 reserve 在這裡的關鍵差別
//     讀檔前必須用 resize(n)——它會真的建構 n 個元素，
//     使 data() 指向的記憶體是「已存在的物件」。
//     若只用 reserve(n)，size() 仍是 0，
//     往 data() 寫入等於在未建構的儲存空間上動手，且之後 size() 也不對。
//   ▸ 為什麼建議用 std::filesystem::file_size（C++17）
//     它不需要 seek、語意明確，而且對「檔案不存在」
//     這類錯誤有清楚的錯誤碼或例外，比 seekg/tellg 的組合穩健得多。
//
// 【注意事項 Pay Attention】
//   1. 二進位 I/O 一律加 std::ios::binary，否則 Windows 上會做換行轉換。
//   2. 讀檔前用 resize()（真的建構元素），不能只用 reserve()。
//   3. read() 不保證讀滿，務必用 gcount() 檢查實際讀取量。
//   4. 只有 trivially copyable 型別能這樣直接讀寫；string/含指標的類別不行。
//   5. 這種格式不可攜（位元組序、型別大小、struct padding 都相依於平台）。
//   6. seekg/tellg 取大小對非 seekable 串流無效；C++17 可用 filesystem::file_size。
//   7. 每次開檔後都要檢查串流狀態，不要假設一定成功。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】vector 與二進位檔 I/O
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 讀二進位檔到 vector 之前，該用 resize() 還是 reserve()？為什麼？
//     答：必須用 resize()。resize(n) 會真的建構 n 個元素，
//         使 size() 變成 n、data() 指向的是已存在的物件，
//         之後對 vector 的所有操作（走訪、size()、範圍 for）才正確。
//         reserve(n) 只抬高 capacity，size() 仍是 0，
//         往 data() 寫入是在未建構的儲存空間上動手，而且 size() 也不對。
//     追問：對 int 這種型別，resize 的「建構」會不會多花時間？
//         → 會把元素 value-initialize 成 0，等於多一次 memset。
//           對極大的緩衝區若在意這個成本，可用自訂的
//           default-init allocator 或 std::make_unique_for_overwrite（C++20）。
//
// 🔥 Q2. 為什麼二進位開檔一定要加 std::ios::binary？
//     答：在 Windows 上，文字模式會做換行轉換——寫出時把 '\n'（0x0A）
//         轉成 "\r\n"，讀入時反向轉換。對二進位資料而言，
//         任何值剛好是 0x0A 的位元組都會被悄悄改成兩個位元組，
//         檔案長度與內容全部錯亂。
//         Linux/macOS 上兩種模式相同，所以這個 bug 只在移植時才爆發。
//     追問：那 reinterpret_cast<const char*> 合法嗎？
//         → 合法。標準明文允許透過 char*、unsigned char*、std::byte*
//           檢視任何物件的位元組表示，這是 strict aliasing 的豁免條款。
//           反向（把任意 char* 轉成 int* 再解參考）則不一定合法。
//
// ⚠️ 陷阱. 「我用 write 把 vector<int> 存檔、read 讀回來，
//          在我的 Linux 機器上完全正確，所以這個格式沒問題。」
//     答：這個檔案不可攜。它隱含了三個平台相依假設：
//         位元組序（x86 是小端，某些架構是大端）、
//         sizeof(int) 的實際大小（標準只保證至少 16 bits）、
//         以及若寫的是 struct，還有編譯器決定的 padding 佈局。
//         換一台機器讀，數值可能整個錯亂而且不會有任何錯誤訊息。
//     為什麼會錯：把「記憶體佈局」誤當成「檔案格式」。
//         記憶體佈局是實作細節，檔案格式是需要明確定義的契約。
//         自用的快取檔這樣寫沒問題；
//         要跨機器交換就必須用固定寬度型別（int32_t）
//         加上明確的位元組序轉換，或直接用成熟的序列化函式庫。
// ═══════════════════════════════════════════════════════════════════════════

#include <cstdint>
#include <cstdio>     // std::remove
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

// -----------------------------------------------------------------------------
// 【日常實務範例】感測器資料紀錄器：把量測結果寫成本機快取檔再讀回
//   情境：一個資料擷取程式每秒收集數千筆讀數，
//         為了避免當機遺失，每隔一段時間就把緩衝區落地成二進位檔；
//         重啟時再把它讀回來繼續處理。
//   為什麼用本主題：這是「自用快取檔」的典型場景——
//         同一台機器寫、同一台機器讀，不需要跨平台，
//         所以直接倒記憶體是最快也最簡單的做法。
//   範例刻意加上「magic number + 版本 + 筆數」的檔頭，
//   因為即使是自用檔案，也應該能偵測出「讀到不是自己寫的檔案」
//   或「格式版本不符」——這是最低限度的自我保護。
// -----------------------------------------------------------------------------
struct Reading {
    std::int32_t sensorId;
    std::int32_t timestamp;
    float        value;
};   // 全部是固定寬度的 POD 成員，可以直接倒記憶體

struct FileHeader {
    std::uint32_t magic;    // 'SENS' = 0x53454E53
    std::uint32_t version;
    std::uint64_t count;
};

static const std::uint32_t kMagic   = 0x53454E53u;
static const std::uint32_t kVersion = 1u;

bool saveReadings(const std::string& path, const std::vector<Reading>& data) {
    std::ofstream ofs(path, std::ios::binary);
    if (!ofs) return false;

    FileHeader h{kMagic, kVersion, static_cast<std::uint64_t>(data.size())};
    ofs.write(reinterpret_cast<const char*>(&h), sizeof(h));
    if (!data.empty()) {
        ofs.write(reinterpret_cast<const char*>(data.data()),
                  static_cast<std::streamsize>(data.size() * sizeof(Reading)));
    }
    return static_cast<bool>(ofs);      // 檢查寫入過程中是否出錯
}

// 回傳值：0 成功、1 開檔失敗、2 檔頭不符、3 資料不完整
int loadReadings(const std::string& path, std::vector<Reading>& out) {
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) return 1;

    FileHeader h{};
    ifs.read(reinterpret_cast<char*>(&h), sizeof(h));
    if (ifs.gcount() != static_cast<std::streamsize>(sizeof(h))) return 2;
    if (h.magic != kMagic || h.version != kVersion) return 2;

    out.resize(static_cast<std::size_t>(h.count));   // resize 而非 reserve
    if (h.count == 0) return 0;

    const std::streamsize want =
        static_cast<std::streamsize>(h.count * sizeof(Reading));
    ifs.read(reinterpret_cast<char*>(out.data()), want);
    if (ifs.gcount() != want) {                       // 必須檢查實際讀到多少
        out.clear();
        return 3;
    }
    return 0;
}

int main() {
    std::cout << std::boolalpha;

    std::cout << "=== 一、寫入二進位檔 ===" << std::endl;
    {
        std::vector<int> data = {100, 200, 300, 400, 500};

        std::ofstream ofs("data.bin", std::ios::binary);   // binary 不可省
        if (ofs) {
            // 直接把 vector 的底層記憶體寫入檔案
            ofs.write(reinterpret_cast<const char*>(data.data()),
                      static_cast<std::streamsize>(data.size() * sizeof(int)));
            std::cout << "寫入 " << data.size() << " 個 int 到 data.bin"
                      << "（共 " << data.size() * sizeof(int) << " bytes）" << std::endl;
        } else {
            std::cout << "開檔失敗" << std::endl;
        }
    }

    std::cout << "\n=== 二、讀取二進位檔 ===" << std::endl;
    {
        std::ifstream ifs("data.bin", std::ios::binary);
        if (ifs) {
            // 先取得檔案大小
            ifs.seekg(0, std::ios::end);
            std::streamoff file_size = ifs.tellg();     // tellg 回傳 streampos
            ifs.seekg(0, std::ios::beg);

            std::size_t count = static_cast<std::size_t>(file_size) / sizeof(int);
            std::cout << "檔案大小 " << file_size << " bytes → "
                      << count << " 個 int" << std::endl;

            // 關鍵：用 resize（真的建構元素），不能只用 reserve
            std::vector<int> data(count);
            ifs.read(reinterpret_cast<char*>(data.data()),
                     static_cast<std::streamsize>(count * sizeof(int)));

            // 必須檢查實際讀到多少
            std::cout << "gcount() 實際讀到 " << ifs.gcount() << " bytes，符合預期: "
                      << (ifs.gcount() ==
                          static_cast<std::streamsize>(count * sizeof(int))) << std::endl;

            std::cout << "從 data.bin 讀取：";
            for (int x : data) std::cout << x << " ";
            std::cout << std::endl;
        } else {
            std::cout << "開檔失敗" << std::endl;
        }
    }

    std::cout << "\n=== 三、resize 與 reserve 的差別 ===" << std::endl;
    {
        std::vector<int> a;
        a.reserve(5);
        std::cout << "reserve(5) → size=" << a.size() << ", capacity=" << a.capacity()
                  << "  ← size 是 0，往 data() 寫入是錯的" << std::endl;

        std::vector<int> b;
        b.resize(5);
        std::cout << "resize(5)  → size=" << b.size() << ", capacity=" << b.capacity()
                  << "  ← 5 個元素已建構，可安全當緩衝區" << std::endl;
    }

    std::cout << "\n=== 四、日常實務：感測器資料紀錄器 ===" << std::endl;
    const std::string path = "readings.bin";
    std::vector<Reading> readings = {
        {1, 1700000000, 21.5f}, {1, 1700000001, 21.7f}, {2, 1700000001, 55.2f},
        {1, 1700000002, 21.6f}, {2, 1700000002, 55.4f}
    };

    std::cout << "sizeof(Reading)   = " << sizeof(Reading) << " bytes" << std::endl;
    std::cout << "sizeof(FileHeader)= " << sizeof(FileHeader) << " bytes" << std::endl;

    bool saved = saveReadings(path, readings);
    std::cout << "存檔成功: " << saved << "（" << readings.size() << " 筆）" << std::endl;

    std::vector<Reading> loaded;
    int rc = loadReadings(path, loaded);
    std::cout << "讀檔回傳碼: " << rc << "（0=成功）, 讀回 " << loaded.size() << " 筆" << std::endl;
    for (const Reading& r : loaded) {
        std::cout << "  sensor=" << r.sensorId << " t=" << r.timestamp
                  << " value=" << r.value << std::endl;
    }

    std::cout << "\n=== 五、檔頭檢查能擋下「不是自己寫的檔案」 ===" << std::endl;
    {
        // 造一個內容不對的檔案
        std::ofstream bad("bogus.bin", std::ios::binary);
        const char junk[] = "this is not a readings file";
        bad.write(junk, sizeof(junk));
    }
    std::vector<Reading> tmp;
    int badRc = loadReadings("bogus.bin", tmp);
    std::cout << "讀取 bogus.bin 的回傳碼: " << badRc
              << "（2 = magic/version 不符，成功擋下）" << std::endl;

    // 清理本範例產生的檔案，避免污染工作目錄
    std::remove("data.bin");
    std::remove("readings.bin");
    std::remove("bogus.bin");
    std::cout << "\n（已清理本範例產生的 data.bin / readings.bin / bogus.bin）" << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 19 課：vector 與原始陣列的互操作12.cpp" -o binary_io
//
// 【關於下方預期輸出的但書】
//   ▸ sizeof(Reading) = 12、sizeof(FileHeader) = 16 是本機
//     （x86-64 / Ubuntu / GCC 15.2）的實測值，屬實作定義：
//     它取決於各成員的大小與編譯器的對齊/填充策略。
//     FileHeader 為 16 而非 4+4+8=16 剛好相等，是因為 uint64_t
//     需要 8-byte 對齊，前兩個 uint32_t 恰好填滿一個 8-byte 單位。
//   ▸ 本範例會在「目前工作目錄」建立 data.bin / readings.bin / bogus.bin，
//     並在程式結束前以 std::remove 全部刪除，不會殘留檔案。
//   ▸ 這種「直接倒記憶體」的檔案格式不可攜（位元組序、型別大小、
//     struct padding 皆相依於平台），僅適合同機器自用的快取檔。
//
// 【本檔未附 LeetCode 範例的理由】
//   本檔的主題是檔案 I/O 與序列化格式設計。
//   LeetCode 的執行環境完全不涉及檔案系統，
//   題目也不會考 ios::binary、gcount() 或位元組序，
//   硬套一題無法呈現重點；因此以感測器資料紀錄器
//   這個真實的落地/回載場景呈現，並示範最低限度的檔頭自我保護。

// === 預期輸出 ===
// === 一、寫入二進位檔 ===
// 寫入 5 個 int 到 data.bin（共 20 bytes）
//
// === 二、讀取二進位檔 ===
// 檔案大小 20 bytes → 5 個 int
// gcount() 實際讀到 20 bytes，符合預期: true
// 從 data.bin 讀取：100 200 300 400 500
//
// === 三、resize 與 reserve 的差別 ===
// reserve(5) → size=0, capacity=5  ← size 是 0，往 data() 寫入是錯的
// resize(5)  → size=5, capacity=5  ← 5 個元素已建構，可安全當緩衝區
//
// === 四、日常實務：感測器資料紀錄器 ===
// sizeof(Reading)   = 12 bytes
// sizeof(FileHeader)= 16 bytes
// 存檔成功: true（5 筆）
// 讀檔回傳碼: 0（0=成功）, 讀回 5 筆
//   sensor=1 t=1700000000 value=21.5
//   sensor=1 t=1700000001 value=21.7
//   sensor=2 t=1700000001 value=55.2
//   sensor=1 t=1700000002 value=21.6
//   sensor=2 t=1700000002 value=55.4
//
// === 五、檔頭檢查能擋下「不是自己寫的檔案」 ===
// 讀取 bogus.bin 的回傳碼: 2（2 = magic/version 不符，成功擋下）
//
// （已清理本範例產生的 data.bin / readings.bin / bogus.bin）
