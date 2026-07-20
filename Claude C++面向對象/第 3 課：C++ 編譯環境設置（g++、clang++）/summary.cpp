// =============================================================================
//  第 3 課 summary.cpp  —  編譯環境、編譯流程、與「同一份原始碼跨編譯器」
// =============================================================================
//
// 【主題資訊 Information】
//   主題      ： g++／clang++／MSVC 的選項對照、預定義巨集、編譯與連結流程
//   標準版本  ： 本檔以 -std=c++17 編譯；程式內用 __cplusplus 自我回報
//   標頭檔    ： <iostream>、<string>、<vector>（皆為驗證編譯環境用）
//   關鍵巨集  ： __GNUC__ / __clang__ / _MSC_VER / __cplusplus /
//               _WIN32 / __linux__ / __APPLE__ / __STRICT_ANSI__
//   本機環境  ： g++ (Ubuntu 15.2.0-16ubuntu1) 15.2.0
//               clang version 21.1.8、x86-64 Linux
//   複雜度    ： 不適用（本課談的是工具鏈，不是演算法）
//
// 【詳細解釋 Explanation】
//
// 【1. 「編譯」其實是四個階段，錯誤訊息來自哪一階段決定你怎麼修】
//   一句 g++ main.cpp -o app 背後依序做了四件事：
//     ① 前置處理 (Preprocess)  g++ -E   展開 #include、#define、#if
//     ② 編譯     (Compile)     g++ -S   C++ → 組合語言
//     ③ 組譯     (Assemble)    g++ -c   組合語言 → .o 目標檔
//     ④ 連結     (Link)        g++      多個 .o + 函式庫 → 執行檔
//   為什麼要知道這個？因為錯誤訊息的「長相」直接對應階段：
//     * fatal error: iostream: No such file or directory   → 階段①，include 路徑
//     * error: 'x' was not declared in this scope          → 階段②，語法／宣告
//     * undefined reference to `foo()'                     → 階段④，連結
//   最後這個特別容易誤診：undefined reference 幾乎永遠不是語法問題，
//   而是「宣告了但沒有把實作的那個 .cpp 一起編進來」或「函式庫沒 -l 上」。
//   看到它去檢查編譯指令，不要回頭改標頭檔。
//
// 【2. -std 不只是「選一個版本」，它同時決定要不要開 GNU 擴充】
//   g++ 有兩組值：c++17（嚴格 ISO）與 gnu++17（ISO + GNU 擴充）。
//   本機 g++ 15.2 實測：
//     * 不加任何 -std          → __cplusplus = 201703L，且「沒有」定義
//                                __STRICT_ANSI__ → 預設其實是 gnu++17
//     * 加 -std=c++17          → __cplusplus = 201703L，且「有」定義
//                                __STRICT_ANSI__ → 嚴格 ISO 模式
//   這代表：不寫 -std 也能編過的程式碼，可能正在用 GNU 擴充而你不知道，
//   換到 MSVC 就爆炸。永遠明確寫出 -std，是跨平台的第一道保險。
//
// 【3. 驗證「這個語法是哪個標準才有」的正確方法：-pedantic-errors】
//   常見的錯誤做法是用 -fsyntax-only 去試 —— 但 g++ 會把不少較新的語法
//   當成擴充放行，看起來像「舊標準也支援」，結論是錯的。
//   正確做法是：
//       g++ -std=c++14 -pedantic-errors test.cpp
//   -pedantic-errors 會把「不符合所選 ISO 標準」的用法從警告升級為錯誤，
//   這才是可信的判準。（本教材對每一個標準版本的宣稱都用這個方式驗過。）
//
// 【4. 為什麼判斷編譯器時，__clang__ 一定要排在 __GNUC__ 前面】
//   因為 clang 為了相容 GCC 的生態系，「也會」定義 __GNUC__。
//   本機 clang 21.1.8 實測，它定義的是：
//       #define __GNUC__ 4
//       #define __clang__ 1
//       #define __clang_major__ 21
//   注意 __GNUC__ 是 4 —— 一個刻意選的保守值（宣告「我至少相容 GCC 4.2」），
//   跟 clang 真正的版本 21 完全無關。所以：
//     * 先寫 #if defined(__clang__) 才問得出真正的編譯器；
//     * 拿 __GNUC__ 去做「GCC 版本夠不夠新」的判斷時，
//       在 clang 下會得到 4，判斷結果會錯得離譜。
//   本檔的 showCompilerInfo() 就是照正確順序寫的。
//
// 【5. MSVC 的 __cplusplus 陷阱】
//   MSVC 為了向後相容，預設把 __cplusplus 固定回報成 199711L，
//   就算你加了 /std:c++17 也一樣。必須額外加 /Zc:__cplusplus 才會回報真值。
//   這代表所有靠 __cplusplus 做版本判斷的程式碼，在 MSVC 上預設全部失準。
//   跨平台專案的慣例是改用 _MSVC_LANG（MSVC 專屬、永遠回報真值），
//   或乾脆一律要求加上 /Zc:__cplusplus。
//
// 【6. -Wall 不是 "all warnings"，這是史上最容易誤會的選項名】
//   本機實測：一段同時有「變數遮蔽（shadow）」與「double 隱式轉 int」的程式，
//       g++ -Wall -Wextra                        → 0 個警告
//       g++ -Wall -Wextra -Wshadow -Wconversion  → 2 個警告
//   -Wall 只是「常見且幾乎不會誤報」的那一組，-Wextra 再補一組。
//   真正嚴格的專案通常還會加 -Wshadow -Wconversion -Wpedantic
//   -Wold-style-cast -Wnon-virtual-dtor，甚至 -Werror 把警告當錯誤。
//
// 【7. -O 等級與除錯的取捨】
//   -O0：不最佳化。除錯時變數值與原始碼行號完全對得上，首選。
//   -O2：一般發布用。跨函式的 inline、迴圈變換都會發生，
//        單步除錯會出現「行號跳來跳去、變數被最佳化掉」。
//   -O3：更積極（更多 inline 與向量化），不保證一定比 -O2 快，要量測。
//   -Og：專為除錯設計的最佳化等級，比 -O0 快又保留可除錯性。
//   重要觀念：最佳化等級會改變「未定義行為的表現」。一段 UB 程式碼在 -O0
//   看似正常、在 -O2 壞掉，是非常典型的情形 —— 那不是編譯器的 bug，
//   而是原本就沒有保證的行為被最佳化器揭露出來。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 同一份原始碼、不同編譯器 → 不同輸出，本檔就是活教材
//   用 g++ 建置時第一行印出 "編譯器: GCC 15.2.0"；
//   本機用 clang++ -std=c++17 建置同一個檔案，同一行變成
//       編譯器: Clang 21.1.8
//   其餘各行（C++17、Linux、功能驗證）完全相同。
//   這正是條件編譯的價值：一份原始碼，各平台自己走自己的分支。
//
// (B) 為什麼 __cplusplus 印出來是 201703 而不是 201703L
//   L 只是「這個整數字面值的型別是 long」的來源碼標記，不是值的一部分。
//   送進 std::cout 時輸出的是數值本身，所以看到的是 201703。
//
// (C) 分步編譯為什麼能加快大型專案
//   g++ -c 只做到產生 .o。改了一個 .cpp 時，只需重編那一個檔再重新連結，
//   而不是整包重來。這就是 make／ninja 這類建置系統的基礎：
//   靠檔案時間戳判斷哪些 .o 過期了。代價是必須自己維護相依關係
//   （所以才有 g++ -MMD 自動產生 .d 相依檔這種選項）。
//
// (D) 標頭檔保護：#pragma once vs include guard
//   #pragma once 不是標準的一部分，但 GCC／Clang／MSVC 都支援，寫起來也
//   不會撞名。include guard（#ifndef／#define／#endif）是標準做法，
//   缺點是巨集名可能撞、複製貼上時容易忘了改。
//   兩者唯一真正有差別的邊角情境是「同一個檔案透過不同路徑（symlink、
//   硬連結）被 include 兩次」—— 這時 #pragma once 可能認不出是同一個檔。
//
// 【注意事項 Pay Attention】
//   1. 永遠明確寫出 -std=。不寫的話，g++ 15.2 預設是 gnu++17（帶 GNU 擴充），
//      你會在不知情的狀況下依賴非標準語法。
//   2. 驗證「某語法屬於哪個標準」要用 -pedantic-errors，不要用 -fsyntax-only。
//   3. 判斷編譯器時 __clang__ 必須排在 __GNUC__ 之前（clang 也定義 __GNUC__，
//      而且值是 4）。
//   4. MSVC 上的 __cplusplus 預設不可信，需 /Zc:__cplusplus 或改用 _MSVC_LANG。
//   5. 中文輸出在 Windows 上要加 /utf-8（MSVC），必要時再設定
//      SetConsoleOutputCP(65001)。Linux／macOS 端末預設 UTF-8，不需處理。
//   6. #ifdef _WIN32 區塊內不能寫 #include —— 本檔 main() 裡那兩行是
//      「示意用的註解」，不是可以直接取消註解的程式碼：
//      #include 必須寫在檔案的最外層，不能放進函式本體。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】編譯流程、編譯選項與條件編譯
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 從 .cpp 到執行檔，中間經過哪些階段？各自對應什麼錯誤訊息？
//     答：前置處理（展開 #include／巨集）→ 編譯（產生組合語言）→ 組譯
//         （產生 .o）→ 連結（合併 .o 與函式庫）。對應關係是：
//         找不到標頭檔＝前置處理階段；'x' was not declared＝編譯階段；
//         undefined reference＝連結階段。
//     追問：undefined reference to `foo()' 最常見的原因？
//         → 宣告了但沒把實作的 .cpp 一起編、或沒 -l 上對應函式庫。
//           另一個經典是 C 與 C++ 混用時忘了 extern "C"，
//           導致名稱修飾（name mangling）對不上。
//
// 🔥 Q2. -Wall 是不是「開啟所有警告」？
//     答：不是。它只是「常見且幾乎不會誤報」的一組。本機實測：一段同時有
//         變數遮蔽與 double→int 隱式轉換的程式碼，-Wall -Wextra 一個警告
//         都不報，加上 -Wshadow -Wconversion 才報出 2 個。
//     追問：那實務上該開到什麼程度？
//         → 新專案建議 -Wall -Wextra -Wpedantic 起跳，再視情況加
//           -Wshadow -Wconversion，並用 -Werror 讓警告無法被忽略。
//           舊專案則要漸進，一次全開會被淹沒。
//
// 🔥 Q3. 為什麼偵測編譯器時，__clang__ 要判斷在 __GNUC__ 之前？
//     答：因為 clang 為了相容 GCC 生態系也定義 __GNUC__。本機 clang 21.1.8
//         實測定義的是 __GNUC__ = 4（一個保守的相容性宣告），
//         與它真正的版本 21 無關。先問 __GNUC__ 會把 clang 誤判成 GCC 4。
//     追問：那要判斷「GCC 版本夠不夠新」該怎麼寫？
//         → 先排除 clang：#if defined(__GNUC__) && !defined(__clang__)，
//           再比較 __GNUC__。
//
// ⚠️ 陷阱 1. 「-fsyntax-only 編得過，就代表這個標準支援這個語法」——錯在哪？
//     答：g++ 會把許多較新或非標準的用法當作 GNU 擴充放行，只是給個警告
//         甚至不給。要驗證標準歸屬必須用 -pedantic-errors，
//         它會把「不符合所選 ISO 標準」的用法升級成錯誤。
//     為什麼會錯：把「這個編譯器接受」等同於「這個標準規定」。
//         編譯器預設是寬容的（預設還是 gnu++17 而非 c++17），
//         寬容模式下的成功不能拿來當標準依據。
//
// ⚠️ 陷阱 2. 「程式在 -O0 跑得好好的，開 -O2 就壞了，是編譯器 bug」——通常不是。
//     答：絕大多數是原本就存在的未定義行為（未初始化變數、越界、
//         違反嚴格別名、有號整數溢位）被最佳化器揭露。最佳化器有權
//         假設 UB 不會發生，並據此重排或刪除程式碼。
//     為什麼會錯：把「測試會過」當成「行為有保證」。-O0 只是碰巧
//         保留了你期待的行為，不是保證。遇到這種狀況先跑
//         -fsanitize=address,undefined，而不是先怪編譯器。
//
// ⚠️ 陷阱 3. 「MSVC 加了 /std:c++17，__cplusplus 就會是 201703L」——不會。
//     答：MSVC 為了向後相容，預設永遠回報 199711L，必須額外加
//         /Zc:__cplusplus 才會反映真實標準版本。
//     為什麼會錯：假設所有編譯器都照標準回報這個巨集。
//         跨平台程式碼若靠 __cplusplus 做特性判斷，在 MSVC 上會整片失準；
//         穩健的寫法是同時檢查 _MSVC_LANG。
// ═══════════════════════════════════════════════════════════════════════════

/*
 * ================================================================
 * 【第3課：C++ 編譯環境設置（g++、clang++）】總複習 summary.cpp
 * ================================================================
 * 編譯方式：g++ -std=c++17 -Wall -Wextra -o summary summary.cpp
 *
 * 本課重點：
 * 1. 三大主流 C++ 編譯器：g++、clang++、MSVC（cl）
 * 2. g++ 常用編譯選項（-std、-Wall、-Wextra、-g、-O2 等）
 * 3. MSVC 常用選項（/std:c++17、/W4、/Zi、/utf-8 等）
 * 4. 編譯器自動偵測巨集（__clang__、__GNUC__、_MSC_VER）
 * 5. C++ 標準版本偵測（__cplusplus）
 * 6. 跨平台作業系統偵測（_WIN32、__linux__、__APPLE__）
 * 7. 多檔案編譯概念（分開編譯再連結）
 * 8. 常見編譯錯誤與解決方法
 * ================================================================
 */

#include <iostream>
#include <string>
#include <vector>

// ===== 重點一：編譯器識別巨集 =====
// 說明：C++ 編譯器在編譯時會預先定義特定的巨集，讓程式碼
//       可以在執行前（編譯期）判斷使用的是哪個編譯器。
// 重要性：讓同一份原始碼能針對不同編譯器做不同處理，
//         例如使用某編譯器特有的最佳化選項或警告抑制。
// 使用方式：搭配 #if defined(...)  #elif  #else  #endif 使用。

void showCompilerInfo() {
    std::cout << "========================================\n";
    std::cout << "    [重點一] 編譯器識別\n";
    std::cout << "========================================\n";

    // __clang__ 由 Clang/clang++ 定義
    // __GNUC__  由 GCC/g++ 定義（注意 Clang 也會定義此值以保持相容性，
    //           所以必須先判斷 __clang__）
    // _MSC_VER  由 MSVC/cl 定義，其值為版本號（例如 1940 代表 VS 2022）
#if defined(__clang__)
    std::cout << "編譯器: Clang " << __clang_major__ << "."
              << __clang_minor__ << "." << __clang_patchlevel__ << "\n";
#elif defined(__GNUC__)
    std::cout << "編譯器: GCC " << __GNUC__ << "."
              << __GNUC_MINOR__ << "." << __GNUC_PATCHLEVEL__ << "\n";
#elif defined(_MSC_VER)
    std::cout << "編譯器: MSVC " << _MSC_VER << "\n";
    // 注意：MSVC 預設 __cplusplus 值是 199711，要正確反映版本
    // 需加上 /Zc:__cplusplus 編譯選項
#else
    std::cout << "編譯器: Unknown\n";
#endif
}

// ===== 重點二：C++ 標準版本偵測 =====
// 說明：__cplusplus 是一個特殊巨集，其值代表正在使用的 C++ 標準。
//       C++11=201103L、C++14=201402L、C++17=201703L、C++20=202002L
// 重要性：可在程式碼中根據標準版本條件性地啟用新功能，
//         確保向後相容性。
// 使用方式：g++ 需加 -std=c++17；MSVC 需加 /std:c++17 /Zc:__cplusplus

void showCppStandard() {
    std::cout << "\n[重點二] C++ 標準版本: ";

#if __cplusplus >= 202302L
    std::cout << "C++23";
#elif __cplusplus >= 202002L
    std::cout << "C++20";
#elif __cplusplus >= 201703L
    std::cout << "C++17";
#elif __cplusplus >= 201402L
    std::cout << "C++14";
#elif __cplusplus >= 201103L
    std::cout << "C++11";
#elif __cplusplus >= 199711L
    std::cout << "C++98/03";
#else
    std::cout << "Pre-C++98";
#endif

    std::cout << "  (__cplusplus = " << __cplusplus << ")\n";
}

// ===== 重點三：作業系統偵測 =====
// 說明：編譯器根據目標平台自動定義對應的巨集。
//       _WIN32 / _WIN64 → Windows（32位或64位）
//       __linux__        → Linux
//       __APPLE__        → macOS / iOS
// 重要性：撰寫跨平台程式碼時，可針對不同 OS 執行不同邏輯，
//         例如路徑分隔符、API 呼叫、文字編碼設定等。

void showPlatformInfo() {
    std::cout << "\n[重點三] 作業系統: ";

#if defined(_WIN32) || defined(_WIN64)
    std::cout << "Windows";
    #if defined(_WIN64)
        std::cout << " (64-bit)";
    #else
        std::cout << " (32-bit)";
    #endif
#elif defined(__linux__)
    std::cout << "Linux";
#elif defined(__APPLE__)
    std::cout << "macOS";
#else
    std::cout << "Unknown";
#endif

    std::cout << "\n";
}

// ===== 重點四：常用 g++ 編譯選項速查 =====
// 說明：正確使用編譯選項可以及早發現 bug 並控制最佳化程度。
//
// 開發階段建議指令：
//   g++ -std=c++17 -Wall -Wextra -g -O0 程式.cpp -o 程式
//
//   -std=c++17  → 使用 C++17 標準
//   -Wall       → 啟用大部分常見警告
//   -Wextra     → 啟用額外的警告（如未使用參數）
//   -g          → 加入除錯資訊（GDB 可用）
//   -O0         → 停用最佳化，方便除錯
//
// 發布階段建議指令：
//   g++ -std=c++17 -Wall -Wextra -O2 程式.cpp -o 程式
//
//   -O2         → 啟用中等最佳化，平衡速度與二進位大小
//
// MSVC 等效選項：
//   /std:c++17  → 指定 C++ 標準
//   /W4         → 警告等級 4（相當於 -Wall -Wextra）
//   /Zi         → 加入除錯資訊
//   /Od         → 停用最佳化
//   /O2         → 速度最佳化
//   /utf-8      → 原始碼與執行期使用 UTF-8（處理中文必備）
//   /EHsc       → 啟用 C++ 例外處理

// ===== 重點五：C++11 以後的基礎新功能（測試編譯環境） =====
// 說明：以下功能可用來驗證編譯環境是否正確支援現代 C++。
//       若這些程式碼無法編譯，請確認 -std=c++11 或以上版本。

void testModernCppFeatures() {
    std::cout << "\n========================================\n";
    std::cout << "    [重點五] 現代 C++ 功能驗證\n";
    std::cout << "========================================\n";

    // auto 型別推導（C++11）
    // 重要性：減少冗長的型別宣告，讓程式碼更簡潔
    auto number = 42;
    auto pi = 3.14159;
    std::cout << "auto 推導 int: " << number << "\n";
    std::cout << "auto 推導 double: " << pi << "\n";

    // 範圍式 for 迴圈（C++11）
    // 重要性：更安全、更易讀地遍歷容器，無需手動管理索引
    std::vector<int> vec = {10, 20, 30, 40, 50};
    std::cout << "範圍 for 遍歷 vector: ";
    for (const auto& v : vec) {
        std::cout << v << " ";
    }
    std::cout << "\n";

    // Lambda 表達式（C++11）
    // 重要性：可就地定義匿名函式，常用於排序、演算法、回呼等場景
    auto multiply = [](int a, int b) -> int { return a * b; };
    std::cout << "lambda 計算 6 * 7 = " << multiply(6, 7) << "\n";

    // 初始化列表（C++11）
    // 重要性：更一致的物件初始化語法
    std::vector<std::string> fruits = {"蘋果", "香蕉", "橘子"};
    std::cout << "水果清單: ";
    for (const auto& fruit : fruits) {
        std::cout << fruit << " ";
    }
    std::cout << "\n";
}

// ===== 重點六：多檔案編譯概念 =====
// 說明：當程式變大，通常將程式碼拆分為多個 .cpp 與 .h 檔案。
//       編譯流程：
//         1. 前置處理（展開 #include、#define）
//         2. 編譯（.cpp → .o 目標檔）
//         3. 連結（多個 .o → 執行檔）
//
// 一次編譯多個 .cpp：
//   g++ -std=c++17 main.cpp calculator.cpp -o program
//
// 分步編譯（適合大型專案，只重新編譯修改過的檔案）：
//   g++ -std=c++17 -c main.cpp -o main.o
//   g++ -std=c++17 -c calculator.cpp -o calculator.o
//   g++ main.o calculator.o -o program
//
// 標頭檔保護（防止重複 include）：
//   #ifndef CALCULATOR_H
//   #define CALCULATOR_H
//   // ... 宣告內容 ...
//   #endif
//
// 或現代寫法（C++11 以後主流編譯器支援）：
//   #pragma once

// ===== 重點七：常見編譯錯誤速查 =====
// 說明：
//
// 錯誤一：'g++' is not recognized...
//   原因：編譯器未安裝或路徑未加入 PATH 環境變數
//   解法：重新安裝編譯器，並確認 PATH 設定
//
// 錯誤二：fatal error: iostream: No such file or directory
//   原因：編譯器安裝不完整，或 include 路徑設定錯誤
//   解法：重新安裝完整的 g++ 工具鏈
//
// 錯誤三：undefined reference to `function_name'
//   原因：宣告了函式但編譯時沒有包含對應的 .cpp 實作檔
//   解法：確認所有 .cpp 都有被列入編譯指令
//
// 錯誤四：error: 'auto' changes meaning in C++11
//   原因：未指定 C++ 標準，預設為舊版本
//   解法：加上 -std=c++11（或更高版本）

// ===== 【日常實務範例 1】把建置資訊做成 --version / bug report 橫幅 =====
// 情境：使用者回報「程式在我這台會當」，你第一件事一定是問「你用的是哪個
//       建置？」。成熟的專案都會提供一段可貼上的建置指紋，讓 issue 一開就
//       帶著編譯器、標準版本、平台、建置型態，省掉好幾輪來回。
// 為什麼用到本主題：這段字串完全由本課的預定義巨集在「編譯期」組出來，
//       執行期不需要任何額外資訊，也不會因為執行環境而改變。
// 刻意不使用 __DATE__ / __TIME__：那會讓每次重建產生不同輸出，
//       破壞可重現建置（reproducible build），也讓自動化測試無法比對。
std::string buildFingerprint() {
    std::string s;

#if defined(__clang__)
    s += "clang++ ";
    s += std::to_string(__clang_major__) + "." + std::to_string(__clang_minor__);
#elif defined(__GNUC__)
    s += "g++ ";
    s += std::to_string(__GNUC__) + "." + std::to_string(__GNUC_MINOR__);
#elif defined(_MSC_VER)
    s += "msvc " + std::to_string(_MSC_VER);
#else
    s += "unknown-compiler";
#endif

    s += " | C++";
#if   __cplusplus >= 202302L
    s += "23";
#elif __cplusplus >= 202002L
    s += "20";
#elif __cplusplus >= 201703L
    s += "17";
#elif __cplusplus >= 201402L
    s += "14";
#elif __cplusplus >= 201103L
    s += "11";
#else
    s += "98";
#endif

    // __STRICT_ANSI__ 只有在 -std=c++NN（嚴格 ISO）時才會被定義；
    // 用 -std=gnu++NN 或完全不指定時不會定義 —— 正好可以用來自我檢查
    // 「我到底是不是在嚴格模式下建置的」。
#if defined(__STRICT_ANSI__)
    s += " (strict ISO)";
#else
    s += " (GNU extensions on)";
#endif

    s += " | ";
#if defined(_WIN32)
    s += "Windows";
#elif defined(__linux__)
    s += "Linux";
#elif defined(__APPLE__)
    s += "macOS";
#else
    s += "unknown-os";
#endif

    // 指標大小是判斷 32/64 位元最可靠的方式（比巨集更不會出錯）
    s += "-" + std::to_string(sizeof(void*) * 8) + "bit";

    s += " | ";
#if defined(NDEBUG)
    s += "Release(NDEBUG)";
#else
    s += "Debug(assert 啟用)";
#endif

    return s;
}

// ===== 【日常實務範例 2】跨平台的路徑分隔符與換行 =====
// 情境：同一份工具要在 Windows 與 Linux 上組出設定檔路徑。硬寫 "\\" 或 "/"
//       都會在另一邊出事，正確做法是讓編譯期決定。
// 為什麼用到本主題：這是條件編譯最典型、也最常被寫錯的用途 ——
//       重點是「差異只寫一次」，其餘程式碼完全不必知道自己在哪個平台。
// 註：C++17 起 <filesystem> 的 std::filesystem::path 已能自動處理分隔符，
//     新程式碼應優先使用它；這裡示範的是它出現之前（或不能用時）的做法。
namespace platform {
#if defined(_WIN32)
    constexpr char        kSep      = '\\';
    constexpr const char* kNewline  = "\r\n";
    constexpr const char* kHomeVar  = "USERPROFILE";
#else
    constexpr char        kSep      = '/';
    constexpr const char* kNewline  = "\n";
    constexpr const char* kHomeVar  = "HOME";
#endif
}

std::string joinPath(const std::string& dir, const std::string& file) {
    if (dir.empty()) return file;
    if (dir.back() == platform::kSep) return dir + file;
    return dir + platform::kSep + file;
}

void showPracticalExamples() {
    std::cout << "\n========================================\n";
    std::cout << "    [日常實務] 建置指紋與跨平台路徑\n";
    std::cout << "========================================\n";
    std::cout << "建置指紋: " << buildFingerprint() << "\n";
    std::cout << "設定檔路徑: "
              << joinPath(joinPath("etc", "myapp"), "config.ini") << "\n";
    std::cout << "家目錄環境變數名稱: " << platform::kHomeVar << "\n";
    std::cout << "換行位元組數: "
              << std::string(platform::kNewline).size() << "\n";
}
int main() {
    // 設定 Windows 控制台 UTF-8 輸出（編譯時加 /utf-8 或執行前設定）
#ifdef _WIN32
    // 若輸出中文出現亂碼，可取消以下兩行的註解：
    // #include <windows.h>
    // SetConsoleOutputCP(65001);
#endif

    std::cout << "================================================================\n";
    std::cout << "  第3課：C++ 編譯環境設置 總複習\n";
    std::cout << "================================================================\n";

    showCompilerInfo();
    showCppStandard();
    showPlatformInfo();
    testModernCppFeatures();
    showPracticalExamples();

    std::cout << "\n================================================================\n";
    std::cout << "  本課重點速查\n";
    std::cout << "================================================================\n";
    std::cout << "  開發編譯：g++ -std=c++17 -Wall -Wextra -g -O0 程式.cpp -o 程式\n";
    std::cout << "  發布編譯：g++ -std=c++17 -Wall -Wextra -O2 程式.cpp -o 程式\n";
    std::cout << "  MSVC開發：cl /std:c++17 /utf-8 /EHsc /W4 /Zi 程式.cpp\n";
    std::cout << "  多檔案：  g++ -std=c++17 main.cpp other.cpp -o program\n";
    std::cout << "================================================================\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra summary.cpp -o summary

// 【輸出會隨「誰來編譯」而改變 —— 這正是本課的重點】
//   下方預期輸出取自本機 g++ (Ubuntu 15.2.0-16ubuntu1) 15.2.0、
//   -std=c++17、x86-64 Linux、未定義 NDEBUG 的建置。
//   換一個編譯器／平台，以下幾行「必然」不同（其餘各行相同）：
//     * 「編譯器:」那一行 —— 本機用 clang++ -std=c++17 建置同一個檔案，
//       實測會變成： 編譯器: Clang 21.1.8
//     * 「建置指紋:」那一行 —— clang 建置時前綴會變成 clang++ 21.1
//     * Windows 上「作業系統」會是 Windows，路徑分隔符變成反斜線，
//       換行位元組數變成 2（CRLF）。
//   若加上 -O2 -DNDEBUG，建置指紋結尾會由 Debug(assert 啟用) 變成
//   Release(NDEBUG)。這些差異都是編譯期決定的，不是執行期。
//
// 【為何本檔沒有 LeetCode 範例】
//   本課主題是工具鏈與條件編譯（編譯器選項、預定義巨集、連結流程），
//   完全不是演算法題型。LeetCode 上沒有任何一題在考 -std / -Wall /
//   __GNUC__ / undefined reference，硬掛一題只會製造假關聯，故從缺。
//   本檔改以兩個真實情境（建置指紋橫幅、跨平台路徑組合）呈現實戰價值。

// === 預期輸出 ===
// ================================================================
//   第3課：C++ 編譯環境設置 總複習
// ================================================================
// ========================================
//     [重點一] 編譯器識別
// ========================================
// 編譯器: GCC 15.2.0
//
// [重點二] C++ 標準版本: C++17  (__cplusplus = 201703)
//
// [重點三] 作業系統: Linux
//
// ========================================
//     [重點五] 現代 C++ 功能驗證
// ========================================
// auto 推導 int: 42
// auto 推導 double: 3.14159
// 範圍 for 遍歷 vector: 10 20 30 40 50
// lambda 計算 6 * 7 = 42
// 水果清單: 蘋果 香蕉 橘子
//
// ========================================
//     [日常實務] 建置指紋與跨平台路徑
// ========================================
// 建置指紋: g++ 15.2 | C++17 (strict ISO) | Linux-64bit | Debug(assert 啟用)
// 設定檔路徑: etc/myapp/config.ini
// 家目錄環境變數名稱: HOME
// 換行位元組數: 1
//
// ================================================================
//   本課重點速查
// ================================================================
//   開發編譯：g++ -std=c++17 -Wall -Wextra -g -O0 程式.cpp -o 程式
//   發布編譯：g++ -std=c++17 -Wall -Wextra -O2 程式.cpp -o 程式
//   MSVC開發：cl /std:c++17 /utf-8 /EHsc /W4 /Zi 程式.cpp
//   多檔案：  g++ -std=c++17 main.cpp other.cpp -o program
// ================================================================
