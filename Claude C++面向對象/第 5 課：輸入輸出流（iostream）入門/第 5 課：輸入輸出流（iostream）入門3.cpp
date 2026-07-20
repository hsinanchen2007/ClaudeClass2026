// =============================================================================
//  第 5 課：輸入輸出流（iostream）入門3.cpp  —  型別安全輸出：overload resolution
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<iostream>（std::string 的 operator<< 宣告在 <string>）
//   std::ostream 提供的成員 operator<< overload（節錄）：
//     ostream& operator<<(bool);            ostream& operator<<(short);
//     ostream& operator<<(int);             ostream& operator<<(long);
//     ostream& operator<<(unsigned);        ostream& operator<<(long long);
//     ostream& operator<<(float);           ostream& operator<<(double);
//     ostream& operator<<(long double);     ostream& operator<<(const void*);
//   非成員 overload（節錄）：
//     ostream& operator<<(ostream&, char);
//     ostream& operator<<(ostream&, const char*);
//     ostream& operator<<(ostream&, const std::string&);
//   標準版本：C++98 起；std::boolalpha 亦為 C++98。
//
// 【詳細解釋 Explanation】
//
// 【1. 「不需要格式符」的真正意思：型別資訊沒有被丟掉】
//   printf("%d", x) 之所以需要 %d，是因為 printf 是 C 的 variadic function：
//   參數經過 default argument promotion 之後，型別資訊在呼叫端就消失了，
//   printf 只能靠你寫的格式字串「猜」棧上是什麼。猜錯 = undefined behavior。
//
//   operator<< 完全相反：它是一整組 overload，
//   編譯器在編譯期用 overload resolution 依「實際的靜態型別」挑函式。
//   型別資訊從頭到尾都在，所以
//     * 不需要格式符
//     * 型別對不上是 compile error，不是 runtime 崩潰
//   這就是 iostream 被稱為 type-safe I/O 的原因。
//
// 【2. 為什麼 bool 印出 1 而不是 true】
//   ostream 有一個 operator<<(bool) 的 overload，所以 bool 並沒有被當成 int，
//   它確實走了自己的版本。只是這個版本的預設行為是「印 0 或 1」。
//   要改成 true/false，得打開 boolalpha 這個 format flag：
//       std::cout << std::boolalpha << b;      // true
//       std::cout << std::noboolalpha << b;    // 1
//   boolalpha 是 stream 的持續性狀態（sticky），設了就一直有效。
//
// 【3. char / const char* / void* 三者的分歧——最容易出事的地方】
//   * char       → 印出那個「字元」（走 operator<<(ostream&, char)）
//   * const char*→ 印出「以 '\0' 結尾的字串內容」（走專屬的字串 overload）
//   * 其它指標   → 印出「位址」（走 operator<<(const void*)）
//   注意第二與第三條的落差：只有 char 系列的指標會被當字串印，
//   其它任何指標（int*、自訂型別*）都會退到 const void* 而印出位址。
//   反過來說，如果你想印 char* 的「位址」而不是內容，必須顯式轉型：
//       std::cout << static_cast<const void*>(s);
//
// 【4. 整數提升不會發生在這裡】
//   有人以為 char 會被提升成 int 再印出數字——不會。
//   overload resolution 會優先選「精確匹配」的 operator<<(ostream&, char)，
//   完全不需要提升。要印數值必須自己轉：static_cast<int>(c)。
//
// 【概念補充 Concept Deep Dive】
//   (A) 為什麼有些 overload 是成員函式、有些是非成員函式？
//       成員版處理的是「數值型別」（bool/int/double/void* …），
//       這些是 ostream 自己就知道怎麼格式化的東西。
//       非成員版處理 char、const char*、std::string 等——
//       尤其 std::string 定義在 <string> 而非 <iostream>，
//       它不可能是 ostream 的成員（ostream 不該依賴 string）。
//       這也是為什麼「忘了 #include <string> 卻 cout << 一個 std::string」
//       在某些標準庫版本下會編譯失敗。
//
//   (B) 浮點數的預設格式
//       預設是 defaultfloat + precision 6，語意接近 printf 的 %g：
//       總共最多 6 位有效數字，且會去掉無意義的尾隨 0。
//       所以 3.14159 剛好 6 位有效數字，完整印出；
//       但 3.141592653 會被截成 3.14159。
//       要控制位數需自行使用 std::setprecision / std::fixed（<iomanip>）。
//
//   (C) unsigned char 與 signed char 也走「字元」版本
//       ostream 對 signed char 與 unsigned char 都有非成員 overload，
//       一樣印字元而非數字。所以 uint8_t（在多數平台就是 unsigned char）
//       印出來是亂碼字元而不是數字——這是很常見的除錯困擾。
//
// 【注意事項 Pay Attention】
//   1. bool 預設印 0/1，需要 std::boolalpha 才印 true/false，且該 flag 是 sticky。
//   2. uint8_t / int8_t 通常就是 char 的 typedef，印出來會是字元不是數字；
//      要印數值請 static_cast<int>(x) 或 +x（unary plus 會促成整數提升）。
//   3. 印 char* 的位址要 static_cast<const void*>，否則會被當字串內容印出。
//      若那塊記憶體不是以 '\0' 結尾的合法字串，讀取會越界，屬 undefined behavior。
//   4. 浮點數預設只有 6 位有效數字，不是「原樣印出」。金額、座標等場合請明確設定格式。
//   5. 印一個 nullptr 的 const char* 是 undefined behavior（不保證印出 "(null)"，
//      那是某些 printf 實作的擴充行為，iostream 並無此保證）。本檔不做此示範。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】iostream 的型別安全與 overload resolution
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 iostream 不需要 %d / %f 這種格式符？跟 printf 的差別在哪？
//     答：printf 是 variadic function，型別資訊在呼叫端就丟失了，只能靠格式字串猜；
//         猜錯是 undefined behavior。operator<< 是一整組 overload，
//         編譯期依實際型別做 overload resolution，型別資訊全程保留，
//         錯配會變成 compile error。代價是產生的程式碼較多、編譯較慢。
//     追問：那 C++20 的 std::format 呢？
//         → std::format 同時拿到兩者的好處：保留 printf 風格的格式字串，
//           但在編譯期（consteval）檢查格式字串與引數型別是否相符。
//
// 🔥 Q2. std::cout << someBool 印出什麼？想印 true/false 要怎麼做？
//     答：預設印 1 或 0（bool 有自己的 overload，只是預設格式是數字）。
//         要印 true/false 需 std::cout << std::boolalpha << someBool，
//         且 boolalpha 是 sticky 的 format flag，設定後會一直影響同一個 stream，
//         需要還原時用 std::noboolalpha。
//
// ⚠️ 陷阱. 為什麼下面兩行印出完全不同的東西？
//         char        c = 'A';   std::cout << c;   // 印 A
//         std::uint8_t u = 65;   std::cout << u;   // 也印 A，不是 65
//     答：uint8_t 在絕大多數平台上就是 unsigned char 的 typedef，
//         而 ostream 對 unsigned char 有「字元」版本的 overload，
//         所以它被當成字元印出，而不是數值。
//     為什麼會錯：大家把 uint8_t 看成「一個 8 位元的整數型別」，
//         以為它跟 int 只差在寬度。實際上它是 char 家族的成員，
//         在 I/O 與型別推導上的行為跟整數完全不同。
//         要印數值請寫 static_cast<int>(u)，或用 unary plus：std::cout << +u;
// ═══════════════════════════════════════════════════════════════════════════

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string>

// -----------------------------------------------------------------------------
// 【日常實務範例】把不同型別的設定值印成人類可讀的設定摘要
// 情境：服務啟動時把生效的設定 dump 到 log，方便事後追查「當時到底跑什麼設定」。
//       設定值型別各異（string / int / bool / double），正是 operator<< 型別
//       自動分派的實際用途；bool 一定要開 boolalpha，否則 log 裡滿滿的 0/1
//       事後根本看不懂哪個欄位是開還是關。
// 為何用本主題：示範 boolalpha 的必要性，以及「同一個函式介面吃不同型別」。
// -----------------------------------------------------------------------------
void dumpConfig(std::ostream& os,
                const std::string& serviceName,
                int port,
                bool tlsEnabled,
                double timeoutSec) {
    os << std::boolalpha;                 // 關鍵：讓 bool 印成 true/false
    os << "service       = " << serviceName  << '\n'
       << "port          = " << port         << '\n'
       << "tls_enabled   = " << tlsEnabled   << '\n'
       << "timeout_sec   = " << timeoutSec   << '\n';
    os << std::noboolalpha;               // 用完還原，避免影響呼叫者
}

// 【LeetCode 實戰範例】—— 本檔從缺，並說明理由
//   本檔主題是「編譯期 overload resolution 與 I-O 格式化」，
//   屬於語言與標準庫機制，不對應任何 LeetCode 演算法題。
//   與其硬湊一題不相關的題目，不如留白。

int main() {
    // ── 原始教學範例：各型別自動識別 ───────────────────────────
    std::cout << "=== 型別自動識別（原始範例）===" << std::endl;
    int i = 42;
    double d = 3.14159;
    char c = 'A';
    const char* s = "Hello";
    bool b = true;

    // 不需要格式符，編譯期依型別挑 overload
    std::cout << "int: " << i << std::endl;
    std::cout << "double: " << d << std::endl;
    std::cout << "char: " << c << std::endl;
    std::cout << "string: " << s << std::endl;
    std::cout << "bool: " << b << std::endl;  // 預設輸出 1

    // ── bool：0/1 vs true/false ───────────────────────────────
    std::cout << "\n=== bool 的兩種印法 ===" << std::endl;
    std::cout << "預設(noboolalpha): " << b << '\n';
    std::cout << "開 boolalpha:      " << std::boolalpha << b << '\n';
    std::cout << "還原 noboolalpha:  " << std::noboolalpha << b << '\n';

    // ── char 家族：印字元還是印數字 ────────────────────────────
    std::cout << "\n=== char 家族印出來是字元，不是數字 ===" << std::endl;
    std::uint8_t u = 65;
    std::cout << "char c            : " << c << '\n';
    std::cout << "uint8_t u (=65)   : " << u << "   <- 被當字元印，不是 65\n";
    std::cout << "static_cast<int>(c): " << static_cast<int>(c) << '\n';
    std::cout << "unary plus  +u     : " << +u << '\n';

    // ── 浮點數預設只有 6 位有效數字 ───────────────────────────
    std::cout << "\n=== 浮點數預設精度是 6 位有效數字 ===" << std::endl;
    double longPi = 3.14159265358979;
    std::cout << "預設格式            : " << longPi << '\n';
    std::cout << "setprecision(12)    : " << std::setprecision(12) << longPi << '\n';
    std::cout << "fixed + prec(3)     : " << std::fixed << std::setprecision(3)
              << longPi << '\n';
    std::cout << std::defaultfloat << std::setprecision(6);   // 還原預設

    // ── const char* 印內容，其它指標印位址 ────────────────────
    std::cout << "\n=== 指標：只有 char* 系列會印內容 ===" << std::endl;
    // 位址每次執行都可能不同，所以這裡只印「是否為同一個位址」這種穩定的布林值，
    // 不直接印位址數值。
    const void* asAddress = static_cast<const void*>(s);
    std::cout << "const char* s 直接印    : " << s << "   <- 印字串內容\n";
    std::cout << "轉成 const void* 後印的是位址（每次執行都不同，故不列出數值）\n";
    std::cout << "轉型前後指的是同一塊記憶體: " << std::boolalpha
              << (asAddress == static_cast<const void*>(s)) << std::noboolalpha << '\n';

    // ── 日常實務：設定摘要 ────────────────────────────────────
    std::cout << "\n=== 日常實務: 啟動時 dump 設定 ===" << std::endl;
    dumpConfig(std::cout, "order-api", 8443, true, 2.5);

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 5 課：輸入輸出流（iostream）入門3.cpp" -o io3

// 說明（環境相關／刻意不列出的值）：
//   * uint8_t 在本機（x86-64 Linux / GCC）是 unsigned char 的 typedef，
//     故 u=65 印出字元 'A'。這是實作定義的 typedef 選擇，
//     但在所有主流平台上結果相同。
//   * 指標位址每次執行都不同，因此本檔只印「兩次轉型是否指向同一位址」的布林值，
//     不把位址數值寫進預期輸出。
//   * 其餘輸出完全決定性，已多次執行確認一致。

// === 預期輸出 ===
// === 型別自動識別（原始範例）===
// int: 42
// double: 3.14159
// char: A
// string: Hello
// bool: 1
//
// === bool 的兩種印法 ===
// 預設(noboolalpha): 1
// 開 boolalpha:      true
// 還原 noboolalpha:  1
//
// === char 家族印出來是字元，不是數字 ===
// char c            : A
// uint8_t u (=65)   : A   <- 被當字元印，不是 65
// static_cast<int>(c): 65
// unary plus  +u     : 65
//
// === 浮點數預設精度是 6 位有效數字 ===
// 預設格式            : 3.14159
// setprecision(12)    : 3.14159265359
// fixed + prec(3)     : 3.142
//
// === 指標：只有 char* 系列會印內容 ===
// const char* s 直接印    : Hello   <- 印字串內容
// 轉成 const void* 後印的是位址（每次執行都不同，故不列出數值）
// 轉型前後指的是同一塊記憶體: true
//
// === 日常實務: 啟動時 dump 設定 ===
// service       = order-api
// port          = 8443
// tls_enabled   = true
// timeout_sec   = 2.5
