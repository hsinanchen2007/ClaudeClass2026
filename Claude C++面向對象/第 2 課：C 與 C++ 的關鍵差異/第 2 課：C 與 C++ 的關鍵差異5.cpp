// =============================================================================
//  第 2 課：C 與 C++ 的關鍵差異 5  —  C 的動態記憶體：malloc / free
// =============================================================================
//
// 【主題資訊 Information】
//   簽名     ：void* malloc (size_t size);
//              void* calloc (size_t n, size_t size);   // 會歸零
//              void* realloc(void* ptr, size_t newSize);
//              void  free   (void* ptr);
//   回傳值   ：malloc/calloc/realloc 成功回傳指標，失敗回傳 NULL（不丟例外）
//   標準版本 ：C89 起；C++ 透過 <cstdlib> 提供
//   標頭檔   ：<stdlib.h>（C）／<cstdlib>（C++）
//   複雜度   ：一般視為攤銷 O(1)，但實際成本取決於配置器與碎片化程度
//
// 【詳細解釋 Explanation】
//
// 【1. malloc 只做一件事：要一塊「夠大的、沒有型別的」位元組】
//   malloc 的回傳型別是 void*，它對「你要拿這塊記憶體放什麼」一無所知。
//   它做的是：向配置器要 size 個位元組，回傳起始位址，內容是未指定的
//   （不保證歸零，裡面是先前被釋放的殘留資料）。
//   這帶來三個連鎖後果，而這三點正是 C++ 要另外發明 new 的理由：
//     (a) 必須自己算大小 → sizeof(int) 寫錯就配置不足；
//     (b) 必須自己轉型   → C++ 不允許 void* 隱式轉成 int*，所以要 (int*)；
//     (c) 不會呼叫建構子 → 對有 ctor 的型別，拿到的是一塊沒被初始化的原始位元組。
//
// 【2. 為什麼 C++ 一定要寫 (int*) 而 C 不用】
//   C 允許 void* 隱式轉換成任何物件指標，所以 C 裡寫
//       int* p = malloc(sizeof(int));      /* C 合法 */
//   是慣用法（甚至有人主張 C 不該加轉型，以免遮蔽「忘了 include stdlib.h」的錯誤）。
//   但 C++ 的型別系統嚴格得多，void* 不能隱式轉成 int*，一定要顯式轉型。
//   這個小差異其實是個訊號：C++ 認為「指標型別轉換是需要被看見的事情」。
//
// 【3. malloc 對「有建構子的型別」根本是壞掉的】
//   這是本主題最重要的一點：
//       std::string* s = (std::string*)malloc(sizeof(std::string));
//       *s = "hello";        // 未定義行為！
//   為什麼？因為 malloc 只給了 sizeof(std::string) 個位元組，
//   但 std::string 內部的指標、長度、容量欄位（或 SSO 緩衝區）從來沒有被
//   建構子初始化過。此時對它做 operator= 會讓 string 以為自己已經持有一塊
//   heap 記憶體，而那個「指標」其實是隨機位元。
//   換句話說：malloc 給的是「記憶體」，new 給的是「物件」。
//   在 C++ 裡，物件的生命週期是從建構子完成才開始的。
//
// 【4. 錯誤處理：NULL 回傳 vs 例外】
//   malloc 失敗回傳 NULL，而且它「不會告訴你」——如果不檢查就直接解參考，
//   就是對空指標解參考。C 的慣用法是每次都檢查：
//       int* p = (int*)malloc(n * sizeof(int));
//       if (p == NULL) { /* 處理失敗 */ }
//   實務上這件事極容易被漏掉，而且會讓每個呼叫點都長出一段錯誤處理。
//   C++ 的 new 改成「失敗就丟 std::bad_alloc」，讓錯誤沿著呼叫堆疊自動上拋，
//   正常路徑的程式碼就不必被錯誤處理淹沒。
//
// 【5. realloc 的陷阱】
//   realloc 可能「原地擴充」，也可能「配置新區塊 + 複製 + 釋放舊的」。
//   所以下面這個寫法有嚴重問題：
//       p = realloc(p, newSize);      // 失敗時回傳 NULL，原本的 p 就洩漏了
//   正確做法是先用暫存變數接：
//       void* tmp = realloc(p, newSize);
//       if (tmp) p = tmp; else { /* p 仍然有效，自行處理失敗 */ }
//   另外，realloc 之後所有指向舊區塊的指標全部失效——這跟 std::vector
//   重新配置後 iterator 失效是同一回事。
//
// 【概念補充 Concept Deep Dive】
//   * malloc 實際配置的位元組通常多於你要求的：配置器要在區塊前後放
//     metadata（大小、狀態），並且做對齊。這就是為什麼 malloc(1) 也會
//     消耗遠多於 1 byte，以及為什麼大量小配置很浪費。
//   * malloc 回傳的位址保證對齊到 max_align_t（在常見 64-bit 平台上是 16 bytes），
//     足以放任何基本型別。要更嚴格的對齊（例如 SIMD 的 32/64 bytes）
//     需要 aligned_alloc（C11）或 posix_memalign。
//   * free(NULL) 是明確合法的空操作，這是刻意的設計，讓清理路徑不必到處寫
//     if (p) free(p)。但 free 同一個指標兩次（double free）是未定義行為，
//     而且是最常見的可利用漏洞之一。
//   * free 之後指標本身不會變成 NULL，它仍指向已釋放的位址（dangling pointer）。
//     防禦性寫法是 free(p); p = NULL;
//
// 【注意事項 Pay Attention】
//   1. malloc 的內容未初始化。要歸零請用 calloc，或自己 memset。
//      讀取未初始化的內容是未定義行為，不要假設它是 0。
//   2. malloc/free 必須成對；malloc 配的絕不能用 delete 釋放，
//      new 配的也絕不能用 free 釋放——兩者可能來自不同的配置器，混用是 UB。
//   3. malloc 不呼叫建構子、free 不呼叫解構子。在 C++ 裡對非 POD 型別
//      使用它們是錯的。
//   4. 一定要檢查回傳值是否為 NULL。
//   5. free 之後指標成為 dangling，再解參考或再 free 都是 UB。
//   6. 本檔的原始示範刻意沒有檢查 malloc 是否為 NULL（那是 C 教材的典型寫法），
//      下面的實務範例則示範了正確的檢查方式。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】malloc / free
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. malloc 和 new 最本質的差別是什麼？
//     答：malloc 配置的是「記憶體」，new 建立的是「物件」。
//         malloc 只向配置器要一塊未初始化的位元組並回傳 void*；
//         new 除了配置，還會呼叫建構子，回傳的是已經是正確型別、
//         已經完成初始化的物件指標。對應地，free 只還記憶體、
//         delete 會先呼叫解構子再還記憶體。
//     追問：那對 int 這種沒有建構子的型別，兩者是不是就一樣了？
//         → 語意上仍不同（new int 值初始化與否要看寫法、失敗會丟 bad_alloc），
//           而且兩者的記憶體不保證來自同一個配置器，所以仍然不可以混用。
//
// 🔥 Q2. 為什麼在 C++ 裡 malloc 一定要寫 (int*)，C 卻不用？
//     答：C 允許 void* 隱式轉換成任何物件指標；C++ 的型別系統不允許，
//         必須顯式轉型。這反映了 C++ 「指標型別轉換應該被明確寫出來」的設計取向。
//     追問：那 C 裡加不加轉型比較好？→ 傳統上主張「不要加」，
//         因為忘記 include <stdlib.h> 時，C 會把 malloc 當成回傳 int 的隱式宣告，
//         加上轉型反而會把這個錯誤蓋掉。這在 C++ 不成立，因為 C++ 沒有隱式函式宣告。
//
// ⚠️ 陷阱. 這段程式碼哪裡錯了？
//         std::string* s = (std::string*)malloc(sizeof(std::string));
//         *s = "hello";
//         free(s);
//     答：三處都錯。malloc 沒有呼叫 std::string 的建構子，
//         所以那塊記憶體裡的指標／長度／容量欄位全是未初始化的位元；
//         對它做 operator= 會讓 string 依據垃圾指標去操作記憶體，是未定義行為。
//         結尾的 free 也沒有呼叫解構子，即使前面僥倖沒爆，
//         string 內部配置的記憶體也會洩漏。
//         正確做法是 new std::string("hello") 搭配 delete，
//         或直接用 std::string s = "hello"; 根本不需要動態配置。
//     為什麼會錯：多數人把 sizeof(std::string) 理解成「這個字串佔多少空間」，
//         於是覺得「大小對了就可以用」。但 sizeof 只是那個「控制結構」的大小
//         （本機 g++ 15.2 / libstdc++ 上是 32 bytes，屬實作定義），
//         字串真正的內容可能在別處的 heap 上，而那條連結是建構子建立的。
//         跳過建構子，等於拿到一個外殼完好、內部指向隨機位址的物件。
//
// 註：LeetCode 的鏈結串列/樹題目雖然會動態配置節點，但正確做法一律是 new
//     （節點型別在 C++ 裡常有建構子），用 malloc 反而是本檔要示範的錯誤。
//     為避免示範一個不該學的寫法，本檔不附 LeetCode 範例；
//     new/delete 的 LeetCode 應用請見同一課的第 6 檔（LeetCode 707）。
// ═══════════════════════════════════════════════════════════════════════════

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// -----------------------------------------------------------------------------
// 【日常實務範例 1】動態成長的緩衝區：正確使用 realloc 的寫法
//
// 情境：讀取長度未知的資料（網路封包、log 行）時，需要一個會自己長大的
//   位元組緩衝區——這正是 std::vector 在底層替我們做的事。
// 為何用到本主題：完整示範 realloc 的正確用法：
//   用暫存變數承接回傳值，避免「失敗回傳 NULL 就把原指標弄丟」的洩漏。
// -----------------------------------------------------------------------------
typedef struct {
    char*  data;
    size_t len;       // 目前已使用
    size_t cap;       // 目前容量
} ByteBuffer;

int bufInit(ByteBuffer* b, size_t initialCap) {
    b->data = (char*)malloc(initialCap);
    if (b->data == NULL) {          // 每一次配置都必須檢查
        b->len = b->cap = 0;
        return 0;
    }
    b->len = 0;
    b->cap = initialCap;
    return 1;
}

int bufAppend(ByteBuffer* b, const char* text) {
    const size_t need = strlen(text);
    if (b->len + need > b->cap) {
        size_t newCap = (b->cap == 0) ? 1 : b->cap;
        while (newCap < b->len + need) newCap *= 2;      // 倍增策略，同 vector

        // 正確寫法：先用 tmp 接住，失敗時 b->data 仍然有效、沒有洩漏
        char* tmp = (char*)realloc(b->data, newCap);
        if (tmp == NULL) return 0;                       // 舊記憶體仍可用
        b->data = tmp;
        b->cap  = newCap;
    }
    memcpy(b->data + b->len, text, need);
    b->len += need;
    return 1;
}

void bufFree(ByteBuffer* b) {
    free(b->data);        // free(NULL) 是合法的空操作
    b->data = NULL;       // 避免留下 dangling pointer
    b->len = b->cap = 0;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】malloc 與 calloc 的差別：內容有沒有被歸零
//
// 情境：配置一張計數表（例如統計 0-9 每個數字出現幾次）時，
//   若用 malloc 而忘記歸零，累加的起點就是垃圾值。
// 為何用到本主題：這是「malloc 不初始化」最常見的實際踩坑點。
//   注意本函式不會印出 malloc 區塊的原始內容——那是未指定的值，
//   印出來既不可重現、也沒有教學意義。這裡只印「歸零之後」的結果。
// -----------------------------------------------------------------------------
void demoCallocVsMalloc(void) {
    const size_t N = 10;

    // calloc：保證全部歸零，可以直接累加
    int* counts = (int*)calloc(N, sizeof(int));
    if (counts == NULL) { printf("  calloc 失敗\n"); return; }

    const char* digits = "31415926535";
    for (const char* p = digits; *p; ++p) {
        counts[*p - '0']++;                   // 起點是 0，安全
    }
    printf("  用 calloc 統計 \"%s\"：\n   ", digits);
    for (size_t i = 0; i < N; ++i) printf(" %zu:%d", i, counts[i]);
    printf("\n");
    free(counts);

    // malloc：內容未指定，必須自己歸零才能當累加器
    int* counts2 = (int*)malloc(N * sizeof(int));
    if (counts2 == NULL) { printf("  malloc 失敗\n"); return; }
    memset(counts2, 0, N * sizeof(int));      // 少了這行就是未定義行為
    for (const char* p = digits; *p; ++p) {
        counts2[*p - '0']++;
    }
    printf("  用 malloc + memset 統計，結果相同：\n   ");
    for (size_t i = 0; i < N; ++i) printf(" %zu:%d", i, counts2[i]);
    printf("\n");
    free(counts2);
}

int main() {
    printf("=== 原始示範：配置單一變數 ===\n");
    // 配置單一變數
    int* p = (int*)malloc(sizeof(int));
    if (p == NULL) { printf("配置失敗\n"); return 1; }   // 原始碼漏了這個檢查
    *p = 42;
    printf("值: %d\n", *p);
    free(p);
    p = NULL;                                            // 避免 dangling pointer

    printf("\n=== 原始示範：配置陣列 ===\n");
    // 配置陣列
    int* arr = (int*)malloc(5 * sizeof(int));
    if (arr == NULL) { printf("配置失敗\n"); return 1; }
    for (int i = 0; i < 5; i++) {
        arr[i] = i * 10;
    }
    // 原始碼配置完就直接 free 了，這裡把值印出來才看得到效果
    printf("陣列內容:");
    for (int i = 0; i < 5; i++) printf(" %d", arr[i]);
    printf("\n");
    free(arr);
    arr = NULL;

    printf("\n=== sizeof：算大小是呼叫端的責任 ===\n");
    printf("sizeof(int)    = %zu bytes\n", sizeof(int));
    printf("sizeof(double) = %zu bytes\n", sizeof(double));
    printf("配置 5 個 int 需要 %zu bytes（本機 x86-64 / g++ 15.2）\n",
           5 * sizeof(int));

    printf("\n=== 日常實務 1：會自己長大的緩衝區（realloc 正確用法）===\n");
    {
        ByteBuffer b;
        if (bufInit(&b, 4)) {
            const char* parts[] = {"GET ", "/api/users", " HTTP/1.1"};
            for (int i = 0; i < 3; ++i) {
                if (!bufAppend(&b, parts[i])) { printf("  append 失敗\n"); break; }
                printf("  append %-12s → len=%zu cap=%zu\n", parts[i], b.len, b.cap);
            }
            printf("  最終內容：[%.*s]\n", (int)b.len, b.data);
            bufFree(&b);
        }
    }

    printf("\n=== 日常實務 2：calloc 歸零 vs malloc 不歸零 ===\n");
    demoCallocVsMalloc();

    printf("\n=== free 的兩個規則 ===\n");
    {
        int* q = NULL;
        free(q);                 // free(NULL) 合法，什麼都不做
        printf("  free(NULL) 是合法的空操作，執行到這裡代表沒事\n");
        printf("  但 double free（對同一指標 free 兩次）是未定義行為，不可示範\n");
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 2 課：C 與 C++ 的關鍵差異5.cpp" -o demo5
// 記憶體檢查: valgrind --leak-check=full ./demo5
//
// 說明：本檔所有輸出都與配置到的實際位址無關（刻意不印任何指標值，
//       因為位址每次執行都不同，且受 ASLR 影響）。
//       緩衝區的 cap 成長序列（實跑為 4 → 16 → 32）是本檔自己寫的倍增策略，
//       不是 malloc 或標準的規定。注意它不是每次都剛好翻一倍：
//       bufAppend 內的 while 迴圈會「一路倍增到足夠為止」，
//       所以一次追加 10 bytes 時是 4 → 8 → 16 連跳兩次，只有 16 被記錄下來。

// === 預期輸出 ===
// === 原始示範：配置單一變數 ===
// 值: 42
//
// === 原始示範：配置陣列 ===
// 陣列內容: 0 10 20 30 40
//
// === sizeof：算大小是呼叫端的責任 ===
// sizeof(int)    = 4 bytes
// sizeof(double) = 8 bytes
// 配置 5 個 int 需要 20 bytes（本機 x86-64 / g++ 15.2）
//
// === 日常實務 1：會自己長大的緩衝區（realloc 正確用法）===
//   append GET          → len=4 cap=4
//   append /api/users   → len=14 cap=16
//   append  HTTP/1.1    → len=23 cap=32
//   最終內容：[GET /api/users HTTP/1.1]
//
// === 日常實務 2：calloc 歸零 vs malloc 不歸零 ===
//   用 calloc 統計 "31415926535"：
//     0:0 1:2 2:1 3:2 4:1 5:3 6:1 7:0 8:0 9:1
//   用 malloc + memset 統計，結果相同：
//     0:0 1:2 2:1 3:2 4:1 5:3 6:1 7:0 8:0 9:1
//
// === free 的兩個規則 ===
//   free(NULL) 是合法的空操作，執行到這裡代表沒事
//   但 double free（對同一指標 free 兩次）是未定義行為，不可示範
