// =============================================================================
//  第三課：STL 的六大組件概覽 8  —  Lambda 表達式：捕獲、閉包型別與 transform
// =============================================================================
//
// 【主題資訊 Information】
//   語法：[捕獲列表](參數列表) mutable noexcept -> 回傳型別 { 函式本體 }
//         其中只有 [] 與 {} 是必要的，最短合法 lambda 是 []{}。
//   標準版本：
//     C++11  基本 lambda、[=] / [&] / [x] / [&x] 捕獲
//     C++14  泛型 lambda（auto 參數）、初始化捕獲 [p = std::move(ptr)]
//     C++17  constexpr lambda、[*this] 捕獲物件副本
//     C++20  樣板參數 lambda []<typename T>(T x){}、lambda 可用於未求值語境
//   複雜度：lambda 本身零成本；本檔的 count_if / transform 皆為 O(N)。
//   標頭檔：<algorithm>（count_if / transform）；lambda 本身不需要任何標頭檔。
//
// 【詳細解釋 Explanation】
//
// 【1. lambda 就是編譯器幫你寫的 functor】
//   編譯器看到 [divisor](int n) { return n % divisor == 0; } 會生成一個
//   獨一無二的匿名類別（稱為 closure type）：
//       class __lambda_xyz {
//           int divisor;                                            // 捕獲 → 成員變數
//       public:
//           __lambda_xyz(int d) : divisor(d) {}
//           bool operator()(int n) const { return n % divisor == 0; }  // 本體 → operator()
//       };
//   注意 operator() 預設帶 const —— 這就是為什麼在 lambda 裡改捕獲來的變數
//   會編譯錯誤，除非加 mutable 關鍵字。
//   因為型別是編譯期就確定的具體類別，count_if 可以完全 inline，
//   效能與手寫 functor 一致、與手寫迴圈也一致。
//
// 【2. 捕獲的四種寫法與各自風險】
//     [ ]        不捕獲。可隱式轉成函式指標（無捕獲 lambda 的特權），
//                因此能傳給 C API 的 callback 參數。
//     [x]        按值捕獲：建構 lambda 的當下複製一份。之後外部 x 怎麼變都不影響。
//     [&x]       按參考捕獲：存的是參考。**外部 x 的生命週期必須長於 lambda**。
//     [=] / [&]  全部按值 / 全部按參考。**不建議在會被保存的 lambda 上使用**，
//                因為讀者看不出到底捕獲了什麼，也容易誤捕獲 this。
//   最危險的是「回傳一個按參考捕獲區域變數的 lambda」：
//       auto make() { int local = 42; return [&local]{ return local; }; }  // 懸空參考！
//   函式返回後 local 已銷毀，之後呼叫該 lambda 是未定義行為。
//
// 【3. mutable 的真正意義】
//   lambda 的 operator() 預設是 const，所以按值捕獲的成員在 lambda 內是唯讀的：
//       int c = 0;
//       auto f = [c]() { ++c; };            // 編譯錯誤：在 const 成員函式中修改成員
//       auto g = [c]() mutable { ++c; };    // OK：operator() 不再是 const
//   但要注意 mutable 改的是**副本**，外部的 c 完全不受影響。
//   想改外部變數請用 [&c]，不是 mutable。
//
// 【4. 為什麼「只有無捕獲 lambda 能轉成函式指標」】
//   函式指標只是一個位址，沒有地方存狀態。
//   無捕獲的 closure 沒有成員變數，行為完全由程式碼決定，
//   因此標準特別允許它隱式轉換成對應的函式指標。
//   一旦捕獲了任何東西，那份狀態必須跟著物件走，函式指標裝不下 → 轉換不成立。
//   這正是「callback 要傳 void* user_data」這種 C 風格 API 存在的原因。
//
// 【概念補充 Concept Deep Dive】
//   本檔的 transform 有個容易被忽略的正確性前提：**目的地必須已經有足夠空間**。
//       std::vector<int> squared;
//       std::transform(vec.begin(), vec.end(), squared.begin(), f);   // UB！
//   squared 是空的，squared.begin() == squared.end()，
//   寫入它等於對空容器越界寫 —— 未定義行為（可能沒立刻崩潰，這更糟）。
//   兩種正確作法：
//     (a) 先 squared.resize(vec.size())，讓元素真的存在（本檔採用）
//     (b) 用 std::back_inserter(squared)，讓每次寫入變成 push_back
//   差別：(a) 會先值初始化 N 個元素（多一次寫入），但只配置一次記憶體；
//         (b) 不做多餘初始化，但可能經歷數次重新配置（可先 reserve 緩解）。
//   注意 reserve 只改變 capacity 不改變 size，所以 reserve 之後仍**不能**
//   直接用 begin() 當輸出迭代器 —— 這是很常見的誤解。
//
// 【注意事項 Pay Attention】
//   1. 按參考捕獲 [&] 的變數必須活得比 lambda 久；回傳這種 lambda 會產生懸空參考。
//   2. mutable 只讓你改「副本」，不會影響外部變數。
//   3. 在類別成員函式裡寫 [=] 會捕獲 this **指標**（不是物件副本），
//      物件若先被銷毀就會懸空。C++17 起可用 [*this] 捕獲物件副本。
//      C++20 起 [=] 隱式捕獲 this 已被 deprecated。
//   4. transform 的目的地必須已有元素或使用 back_inserter；
//      對空 vector 傳 begin() 是未定義行為。
//   5. 每個 lambda 運算式的型別都獨一無二，無法宣告該型別的變數；
//      要存起來用 auto，要跨型別統一存放用 std::function（有額外成本）。
//   6. 遞迴 lambda 不能直接寫（型別推導中無法引用自己），
//      需借助 std::function 或 C++23 的 deducing this。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】Lambda 表達式
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. lambda 的底層是什麼？捕獲的變數存在哪裡？
//     答：編譯器產生一個匿名類別（closure type），捕獲的變數成為它的成員變數，
//         lambda 本體成為 operator()（預設帶 const）。
//         按值捕獲存的是副本、按參考捕獲存的是參考。
//         因為型別在編譯期確定，呼叫可以完全 inline，沒有任何額外成本。
//     追問：那 lambda 物件本身有多大？
//           → 等於所有捕獲變數的大小總和（加上對齊）。無捕獲的 lambda 是空類別，
//             sizeof 為 1（本機 g++ 15.2 實測）。
//
// 🔥 Q2. [=] 和 [&] 各在什麼情況下該用？有什麼風險？
//     答：[=] 按值複製，安全但可能有複製成本，且在成員函式中會**捕獲 this 指標**
//         而非物件副本 —— 物件先死掉就懸空。[&] 按參考，零複製但要求
//         被捕獲的變數活得比 lambda 久。
//         規則：立即使用（傳給 STL 演算法）用 [&] 很安全；
//         要保存起來稍後執行（存進容器、丟給執行緒）務必用明確的按值捕獲。
//     追問：C++14 的初始化捕獲解決了什麼？
//           → [p = std::move(ptr)] 允許把 move-only 型別（如 unique_ptr）
//             搬進 lambda，這在 C++11 是做不到的（只能複製或參考）。
//
// ⚠️ 陷阱. std::vector<int> out; std::transform(v.begin(), v.end(), out.begin(), f);
//          —— 為什麼這段是錯的？
//     答：out 是空的，out.begin() 就是 out.end()，往那裡寫入是越界寫入，
//         屬於未定義行為。可能當場崩潰、可能寫壞堆積後在別處爆炸、
//         也可能「看起來正常」—— 症狀不固定，正是 UB 最難查的地方。
//         正解：先 out.resize(v.size())，或改用 std::back_inserter(out)。
//     為什麼會錯：把 out.begin() 想成「out 的寫入起點」，
//         但迭代器指向的是**已存在的元素**。空容器沒有元素，也就沒有可寫的位置。
//         另一個常見變形是「我已經 reserve 了應該就可以」—— 不行，
//         reserve 只配置 capacity，size 仍是 0，元素尚未建構。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 27. Remove Element
//   題目：就地移除陣列中所有等於 val 的元素，回傳剩餘長度 k；
//         前 k 個位置須是保留下來的元素（順序不限）。
//   為什麼用到本主題：標準解法就是 erase-remove 慣用法，
//         而「哪些元素該被移除」這個條件正是用 lambda 表達最自然的地方。
//         這裡用 std::remove_if + lambda，示範 lambda 捕獲 val 當作述詞。
//   複雜度：時間 O(N)、空間 O(1)。
//   注意：std::remove_if 不會真的縮短容器，它只是把要保留的元素往前搬，
//         回傳「新的邏輯結尾」。要真的刪除必須再呼叫 erase（見下）。
// -----------------------------------------------------------------------------
int removeElement(std::vector<int>& nums, int val) {
    auto new_end = std::remove_if(nums.begin(), nums.end(),
                                  [val](int n) { return n == val; });
    int k = static_cast<int>(new_end - nums.begin());
    nums.erase(new_end, nums.end());   // LeetCode 只看前 k 個，但真實程式碼要 erase
    return k;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】使用者上傳的檔案清單：批次過濾與重新命名
//   情境：後端收到一批上傳檔名，要 (1) 過濾掉不允許的副檔名，
//         (2) 統一加上租戶前綴避免不同客戶的檔名互撞。
//   為什麼用到本主題：兩個步驟都是「把一段可變邏輯交給演算法」——
//         copy_if 收一個述詞、transform 收一個轉換函式，
//         而且兩者都需要捕獲外部設定（允許清單、租戶代號），
//         正是 lambda 捕獲最典型的用途。
// -----------------------------------------------------------------------------
bool hasAllowedExtension(const std::string& filename,
                         const std::vector<std::string>& allowed) {
    std::size_t dot = filename.rfind('.');
    if (dot == std::string::npos) return false;
    std::string ext = filename.substr(dot);
    return std::find(allowed.begin(), allowed.end(), ext) != allowed.end();
}

std::vector<std::string> sanitizeUploads(const std::vector<std::string>& raw,
                                         const std::vector<std::string>& allowed,
                                         const std::string& tenant) {
    // 步驟 1：過濾（lambda 捕獲 allowed）
    std::vector<std::string> kept;
    std::copy_if(raw.begin(), raw.end(), std::back_inserter(kept),
                 [&allowed](const std::string& f) {
                     return hasAllowedExtension(f, allowed);
                 });

    // 步驟 2：改名（lambda 捕獲 tenant）
    std::vector<std::string> renamed(kept.size());   // 先配好空間，才能用 begin() 當輸出
    std::transform(kept.begin(), kept.end(), renamed.begin(),
                   [&tenant](const std::string& f) { return tenant + "_" + f; });
    return renamed;
}

int main() {
    std::vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    // Lambda 表達式
    int count = static_cast<int>(std::count_if(vec.begin(), vec.end(),
        [](int n) { return n % 2 == 0; }  // Lambda
    ));
    std::cout << "偶數個數: " << count << std::endl;

    // 帶捕獲的 Lambda
    int divisor = 3;
    int count3 = static_cast<int>(std::count_if(vec.begin(), vec.end(),
        [divisor](int n) { return n % divisor == 0; }  // 捕獲外部變數
    ));
    std::cout << "3的倍數個數: " << count3 << std::endl;

    // 用 Lambda 做 transform
    std::vector<int> squared;
    squared.resize(vec.size());   // 關鍵：先讓元素存在，才能用 begin() 當輸出迭代器
    std::transform(vec.begin(), vec.end(), squared.begin(),
        [](int n) { return n * n; }
    );
    std::cout << "平方: ";
    for (int n : squared) std::cout << n << " ";
    std::cout << std::endl;

    // 按值 vs 按參考捕獲
    std::cout << "\n=== 按值 vs 按參考捕獲 ===" << std::endl;
    int base = 100;
    auto by_value = [base](int n) { return n + base; };   // 建構當下就複製了 100
    auto by_ref   = [&base](int n) { return n + base; };  // 每次呼叫才讀 base
    base = 999;                                            // 改動外部變數
    std::cout << "改 base 為 999 之後：" << std::endl;
    std::cout << "  按值捕獲 [base] (1) = " << by_value(1) << "  ← 仍用舊值 100" << std::endl;
    std::cout << "  按參考捕獲[&base](1) = " << by_ref(1)   << "  ← 用新值 999" << std::endl;

    // mutable：改的是副本
    std::cout << "\n=== mutable 改的是副本 ===" << std::endl;
    int counter = 0;
    auto tick = [counter]() mutable { ++counter; return counter; };
    std::cout << "  呼叫三次 tick(): " << tick() << " " << tick() << " " << tick() << std::endl;
    std::cout << "  外部 counter 仍是: " << counter << "  ← mutable 不影響外部" << std::endl;

    // lambda 的大小（實作定義，見檔尾說明）
    std::cout << "\n=== lambda 物件的大小 ===" << std::endl;
    auto no_capture  = [](int n) { return n * 2; };
    auto cap_one_int = [divisor](int n) { return n % divisor; };
    std::cout << "  無捕獲 lambda      : " << sizeof(no_capture)  << " byte" << std::endl;
    std::cout << "  捕獲一個 int       : " << sizeof(cap_one_int) << " bytes" << std::endl;

    // 無捕獲 lambda 可轉成函式指標
    int (*fp)(int) = no_capture;   // 只有無捕獲時這行才成立
    std::cout << "  轉成函式指標後 fp(21) = " << fp(21) << std::endl;

    std::cout << "\n=== LeetCode 27. Remove Element ===" << std::endl;
    std::vector<int> nums1 = {3, 2, 2, 3};
    int k1 = removeElement(nums1, 3);
    std::cout << "[3,2,2,3] 移除 3 → k=" << k1 << "，內容 = ";
    for (int n : nums1) std::cout << n << " ";
    std::cout << std::endl;

    std::vector<int> nums2 = {0, 1, 2, 2, 3, 0, 4, 2};
    int k2 = removeElement(nums2, 2);
    std::cout << "[0,1,2,2,3,0,4,2] 移除 2 → k=" << k2 << "，內容 = ";
    for (int n : nums2) std::cout << n << " ";
    std::cout << std::endl;

    std::cout << "\n=== 日常實務：上傳檔案過濾與改名 ===" << std::endl;
    std::vector<std::string> uploads = {
        "report.pdf", "malware.exe", "photo.png", "notes.txt",
        "script.sh", "avatar.jpg", "README"
    };
    std::vector<std::string> allowed = {".pdf", ".png", ".jpg", ".txt"};
    std::cout << "原始上傳 " << uploads.size() << " 個檔案" << std::endl;
    for (const std::string& f : sanitizeUploads(uploads, allowed, "tenant42")) {
        std::cout << "  接受: " << f << std::endl;
    }

    return 0;
}

// 注意：lambda 物件的 sizeof 為「實作定義」。以下 1 byte / 4 bytes 是本機
//       g++ 15.2 / libstdc++ / x86-64 的實測值（空類別最小 1 byte；
//       捕獲一個 int 則為 4 bytes）。其他平台的對齊規則可能不同。

// 編譯: g++ -std=c++17 -Wall -Wextra 第三課：STL 的六大組件概覽8.cpp -o demo8

// === 預期輸出 ===
// 偶數個數: 5
// 3的倍數個數: 3
// 平方: 1 4 9 16 25 36 49 64 81 100
//
// === 按值 vs 按參考捕獲 ===
// 改 base 為 999 之後：
//   按值捕獲 [base] (1) = 101  ← 仍用舊值 100
//   按參考捕獲[&base](1) = 1000  ← 用新值 999
//
// === mutable 改的是副本 ===
//   呼叫三次 tick(): 1 2 3
//   外部 counter 仍是: 0  ← mutable 不影響外部
//
// === lambda 物件的大小 ===
//   無捕獲 lambda      : 1 byte
//   捕獲一個 int       : 4 bytes
//   轉成函式指標後 fp(21) = 42
//
// === LeetCode 27. Remove Element ===
// [3,2,2,3] 移除 3 → k=2，內容 = 2 2
// [0,1,2,2,3,0,4,2] 移除 2 → k=5，內容 = 0 1 3 0 4
//
// === 日常實務：上傳檔案過濾與改名 ===
// 原始上傳 7 個檔案
//   接受: tenant42_report.pdf
//   接受: tenant42_photo.png
//   接受: tenant42_notes.txt
//   接受: tenant42_avatar.jpg
