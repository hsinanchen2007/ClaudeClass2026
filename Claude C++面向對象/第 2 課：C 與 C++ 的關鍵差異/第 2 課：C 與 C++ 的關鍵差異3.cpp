// =============================================================================
//  第 2 課：C 與 C++ 的關鍵差異 3  —  C 風格 I/O：printf / scanf 與它的代價
// =============================================================================
//
// 【主題資訊 Information】
//   簽名     ：int printf(const char* format, ...);
//              int scanf (const char* format, ...);
//              int sscanf(const char* str, const char* format, ...);
//   回傳值   ：printf → 實際輸出的字元數；scanf/sscanf → 「成功指派的項目數」
//              （不是讀到的位元組數！），遇到 EOF 回傳 EOF (-1)
//   標準版本 ：C89 起；C++ 透過 <cstdio> 提供（本檔沿用原始碼的 <stdio.h>）
//   標頭檔   ：<stdio.h>（C）／<cstdio>（C++ 建議寫法，符號進 std::）
//
// 【詳細解釋 Explanation】
//
// 【1. printf 是「可變參數 + 執行期格式字串解譯」】
//   printf 的型別安全完全建立在「你寫的格式字串」與「你傳的參數」一致上，
//   而這件事編譯器本來是不檢查的——因為 ... (varargs) 在型別系統裡是個黑洞。
//   實際發生的事情是：
//     * 參數經過 default argument promotion（char/short → int，float → double）
//       之後被推上 stack 或暫存器；
//     * printf 在執行期逐字元掃描格式字串，看到 %d 就從參數區「取一個 int」，
//       看到 %s 就「取一個 pointer 然後一路讀到 '\0'」。
//   所以 printf("%d", 3.14) 不是「印出 3」，而是把 double 的位元當 int 解讀——
//   這是未定義行為（undefined behavior），結果由 ABI 與實作決定，不可預期。
//   GCC/Clang 之所以能抓到這類錯誤，是靠 __attribute__((format(printf,...)))
//   這個「編譯器擴充」對標準函式做特判，不是語言本身的能力。
//   這正是 C++ 要另外做 iostream / std::format 的核心動機：
//   讓「印出什麼型別」由型別系統決定，而不是由一個執行期字串決定。
//
// 【2. scanf("%s", ...) 為什麼是教科書等級的安全漏洞】
//   %s 的語意是「跳過前導空白，然後一路讀非空白字元，直到遇到空白或 EOF」。
//   注意：這裡完全沒有「目標緩衝區有多大」的資訊——scanf 不知道 name[] 只有
//   50 bytes。使用者輸入 200 個字元，它就寫 200 個字元 + 一個 '\0'，
//   直接踩爛 stack 上相鄰的資料。這就是經典的 stack buffer overflow。
//   正確寫法是用「最大欄寬」限制：
//       char name[50];
//       scanf("%49s", name);      // 49 = sizeof(name) - 1，留一格給 '\0'
//   注意欄寬是「不含結尾 '\0'」的字元數，所以是 49 不是 50——這個 off-by-one
//   本身也是常見錯誤。更麻煩的是欄寬必須寫死在格式字串裡，沒辦法用 sizeof
//   算出來（除非動用巨集拼接），這使得緩衝區大小一改就得同步改格式字串。
//
// 【3. scanf 的回傳值：最常被忽略的那個 int】
//   scanf 回傳「成功指派的項目數」。scanf("%d", &age) 若使用者輸入 "abc"：
//     * 回傳 0（沒有任何項目被成功指派）；
//     * age 完全沒有被寫入 —— 如果 age 當初沒有初始化，它就維持
//       indeterminate value（不定值），此時讀取它是未定義行為；
//     * 更糟的是 "abc" 還留在輸入緩衝區裡沒被消耗，下一次 scanf("%d") 會
//       再次失敗、再次回傳 0，形成無窮迴圈。這是初學者寫互動選單時
//       「程式突然狂捲」的頭號原因。
//   所以任何 scanf 都應該檢查回傳值，而且失敗後要主動清掉那行輸入。
//
// 【4. printf/scanf 相對於 iostream 的取捨】
//   C 風格 I/O 並非一無是處：
//     * 格式控制精簡：printf("%.2f%%", x) 比 iostream 的
//       std::setprecision(2) << std::fixed 一長串直觀得多；
//     * 產生的執行檔較小（不必實例化一堆 template）；
//     * 對已經是 C 的程式碼庫，混用 iostream 反而要處理同步問題。
//   但它換來的代價是：型別不安全、無法擴充到自訂型別（沒有 %MyClass）、
//   無法 RAII、錯誤處理只有一個 int 回傳值。
//   C++20 起的 std::format 才真正兩者兼得：printf 式的簡潔語法 +
//   編譯期格式字串檢查 + 型別安全 + 可擴充。
//
// 【概念補充 Concept Deep Dive】
//   * varargs 沒有型別資訊：printf 拿到的只是一段參數區與一個字串。
//     它「相信」格式字串，這就是所謂的 format string vulnerability 根源——
//     若把使用者輸入直接當格式字串（printf(userInput)），使用者輸入 "%n"
//     甚至可以寫入記憶體。格式字串永遠必須是程式寫死的字面值。
//   * default argument promotion 的實際後果：printf("%f", 1.0f) 是對的，
//     因為 float 已被提升成 double；但 printf("%hhd", ...) 這種小型別
//     反而要靠長度修飾詞把 int 縮回去。
//   * scanf 的 %s 與 %d 對「前導空白」的處理都是跳過，但 %c 不是——
//     %c 會讀到那個殘留的換行符，這是「上一次讀完數字後，下一次讀字元
//     讀到空白」的經典坑。
//
// 【注意事項 Pay Attention】
//   1. scanf("%s", buf) 沒有邊界檢查 → 一律改用 scanf("%49s", buf) 之類的欄寬。
//   2. 一定要檢查 scanf 的回傳值；失敗時輸入緩衝區還留著壞資料，
//      要用類似 while ((c = getchar()) != '\n' && c != EOF) 的方式清掉。
//   3. 讀取「未初始化且 scanf 失敗的變數」是未定義行為。不要假設它是 0，
//      也不要假設它一定是某個垃圾值——編譯器最佳化等級不同結果就可能不同。
//      本檔的示範一律把變數初始化，並且只印出有確實被寫入的值。
//   4. 格式字串永遠不要用使用者輸入。printf(msg) 是漏洞，printf("%s", msg) 才對。
//   5. C++ 中混用 printf 與 std::cout 時，若關閉了
//      std::ios_base::sync_with_stdio(false)，兩者的輸出順序就不再保證。
//      預設是同步的，可以安全混用。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】C 風格 I/O
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. scanf("%s", name) 有什麼問題？怎麼修？
//     答：%s 不知道 name 有多大，會一路寫到遇見空白為止，輸入夠長就造成
//         stack buffer overflow。修法是加上最大欄寬：scanf("%49s", name)，
//         其中 49 = sizeof(name) - 1，那一格留給結尾的 '\0'。
//     追問：為什麼是 49 不是 50？→ scanf 的欄寬指的是「最多讀幾個字元」，
//         不含它自動補上的 '\0'。寫 50 就會寫入 51 bytes，還是溢位。
//
// 🔥 Q2. scanf 的回傳值是什麼？不檢查會怎樣？
//     答：回傳「成功指派的項目數」，不是讀到的位元組數。scanf("%d", &age)
//         碰到非數字輸入會回傳 0，且 age 完全沒被寫入。若 age 未初始化，
//         此時讀它就是未定義行為。而且壞輸入還留在緩衝區，
//         迴圈裡再讀一次還是失敗，於是無窮迴圈。
//     追問：失敗後怎麼復原？→ 必須主動把該行剩餘內容讀掉，
//         例如 while ((c = getchar()) != '\n' && c != EOF) {}
//
// ⚠️ 陷阱. printf("%d\n", 3.14) 會印出什麼？
//     答：這是未定義行為，沒有「會印出什麼」的正確答案。%d 要求從參數區取一個
//         int，但實際傳入的是 double（而且在常見的 x86-64 ABI 上，浮點數走的是
//         XMM 暫存器、整數走一般暫存器，根本不在同一個地方），
//         於是 printf 取到的是完全無關的內容。
//         GCC/Clang 會用 -Wformat 警告，但那是編譯器擴充的好意，不是語言保證。
//     為什麼會錯：很多人以為 printf 會「幫忙轉型」成 int 印出 3，
//         把它想成 std::cout 那種有型別資訊的東西。
//         但 printf 收到的參數已經丟失型別，它只能盲目相信格式字串。
//
// 註：C 風格 I/O 是語言/安全主題，LeetCode 的判題是把輸入直接餵給函式參數，
//     不考 I/O 函式本身，硬套任何題號都是牽強，故本檔不附 LeetCode 範例。
// ═══════════════════════════════════════════════════════════════════════════

#include <stdio.h>
#include <string.h>

// -----------------------------------------------------------------------------
// 【日常實務範例 1】用 sscanf 解析 Nginx 風格的 access log 時間戳與狀態碼
//
// 情境：維運時常要從一行 log 抽出「時間 + HTTP 狀態碼 + 回應毫秒數」做統計。
// 為何用到本主題：sscanf 是 C 解析固定格式字串最短的寫法，
//   而它的正確用法核心就在「檢查回傳值是否等於預期的欄位數」——
//   這正是 scanf 家族最常被忽略的一點。
//   注意所有輸出變數都先初始化，這樣即使解析失敗也不會讀到不定值。
// -----------------------------------------------------------------------------
int parseAccessLog(const char* line, int* outHour, int* outMin,
                   int* outStatus, int* outMillis) {
    int hh = 0, mm = 0, status = 0, ms = 0;

    // 期望格式： "10:15 GET /api/users 200 37ms"
    // 回傳值必須「剛好等於 4」才代表四個欄位都成功指派
    const int matched = sscanf(line, "%d:%d %*s %*s %d %dms", &hh, &mm, &status, &ms);
    if (matched != 4) {
        return 0;                    // 解析失敗，不動 out 參數
    }

    *outHour   = hh;
    *outMin    = mm;
    *outStatus = status;
    *outMillis = ms;
    return 1;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】安全地把字串解析成整數埠號（示範欄寬 + 回傳值檢查）
//
// 情境：從設定檔讀到 "port=8080" 這種字串，要轉成整數並驗證範圍。
// 為何用到本主題：示範 %49s 的欄寬保護，以及「回傳值 != 預期欄位數就當失敗」。
// -----------------------------------------------------------------------------
int parsePortSetting(const char* text, int* outPort) {
    char key[50] = {0};              // 先歸零，避免任何未初始化的讀取
    int  port    = 0;

    // %49s 明確限制最多讀 49 個字元，第 50 格留給 '\0'
    const int matched = sscanf(text, "%49[^=]=%d", key, &port);
    if (matched != 2) return 0;
    if (strcmp(key, "port") != 0) return 0;
    if (port < 1 || port > 65535)  return 0;   // 埠號合法範圍

    *outPort = port;
    return 1;
}

int main() {
    // ── 原始示範：C 風格互動式輸入 ──────────────────────────────────
    // 注意：age 與 name 都先初始化，並且下面會檢查 scanf 的回傳值，
    //       所以不論輸入是什麼，本程式都不會讀到不定值。
    int  age     = 0;
    char name[50] = {0};

    printf("請輸入你的名字: ");
    // 原始碼是 scanf("%s", name);  ← 沒有邊界，會溢位
    // 這裡改成有欄寬的安全版本，並檢查回傳值
    const int gotName = scanf("%49s", name);

    printf("請輸入你的年齡: ");
    const int gotAge = scanf("%d", &age);

    if (gotName == 1 && gotAge == 1) {
        printf("你好 %s，你今年 %d 歲\n", name, age);
    } else {
        // 只印出「解析結果」，絕不印出沒有被成功寫入的變數
        printf("輸入解析失敗：gotName=%d gotAge=%d\n", gotName, gotAge);
    }

    // ── 以下改用 sscanf 對字面字串解析，輸出完全可重現 ───────────────
    printf("\n=== 日常實務 1：解析 access log ===\n");
    const char* logs[] = {
        "10:15 GET /api/users 200 37ms",
        "23:04 POST /api/login 500 812ms",
        "這行格式不對"
    };
    for (int i = 0; i < 3; ++i) {
        int hh = 0, mm = 0, st = 0, ms = 0;
        if (parseAccessLog(logs[i], &hh, &mm, &st, &ms)) {
            printf("  %02d:%02d  status=%d  %dms%s\n",
                   hh, mm, st, ms, (st >= 500 ? "  <-- 伺服器錯誤" : ""));
        } else {
            printf("  解析失敗：%s\n", logs[i]);
        }
    }

    printf("\n=== 日常實務 2：解析埠號設定 ===\n");
    const char* settings[] = {
        "port=8080",
        "port=70000",      // 超出合法範圍
        "host=127.0.0.1",  // key 不對
        "port=abc"         // 值不是數字
    };
    for (int i = 0; i < 4; ++i) {
        int port = 0;
        if (parsePortSetting(settings[i], &port)) {
            printf("  [%s] -> 合法埠號 %d\n", settings[i], port);
        } else {
            printf("  [%s] -> 拒絕\n", settings[i]);
        }
    }

    printf("\n=== scanf 回傳值的意義 ===\n");
    {
        int a = 0, b = 0;
        // 對字面字串示範：第二個欄位不是數字，所以只有 1 個項目被指派
        const int n = sscanf("42 abc", "%d %d", &a, &b);
        printf("  sscanf(\"42 abc\", \"%%d %%d\") 回傳 %d\n", n);
        printf("  → 只有 a 被成功指派，a=%d；b 未被寫入，維持初始值 %d\n", a, b);
        printf("  → 若 b 當初沒有初始化，這裡讀 b 就是未定義行為\n");
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 2 課：C 與 C++ 的關鍵差異3.cpp" -o demo3
// 執行: printf 'Alice\n20\n' | ./demo3
//
// 說明：本程式開頭會從 stdin 讀取姓名與年齡。下面的預期輸出是以
//       printf 'Alice\n20\n' | ./demo3 取得的實際結果。
//       換不同輸入，開頭那行「你好 ...」自然會不同；其餘段落與輸入無關，固定不變。
//       另外 printf 的提示字串結尾沒有換行，所以兩行提示會與後續輸出接在同一行。

// === 預期輸出 ===
// 請輸入你的名字: 請輸入你的年齡: 你好 Alice，你今年 20 歲
//
// === 日常實務 1：解析 access log ===
//   10:15  status=200  37ms
//   23:04  status=500  812ms  <-- 伺服器錯誤
//   解析失敗：這行格式不對
//
// === 日常實務 2：解析埠號設定 ===
//   [port=8080] -> 合法埠號 8080
//   [port=70000] -> 拒絕
//   [host=127.0.0.1] -> 拒絕
//   [port=abc] -> 拒絕
//
// === scanf 回傳值的意義 ===
//   sscanf("42 abc", "%d %d") 回傳 1
//   → 只有 a 被成功指派，a=42；b 未被寫入，維持初始值 0
//   → 若 b 當初沒有初始化，這裡讀 b 就是未定義行為
