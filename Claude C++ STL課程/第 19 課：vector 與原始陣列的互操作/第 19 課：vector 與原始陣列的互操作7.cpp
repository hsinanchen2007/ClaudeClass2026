// =============================================================================
//  第 19 課：vector 與原始陣列的互操作 7  —  從原始陣列建構 vector（含陣列退化）
// =============================================================================
//
// 【主題資訊 Information】
//   template <class InputIt>
//   vector(InputIt first, InputIt last, const Allocator& alloc = Allocator());
//                                                     // 迭代器範圍建構子（C++98 起）
//
//   輔助工具:
//     std::begin(arr) / std::end(arr)   // <iterator>，C++11
//     std::size(arr)                    // <iterator>，C++17（已用 -pedantic-errors 驗證）
//     sizeof(arr) / sizeof(arr[0])      // 傳統寫法，只在陣列型別可見處成立
//
//   標頭檔 : <vector>、<iterator>
//   複雜度 : O(N)，一次配置 + N 次複製建構。
//   關鍵   : 原始指標本身就是合法的 random access iterator，
//            所以 vector<int> v(arr, arr + n) 可以直接運作，不需要任何轉接器。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼指標可以當迭代器用】
//   STL 的迭代器是「概念（concept）」而不是某個基底類別——
//   只要支援 *it、++it、it + n、it1 - it2 這些操作就算數。
//   原始指標天生全部支援，所以它就是最原始、也最高效的 random access iterator。
//   這是 STL 設計上最漂亮的一點：泛型演算法不必為「陣列」寫特例，
//   std::sort(arr, arr + n) 和 std::sort(v.begin(), v.end()) 走的是同一份程式碼。
//
//   也因此 vector 的迭代器範圍建構子可以直接吃 (arr, arr + n)：
//   它會先算出距離 last - first（random access 迭代器是 O(1)），
//   一次配置好足夠空間，再逐一複製建構——沒有多餘的重新配置。
//
// 【2. 陣列退化（array decay）：本檔最重要的觀念】
//   C/C++ 中，陣列在大多數情境下會自動轉成「指向首元素的指標」，這叫退化。
//   最要命的是「傳進函式時一定會退化」：
//       void f(int arr[10]);   // 騙人的寫法，編譯器把它當成 void f(int* arr);
//       void f(int arr[]);     // 同上
//       void f(int* arr);      // 三者完全等價
//   一旦退化，長度資訊就永久遺失。所以：
//       sizeof(arr) 在 main 裡是「整個陣列的位元組數」（例如 20），
//       同一個 arr 傳進函式後 sizeof(arr) 變成「指標的大小」（本機 8）。
//   這就是為什麼 sizeof(arr)/sizeof(arr[0]) 這個經典寫法只能在
//   「陣列型別還看得見」的地方用——它在函式內部會安靜地算出錯誤答案。
//
// 【3. std::begin / std::end 為什麼比較好】
//   std::end(arr) 靠模板參數推導出陣列長度 N（簽名是 T (&)[N]），
//   所以它「只接受真正的陣列」。如果你不小心傳的是已退化的指標，
//   會直接編譯失敗——把執行期的沉默錯誤變成編譯期的明確錯誤。
//   C++17 起還有 std::size(arr)，同樣是編譯期取得長度，且回傳 size_t。
//
// 【4. 三種建構寫法的比較】
//   vector<int> v1(arr, arr + n);                  // 需要自己算 n，可能算錯
//   vector<int> v2(std::begin(arr), std::end(arr)); // 長度自動推導，最安全
//   vector<int> v3(arr, arr + std::size(arr));      // C++17，語意最直白
//   三者結果完全相同，差別只在「長度是誰算的」。優先用 v2 / v3。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 建構後是「複製」，兩者從此獨立
//   vector 的迭代器範圍建構子會複製每一個元素到自己配置的記憶體。
//   之後改 arr 不會影響 v，改 v 也不影響 arr。
//   這也表示原始陣列若是 new[] 來的，複製完就可以（也應該）釋放——
//   見本課第 10 個檔案。
//
// (B) 為什麼 vector 知道要配置多少
//   標準要求：若迭代器是 forward 以上，實作可以先算 distance 再一次配置。
//   指標是 random access，distance 是 O(1) 的減法，所以完全沒有多次配置的問題。
//   反之若傳的是 input iterator（例如 istream_iterator），
//   vector 無法預知長度，只能邊讀邊成長，可能發生多次重新配置。
//
// (C) 二維陣列不會完全退化
//   int m[3][4] 傳進函式時退化成 int (*)[4]——只有最外層退化，
//   第二維的長度 4 被保留在型別裡。這是為什麼二維陣列參數一定要寫
//   void f(int m[][4]) 而不能省略第二維。
//   要把二維陣列攤平成 vector，可以利用「整塊記憶體本身是連續的」這件事：
//   vector<int>(&m[0][0], &m[0][0] + 3 * 4)。
//
// 【注意事項 Pay Attention】
//   1. 陣列傳進函式一定會退化成指標，sizeof 技巧在函式內部會算出錯誤答案。
//   2. std::begin/std::end 只接受真正的陣列，傳指標會編譯失敗（這是優點）。
//   3. std::size 是 C++17；std::begin/std::end 是 C++11。
//   4. 建構是複製，vector 與原陣列從此完全獨立。
//   5. 二維陣列只有最外層退化，內層維度保留在型別中。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】陣列轉 vector 與陣列退化
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 vector<int> v(arr, arr + n); 這樣寫可以成立？
//     答：因為原始指標本身就滿足 random access iterator 的所有要求
//         （*、++、+n、相減），STL 的迭代器是概念不是基底類別。
//         vector 的迭代器範圍建構子接受任何符合概念的型別，指標當然可以。
//         而且因為是 random access，vector 能先用 last - first 算出長度、
//         一次配置到位，不會發生多次重新配置。
//     追問：如果傳的是 std::istream_iterator 呢？
//         → 那是 input iterator，無法預知長度，vector 只能邊讀邊成長，
//           過程中可能多次重新配置。
//
// 🔥 Q2. sizeof(arr) / sizeof(arr[0]) 這個算長度的寫法，什麼時候會出錯？
//     答：把陣列當參數傳進函式之後就會出錯。陣列傳參一定會退化成指標，
//         此時 sizeof(arr) 是指標大小（本機 x86-64 為 8），
//         sizeof(arr[0]) 是 4，算出來永遠是 2，跟真正的長度無關。
//         而且編譯器不會報錯，是安靜的錯誤答案。
//     追問：怎麼避免？
//         → 用 std::size(arr)（C++17）或 std::end(arr) - std::begin(arr)（C++11）。
//           它們靠模板推導陣列長度，傳指標進去會直接編譯失敗。
//     補充：GCC 有專門的警告 -Wsizeof-array-argument（-Wall 即開啟），
//           會在你對 int arr[] 形式的參數 sizeof 時直接提醒
//           「will return size of 'int*'」。看到它請務必修正，別關掉。
//
// ⚠️ 陷阱. void f(int arr[10]) 這樣宣告，編譯器會檢查呼叫端真的傳 10 個元素嗎？
//     答：不會。這個 [10] 完全被忽略，簽名等同 void f(int* arr)。
//         你可以傳 3 個元素的陣列進去，編譯完全不會抱怨，
//         函式裡若照著存取 10 個就是越界讀寫，屬於未定義行為。
//     為什麼會錯：以為寫在參數列的陣列長度是一種契約或檢查，
//         但它只是「給人看的註解」，編譯器不採用。
//         真的要在型別層面綁定長度，必須用參考：void f(int (&arr)[10])——
//         這樣長度就是型別的一部分，傳錯大小會編譯失敗。
// ═══════════════════════════════════════════════════════════════════════════

#include <cstddef>
#include <iostream>
#include <iterator>
#include <vector>

static void printVec(const char* label, const std::vector<int>& v) {
    std::cout << label;
    for (std::size_t i = 0; i < v.size(); ++i) std::cout << (i ? " " : "") << v[i];
    std::cout << "\n";
}

// -----------------------------------------------------------------------------
// 示範陣列退化：同一個陣列，在函式內外 sizeof 的結果完全不同
//
//   註：這裡刻意把參數寫成 int* arr 而不是 int arr[]。
//   兩種寫法的語意 100% 相同（都是指標），但 GCC 對後者有一個專門的警告
//   -Wsizeof-array-argument（-Wall 預設開啟）：
//       warning: 'sizeof' on array function parameter 'arr' will return size of 'int*'
//   也就是說，編譯器已經知道「你寫 int arr[] 卻對它 sizeof」幾乎一定是 bug，
//   會主動提醒你。這是很值得記住的一件事——
//   真實專案裡看到這個警告，不要關掉它，那通常是真的算錯長度了。
//   本檔為了在 -Wall -Wextra 下零警告，改用等價的 int* 寫法示範同一個現象。
// -----------------------------------------------------------------------------
void showDecay(int* arr) {                  // 與 int arr[] / int arr[10] 完全等價
    std::cout << "  函式內 sizeof(arr)          = " << sizeof(arr)
              << "（這是指標大小，不是陣列大小！）\n";
    std::cout << "  函式內 sizeof(arr)/sizeof(arr[0]) = "
              << sizeof(arr) / sizeof(arr[0])
              << "（錯誤答案，與真實長度無關）\n";
}

// 用參考綁住長度：長度成為型別的一部分，傳錯大小會編譯失敗
template <std::size_t N>
void showByReference(int (&arr)[N]) {
    std::cout << "  以 int(&)[N] 接收，N = " << N << "（正確，編譯期就知道）\n";
    std::cout << "  函式內 sizeof(arr)          = " << sizeof(arr)
              << "（陣列型別完整保留）\n";
}

// -----------------------------------------------------------------------------
// 【日常實務範例】把韌體裡的「唯讀校正表」載入成可修改的 vector
//   情境：感測器的出廠校正係數以 const int 陣列寫死在韌體標頭檔裡
//         （這是嵌入式常見做法：放在 ROM，不佔 RAM）。
//         執行時要依現場溫度做微調，所以得複製一份到可寫的容器。
//   為何用範圍建構：陣列長度在編譯期已知，用 std::begin/std::end 一行複製完成，
//         既不會算錯長度，也不需要手動迴圈 push_back。
//         複製後 vector 與 ROM 中的原表完全獨立，微調不會（也不能）動到原表。
// -----------------------------------------------------------------------------
const int kFactoryCalibration[] = {1000, 1005, 1010, 1008, 1002, 998, 995};

std::vector<int> loadCalibrationTable(int tempOffset) {
    // std::begin/std::end 直接吃陣列，長度由型別推導，不可能算錯
    std::vector<int> table(std::begin(kFactoryCalibration),
                           std::end(kFactoryCalibration));
    for (int& v : table) v += tempOffset;    // 現場微調，只動副本
    return table;
}

int main() {
    int arr[] = {10, 20, 30, 40, 50};

    // -------------------------------------------------------------------------
    std::cout << "=== 三種從陣列建構 vector 的寫法 ===\n";
    // -------------------------------------------------------------------------
    // 方法一：手動算長度（傳統，但長度是自己算的，有算錯的可能）
    const std::size_t arrSize = sizeof(arr) / sizeof(arr[0]);
    std::vector<int> v1(arr, arr + arrSize);

    // 方法二：std::begin / std::end（C++11）—— 長度由型別自動推導
    std::vector<int> v2(std::begin(arr), std::end(arr));

    // 方法三：std::size（C++17）—— 語意最直白
    std::vector<int> v3(arr, arr + std::size(arr));

    printVec("v1（sizeof 算長度）      : ", v1);
    printVec("v2（std::begin/end）     : ", v2);
    printVec("v3（std::size, C++17）   : ", v3);
    std::cout << "三者內容是否相同: " << ((v1 == v2 && v2 == v3) ? "是" : "否") << "\n";

    // -------------------------------------------------------------------------
    std::cout << "\n=== 陣列退化：同一個 arr，函式內外 sizeof 不同 ===\n";
    // -------------------------------------------------------------------------
    std::cout << "  main 內 sizeof(arr)         = " << sizeof(arr)
              << "（整個陣列的位元組數）\n";
    std::cout << "  main 內 sizeof(arr[0])      = " << sizeof(arr[0]) << "\n";
    std::cout << "  main 內算出的長度           = " << sizeof(arr) / sizeof(arr[0])
              << "（正確）\n";
    std::cout << "  sizeof(int*)                = " << sizeof(int*)
              << "（實作定義，本機 x86-64 為 8）\n";
    showDecay(arr);
    showByReference(arr);

    // -------------------------------------------------------------------------
    std::cout << "\n=== 建構是複製：兩者從此完全獨立 ===\n";
    // -------------------------------------------------------------------------
    std::vector<int> copyOf(std::begin(arr), std::end(arr));
    arr[0] = 999;                     // 改原陣列
    copyOf[1] = 888;                  // 改 vector
    std::cout << "改 arr[0]=999、copyOf[1]=888 之後：\n";
    std::cout << "  arr    : ";
    for (std::size_t i = 0; i < std::size(arr); ++i)
        std::cout << (i ? " " : "") << arr[i];
    std::cout << "\n";
    printVec("  copyOf : ", copyOf);
    std::cout << "  → 互不影響，證實建構時做了完整複製\n";
    arr[0] = 10;                      // 還原，方便後面示範

    // -------------------------------------------------------------------------
    std::cout << "\n=== 二維陣列：只有最外層退化 ===\n";
    // -------------------------------------------------------------------------
    int matrix[3][4] = {{1, 2, 3, 4}, {5, 6, 7, 8}, {9, 10, 11, 12}};
    std::cout << "  sizeof(matrix)    = " << sizeof(matrix) << " bytes\n";
    std::cout << "  sizeof(matrix[0]) = " << sizeof(matrix[0])
              << " bytes（一整列）\n";
    std::cout << "  列數 = " << sizeof(matrix) / sizeof(matrix[0])
              << "，行數 = " << sizeof(matrix[0]) / sizeof(matrix[0][0]) << "\n";

    // 二維陣列整塊記憶體是連續的，可以直接攤平成一維 vector
    std::vector<int> flat(&matrix[0][0], &matrix[0][0] + 3 * 4);
    printVec("  攤平成一維: ", flat);

    // -------------------------------------------------------------------------
    std::cout << "\n=== 日常實務：載入韌體校正表並做溫度微調 ===\n";
    // -------------------------------------------------------------------------
    std::vector<int> calib = loadCalibrationTable(+3);
    std::cout << "  ROM 原表（唯讀）: ";
    for (std::size_t i = 0; i < std::size(kFactoryCalibration); ++i)
        std::cout << (i ? " " : "") << kFactoryCalibration[i];
    std::cout << "\n";
    printVec("  微調 +3 後的副本: ", calib);
    std::cout << "  ROM 原表未被改動: "
              << (kFactoryCalibration[0] == 1000 ? "是" : "否") << "\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第\ 19\ 課：vector\ 與原始陣列的互操作7.cpp -o interop7
//
// ── 輸出但書 ────────────────────────────────────────────────────────────
// 1.【與上文的更正】上面 showDecay() 的註解寫「改用 int* 寫法可在 -Wall
//    -Wextra 下零警告」，本機實測並非如此。GCC 15.2.0 對 int* 版本仍會發出
//    另一個警告（改由 -Wsizeof-pointer-div 接手）：
//      warning: division 'sizeof (int*) / sizeof (int)' does not compute
//               the number of array elements [-Wsizeof-pointer-div]
//    也就是說：不論寫 int arr[] 還是 int*，只要對「已退化的指標」做
//    sizeof(arr)/sizeof(arr[0])，現代 GCC 都攔得下來。這其實強化了本課重點
//    —— 編譯器把這個算法視為幾乎必然的 bug。此警告是預期中的教學示範，
//    不影響編譯成功（rc=0），請勿為了消警告而移除這段示範。
// 2. sizeof(int*) = 8 屬實作定義（本機 x86-64 LP64）；32-bit 平台為 4。
//    連帶「函式內 sizeof(arr)/sizeof(arr[0]) = 2」也是 8/4 的結果，
//    在不同平台上這個「錯誤答案」會是不同的數字。

// === 預期輸出 ===
// === 三種從陣列建構 vector 的寫法 ===
// v1（sizeof 算長度）      : 10 20 30 40 50
// v2（std::begin/end）     : 10 20 30 40 50
// v3（std::size, C++17）   : 10 20 30 40 50
// 三者內容是否相同: 是
//
// === 陣列退化：同一個 arr，函式內外 sizeof 不同 ===
//   main 內 sizeof(arr)         = 20（整個陣列的位元組數）
//   main 內 sizeof(arr[0])      = 4
//   main 內算出的長度           = 5（正確）
//   sizeof(int*)                = 8（實作定義，本機 x86-64 為 8）
//   函式內 sizeof(arr)          = 8（這是指標大小，不是陣列大小！）
//   函式內 sizeof(arr)/sizeof(arr[0]) = 2（錯誤答案，與真實長度無關）
//   以 int(&)[N] 接收，N = 5（正確，編譯期就知道）
//   函式內 sizeof(arr)          = 20（陣列型別完整保留）
//
// === 建構是複製：兩者從此完全獨立 ===
// 改 arr[0]=999、copyOf[1]=888 之後：
//   arr    : 999 20 30 40 50
//   copyOf : 10 888 30 40 50
//   → 互不影響，證實建構時做了完整複製
//
// === 二維陣列：只有最外層退化 ===
//   sizeof(matrix)    = 48 bytes
//   sizeof(matrix[0]) = 16 bytes（一整列）
//   列數 = 3，行數 = 4
//   攤平成一維: 1 2 3 4 5 6 7 8 9 10 11 12
//
// === 日常實務：載入韌體校正表並做溫度微調 ===
//   ROM 原表（唯讀）: 1000 1005 1010 1008 1002 998 995
//   微調 +3 後的副本: 1003 1008 1013 1011 1005 1001 998
//   ROM 原表未被改動: 是
