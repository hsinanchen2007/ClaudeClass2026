// =============================================================================
//  第 11 課：vector 的容量管理：size、capacity、reserve9.cpp
//  —  實務場景：讀取「已宣告筆數」的資料，用 reserve 一次配置到位
// =============================================================================
//
// 【主題資訊 Information】
//   本檔示範 reserve 最典型、也最該用的場景：
//     **資料來源已經告訴你有幾筆** → 直接 reserve(n) → 逐筆 push_back。
//
//   標頭檔：<vector>、<sstream>（模擬輸入來源）、<iostream>
//   標準版本：C++98 起（reserve）；本檔用到的語法皆為 C++11 相容。
//   複雜度：reserve(n) 後 n 次 push_back 共 0 次重新配置，總計 O(n)。
//
//   為什麼用 std::istringstream 而不是 std::cin？
//     教學範例必須**可重現地執行與驗證**。讀 std::cin 的程式在自動化測試
//     （沒有互動輸入）下行為不確定：讀取失敗時 n 會是 0（C++11 起失敗會
//     value-initialize），輸出便不可預期。改用 istringstream 內嵌固定資料，
//     邏輯與讀檔案完全相同，但結果穩定可驗證。
//     實務上把 iss 換成 std::ifstream 或 std::cin 即可，程式碼一行不用改。
//
// 【詳細解釋 Explanation】
//
// 【1. 這個模式為什麼重要】
// 許多真實資料格式都會在開頭宣告筆數：
//   * 競賽 / OJ 題目的輸入第一行就是 N
//   * 二進位格式的檔頭（record count 欄位）
//   * 網路協定的長度前綴（length-prefixed message）
//   * 資料庫匯出檔的 metadata 行
// 這是 reserve 的完美場景 —— 你在讀第一筆資料**之前**就知道總量，
// 一次配置到位，之後零重新配置、零搬移、零 iterator 失效。
//
// 【2. 讀取失敗必須先檢查，再拿去 reserve】
// 這是本檔刻意強調的安全點。若輸入格式錯誤（例如檔頭不是數字），
// `iss >> n` 會失敗；此時：
//   * C++11 起讀取失敗會把 n 設為 0（value-initialize），不是留著垃圾值。
//   * 但若 n 被讀成**負數**（例如檔案內容是 "-1"），`reserve(n)` 會把
//     負的 int 轉成無號的 size_type → 變成天文數字 → 丟 std::length_error。
// 所以正確流程永遠是：先驗證讀取成功、再驗證數值合理（非負、不超過上限），
// 最後才 reserve。本檔第三段實測示範了「-1 變成天文數字」這個轉換。
//
// 【3. 不要盲目相信檔頭宣告的數字】
// 「檔頭說有 10 億筆」不代表真的有，也不代表你該立刻配置 10 億筆的空間。
// 惡意或損壞的輸入可以用一個巨大的 count 讓你的程式瞬間耗盡記憶體
// （這是真實存在的 DoS 攻擊手法，稱為 decompression bomb 的變體）。
// 實務做法是設一個合理上限：
//     const std::size_t kMaxRecords = 10'000'000;
//     data.reserve(std::min(static_cast<std::size_t>(n), kMaxRecords));
// 超過上限就讓 vector 自己幾何成長 —— 慢一點，但不會被一個數字打垮。
//
// 【4. 讀完之後要不要 shrink_to_fit？】
// 看實際筆數與宣告筆數的落差：
//   * 完全相符（最常見）→ 不需要，capacity 已經剛好。
//   * 實際遠少於宣告（檔案截斷）→ 若資料要長期留著，可以 shrink_to_fit。
//   * 讀完馬上就要處理完丟掉 → 不必浪費一次 O(n) 搬移。
//
// 【概念補充 Concept Deep Dive】
// (A) 為什麼 reserve 要放在迴圈之前，而不是迴圈裡面
//   放迴圈裡（每輪 reserve(size()+1)）會關掉幾何成長，讓每次插入都重新
//   配置 → O(n²)。本機實測推 10 個元素，逐次 reserve 觸發 10 次重新配置，
//   什麼都不做只要 5 次（見 2.cpp 陷阱 2）。reserve 的價值來自
//   **一次就要到最終大小**。
//
// (B) 為什麼是 push_back 而不是 v[i] = x
//   reserve 不改變 size，reserve(n) 之後 size() 仍是 0，v[i] 是 UB
//   （見 5.cpp）。若想用索引賦值就得改用 resize(n)，但那會先把 n 個元素
//   value-initialize 一遍 —— 對「馬上要覆蓋」的情境是白做工。
//   讀取迴圈用 reserve + push_back 是最省的組合。
//
// (C) 讀取失敗時 n 的值：C++11 前後的差異
//   C++98：`iss >> n` 失敗時 **n 維持原值不變**（可能是未初始化的垃圾）。
//   C++11 起：失敗時 n 被設為 0。
//   所以在舊標準下 `int n;` 未初始化再讀取失敗，會拿到真正的垃圾值。
//   養成 `int n = 0;` 並檢查串流狀態的習慣，兩個標準下都安全。
//
// 【注意事項 Pay Attention】
// 1. **先檢查讀取是否成功、數值是否合理，再 reserve。** 負數轉成無號會變
//    天文數字，直接丟 std::length_error。
// 2. 不要無條件信任外部宣告的筆數，設上限以免被惡意輸入耗盡記憶體。
// 3. reserve 之後仍必須用 push_back / emplace_back 填入；size 還是 0，
//    用 v[i] 是 UB。
// 4. 實際讀到的筆數可能少於宣告（檔案截斷）—— 用 data.size() 而不是 n
//    作為後續處理的依據。
// 5. 本檔 capacity 數值為 libstdc++ / GCC 15.2 實測；標準只保證
//    reserve(n) 後 capacity() >= n。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】用 reserve 讀取已知筆數的資料
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 讀取 N 筆資料時，reserve(N) 該放在哪裡？放錯會怎樣？
//     答：放在讀取迴圈**之前**，一次配置到最終大小。若放進迴圈裡寫成
//         reserve(size()+1)，等於把幾何成長改成「每次加 1」——
//         libstdc++ 的 reserve 剛好配置 n，於是每次插入都重新配置，
//         整體退化成 O(n²)，比完全不 reserve 還慢。
//     追問：那 reserve 之後要用 v[i] = x 還是 push_back？
//         → 必須用 push_back。reserve 不改變 size，此時 size() 仍是 0，
//           v[i] 是越界存取 → UB。
//
// 🔥 Q2. `std::cin >> n;` 讀取失敗時，n 的值是什麼？後續 reserve(n) 安全嗎？
//     答：C++11 起讀取失敗會把 n 設為 0（C++98 則維持原值，可能是垃圾）。
//         但真正的風險是**讀到負數**：reserve(int) 會把負數轉成無號的
//         size_type，-1 變成 18446744073709551615，遠超過 max_size
//         → 丟 std::length_error。
//     追問：正確的防禦寫法？
//         → 先檢查串流狀態（if (!(iss >> n)) 錯誤處理），再檢查 n >= 0
//           且不超過合理上限，最後才 reserve。
//
// ⚠️ 陷阱. 檔頭宣告「有 20 億筆記錄」，直接 reserve 會怎樣？
//     答：程式會嘗試配置約 8 GB 記憶體（int 為 4 bytes）。在記憶體不足的
//         機器上丟 std::bad_alloc，甚至可能觸發 OOM killer ——
//         而這只需要攻擊者在檔頭改一個數字。
//     為什麼會錯：把外部輸入當成可信資料。正確做法是設一個合理上限，
//         超過就退回讓 vector 自己幾何成長：慢一點，但不會被一個數字打垮。
// ═══════════════════════════════════════════════════════════════════════════

#include <vector>
#include <iostream>
#include <sstream>
#include <string>
#include <algorithm>   // std::min

// -----------------------------------------------------------------------------
// 【日常實務範例】讀取「檔頭宣告筆數」的資料檔（感測器讀數批次匯入）
//   真實格式範例：
//       6                    ← 檔頭：本檔共 6 筆記錄
//       23 19 31 28 22 25    ← 資料列
//   實務上把 std::istream& 換成 std::ifstream 即可直接讀檔，程式碼不用改。
//
//   安全上限：不盲目相信檔頭宣告的數字，避免惡意/損壞輸入耗盡記憶體。
// -----------------------------------------------------------------------------
static const std::size_t kMaxRecords = 10000000;  // 一千萬筆的防護上限

struct LoadResult {
    std::vector<int> data;
    bool             ok;
    std::string      message;
};

static LoadResult loadRecords(std::istream& in) {
    LoadResult result;
    result.ok = false;

    int declared = 0;
    // ① 先檢查讀取是否成功 —— 不要讀完就直接拿去 reserve
    if (!(in >> declared)) {
        result.message = "檔頭讀取失敗（格式錯誤或檔案為空）";
        return result;
    }
    // ② 再檢查數值是否合理 —— 負數轉成無號會變天文數字
    if (declared < 0) {
        result.message = "檔頭宣告筆數為負數，拒絕處理";
        return result;
    }

    // ③ 設上限後才 reserve：一次配置到位，之後零重新配置
    const std::size_t want =
        std::min(static_cast<std::size_t>(declared), kMaxRecords);
    result.data.reserve(want);

    // ④ reserve 不改變 size，只能用 push_back 填入（不可用 data[i]）
    int x = 0;
    while (static_cast<int>(result.data.size()) < declared && (in >> x)) {
        result.data.push_back(x);
    }

    result.ok = true;
    if (static_cast<int>(result.data.size()) < declared) {
        result.message = "檔案截斷：實際筆數少於檔頭宣告";
    } else {
        result.message = "讀取完整";
    }
    return result;
}

int main() {
    std::cout << "=== 正常情況：檔頭宣告 6 筆，實際 6 筆 ===" << std::endl;
    {
        std::istringstream iss("6\n23 19 31 28 22 25\n");
        LoadResult r = loadRecords(iss);
        std::cout << "狀態: " << r.message << std::endl;
        std::cout << "size=" << r.data.size()
                  << ", capacity=" << r.data.capacity()
                  << "（reserve 一次到位，過程零重新配置）" << std::endl;
        std::cout << "內容: [";
        for (std::size_t i = 0; i < r.data.size(); ++i) {
            if (i != 0) std::cout << ' ';
            std::cout << r.data[i];
        }
        std::cout << "]" << std::endl;
    }

    std::cout << "\n=== 檔案截斷：宣告 10 筆，實際只有 4 筆 ===" << std::endl;
    {
        std::istringstream iss("10\n1 2 3 4\n");
        LoadResult r = loadRecords(iss);
        std::cout << "狀態: " << r.message << std::endl;
        std::cout << "size=" << r.data.size()
                  << ", capacity=" << r.data.capacity()
                  << "（capacity 依宣告配置，比實際多）" << std::endl;
        std::cout << "後續處理要用 data.size() 而不是檔頭的 10" << std::endl;
        // 資料若要長期留著，此時才值得 shrink_to_fit
        r.data.shrink_to_fit();
        std::cout << "shrink_to_fit() 後 capacity=" << r.data.capacity()
                  << "（註：non-binding request，libstdc++ 實測會縮）"
                  << std::endl;
    }

    std::cout << "\n=== 防禦：檔頭是負數 ===" << std::endl;
    {
        std::istringstream iss("-1\n1 2 3\n");
        LoadResult r = loadRecords(iss);
        std::cout << "狀態: " << r.message << std::endl;
        // 示範若沒擋下來會發生什麼：-1 轉成無號是天文數字
        const std::size_t asUnsigned = static_cast<std::size_t>(-1);
        std::cout << "若直接 reserve(-1)，實際會要求 " << asUnsigned
                  << " 個元素" << std::endl;
        std::cout << "→ 遠超過 max_size ("
                  << std::vector<int>().max_size()
                  << ")，會丟 std::length_error" << std::endl;
    }

    std::cout << "\n=== 防禦：檔頭讀取失敗 ===" << std::endl;
    {
        std::istringstream iss("not_a_number\n");
        LoadResult r = loadRecords(iss);
        std::cout << "狀態: " << r.message << std::endl;
        std::cout << "（C++11 起讀取失敗會把變數設為 0；"
                  << "C++98 則維持原值，可能是垃圾值）" << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 11 課：vector 的容量管理：size、capacity、reserve9.cpp" -o demo9

// 註：以下 capacity 數值為 libstdc++ / GCC 15.2 實測，非標準保證
//     （標準只保證 reserve(n) 後 capacity() >= n）。
//     max_size 的值同樣是實作定義（libstdc++ 為 PTRDIFF_MAX / sizeof(T)）。
//     shrink_to_fit 是 non-binding request，capacity 真的下降是實作行為。

// === 預期輸出 ===
// === 正常情況：檔頭宣告 6 筆，實際 6 筆 ===
// 狀態: 讀取完整
// size=6, capacity=6（reserve 一次到位，過程零重新配置）
// 內容: [23 19 31 28 22 25]
//
// === 檔案截斷：宣告 10 筆，實際只有 4 筆 ===
// 狀態: 檔案截斷：實際筆數少於檔頭宣告
// size=4, capacity=10（capacity 依宣告配置，比實際多）
// 後續處理要用 data.size() 而不是檔頭的 10
// shrink_to_fit() 後 capacity=4（註：non-binding request，libstdc++ 實測會縮）
//
// === 防禦：檔頭是負數 ===
// 狀態: 檔頭宣告筆數為負數，拒絕處理
// 若直接 reserve(-1)，實際會要求 18446744073709551615 個元素
// → 遠超過 max_size (2305843009213693951)，會丟 std::length_error
//
// === 防禦：檔頭讀取失敗 ===
// 狀態: 檔頭讀取失敗（格式錯誤或檔案為空）
// （C++11 起讀取失敗會把變數設為 0；C++98 則維持原值，可能是垃圾值）
