// =============================================================================
//  第 19 課：vector 與原始陣列的互操作 4  —  讓 C 函式把資料寫進 vector
// =============================================================================
//
// 【主題資訊 Information】
//   慣用法 : std::vector<T> buf(n);          // 先把 size 撐到 n（元素真的存在）
//            c_fill(buf.data(), n);          // 再把底層指標交給 C 函式寫入
//
//   標頭檔 : <vector>
//   關鍵點 : 必須用「會改變 size() 的方式」建立空間（建構子指定大小或 resize），
//            不能用 reserve()——reserve 只動 capacity，元素並不存在。
//   複雜度 : vector<T> buf(n) 為 O(n)（要值初始化 n 個元素）
//   標準   : data() 為 C++11；本檔其餘語法在 C++11 起皆可用。
//
// 【詳細解釋 Explanation】
//
// 【1. 「輸出參數緩衝區」是 C API 的基本形狀】
//   C 沒有辦法回傳一個「動態大小的陣列」而不牽扯記憶體所有權，
//   所以絕大多數 C API 採用「呼叫端配置、被呼叫端填寫」的分工：
//       void c_fill(int* out, int count);
//   呼叫端負責準備空間與釋放，被呼叫端只管寫值。
//   這個分工的好處是所有權清楚（誰配置誰釋放），vector 剛好完美擔任呼叫端角色：
//   自動配置、自動釋放、例外安全，中途 return 或 throw 都不會漏。
//
// 【2. 為什麼一定要先讓 size() 變成 n】
//   vector 用 size() 定義「哪些元素是存在的」。
//   如果只 reserve(n)，記憶體雖然配好了，但 size() 還是 0，
//   對 vector 而言那塊區域裡「沒有物件」。此時：
//     - range-for 一次都不會跑（它走的是 begin() 到 end()，而 end() 由 size 決定）
//     - 把值寫進那塊區域，是在「尚未建構物件的儲存空間」上寫入，屬未定義行為
//     - 之後任何 push_back 都會從索引 0 開始覆蓋你辛苦填的資料
//   所以正解是 vector<T> buf(n) 或 buf.resize(n)——這兩者都會真的建構 n 個元素，
//   size() 變成 n，之後 data() 給出的區間 [data(), data()+n) 才是合法可寫的。
//   （reserve 的完整對照見本課第 6 個檔案。）
//
// 【3. 值初始化的成本，以及該不該在意】
//   vector<int> buf(5) 會把 5 個 int 值初始化成 0。
//   如果 C 函式馬上就要覆蓋全部內容，這一次歸零其實是白做的工——
//   對 5 個元素無所謂，對 100 MB 的影像緩衝區就是實際可測的成本。
//   標準容器沒有「配置但不初始化」的官方寫法（這正是 P1010 等提案想解決的問題）。
//   實務上的取捨：
//     - 絕大多數情況：直接用 vector<T> buf(n)，可讀性優先，別過早最佳化。
//     - 真的量測到瓶頸：改用 std::unique_ptr<T[]>(new T[n])（同樣會值初始化）、
//       自訂 allocator，或直接 malloc + 自己管理。
//   重點是「先量測再最佳化」，不要為了省一次 memset 就放棄 vector 的安全性。
//
// 【4. 呼叫後 vector 就是資料的擁有者】
//   C 函式寫完就結束了，它不持有任何東西。
//   vector 的 size() 本來就是 n，內容已經被填好，之後就是一個普通的 C++ 物件：
//   可以 range-for、可以傳給 STL 演算法、可以複製、可以 return（會 move）。
//   從這一刻起你就完全回到 C++ 的世界，不必再管指標。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼不用固定大小的 C 陣列就好
//   int buf[5]; c_fill(buf, 5); 當然可以，但只在「大小是編譯期常數且很小」時成立。
//   大小由執行期決定時，C 陣列做不到（VLA 是 C99 特性，不是標準 C++）；
//   大小很大時放在 stack 上會爆堆疊。vector 一律配置在 heap，兩個問題都沒有。
//
// (B) 呼叫端與被呼叫端對「count」必須有一致認知
//   本檔刻意寫 c_fill(buf.data(), static_cast<int>(buf.size()))，
//   而不是寫死 c_fill(buf.data(), 5)。
//   一旦長度來源與緩衝區來源是同一個物件，日後改大小就不可能改漏一邊。
//   寫死常數是這類程式最典型的緩衝區溢位來源。
//
// (C) 這個模式與「兩階段查詢」的差別
//   本檔的前提是「呼叫前就知道要幾筆」。
//   如果連筆數都要問 API 才知道，就要用兩階段查詢（先問大小、再配置、再取資料），
//   那是本課第 5 個檔案的主題。
//
// 【注意事項 Pay Attention】
//   1. 一定要先讓 size() 到位（建構子或 resize），不可以只 reserve。
//   2. 傳給 C 函式的長度要來自 buf.size()，不要另外寫死一個常數。
//   3. C 函式若可能寫超過你給的長度，vector 不會保護你——那是緩衝區溢位，
//      屬未定義行為。長度契約要看清楚 API 文件。
//   4. 呼叫期間不可以對該 vector 做 push_back/resize/insert 等會重新配置的操作，
//      否則 C 函式手上的指標會失效（見本課第 15 個檔案）。
//   5. vector<T> buf(n) 的值初始化成本在大緩衝區時是實際可測的，
//      但請先量測再決定要不要繞過它。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】用 vector 當 C 函式的輸出緩衝區
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 要讓 C 函式把 n 筆資料寫進 vector，該怎麼準備這個 vector？
//     答：用 vector<T> buf(n) 或 buf.resize(n)，讓 size() 真的變成 n，
//         再把 buf.data() 傳給 C 函式。這樣 [data(), data()+n) 是合法且
//         元素已建構的區間，寫入安全，寫完 vector 立刻就是資料的擁有者。
//         不可以用 reserve(n)——它只改 capacity，size() 仍是 0。
//     追問：那 reserve 完全沒用嗎？
//         → 有用，但用在「之後要 push_back n 次」的情境，目的是避免多次重新配置。
//           它不適合當「交給別人直接寫」的緩衝區。
//
// 🔥 Q2. vector<int> buf(1000000) 會做值初始化，這個成本可以避免嗎？
//     答：標準容器沒有提供「配置但不初始化」的官方做法，
//         所以用 vector 就一定會付這次歸零的成本。
//         真的成為瓶頸時，可用自訂 allocator 或改用手動配置繞過，
//         但那會失去部分安全性。實務建議是先量測，多數情況這個成本
//         遠小於後續真正的資料處理。
//     追問：那 buf.reserve(1000000) 不就沒有初始化成本了？
//         → 是沒有，但那塊記憶體上沒有元素，size() 是 0，
//           不能拿去給 C 函式當緩衝區寫入。兩者要解決的問題不同。
//
// ⚠️ 陷阱. 「我把長度寫死成 c_fill(buf.data(), 5)，反正 buf 也是 5 個，有差嗎？」
//     答：有。緩衝區大小與傳出去的長度變成兩個獨立的事實來源，
//         日後有人把 buf 改成 3 個元素卻沒改這一行，C 函式就會往後多寫 2 個 int，
//         造成堆積區緩衝區溢位——那是未定義行為，可能立刻崩潰、
//         也可能默默破壞相鄰的資料結構，之後在完全不相干的地方才爆炸。
//     為什麼會錯：覺得「現在看起來一樣，就是一樣」。
//         但程式碼會被修改，唯一可靠的做法是讓長度只有一個來源：
//         永遠寫 buf.size()，讓兩者在語法上就不可能不同步。
// ═══════════════════════════════════════════════════════════════════════════

#include <cstddef>
#include <iostream>
#include <vector>

// -----------------------------------------------------------------------------
// 模擬一個 C 函數：把結果寫入呼叫端提供的 buffer
// -----------------------------------------------------------------------------
void fill_buffer(int* buffer, int count) {
    for (int i = 0; i < count; ++i) {
        buffer[i] = (i + 1) * 100;
    }
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 977. Squares of a Sorted Array
//   題目：給一個「已排序」的整數陣列 nums（可能含負數），
//         回傳每個元素平方後、依然遞增排序的陣列。
//   為什麼用到本主題：最優解是 O(n) 雙指標——但它必須「從結果的尾巴往前填」，
//         所以不能用 push_back（那只能從頭長）。
//         正確做法正是本檔的模式：先 vector<int> res(n) 把 size 撐滿，
//         再用索引/指標直接往指定位置寫。這就是「預先配置好緩衝區再填寫」
//         在演算法題裡的典型體現。
//   複雜度：時間 O(n)，空間 O(n)（輸出本身）。
// -----------------------------------------------------------------------------
std::vector<int> sortedSquares(const std::vector<int>& nums) {
    const std::size_t n = nums.size();
    std::vector<int> res(n);            // 先撐滿 size，才能任意位置寫入
    if (n == 0) return res;

    std::size_t left = 0, right = n - 1;
    for (std::size_t pos = n; pos-- > 0; ) {   // 由後往前填
        int a = nums[left] * nums[left];
        int b = nums[right] * nums[right];
        if (a > b) {
            res[pos] = a;
            ++left;
        } else {
            res[pos] = b;
            if (right == 0) break;             // 無號數不能減到 -1
            --right;
        }
    }
    return res;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】解碼一段 PCM 音訊：讓 C 解碼器直接寫進 vector
//   情境：音訊解碼器（想像成 libopus / libmp3lame 這類 C 函式庫）的介面是
//         decode_frame(handle, int16_t* out, int sampleCount)，
//         由呼叫端提供輸出緩衝區。一個 frame 的樣本數在協商階段就已確定。
//   為何用本模式：樣本數已知 → 直接 vector<int16_t> pcm(frameSamples)，
//         把 pcm.data() 交給解碼器，解完 vector 就擁有整個 frame，
//         可以直接送給 C++ 的混音/重取樣流程，全程零額外複製、零手動釋放。
// -----------------------------------------------------------------------------
using Sample = short;   // 用 short 代表 16-bit PCM 樣本

void decode_frame(Sample* out, int sampleCount, int toneStep) {  // 假裝是 C 解碼器
    // 用簡單三角波代替真正的解碼運算
    for (int i = 0; i < sampleCount; ++i) {
        int phase = (i * toneStep) % 40;
        int v = (phase < 20) ? phase : (40 - phase);
        out[i] = static_cast<Sample>((v - 10) * 800);
    }
}

std::vector<Sample> decodeOneFrame(int frameSamples, int toneStep) {
    std::vector<Sample> pcm(frameSamples);      // size 到位，元素已存在
    decode_frame(pcm.data(), static_cast<int>(pcm.size()), toneStep);
    return pcm;                                 // 回傳時是 move，不複製
}

int main() {
    // -------------------------------------------------------------------------
    std::cout << "=== 基本模式：先配置，再讓 C 函式填寫 ===\n";
    // -------------------------------------------------------------------------
    const int count = 5;

    std::vector<int> v(count);      // 步驟 1：size 直接到位，5 個元素值初始化為 0
    std::cout << "呼叫前 size = " << v.size()
              << "，capacity = " << v.capacity() << "（實作定義）\n";
    std::cout << "呼叫前內容：";
    for (std::size_t i = 0; i < v.size(); ++i) std::cout << (i ? " " : "") << v[i];
    std::cout << "（值初始化為 0）\n";

    // 步驟 2：長度取自 v.size()，不要寫死常數
    fill_buffer(v.data(), static_cast<int>(v.size()));

    // 步驟 3：vector 已經擁有資料，回到純 C++ 世界
    std::cout << "呼叫後內容：";
    for (std::size_t i = 0; i < v.size(); ++i) std::cout << (i ? " " : "") << v[i];
    std::cout << "\n";
    std::cout << "呼叫後 size = " << v.size() << "（C 函式寫值不會改變 size）\n";

    // -------------------------------------------------------------------------
    std::cout << "\n=== LeetCode 977. Squares of a Sorted Array ===\n";
    // -------------------------------------------------------------------------
    std::vector<int> nums1 = {-4, -1, 0, 3, 10};
    std::vector<int> r1 = sortedSquares(nums1);
    std::cout << "輸入 [-4,-1,0,3,10] → ";
    for (std::size_t i = 0; i < r1.size(); ++i) std::cout << (i ? " " : "") << r1[i];
    std::cout << "\n";

    std::vector<int> nums2 = {-7, -3, 2, 3, 11};
    std::vector<int> r2 = sortedSquares(nums2);
    std::cout << "輸入 [-7,-3,2,3,11] → ";
    for (std::size_t i = 0; i < r2.size(); ++i) std::cout << (i ? " " : "") << r2[i];
    std::cout << "\n";

    std::vector<int> nums3;
    std::cout << "輸入 [] → 結果 size = " << sortedSquares(nums3).size() << "\n";

    // -------------------------------------------------------------------------
    std::cout << "\n=== 日常實務：解碼一個 PCM 音訊 frame ===\n";
    // -------------------------------------------------------------------------
    const int frameSamples = 12;
    std::vector<Sample> pcm = decodeOneFrame(frameSamples, 3);

    std::cout << "frame 樣本數 = " << pcm.size() << "\n";
    std::cout << "樣本內容     : ";
    for (std::size_t i = 0; i < pcm.size(); ++i) std::cout << (i ? " " : "") << pcm[i];
    std::cout << "\n";

    // 回到 C++ 世界後，一般的處理都能直接做
    long long energy = 0;
    for (Sample s : pcm) energy += static_cast<long long>(s) * s;
    std::cout << "能量總和     = " << energy << "\n";
    std::cout << "所需位元組數 = " << pcm.size() * sizeof(Sample)
              << "（sizeof(short) = " << sizeof(Sample) << "，實作定義）\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第\ 19\ 課：vector\ 與原始陣列的互操作4.cpp -o interop4

// === 預期輸出 ===
// === 基本模式：先配置，再讓 C 函式填寫 ===
// 呼叫前 size = 5，capacity = 5（實作定義）
// 呼叫前內容：0 0 0 0 0（值初始化為 0）
// 呼叫後內容：100 200 300 400 500
// 呼叫後 size = 5（C 函式寫值不會改變 size）
//
// === LeetCode 977. Squares of a Sorted Array ===
// 輸入 [-4,-1,0,3,10] → 0 1 9 16 100
// 輸入 [-7,-3,2,3,11] → 4 9 9 49 121
// 輸入 [] → 結果 size = 0
//
// === 日常實務：解碼一個 PCM 音訊 frame ===
// frame 樣本數 = 12
// 樣本內容     : -8000 -5600 -3200 -800 1600 4000 6400 7200 4800 2400 0 -2400
// 能量總和     = 252160000
// 所需位元組數 = 24（sizeof(short) = 2，實作定義）
