// =============================================================================
//  第 24 課：類別內的靜態成員變數 6  —  sizeof 證明靜態成員不佔物件空間
// =============================================================================
//
// 【主題資訊 Information】
//   語法:  sizeof(ClassName)  → 該類別「一個物件」佔的位元組數
//   標準保證: 靜態成員不是物件表示的一部分，因此不計入 sizeof
//   標準版本: 靜態成員 C++98；inline static 就地初始化為 C++17
//   複雜度: sizeof 是編譯期常數，執行期成本為 0
//   本機實測值（x86-64 / GCC，屬實作定義）:
//     sizeof(int)=4、sizeof(double)=8、兩者皆為 8（見檔尾實際輸出）
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼靜態成員不佔物件空間】
//   物件的大小由「它自己必須攜帶的資料」決定。
//   非靜態成員每個物件各一份，所以必須放在物件內部；
//   靜態成員整個程式只有一份，放在靜態儲存區（.data / .bss），
//   所有物件共用同一個位址。既然不在物件裡，自然不計入 sizeof。
//   本檔用 WithStatic 與 WithoutStatic 對照 —— 前者多了兩個靜態成員
//   （一個 int、一個 double），大小卻完全相同，這就是直接證據。
//
// 【2. 這不是最佳化，是語意】
//   常見誤解是「編譯器很聰明，幫我們省下來了」。
//   不是。標準規定靜態成員具有靜態儲存期、不屬於物件表示，
//   所以就算關掉所有最佳化（-O0），結果仍然一樣。
//   可以自己驗證：加上 -O0 重新編譯，sizeof 不會變。
//
// 【3. sizeof 的值是實作定義的】
//   本機（x86-64、GCC）兩個 int 相鄰、無需 padding，得到 8 bytes。
//   但這不是標準保證的數字：
//     * sizeof(int) 由實作決定（標準只保證至少 16 bits）。
//     * 成員之間可能被插入 padding 以滿足對齊要求。
//     * 空類別的 sizeof 必定 >= 1（保證不同物件有不同位址）。
//   所以「兩個 int 就是 8 bytes」只在特定平台成立，
//   要寫可攜程式碼就不能把它寫死。
//
// 【4. 實務意義：資料量大時差距是量級的】
//   單一物件省 8 bytes 沒有意義，但物件有一百萬個時就是 8 MB。
//   判斷準則：這個欄位「對每個物件都一樣嗎」?
//   一樣就該是 static（或移到共用的設定物件），
//   讓每個物件只帶真正屬於自己的資料。
//   本檔下方的粒子系統範例會用實測數字呈現這個差距。
//
// 【概念補充 Concept Deep Dive】
//   * 記憶體佈局：非靜態成員依宣告順序排在物件內（同一存取權限區段內），
//     編譯器可插入 padding 但不得重排。靜態成員則在連結期被配置到
//     .data（有非零初值）或 .bss（零初值）節區。
//   * 空類別 class E {}; 的 sizeof 是 1（不是 0）——
//     因為兩個不同物件必須有不同位址。但空基底類別可被最佳化掉
//     （Empty Base Optimization），這是標準允許的例外。
//   * 加入虛擬函式會讓 sizeof 變大（多一個 vptr，本機為 8 bytes），
//     這與靜態成員無關，是另一個機制。
//   * sizeof 只算「物件本身」。像 std::string 成員，
//     sizeof 只含它的控制結構（本機 32 bytes），
//     堆積上的字元資料不計入。
//
// 【注意事項 Pay Attention】
//   1. sizeof 的具體數值是實作定義，不要寫進斷言或序列化格式。
//   2. 靜態成員不計入 sizeof 是標準語意，不是最佳化結果。
//   3. 空類別 sizeof >= 1，不是 0。
//   4. 有虛擬函式時 sizeof 會多出 vptr 的大小（本機 8 bytes）。
//   5. sizeof 不含堆積配置的內容（指標成員只算指標本身）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】sizeof 與靜態成員
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 類別加了靜態成員變數，sizeof 會變大嗎?為什麼?
//     答：不會。靜態成員具有靜態儲存期，整個程式只有一份，
//         存放在 .data / .bss，不屬於物件表示的一部分。
//         sizeof 量的是「單一物件要攜帶多少資料」，
//         所以靜態成員完全不計入。
//     追問：這是編譯器最佳化嗎?
//         → 不是，是標準規定的語意。用 -O0 編譯結果一模一樣。
//
// 🔥 Q2. 空類別 class E {}; 的 sizeof 是多少?為什麼不是 0?
//     答：至少 1（本機 GCC 為 1）。因為標準要求「同型別的不同物件
//         必須有不同的位址」，大小為 0 的話陣列中相鄰元素會同位址。
//     追問：那 Empty Base Optimization 是什麼?
//         → 當空類別作為「基底類別」時，允許不佔額外空間，
//         所以 struct D : E { int x; }; 的 sizeof 可以是 4 而非 8。
//         這是標準明文允許的例外，也是 std::allocator 等
//         無狀態物件常被寫成基底類別的原因。
//
// ⚠️ 陷阱. 「sizeof 把物件用到的記憶體都算進去了。」
//     答：沒有。sizeof 只算物件自身的表示，且是編譯期常數。
//         class Doc { std::string body_; }; 的 sizeof 只有
//         std::string 控制結構的大小（本機 32 bytes），
//         不論 body_ 裡裝的是 10 個字還是 10 MB。
//     為什麼會錯：把「這個物件總共用掉多少記憶體」和
//         「這個物件本體多大」混為一談。
//         前者要遞迴走訪所有堆積配置才算得出來，
//         而 sizeof 在編譯期就決定了，根本不可能知道執行期的內容。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】—— 從缺，理由如下
//   sizeof 與物件記憶體佈局屬於語言的實作模型，
//   LeetCode 判題只看輸入輸出，不會考記憶體佈局。
//   硬掛一題不相關的反而誤導，故從缺；
//   下方改以「資料量放大後記憶體差距」的實務範例呈現同一個重點。
//
// =============================================================================

#include <iostream>
#include <string>
using namespace std;

class WithStatic {
    int a_;                          // 4 bytes
    int b_;                          // 4 bytes
    inline static int shared_ = 0;    // 不計入 sizeof, 所有對象共享同一份資料
    inline static double config_ = 0; // 不計入 sizeof, 所有對象共享同一份資料
};

class WithoutStatic {
    int a_;   // 4 bytes
    int b_;   // 4 bytes
};

// -----------------------------------------------------------------------------
// 【日常實務範例】粒子系統：把「每顆粒子都一樣」的欄位移成 static
//   情境：遊戲的粒子特效，同一批粒子共用同一張貼圖名稱與重力參數。
//         若每顆粒子各存一份，資料量放大後就是純浪費。
//   下面兩個版本欄位語意相同，差別只在「共用的欄位是不是 static」，
//   用 sizeof 實測差距，再乘上粒子數量看總量。
// -----------------------------------------------------------------------------
struct ParticleNaive {          // 每顆粒子各自帶一份共用設定
    float  x_, y_, vx_, vy_;    // 真正屬於自己的狀態
    string texture_;            // 每顆都一樣 → 重複儲存
    double gravity_;            // 每顆都一樣 → 重複儲存
};

struct ParticleShared {         // 共用設定移到類別層級
    float  x_, y_, vx_, vy_;    // 只留真正屬於自己的狀態
    inline static string texture_ = "spark.png";
    inline static double gravity_ = 9.81;
};

// 用來對照 sizeof 的輔助型別
struct Empty {};                        // 空類別
struct WithVirtual { int a_; virtual ~WithVirtual() = default; };

int main() {
    cout << "=== sizeof 與靜態成員 ===" << endl;
    cout << "  WithStatic 大小：" << sizeof(WithStatic) << " bytes" << endl;
    cout << "  WithoutStatic 大小：" << sizeof(WithoutStatic) << " bytes" << endl;
    cout << "  兩者相同！靜態成員不佔對象空間。" << endl;

    cout << "\n=== 對照組：其他 sizeof 行為（皆為本機實測，實作定義）===" << endl;
    cout << "  sizeof(Empty)       = " << sizeof(Empty)
         << " bytes（空類別仍 >= 1，保證不同物件不同位址）" << endl;
    cout << "  sizeof(WithVirtual) = " << sizeof(WithVirtual)
         << " bytes（int 4 + vptr，並含對齊 padding）" << endl;
    cout << "  sizeof(std::string) = " << sizeof(string)
         << " bytes（只含控制結構，不含堆積上的字元資料）" << endl;

    cout << "\n=== 日常實務：粒子系統的記憶體差距 ===" << endl;
    const size_t kParticles = 1'000'000;

    cout << "  sizeof(ParticleNaive)  = " << sizeof(ParticleNaive)  << " bytes" << endl;
    cout << "  sizeof(ParticleShared) = " << sizeof(ParticleShared) << " bytes" << endl;

    const size_t naiveTotal  = sizeof(ParticleNaive)  * kParticles;
    const size_t sharedTotal = sizeof(ParticleShared) * kParticles;

    cout << "  一百萬顆粒子：" << endl;
    cout << "    各自儲存 = " << naiveTotal  / (1024 * 1024) << " MB" << endl;
    cout << "    共用設定 = " << sharedTotal / (1024 * 1024) << " MB" << endl;
    cout << "    省下     = " << (naiveTotal - sharedTotal) / (1024 * 1024)
         << " MB（共用欄位不論幾顆粒子都只存一份）" << endl;
    cout << "  註：ParticleNaive 的 string 還會各自在堆積上再配置字元資料，" << endl;
    cout << "      實際差距比上面的 sizeof 差距更大。" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第 24 課：類別內的靜態成員變數6.cpp -o static_member6

// === 預期輸出 ===
// === sizeof 與靜態成員 ===
//   WithStatic 大小：8 bytes
//   WithoutStatic 大小：8 bytes
//   兩者相同！靜態成員不佔對象空間。
//
// === 對照組：其他 sizeof 行為（皆為本機實測，實作定義）===
//   sizeof(Empty)       = 1 bytes（空類別仍 >= 1，保證不同物件不同位址）
//   sizeof(WithVirtual) = 16 bytes（int 4 + vptr，並含對齊 padding）
//   sizeof(std::string) = 32 bytes（只含控制結構，不含堆積上的字元資料）
//
// === 日常實務：粒子系統的記憶體差距 ===
//   sizeof(ParticleNaive)  = 56 bytes
//   sizeof(ParticleShared) = 16 bytes
//   一百萬顆粒子：
//     各自儲存 = 53 MB
//     共用設定 = 15 MB
//     省下     = 38 MB（共用欄位不論幾顆粒子都只存一份）
//   註：ParticleNaive 的 string 還會各自在堆積上再配置字元資料，
//       實際差距比上面的 sizeof 差距更大。
