// =============================================================================
//  第 19 課：動態對象的創建與銷毀 4  —  new/delete 與 new[]/delete[] 必須配對
// =============================================================================
//
// 【主題資訊 Information】
//   規則：配置與釋放必須使用「對應的一組」運算子。
//         new T      ↔  delete p
//         new T[n]   ↔  delete[] p
//         malloc     ↔  free
//   違反的後果：未定義行為（undefined behavior），不是「可能有點問題」。
//   標準版本：C++98 起即如此規定，至今未變。
//   標頭檔：本檔用 <iostream>、<memory>（unique_ptr）。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼不能混用？兩個獨立的原因】
//   (a) 解構次數不對
//       delete 只呼叫「一個」解構函式。對 new T[3] 來的指標用 delete，
//       只有第 0 個元素被解構，第 1、2 個元素持有的資源全部洩漏。
//   (b) 交還給配置器的位址可能不對（更嚴重）
//       如 3.cpp 所述，對有非平凡解構函式的型別，new[] 會在使用者資料
//       前面存一個「元素個數」的標頭：
//           [ n ][ elem0 ][ elem1 ][ elem2 ]
//                 ↑ new[] 回傳這裡
//       delete[] 知道要往前偏移才是真正的區塊起點；delete 不知道，
//       它會把「使用者資料位址」直接交給配置器 → 配置器的中繼資料錯亂
//       → 堆積損毀，可能立刻崩潰，也可能在很久之後的某次配置才爆炸。
//   ★ (b) 說明了為什麼這種 bug 特別難查：崩潰點往往離出錯點很遠。
//
// 【2. 為什麼「用 delete[] 釋放 new 來的指標」同樣是未定義行為？】
//   反方向也一樣糟：delete[] 會往前偏移去讀「元素個數」，
//   但那塊記憶體根本不存在這個欄位，讀到的是配置器的中繼資料或其他垃圾，
//   然後它會依那個荒謬的數字去呼叫成千上萬次解構函式。
//
// 【3. 為什麼對 int 有時「看起來沒事」？】
//   int 沒有解構函式，實作通常不需要存元素個數，
//   於是 new int[5] 與 new int 回傳的指標在配置器眼中形式相同，
//   混用可能碰巧不崩潰。
//   ★ 但這仍然是未定義行為：標準沒有保證，換一個配置器
//     （例如 debug 版 CRT、jemalloc、開了 sanitizer）就可能立刻報錯。
//     「測起來沒事」永遠不能作為正確性的依據。
//
// 【4. 根本解法：讓型別系統替你記住】
//   人一定會忘記配對，但編譯器不會。現代 C++ 的做法是：
//       std::unique_ptr<T>     解構時呼叫 delete
//       std::unique_ptr<T[]>   解構時呼叫 delete[]
//   兩者是「不同的型別」，特化選擇了正確的 deleter。
//   你在寫下型別的那一刻就決定了正確的釋放方式，之後不可能寫錯。
//   ★ 更進一步：動態陣列直接用 std::vector，連 unique_ptr<T[]> 都不必。
//   本檔會實際示範 unique_ptr<T> 與 unique_ptr<T[]> 各自呼叫了正確的版本。
//
// 【概念補充 Concept Deep Dive】
//   ● operator delete 與 operator delete[] 是兩個不同的函式，
//     可以分別多載。編譯器依你寫的是 delete 還是 delete[] 決定呼叫哪一個，
//     完全在編譯期決定，沒有任何執行期判斷。
//   ● 型別 T* 上沒有「我是不是陣列」的資訊。這就是為什麼責任落在程式設計師
//     身上——指標本身不記得它的來歷。
//   ● AddressSanitizer 能抓到這類錯誤（回報 alloc-dealloc-mismatch），
//     Valgrind 也會回報 “Mismatched free() / delete / delete[]”。
//     所以本檔雖然不執行錯誤配對，但實務上請務必用這些工具跑測試。
//   ● 多型陣列另有陷阱：透過 Base* 對 new Derived[n] 用 delete[]
//     是未定義行為（指標算術用錯了元素大小），即使解構函式是虛擬的也一樣。
//     這是「不要用多型陣列」的主要理由。
//
// 【注意事項 Pay Attention】
//   1. new↔delete、new[]↔delete[]、malloc↔free，三組不可交叉。
//   2. 混用是未定義行為，不可以描述成「只會洩漏」或「一定崩潰」——
//      它可能安靜地損毀堆積，在很久之後才爆炸。
//   3. 對 int 等無解構函式的型別，混用「可能」不崩潰，但仍是未定義行為。
//   4. 本檔不實際執行錯誤配對，因為未定義行為沒有可驗證的預期輸出。
//   5. 用 std::vector / std::unique_ptr<T[]>，讓型別系統保證配對正確。
//   6. 避免多型陣列（Base* 指向 Derived[]），那是另一個未定義行為來源。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】new/delete 的配對
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 對 new T[3] 用 delete（沒有 []）會發生什麼？
//     答：未定義行為。兩個獨立的問題：(1) 只有第一個元素被解構，
//         其餘元素持有的資源洩漏；(2) 更嚴重的是，對有非平凡解構函式的
//         型別，new[] 會在使用者資料前面存元素個數，delete 不會往前偏移，
//         等於把錯誤的位址交還給配置器，導致堆積中繼資料損毀。
//     追問：那反過來，對 new T 用 delete[] 呢？
//         → 同樣是未定義行為，而且更兇險。delete[] 會往前讀一個
//           根本不存在的「元素個數」欄位，拿到垃圾值，
//           然後依那個數字呼叫大量解構函式。
//
// 🔥 Q2. 怎麼從根本上避免配對錯誤？
//     答：不要自己配對——讓型別系統做。std::unique_ptr<T> 的解構器呼叫
//         delete，std::unique_ptr<T[]> 的特化版呼叫 delete[]，
//         兩者是不同型別，寫下型別的那一刻就決定了正確的釋放方式。
//         動態陣列則直接用 std::vector，連 unique_ptr<T[]> 都不需要。
//     追問：這樣有額外成本嗎？
//         → 沒有。unique_ptr 大小與裸指標相同，其解構會被完全內聯，
//           是零成本抽象。
//
// ⚠️ 陷阱. 我對 new int[100] 用了 delete，程式跑得好好的，那是不是代表
//          「對基本型別可以混用」？
//     答：不是。int 沒有解構函式，實作通常不需要存元素個數標頭，
//         所以碰巧沒有位址偏移問題。但標準明確規定這是未定義行為，
//         換一個配置器、開啟 sanitizer、或換編譯器版本就可能立刻報錯。
//     為什麼會錯：把「這個實作剛好容忍」當成「語言允許」。
//         未定義行為的意思是「標準對此不作任何要求」，
//         包含「碰巧正常運作」也完全合法——那不是保證，只是運氣。
//         更實際的風險是：今天型別是 int，明天有人把它改成
//         有解構函式的類別，這行 delete 就從「碰巧沒事」變成「必定損毀」，
//         而且沒有任何編譯警告會提醒你。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <memory>
using namespace std;

class Item {
private:
    int id;
    static int nextId;
public:
    Item() : id(nextId++) {
        cout << "  [+] Item #" << id << endl;
        ++liveCount;
    }
    ~Item() {
        cout << "  [-] Item #" << id << endl;
        --liveCount;
    }
    static int liveCount;      // 目前還活著的 Item 數量
};

int Item::nextId = 1;
int Item::liveCount = 0;

// -----------------------------------------------------------------------------
// 【示範】用 unique_ptr 讓「型別」決定該用 delete 還是 delete[]
//   Tracer 在解構時印出訊息，我們可以直接觀察到：
//     unique_ptr<Tracer>    → 解構一次
//     unique_ptr<Tracer[]>  → 解構全部元素（反序）
//   兩者是不同的型別特化，各自綁定了正確的 deleter，
//   程式設計師沒有任何機會寫錯。
// -----------------------------------------------------------------------------
struct Tracer {
    int tag;
    static int seq;
    Tracer() : tag(seq++) { cout << "    [+] Tracer #" << tag << endl; }
    ~Tracer() { cout << "    [-] Tracer #" << tag << endl; }
};
int Tracer::seq = 1;

// -----------------------------------------------------------------------------
// 【日常實務範例】封裝一段「執行期大小」的像素緩衝區
//   情境：影像處理，緩衝區大小 = 寬 × 高，執行期才知道。
//   ● 舊寫法：成員存裸指標，建構 new[]、解構 delete[]，
//     還必須手動處理複製（否則淺複製會造成 double delete）。
//   ● 新寫法：成員直接用 unique_ptr<unsigned char[]>，
//     配對由型別保證，複製也因為 unique_ptr 不可複製而自動被禁止，
//     編譯器會在你寫錯的當下就報錯，而不是等到執行期崩潰。
// -----------------------------------------------------------------------------
class PixelBufferModern {
    unique_ptr<unsigned char[]> pixels;   // ★ [] 特化：解構時呼叫 delete[]
    size_t w, h;
public:
    PixelBufferModern(size_t width, size_t height)
        : pixels(make_unique<unsigned char[]>(width * height)),  // 值初始化為 0
          w(width), h(height) {}

    void set(size_t x, size_t y, unsigned char v) {
        if (x < w && y < h) pixels[y * w + x] = v;
    }
    int get(size_t x, size_t y) const {
        if (x < w && y < h) return static_cast<int>(pixels[y * w + x]);
        return -1;
    }
    size_t bytes() const { return w * h; }
    // 不需要寫解構函式、不需要禁止複製——unique_ptr 已經處理好了
};

// 註：本檔不加 LeetCode 範例。
//     「配置與釋放的配對規則」是記憶體管理的正確性議題，
//     其錯誤形式屬未定義行為、無法寫出可驗證的預期輸出；
//     LeetCode 也不考這個，硬掛一題會失焦，故從缺。
//     本課需要動態配置的解題範例已放在 1.cpp（707）與 3.cpp（705）。

int main() {
    cout << "=== 正確配對 ===" << endl;
    
    // 正確：new 配 delete, 會調用建構函數和解構函數，內部資源被正確管理
    Item* single = new Item;
    delete single;
    
    // 正確：new[] 配 delete[], 會調用陣列中每個元素的建構函數和解構函數，內部資源被正確管理
    Item* array = new Item[3];
    delete[] array;

    cout << "\n  存活中的 Item 數量 = " << Item::liveCount
         << "（0 代表每個建構都有對應的解構）" << endl;
    
    cout << "\n=== 錯誤配對（千萬不要這樣做！）===" << endl;
    cout << "  // Item* p = new Item[3];" << endl;
    cout << "  // delete p;      ← 錯誤！應該用 delete[]" << endl;
    cout << "  // 後果：只解構第一個元素，其餘洩漏" << endl;
    cout << "  //        或者直接崩潰（未定義行為）" << endl;
    
    cout << "\n  // Item* q = new Item;" << endl;
    cout << "  // delete[] q;    ← 錯誤！應該用 delete" << endl;
    cout << "  // 後果：未定義行為，可能崩潰" << endl;

    cout << "\n  註：以上錯誤配對刻意不實際執行——未定義行為沒有「預期輸出」，" << endl;
    cout << "      而且它可能安靜地損毀堆積，在很久之後才爆炸。" << endl;
    cout << "      實務上請用 AddressSanitizer 或 Valgrind 偵測這類錯誤。" << endl;

    // ====== 讓型別系統保證配對 ======
    cout << "\n=== 根本解法：讓型別決定 delete 還是 delete[] ===" << endl;
    {
        cout << "  unique_ptr<Tracer>（單一物件）：" << endl;
        {
            unique_ptr<Tracer> one = make_unique<Tracer>();
            (void)one;
        }   // 自動呼叫 delete

        cout << "  unique_ptr<Tracer[]>（陣列，注意反序解構）：" << endl;
        {
            unique_ptr<Tracer[]> many = make_unique<Tracer[]>(3);
            (void)many;
        }   // 自動呼叫 delete[]，解構全部 3 個元素

        cout << "  → 兩者型別不同，各自綁定正確的 deleter，不可能寫錯" << endl;
    }

    // ====== 實務範例 ======
    cout << "\n=== 日常實務：像素緩衝區（unique_ptr<T[]>）===" << endl;
    {
        PixelBufferModern img(4, 3);
        cout << "  緩衝區大小 = " << img.bytes() << " bytes（4x3）" << endl;
        cout << "  初始值 get(0,0) = " << img.get(0, 0)
             << "（make_unique<T[]> 會值初始化為 0）" << endl;
        img.set(2, 1, 200);
        cout << "  set(2,1,200) 之後 get(2,1) = " << img.get(2, 1) << endl;
        cout << "  越界 get(9,9) = " << img.get(9, 9) << "（回傳 -1）" << endl;
        cout << "  離開作用域：unique_ptr<unsigned char[]> 自動 delete[]" << endl;
        cout << "  → 不需要自訂解構函式，也不可能忘記或用錯釋放方式" << endl;
    }
    
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 19 課：動態對象的創建與銷毀（new  delete）4.cpp" -o newdel4

// 【輸出說明】本檔刻意「不執行」任何錯誤配對——那是未定義行為，
//   沒有可驗證的預期輸出，且可能安靜地損毀堆積。
//   下列輸出全部來自正確配對的路徑，完全可重現。
//   注意 Item 與 Tracer 陣列都是「順序建構、反序解構」。

// === 預期輸出 ===
// === 正確配對 ===
//   [+] Item #1
//   [-] Item #1
//   [+] Item #2
//   [+] Item #3
//   [+] Item #4
//   [-] Item #4
//   [-] Item #3
//   [-] Item #2
//
//   存活中的 Item 數量 = 0（0 代表每個建構都有對應的解構）
//
// === 錯誤配對（千萬不要這樣做！）===
//   // Item* p = new Item[3];
//   // delete p;      ← 錯誤！應該用 delete[]
//   // 後果：只解構第一個元素，其餘洩漏
//   //        或者直接崩潰（未定義行為）
//
//   // Item* q = new Item;
//   // delete[] q;    ← 錯誤！應該用 delete
//   // 後果：未定義行為，可能崩潰
//
//   註：以上錯誤配對刻意不實際執行——未定義行為沒有「預期輸出」，
//       而且它可能安靜地損毀堆積，在很久之後才爆炸。
//       實務上請用 AddressSanitizer 或 Valgrind 偵測這類錯誤。
//
// === 根本解法：讓型別決定 delete 還是 delete[] ===
//   unique_ptr<Tracer>（單一物件）：
//     [+] Tracer #1
//     [-] Tracer #1
//   unique_ptr<Tracer[]>（陣列，注意反序解構）：
//     [+] Tracer #2
//     [+] Tracer #3
//     [+] Tracer #4
//     [-] Tracer #4
//     [-] Tracer #3
//     [-] Tracer #2
//   → 兩者型別不同，各自綁定正確的 deleter，不可能寫錯
//
// === 日常實務：像素緩衝區（unique_ptr<T[]>）===
//   緩衝區大小 = 12 bytes（4x3）
//   初始值 get(0,0) = 0（make_unique<T[]> 會值初始化為 0）
//   set(2,1,200) 之後 get(2,1) = 200
//   越界 get(9,9) = -1（回傳 -1）
//   離開作用域：unique_ptr<unsigned char[]> 自動 delete[]
//   → 不需要自訂解構函式，也不可能忘記或用錯釋放方式
