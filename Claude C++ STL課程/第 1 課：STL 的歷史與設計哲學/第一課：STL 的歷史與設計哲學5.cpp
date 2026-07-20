// =============================================================================
//  第一課：STL 的歷史與設計哲學 5  —  對照組：沒有 STL 的世界長什麼樣
// =============================================================================
//
// 【主題資訊 Information】
//   本檔是「STL 之前」的對照組，全部用 C 標準函式庫完成：
//     malloc / realloc / free   <stdlib.h>   手動記憶體管理
//     qsort                     <stdlib.h>   通用排序，透過函式指標比較
//     printf / fprintf          <stdio.h>    輸出
//   對照的 STL 版本請看同課的 summary.cpp 與第 6 檔。
//   ⚠️ 本檔雖然是 C 風格，仍以 g++ 編譯（副檔名 .cpp），
//      所以 malloc 的回傳值必須顯式轉型——這正是 C 與 C++ 的一個真實差異。
//
// 【詳細解釋 Explanation】
//
// 【1. 這 60 行在做的事，STL 版本用 10 行】
//   任務只有四件：收集數字、排序、找最大值、加總。
//   C 版本卻要處理：
//     ① 初始容量要開多大
//     ② 滿了要 realloc，倍率自己決定
//     ③ realloc 可能失敗，失敗時原指標仍有效，不能直接覆蓋（否則洩漏）
//     ④ 每條錯誤路徑都要記得 free
//     ⑤ qsort 需要一個型別不安全的比較函式
//     ⑥ 正常結束也要記得 free
//   這六件事沒有一件跟「排序與加總」這個問題本身有關，
//   全部都是「因為要自己管記憶體」而附帶的稅。
//
// 【2. realloc 的正確用法：為什麼要用 temp 變數】
//   常見的錯誤寫法是：
//       numbers = (int*)realloc(numbers, newSize);   // 危險！
//   若 realloc 失敗回傳 NULL，原本的 numbers 指標就被 NULL 覆蓋了，
//   那塊記憶體再也沒有人指向它 —— 記憶體洩漏，而且無法補救。
//   正確寫法（本檔採用）是先接到暫存指標，確認非 NULL 再賦值回去。
//   vector 的 push_back 把這整套邏輯連同強例外安全保證一起封裝掉了。
//
// 【3. qsort 的比較函式：型別安全在這裡整個消失】
//   qsort 的簽章是
//       void qsort(void* base, size_t nmemb, size_t size,
//                  int (*compar)(const void*, const void*));
//   三個地方會出事而編譯器不會警告你：
//     ① 比較函式收 const void*，你要自己轉型回正確型別。
//        轉錯型別（例如把 int* 當 long* 解）能編譯，執行期讀到垃圾。
//     ② size 參數要自己填 sizeof(int)。填錯了照樣能編譯。
//     ③ 陣列型別和比較函式的型別沒有任何關聯，編譯器無從檢查。
//   std::sort 把這三個風險全部消滅：型別由 iterator 推導，
//   元素大小由編譯器計算，比較子的參數型別對不上就編譯失敗。
//
// 【4. 本檔保留的一個真實地雷：相減式比較函式】
//   原始碼中的
//       int compare(const void* a, const void* b) {
//           return (*(int*)a - *(int*)b);
//       }
//   是幾乎每本 C 教科書都會出現的寫法，而它是錯的。
//   當兩數相差超過 INT_MAX 時（例如 a = 2000000000, b = -2000000000），
//   減法會發生「有號整數溢位」——這在 C 和 C++ 都是未定義行為，
//   不是「回繞成負數」這種可預期的結果。開了最佳化之後，
//   編譯器有權假設溢位不會發生，因而產生出乎意料的排序結果。
//   本檔保留它（因為測資很小、不會觸發），但同時提供正確的三路比較版本，
//   並在 main 中用會觸發溢位的資料實測兩者的差異。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 這裡的手寫成長策略，就是 vector 內部在做的事
//     `capacity *= 2` 是幾何成長。為什麼不是 `capacity += 1`？
//     因為每次成長都要把舊資料複製到新位置（O(n)）。
//     若每次只加 1，插入 n 個元素的總成本是 1+2+…+n = O(n²)。
//     倍增則讓總搬移量收斂成 O(n)，攤還下來每次 push 是 O(1)。
//     libstdc++ 的 vector 用的正是 2×（實作定義；MSVC 用 1.5×）。
//     標準本身只要求「攤還 O(1)」，沒有規定倍率。
//
// (B) C 版本沒有、也做不到的：例外安全與 RAII
//     本檔的每條錯誤路徑都要手動 free。只要多加一個 return 就可能漏掉。
//     C++ 的 vector 在解構子裡釋放記憶體，不管函式是正常返回、
//     提前 return、還是拋出例外，記憶體都會被回收。
//     這是 RAII 的核心價值，而它在 C 裡沒有對應物
//     （GCC 的 __attribute__((cleanup)) 是編譯器擴充，非標準）。
//
// (C) 為什麼 malloc 在 .cpp 裡一定要轉型
//     C 允許 void* 隱式轉成任何物件指標，所以 C 程式寫
//       int* p = malloc(n);        // C 合法
//     C++ 移除了這個隱式轉換（型別安全考量），所以必須寫
//       int* p = (int*)malloc(n);  // C++ 必須顯式轉型
//     這也是「同一份 C 程式碼未必是合法 C++」的常見例子之一。
//
// 【注意事項 Pay Attention】
//   1. 相減式比較函式在大數值下是未定義行為。不要因為「一直都這樣寫」
//      就沿用；正確做法是三路比較（本檔的 compare_safe）。
//   2. realloc 失敗時原記憶體仍然有效，必須用暫存指標接，否則洩漏。
//   3. 本檔的 free 只有在「正常結束」與「已知錯誤路徑」被呼叫。
//      真實專案中只要控制流一複雜，漏 free 幾乎是必然。
//   4. qsort 的元素大小與比較函式的型別由程式設計師負責一致，
//      編譯器不檢查。這類 bug 通常只在特定資料下才浮現。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】C 風格資源管理與 qsort
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. `ptr = realloc(ptr, newSize);` 這行常見寫法有什麼問題？
//     答：realloc 失敗時回傳 NULL，但原本那塊記憶體仍然有效。
//         把 NULL 直接賦值回 ptr，就永遠失去了原記憶體的位址 → 洩漏，
//         而且無法補救。正確做法是先用暫存指標接住，
//         確認非 NULL 之後才賦值回去。
//     追問：realloc 成功時，原指標還能用嗎？
//         → 不能。realloc 可能搬移到新位置，原指標即成為懸空指標，
//           任何解參考都是未定義行為。
//
// 🔥 Q2. 為什麼手動實作動態陣列時要用「容量倍增」而不是「每次加 1」？
//     答：每次成長都要複製舊資料，是 O(n)。若每次只加 1，
//         插入 n 個元素總成本是 1+2+…+n = O(n²)。倍增讓總搬移量
//         收斂為 O(n)，攤還後每次插入是 O(1)。
//         這正是 std::vector 的成長策略（libstdc++ 用 2×、MSVC 用 1.5×，
//         皆為實作定義；標準只要求攤還 O(1)）。
//     追問：那 1.5× 和 2× 有什麼取捨？
//         → 1.5× 比較省記憶體，且理論上釋放的舊區塊有機會被後續配置
//           重複利用；2× 的重新配置次數較少。兩者都滿足攤還 O(1)。
//
// ⚠️ 陷阱. qsort 的比較函式寫成 `return *(int*)a - *(int*)b;`，
//         這是 C 教科書的標準寫法，為什麼它其實是錯的？
//     答：兩數相差超過 INT_MAX 時會發生有號整數溢位，那是未定義行為。
//         例如 a = 2000000000、b = -2000000000，數學上差是 4000000000，
//         遠超 int 能表示的範圍。UB 意味著「任何結果都可能」——
//         不是「一定會回繞成負數」。開了最佳化之後，編譯器可以基於
//         「溢位不會發生」這個假設做推導，產生完全出乎意料的排序結果。
//         正解是三路比較：if (x < y) return -1; if (x > y) return 1; return 0;
//     為什麼會錯：把「int 的算術等同於數學上的整數運算」當成理所當然。
//         實際上 C/C++ 的有號整數溢位是 UB，而不是有定義的環繞
//         （unsigned 才是有定義的環繞）。這個寫法在小數值測資下
//         永遠測不出問題，才會流傳這麼久。
// ═══════════════════════════════════════════════════════════════════════════

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>   // INT_MAX / INT_MIN

// 比較函數（給 qsort 用）
// ⚠️ 這是教科書上最常見的寫法，但兩數相差超過 INT_MAX 時會有號溢位（UB）。
//    本檔的測資都是小數字，不會觸發；保留它是為了讓你認得這個 pattern。
//    正式程式請一律使用下面的 compare_safe。
int compare(const void* a, const void* b) {
    return (*(int*)a - *(int*)b);
}

// 正確的三路比較：任何數值範圍都安全，因為完全沒有算術運算
int compare_safe(const void* a, const void* b) {
    int x = *(const int*)a;
    int y = *(const int*)b;
    if (x < y) return -1;
    if (x > y) return 1;
    return 0;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】讀取「未知筆數」的量測值，動態擴充緩衝區
//   情境：從感測器 / 序列埠 / 檔案讀取數值，事前不知道總共有幾筆，
//         只能邊讀邊擴充緩衝區。這是動態陣列最原始的需求，
//         也是 std::vector 誕生的理由。
//   為什麼用到本主題：這裡把 vector 的內部工作完整攤開一次——
//     容量、成長倍率、realloc 失敗處理、釋放。看過這一遍之後，
//     才會真正理解 v.push_back(x) 這一行替你擋掉了多少事。
//   回傳：成功回傳 1 並更新 *arr/*size/*cap；失敗回傳 0（原資料保持有效）。
// -----------------------------------------------------------------------------
int push_back_int(int** arr, int* size, int* cap, int value) {
    if (*size >= *cap) {
        int newCap = (*cap > 0) ? (*cap * 2) : 4;     // 幾何成長：攤還 O(1)
        /* 關鍵：先接到暫存指標。若直接寫 *arr = realloc(*arr, ...)，
           失敗時 NULL 會蓋掉原指標，那塊記憶體就永遠找不回來了。 */
        int* temp = (int*)realloc(*arr, (size_t)newCap * sizeof(int));
        if (temp == NULL) {
            return 0;                                  // 原 *arr 仍然有效
        }
        *arr = temp;
        *cap = newCap;
    }
    (*arr)[(*size)++] = value;
    return 1;
}

int main() {
    int* numbers = NULL;
    int capacity = 4;
    int size = 0;

    printf("=== C 風格：手動管理動態陣列 ===\n");

    // 動態配置
    numbers = (int*)malloc(capacity * sizeof(int));
    if (numbers == NULL) {
        fprintf(stderr, "記憶體配置失敗\n");
        return 1;
    }

    // 模擬輸入
    int inputs[] = {5, 2, 8, 1, 9, 3, 7, 4, 6};
    int input_count = sizeof(inputs) / sizeof(inputs[0]);

    for (int i = 0; i < input_count; ++i) {
        // 需要擴展嗎？
        if (size >= capacity) {
            capacity *= 2;
            int* temp = (int*)realloc(numbers, capacity * sizeof(int));
            if (temp == NULL) {
                fprintf(stderr, "記憶體重新配置失敗\n");
                free(numbers);          // 每條錯誤路徑都必須自己記得 free
                return 1;
            }
            numbers = temp;
        }
        numbers[size++] = inputs[i];
    }

    printf("最終 size=%d, capacity=%d（初始 4，倍增到 16）\n", size, capacity);

    // 排序
    qsort(numbers, size, sizeof(int), compare);

    // 找最大值、計算總和
    int max = numbers[0];
    int sum = 0;
    for (int i = 0; i < size; ++i) {
        if (numbers[i] > max) max = numbers[i];
        sum += numbers[i];
    }

    printf("排序後: ");
    for (int i = 0; i < size; ++i) {
        printf("%d ", numbers[i]);
    }
    printf("\n");
    printf("最大值: %d\n", max);
    printf("總和: %d\n", sum);

    // 別忘了釋放記憶體！
    free(numbers);
    numbers = NULL;              // 良好習慣：避免後續誤用懸空指標

    // -------------------------------------------------------------------------
    printf("\n=== 相減式比較函式的地雷（用安全版排序，只展示差值）===\n");
    {
        /* 這組資料的兩兩差值會超出 int 範圍。
           我們「不」用相減式比較函式去排它——那會是未定義行為，
           結果不可預測，也不該寫進教材當成固定輸出。
           這裡改用 long long 把「數學上的差」算出來，
           讓你看到它確實塞不進 int。 */
        int extreme[] = {2000000000, -2000000000, 0, 1000000000};
        int n = (int)(sizeof(extreme) / sizeof(extreme[0]));

        long long diff = (long long)extreme[0] - (long long)extreme[1];
        printf("2000000000 - (-2000000000) 的數學值 = %lld\n", diff);
        printf("int 能表示的上限 INT_MAX      = %d\n", INT_MAX);
        printf("差值是否超出 int 範圍: %s\n", (diff > INT_MAX) ? "是" : "否");
        printf("→ 相減式比較函式在這裡會有號溢位，那是未定義行為，\n");
        printf("  不保證任何特定結果（不是「一定變成負數」）。\n");

        qsort(extreme, n, sizeof(int), compare_safe);   // 用安全版
        printf("用 compare_safe 排序的正確結果: ");
        for (int i = 0; i < n; ++i) printf("%d ", extreme[i]);
        printf("\n");
    }

    // -------------------------------------------------------------------------
    printf("\n=== 日常實務：未知筆數的動態緩衝（把 vector 的內部攤開）===\n");
    {
        int* buf = NULL;
        int  bufSize = 0;
        int  bufCap = 0;

        printf("逐筆 push，觀察容量怎麼成長：\n");
        for (int i = 1; i <= 10; ++i) {
            int oldCap = bufCap;
            if (!push_back_int(&buf, &bufSize, &bufCap, i * 11)) {
                fprintf(stderr, "擴充失敗\n");
                free(buf);
                return 1;
            }
            if (bufCap != oldCap) {
                printf("  第 %2d 筆觸發擴容: capacity %d -> %d\n", i, oldCap, bufCap);
            }
        }
        printf("最終內容: ");
        for (int i = 0; i < bufSize; ++i) printf("%d ", buf[i]);
        printf("\n");
        printf("size=%d capacity=%d\n", bufSize, bufCap);
        printf("→ 同樣的事，C++ 只要 std::vector<int> v; v.push_back(x);\n");
        printf("  而且離開作用域自動釋放，不會因為中途 return 而洩漏。\n");

        free(buf);
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第一課：STL 的歷史與設計哲學5.cpp -o demo5
// （本檔是 C 風格，但以 .cpp 副檔名用 g++ 編譯；malloc 因此必須顯式轉型）

// === 預期輸出 ===
// === C 風格：手動管理動態陣列 ===
// 最終 size=9, capacity=16（初始 4，倍增到 16）
// 排序後: 1 2 3 4 5 6 7 8 9
// 最大值: 9
// 總和: 45
//
// === 相減式比較函式的地雷（用安全版排序，只展示差值）===
// 2000000000 - (-2000000000) 的數學值 = 4000000000
// int 能表示的上限 INT_MAX      = 2147483647
// 差值是否超出 int 範圍: 是
// → 相減式比較函式在這裡會有號溢位，那是未定義行為，
//   不保證任何特定結果（不是「一定變成負數」）。
// 用 compare_safe 排序的正確結果: -2000000000 0 1000000000 2000000000
//
// === 日常實務：未知筆數的動態緩衝（把 vector 的內部攤開）===
// 逐筆 push，觀察容量怎麼成長：
//   第  1 筆觸發擴容: capacity 0 -> 4
//   第  5 筆觸發擴容: capacity 4 -> 8
//   第  9 筆觸發擴容: capacity 8 -> 16
// 最終內容: 11 22 33 44 55 66 77 88 99 110
// size=10 capacity=16
// → 同樣的事，C++ 只要 std::vector<int> v; v.push_back(x);
//   而且離開作用域自動釋放，不會因為中途 return 而洩漏。
