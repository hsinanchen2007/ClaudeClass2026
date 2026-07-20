// =============================================================================
//  06_fstream_binary.cpp  —  二進位檔案讀寫
// =============================================================================
//  參考：
//    - https://en.cppreference.com/w/cpp/io/basic_fstream
//    - https://en.cppreference.com/w/cpp/io/basic_ostream/write
//    - https://en.cppreference.com/w/cpp/io/basic_istream/read
//    - https://en.cppreference.com/w/cpp/io/ios_base/openmode
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、文字模式 vs 二進位模式                                 │
//  └────────────────────────────────────────────────────────────┘
//
//  Windows：文字模式自動把 '\n' ↔ "\r\n" 互轉；對二進位資料就會破壞內容。
//  Linux/macOS：兩種模式行為一致（換行就是 LF），但仍建議「二進位資料用
//  binary mode」 — 維持跨平台一致。
//
//  打開時加 std::ios::binary：
//
//      std::ofstream out{path, std::ios::binary};
//      std::ifstream in{path,  std::ios::binary};
//
//  二進位模式下用：
//   * write(const char* p, std::streamsize n)
//   * read (char* p, std::streamsize n)
//   * gcount() — 上次 read 實際讀了多少 byte
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、序列化 POD 結構的常見姿勢                              │
//  └────────────────────────────────────────────────────────────┘
//
//      struct Header { uint32_t magic; uint32_t version; uint32_t count; };
//      Header h{...};
//      out.write(reinterpret_cast<const char*>(&h), sizeof(h));
//
//  注意：
//   * 結構必須是 trivially_copyable（沒有指標、virtual、不對齊的 ABI）
//   * 跨機器要注意「endian」 — 序列化前最好統一成 little-endian
//   * 結構中的 padding bytes 也會被寫進檔案（沒事，但要小心隱私）
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、tellp / tellg / seekp / seekg                         │
//  └────────────────────────────────────────────────────────────┘
//
//   tellp() 取「寫位置」(put pointer)
//   tellg() 取「讀位置」(get pointer)
//   seekp(off, dir) 跳到某位置寫
//   seekg(off, dir) 跳到某位置讀
//
//   dir 可以是 std::ios::beg / cur / end，省略預設 beg。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 四、本檔示範                                               │
//  └────────────────────────────────────────────────────────────┘
//
//   * Demo 1：寫一個含 magic、count、N 個 int 的二進位檔
//   * Demo 2：讀回來、驗證
//   * Demo 3：用 seekg 跳到中間讀
// =============================================================================

/*
補充筆記：std::fstream_binary
  - std::fstream_binary 屬於 iostream；stream 同時保存資料流位置、格式設定和錯誤狀態。
  - operator>> 會跳過空白並遇空白停止，getline 會讀到換行；混用時常需要先處理殘留 newline。
  - failbit、badbit、eofbit 代表不同狀態；讀取失敗後要 clear() 才能繼續使用 stream。
  - fstream 開檔模式要明確：in/out/app/binary/trunc 組合不同會影響是否覆蓋、追加或以二進位處理。
  - sync_with_stdio(false) 和 cin.tie(nullptr) 可提升競賽輸入速度，但混用 C stdio 和 iostream 時要小心順序。
  - stringstream 適合字串解析與格式化，但大量資料轉換時可考慮 charconv 降低 locale 和配置成本。
  - 二進位模式避免文字換行轉換，適合保存結構化 bytes、圖片或自訂格式。
  - 直接寫 struct bytes 會受 padding、endianness 和版本變動影響；跨平台格式應明確序列化欄位。
  - 讀取 binary 後要檢查實際讀到的 byte 數，不能假設一次 read 一定填滿 buffer。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】二進位檔案讀寫
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 文字模式與 std::ios::binary 差在哪？
//     答：在 Windows 上，文字模式會做換行轉換（寫入時 '\n' → "\r\n"，讀取時反向），
//     並可能對 Ctrl+Z 做特殊處理；在 Linux / macOS 上兩者沒有差別。因此任何非純文字的
//     資料（圖片、序列化結構、雜湊）都必須加 std::ios::binary，否則在 Windows 上會被
//     靜默損毀。二進位讀寫用 read()／write() 搭配 gcount()，而不是 >> / <<。
//     追問：tellg() 在文字模式下可靠嗎？（不完全可靠，換行轉換讓位移與檔案實體位元組
//     不一致；二進位模式才有明確語意）
//
// 🔥 Q2. 直接 write(reinterpret_cast<char*>(&obj), sizeof(obj)) 有什麼問題？
//     答：三個層面：① padding——struct 內的填充位元組內容未指定，寫出去的檔案不可攜
//     ② endianness——同一份位元組在大小端機器上解讀出的數值不同 ③ 型別大小與對齊在
//     不同平台／編譯選項下可能不同。而且只要 struct 含有指標或非 trivially copyable
//     的成員（如 std::string），這種寫法根本就是錯的。正解是定義明確的序列化格式，
//     逐欄位以固定寬度與固定位元組序寫出。
//
// ⚠️ 陷阱. read() 回傳成功就代表真的讀到我要的位元組數嗎？
//     答：不一定。要用 gcount() 取得「實際讀到幾個位元組」，並檢查 stream 狀態；檔案
//     提早結束時會設 eofbit 與 failbit，但已經讀進去的那部分資料仍在緩衝區裡。
//     為什麼會錯：把 read() 想成「要嘛全讀到、要嘛什麼都沒讀到」的原子操作，它其實是
//     「盡量讀」，短讀（short read）必須自己處理。
// ═══════════════════════════════════════════════════════════════════════════

#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>

struct Header {
    std::uint32_t magic;
    std::uint32_t count;
};

int main() {
    const std::string path = "tmp_data.bin";

    // ─────────────────────────────────────────────────────────
    // Demo 1：寫
    // ─────────────────────────────────────────────────────────
    {
        std::ofstream out{path, std::ios::binary};
        if (!out) { std::cerr << "open failed\n"; return 1; }

        Header h{0x43504D49u /* 'CPMI' */, 5};
        out.write(reinterpret_cast<const char*>(&h), sizeof(h));

        std::int32_t data[] = {10, 20, 30, 40, 50};
        out.write(reinterpret_cast<const char*>(data), sizeof(data));
    }

    // ─────────────────────────────────────────────────────────
    // Demo 2：讀整個結構回來
    // ─────────────────────────────────────────────────────────
    {
        std::ifstream in{path, std::ios::binary};
        if (!in) { std::cerr << "open failed\n"; return 1; }

        Header h;
        in.read(reinterpret_cast<char*>(&h), sizeof(h));
        std::cout << "[Demo2] magic = 0x" << std::hex << h.magic << std::dec
                  << ", count = " << h.count << '\n';

        std::vector<std::int32_t> data(h.count);
        in.read(reinterpret_cast<char*>(data.data()),
                static_cast<std::streamsize>(sizeof(std::int32_t) * h.count));
        std::cout << "[Demo2] data =";
        for (int v : data) std::cout << ' ' << v;
        std::cout << '\n';

        std::cout << "[Demo2] last read bytes = " << in.gcount() << '\n';
    }

    // ─────────────────────────────────────────────────────────
    // Demo 3：seek 跳到第 3 個 int 開始讀
    // ─────────────────────────────────────────────────────────
    {
        std::ifstream in{path, std::ios::binary};
        // 跳過 header
        in.seekg(sizeof(Header), std::ios::beg);
        // 再跳 2 個 int
        in.seekg(sizeof(std::int32_t) * 2, std::ios::cur);

        std::int32_t third;
        in.read(reinterpret_cast<char*>(&third), sizeof(third));
        std::cout << "[Demo3] data[2] = " << third << '\n';   // 預期 30
    }

    // ─────────────────────────────────────────────────────────
    // 實用範例 A：取得二進位檔案大小 — seekg(end) + tellg()
    //   工作上常見：要先知道檔案大小才能 reserve 適合的 buffer。
    // ─────────────────────────────────────────────────────────
    {
        std::ifstream in{path, std::ios::binary};
        in.seekg(0, std::ios::end);
        std::streampos sz = in.tellg();        // 檔案大小（bytes）
        in.seekg(0, std::ios::beg);            // 倒回開頭
        std::cout << "[file-size] " << path << " = "
                  << static_cast<long long>(sz) << " bytes\n";

        // 一次讀進整個 buffer
        std::vector<char> all(static_cast<size_t>(sz));
        in.read(all.data(), sz);
        std::cout << "[file-size] read " << in.gcount() << " bytes\n";
    }

    // ─────────────────────────────────────────────────────────
    // 實用範例 B：XOR 校驗和 — 把所有 bytes 異或起來，得到一個簡單 checksum
    //   工作上常用：簡單偵測檔案是否被修改（注意：XOR 弱、生產環境用 CRC/SHA）
    // ─────────────────────────────────────────────────────────
    {
        std::ifstream in{path, std::ios::binary};
        char ch;
        std::uint8_t xs = 0;
        while (in.get(ch)) {
            xs ^= static_cast<std::uint8_t>(ch);
        }
        std::cout << "[xor-checksum] = 0x" << std::hex
                  << static_cast<int>(xs) << std::dec << '\n';
    }

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：為什麼 read/write 要 reinterpret_cast 成 char*？
    //    A：standard 的 read/write API 接受 char*，因為「char* 可以對任何
    //       物件做 byte-level 存取」(strict aliasing 例外)。這是「逐 byte
    //       看記憶體」的合法路徑。
    //
    //  Q2：跨機器序列化要注意什麼？
    //    A：(1) endian — 用 htonl / ntohl 或 std::endian (C++20) 統一
    //       (2) 結構 padding — 用 #pragma pack / __attribute__((packed)) 強制
    //       (3) 32-bit / 64-bit 的型別大小不同 — 用 int32_t/int64_t 明寫
    //       (4) 壓縮 / 編碼 — 想跨語言考慮 protobuf / flatbuffers / cap'n proto
    //
    //  Q3：怎麼讀「未知大小」的二進位檔？
    //    A：開檔後 in.seekg(0, std::ios::end) → tellg() 取大小，再 seekg
    //       回 0；或用 std::filesystem::file_size(path)（C++17）。
    //
    return 0;
}
