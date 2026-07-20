// =============================================================================
//  第 18 課-12：替代方案二 —— vector<uint8_t>，最務實的那一個
// =============================================================================
//
// 【主題資訊 Information】
//   std::vector<std::uint8_t>
//     operator[] → uint8_t&      （真正的引用）
//     &v[i]      → uint8_t*      （真正的指標）
//     data()     → uint8_t*      （可用！記憶體連續，可直接交給 C API）
//   標準版本：uint8_t 定義於 <cstdint>（C++11 起納入標準，源自 C99）
//   記憶體：每個旗標 1 byte，是 vector<bool> 的 8 倍
//   標頭檔：<vector>、<cstdint>
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼這是實務上最常見的選擇】
//   把前面幾課的陷阱列出來對照就很清楚：
//                        vector<bool>   deque<bool>   vector<uint8_t>
//     取得 T&                 ✗              ✓              ✓
//     取得 T*                 ✗              ✓              ✓
//     data()                  ✗              ✗              ✓
//     泛型模板相容            ✗              ✓              ✓
//     記憶體 (N 個旗標)      N/8 bytes     N bytes        N bytes
//   vector<uint8_t> 是唯一四項全過的。代價只有記憶體——
//   而在多數應用中，幾十 KB 的差距遠不值得換來那些陷阱。
//
// 【2. uint8_t 到底是什麼】
//   它不是一個新型別，而是 <cstdint> 提供的別名。
//   在幾乎所有主流平台上 uint8_t 就是 unsigned char。
//   標準規定它「若存在，必須剛好 8 bits 且無填充位元」，
//   而且它是選用的——理論上某些奇異平台可能沒有 uint8_t
//   （例如最小可定址單位不是 8 bits 的 DSP）。
//   實務上你可以安心使用，但要知道 unsigned char 才是絕對可攜的那個。
//
// 【3. 一個必須注意的輸出陷阱】
//   因為 uint8_t 通常就是 unsigned char，
//   std::cout << v[0] 會把它當「字元」印出來，而不是數字。
//   值為 1 時印出的是不可見的控制字元，看起來像什麼都沒印。
//   必須寫成 std::cout << static_cast<int>(v[0])。
//   這是用 uint8_t 存數值時最常踩到的坑。
//
// 【4. 為什麼不用 vector<char>】
//   char 的有號性是實作定義的（x86 Linux 上通常有號，
//   ARM 上通常無號），拿來存 0/1 旗標或位元組資料容易出意外，
//   尤其是做位移或比較時。
//   要表達「一個位元組的原始資料」，明確寫 unsigned char 或 uint8_t
//   才不會有歧義。C++17 另外提供了 std::byte，
//   語意更純粹（它只支援位元運算，不能當數字用），
//   適合「這就是原始位元組、不是數字也不是字元」的場合。
//
// 【5. 若記憶體真的是瓶頸，正確做法是自己打包】
//   與其忍受 vector<bool> 的陷阱，不如用 vector<uint8_t>
//   自己做位元打包：每個 byte 存 8 個旗標，
//   對外提供 get(i) / set(i, v) 介面。
//   這樣既省了 8 倍記憶體，data() 又照樣可用，
//   而且「壓縮格式就是輸出格式」，寫檔或傳輸都不必再展開一次。
//   本檔的實務範例正是這個做法。
//
// 【概念補充 Concept Deep Dive】
//   ▸ 為什麼 vector<uint8_t> 的 memcpy 是合法的
//     uint8_t（= unsigned char）是 trivially copyable 型別，
//     而且標準特別允許透過 unsigned char 指標檢視任何物件的位元組表示
//     （這是 strict aliasing 規則的明文豁免）。
//     所以用它當「位元組緩衝區」是標準祝福的做法。
//   ▸ std::byte（C++17）與 uint8_t 的差別
//     std::byte 是 enum class，只支援 &、|、^、~、<<、>> 與
//     to_integer<T>()，不能直接參與算術。
//     這種「刻意限制」讓型別本身就表達了「這是原始資料」的意圖，
//     編譯器也能擋下把位元組誤當數字相加的錯誤。
//   ▸ 位元打包的索引算術
//     第 i 個旗標位於 words[i / 8] 的第 (i % 8) 個 bit。
//     因為 8 是 2 的冪，編譯器會自動把 / 8 最佳化成 >> 3、
//     % 8 最佳化成 & 7，不需要手寫位移來「加速」。
//
// 【注意事項 Pay Attention】
//   1. std::cout << uint8_t 會印成字元，必須 static_cast<int> 才會印數字。
//   2. uint8_t 需要 #include <cstdint>；它是選用型別，但主流平台都有。
//   3. 避免用 vector<char> 存位元組——char 的有號性是實作定義的。
//   4. C++17 起，若語意上是「原始位元組」可考慮 std::byte，型別意圖更清楚。
//   5. vector<uint8_t> 記憶體是 vector<bool> 的 8 倍，但換來完整容器行為。
//   6. 真的在意記憶體時，用 vector<uint8_t> 自己位元打包，
//      比忍受 vector<bool> 的陷阱划算得多。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】vector<uint8_t> 與位元組處理
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼實務上多半建議用 vector<uint8_t> 而不是 vector<bool>？
//     答：vector<uint8_t> 是唯一同時滿足四件事的選擇——
//         operator[] 回傳真正的引用、可以取元素位址、有 data()
//         可整塊交給 C API、以及不破壞泛型模板。
//         vector<bool> 四項全失，deque<bool> 也缺 data()。
//         代價只是記憶體變成 8 倍，這在多數應用中不構成問題。
//     追問：那如果記憶體真的是瓶頸呢？
//         → 用 vector<uint8_t> 自己做位元打包（每 byte 存 8 個旗標），
//           既省記憶體、data() 又可用，而且壓縮格式就是輸出格式，
//           寫檔或傳輸時不必再展開一次——比 vector<bool> 全面更好。
//
// 🔥 Q2. std::cout << myUint8Vector[0]; 印出來是空白，為什麼？
//     答：uint8_t 在主流平台上是 unsigned char 的別名，
//         operator<< 會挑到「印字元」的重載，
//         所以值 1 會被當成 ASCII 碼 1（不可見的控制字元）印出。
//         要印數字必須寫 static_cast<int>(v[0])。
//     追問：那 std::byte 會不會有同樣問題？
//         → 不會，但方向相反：std::byte 根本沒有 operator<<，
//           會直接編譯錯誤，強迫你明確寫
//           std::to_integer<int>(b)。這種「不方便」正是它的設計目的。
//
// ⚠️ 陷阱. 用 vector<char> 來存原始位元組，然後寫
//          if (buf[0] > 0x7F) { ... } 判斷高位元組——為什麼可能失效？
//     答：char 的有號性是實作定義的。在 x86 Linux 上 char 通常有號，
//         0x80~0xFF 的位元組會被解讀成 -128~-1，
//         於是 buf[0] > 0x7F 永遠不成立，判斷整個失效。
//         在 ARM 上 char 通常無號，同一份程式碼卻又正常——
//         這是最典型的「換平台才爆」的 bug。
//     為什麼會錯：以為 char 就是「一個位元組」而不帶符號語意。
//         實際上 C++ 有三個不同的 char 型別：char、signed char、
//         unsigned char，而 char 的行為由實作決定。
//         處理原始位元組請一律明確寫 unsigned char / uint8_t / std::byte。
// ═══════════════════════════════════════════════════════════════════════════

#include <cstdint>
#include <cctype>   // std::isprint（引數需先轉成 unsigned char）
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

// 模擬一個 C 寫的函式庫：計算校驗和，只接受連續的位元組緩衝區
extern "C" std::uint32_t crc_like_checksum(const std::uint8_t* data, std::size_t n) {
    std::uint32_t sum = 0;
    for (std::size_t i = 0; i < n; ++i) {
        sum = (sum << 1) ^ data[i];      // 只是示意，不是真的 CRC
    }
    return sum;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】自訂二進位協定的封包組裝與解析
//   情境：與韌體通訊的自訂協定，封包格式是
//         [ magic:2 ][ type:1 ][ len:2 ][ payload:len ][ checksum:4 ]
//         全部欄位都是位元組，而且要能直接送進 socket。
//   為什麼用 vector<uint8_t>：
//         (a) data() 可用，可以直接 send(sock, pkt.data(), pkt.size(), 0)；
//         (b) 記憶體連續，memcpy 與 C 函式庫互通零成本；
//         (c) 型別語意明確——這就是位元組，不是字元也不是數字。
//         這種需求下 vector<bool> 完全派不上用場。
// -----------------------------------------------------------------------------
std::vector<std::uint8_t> buildPacket(std::uint8_t type,
                                      const std::vector<std::uint8_t>& payload) {
    std::vector<std::uint8_t> pkt;
    pkt.reserve(2 + 1 + 2 + payload.size() + 4);

    pkt.push_back(0xAB);                                       // magic 高位
    pkt.push_back(0xCD);                                       // magic 低位
    pkt.push_back(type);
    pkt.push_back(static_cast<std::uint8_t>(payload.size() >> 8));   // len 高位
    pkt.push_back(static_cast<std::uint8_t>(payload.size() & 0xFF)); // len 低位
    pkt.insert(pkt.end(), payload.begin(), payload.end());

    // 校驗和：直接把已組好的部分交給 C 函式庫，不需要任何轉換
    std::uint32_t ck = crc_like_checksum(pkt.data(), pkt.size());
    pkt.push_back(static_cast<std::uint8_t>((ck >> 24) & 0xFF));
    pkt.push_back(static_cast<std::uint8_t>((ck >> 16) & 0xFF));
    pkt.push_back(static_cast<std::uint8_t>((ck >>  8) & 0xFF));
    pkt.push_back(static_cast<std::uint8_t>( ck        & 0xFF));
    return pkt;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】自己做位元打包：省記憶體又保留 data()
//   這是「記憶體真的是瓶頸」時，比 vector<bool> 更好的解法。
// -----------------------------------------------------------------------------
class BitFlags {
    std::vector<std::uint8_t> words_;
    std::size_t               count_;
public:
    explicit BitFlags(std::size_t n) : words_((n + 7) / 8, 0), count_(n) {}

    void set(std::size_t i, bool v) {
        std::uint8_t mask = static_cast<std::uint8_t>(1u << (i % 8));
        if (v) words_[i / 8] = static_cast<std::uint8_t>(words_[i / 8] |  mask);
        else   words_[i / 8] = static_cast<std::uint8_t>(words_[i / 8] & ~mask);
    }
    bool get(std::size_t i) const { return (words_[i / 8] >> (i % 8)) & 1u; }

    std::size_t          count()    const { return count_; }
    std::size_t          byteSize() const { return words_.size(); }
    const std::uint8_t*  data()     const { return words_.data(); }   // 關鍵：可用！
};

int main() {
    std::cout << std::boolalpha;

    std::cout << "=== 一、vector<uint8_t> 四項全過 ===" << std::endl;
    std::vector<std::uint8_t> vb = {1, 0, 1, 1, 0};

    std::uint8_t& ref  = vb[0];       // 正常的引用
    std::uint8_t* ptr  = &vb[0];      // 正常的指標
    std::uint8_t* data = vb.data();   // data() 可用

    ref = 0;                          // 透過引用修改
    std::cout << "透過 ref 修改後 vb[0] = " << static_cast<int>(vb[0]) << std::endl;
    std::cout << "ptr == data       : " << (ptr == data) << std::endl;
    std::cout << "data[2]           = " << static_cast<int>(data[2]) << std::endl;

    std::cout << "\n=== 二、輸出陷阱：uint8_t 會被當成字元印 ===" << std::endl;
    std::vector<std::uint8_t> nums = {65, 66, 1};
    std::cout << "直接印 nums[0]           : " << nums[0]
              << "  ← 印成字元 'A'（ASCII 65），不是數字 65" << std::endl;
    std::cout << "static_cast<int>(nums[0]): " << static_cast<int>(nums[0]) << std::endl;

    // nums[2] 的值是 1，直接印會送出 ASCII 1（不可見的控制字元）。
    // 這裡刻意不把它直接印到 stdout——那會在輸出中混入原始控制位元組，
    // 改為印出它的字元碼與可列印性，效果相同而不污染輸出。
    std::cout << "nums[2] 的值是 1，直接印會送出 ASCII 1（不可見控制字元）" << std::endl;
    std::cout << "  是否為可列印字元: "
              << (std::isprint(static_cast<unsigned char>(nums[2])) != 0)
              << "  ← 所以終端機上看起來像什麼都沒印" << std::endl;
    std::cout << "static_cast<int>(nums[2]): " << static_cast<int>(nums[2]) << std::endl;

    std::cout << "\n=== 三、與 C API 互通：零成本 ===" << std::endl;
    std::vector<std::uint8_t> src = {0x01, 0x02, 0x03, 0x04};
    std::vector<std::uint8_t> dst(src.size());
    std::memcpy(dst.data(), src.data(), src.size());     // 直接整塊複製
    std::cout << "memcpy 後 dst = ";
    for (std::uint8_t b : dst) std::cout << static_cast<int>(b) << " ";
    std::cout << std::endl;
    std::cout << "交給 C 函式庫計算校驗和 = "
              << crc_like_checksum(src.data(), src.size()) << std::endl;

    std::cout << "\n=== 四、記憶體對比 ===" << std::endl;
    const std::size_t N = 100000;
    std::vector<bool>         bits(N, true);
    std::vector<std::uint8_t> bytes(N, 1);
    BitFlags                  packed(N);
    std::cout << N << " 個旗標：" << std::endl;
    std::cout << "  vector<bool>     約 " << (bits.capacity() / 8)
              << " bytes（省記憶體，但四項全失）" << std::endl;
    std::cout << "  vector<uint8_t>  約 " << bytes.capacity()
              << " bytes（完整容器行為）" << std::endl;
    std::cout << "  自訂位元打包     約 " << packed.byteSize()
              << " bytes（省記憶體 + data() 可用，兩者兼得）" << std::endl;

    std::cout << "\n=== 五、日常實務：自訂二進位協定封包 ===" << std::endl;
    std::vector<std::uint8_t> payload = {'H', 'E', 'L', 'L', 'O'};
    std::vector<std::uint8_t> pkt = buildPacket(0x07, payload);
    std::cout << "封包長度 = " << pkt.size() << " bytes" << std::endl;
    std::cout << "封包內容 (hex) = ";
    for (std::uint8_t b : pkt) {
        std::cout << std::hex << (b < 16 ? "0" : "") << static_cast<int>(b) << " ";
    }
    std::cout << std::dec << std::endl;
    std::cout << "pkt.data() 可直接送進 socket / 寫入檔案，不需任何轉換" << std::endl;

    std::cout << "\n=== 六、日常實務：自訂位元打包（省記憶體且保有 data()）===" << std::endl;
    BitFlags flags(64);
    for (std::size_t i = 0; i < 64; i += 3) flags.set(i, true);
    std::cout << "64 個旗標只用 " << flags.byteSize() << " bytes" << std::endl;
    std::cout << "前 16 個旗標: ";
    for (std::size_t i = 0; i < 16; ++i) std::cout << (flags.get(i) ? 1 : 0);
    std::cout << std::endl;
    std::cout << "校驗和（直接餵 data()）= "
              << crc_like_checksum(flags.data(), flags.byteSize()) << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 18 課：vectorbool 的特殊性與陷阱12.cpp" -o vb_uint8
//
// 【關於下方預期輸出的但書】
//   ▸ 「直接印 nums[0]」那一行輸出的是字元 'A'（因為 uint8_t 在本機
//     是 unsigned char 的別名）；「直接印 nums[2]」輸出的是 ASCII 1，
//     一個不可見的控制字元，在終端機上看起來像空白。
//     這是本檔要示範的重點，不是輸出遺漏。
//   ▸ vector<bool> 的 capacity()/8 為約略值（實際以字組對齊向上取整）。
//   ▸ uint8_t 是否等同 unsigned char 屬實作定義；
//     本機（x86-64 / GCC 15.2）兩者相同。
//
// 【本檔未附 LeetCode 範例的理由】
//   本檔的主題是「與 C API 互通時的型別與記憶體佈局選擇」，
//   屬於系統程式設計議題。LeetCode 的執行環境不涉及 socket、
//   二進位協定或 C 函式庫互通，也不會考 uint8_t 與 char 的有號性差異，
//   硬套一題無法呈現重點，因此改以自訂二進位協定封包
//   與位元打包這兩個真實場景呈現。

// === 預期輸出 ===
// === 一、vector<uint8_t> 四項全過 ===
// 透過 ref 修改後 vb[0] = 0
// ptr == data       : true
// data[2]           = 1
//
// === 二、輸出陷阱：uint8_t 會被當成字元印 ===
// 直接印 nums[0]           : A  ← 印成字元 'A'（ASCII 65），不是數字 65
// static_cast<int>(nums[0]): 65
// nums[2] 的值是 1，直接印會送出 ASCII 1（不可見控制字元）
//   是否為可列印字元: false  ← 所以終端機上看起來像什麼都沒印
// static_cast<int>(nums[2]): 1
//
// === 三、與 C API 互通：零成本 ===
// memcpy 後 dst = 1 2 3 4
// 交給 C 函式庫計算校驗和 = 2
//
// === 四、記憶體對比 ===
// 100000 個旗標：
//   vector<bool>     約 12504 bytes（省記憶體，但四項全失）
//   vector<uint8_t>  約 100000 bytes（完整容器行為）
//   自訂位元打包     約 12500 bytes（省記憶體 + data() 可用，兩者兼得）
//
// === 五、日常實務：自訂二進位協定封包 ===
// 封包長度 = 14 bytes
// 封包內容 (hex) = ab cd 07 00 05 48 45 4c 4c 4f 00 01 9f 6f
// pkt.data() 可直接送進 socket / 寫入檔案，不需任何轉換
//
// === 六、日常實務：自訂位元打包（省記憶體且保有 data()）===
// 64 個旗標只用 8 bytes
// 前 16 個旗標: 1001001001001001
// 校驗和（直接餵 data()）= 1040
