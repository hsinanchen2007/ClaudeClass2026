// =============================================================================
//  第 5 課：輸入輸出流（iostream）入門2.cpp  —  operator<< 的鏈式輸出（chaining）
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<iostream>
//   核心簽名（成員版，處理內建型別）：
//     std::ostream& std::ostream::operator<<(int    value);
//     std::ostream& std::ostream::operator<<(double value);
//   核心簽名（非成員版，處理 char / C-string / std::string）：
//     std::ostream& operator<<(std::ostream& os, const char* s);
//     std::ostream& operator<<(std::ostream& os, const std::string& s); // 宣告於 <string>
//   標準版本：C++98 起即有；std::endl 亦為 C++98。
//   複雜度：每次插入 O(輸出字元數)。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼 a << b << c 串得起來？——回傳 ostream& 這個設計】
//   關鍵只有一句話：operator<< 回傳的是「同一個 stream 的 reference」。
//   所以 std::cout << "年齡: " << age 實際上是：
//       ( (std::cout << "年齡: ") << age )
//   內層先執行，回傳 std::cout 本身的 reference，外層再拿它繼續插入。
//   這是 C++ 的 fluent interface（流暢介面）最早、也最經典的應用。
//
//   為什麼回傳 reference 而不是 value？
//     * std::ostream 的 copy constructor 被 delete（C++11 起明確標為 = delete），
//       stream 本來就代表一個「唯一的輸出裝置」，複製沒有意義。
//     * 回傳 reference 沒有任何複製成本。
//
// 【2. operator<< 為什麼是「左結合」——這不是巧合，是必要條件】
//   C++ 的 << 運算子結合性是 left-to-right（左結合）。
//   若它是右結合，a << b << c 會變成 a << (b << c)，(b << c) 是「整數 b 左移 c 位」，
//   整個鏈式輸出就完全不成立。C++ 不允許改變運算子的結合性，
//   剛好左結合天生就符合「由左往右依序輸出」的直覺。
//
// 【3. 為什麼是 << 這個符號？】
//   << 原本是「位元左移」。Bjarne Stroustrup 選它有幾個理由：
//     * 視覺上像「資料流向 cout」的箭頭。
//     * 優先級比 = 高、比 == 低——但這也埋了一個經典陷阱（見【注意事項】第 2 點）。
//
// 【4. 型別自動分派：不需要 printf 的 %d / %f】
//   printf("%d", x) 是 C 的 variadic function，格式字串與參數型別由「人」保證一致，
//   型別錯了是 undefined behavior。
//   operator<< 則是一整組 overload，編譯期由 overload resolution 挑對應版本，
//   型別錯配是 compile error 而非 runtime UB——這是 iostream 相對 printf
//   最重要的安全性優勢。
//
// 【概念補充 Concept Deep Dive】
//   (A) 一行 std::cout << a << b << c 編譯後長什麼樣？
//       它不是一個函式呼叫，而是 N 個函式呼叫串成的表達式；
//       每個呼叫都吃一個 ostream& 並回傳同一個 ostream&。
//       iostream 大量程式碼位於 libstdc++ 之中，這也是 iostream 版本的執行檔
//       通常比 printf 版本大的原因之一。
//
//   (B) 求值順序：C++17 之前這裡有個真實地雷
//       C++17 之前，std::cout << f() << g() 中 f() 與 g() 的「呼叫順序」是
//       unspecified，只有「插入動作」的先後被保證。
//       C++17 起（P0145R3）明確規定 operator<< 的左運算元先於右運算元求值，
//       順序才被定死為由左至右。所以在 C++17 之前寫
//       std::cout << i++ << i; 是不可靠的（且對同一物件做兩次修改還可能是 UB）。
//
//   (C) std::endl 不是 '\n'
//       std::endl 是一個 manipulator function，等價於 os.put('\n') 之後再 os.flush()。
//       flush 會強迫把緩衝區內容交給作業系統。
//       在迴圈裡大量使用 std::endl 會產生大量 flush，是常見的效能問題。
//
// 【注意事項 Pay Attention】
//   1. std::endl 會 flush，'\n' 不會。除非真的需要立即可見（例如即將 abort 前的
//      除錯輸出、或輸出到 pipe 給另一支程式即時消費），否則優先用 '\n'。
//   2. 優先級陷阱：<< 的優先級比 == 高。
//        std::cout << a == b;      // 解析成 (std::cout << a) == b —— 編譯錯誤
//        std::cout << (a == b);    // 正確：必須自己加括號
//      同理 std::cout << a ? "y" : "n"; 也會解析錯。
//   3. std::cout << 'A' 印出字元 A；std::cout << 65 印出數字 65。
//      char 與 int 走不同 overload，型別決定一切。
//   4. 鏈式輸出中若有一段輸出失敗（例如磁碟滿），stream 會進入 fail 狀態，
//      後續插入全部變成 no-op 而「靜默失敗」——預設不會丟例外
//      （除非你事先呼叫 os.exceptions(...) 開啟例外）。
//   5. manipulator（如 std::fixed、std::setprecision）設定的是 stream 的
//      「持續性狀態」，不是只影響下一個輸出項；設了就會一直生效到被改掉。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】operator<< 鏈式輸出
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::cout << a << b 為什麼串得起來？請說明背後機制。
//     答：operator<< 回傳 std::ostream&（同一個 stream 的 reference），
//         且 << 是左結合，所以 (cout << a) 先求值並回傳 cout，
//         外層再以這個 cout 繼續插入 b。回傳 reference 而非 value 是必要的，
//         因為 std::ostream 的 copy constructor 被 delete。
//     追問：那 operator>> 呢？→ 同理回傳 std::istream&，所以 cin >> a >> b 也能串。
//
// 🔥 Q2. std::endl 和 '\n' 差在哪？什麼時候該用 endl？
//     答：endl = 輸出 '\n' + flush；'\n' 只輸出換行字元。
//         flush 會觸發實際的寫出動作，迴圈中濫用會明顯拖慢輸出。
//         只有在「需要保證輸出立刻被看到」時才用 endl，例如程式可能即將 abort、
//         或輸出正被另一支程式即時讀取。
//     追問：程式正常結束時沒 flush 的內容會遺失嗎？
//         → 不會。std::cout 在正常結束（從 main 回傳或呼叫 exit）時會被 flush。
//           但 abort() 或被未處理的 signal 終止時，緩衝內容確實可能遺失。
//
// ⚠️ 陷阱. 下面這行為什麼不是你以為的結果？
//         int a = 3, b = 3;
//         std::cout << a == b << std::endl;
//     答：<< 的優先級高於 ==，所以解析成 (std::cout << a) == b，
//         也就是拿「一個 ostream」和「int b」比較——這無法通過編譯。
//         正確寫法是自己加括號：std::cout << (a == b) << std::endl;
//         （預設格式下印出 1；若該 stream 先前設過 std::boolalpha 則印出 true——
//           本檔的實測輸出正是 true，因為前一段示範已經開啟了 boolalpha）。
//     為什麼會錯：多數人腦中把 << 當成「一個特殊的輸出語法」而不是「一個運算子」，
//         就忘了它必須遵守運算子優先級表。凡是 << 右邊出現 == != < > ?: 一律加括號。
// ═══════════════════════════════════════════════════════════════════════════

#include <iomanip>
#include <iostream>
#include <string>

// -----------------------------------------------------------------------------
// 【日常實務範例 1】組出一行結構化的存取 log
// 情境：伺服器每處理完一個 request 就寫一行 log，欄位固定、以空白分隔，
//       方便之後用 awk / grep 分析。鏈式輸出讓「一行 = 一個表達式」，
//       比反覆做字串相加清楚，也不需要中間的暫存 std::string。
// 為何用本主題：這正是 operator<< 回傳 ostream& 的最典型用途——
//       把不同型別（string / int / double）串在同一條輸出鏈上。
// -----------------------------------------------------------------------------
void writeAccessLog(std::ostream& os,
                    const std::string& method,
                    const std::string& path,
                    int statusCode,
                    double elapsedMs) {
    // 這裡刻意接受 std::ostream& 而不是寫死 std::cout，
    // 同一個函式就能輸出到 cout / 檔案 / std::ostringstream（方便單元測試）。
    os << method << ' ' << path
       << " status=" << statusCode
       << " elapsed=" << std::fixed << std::setprecision(2) << elapsedMs << "ms"
       << '\n';   // 用 '\n' 不用 endl：log 是高頻輸出，不該每行都 flush
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】輸出 CSV 的一列
// 情境：把記憶體中的資料匯出成 CSV 給試算表或 pandas 讀。
// 為何用本主題：分隔符與欄位交錯出現，鏈式輸出寫起來最貼近「格式的長相」。
// -----------------------------------------------------------------------------
void writeCsvRow(std::ostream& os,
                 const std::string& name,
                 int age,
                 double height) {
    os << name << ',' << age << ',' << height << '\n';
}

// 【LeetCode 實戰範例】—— 本檔從缺，並說明理由
//   operator<< 的鏈式輸出屬於「語言機制／I-O 介面設計」，
//   不對應任何一個 LeetCode 演算法題（LeetCode 只看函式回傳值，
//   多數題目甚至不需要印任何東西到 stdout）。
//   硬套一題不相關的演算法只會誤導，因此本檔刻意不放 LeetCode 範例。

int main() {
    // ── 原始教學範例：最基本的鏈式輸出 ─────────────────────────
    std::cout << "=== 基本鏈式輸出 ===" << std::endl;
    int age = 25;
    double height = 175.5;

    // 一個表達式，多次 operator<< 呼叫，全部作用在同一個 std::cout 上
    std::cout << "年齡: " << age << " 歲，身高: " << height << " cm" << std::endl;

    // ── 拆解：證明 operator<< 真的回傳同一個 stream ─────────────
    std::cout << "\n=== 證明回傳的是同一個 stream ===" << std::endl;
    std::ostream& r = (std::cout << "先輸出這段，");
    // 印布林值而不是位址：位址每次執行都可能不同，不適合當穩定的預期輸出
    std::cout << "\n回傳的 reference 是否就是 std::cout: "
              << std::boolalpha << (&r == &std::cout) << '\n';

    // ── 優先級陷阱示範（正確寫法） ─────────────────────────────
    std::cout << "\n=== 優先級：<< 右邊有 == 一定要加括號 ===" << std::endl;
    int a = 3, b = 3;
    // std::cout << a == b;      // ← 這行無法編譯：解析成 (std::cout << a) == b
    // 注意：上一段設了 std::boolalpha 且尚未還原，所以這裡印的是 true 而非 1
    std::cout << "a == b 的結果: " << (a == b) << '\n';   // 加了括號才正確

    // ── char 與 int 走不同 overload ────────────────────────────
    std::cout << "\n=== 型別決定 overload ===" << std::endl;
    char ch = 'A';
    std::cout << "char 'A' 印出: " << ch << '\n';
    std::cout << "int  65  印出: " << 65 << '\n';
    std::cout << "把 char 轉成 int: " << static_cast<int>(ch) << '\n';

    // ── 實務範例 1：存取 log ──────────────────────────────────
    std::cout << "\n=== 日常實務 1: 存取 log ===" << std::endl;
    writeAccessLog(std::cout, "GET",  "/api/users",  200, 12.3456);
    writeAccessLog(std::cout, "POST", "/api/orders", 500, 1043.7);

    // ── 實務範例 2：CSV ───────────────────────────────────────
    std::cout << "\n=== 日常實務 2: CSV 匯出 ===" << std::endl;
    std::cout << "name,age,height\n";          // header
    writeCsvRow(std::cout, "Alice", 30, 165.5);
    writeCsvRow(std::cout, "Bob",   25, 178.0);

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 5 課：輸入輸出流（iostream）入門2.cpp" -o io2

// 說明（格式狀態的持續性，非不確定行為）：
//   1. std::boolalpha 在「證明回傳同一個 stream」那段被設定後就一直生效，
//      所以後面 (a == b) 印出的是 true 而不是 1。
//   2. writeAccessLog 內設定了 std::fixed << std::setprecision(2)，
//      同樣是「黏在 stream 上的持續性狀態」，不會在函式返回時自動還原。
//      因此後面 CSV 段落的浮點數也以兩位小數印出（165.50 / 178.00），
//      而不是預設格式的 165.5 / 178。
//   這兩點都是刻意保留的示範：manipulator 是狀態，不是一次性效果。
//   若要還原可用 std::defaultfloat / std::noboolalpha，
//   或自行以 RAII 保存並還原 stream 的 flags。
//   以下輸出為實跑擷取（本程式輸出完全決定性，多次執行結果相同）。

// === 預期輸出 ===
// === 基本鏈式輸出 ===
// 年齡: 25 歲，身高: 175.5 cm
//
// === 證明回傳的是同一個 stream ===
// 先輸出這段，
// 回傳的 reference 是否就是 std::cout: true
//
// === 優先級：<< 右邊有 == 一定要加括號 ===
// a == b 的結果: true
//
// === 型別決定 overload ===
// char 'A' 印出: A
// int  65  印出: 65
// 把 char 轉成 int: 65
//
// === 日常實務 1: 存取 log ===
// GET /api/users status=200 elapsed=12.35ms
// POST /api/orders status=500 elapsed=1043.70ms
//
// === 日常實務 2: CSV 匯出 ===
// name,age,height
// Alice,30,165.50
// Bob,25,178.00
