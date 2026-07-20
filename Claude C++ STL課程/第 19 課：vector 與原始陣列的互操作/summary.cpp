// =============================================================================
//  第 19 課 總複習：vector 與原始陣列的互操作
//                 —— 「記憶體連續」這個保證換來了什麼
// =============================================================================
//
// 【主題資訊 Information】
//   T*       vector<T>::data() noexcept;         // C++11 加入
//   const T* vector<T>::data() const noexcept;
//   標準保證：vector 的元素在記憶體中連續存放（C++11 明文化，
//             C++03 已透過 DR 補上；C++20 起 vector 屬 contiguous_range）
//   建構：vector(InputIt first, InputIt last)    // 裸指標即合格迭代器
//   標準版本：data() 為 C++11；連續性保證 C++03/C++11；std::span 為 C++20
//   標頭檔：<vector>、<cstring>（memcpy）、<span>（C++20）
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼「連續」是 vector 最重要的一個承諾】
//   標準容器裡只有 vector、array、string 保證元素連續。
//   這個保證帶來三件事：
//     (a) 可以用一根 T* 走完全部元素 → 能無縫接上整個 C 生態系
//     (b) 索引存取是一次位址算術 → operator[] 極快
//     (c) 對 CPU 快取與預取器極友善 → 走訪速度遠勝 list
//   代價則是第 17 課的全部內容：容量不夠時必須整塊重新配置、
//   所有迭代器/指標/引用同時失效。
//   「連續」與「擴容成本」是同一枚硬幣的兩面。
//
// 【2. data() 與 &v[0] 的差別】
//   對非空 vector 兩者結果相同，但 data() 更好：
//   空容器時 &v[0] 必須先解參考 v[0]，而空容器的 v[0] 已經是
//   undefined behavior；data() 對空容器的呼叫本身完全合法
//   （回傳值不保證非空、也不可解參考，但不會觸發 UB）。
//   C++11 之後應該一律用 data()。
//
// 【3. 裸指標就是最強的迭代器】
//   STL 的迭代器是行為契約而非特定類別：
//   能 *it、能 ++it、能 != 比較就算數，而裸指標全部滿足，
//   而且屬於最強的 random access（C++20 起更精確地說是 contiguous）類別。
//   所以 std::sort(arr, arr + n)、vector<int> v(arr, arr + n)
//   都是完全合法的寫法。
//   這正是 STL 能同時服務「C 陣列」與「C++ 容器」的關鍵設計。
//
// 【4. 從陣列建 vector 是複製，不是接管】
//   vector<int> v(arr, arr + n) 會配置自己的記憶體並複製元素，
//   之後兩者完全獨立，所以複製完就可以（也應該）釋放原陣列。
//   C++ 標準沒有提供「讓 vector 託管既有 heap 指標」的辦法，
//   因為 vector 的解構子只會呼叫自己 allocator 的 deallocate。
//   要零複製地持有既有記憶體，用 std::unique_ptr<T[]>（管生命週期）
//   或 std::span<T>（C++20，非擁有的視圖）。
//
// 【5. memcpy 的前提是 trivially copyable】
//   memcpy 逐位元組複製，對 int、double、POD struct 正確；
//   對 std::string 則是災難——只複製了那根指向堆積的指標，
//   兩個物件指向同一塊記憶體，解構時 double free。
//   判斷方式是 std::is_trivially_copyable_v<T>。
//   非 POD 一律用 std::copy——而且對 POD 型別 std::copy
//   內部同樣會走 memmove，不會比較慢。
//
// 【6. 指標失效的規則和迭代器完全一樣】
//   data() 回傳的指標，在任何觸發重新配置的操作
//   （push_back 超過 capacity、insert、resize、reserve、shrink_to_fit）
//   之後就懸空了。
//   最常見的 bug 樣式是「先取 data() 存起來 → 中間 push_back → 再用那根指標」。
//   規則：任何可能改變 capacity 的操作之後，一律重新取 data()。
//
// 【概念補充 Concept Deep Dive】
//   ▸ 為什麼 C++11 才明文保證連續性
//     C++98 其實沒有寫死這件事（雖然所有實作都這麼做），
//     C++03 透過缺陷報告（DR 69）補上，C++11 再加上 data()。
//     這也是為什麼很老的程式碼裡到處是 &v[0] 而看不到 data()。
//   ▸ std::span（C++20）解決的是什麼問題
//     C 風格介面必須傳「指標 + 長度」兩個參數，
//     兩者可能不同步（傳錯長度是經典的緩衝區溢位來源）。
//     span 把它們綁成一個物件，而且能從 vector、原生陣列、
//     std::array 隱式建構，同時保有 size()、begin()/end() 與範圍 for。
//     它是非擁有的視圖——不管生命週期，來源死了 span 就懸空。
//   ▸ 為什麼 struct 直接倒進檔案不可攜
//     位元組序（endianness）、型別大小、以及編譯器決定的
//     欄位填充（padding）三者都相依於平台。
//     同機器自用的快取檔沒問題，跨機器交換必須定義明確格式。
//
// 【注意事項 Pay Attention】
//   1. 用 data() 而非 &v[0]——後者對空容器是 UB。
//   2. data() 的指標與迭代器有完全相同的失效規則。
//   3. memcpy/memset 只能用於 trivially copyable 型別。
//   4. memset 的第二個參數是「byte 值」，只有填 0 才符合直覺。
//   5. 從陣列建 vector 是複製；C++ 無法讓 vector 託管既有指標。
//   6. 陣列傳進函式會退化成指標，sizeof 技巧失效，必須另傳長度。
//   7. 讀檔進 vector 前用 resize()（真的建構元素），不能只用 reserve()。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】vector 與原始陣列的互操作
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. vector 保證元素連續嗎？這個保證有什麼實際價值與代價？
//     答：保證（C++11 明文化，C++03 已透過缺陷報告補上）。
//         價值有三：可以用一根 T* 走完全部元素而無縫接上 C 生態系；
//         索引存取是一次位址算術，極快；對 CPU 快取極友善。
//         代價是容量不夠時必須整塊重新配置、搬移全部元素，
//         而且所有迭代器/指標/引用同時失效。
//     追問：還有哪些標準容器保證連續？
//         → 只有 std::array 與 std::string。
//           deque 是分段配置、list 是節點串接、map 是樹，全都不連續，
//           所以它們也都沒有 data()。
//
// 🔥 Q2. v.data() 和 &v[0] 有什麼差別？該用哪個？
//     答：對非空 vector 結果相同，但應該用 data()。
//         空容器時 &v[0] 必須先解參考 v[0]，
//         而空容器的 v[0] 已經是 undefined behavior；
//         data() 的呼叫本身完全合法（回傳值不保證非空、不可解參考，
//         但不會觸發 UB）。
//     追問：data() 回傳的指標什麼時候失效？
//         → 和迭代器規則完全相同：任何觸發重新配置的操作
//           （push_back 超過 capacity、insert、resize、reserve、
//           shrink_to_fit）之後就懸空。規則是「動過容量就重新取」。
//
// 🔥 Q3. 為什麼 memcpy 可以用在 vector<int> 卻不能用在 vector<std::string>？
//     答：memcpy 逐位元組複製，只對 trivially copyable 型別正確。
//         std::string 內部持有一根指向堆積的指標，
//         memcpy 只複製那根指標的位元、不複製它指向的內容，
//         於是兩個 string 指向同一塊記憶體，解構時各 delete 一次
//         → double free。判斷依據是 std::is_trivially_copyable_v<T>。
//     追問：改用 std::copy 會不會比較慢？
//         → 不會。libstdc++ 對 trivially copyable 型別有特化路徑，
//           std::copy 最終同樣呼叫 memmove。安全不必付出效能代價。
//
// ⚠️ 陷阱. std::vector<int> v = {1,2,3};
//          int* p = v.data();
//          v.push_back(4);
//          p[0] = 99;                 // ← 為什麼這是 undefined behavior？
//     答：push_back 若觸發重新配置，整塊緩衝區會搬到新位址、
//         舊的已被釋放，p 就成了懸空指標，寫入它是 heap-use-after-free
//         （AddressSanitizer 會直接抓到）。
//         關鍵在「若觸發」——容量還夠時它其實仍然有效，
//         所以這個 bug 在小資料量的測試下經常不會顯現。
//     為什麼會錯：以為 data() 取到的是「這個 vector 的固定位址」。
//         它其實只是「目前這塊緩衝區」的位址，
//         而緩衝區會隨著容量變化而整塊搬家。
//         正確心態是把「可能失效」直接當成「已經失效」——
//         標準允許失效，你就不能依賴它沒失效。
// ═══════════════════════════════════════════════════════════════════════════

/*
 * ================================================================
 * 【第 19 課：vector 與原始陣列的互操作】總複習 summary.cpp
 * ================================================================
 * 編譯方式：g++ -std=c++17 -o summary summary.cpp
 *
 * 本課重點：
 * 1. data() —— 取得 vector 底層原始指標，與 C API 介接
 * 2. 從原始陣列建立 vector
 * 3. 將 vector 資料傳給需要 C 陣列的函數
 * 4. 指標算術與 vector 底層的關係
 * 5. 記憶體連續性保證（C++11 起正式保證）
 * 6. 安全注意事項：迭代器失效與指標失效
 * ================================================================
 */

#include <iostream>
#include <vector>
#include <algorithm>
#include <cstring>
#include <string>
using namespace std;

// ================================================================
// 重點一：data() 取得底層指標
// ================================================================
// vector<T>::data() 回傳 T*（或 const T* for const vector）
// 指向 vector 的第一個元素
// C++11 正式保證 vector 的元素在記憶體中是「連續」的
// 這讓 vector 可以直接替代 C 風格陣列

void demoData() {
    cout << "\n【data() 取得底層指標】" << endl;

    vector<int> v = {10, 20, 30, 40, 50};

    int* ptr = v.data();   // 取得底層指標
    // 刻意不印位址本身——位址每次執行都不同，無法作為預期輸出。
    // 改印「兩者是否相同」這個可重現的關係。
    cout << "v.data() == &v[0] : " << (ptr == &v[0] ? "是" : "否") << endl;
    cout << "元素是否連續      : "
         << ((&v[1] == &v[0] + 1 && &v[2] == &v[0] + 2) ? "是" : "否")
         << "（C++11 起標準明文保證）" << endl;

    // 透過指標讀取元素（指標算術）
    cout << "ptr[0]=" << ptr[0] << ", ptr[2]=" << ptr[2] << ", ptr[4]=" << ptr[4] << endl;

    // 透過指標修改元素
    ptr[1] = 200;
    cout << "ptr[1] 改為 200，v[1] = " << v[1] << endl;  // 同步修改

    // const vector：data() 回傳 const T*
    const vector<int> cv = {1, 2, 3};
    const int* cptr = cv.data();
    cout << "const vector data()[0] = " << cptr[0] << endl;
    // cptr[0] = 99;  // 錯誤！const 指標不能修改
}

// ================================================================
// 重點二：傳給 C API 函數
// ================================================================
// 大量 C 函數（如 memcpy、POSIX API、網路 API）需要 T* 指標
// 用 v.data() 可以直接傳入，無需手動複製到 C 陣列

// 模擬 C 函數庫的函數
void c_print_array(const int* arr, int size) {
    printf("C 函數印出: ");
    for (int i = 0; i < size; ++i) {
        printf("%d ", arr[i]);
    }
    printf("\n");
}

void c_double_elements(int* arr, int size) {
    for (int i = 0; i < size; ++i) {
        arr[i] *= 2;
    }
}

void demoCInterface() {
    cout << "\n【與 C API 介接】" << endl;

    vector<int> v = {1, 2, 3, 4, 5};

    // 傳給 const int* 的函數（唯讀）
    c_print_array(v.data(), static_cast<int>(v.size()));

    // 傳給 int* 的函數（可修改）
    c_double_elements(v.data(), static_cast<int>(v.size()));
    cout << "乘以 2 後: ";
    for (int n : v) cout << n << " ";
    cout << endl;
}

// ================================================================
// 重點三：從原始陣列建立 vector
// ================================================================
// 三種方式將 C 陣列轉為 vector：
// 1. 迭代器範圍建構：vector<T>(arr, arr + size)
// 2. assign()
// 3. C++11 起用 begin(arr)/end(arr)

void demoArrayToVector() {
    cout << "\n【從原始陣列建立 vector】" << endl;

    int arr[] = {10, 20, 30, 40, 50};
    int size = sizeof(arr) / sizeof(arr[0]);

    // 方式一：迭代器範圍建構（最常用）
    vector<int> v1(arr, arr + size);
    cout << "方式一（迭代器範圍）: ";
    for (int n : v1) cout << n << " ";
    cout << endl;

    // 方式二：assign()
    vector<int> v2;
    v2.assign(arr, arr + size);
    cout << "方式二（assign）: ";
    for (int n : v2) cout << n << " ";
    cout << endl;

    // 方式三：C++11 std::begin / std::end
    vector<int> v3(begin(arr), end(arr));
    cout << "方式三（std::begin/end）: ";
    for (int n : v3) cout << n << " ";
    cout << endl;
}

// ================================================================
// 重點四：memcpy 與 vector 的配合
// ================================================================
// 由於 vector 記憶體連續，可以使用 memcpy 進行批量複製
// 注意：只適用於 trivially copyable 的類型（int、char 等）
// 對於有建構函數的複雜類型，應使用 std::copy 或迭代器

void demoMemcpy() {
    cout << "\n【memcpy 與 vector 配合】" << endl;

    vector<int> src = {1, 2, 3, 4, 5};
    vector<int> dst(src.size());

    // 用 memcpy 複製（只適用於 trivially copyable 類型）
    memcpy(dst.data(), src.data(), src.size() * sizeof(int));

    cout << "memcpy 複製結果: ";
    for (int n : dst) cout << n << " ";
    cout << endl;

    // 對於複雜類型，應使用 std::copy
    vector<string> words_src = {"hello", "world", "cpp"};
    vector<string> words_dst(words_src.size());
    copy(words_src.begin(), words_src.end(), words_dst.begin());
    cout << "std::copy 結果: ";
    for (const string& s : words_dst) cout << s << " ";
    cout << endl;
}

// ================================================================
// 重點五：span（C++20）—— 更安全的陣列視圖
// ================================================================
// std::span 提供對連續記憶體的非擁有視圖（view）
// 可以接受 vector、原生陣列、data()+size 等
// 避免傳遞「指標+大小」兩個分開的參數

// 使用傳統方式（指標 + 大小）
void printOldStyle(const int* data, size_t size) {
    for (size_t i = 0; i < size; ++i) cout << data[i] << " ";
    cout << endl;
}

void demoSpanLike() {
    cout << "\n【指標+大小 vs span 風格】" << endl;

    vector<int> v = {10, 20, 30, 40, 50};

    // 傳統：指標 + 大小（容易傳錯大小）
    printOldStyle(v.data(), v.size());

    // 現代（C++20 span）：單一參數包含指標和大小
    // span<int> s(v.data(), v.size());  // C++20 需要 #include <span>
    cout << "（C++20 std::span 提供更安全的介面）" << endl;
}

// ================================================================
// 重點六：指標失效警告
// ================================================================
// 重要！當 vector 重新分配記憶體時，data() 返回的指標會失效
// 任何導致 capacity 增加的操作都可能使指標失效：
//   - push_back（超過 capacity）
//   - insert
//   - resize（增大）
//   - reserve（增大）

void demoPointerInvalidation() {
    cout << "\n【指標失效警告】" << endl;

    vector<int> v = {1, 2, 3};
    int* ptr = v.data();
    cout << "v.data() 取得指標，ptr[0] = " << ptr[0] << endl;

    // 警告！push_back 可能導致重分配，使 ptr 失效
    v.push_back(4);  // 可能觸發重分配！

    // ptr 此時可能已失效！
    // cout << ptr[0];  // 危險的未定義行為！

    // 正確做法：push_back 後重新取指標
    int* new_ptr = v.data();
    cout << "重新取指標後 new_ptr[0] = " << new_ptr[0] << endl;

    cout << "規則：任何可能改變 capacity 的操作後，都要重新取 data()" << endl;
}

// ================================================================
// 【LeetCode 實戰範例】LeetCode 88. Merge Sorted Array
// ================================================================
// 題目：nums1 的長度是 m + n，前 m 個是有效元素、後 n 個是預留空位；
//       把已排序的 nums2（n 個）併進 nums1，結果仍需保持排序，且必須原地完成。
// 為什麼用到本主題：這題的精髓正是「連續記憶體 + 指標算術」——
//       因為 vector 保證元素連續，才能用「從後往前填」的技巧
//       在不配置任何額外空間的前提下完成合併。
//       若容器不連續（例如 list），這個解法根本寫不出來。
// 關鍵洞察：從前往後填會覆蓋掉還沒讀的資料；從後往前填則永遠
//       寫在「已經讀過」的位置上，因此絕對安全。
void merge(vector<int>& nums1, int m, vector<int>& nums2, int n) {
    int i = m - 1;          // nums1 有效區的最後一個
    int j = n - 1;          // nums2 的最後一個
    int k = m + n - 1;      // 要填入的位置（從最尾端開始）

    while (j >= 0) {
        if (i >= 0 && nums1[static_cast<size_t>(i)] > nums2[static_cast<size_t>(j)]) {
            nums1[static_cast<size_t>(k--)] = nums1[static_cast<size_t>(i--)];
        } else {
            nums1[static_cast<size_t>(k--)] = nums2[static_cast<size_t>(j--)];
        }
    }
    // nums2 用完就結束——nums1 剩下的部分本來就已經在正確位置上
}

// ================================================================
// 【日常實務範例】把 vector 直接當成網路封包的接收緩衝區
// ================================================================
// 情境：從 socket 讀資料時，底層 API（recv / read）要的是
//       「一根 char* + 一個長度」，而且一次不保證讀滿，
//       必須反覆讀取直到湊滿預期長度。
// 為什麼用本主題：這是 data() 最典型的正當用途——
//       vector 同時滿足「大小執行期決定」與「記憶體連續」兩個條件，
//       是唯一能直接當 I/O 緩衝區的動態容器。
// 重點示範：
//   (a) 必須用 resize() 而非 reserve()（要真的建構元素）；
//   (b) 每次讀取後 data() + offset 繼續往後填；
//   (c) 中途若 vector 被 resize 放大，先前存下的指標就失效了——
//       所以每一輪都重新取 data()。

// 模擬一個「一次只讀得到一小段」的 C 風格 API
size_t fake_recv(char* buf, size_t want, size_t& fakeSourcePos,
                 const vector<char>& fakeSource) {
    size_t remain = fakeSource.size() - fakeSourcePos;
    size_t give = (remain < want) ? remain : want;
    if (give > 7) give = 7;                       // 刻意每次最多給 7 bytes
    for (size_t i = 0; i < give; ++i) {
        buf[i] = fakeSource[fakeSourcePos + i];
    }
    fakeSourcePos += give;
    return give;
}

vector<char> receiveExactly(size_t totalLen, const vector<char>& fakeSource) {
    vector<char> buf;
    buf.resize(totalLen);          // 關鍵：resize 而非 reserve

    size_t filled = 0;
    size_t srcPos = 0;
    int    rounds = 0;
    while (filled < totalLen) {
        // 每一輪都重新取 data()——若中途容器被放大，舊指標就失效了
        size_t got = fake_recv(buf.data() + filled, totalLen - filled, srcPos, fakeSource);
        if (got == 0) break;       // 對端關閉
        filled += got;
        ++rounds;
    }
    buf.resize(filled);            // 若沒收滿，縮到實際長度
    cout << "  共 " << rounds << " 次讀取才湊滿 " << filled << " bytes" << endl;
    return buf;
}

void demoLeetCodeAndPractice() {
    cout << "\n【LeetCode 88. Merge Sorted Array】" << endl;
    vector<int> nums1 = {1, 2, 3, 0, 0, 0};
    vector<int> nums2 = {2, 5, 6};
    merge(nums1, 3, nums2, 3);
    cout << "nums1={1,2,3,_,_,_}, nums2={2,5,6} → ";
    for (int x : nums1) cout << x << " ";
    cout << endl;

    vector<int> a = {0};
    vector<int> b = {1};
    merge(a, 0, b, 1);
    cout << "邊界 nums1={_}, m=0, nums2={1} → ";
    for (int x : a) cout << x << " ";
    cout << endl;
    cout << "（能原地合併，靠的正是 vector 的連續記憶體保證）" << endl;

    cout << "\n【日常實務：把 vector 當網路接收緩衝區】" << endl;
    string message = "HTTP/1.1 200 OK\r\nContent-Length: 5\r\n\r\nhello";
    vector<char> source(message.begin(), message.end());
    cout << "  預期接收 " << source.size() << " bytes（模擬每次最多給 7 bytes）" << endl;

    vector<char> received = receiveExactly(source.size(), source);
    cout << "  實際收到 " << received.size() << " bytes" << endl;
    // 內容含 CR/LF，直接印會讓輸出混入控制字元；改以逸出形式呈現
    string shown;
    for (char c : received) {
        if      (c == '\r') shown += "\\r";
        else if (c == '\n') shown += "\\n";
        else                shown += c;
    }
    cout << "  內容(逸出): [" << shown << "]" << endl;
    cout << "  與來源相符: " << (received == source ? "是" : "否") << endl;
}

int main() {
    cout << "=============================================" << endl;
    cout << "   第 19 課：vector 與原始陣列的互操作" << endl;
    cout << "=============================================" << endl;

    demoData();
    demoCInterface();
    demoArrayToVector();
    demoMemcpy();
    demoSpanLike();
    demoPointerInvalidation();
    demoLeetCodeAndPractice();

    cout << "\n==============================================" << endl;
    cout << " 重點：vector 記憶體連續，可透過 data() 與 C API 互操作" << endl;
    cout << " 警告：重分配後指標失效，操作後需重新取 data()" << endl;
    cout << "==============================================" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra summary.cpp -o summary19
//
// 【關於下方預期輸出的但書】
//   ▸ 本檔輸出完全是確定性的，每次執行都應完全相同。
//   ▸ 第一段刻意不印記憶體位址（每次執行都不同），
//     改印「v.data() == &v[0]」與「元素是否連續」這兩個可重現的關係。
//   ▸ 最後一段的網路緩衝區內容含 CR/LF，為避免在輸出中混入控制字元，
//     程式已將其轉為 \r / \n 的逸出形式再印出。

// === 預期輸出 ===
// =============================================
//    第 19 課：vector 與原始陣列的互操作
// =============================================
//
// 【data() 取得底層指標】
// v.data() == &v[0] : 是
// 元素是否連續      : 是（C++11 起標準明文保證）
// ptr[0]=10, ptr[2]=30, ptr[4]=50
// ptr[1] 改為 200，v[1] = 200
// const vector data()[0] = 1
//
// 【與 C API 介接】
// C 函數印出: 1 2 3 4 5
// 乘以 2 後: 2 4 6 8 10
//
// 【從原始陣列建立 vector】
// 方式一（迭代器範圍）: 10 20 30 40 50
// 方式二（assign）: 10 20 30 40 50
// 方式三（std::begin/end）: 10 20 30 40 50
//
// 【memcpy 與 vector 配合】
// memcpy 複製結果: 1 2 3 4 5
// std::copy 結果: hello world cpp
//
// 【指標+大小 vs span 風格】
// 10 20 30 40 50
// （C++20 std::span 提供更安全的介面）
//
// 【指標失效警告】
// v.data() 取得指標，ptr[0] = 1
// 重新取指標後 new_ptr[0] = 1
// 規則：任何可能改變 capacity 的操作後，都要重新取 data()
//
// 【LeetCode 88. Merge Sorted Array】
// nums1={1,2,3,_,_,_}, nums2={2,5,6} → 1 2 2 3 5 6
// 邊界 nums1={_}, m=0, nums2={1} → 1
// （能原地合併，靠的正是 vector 的連續記憶體保證）
//
// 【日常實務：把 vector 當網路接收緩衝區】
//   預期接收 43 bytes（模擬每次最多給 7 bytes）
//   共 7 次讀取才湊滿 43 bytes
//   實際收到 43 bytes
//   內容(逸出): [HTTP/1.1 200 OK\r\nContent-Length: 5\r\n\r\nhello]
//   與來源相符: 是
//
// ==============================================
//  重點：vector 記憶體連續，可透過 data() 與 C API 互操作
//  警告：重分配後指標失效，操作後需重新取 data()
// ==============================================
