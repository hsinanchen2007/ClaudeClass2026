// =============================================================================
// 檔名: c_str.cpp
// 主題: std::string::c_str (取得 null-terminated C-style 字串)
// 參考: https://en.cppreference.com/cpp/string/basic_string/c_str
//       https://cplusplus.com/reference/string/string/c_str/
// =============================================================================
//
// 【函式資訊 Information】
//   const char* c_str() const noexcept;
//
// 回傳: 永遠 const、永遠以 '\0' 結尾的指標。
//
// 【詳細解釋 Explanation】
//
// (1) 設計理念:跨越「C / C++ 字串世界」的橋樑
//   C 語言的字串只是「以 '\0' 結尾的 char 陣列」,所有 C 函式 (printf、
//   strcmp、fopen、open、execlp、syslog 等) 都期待 const char*。
//   而 std::string 的內部表示是「指標 + size + capacity」,
//   不能直接傳給 C 函式。c_str() 就是「請給我一份 C 認得的指標」的標準介面。
//
// (2) 歷史背景:從不保證到必定 null-terminated
//   - C++98/03: c_str() 規定回傳 null-terminated;data() 不保證!
//     兩者經常被視為不同的東西(c_str() 多一個收尾的 '\0')。
//     有些實作 data() 真的不放 '\0',混用會踩雷。
//   - C++11 起: 兩者統一 — std::string 的內部 buffer 末端必須有 '\0',
//     即 data() == c_str() 在大部分情況下指向同一塊記憶體。
//   - C++17 起: data() 多了一個非 const 的 overload,可寫入內部 buffer
//     (但不能改動末端的 '\0',否則違反不變式)。c_str() 始終只有 const 版本。
//
// (3) 底層運作機制
//   std::string 的內部 buffer 永遠多預留一個 byte 給 '\0'。也就是說:
//     buffer[0..size()-1] 是字串內容
//     buffer[size()]      是 '\0'
//     buffer[size()+1..capacity()] 是未初始化或保留空間
//   c_str() 只不過是 return _ptr; — 完全不需要計算或複製。
//
// (4) 與 data() 的細微差異
//   - 語意:c_str() 名稱意指「給 C 用」,讀者一眼看得出意圖。
//   - 型別:c_str() 永遠是 const char*;C++17 起 data() 在 non-const string
//           上回傳 char*,可直接寫入內部 buffer。
//   - 行為:C++11 之後兩者結果完全相同;C++98 時代不一樣。
//   - 風格建議:餵給 C API 時用 c_str(),需要寫入內部 buffer 時才用 data()。
//
// (5) 與 std::string_view 的互動
//   string_view 不保證 null-terminated。要將 string_view 傳給 C API 必須
//   先建立 std::string(sv) 再呼叫 c_str()。直接 sv.data() 是錯的!
//
// (6) 複雜度
//   時間 O(1)、空間 O(1)、不會配置記憶體、noexcept。
//
// 【注意事項 Pay Attention】
// 1. 別保留 c_str() 的指標太久;只要原 string 被修改 (push_back、+=、
//    reserve、clear 等可能 invalidate),或 string 被解構,指標就失效。
// 2. 不要對 c_str() 用 const_cast 拿掉 const 後寫入 — 是 UB。
//    要寫入請改用 C++17 的 non-const data()。
// 3. 若 string 內含 '\0' (例如二進位資料),c_str() 仍會在 size() 位置補
//    '\0',但 strlen 等 C 函式會在第一個 '\0' 就停下,結果不等於 s.size()。
// 4. c_str() 在 const string、空字串、含內嵌 null 的 string 上都安全。
//    對 std::string{} 呼叫 c_str() 回傳的不是 nullptr,而是指向 '\0' 的指標。
// 5. 把 c_str() 結果傳給多執行緒、跨函式長期持有,要特別小心 lifetime。
//
// 【概念補充 Concept Deep Dive】
//
// ★ 為何 C++11 起保證 null-terminated?
//   標準制定時,大量真實程式 (尤其是 Linux / POSIX 介面) 都假設 c_str()
//   end 一定有 '\0'。原本 C++98 限定 c_str() 才保證,等於是在「需要 C
//   字串」與「需要 raw 資料指標」之間用兩個函式區隔,造成混亂。
//   既然現實實作都已經多預留一個 byte,標準就統一 data() 也保證,
//   讓兩者語意一致、減少 bug。
//
// ★ c_str() 是否會配置記憶體?
//   不會。它只是回傳內部 buffer 的指標。string 在生命週期內已維護好
//   結尾 '\0',不需要任何即時動作。這也是為何能標 noexcept。
//
// ★ 為什麼不能修改 c_str() 指向的內容?
//   const char* 的 contract 就是不能寫。標準保證:對 c_str() 結果做寫入是 UB。
//   實作上,某些 short string buffer 在 stack 上、可能與其他資料 alias,
//   寫入會破壞 string 不變式 (size、'\0' 位置、SSO flag) 而引發後續災難。
//
// ★ c_str() 與字元編碼
//   std::string 是 byte container,不知道編碼。c_str() 回傳的是 raw bytes。
//   對 UTF-8 字串,c_str() 回傳的 bytes 仍正確,因為 UTF-8 不含 0x00 byte。
//   但若是 UTF-16 / UTF-32 字串,內部會有 0x00 byte,需用 std::wstring /
//   std::u16string / std::u32string,然後對應 c_str() 回傳 wchar_t*、
//   char16_t*、char32_t*。
//
// ★ 與作業系統 API 的互動
//   POSIX 的 open(), fopen(), execlp() 等全都接受 const char*。
//   Windows API 大部分有 A 版本 (ANSI) 與 W 版本 (UTF-16);
//   c_str() 對應 A 版本,wstring.c_str() 對應 W 版本。
//
// =============================================================================

/*
補充筆記：std::string::c_str
  - c_str 回傳給 C API 用的 null-terminated 指標，但該指標由 string 物件擁有。
  - 只要 string 被修改到可能重配，舊 c_str 指標就不能再用。
  - 不要 delete c_str 回傳的指標，也不要在 string 死亡後保存它。
  - std::string::c_str 是 std::string 主題的一部分；先分清楚這個操作會讀取、追加、插入、刪除、搜尋，還是只暴露底層字元。
  - std::string 的索引型別是 size_type；和 int 混用時要小心 unsigned underflow，尤其是倒著迴圈或拿 npos 比較。
  - 修改字串內容或容量可能讓先前取得的 iterator、reference、pointer、c_str() 指標失效。
  - operator[] 不做範圍檢查，at() 越界會丟 std::out_of_range；教學範例可比較兩者，工作程式要依錯誤處理需求選。
  - find/rfind/find_first_of 這類搜尋函式失敗回傳 std::string::npos；判斷時應和 npos 比，不要寫成 >= 0。
  - 字串和 C string 互通時要記得 null terminator；std::string 可以保存內含 \0 的資料，但很多 C API 會在第一個 \0 停止。
  - 若只是傳入唯讀文字片段且不需要擁有資料，std::string_view 可避免複製；但它不能延長原字串生命週期。
  - 處理中文或 UTF-8 時，std::string 的 size() 回傳 byte 數，不是人眼看到的字元數。
*/
#include <iostream>
#include <string>
#include <cstdio>
#include <fstream>

void demoCStr() {
    std::string path = "/tmp/c_str_demo.txt";

    // 範例 1: 給 C 風格 API 使用
    std::printf("path c_str = %s\n", path.c_str());

    // 範例 2: 給 fopen / std::ofstream 使用
    // 注意 std::ofstream 在 C++11 起本身就吃 std::string,但仍可示範
    std::ofstream ofs(path.c_str());
    if (ofs) {
        ofs << "demo\n";
        std::printf("檔案已寫入 %s\n", path.c_str());
    }
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 8. String to Integer (atoi)
// 題目: 實作簡化版的 atoi,將字串轉為 int。
// 為何用 c_str: 我們可以直接呼叫 C 標準的 strtol 來處理大多數邊界。
//               展示 std::string 與 C 函式的接合。
// -----------------------------------------------------------------------------
#include <cstdlib>
#include <climits>
#include <cctype>
int myAtoi(const std::string& s) {
    // 略過前導空白
    size_t i = 0;
    while (i < s.size() && s[i] == ' ') ++i;
    if (i == s.size()) return 0;

    // 拿子字串 → 給 strtol
    std::string sub = s.substr(i);
    char* end = nullptr;
    long  val = std::strtol(sub.c_str(), &end, 10);

    if (val > INT_MAX) return INT_MAX;
    if (val < INT_MIN) return INT_MIN;
    return static_cast<int>(val);
}

// -----------------------------------------------------------------------------
// 【日常實務範例】從環境變數讀取設定 (POSIX getenv)
// 為何用 c_str: getenv 接受 const char*。讀取環境變數是日常後端 / DevOps
//               常見任務 (DATABASE_URL, LOG_LEVEL, API_KEY...)。
// -----------------------------------------------------------------------------
std::string getEnvOrDefault(const std::string& name, const std::string& defaultValue) {
    const char* v = std::getenv(name.c_str());
    return v ? std::string(v) : defaultValue;
}

// -----------------------------------------------------------------------------
// 【LeetCode 範例 2】LeetCode 1290. Convert Binary Number in Linked List to Integer
// 題目: 給一串 0/1 字元組成的字串 (代表二進位數),轉成十進位整數。
// 為何用 c_str: 直接餵給 C API std::strtoul 並指定 base=2,一行解。
//                展示「string 借 c_str 接 C 標準函式庫」的典型用法。
// 複雜度: O(N)。
// -----------------------------------------------------------------------------
#include <cstdlib>
int getDecimalValue(const std::string& binary) {
    return static_cast<int>(std::strtoul(binary.c_str(), nullptr, 2));
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】把字串路徑交給 POSIX 系統呼叫 (fopen)
// 為何用 c_str: 所有 C 標準 IO API 都吃 const char*,std::string 必須轉。
// -----------------------------------------------------------------------------
#include <cstdio>
bool fileExists(const std::string& path) {
    std::FILE* fp = std::fopen(path.c_str(), "rb");
    if (!fp) return false;
    std::fclose(fp);
    return true;
}

int main() {
    demoCStr();
    std::cout << "\n=== LeetCode 8 ===\n";
    std::cout << myAtoi("42") << "\n";              // 42
    std::cout << myAtoi("   -42") << "\n";          // -42
    std::cout << myAtoi("4193 with words") << "\n"; // 4193
    std::cout << myAtoi("-91283472332") << "\n";    // -2147483648 (INT_MIN)

    std::cout << "\n=== LeetCode 1290 ===\n";
    std::cout << getDecimalValue("101")  << "\n";   // 5
    std::cout << getDecimalValue("1101") << "\n";   // 13

    std::cout << "\n=== 日常實務: 讀取環境變數 ===\n";
    std::cout << "HOME = " << getEnvOrDefault("HOME", "(not set)") << "\n";
    std::cout << "MISSING = " << getEnvOrDefault("MISSING_VAR", "fallback") << "\n";

    std::cout << "\n=== 日常實務: fileExists ===\n";
    std::cout << std::boolalpha << fileExists("/etc/hostname") << "\n";
    std::cout << fileExists("/nonexistent_xyz") << "\n";

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：c_str() 與 data() 在 C++11 之後還有差別嗎?
    //    A：C++11 起兩者結果完全相同 ── 都保證 null-terminated。差別在型別:
    //       c_str() 永遠 const char*;C++17 起 data() 多一個非 const 版本可
    //       回傳 char* 讓你寫入內部 buffer。風格建議:餵給 C API 用 c_str()
    //       表達意圖,需要寫入內部 buffer 才用 data()。
    //
    //  Q2：對空字串呼叫 c_str() 安全嗎?會回傳 nullptr 嗎?
    //    A：絕對安全,且永遠不會回傳 nullptr。空字串內部仍有一個合法的
    //       buffer,c_str() 回傳指向 '\0' 的指標。把它傳給 strlen 會得到 0,
    //       傳給 printf("%s") 會印空字串。c_str() 是 noexcept 且絕不配置
    //       記憶體。
    //
    //  Q3：能把 string_view 直接傳給 C API 嗎?為什麼?
    //    A：不能!string_view 不保證 null-terminated。其 data() + size() 只
    //       描述一段範圍,後面可能不是 '\0'。要傳給 C API 必須先建構
    //       std::string(sv) 再 .c_str()。直接 sv.data() 給 strlen / printf
    //       是 UB,可能讀越界。
    //
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra c_str.cpp -o c_str

// === 預期輸出 (節錄) ===
// === LeetCode 1290 ===
// 5
// 13
// === 日常實務: fileExists ===
// true   (在多數 Linux/WSL 環境 /etc/hostname 存在)
// false
