// =============================================================================
//  第 18 課-7：陷阱四 —— vector<bool> 沒有 data()
// =============================================================================
//
// 【主題資訊 Information】
//   vector<T>::data()          → T*       （C++11 加入）
//   vector<T>::data() const    → const T*
//   vector<bool>               → 完全沒有 data() 這個成員函式
//   標準保證：vector<T>（T != bool）的元素在記憶體中連續存放，
//             因此 [data(), data()+size()) 是一段合法的連續範圍
//   標準版本：data() 是 C++11 加入；連續性保證也是 C++11 明文化
//   標頭檔：<vector>
//
// 【詳細解釋 Explanation】
//
// 【1. data() 的存在前提是「元素連續且型別為 T」】
//   data() 的合約是：回傳一根 T*，讓你能用 ptr[i] 存取第 i 個元素，
//   而且這段記憶體可以整塊交給 memcpy、fwrite、socket send 或任何 C API。
//   這個合約成立的前提有兩個：元素連續、而且每個元素真的是一個 T 物件。
//   vector<bool> 兩個都不滿足——它存的是壓縮過的 bit，
//   底層是一串 unsigned long 字組，不是 bool 陣列。
//   所以標準乾脆不給它 data()，而不是給一個會誤導人的版本。
//
// 【2. 這個「缺席」其實是誠實的設計】
//   假設標準硬要給 vector<bool> 一個 data()，它能回傳什麼？
//   回傳 unsigned long* 會讓泛型程式碼完全對不上型別；
//   回傳 bool* 則是徹底的謊言——那段記憶體裡根本不是 bool。
//   直接不提供，讓誤用在編譯期就被擋下來，是三個選項中最好的。
//   這也是為什麼「編譯錯誤」在這裡應該被視為保護而非阻礙。
//
// 【3. 沒有 data() 的實際後果】
//   凡是需要「一整塊連續 bool」的操作全部做不到：
//     memcpy(dst, vb.data(), n)          // 無法取得來源指標
//     fwrite(vb.data(), 1, n, fp)        // 無法直接寫檔
//     send(sock, vb.data(), n, 0)        // 無法直接送出
//     some_c_api(vb.data(), n)           // 無法與 C 函式庫互通
//   唯一的辦法是先逐一取值、複製到一個 vector<unsigned char>，
//   等於付出一次完整的 O(n) 轉換成本，也失去了原本省記憶體的意義。
//
// 【4. C++11 之前用 &v[0]，之後應該用 data()】
//   兩者對非空的 vector<T> 結果相同，但 data() 更好：
//   空容器時 &v[0] 要先解參考 v[0]，那已經是 undefined behavior；
//   data() 對空容器完全合法（回傳值不保證非空指標，但呼叫本身是安全的）。
//   所以現代程式碼一律用 data()。
//
// 【5. 替代方案的選擇】
//   需要與 C API 互通 → vector<unsigned char> / vector<uint8_t>
//     （每元素 1 byte，data() 可用，泛型行為完全正常）
//   固定大小 + 需要位元運算 → std::bitset<N>
//     （同樣 1 bit/元素，且提供 to_ulong()/to_string() 做轉換，
//       但大小必須是編譯期常數，也一樣沒有 data()）
//   兩者的取捨就是「記憶體」對上「可互通性」。
//
// 【概念補充 Concept Deep Dive】
//   ▸ vector<bool> 底層到底存什麼
//     libstdc++ 用 unsigned long（本機 x86-64 為 64 bits）當作字組單位，
//     內部維護一組 _Bit_iterator，各自記錄「指向哪個字組」與「第幾個 bit」。
//     所以它更像一個 bitset 的動態版本，只是名字取成了 vector。
//   ▸ 為什麼 std::array<bool, N> 沒有這個問題
//     array 沒有被特化，array<bool, N> 就是一個實實在在的 bool 陣列，
//     data() 可用、sizeof 就是 N。若大小固定且需要與 C 互通，
//     它比 vector<bool> 適合得多。
//   ▸ 連續性保證的歷史
//     C++98 其實沒有明文保證 vector 元素連續（只是所有實作都這麼做），
//     C++03 才透過 DR 補上，C++11 再加上 data()。
//     這也是為什麼很老的程式碼裡到處是 &v[0] 而看不到 data()。
//
// 【注意事項 Pay Attention】
//   1. vector<bool> 沒有 data()，呼叫它是編譯錯誤（不是回傳空指標）。
//   2. 要與 C API 互通請改用 vector<unsigned char> 或 std::array<bool, N>。
//   3. 現代程式碼一律用 data() 取代 &v[0]——後者對空容器是 UB。
//   4. data() 對空 vector 呼叫是安全的，但回傳值不保證非空，也不可解參考。
//   5. std::bitset 同樣沒有 data()，它提供 to_ulong()/to_string() 做轉換。
//   6. 把 vector<bool> 轉成連續 byte 需要一次 O(n) 複製，省下的記憶體會賠回去。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】vector<bool> 與 data()
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 vector<bool> 沒有 data() 成員函式？
//     答：data() 的合約是「回傳一根 T*，讓 [data(), data()+size())
//         成為一段可直接使用的連續 T 陣列」。
//         vector<bool> 底層是壓縮的 bit（libstdc++ 用 unsigned long 字組），
//         既不是連續的 bool 物件序列，單一 bit 也沒有位址，
//         這個合約根本無法滿足，所以標準選擇完全不提供。
//     追問：那為什麼不回傳 unsigned long* 之類的底層指標？
//         → 那會讓泛型程式碼型別完全對不上，而且暴露實作細節。
//           直接不提供、讓誤用在編譯期被擋下，是最誠實的選擇。
//
// 🔥 Q2. C++11 之後為什麼建議用 v.data() 而不是 &v[0]？
//     答：對非空容器兩者結果相同，但空容器時 &v[0] 必須先解參考 v[0]，
//         而空容器的 v[0] 已經是 undefined behavior。
//         data() 對空容器的呼叫本身完全合法（回傳值不保證非空、
//         也不可解參考，但不會觸發 UB）。
//     追問：data() 回傳的指標什麼時候會失效？
//         → 和迭代器完全相同：任何觸發重新配置的操作
//           （push_back 超過 capacity、insert、resize、reserve、
//           shrink_to_fit）都會使它失效。
//
// ⚠️ 陷阱. 「vector<bool> 省 8 倍記憶體，拿來存百萬個旗標再寫進檔案，
//          應該是最省空間的做法吧？」
//     答：寫檔這一步會把優勢全部賠掉。因為沒有 data()，
//         你無法把它直接交給 fwrite，必須先逐一取值複製到一塊
//         連續的 byte 緩衝區——那個緩衝區就是完整的 N bytes，
//         等於同時佔用了壓縮版與展開版兩份記憶體，
//         而且多付一次 O(n) 的轉換成本。
//     為什麼會錯：只算了「常駐記憶體」，沒算「I/O 時的瞬間峰值」
//         與轉換成本。真的要省空間又要能寫檔，
//         應該自己用 vector<uint8_t> 做位元打包，
//         或直接用 std::bitset 的 to_string()／to_ulong()——
//         關鍵是讓「壓縮格式」與「輸出格式」是同一份資料。
// ═══════════════════════════════════════════════════════════════════════════

#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

// -----------------------------------------------------------------------------
// 【日常實務範例】把大量旗標寫進二進位檔（點陣圖 / 遮罩檔）
//   情境：影像處理中常見的「前景遮罩」——每個像素一個 bool，
//         處理完要存成檔案給下游工具讀。
//   為什麼用本主題：這是「沒有 data() 到底有多痛」的具體展示。
//         vector<bool> 常駐時省 8 倍，但寫檔時必須整份展開成 byte，
//         峰值記憶體反而是壓縮版的 9 倍。
//         若改成自己用 vector<uint8_t> 做位元打包，
//         壓縮格式與輸出格式一致，就能直接 data() 寫出去。
// -----------------------------------------------------------------------------

// 做法 A：vector<bool> —— 必須先展開成連續 byte 才能寫出
std::size_t writeMaskFromBitVector(const std::vector<bool>& mask,
                                   std::vector<unsigned char>& outBuf) {
    outBuf.assign(mask.size(), 0);            // 這裡就佔掉完整的 N bytes
    for (std::size_t i = 0; i < mask.size(); ++i) {
        outBuf[i] = mask[i] ? 1u : 0u;
    }
    // 此時才能 fwrite(outBuf.data(), 1, outBuf.size(), fp)
    return outBuf.size();
}

// 做法 B：自己用 vector<uint8_t> 做位元打包 —— 壓縮格式即輸出格式
class PackedMask {
    std::vector<unsigned char> words_;   // 每個 byte 存 8 個旗標
    std::size_t                bits_ = 0;
public:
    explicit PackedMask(std::size_t n) : words_((n + 7) / 8, 0), bits_(n) {}

    void set(std::size_t i, bool v) {
        unsigned char& w = words_[i / 8];
        unsigned char  m = static_cast<unsigned char>(1u << (i % 8));
        if (v) w = static_cast<unsigned char>(w | m);
        else   w = static_cast<unsigned char>(w & ~m);
    }
    bool get(std::size_t i) const {
        return (words_[i / 8] >> (i % 8)) & 1u;
    }
    std::size_t bits() const { return bits_; }

    // 關鍵：壓縮後的資料本身就是連續的 byte，可以直接交給 C API
    const unsigned char* data() const { return words_.data(); }
    std::size_t          byteSize() const { return words_.size(); }
};

int main() {
    std::cout << std::boolalpha;

    std::cout << "=== 一、vector<int> 的 data() 正常可用 ===" << std::endl;
    std::vector<int> vi = {10, 20, 30};
    int* data_i = vi.data();
    std::cout << "vi.data()[0] = " << data_i[0] << std::endl;
    std::cout << "data() == &vi[0] : " << (vi.data() == &vi[0]) << std::endl;

    // data() 真正的價值：整塊交給 C API
    std::vector<int> copy(vi.size());
    std::memcpy(copy.data(), vi.data(), vi.size() * sizeof(int));
    std::cout << "memcpy 整塊複製後 copy = ";
    for (int x : copy) std::cout << x << " ";
    std::cout << std::endl;

    std::cout << "\n=== 二、vector<bool> 沒有 data()（編譯錯誤，故註解）===" << std::endl;
    std::vector<bool> vb = {true, false, true};
    // bool* data_b = vb.data();  // 編譯錯誤！vector<bool> 沒有 data() 成員函式
    //   底層不是 bool 陣列，而是位元壓縮的字組結構，
    //   不存在一段可以被指向的「連續 bool 序列」
    std::cout << "vb 讀值完全正常：";
    for (std::size_t i = 0; i < vb.size(); ++i) std::cout << static_cast<bool>(vb[i]) << " ";
    std::cout << std::endl;
    std::cout << "但無法取得底層指標，因此無法直接 memcpy / fwrite / send" << std::endl;

    std::cout << "\n=== 三、data() 對空容器安全，&v[0] 則是 UB ===" << std::endl;
    std::vector<int> empty;
    std::cout << "empty.data() 呼叫本身合法（回傳值不保證非空，且不可解參考）"
              << std::endl;
    std::cout << "  empty.size() = " << empty.size() << std::endl;
    std::cout << "  而 &empty[0] 必須先解參考 empty[0]，那已經是 UB（故不示範）"
              << std::endl;

    std::cout << "\n=== 四、日常實務：把遮罩寫成二進位檔 ===" << std::endl;
    const std::size_t N = 64000;      // 假設是 320x200 的影像遮罩

    // 做法 A：vector<bool> 常駐省空間，但輸出時要整份展開
    std::vector<bool> bitMask(N, false);
    for (std::size_t i = 0; i < N; i += 5) bitMask[i] = true;
    std::vector<unsigned char> expanded;
    std::size_t writtenA = writeMaskFromBitVector(bitMask, expanded);
    std::cout << "做法A vector<bool>：" << std::endl;
    std::cout << "  常駐約 " << (bitMask.capacity() / 8) << " bytes" << std::endl;
    std::cout << "  但輸出時另需展開緩衝區 " << writtenA << " bytes（峰值 ≈ 9 倍壓縮版）"
              << std::endl;

    // 做法 B：自己位元打包，壓縮格式即輸出格式
    PackedMask packed(N);
    for (std::size_t i = 0; i < N; i += 5) packed.set(i, true);
    std::cout << "做法B 自訂位元打包：" << std::endl;
    std::cout << "  常駐 " << packed.byteSize() << " bytes" << std::endl;
    std::cout << "  data() 可直接寫檔，不需展開（峰值就是 " << packed.byteSize()
              << " bytes）" << std::endl;

    // 驗證兩者內容一致
    bool same = true;
    for (std::size_t i = 0; i < N && same; ++i) {
        if (static_cast<bool>(bitMask[i]) != packed.get(i)) same = false;
    }
    std::cout << "兩者旗標內容一致: " << same << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 18 課：vectorbool 的特殊性與陷阱7.cpp" -o vb_no_data
//
// 【關於下方預期輸出的但書】
//   vector<bool> 的 capacity()/8 只是「約略」的 byte 數：
//   實際配置以字組（libstdc++ 在 x86-64 上為 64-bit 的 unsigned long）
//   為單位對齊並向上取整，故此數值僅供量級比較，屬實作定義。
//
// 【本檔未附 LeetCode 範例的理由】
//   本檔的主題是「與 C API 互通時的記憶體佈局約束」，
//   屬於系統程式設計與 ABI 層面的議題。
//   LeetCode 的執行環境不涉及檔案 I/O、socket 或 C 函式庫互通，
//   也不會考容器的底層佈局，硬套一題無法呈現重點，
//   因此改以影像遮罩寫檔的實務範例呈現這個取捨。

// === 預期輸出 ===
// === 一、vector<int> 的 data() 正常可用 ===
// vi.data()[0] = 10
// data() == &vi[0] : true
// memcpy 整塊複製後 copy = 10 20 30
//
// === 二、vector<bool> 沒有 data()（編譯錯誤，故註解）===
// vb 讀值完全正常：true false true
// 但無法取得底層指標，因此無法直接 memcpy / fwrite / send
//
// === 三、data() 對空容器安全，&v[0] 則是 UB ===
// empty.data() 呼叫本身合法（回傳值不保證非空，且不可解參考）
//   empty.size() = 0
//   而 &empty[0] 必須先解參考 empty[0]，那已經是 UB（故不示範）
//
// === 四、日常實務：把遮罩寫成二進位檔 ===
// 做法A vector<bool>：
//   常駐約 8000 bytes
//   但輸出時另需展開緩衝區 64000 bytes（峰值 ≈ 9 倍壓縮版）
// 做法B 自訂位元打包：
//   常駐 8000 bytes
//   data() 可直接寫檔，不需展開（峰值就是 8000 bytes）
// 兩者旗標內容一致: true
