// =============================================================================
//  第 26 課：this 指標5.cpp  —  用 this 做自我賦值檢查（self-assignment guard）
// =============================================================================
//
// 【主題資訊 Information】
//   語法：  if (this == &other) return *this;
//   型別：  在非 const 成員函數中 this 的型別是 Buffer* const
//           在 const  成員函數中 this 的型別是 const Buffer* const
//   標準：  this 自 C++98 即存在；本檔語法在 C++11 以上皆可編譯
//   標頭檔：不需要（this 是語言關鍵字，不是函式庫設施）
//   複雜度：this == &other 是單一指標比較，O(1)，且編譯器通常可內聯掉
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼「拷貝賦值運算子」非得處理自我賦值不可】
//   典型的深拷貝賦值長這樣：
//       delete[] data_;                 // ① 先釋放自己舊的資源
//       data_ = new int[other.size_];   // ② 再配置新空間
//       copy(other.data_ -> data_);     // ③ 從來源複製
//   如果 other 就是 *this，① 已經把 other.data_ 指向的那塊記憶體 delete 掉了，
//   ③ 讀 other.data_ 就是在讀「已釋放的記憶體」——這是 use-after-free，
//   屬於未定義行為（undefined behavior），不保證任何固定結果：
//   可能印出舊值、可能印出垃圾、可能當場崩潰，也可能在 release build 下
//   看起來完全正常而在半年後的某次上線炸掉。
//
//   注意這裡真正的元凶不是「a = a 這行程式碼很蠢」，而是**別名（aliasing）**：
//       arr[i] = arr[j];        // i == j 時就是自我賦值
//       *p = *q;                // p、q 指到同一物件時就是自我賦值
//       v[0] = v[0];            // 排序、去重、交換的邊界情況
//   沒有人會故意寫 buf = buf，但泛型演算法會替你寫出來。
//
// 【2. this 在這裡扮演的角色】
//   this 是「當前物件的位址」。成員函數並沒有第二個管道能知道自己被誰呼叫，
//   唯一能拿到自身身分（identity）的東西就是 this。
//   而 &other 是來源物件的位址。兩個位址相等 ⇔ 同一個物件 ⇔ 自我賦值。
//
//   為什麼比較「位址」而不是比較「內容」？
//     * 位址相等才是「同一物件」；內容相等只是「值相同」。
//       兩個內容相同但獨立的 Buffer 必須真的做深拷貝，不能跳過。
//     * 位址比較 O(1)；內容比較 O(n)，而且需要 operator==。
//
// 【3. 三種寫法的取捨】
//   (a) 本檔的「早退檢查」（identity test）
//         if (this == &other) return *this;
//       優點：直觀、成本極低。
//       缺點：不是 exception-safe——若 new 拋出 std::bad_alloc，
//             data_ 已被 delete 掉，物件停在 dangling 狀態。
//
//   (b) 「先配置、後釋放」（copy-then-release）
//         int* tmp = new int[other.size_];   // 先配置，失敗就直接拋出，物件未受損
//         copy...;
//         delete[] data_;                    // 到這裡才動舊資源
//         data_ = tmp; size_ = other.size_;
//       優點：strong exception guarantee；順帶天然對自我賦值免疫。
//
//   (c) copy-and-swap（本檔尾端【日常實務範例】示範）
//         Buffer& operator=(Buffer other) { swap(*this, other); return *this; }
//       優點：一份程式碼同時處理 copy/move、天然 exception-safe、天然自我賦值安全。
//       缺點：即使是自我賦值也會實際做一次複製，多花一點成本。
//
//   結論：identity test 不是「最佳解」，而是「最容易解釋的解」。工程上
//   (b)/(c) 更受推薦；但面試官仍最愛問 (a)，因為它直接考 this 的語意。
//
// 【概念補充 Concept Deep Dive】
//   * this 不是存在物件裡的欄位。它是呼叫慣例的一部分——在 x86-64 System V ABI
//     上，非靜態成員函數的第一個隱藏參數就是物件位址（傳在 %rdi）。
//     所以 sizeof(Buffer) 只有 data_ + size_ 的大小，不含 this。
//     本機 g++ 15.2 x86-64 實測：sizeof(int*)=8、sizeof(int)=4，
//     加上對齊補齊 4 bytes 後 sizeof(Buffer)=16（實作定義，見輸出）。
//   * this 的型別中「const 在星號右邊」：Buffer* const。所以不能寫 this = ...，
//     但可以寫 this->value = ...。const 成員函數則升級為 const Buffer* const，
//     兩層都是 const。
//   * 成員初始化順序由**宣告順序**決定，與初始化列表的書寫順序無關。
//     本檔原本寫成 : size_(size), data_(new int[size])，但宣告順序是
//     data_ 在前、size_ 在後，因此 g++ 會發出 -Wreorder 警告（-Wall 內含）。
//     已改為與宣告順序一致。這在「用另一個成員初始化自己」時會是真 bug：
//         Buffer(int n) : size_(n), data_(new int[size_]) // size_ 此時尚未初始化！
//
// 【注意事項 Pay Attention】
//   1. 自我賦值下的 use-after-free 是 UB，不可描述成「一定崩潰」或「一定印出 0」。
//      實務上它最常見的表現是「測試環境永遠正常、正式環境偶爾壞掉」。
//   2. 早退檢查只擋得住「同一物件」，擋不住「兩個物件共用同一塊 data_」
//      （淺拷貝造成的別名）。根治之道是一開始就不要淺拷貝。
//   3. 拷貝賦值運算子回傳 Buffer&（不是 Buffer、也不是 void），
//      才能支援 a = b = c 的鏈式賦值並避免多餘複製。
//   4. 有了自訂解構函數 + 自訂拷貝賦值，依「三法則（Rule of Three）」
//      就也必須提供拷貝建構函數，否則 Buffer b = a; 會走編譯器產生的淺拷貝 → double free。
//      本檔為聚焦 this 的語意而未補上，第 30 課會完整處理。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】this 指標與自我賦值
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼拷貝賦值運算子要寫 if (this == &other) return *this; ?
//     答：因為典型實作會「先 delete 舊資源、再從 other 複製」。當 other 就是
//         *this 時，delete 已經把來源殺掉，接下來的複製是 use-after-free（UB）。
//         this 是唯一能取得「自己是誰」的管道，比較 this 與 &other 即可判定同一物件。
//     追問：那 copy-and-swap 為什麼可以不寫這行？
//         → 因為它是傳值收參數，來源早在進函式前就複製好了，
//           swap 只交換指標，從頭到尾沒有「先釋放後讀取」的窗口。
//
// 🔥 Q2. this 的型別到底是什麼？可以對它賦值嗎？
//     答：非 const 成員函數中是 Buffer* const；const 成員函數中是 const Buffer* const。
//         const 位於 * 右側，代表「指標本身」是常數，所以 this = nullptr 編譯錯誤；
//         但 this->size_ = 5 在非 const 成員函數中合法。
//     追問：static 成員函數裡的 this 是什麼？→ 不存在。static 成員函數沒有物件，
//           所以編譯期就沒有這個隱藏參數，寫 this 直接編譯錯誤。
//
// ⚠️ 陷阱1. 「我改成比較內容 if (*this == other) return *this; 不是更安全嗎？」
//     答：不對。這會把「兩個值剛好相等的獨立物件」也跳過，導致該做的深拷貝沒做。
//         雖然此時兩者值相同看似無害，但如果後續其中一個被修改，
//         另一個不會跟著變——你要的是 identity（同一物件），不是 equality（值相等）。
//     為什麼會錯：把「相等」和「同一個」混為一談。這在有 shared state 的類別上
//         會變成非常難查的 bug。
//
// ⚠️ 陷阱2. 「早退檢查寫了，所以我的 operator= 就安全了。」
//     答：仍然不安全。若 new int[size_] 拋出 bad_alloc，此時 delete[] data_ 已執行，
//         data_ 是懸空指標，解構時會二次釋放。早退檢查解決的是「別名」，
//         不是「例外安全」。
//     為什麼會錯：以為 operator= 只有一種風險。實際上有兩個獨立的正確性維度：
//         別名安全（aliasing safety）與例外安全（exception safety）。
//
// ⚠️ 陷阱3. 「初始化列表照我寫的順序執行。」
//     答：不是。成員初始化順序永遠是**類別中的宣告順序**。
//         本檔原始碼寫 : size_(size), data_(...)，但宣告是 data_ 先，
//         所以 g++ -Wall 會給 -Wreorder 警告。
//     為什麼會錯：直覺認為程式碼由左到右執行。若寫成
//         : size_(n), data_(new int[size_]) 就會用到尚未初始化的 size_（UB）。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <utility>
#include <string>
using namespace std;

class Buffer {
private:
    int* data_;
    int size_;

public:
    // 初始化列表順序已對齊「宣告順序」（data_ 在前、size_ 在後），消除 -Wreorder
    Buffer(int size) : data_(new int[size]), size_(size) {
        for (int i = 0; i < size; i++) data_[i] = i;
        cout << "  [建構] 大小:" << size_ << endl;
    }

    ~Buffer() {
        delete[] data_;
        cout << "  [解構]" << endl;
    }

    // 拷貝賦值運算子——必須檢查自我賦值
    Buffer& operator=(const Buffer& other) {
        cout << "  [賦值] ";

        // 自我賦值檢查：this == &other 嗎？
        // this 是「我自己的位址」，&other 是「來源的位址」，
        // 位址相同 ⇔ 同一物件 ⇔ 下面的 delete 會把來源一起殺掉。
        if (this == &other) {
            cout << "偵測到自我賦值，跳過" << endl;
            return *this;
        }

        // 正常賦值邏輯
        delete[] data_;
        size_ = other.size_;
        data_ = new int[size_];
        for (int i = 0; i < size_; i++) data_[i] = other.data_[i];
        cout << "完成" << endl;

        return *this;
    }

    void print() const {
        cout << "  [";
        for (int i = 0; i < size_; i++) {
            if (i > 0) cout << ", ";
            cout << data_[i];
        }
        cout << "]" << endl;
    }
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】—— 從缺，並說明原因
//
// 本檔主題是「this 指標的身分語意 + 拷貝賦值的別名安全」，屬於 C++ 物件模型
// 與資源管理範疇。LeetCode 題庫（含指定清單中的 146 LRU Cache、155 Min Stack、
// 705 Design HashSet、707 Design Linked List 等 design 類題）考的是資料結構
// 設計與演算法複雜度，判題只看回傳值，不會觸發自我賦值，也不檢查 Rule of Three。
// 硬把 LRU Cache 套進來，只會變成「一個剛好有 operator= 的 LRU」，
// 對理解 this 毫無幫助。依規格「寧缺勿濫」，此處明確從缺。
// （若要練 design 類題，建議在第 30 課「三法則」再一併示範資源類別的完整寫法。）
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 【日常實務範例】連線池設定（PoolConfig）— 用 copy-and-swap 免疫自我賦值
//
// 情境：服務啟動時從設定檔讀出 DB 連線池參數；熱更新（hot reload）時會把新的
//       設定物件賦值回舊的。熱更新程式碼常寫成 cfg = registry.lookup(name)，
//       而 lookup 有可能就回傳 cfg 自己（例如快取命中同一份）——這就是
//       正式環境裡真正會發生的自我賦值。
//
// 作法：operator= 以「傳值」收參數，複製由編譯器在進入函式前完成；
//       函式內只做 swap（交換指標，不會釋放來源）。
//       因此 (1) 自我賦值天然安全 (2) 例外安全 (3) copy/move 共用一份實作。
// -----------------------------------------------------------------------------
class PoolConfig {
private:
    string  name_;
    char*   dsn_;       // 故意用裸指標，才看得出資源管理的差別
    int     maxConn_;

    static char* dup(const char* s) {
        char* p = new char[std::char_traits<char>::length(s) + 1];
        std::char_traits<char>::copy(p, s, std::char_traits<char>::length(s) + 1);
        return p;
    }

public:
    PoolConfig(string name, const char* dsn, int maxConn)
        : name_(std::move(name)), dsn_(dup(dsn)), maxConn_(maxConn) {}

    PoolConfig(const PoolConfig& o)
        : name_(o.name_), dsn_(dup(o.dsn_)), maxConn_(o.maxConn_) {}

    ~PoolConfig() { delete[] dsn_; }

    friend void swap(PoolConfig& a, PoolConfig& b) noexcept {
        using std::swap;
        swap(a.name_,    b.name_);
        swap(a.dsn_,     b.dsn_);
        swap(a.maxConn_, b.maxConn_);
    }

    // 注意：參數是「傳值」，不是 const&。
    // 呼叫端的複製動作在此函式外完成，所以函式內永遠不會出現
    // 「已經 delete 掉來源、卻還要讀來源」的窗口。
    PoolConfig& operator=(PoolConfig other) noexcept {
        swap(*this, other);
        return *this;   // other 帶著「舊的我」離開作用域並自動釋放
    }

    void dump(const char* tag) const {
        cout << "  " << tag << " name=" << name_
             << " dsn=" << dsn_ << " maxConn=" << maxConn_ << endl;
    }
};

int main() {
    cout << "=== 場景四：自我賦值檢查 ===" << endl;

    Buffer buf(5);
    buf.print();

    // 自我賦值：buf = buf
    buf = buf;   // this == &other，被安全攔截
    buf.print();

    cout << "\n=== this 的身分語意 ===" << endl;
    cout << "  sizeof(Buffer) = " << sizeof(Buffer)
         << " bytes（實作定義：8 bytes 指標 + 4 bytes int + 4 bytes 對齊補齊）" << endl;
    cout << "  this 不佔物件空間，它是成員函數的隱藏參數" << endl;

    cout << "\n=== 日常實務：連線池設定熱更新（copy-and-swap）===" << endl;
    PoolConfig cfg("primary", "host=db1;port=5432", 32);
    cfg.dump("初始   :");

    PoolConfig hotReload("primary", "host=db2;port=5432", 64);
    cfg = hotReload;              // 一般賦值
    cfg.dump("熱更新後:");

    cfg = cfg;                    // 自我賦值：copy-and-swap 不需要任何檢查即安全
    cfg.dump("自我賦值後:");

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 26 課：this 指標5.cpp" -o this5

// 注意事項（輸出相關）：
//   * sizeof(Buffer) = 16 是本機 g++ 15.2 / x86-64 的實測值，屬**實作定義**：
//     8 (int*) + 4 (int) + 4 (為對齊到 8 而補齊)。在 32-bit 平台上會是 8。
//   * 本檔完全不列印任何指標位址。位址每次執行都不同（ASLR），
//     不適合寫進「預期輸出」。要驗證身分請比較指標是否相等（布林值），
//     而非印出位址本身。
//   * 「[解構]」共出現 1 次（buf）；PoolConfig 的解構不印訊息，故不出現在輸出中。
//   * 自我賦值那行走的是早退分支，因此不會出現第二次「完成」。

// === 預期輸出 ===
// === 場景四：自我賦值檢查 ===
//   [建構] 大小:5
//   [0, 1, 2, 3, 4]
//   [賦值] 偵測到自我賦值，跳過
//   [0, 1, 2, 3, 4]
//
// === this 的身分語意 ===
//   sizeof(Buffer) = 16 bytes（實作定義：8 bytes 指標 + 4 bytes int + 4 bytes 對齊補齊）
//   this 不佔物件空間，它是成員函數的隱藏參數
//
// === 日常實務：連線池設定熱更新（copy-and-swap）===
//   初始   : name=primary dsn=host=db1;port=5432 maxConn=32
//   熱更新後: name=primary dsn=host=db2;port=5432 maxConn=64
//   自我賦值後: name=primary dsn=host=db2;port=5432 maxConn=64
//   [解構]
