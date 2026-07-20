// =============================================================================
//  第 2 課：C 與 C++ 的關鍵差異 7  —  C 的布林：從「沒有」到 _Bool 到 stdbool.h
// =============================================================================
//
// 【主題資訊 Information】
//   C89      ：沒有布林型別。慣例用 int，0 為假、非 0 為真
//   C99      ：新增內建型別 _Bool；<stdbool.h> 提供 bool / true / false 巨集
//   C23      ：bool / true / false 升格為真正的關鍵字，不再需要 stdbool.h
//   C++      ：bool / true / false 從 C++98 起就是關鍵字，從不需要標頭檔
//   標頭檔   ：<stdbool.h>（C99–C17 需要；C23 起僅為相容而保留）
//   sizeof   ：本機 g++ 15.2 / gcc 15.2、x86-64 上 sizeof(_Bool) 與
//              sizeof(bool) 皆為 1（實作定義，標準只保證足以存 0 與 1）
//
// 【詳細解釋 Explanation】
//
// 【1. C89 為什麼沒有布林型別】
//   C 的設計哲學是「貼近機器」，而機器層級只有「零」與「非零」——
//   條件跳躍指令看的是 zero flag。既然 if / while 判斷的本來就是
//   「這個整數是不是 0」，再造一個布林型別在當時看起來是多餘的。
//   於是 C89 的慣例是：
//       int found = 0;              /* 假 */
//       if (found) { ... }          /* 非 0 即真 */
//   問題是這讓「這個 int 是數量還是真假？」完全靠命名與註解，型別系統
//   一點忙都幫不上。大型專案於是各自造輪子：
//       typedef int BOOL;  #define TRUE 1  #define FALSE 0     /* Windows */
//       typedef enum { false, true } bool;                     /* 各家自製 */
//   結果是不同函式庫的 BOOL 互不相容，TRUE 到底是 1 還是 -1 也各有主張
//   （Win32 的 VARIANT_BOOL 就用 -1 當 TRUE），整合時災難不斷。
//   C99 引入 _Bool 正是為了終結這場混亂。
//
// 【2. 為什麼 C99 的新型別叫 _Bool 這麼難看】
//   因為「bool」這個識別字在 1999 年時已經被無數既有程式碼用掉了——
//   有人 typedef 成 int，有人做成 enum。若 C99 直接把 bool 變成關鍵字，
//   所有這些程式碼會在一夜之間全部編譯失敗。
//   C 標準的解法是保留字空間規則：底線開頭 + 大寫字母（_Bool、_Complex、
//   _Atomic）是「保留給未來標準」的命名空間，使用者程式不該用。
//   然後把好看的名字放進一個「你自己決定要不要 include」的標頭檔：
//       /* stdbool.h 的實際內容大致是 */
//       #define bool  _Bool
//       #define true  1
//       #define false 0
//       #define __bool_true_false_are_defined 1
//   所以在 C 裡，bool 長期以來是一個「巨集」而非型別名稱。
//   本機實測（gcc 15.2 -std=c11）：#ifdef bool 成立，證實它確實是巨集。
//   到了 C23，bool/true/false 終於升格為真正的關鍵字，
//   本機實測（gcc 15.2 -std=c23）：#ifdef bool 不再成立。
//   而 C++ 從 C++98 起就是關鍵字，本機實測 g++ 下 #ifdef bool 同樣不成立。
//
// 【3. _Bool 不只是「小一點的 int」——它有轉換規則】
//   這是 _Bool 相對於 typedef int BOOL 的關鍵價值：
//   任何純量值轉成 _Bool 時，會被「正規化」成 0 或 1：
//       _Bool b = 5;        /* b 變成 1，不是 5 */
//       _Bool c = 0.3;      /* c 變成 1（非零即真），不是 0 */
//       _Bool d = 256;      /* d 變成 1；若是 char 則會被截斷成 0 */
//   最後一行是重點：如果用 char 當布林，256 截斷後是 0（假）；
//   用 _Bool 則正確得到 1（真）。這是型別語意，不是大小問題。
//   本機實測見下方輸出：(bool)5 的結果是 1。
//
// 【4. 為什麼 C89 風格的 int 當布林仍到處都是】
//   * 大量既有 API 的回傳值是 int，而且「0 代表成功」（POSIX 慣例），
//     與「0 代表假」剛好相反，混用時極易寫反：
//         if (strcmp(a, b)) { /* 這是「不相等」才進來，很多人搞錯 */ }
//         if (access(path, F_OK) == 0) { /* 0 才是成功 */ }
//   * 這正是 C++ 要有真正 bool 型別的動機之一：讓函式簽名自己說清楚
//     「我回傳的是真假，不是錯誤碼，也不是數量」。
//
// 【概念補充 Concept Deep Dive】
//   * sizeof(_Bool) 是實作定義的。標準只要求它「大到足以存 0 和 1」，
//     所以理論上可以是 1、4 或別的值；本機 x86-64 上是 1（實測）。
//     不要在跨平台的二進位格式（檔案、網路封包）裡直接寫入 _Bool，
//     應該明確用 uint8_t 之類的固定寬度型別。
//   * _Bool 的位元表示只有 0 與 1 是有效的。若透過 memcpy 或 union
//     把其他位元樣式塞進 _Bool 變數再讀取，是未定義行為——
//     編譯器可能已經假設「這裡不是 0 就是 1」而據此最佳化。
//   * C 沒有函式多載，所以無法針對 bool 做特別處理；
//     printf 也沒有 %b 給布林用（C23 的 %b 是二進位整數，不是布林），
//     只能用 %d 印出 0/1，或自己寫三元運算子印文字。
//
// 【注意事項 Pay Attention】
//   1. C99–C17 要用 bool 必須 #include <stdbool.h>；C23 起不需要。
//      C++ 完全不需要（本檔沿用原始碼的 include 只是為了對照 C 的寫法）。
//   2. 不要假設 sizeof(bool) 是多少——它是實作定義的。
//   3. 不要拿 bool 值做算術當計數器。b + b 在整數提升後是 2（本機實測），
//      但這是把布林當數字用，語意混亂。要計數就用整數型別。
//   4. 不要把布林值直接寫進檔案或網路封包，改用固定寬度整數。
//   5. 與 C 的 int 回傳碼互動時要特別小心「0 是成功」與「0 是假」的衝突。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】C 的布林型別
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. C 語言的 bool 和 C++ 的 bool 有什麼不同？
//     答：C++ 的 bool 從 C++98 起就是語言關鍵字，是真正的內建型別。
//         C 直到 C99 才有內建的 _Bool；bool/true/false 是 <stdbool.h> 提供的
//         「巨集」（#define bool _Bool），要 include 才有。
//         C23 起 bool 才升格為 C 的關鍵字，不再需要標頭檔。
//     追問：為什麼 C99 不直接把型別叫 bool？→ 因為當時已有海量程式碼
//         自己 typedef 或 #define 了 bool，直接搶這個名字會讓它們全部編譯失敗。
//         用底線開頭 + 大寫（保留識別字空間）的 _Bool 才能安全新增。
//
// 🔥 Q2. _Bool 和 typedef int BOOL 有什麼實質差別？只是佔的空間不同嗎？
//     答：不只是大小。關鍵是「轉換規則」：任何純量轉成 _Bool 時會被正規化成
//         0 或 1，非零一律變 1。typedef int BOOL 沒有這個語意，
//         BOOL b = 5; 之後 b 就真的是 5，跟 TRUE(1) 比較會失敗。
//         換句話說 _Bool 帶來的是型別語意，不是省記憶體。
//     追問：那用 char 當布林可以嗎？→ 有陷阱。char 只有 8 bit，
//         把 256 指派給 char 會截斷成 0（變成假），而 _Bool 會正確得到 1。
//
// ⚠️ 陷阱. 這段 C 程式碼為什麼判斷反了？
//         if (strcmp(name, "admin")) {
//             printf("是管理員\n");
//         }
//     答：strcmp 回傳的是「差值」不是「真假」：相等時回傳 0，
//         不相等時回傳非 0。而 if 判斷的是「非 0 為真」，
//         所以這段是「名字不等於 admin 時才印出是管理員」，完全反了。
//         正確寫法是 if (strcmp(name, "admin") == 0)。
//     為什麼會錯：C 沒有布林型別的歷史，讓「回傳 int」同時承擔了
//         「真假」與「比較結果／錯誤碼」兩種完全相反的慣例。
//         腦中把 strcmp 讀成「字串比較 → 相同就是真」，
//         但它其實是「相減 → 相同就是 0」。
//         這正是 C++ 讓 operator== 回傳真正的 bool 所要消除的歧義。
//
// 註：布林型別的歷史與轉換規則屬於語言層面主題，沒有對應的 LeetCode 題目
//     （LeetCode 不會考 stdbool.h），硬套題號只會誤導，故本檔僅附實務範例。
// ═══════════════════════════════════════════════════════════════════════════

#include <stdio.h>
#include <stdbool.h>  // C99 需要這個標頭檔
#include <string.h>

// -----------------------------------------------------------------------------
// 【日常實務範例 1】C89 風格的旗標位元運算：為什麼「int 當布林」還沒消失
//
// 情境：解析檔案權限／功能開關這類 bitmask，是 C 系統程式的日常。
//   這裡刻意示範「bitmask 的每一位是旗標，但取出來要轉成布林才安全」。
// 為何用到本主題：直接把 (flags & PERM_WRITE) 當布林用時，它的值是 2 而不是 1，
//   若拿去和 1（true）比較就會失敗——這正是需要 _Bool 正規化語意的地方。
// -----------------------------------------------------------------------------
#define PERM_READ    0x1
#define PERM_WRITE   0x2
#define PERM_EXECUTE 0x4

bool hasPermission(unsigned flags, unsigned want) {
    // 轉成 bool 時會被正規化成 0/1，這正是 _Bool 的價值
    return (flags & want) != 0;
}

void describePermissions(const char* path, unsigned flags) {
    printf("  %-14s r=%s w=%s x=%s\n", path,
           hasPermission(flags, PERM_READ)    ? "是" : "否",
           hasPermission(flags, PERM_WRITE)   ? "是" : "否",
           hasPermission(flags, PERM_EXECUTE) ? "是" : "否");
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】解析設定檔的布林值："true"/"yes"/"1" 都要接受
//
// 情境：讀 .env / .ini 時，布林設定的寫法五花八門，
//   必須把各種人類寫法正規化成一個 bool。
// 為何用到本主題：這是「把外界的雜訊收斂成一個明確布林型別」的典型場景，
//   也示範了為什麼需要一個「解析成功了嗎」與「值是什麼」分開的介面
//   （C 沒有 std::optional，只能用回傳碼 + 輸出參數）。
// -----------------------------------------------------------------------------
int parseBoolSetting(const char* text, bool* out) {
    if (text == NULL || out == NULL) return 0;

    static const char* trueWords[]  = {"true",  "yes", "on",  "1"};
    static const char* falseWords[] = {"false", "no",  "off", "0"};

    for (int i = 0; i < 4; ++i) {
        if (strcmp(text, trueWords[i]) == 0)  { *out = true;  return 1; }
        if (strcmp(text, falseWords[i]) == 0) { *out = false; return 1; }
    }
    return 0;                       // 無法辨識，不動 *out
}

int main() {
    printf("=== 原始示範 ===\n");
    bool flag = true;   // 需要 stdbool.h
    int old_style = 1;  // C89 常用 int 代替

    if (flag) {
        printf("flag is true\n");
    }
    // 原始碼宣告了 old_style 卻沒用到，這裡把 C89 的寫法一併示範出來
    if (old_style) {
        printf("old_style（C89 用 int 當布林）is true\n");
    }

    printf("\n=== bool 在 C 與 C++ 的身分不同（本機實測）===\n");
#ifdef bool
    printf("目前編譯模式下 bool 是「巨集」（C99–C17 的 stdbool.h 行為）\n");
#else
    printf("目前編譯模式下 bool 是「關鍵字」，不是巨集\n");
    printf("  → 本檔用 g++ 編譯，C++ 的 bool 自 C++98 起就是內建關鍵字\n");
    printf("  → 在 gcc -std=c11 下編同樣的判斷會走另一個分支\n");
#endif
#ifdef __bool_true_false_are_defined
    printf("__bool_true_false_are_defined = %d（stdbool.h 有被引入）\n",
           __bool_true_false_are_defined);
#endif
    printf("sizeof(bool) = %zu（實作定義，本機 x86-64 為 1）\n", sizeof(bool));

    printf("\n=== _Bool 的正規化轉換 ===\n");
    {
        bool b1 = (bool)5;
        bool b2 = (bool)0.3;
        bool b3 = (bool)(-7);
        printf("  (bool)5    = %d  ← 非 0 一律變成 1，不是 5\n", (int)b1);
        printf("  (bool)0.3  = %d  ← 浮點數非 0 也是真\n", (int)b2);
        printf("  (bool)(-7) = %d  ← 負數同樣是真\n", (int)b3);
        printf("  對照：char c = (char)256 會被截斷成 %d（假），\n", (int)(char)256);
        printf("        但 bool b = (bool)256 是 %d（真）——這是型別語意的差別\n",
               (int)(bool)256);
    }

    printf("\n=== 日常實務 1：權限旗標 ===\n");
    describePermissions("/etc/passwd", PERM_READ);
    describePermissions("/tmp/scratch", PERM_READ | PERM_WRITE);
    describePermissions("/usr/bin/ls",  PERM_READ | PERM_EXECUTE);
    {
        // 這裡示範為什麼要 != 0：位元取出來的值不是 1
        unsigned flags = PERM_READ | PERM_WRITE;
        printf("  (flags & PERM_WRITE) 的原始值 = %u（不是 1！）\n",
               flags & PERM_WRITE);
        printf("  轉成 bool 之後 = %d（被正規化成 1）\n",
               (int)hasPermission(flags, PERM_WRITE));
    }

    printf("\n=== 日常實務 2：解析設定檔的布林值 ===\n");
    {
        const char* inputs[] = {"true", "yes", "0", "off", "maybe", "TRUE"};
        for (int i = 0; i < 6; ++i) {
            bool value = false;
            if (parseBoolSetting(inputs[i], &value)) {
                printf("  [%-6s] → %s\n", inputs[i], value ? "true" : "false");
            } else {
                printf("  [%-6s] → 無法辨識（大小寫敏感，TRUE 不在清單中）\n",
                       inputs[i]);
            }
        }
    }

    printf("\n=== C 沒有布林型別留下的後遺症 ===\n");
    {
        const char* name = "guest";
        printf("  strcmp(\"%s\", \"admin\") = %d\n", name, strcmp(name, "admin"));
        printf("  → 相等時回傳 0（假），不相等時回傳非 0（真），語意與直覺相反\n");
        printf("  → 正確寫法一定要寫 == 0，不可以直接 if (strcmp(...))\n");
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 2 課：C 與 C++ 的關鍵差異7.cpp" -o demo7
// 對照(C): gcc -std=c11 -Wall -Wextra "第 2 課：C 與 C++ 的關鍵差異7.cpp" -o demo7c
//
// 說明：
//   1. 本檔用 g++ 編譯，所以 #ifdef bool 走的是「關鍵字」分支。
//      若改用 gcc -std=c11 編譯同一份原始碼，會改印「bool 是巨集」那一行——
//      這正是本檔要示範的差異。C23 則又會變回關鍵字分支。
//   2. strcmp("guest", "admin") 回傳的「非 0」具體數值由實作決定
//      （標準只保證正負號），本機 glibc 實測為 6，換平台可能不同。
//   3. sizeof(bool) 為實作定義，本機 x86-64 為 1。

// === 預期輸出 ===
// === 原始示範 ===
// flag is true
// old_style（C89 用 int 當布林）is true
//
// === bool 在 C 與 C++ 的身分不同（本機實測）===
// 目前編譯模式下 bool 是「關鍵字」，不是巨集
//   → 本檔用 g++ 編譯，C++ 的 bool 自 C++98 起就是內建關鍵字
//   → 在 gcc -std=c11 下編同樣的判斷會走另一個分支
// __bool_true_false_are_defined = 1（stdbool.h 有被引入）
// sizeof(bool) = 1（實作定義，本機 x86-64 為 1）
//
// === _Bool 的正規化轉換 ===
//   (bool)5    = 1  ← 非 0 一律變成 1，不是 5
//   (bool)0.3  = 1  ← 浮點數非 0 也是真
//   (bool)(-7) = 1  ← 負數同樣是真
//   對照：char c = (char)256 會被截斷成 0（假），
//         但 bool b = (bool)256 是 1（真）——這是型別語意的差別
//
// === 日常實務 1：權限旗標 ===
//   /etc/passwd    r=是 w=否 x=否
//   /tmp/scratch   r=是 w=是 x=否
//   /usr/bin/ls    r=是 w=否 x=是
//   (flags & PERM_WRITE) 的原始值 = 2（不是 1！）
//   轉成 bool 之後 = 1（被正規化成 1）
//
// === 日常實務 2：解析設定檔的布林值 ===
//   [true  ] → true
//   [yes   ] → true
//   [0     ] → false
//   [off   ] → false
//   [maybe ] → 無法辨識（大小寫敏感，TRUE 不在清單中）
//   [TRUE  ] → 無法辨識（大小寫敏感，TRUE 不在清單中）
//
// === C 沒有布林型別留下的後遺症 ===
//   strcmp("guest", "admin") = 6
//   → 相等時回傳 0（假），不相等時回傳非 0（真），語意與直覺相反
//   → 正確寫法一定要寫 == 0，不可以直接 if (strcmp(...))
