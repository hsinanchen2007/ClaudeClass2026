/*
 * ================================================================
 * 【第 12 課：vector 元素存取：operator[]、at、front、back】
 * 總複習 summary.cpp
 * ================================================================
 * 編譯方式：g++ -std=c++17 -o summary summary.cpp
 *
 * 本課重點：
 * 1. operator[] —— 不做邊界檢查，高效存取
 * 2. at()        —— 帶邊界檢查，安全存取（拋出 out_of_range）
 * 3. front()     —— 存取第一個元素
 * 4. back()      —— 存取最後一個元素
 * 5. data()      —— 取得底層原始指標
 * 6. 讀取 vs 修改元素的方式
 * 7. const vector 的存取行為
 * ================================================================
 */

// =============================================================================
//  第 12 課 總複習  —  vector 元素存取：operator[]、at、front、back、data
// =============================================================================
//
// 【主題資訊 Information】
//   五種存取方式（皆 <vector>）：
//     reference operator[](size_type pos);   O(1)，無檢查，越界是 UB
//     reference at(size_type pos);           O(1)，有檢查，越界丟 out_of_range
//     reference front();                     O(1)，等同 v[0]，空容器是 UB
//     reference back();                      O(1)，等同 v[size()-1]，空容器是 UB
//     T*        data();                      O(1)，回傳底層陣列首位址
//   標準版本：前四者 C++98；data() 是 **C++11**。
//   每一個都有 const 與非 const 兩個重載：
//     非 const 版回傳 T&（可讀可寫）
//     const   版回傳 const T&（唯讀）
//   所以 `v.front() = 999;` 能改值，而對 const vector 就會編譯失敗。
//
// 【詳細解釋 Explanation】
//
// 【1. 五個介面對應五種不同的「你知道什麼」】
//     operator[] —— 你知道索引一定合法。不檢查是你付得起的信任。
//     at()       —— 你不確定索引合法，而且打算處理這個錯誤。
//     front()    —— 你要的是「第一個」這個語意，不是「索引 0」這個算式。
//     back()     —— 你要的是「最後一個」，不必自己寫 size()-1（那還會無號環繞）。
//     data()     —— 你要把整塊資料交給一個不懂 C++ 容器的 API。
//   選對介面，程式碼會自己說明意圖；選錯了，就得靠註解解釋。
//
// 【2. front()/back() 不只是語法糖】
//   `v[v.size() - 1]` 有一個真實的地雷：v 為空時 size()-1 會無號環繞成
//   SIZE_MAX，於是變成 v[18446744073709551615]，越界得非常徹底。
//   `v.back()` 對空容器同樣是未定義行為，但它至少沒有那個「先算出
//   一個天文數字」的中間步驟——用 AddressSanitizer 或 debug 模式
//   比較容易抓到正確的位置。
//   更重要的是可讀性：back() 明說「我要最後一個」，
//   size()-1 則要讀者自己在腦中還原意圖。
//
// 【3. 這些函式回傳「參考」帶來的兩件事】
//   好處：可以直接寫（v.front() = 999），也不會複製大型元素。
//   風險：參考的壽命綁在容器的記憶體上。任何造成重新配置的操作
//         （push_back / insert / reserve / resize）都可能讓它懸空。
//   所以「取得參考 → 改動容器 → 使用參考」是 vector 最經典的錯誤序列。
//   詳見同課第 11 檔。
//
// 【4. data() 的存在意義：與 C 世界接軌】
//   很多既有 API 只吃 `const T*` + 長度：memcpy、write()、
//   OpenGL 的 glBufferData、加密函式庫的 hash 函式。
//   data() 讓 vector 可以零成本地餵進去，而不必先複製到一個 C 陣列。
//   兩個細節：
//     * 空 vector 呼叫 data() 是合法的（可能回傳 nullptr），
//       但 &v[0] 是未定義行為——所以一律用 data()。
//     * C++11 起 vector 保證元素連續存放，data() 才有意義。
//       （std::deque 就沒有 data()，因為它的記憶體是分段的。）
//
// 【概念補充 Concept Deep Dive】
//
// (A) vector<bool> 是這一切的例外
//     std::vector<bool> 是一個「打包成位元」的特化版本，一個 bool 只佔 1 bit。
//     後果是：
//       * operator[] 回傳的【不是 bool&】，而是一個代理物件（proxy）
//       * 沒有 data() 成員（位元不是可定址的物件）
//       * `bool& r = v[0];` 編譯失敗；`auto r = v[0];` 拿到的是代理不是 bool
//     這被公認為標準函式庫的設計失誤。需要真正的 bool 陣列時，
//     用 std::vector<char> 或 std::deque<bool>。
//
// (B) 為什麼 at() 丟的是 out_of_range 而不是 assert
//     assert 在 NDEBUG 下會被整個移除，release build 就失去保護；
//     而且 assert 直接呼叫 abort()，呼叫端沒有機會處理。
//     例外則可以被上層轉譯成有意義的回應（HTTP 400、錯誤訊息、重試）。
//     這對應了「越界代表什麼」的區別：at() 假設越界是【可處理的錯誤】，
//     assert 假設越界是【不該發生的 bug】。
//
// (C) 這些存取函式全部都是 O(1)，但不代表成本相同
//     複雜度相同，常數項不同：at() 多一次比較與一條 throw 路徑。
//     不過本課第 8 檔實測顯示，在 -O2 且索引可被編譯器分析的情況下，
//     那個檢查會被完全刪除，兩者耗時相同。
//     結論：不要憑「多寫了一個檢查」就斷定較慢，要在 release 設定下量測。
//
// 【注意事項 Pay Attention】
//   1. operator[]、front()、back()、data() 全都不檢查。
//      對空 vector 呼叫 front()/back() 是未定義行為。
//   2. `v[v.size() - 1]` 對空 vector 會無號環繞成天文數字，
//      比 back() 更危險。要取最後一個就用 back()。
//   3. 這些函式回傳參考，參考會隨容器重新配置而失效。
//   4. 空 vector 請用 data()，不要用 &v[0]（後者是 UB）。
//   5. vector<bool> 是特化版本，operator[] 回傳代理物件、沒有 data()。
//      需要真正的 bool 陣列請改用 vector<char>。
//   6. const vector 的存取一律回傳 const 參考，不能寫入。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】vector 的元素存取
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. v.back() 和 v[v.size() - 1] 有什麼差別？
//     答：對非空 vector 兩者等價。差別在空 vector：
//         v.back() 是未定義行為；
//         v[v.size()-1] 則會先讓 size()-1 無號環繞成 SIZE_MAX
//         （18446744073709551615），再拿它去索引——同樣是 UB，
//         但越界的距離更誇張，也更難從崩潰現場看出原因。
//         此外 back() 直接表達「最後一個」的意圖，可讀性較好。
//     追問：那要怎麼安全地取最後一個元素？
//         → 先判斷 if (!v.empty())。這兩個函式都沒有內建檢查，
//           標準也沒有提供「檢查版的 back()」。
//
// 🔥 Q2. 為什麼 std::vector<bool> 不能寫 `bool& r = v[0];`？
//     答：因為 vector<bool> 是標準明訂的特化版本，把每個 bool
//         打包成 1 bit。一個 bit 不是可定址的物件，無法回傳 bool&，
//         所以 operator[] 回傳的是一個代理物件（proxy class），
//         它重載了 operator bool 與 operator=。
//         同理它也沒有 data() 成員。
//     追問：那實務上該怎麼辦？
//         → 需要真正的 bool 陣列時用 std::vector<char> 或
//           std::deque<bool>；只需要位元集合且大小固定時用 std::bitset。
//           vector<bool> 這個特化普遍被認為是標準的設計失誤。
//
// ⚠️ 陷阱. 「front()、back()、at() 都是成員函式，所以它們都會做檢查，
//         比 operator[] 安全」——這個推論錯在哪？
//     答：只有 at() 會檢查。front()、back()、data() 全部【不檢查】，
//         對空 vector 呼叫 front() 或 back() 是未定義行為，
//         和 v[0] 越界是完全相同性質的錯誤。
//         標準沒有提供「有檢查版的 front()/back()」——
//         要安全就得自己先 if (!v.empty())。
//     為什麼會錯：把「是成員函式、名字比較長」當成「比較安全」的暗示。
//         實際上 STL 的安全性完全取決於每個函式各自的契約，
//         而 vector 的預設立場是「不檢查、由呼叫端負責」，
//         at() 是那個唯一的例外，不是通則。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <string>
#include <numeric>
#include <cstring>     // std::memcpy
#include <stdexcept>
using namespace std;

// ================================================================
// 重點一：operator[] —— 無邊界檢查，最快
// ================================================================
// 語法：v[index]
// 優點：無額外開銷，效能最佳
// 缺點：越界行為是「未定義行為（Undefined Behavior）」，不會拋出例外
// 適用：在確保索引合法的情況下使用

void demoSubscript() {
    cout << "\n【operator[] 基本存取】" << endl;

    vector<int> v = {10, 20, 30, 40, 50};

    // 讀取元素
    cout << "v[0] = " << v[0] << endl;   // 10
    cout << "v[2] = " << v[2] << endl;   // 30
    cout << "v[4] = " << v[4] << endl;   // 50

    // 修改元素
    v[1] = 200;
    v[3] = 400;
    cout << "修改後: ";
    for (int n : v) cout << n << " ";
    cout << endl;

    // 危險！越界 —— 未定義行為（不要這樣做）
    // v[10] = 999;  // 可能崩潰、可能靜默損毀記憶體
}

// ================================================================
// 重點二：at() —— 帶邊界檢查，安全
// ================================================================
// 語法：v.at(index)
// 若 index >= size()，拋出 std::out_of_range 例外
// 適用：不確定索引是否合法時，用 try-catch 捕獲例外

void demoAt() {
    cout << "\n【at() 安全存取】" << endl;

    vector<int> v = {10, 20, 30, 40, 50};

    // 正常存取
    cout << "v.at(0) = " << v.at(0) << endl;
    cout << "v.at(4) = " << v.at(4) << endl;

    // 修改
    v.at(2) = 300;
    cout << "v.at(2) 修改為 300" << endl;

    // 邊界檢查：捕獲越界例外
    try {
        cout << v.at(100) << endl;  // 越界！
    } catch (const out_of_range& e) {
        cout << "捕獲例外：" << e.what() << endl;
    }
}

// ================================================================
// 重點三：front() —— 存取第一個元素
// ================================================================
// 語法：v.front()
// 等同於 v[0]，但語意更清晰
// 警告：若 vector 為空，行為未定義

void demoFront() {
    cout << "\n【front() 存取第一個元素】" << endl;

    vector<int> v = {10, 20, 30, 40, 50};

    // 讀取
    cout << "front() = " << v.front() << endl;  // 10

    // 修改
    v.front() = 999;  // front() 回傳參考，可以修改！
    cout << "修改 front 後，v[0] = " << v[0] << endl;  // 999

    // 安全做法：先檢查是否為空
    if (!v.empty()) {
        cout << "第一個元素: " << v.front() << endl;
    }
}

// ================================================================
// 重點四：back() —— 存取最後一個元素
// ================================================================
// 語法：v.back()
// 等同於 v[v.size() - 1]，但語意更清晰
// 警告：若 vector 為空，行為未定義

void demoBack() {
    cout << "\n【back() 存取最後一個元素】" << endl;

    vector<int> v = {10, 20, 30, 40, 50};

    // 讀取
    cout << "back() = " << v.back() << endl;  // 50

    // 修改
    v.back() = 888;  // back() 回傳參考，可以修改！
    cout << "修改 back 後，最後元素 = " << v.back() << endl;  // 888

    // pop_back 前先用 back() 讀取
    cout << "pop 前最後元素: " << v.back() << endl;
    v.pop_back();
    cout << "pop 後最後元素: " << v.back() << endl;
}

// ================================================================
// 重點五：data() —— 取得底層原始指標
// ================================================================
// 語法：v.data()
// 回傳指向 vector 內部陣列首元素的指標（T* 或 const T*）
// 主要用途：與需要原始陣列指標的 C 函數介接

void c_style_function(const int* arr, int size) {
    cout << "C 函數收到: ";
    for (int i = 0; i < size; ++i) cout << arr[i] << " ";
    cout << endl;
}

void demoData() {
    cout << "\n【data() 取得底層指標】" << endl;

    vector<int> v = {10, 20, 30, 40, 50};

    // 取得指標
    int* ptr = v.data();
    cout << "ptr[0] = " << ptr[0] << endl;
    cout << "ptr[4] = " << ptr[4] << endl;

    // 透過指標修改
    ptr[2] = 300;
    cout << "透過 ptr 修改後 v[2] = " << v[2] << endl;

    // 傳給 C 函數
    c_style_function(v.data(), static_cast<int>(v.size()));
}

// ================================================================
// 重點六：const vector 的存取
// ================================================================
// const vector 中，at()、[]、front()、back()、data()
// 都只能讀取，不能修改（回傳 const 參考）

void demoConstVector() {
    cout << "\n【const vector 的存取】" << endl;

    const vector<int> cv = {1, 2, 3, 4, 5};

    // 這些都可以讀取
    cout << "cv[2] = " << cv[2] << endl;
    cout << "cv.at(2) = " << cv.at(2) << endl;
    cout << "cv.front() = " << cv.front() << endl;
    cout << "cv.back() = " << cv.back() << endl;

    // const T* 指標（不能修改元素）
    const int* cptr = cv.data();
    cout << "cptr[0] = " << cptr[0] << endl;

    // 這些都不能修改：
    // cv[2] = 999;      // 錯誤！const 不允許修改
    // cv.front() = 0;   // 錯誤！
}

// ================================================================
// 重點七：存取方式的選擇建議
// ================================================================
//
// ┌──────────────┬─────────────────┬─────────────────────────────┐
// │ 存取方式      │ 邊界檢查         │ 適用場景                    │
// ├──────────────┼─────────────────┼─────────────────────────────┤
// │ v[i]         │ 無（UB 越界）   │ 確定索引合法，追求效能      │
// │ v.at(i)      │ 有（拋例外）    │ 不確定索引是否合法          │
// │ v.front()    │ 無              │ 存取第一個元素（語意清晰）  │
// │ v.back()     │ 無              │ 存取最後一個元素（語意清晰）│
// │ v.data()     │ 無              │ 與 C API 介接               │
// └──────────────┴─────────────────┴─────────────────────────────┘

// ================================================================
// 重點八：vector<bool> —— 這一切的例外
// ================================================================
// vector<bool> 是標準明訂的特化：每個 bool 被打包成 1 bit。
// 代價是它不再是一個「正常的容器」：
//   * operator[] 回傳代理物件，不是 bool&
//   * 沒有 data() 成員
//   * bool& r = v[0]; 編譯失敗
void demoVectorBool() {
    cout << "\n【vector<bool> 的特殊性】" << endl;

    vector<bool> vb = {true, false, true, true};
    vector<char> vc = {1, 0, 1, 1};        // 對照組：正常的容器

    cout << "vector<bool> 內容: ";
    for (size_t i = 0; i < vb.size(); ++i) cout << vb[i] << " ";
    cout << endl;

    // 讀寫都正常，只是型別不是你以為的那個
    vb[1] = true;
    cout << "vb[1] 改成 true 後: " << vb[1] << endl;

    // bool& r = vb[0];        // 編譯失敗：operator[] 回傳的是代理物件
    bool copied = vb[0];       // 這樣可以：代理物件轉換成 bool
    cout << "複製出來的值: " << copied << endl;

    // vb.data();              // 編譯失敗：vector<bool> 沒有 data()
    cout << "vector<char> 有 data(): " << (vc.data() != nullptr ? "是" : "否") << endl;
    cout << "vector<bool> 沒有 data() 成員（位元不是可定址的物件）" << endl;

    cout << "空間效率的代價：8 個 bool 只佔約 1 byte，但失去了容器的一般性" << endl;
    cout << "→ 需要真正的 bool 陣列請用 vector<char>；固定大小的位元集合用 bitset" << endl;
}

// ================================================================
// 重點九：空容器 —— 哪些會檢查、哪些不會
// ================================================================
void demoEmptyContainer() {
    cout << "\n【空 vector 的存取行為】" << endl;

    vector<int> empty;

    cout << "empty.size()     = " << empty.size() << endl;
    cout << "empty.empty()    = " << boolalpha << empty.empty() << endl;

    // 只有 at() 會檢查
    try {
        cout << "empty.at(0)      -> ";
        cout << empty.at(0) << endl;
    } catch (const out_of_range& e) {
        cout << "丟出 out_of_range（唯一會檢查的）" << endl;
    }

    cout << "empty.front()    -> 未定義行為（不檢查）" << endl;
    cout << "empty.back()     -> 未定義行為（不檢查）" << endl;
    cout << "empty[0]         -> 未定義行為（不檢查）" << endl;
    cout << "empty[size()-1]  -> size()-1 先環繞成 "
         << (empty.size() - 1) << "，再越界（更糟）" << endl;
    cout << "empty.data()     -> 合法！回傳值可能是 nullptr: "
         << (empty.data() == nullptr) << endl;

    cout << "\n安全的存取方式：一律先檢查 empty()" << endl;
    if (!empty.empty()) {
        cout << "  front = " << empty.front() << endl;
    } else {
        cout << "  容器是空的，跳過存取（正確做法）" << endl;
    }
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 20. Valid Parentheses
//   題目：給一個只含 "()[]{}" 的字串，判斷括號是否正確配對。
//   為什麼用到本主題：這題的標準解法是用堆疊，而 C++ 最務實的堆疊
//     就是 std::vector —— push_back 進、back() 看、pop_back() 出。
//     它同時用上本課三個重點：
//       ① back() 取最後一個元素（語意清楚，勝過 v[v.size()-1]）
//       ② back() 對空容器是 UB，所以每次都必須先檢查 empty()
//       ③ 全程不需要 at()，因為 empty() 檢查已經保證了合法性
//     這正是「用對介面 + 自己守住前置條件」的實際樣貌。
//   複雜度：O(N) 時間、O(N) 空間。
// -----------------------------------------------------------------------------
bool isValidParentheses(const string& s) {
    vector<char> stk;
    stk.reserve(s.size());          // 上界已知，一次配置

    for (char c : s) {
        if (c == '(' || c == '[' || c == '{') {
            stk.push_back(c);
        } else {
            // 關鍵：back() 不檢查空容器，所以呼叫前必須自己確認
            if (stk.empty()) return false;
            char top = stk.back();
            if ((c == ')' && top != '(') ||
                (c == ']' && top != '[') ||
                (c == '}' && top != '{')) {
                return false;
            }
            stk.pop_back();
        }
    }
    return stk.empty();
}

// -----------------------------------------------------------------------------
// 【日常實務範例】用 data() 把 vector 交給只吃 C 陣列的 API
//   情境：計算一段資料的 checksum，而那個 checksum 函式是既有的 C 程式庫
//         （或系統呼叫、加密函式庫），簽章只吃 const unsigned char* + 長度。
//   為什麼用到本主題：這是 data() 唯一也是最重要的用途——
//     讓 vector 能零成本地與 C 世界接軌，不必先複製到一個 C 陣列。
//   兩個要點：
//     ① 一律用 data() 不用 &v[0]：空 vector 時前者合法、後者是 UB。
//     ② 傳出去的指標只在「容器沒有被改動」的期間有效，
//        C API 呼叫期間絕不能對容器做 push_back。
// -----------------------------------------------------------------------------

// 模擬一個既有的 C 風格 API（只吃原始指標與長度）
extern "C" unsigned long simple_checksum(const unsigned char* buf, size_t len) {
    unsigned long sum = 0;
    for (size_t i = 0; i < len; ++i) {
        sum = (sum * 31 + buf[i]) & 0xFFFFFFFFUL;
    }
    return sum;
}

unsigned long checksumOf(const vector<unsigned char>& payload) {
    // 空 vector 時 data() 可能回 nullptr，但長度也是 0，
    // 標準要求 C API 在 len==0 時不得解參考指標——這裡先擋掉比較保險。
    if (payload.empty()) return 0;
    return simple_checksum(payload.data(), payload.size());
}

int main() {
    cout << "=============================================" << endl;
    cout << "   第 12 課：vector 元素存取方式總複習" << endl;
    cout << "=============================================" << endl;

    demoSubscript();
    demoAt();
    demoFront();
    demoBack();
    demoData();
    demoConstVector();
    demoVectorBool();
    demoEmptyContainer();

    cout << "\n【LeetCode 20. Valid Parentheses】" << endl;
    {
        const char* cases[] = {"()", "()[]{}", "(]", "([)]", "{[]}", "", "((("};
        for (const char* c : cases) {
            cout << "  \"" << c << "\" -> " << boolalpha
                 << isValidParentheses(c) << endl;
        }
        cout << "  關鍵：每次呼叫 back() 前都先檢查 empty()——" << endl;
        cout << "        back() 自己不做這件事，對空容器是 UB。" << endl;
    }

    cout << "\n【日常實務：用 data() 對接 C API】" << endl;
    {
        string text = "GET /index.html HTTP/1.1";
        vector<unsigned char> payload(text.begin(), text.end());

        cout << "  資料長度: " << payload.size() << " bytes" << endl;
        cout << "  checksum: " << checksumOf(payload) << endl;

        // 空容器也能安全處理
        vector<unsigned char> nothing;
        cout << "  空資料的 checksum: " << checksumOf(nothing)
             << "（data() 可能是 nullptr，但我們先擋掉了 len==0）" << endl;

        // 也可以反過來：用 memcpy 從 C 緩衝區填進 vector
        const char raw[] = "binary\0data";      // 含內嵌的 '\0'
        vector<unsigned char> fromC(sizeof(raw) - 1);
        memcpy(fromC.data(), raw, fromC.size());
        cout << "  從 C 陣列 memcpy 進 vector，size=" << fromC.size()
             << "（含內嵌 '\\0'，這是 C 字串做不到的）" << endl;
        cout << "  checksum: " << checksumOf(fromC) << endl;

        cout << "  → 一律用 data() 而非 &v[0]：空 vector 時後者是 UB。" << endl;
    }

    cout << "\n==============================================" << endl;
    cout << " 建議：一般用 []；不確定索引合法性時用 at()" << endl;
    cout << " front()/back() 語意清晰，優先於 v[0]/v[size-1]" << endl;
    cout << "==============================================" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra summary.cpp -o summary

// === 預期輸出 ===
// =============================================
//    第 12 課：vector 元素存取方式總複習
// =============================================
//
// 【operator[] 基本存取】
// v[0] = 10
// v[2] = 30
// v[4] = 50
// 修改後: 10 200 30 400 50
//
// 【at() 安全存取】
// v.at(0) = 10
// v.at(4) = 50
// v.at(2) 修改為 300
// 捕獲例外：vector::_M_range_check: __n (which is 100) >= this->size() (which is 5)
//
// 【front() 存取第一個元素】
// front() = 10
// 修改 front 後，v[0] = 999
// 第一個元素: 999
//
// 【back() 存取最後一個元素】
// back() = 50
// 修改 back 後，最後元素 = 888
// pop 前最後元素: 888
// pop 後最後元素: 40
//
// 【data() 取得底層指標】
// ptr[0] = 10
// ptr[4] = 50
// 透過 ptr 修改後 v[2] = 300
// C 函數收到: 10 20 300 40 50
//
// 【const vector 的存取】
// cv[2] = 3
// cv.at(2) = 3
// cv.front() = 1
// cv.back() = 5
// cptr[0] = 1
//
// 【vector<bool> 的特殊性】
// vector<bool> 內容: 1 0 1 1
// vb[1] 改成 true 後: 1
// 複製出來的值: 1
// vector<char> 有 data(): 是
// vector<bool> 沒有 data() 成員（位元不是可定址的物件）
// 空間效率的代價：8 個 bool 只佔約 1 byte，但失去了容器的一般性
// → 需要真正的 bool 陣列請用 vector<char>；固定大小的位元集合用 bitset
//
// 【空 vector 的存取行為】
// empty.size()     = 0
// empty.empty()    = true
// empty.at(0)      -> 丟出 out_of_range（唯一會檢查的）
// empty.front()    -> 未定義行為（不檢查）
// empty.back()     -> 未定義行為（不檢查）
// empty[0]         -> 未定義行為（不檢查）
// empty[size()-1]  -> size()-1 先環繞成 18446744073709551615，再越界（更糟）
// empty.data()     -> 合法！回傳值可能是 nullptr: true
//
// 安全的存取方式：一律先檢查 empty()
//   容器是空的，跳過存取（正確做法）
//
// 【LeetCode 20. Valid Parentheses】
//   "()" -> true
//   "()[]{}" -> true
//   "(]" -> false
//   "([)]" -> false
//   "{[]}" -> true
//   "" -> true
//   "(((" -> false
//   關鍵：每次呼叫 back() 前都先檢查 empty()——
//         back() 自己不做這件事，對空容器是 UB。
//
// 【日常實務：用 data() 對接 C API】
//   資料長度: 24 bytes
//   checksum: 2919735937
//   空資料的 checksum: 0（data() 可能是 nullptr，但我們先擋掉了 len==0）
//   從 C 陣列 memcpy 進 vector，size=11（含內嵌 '\0'，這是 C 字串做不到的）
//   checksum: 2694134857
//   → 一律用 data() 而非 &v[0]：空 vector 時後者是 UB。
//
// ==============================================
//  建議：一般用 []；不確定索引合法性時用 at()
//  front()/back() 語意清晰，優先於 v[0]/v[size-1]
// ==============================================
