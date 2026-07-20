// =============================================================================
//  第 12 課 (6)  —  data()：取得底層連續記憶體的原始指標
// =============================================================================
//
// 【主題資訊 Information】
//   T*       data() noexcept;
//   const T* data() const noexcept;
//
//   標頭檔：<vector>
//   標準版本：**C++11 起**（C++98 沒有 data()，當年只能用 &v[0]）
//   複雜度：O(1)
//   noexcept：是
//   回傳：指向底層陣列首元素的指標。
//         保證 [data(), data() + size()) 是合法的連續範圍。
//   空容器：**可以呼叫**（不是 UB），但回傳的指標不可解參考，
//           且是否為 nullptr 屬於實作定義
//
// 【詳細解釋 Explanation】
//
// 【1. data() 存在的根本原因：C++ 要能跟 C 世界對話】
// 大量真實世界的 API 只吃「指標 + 長度」：
//       read() / write() / memcpy() / OpenGL glBufferData()
//       zlib compress() / OpenSSL / 各種硬體 SDK / 資料庫驅動
// 這些 API 不認識 std::vector。data() 就是 vector 與它們之間的橋樑：
//       write(fd, v.data(), v.size() * sizeof(T));
// 有了 data()，我們可以在 C++ 這一側享受 RAII 的自動記憶體管理，
// 同時無縫地把資料交給 C API —— 不必手動 malloc/free，也不必多複製一份。
//
// 【2. 為什麼 data() 能成立？—— 連續性保證】
// data() 之所以有意義，前提是 vector 的元素在記憶體中**連續存放**。
// 這個保證寫在標準裡（[vector.overview]）：
//       &v[n] == v.data() + n，對所有 0 <= n < size() 成立
// 這是 vector 與其他容器最關鍵的差別：
//       std::vector  連續 → 有 data()
//       std::array   連續 → 有 data()
//       std::string  連續（C++11 起保證）→ 有 data()
//       std::deque   **不連續**（分段配置）→ **沒有** data()
//       std::list    不連續（節點各自配置）→ 沒有 data()
// 所以「能不能傳給 C API」這件事，本質上就是在問「元素連不連續」。
//
// 【3. data() vs &v[0]：為什麼 C++11 要新增 data()】
// C++98 時代想拿指標只能寫 &v[0]，但這個寫法有個缺陷：
// **當 v 為空時，v[0] 本身就是未定義行為**（解參考尾後位置），
// 即使你只是要取位址而沒有真的讀值。
//       T* p = v.empty() ? nullptr : &v[0];    // C++98 必須這樣寫
//       T* p = v.data();                        // C++11 之後，空容器也合法
// data() 對空容器是**明確定義**的：可以安全呼叫，只是回傳值不可解參考。
// 這讓「先取指標、再用 size() 判斷要不要用」的常見寫法變得安全。
//
// 【4. 空 vector 的 data() 回傳什麼？】
// 標準只說「回傳的指標不保證可解參考」，**沒有規定必須是 nullptr**。
//   * libstdc++（本機實測）：空 vector 的 data() 回傳 nullptr
//   * 其他實作可能回傳一個非 null 但不可解參考的位址
// 因此：**不可以用 data() == nullptr 來判斷容器是否為空**，
// 要判空請用 empty()。這是實作定義行為的典型例子。
//
// 【概念補充 Concept Deep Dive】
// data() 回傳的指標什麼時候失效？
// 答案與迭代器失效規則完全一致 —— **任何造成重新配置的操作**：
//       push_back / emplace_back（超過 capacity 時）
//       insert / resize / reserve / shrink_to_fit
// 因為擴容的動作是「配置新的更大的區塊 → 搬移元素 → 釋放舊區塊」，
// 舊指標指向的記憶體已經被 free 掉了。
//
//   擴容前:  data() ──▶ ┌────┬────┬────┐ (capacity 3，已滿)
//                       │ 10 │ 20 │ 30 │
//                       └────┴────┴────┘
//   push_back(40) 之後:
//            舊指標 ──▶ ✗ 已釋放的記憶體（懸空指標，dangling pointer）
//            data() ──▶ ┌────┬────┬────┬────┬────┬────┐ (新區塊)
//                       │ 10 │ 20 │ 30 │ 40 │    │    │
//                       └────┴────┴────┴────┴────┴────┘
//
// 實務守則：**要用 data() 就當場取、當場用完**，不要存起來跨越可能改動容器的程式碼。
// 若必須先取指標再大量寫入，記得先 reserve() 足夠容量，避免中途擴容。
//
// 【注意事項 Pay Attention】
// 1. data() 的指標在擴容後失效，使用失效指標是 UB。
// 2. 傳給 C API 時長度要用「位元組數」還是「元素數」要看該 API 的規格：
//    memcpy 要位元組數（v.size() * sizeof(T)），
//    而多數自訂的 C 函式吃的是元素個數。搞錯就是緩衝區溢位。
// 3. 空容器呼叫 data() 合法，但**不可解參考**；
//    也不可用回傳值是否為 nullptr 來判空（實作定義）。
// 4. const vector 的 data() 回傳 const T*，無法用來修改元素 —— 這是好事。
// 5. 別為了「效能」把 data() 當成繞過邊界檢查的手段。
//    ptr[i] 與 v[i] 產生的機器碼相同，用 data() 只是讓你失去容器的語意保護。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】vector::data()
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 std::vector 有 data() 而 std::deque 沒有？
//     答：data() 的意義建立在「元素連續存放」這個保證上 ——
//         標準保證 &v[n] == v.data() + n。deque 是分段配置的，
//         元素不在單一連續區塊中，因此無法回傳一個能涵蓋全部元素的指標。
//         vector / array / string（C++11 起）連續，所以都有 data()。
//     追問：那 deque 要傳給 C API 怎麼辦？→ 只能先複製到一個 vector
//         或連續緩衝區，沒有零成本的做法 —— 這正是選容器時要考慮的取捨。
//
// 🔥 Q2. C++11 為什麼要新增 data()？用 &v[0] 不就好了？
//     答：因為當 v 為空時，v[0] 本身就是未定義行為（解參考尾後位置），
//         即使只是取位址也一樣。data() 對空容器是明確定義的：
//         可以安全呼叫，只是回傳的指標不可解參考。
//     追問：空 vector 的 data() 一定回傳 nullptr 嗎？
//         → 不一定，標準沒有規定，屬於實作定義。
//           libstdc++ 本機實測回傳 nullptr，但不可依賴此行為判空，要用 empty()。
//
// ⚠️ 陷阱. 以下程式碼哪裡有問題？
//         int* p = v.data();
//         v.push_back(42);
//         p[0] = 99;
//     答：push_back 若觸發擴容，vector 會配置新區塊、搬移元素、釋放舊區塊，
//         此時 p 成為懸空指標（dangling pointer），p[0] = 99 就是未定義行為。
//         而且**只有在剛好觸發擴容時才會出錯** —— capacity 還夠時它看起來完全正常，
//         這種「時好時壞」正是最難除錯的 bug。
//     為什麼會錯：把 data() 想成「指向 vector 這個物件」，
//         其實它指向的是 heap 上那塊資料，而那塊資料隨時可能被換掉。
//         正確做法是用完即棄，或先 reserve() 確保不會擴容。
// ═══════════════════════════════════════════════════════════════════════════

#include <vector>
#include <iostream>
#include <cstring>   // std::memcpy
#include <cstdio>    // std::snprintf
#include <cstdint>

// -----------------------------------------------------------------------------
// 【日常實務範例 1】把量測資料交給只吃「指標 + 長度」的 C API
//   情境：專案裡有一支既有的 C 函式庫（或硬體 SDK）負責計算統計量，
//         介面是傳統的 const double* + size_t，完全不認識 std::vector。
//   為什麼用 data()：我們在 C++ 這側用 vector 管理生命週期（RAII，不會漏記憶體），
//         呼叫時用 data() 交出指標即可，**不需要額外複製一份資料**。
//         這是 C++ 與既有 C 程式碼共存最標準的橋接方式。
// -----------------------------------------------------------------------------
extern "C" double c_library_mean(const double* values, std::size_t count) {
    if (count == 0) return 0.0;          // C API 自己負責處理 count == 0
    double sum = 0.0;
    for (std::size_t i = 0; i < count; ++i) sum += values[i];
    return sum / static_cast<double>(count);
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】組出二進位封包：把結構化資料序列化進位元組緩衝區
//   情境：要送出一個自訂協定的封包，格式是
//         [1 byte 版本][2 bytes 長度][N bytes payload]。
//         網路 / 檔案 I/O 幾乎都是這種「先組好一塊連續位元組再一次送出」的模式。
//   為什麼用 data()：memcpy 與 send()/write() 都只吃 void*，
//         vector<uint8_t> 搭配 data() 就是最自然的可變長度位元組緩衝區。
//   重點：先 reserve/resize 配足空間再取 data()，避免寫入途中擴容導致指標失效。
// -----------------------------------------------------------------------------
std::vector<std::uint8_t> buildPacket(std::uint8_t version, const std::string& payload) {
    const std::uint16_t len = static_cast<std::uint16_t>(payload.size());

    std::vector<std::uint8_t> packet(1 + 2 + payload.size());  // 一次配足，不會中途擴容
    std::uint8_t* p = packet.data();                           // 配置完成後才取指標

    p[0] = version;
    std::memcpy(p + 1, &len, sizeof(len));                     // 長度欄（本機位元組序）
    std::memcpy(p + 3, payload.data(), payload.size());        // payload

    return packet;
}

void hexDump(const std::vector<std::uint8_t>& bytes) {
    char buf[4];
    for (std::uint8_t b : bytes) {
        std::snprintf(buf, sizeof(buf), "%02X ", b);
        std::cout << buf;
    }
}

int main() {
    std::cout << "=== data() 取得底層指標 ===\n";
    std::vector<int> v = {10, 20, 30, 40, 50};
    int* ptr = v.data();

    std::cout << "ptr[0] = " << ptr[0] << "\n";
    std::cout << "ptr[2] = " << ptr[2] << "\n";

    std::cout << "\n=== 透過指標修改，改的是容器本身 ===\n";
    ptr[1] = 200;
    std::cout << "ptr[1] = 200 之後，v[1] = " << v[1] << "\n";

    std::cout << "\n=== 連續性保證：&v[n] == v.data() + n ===\n";
    bool contiguous = true;
    for (std::size_t i = 0; i < v.size(); ++i) {
        if (&v[i] != v.data() + i) contiguous = false;
    }
    std::cout << "所有元素都滿足 &v[i] == data() + i ? "
              << std::boolalpha << contiguous << "\n";

    std::cout << "\n=== 空容器呼叫 data() 是合法的 ===\n";
    std::vector<int> empty_vec;
    const int* ep = empty_vec.data();
    std::cout << "空容器 data() 可安全呼叫（但回傳值不可解參考）\n";
    std::cout << "本機（libstdc++）回傳 nullptr ? "
              << std::boolalpha << (ep == nullptr) << "\n";
    std::cout << "註：是否為 nullptr 屬實作定義，判空請一律用 empty()\n";

    std::cout << "\n=== 與 C 風格 API 互動：memcpy ===\n";
    int dest[5] = {};
    std::memcpy(dest, v.data(), v.size() * sizeof(int));   // 注意是「位元組數」
    std::cout << "memcpy 到原生陣列: ";
    for (int x : dest) std::cout << x << " ";
    std::cout << "\n";

    std::cout << "\n=== 日常實務：交給只吃指標+長度的 C 函式庫 ===\n";
    std::vector<double> samples = {61.5, 62.0, 63.2, 64.8, 66.1};
    std::cout << "樣本數 = " << samples.size()
              << "，平均 = " << c_library_mean(samples.data(), samples.size()) << "\n";
    std::vector<double> no_samples;
    std::cout << "空樣本（data() 可能是 nullptr，但 count=0 所以 API 不會解參考）平均 = "
              << c_library_mean(no_samples.data(), no_samples.size()) << "\n";

    std::cout << "\n=== 日常實務：組出二進位封包 ===\n";
    std::vector<std::uint8_t> pkt = buildPacket(1, "HELLO");
    std::cout << "封包長度 = " << pkt.size() << " bytes\n";
    std::cout << "十六進位內容 = ";
    hexDump(pkt);
    std::cout << "\n（第 1 byte 是版本，接著 2 bytes 長度，其餘為 payload）\n";

    std::cout << "\n=== 指標失效示範（安全地觀察，不解參考舊指標）===\n";
    std::vector<int> w = {1, 2, 3};
    const int* before = w.data();
    std::cout << "push_back 前 capacity = " << w.capacity() << "\n";
    w.push_back(4);
    w.push_back(5);
    w.push_back(6);
    w.push_back(7);
    const int* after = w.data();
    std::cout << "push_back 後 capacity = " << w.capacity() << "\n";
    std::cout << "data() 指標是否改變 ? " << std::boolalpha << (before != after) << "\n";
    std::cout << "（指標若改變，先前取得的舊指標即成為懸空指標，不可再使用）\n";
    std::cout << "註：capacity 的成長倍率屬實作定義，libstdc++ 為 2 倍\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 12 課：vector 元素存取：operator[]、at、front、back6.cpp" -o access6
//
// ── 輸出但書 ────────────────────────────────────────────────────────────
// 1. 空容器的 data() 是否回傳 nullptr 屬實作定義；本機（GCC 15.2.0 /
//    libstdc++）為 nullptr，但標準只保證「可安全呼叫、不可解參考」。
// 2. capacity 由 3 成長為 12 是「libstdc++ 的 2 倍成長 + 本次插入需求」
//    的結果，成長倍率屬實作定義（libstdc++ 為 2 倍，MSVC 約 1.5 倍）。
// 3. 指標是否改變一律以布林印出，不印原始位址（位址每次執行都不同）。

// === 預期輸出 ===
// === data() 取得底層指標 ===
// ptr[0] = 10
// ptr[2] = 30
//
// === 透過指標修改，改的是容器本身 ===
// ptr[1] = 200 之後，v[1] = 200
//
// === 連續性保證：&v[n] == v.data() + n ===
// 所有元素都滿足 &v[i] == data() + i ? true
//
// === 空容器呼叫 data() 是合法的 ===
// 空容器 data() 可安全呼叫（但回傳值不可解參考）
// 本機（libstdc++）回傳 nullptr ? true
// 註：是否為 nullptr 屬實作定義，判空請一律用 empty()
//
// === 與 C 風格 API 互動：memcpy ===
// memcpy 到原生陣列: 10 200 30 40 50 
//
// === 日常實務：交給只吃指標+長度的 C 函式庫 ===
// 樣本數 = 5，平均 = 63.52
// 空樣本（data() 可能是 nullptr，但 count=0 所以 API 不會解參考）平均 = 0
//
// === 日常實務：組出二進位封包 ===
// 封包長度 = 8 bytes
// 十六進位內容 = 01 05 00 48 45 4C 4C 4F 
// （第 1 byte 是版本，接著 2 bytes 長度，其餘為 payload）
//
// === 指標失效示範（安全地觀察，不解參考舊指標）===
// push_back 前 capacity = 3
// push_back 後 capacity = 12
// data() 指標是否改變 ? true
// （指標若改變，先前取得的舊指標即成為懸空指標，不可再使用）
// 註：capacity 的成長倍率屬實作定義，libstdc++ 為 2 倍
