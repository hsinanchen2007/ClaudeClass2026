// =============================================================================
//  第 2 課：C 與 C++ 的關鍵差異 15  —  C 的 struct：獨立的標籤命名空間
// =============================================================================
//
// 【主題資訊 Information】
//   語法（C）：struct Point { int x; int y; };
//              宣告變數必須寫  struct Point p1;   （不能只寫 Point p1;）
//   標準版本：C89 起皆然，直到 C23 仍是如此；C++ 從一開始就不需要 struct 關鍵字。
//   標頭檔：<stdio.h>
//   複雜度：純語法/命名空間規則，不影響任何執行期行為。
//
// 【詳細解釋 Explanation】
//
// 【1. 根本原因：C 有「兩套」名字空間】
//   C 語言把識別字分成幾個獨立的命名空間（name space），其中兩個是：
//     ● 一般識別字：變數名、函式名、typedef 名
//     ● 標籤（tag）：struct / union / enum 的名字
//   `struct Point { ... };` 只是在「標籤空間」登記了 Point，
//   並沒有在「一般識別字空間」建立任何叫 Point 的型別名。
//   所以你必須用 `struct Point` 這個完整寫法，才能指涉到標籤空間裡的它。
//   單寫 `Point p1;` 會被當成「使用一個從未宣告的一般識別字」而編譯失敗。
//
//   這個分離帶來一個 C 特有、C++ 沒有的現象——同一個名字可以同時是
//   標籤和變數，兩者互不干擾：
//       struct stat { ... };
//       int stat;                  /* 完全合法，兩者在不同空間 */
//   POSIX 的 `struct stat` 與函式 `stat()` 就是靠這條規則共存的。
//
// 【2. C 程式設計師的標準對策：typedef】
//       typedef struct Point { int x; int y; } Point;
//   這行做了兩件事：在標籤空間登記 Point，並在一般識別字空間
//   建立一個叫 Point 的 typedef 名。之後就能寫 `Point p1;`。
//   Linux 核心風格反對這種寫法（認為隱藏了「這是一個 struct」的事實），
//   而多數應用層 C 專案則普遍採用。這是 C 圈長年的風格之爭。
//
// 【3. C++ 的作法：標籤名自動注入一般識別字空間】
//   C++ 把 class/struct/union/enum 的名字同時放進一般識別字空間，
//   因此宣告完 `struct Point { ... };` 之後直接寫 `Point p1;` 就好，
//   不需要 typedef。這在標準裡稱為 injected-class-name。
//   ★ 但 C++ 保留了「用 struct Point 指涉」的寫法（稱為 elaborated
//     type specifier），為的正是相容 C，以及處理名字被變數遮蔽的情況：
//         struct Point { int x, y; };
//         int Point;                 // 變數遮蔽了型別名
//         struct Point p;            // 仍可用 elaborated 形式取得型別
//     這種寫法在正常程式碼裡不該出現，但它解釋了為何語法還留著。
//
// 【4. C 與 C++ 的 struct，還有哪些差異？】
//   除了關鍵字，C 的 struct 是「純資料聚合」，沒有：
//     成員函式、存取控制（public/private）、建構/解構函式、繼承、
//     運算子多載、樣板成員……
//   C++ 的 struct 和 class 只差一個地方：預設存取權限
//   （struct 是 public，class 是 private），其餘能力完全相同。
//   慣例上：純資料用 struct，有封裝與不變式（invariant）的用 class。
//
// 【概念補充 Concept Deep Dive】
//   ● 記憶體佈局在 C 和 C++ 是相容的——只要是 standard-layout 型別
//     （無虛擬函式、無虛擬繼承、所有非靜態成員存取權限相同…），
//     C 和 C++ 對它的佈局就一致，可以安全地跨語言傳遞。
//     這是 C++ 能直接使用 C 標頭檔與作業系統 API 的基礎。
//   ● 空 struct 的大小：C++ 保證 sizeof 至少為 1（每個物件必須有唯一位址），
//     而在 C 裡空 struct 根本不合法（gcc 允許是擴充，大小為 0）。
//     這是少數「同樣寫法、兩語言結果不同」的地方。
//   ● 成員之間會有 padding（填充位元組）以滿足對齊要求，
//     所以 sizeof(struct) 不一定等於各成員大小之和。本檔實測會印出來。
//
// 【注意事項 Pay Attention】
//   1. 本檔是刻意保留 C 風格的對照組，不是 C++ 的建議寫法。
//   2. C 中未初始化的區域 struct，其成員值是不確定的（indeterminate）；
//      讀取它是未定義行為。本檔在使用前一律先賦值。
//   3. `struct Point p1;` 與 C++ 的 `Point p1;` 產生的物件完全相同，
//      差別只在型別名稱如何被查找，沒有任何執行期成本。
//   4. sizeof(struct) 受對齊與 padding 影響，是實作定義的；
//      本檔印出的數值是本機 g++ 15.2 / x86-64 的結果，不是標準保證。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】C 的 struct 與標籤命名空間
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 C 宣告結構變數要寫 struct Point p; 而 C++ 不用？
//     答：C 把 struct/union/enum 的名字放在獨立的「標籤命名空間」，
//         不會自動出現在一般識別字空間，所以必須用 `struct Point`
//         這個完整形式去指涉。C++ 則把類別名同時注入一般識別字空間
//         （injected-class-name），因此可以直接寫 Point p。
//     追問：C 要怎麼省掉 struct 關鍵字？
//         → 用 typedef struct Point { ... } Point;
//           在一般識別字空間另外建立一個同名的型別別名。
//
// ⚠️ 陷阱. 在 C 裡寫 struct stat { ... }; 之後又宣告 int stat;，會衝突嗎？
//     答：不會，完全合法。struct 標籤和一般識別字在 C 裡屬於不同的
//         命名空間，兩個 stat 各自獨立。POSIX 的 `struct stat` 與
//         `stat()` 函式正是靠這條規則並存的。
//     為什麼會錯：多數人假設「同一個作用域裡名字不能重複」是通則。
//         實際上 C 有多個獨立命名空間（標籤、一般識別字、
//         結構成員、goto 標籤），同名只有在「同一個命名空間內」才算衝突。
//         把 C++ 的單一命名空間直覺套到 C，就會誤判。
// ═══════════════════════════════════════════════════════════════════════════

#include <stdio.h>
#include <string.h>   /* strncpy */

struct Point {
    int x;
    int y;
};

/* ---------------------------------------------------------------------------
 * 【日常實務範例】C 風格的設定結構：typedef + 初始化函式
 *   情境：讀取伺服器設定（主機、埠號、逾時秒數）。
 *   在 C 裡沒有建構函式，所以「保證欄位都有合理初值」這件事
 *   只能靠一個約定俗成的 init 函式來達成——而且呼叫端可能忘記呼叫，
 *   語言完全不會提醒你。這正是 C++ 建構函式要解決的問題。
 * ------------------------------------------------------------------------- */
typedef struct ServerConfig {      /* typedef 讓後續可以省略 struct 關鍵字 */
    char host[32];
    int  port;
    int  timeoutSec;
} ServerConfig;

/* 手動的「建構函式」：必須由呼叫端記得呼叫 */
void serverConfigInit(ServerConfig* cfg) {
    if (cfg == NULL) return;
    strncpy(cfg->host, "127.0.0.1", sizeof(cfg->host) - 1);
    cfg->host[sizeof(cfg->host) - 1] = '\0';   /* strncpy 不保證結尾 '\0' */
    cfg->port = 8080;
    cfg->timeoutSec = 30;
}

void serverConfigPrint(const ServerConfig* cfg) {
    if (cfg == NULL) return;
    printf("  host=%s port=%d timeout=%ds\n",
           cfg->host, cfg->port, cfg->timeoutSec);
}

/* 註：本檔不加 LeetCode 範例。
 *     「宣告結構變數要不要寫 struct 關鍵字」是命名空間的語法規則，
 *     不牽涉任何演算法；硬掛一題只會模糊焦點，故從缺。
 *     真正適合示範「struct/class 設計」的 LeetCode 題目放在 16.cpp。 */

int main() {
    /* C 語言中，宣告變數需要 struct 關鍵字 */
    struct Point p1;
    p1.x = 10;
    p1.y = 20;
    
    /* 或者使用 typedef */
    /* typedef struct Point Point; */
    
    printf("(%d, %d)\n", p1.x, p1.y);

    printf("\n=== C 的 struct 是純資料聚合 ===\n");
    printf("  sizeof(struct Point) = %zu bytes（本機 g++ 15.2 / x86-64 實測）\n",
           sizeof(struct Point));
    printf("  兩個 int 相加        = %zu bytes\n", sizeof(int) * 2);
    printf("  → 此例無 padding，但一般而言 sizeof(struct) 受對齊影響，\n");
    printf("    不保證等於各成員大小之和\n");

    printf("\n=== 日常實務：C 風格設定結構 ===\n");
    {
        ServerConfig cfg;          /* 此時所有欄位都是不確定值，不可讀取 */
        serverConfigInit(&cfg);    /* 必須記得呼叫，語言不會強制 */
        printf("  預設設定：\n");
        serverConfigPrint(&cfg);

        cfg.port = 9443;           /* 覆寫其中一項 */
        printf("  改過 port 之後：\n");
        serverConfigPrint(&cfg);
    }

    printf("\n=== 對照 ===\n");
    printf("  C   ：struct Point p1;  且沒有成員函式、沒有建構函式\n");
    printf("  C++ ：Point p1;         struct 可以有成員函式（見 16.cpp）\n");

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 2 課：C 與 C++ 的關鍵差異15.cpp" -o diff15

// === 預期輸出 ===
// (10, 20)
//
// === C 的 struct 是純資料聚合 ===
//   sizeof(struct Point) = 8 bytes（本機 g++ 15.2 / x86-64 實測）
//   兩個 int 相加        = 8 bytes
//   → 此例無 padding，但一般而言 sizeof(struct) 受對齊影響，
//     不保證等於各成員大小之和
//
// === 日常實務：C 風格設定結構 ===
//   預設設定：
//   host=127.0.0.1 port=8080 timeout=30s
//   改過 port 之後：
//   host=127.0.0.1 port=9443 timeout=30s
//
// === 對照 ===
//   C   ：struct Point p1;  且沒有成員函式、沒有建構函式
//   C++ ：Point p1;         struct 可以有成員函式（見 16.cpp）
