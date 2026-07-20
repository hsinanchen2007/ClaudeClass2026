// =============================================================================
//  第 19 課：vector 與原始陣列的互操作 9  —  assign()：整批換掉既有內容
// =============================================================================
//
// 【主題資訊 Information】
//   template <class InputIt> void assign(InputIt first, InputIt last);  // 區間版
//   void assign(size_type count, const T& value);                       // 填充版
//   void assign(std::initializer_list<T> ilist);                        // C++11
//
//   標頭檔：<vector>
//   複雜度：O(舊 size + 新 size)——先銷毀舊元素，再放入新元素
//   本檔標準：C++17
//
// 【詳細解釋 Explanation】
//
// 【1. assign 到底做了什麼】
//   assign 的語意是「把容器的內容整批換成另一批」，等價於
//       v.clear();  然後  v.insert(v.end(), first, last);
//   但它是單一操作，實作可以做得更聰明。關鍵在於：assign **不一定重新配置記憶體**。
//   libstdc++ 的實作邏輯大致是：
//       若 新長度 > capacity() → 只好配置新記憶體、搬過去、釋放舊的
//       若 新長度 <= size()    → 對前段用 operator= 覆寫（重用既有物件！），
//                                 再把多出來的尾巴 destroy 掉
//       若 size() < 新長度 <= capacity() → 前段覆寫、後段在既有空間上建構
//   也就是說 assign 會**盡量重用已配置的記憶體與已建構的物件**，這是它比
//   「clear() + push_back 迴圈」有效率的原因。
//
// 【2. 為什麼要有 assign，v = {...} 不夠嗎】
//   對於「來源是一對 iterator」的情況，operator= 幫不上忙——你沒辦法寫
//       v = {arr, arr + 4};        // 錯，這是兩個指標組成的 initializer_list
//   assign 是唯一能接受「任意 iterator 區間」來整批取代內容的成員函式。
//   來源可以是另一個 vector、list、set、原始陣列、甚至 istream_iterator。
//
// 【3. 與 clear() + push_back 的差別】
//   clear() 會把 size 歸零但 **capacity 不變**，接著 push_back 逐一放入，
//   每次都要檢查是否需要擴容。assign 則能一次算好長度（對 forward iterator 以上），
//   一次決定要不要重新配置。對大批資料，assign 少掉了 N 次容量檢查與可能的多次擴容。
//
// 【4. 三個重載的取捨】
//   assign(first, last)  ── 來源是既有資料（陣列、其他容器）
//   assign(count, value) ── 想要 N 個相同值，例如 assign(1000, 0) 重置緩衝區
//   assign({1,2,3})      ── 少量、寫死的值，可讀性最好
//
// 【概念補充 Concept Deep Dive】
//   ● assign 對 iterator / 指標 / reference 的影響
//     一律視為「全部失效」。即使實際上沒有重新配置、位址沒變，
//     標準也不保證原本的 iterator 還有效，寫程式時必須當作全失效。
//
//   ● capacity 只增不減
//     assign 一個較短的區間**不會**縮小 capacity。
//     本檔 main() 會實測：先 assign 4 個元素讓 capacity 變 4，
//     再 assign 2 個元素，capacity 仍維持 4。要真正還記憶體得用 shrink_to_fit()。
//
//   ● self-assign 的陷阱
//     v.assign(v.begin(), v.end()) 是 UB。實作在覆寫前段時就已經破壞了來源區間，
//     標準明文要求 assign 的來源不得指向容器自身。
//
// 【注意事項 Pay Attention】
//   1. assign 之後，所有先前取得的 iterator / pointer / reference 一律視為失效。
//   2. assign 不縮小 capacity；記憶體只會增加不會自動歸還。
//   3. 來源區間不可指向 vector 自己（self-assignment 是 UB）。
//   4. assign(5, 0) 與 assign({5, 0}) 意義完全不同：前者是 5 個 0，
//      後者是 {5, 0} 兩個元素——和建構子的 () / {} 陷阱同一個根源。
//   5. std::begin/std::end 只對「真正的陣列」有效；陣列一旦退化成指標就編不過，
//      這其實是好事——它在編譯期擋住了「長度資訊已遺失」的錯誤。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】vector::assign
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. v.assign(first, last) 和 v = 另一個 vector 有什麼差別？
//     答：operator= 只能接受同型別的 vector（或 initializer_list）；
//         assign 接受任意 iterator 區間，來源可以是陣列、list、set、
//         甚至 istream_iterator。assign 是「跨容器整批取代」的唯一成員函式。
//     追問：assign 會重新配置記憶體嗎？→ 不一定。只有新長度超過 capacity 時才會；
//         否則會重用既有記憶體，對前段元素直接用 operator= 覆寫。
//
// 🔥 Q2. assign 之後，原本指向元素的指標還能用嗎？
//     答：不能。標準規定 assign 使所有 iterator / pointer / reference 失效。
//         即使實測發現位址沒變（沒觸發重新配置），也不可以依賴——
//         那是實作細節，換個編譯器或換個資料量就會變。
//     追問：那 assign 之後 capacity 會變小嗎？→ 不會。capacity 只增不減，
//         要歸還記憶體必須另外呼叫 shrink_to_fit()（而且標準只說是「請求」）。
//
// ⚠️ 陷阱. 想「清空並重填」，寫成 v.assign(v.begin(), v.end()) 為什麼是錯的？
//     答：這是 self-assignment，屬於 UB。assign 在覆寫前段元素時，
//         已經把來源區間的內容破壞掉了，後面再讀就讀到被改寫的值。
//     為什麼會錯：多數人把 assign 想像成「先把來源整包複製到旁邊，再換掉自己」，
//         但實作為了效率是「就地逐一覆寫」，根本沒有中間備份。
//         同樣的道理也適用於 insert / erase 傳入指向自身的區間。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <iterator>   // std::begin / std::end
#include <string>
#include <sstream>

// -----------------------------------------------------------------------------
// 【日常實務範例 1】從設定檔重新載入「白名單 port」——熱重載情境
//   情境：伺服器收到 SIGHUP 要重讀設定檔。白名單容器是長期存在的成員變數，
//         我們要「換掉內容」而不是「重建物件」——這正是 assign 的主場。
//         用 assign 可以重用已配置的記憶體，避免每次 reload 都 malloc/free 一輪。
// -----------------------------------------------------------------------------
void reload_allowed_ports(std::vector<int>& allowed, const std::string& configLine) {
    std::vector<int> parsed;
    std::istringstream iss(configLine);
    std::string token;
    while (std::getline(iss, token, ',')) {
        try {
            parsed.push_back(std::stoi(token));
        } catch (const std::exception&) {
            // 設定檔有髒資料就跳過該欄，不讓整次 reload 失敗
        }
    }
    // 關鍵：assign 而非 allowed = parsed，語意是「換內容」，
    // 且能重用 allowed 既有的 capacity
    allowed.assign(parsed.begin(), parsed.end());
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】把影像列緩衝區重置成固定背景值
//   情境：繪圖 / 影像處理每畫一張新影格前要把 scanline 清成背景色。
//         assign(count, value) 一行完成「清空 + 填滿」，比 for 迴圈更明確也更快。
// -----------------------------------------------------------------------------
void reset_scanline(std::vector<unsigned char>& line, std::size_t width,
                    unsigned char background) {
    line.assign(width, background);
}

// 註：本檔不附 LeetCode 範例。assign 是「容器內容整批取代」的 API，
//     LeetCode 題目幾乎都直接拿到已填好的 nums，不會考「重新填充既有容器」，
//     硬掛一題反而誤導。相關的就地改寫題型已在同課 8.cpp（LeetCode 26）示範。

int main() {
    std::cout << "=== 1. assign：用陣列整批取代既有內容 ===\n";
    std::vector<int> v = {1, 2, 3};
    std::cout << "assign 前：";
    for (int x : v) std::cout << x << " ";
    std::cout << "(size=" << v.size() << ", capacity=" << v.capacity() << ")\n";

    int arr[] = {100, 200, 300, 400};

    // assign 會清除原有內容，用新資料取代
    v.assign(std::begin(arr), std::end(arr));

    std::cout << "assign 後：";
    for (int x : v) std::cout << x << " ";
    std::cout << "(size=" << v.size() << ", capacity=" << v.capacity() << ")\n";

    std::cout << "\n=== 2. 實測：assign 較短區間不會縮小 capacity ===\n";
    std::size_t capBefore = v.capacity();
    v.assign(arr, arr + 2);              // 只放 2 個
    std::cout << "assign 2 個元素後：size=" << v.size()
              << ", capacity=" << v.capacity() << "\n";
    std::cout << "capacity 由 " << capBefore << " 變成 " << v.capacity()
              << " → " << (capBefore == v.capacity() ? "沒有縮小（符合預期）"
                                                     : "有變動")
              << "\n";
    v.shrink_to_fit();
    std::cout << "呼叫 shrink_to_fit() 後：capacity=" << v.capacity()
              << "（標準只說是『請求』，本機 libstdc++ 確實照做）\n";

    std::cout << "\n=== 3. 三種 assign 重載的差別 ===\n";
    std::vector<int> a;
    a.assign(5, 0);
    std::cout << "assign(5, 0)   → size=" << a.size() << " 內容：";
    for (int x : a) std::cout << x << " ";
    std::cout << "\n";

    a.assign({5, 0});
    std::cout << "assign({5, 0}) → size=" << a.size() << " 內容：";
    for (int x : a) std::cout << x << " ";
    std::cout << "  ← 和上面完全不同！\n";

    std::vector<int> src = {7, 8, 9};
    a.assign(src.begin(), src.end());
    std::cout << "assign(區間)   → size=" << a.size() << " 內容：";
    for (int x : a) std::cout << x << " ";
    std::cout << "\n";

    std::cout << "\n=== 4. 跨容器：從 std::string 的字元建 vector<char> ===\n";
    std::string text = "STL";
    std::vector<char> chars;
    chars.assign(text.begin(), text.end());   // 來源是 string 的 iterator
    std::cout << "vector<char> 內容：";
    for (char c : chars) std::cout << c << " ";
    std::cout << "(size=" << chars.size() << ")\n";

    std::cout << "\n=== 日常實務 1：熱重載設定檔的 port 白名單 ===\n";
    std::vector<int> allowed = {80, 443};
    std::cout << "重載前：";
    for (int p : allowed) std::cout << p << " ";
    std::cout << "\n";

    reload_allowed_ports(allowed, "80,443,8080,9090,壞資料,3000");
    std::cout << "重載後：";
    for (int p : allowed) std::cout << p << " ";
    std::cout << "（髒資料欄位已被跳過）\n";

    reload_allowed_ports(allowed, "22");
    std::cout << "再次重載（只剩 1 個）：";
    for (int p : allowed) std::cout << p << " ";
    std::cout << " size=" << allowed.size()
              << " capacity=" << allowed.capacity()
              << "（capacity 保留，下次重載免再配置）\n";

    std::cout << "\n=== 日常實務 2：重置影像 scanline 背景色 ===\n";
    std::vector<unsigned char> scanline;
    reset_scanline(scanline, 8, 0xFF);
    std::cout << "白色背景 (0xFF) 前 8 像素：";
    for (unsigned char px : scanline) std::cout << static_cast<int>(px) << " ";
    std::cout << "\n";

    reset_scanline(scanline, 8, 0x00);
    std::cout << "黑色背景 (0x00) 前 8 像素：";
    for (unsigned char px : scanline) std::cout << static_cast<int>(px) << " ";
    std::cout << "\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 19 課：vector 與原始陣列的互操作9.cpp" -o demo9

// === 預期輸出 ===
// === 1. assign：用陣列整批取代既有內容 ===
// assign 前：1 2 3 (size=3, capacity=3)
// assign 後：100 200 300 400 (size=4, capacity=4)
//
// === 2. 實測：assign 較短區間不會縮小 capacity ===
// assign 2 個元素後：size=2, capacity=4
// capacity 由 4 變成 4 → 沒有縮小（符合預期）
// 呼叫 shrink_to_fit() 後：capacity=2（標準只說是『請求』，本機 libstdc++ 確實照做）
//
// === 3. 三種 assign 重載的差別 ===
// assign(5, 0)   → size=5 內容：0 0 0 0 0
// assign({5, 0}) → size=2 內容：5 0   ← 和上面完全不同！
// assign(區間)   → size=3 內容：7 8 9
//
// === 4. 跨容器：從 std::string 的字元建 vector<char> ===
// vector<char> 內容：S T L (size=3)
//
// === 日常實務 1：熱重載設定檔的 port 白名單 ===
// 重載前：80 443
// 重載後：80 443 8080 9090 3000 （髒資料欄位已被跳過）
// 再次重載（只剩 1 個）：22  size=1 capacity=5（capacity 保留，下次重載免再配置）
//
// === 日常實務 2：重置影像 scanline 背景色 ===
// 白色背景 (0xFF) 前 8 像素：255 255 255 255 255 255 255 255
// 黑色背景 (0x00) 前 8 像素：0 0 0 0 0 0 0 0
