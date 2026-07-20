// =============================================================================
//  第 12 課：struct 與 class 的差異 2  —  唯一的語法差異：預設存取修飾符
// =============================================================================
//
// 【主題資訊 Information】
//   語法：  struct X { /* 成員預設 public  */ };
//           class  X { /* 成員預設 private */ };
//   標準版本：C++98 起即如此，至今未變（C++11/14/17/20 都一樣）
//   標頭檔：無（語言核心特性）
//   影響範圍：① 成員的預設存取權　② 繼承的預設存取權（class 預設 private 繼承）
//
// 【詳細解釋 Explanation】
//
// 【1. 兩個關鍵字在語意上其實是同一個東西】
//   C++ 標準把 struct 與 class 都歸類為 "class type"（類別型別）。編譯器在解析
//   完 class-head 之後，兩者走的是完全相同的一套規則：都能有成員函式、建構函式、
//   解構函式、virtual 函式、繼承、模板、運算子多載。
//   唯一的差別發生在「還沒遇到任何存取修飾符之前」的那段區域屬於誰：
//       struct → public 區
//       class  → private 區
//   一旦你自己寫了 public: / private: / protected:，之後的行為兩者完全一致。
//
// 【2. 為什麼會有這個差異？—— C 相容性的歷史包袱】
//   C 語言只有 struct，而且它的成員一律可以直接存取。C++ 要能直接沿用 C 的
//   標頭檔（例如 struct tm、struct sockaddr），就必須讓 struct 的成員維持 public，
//   否則所有既有 C 程式碼在 C++ 編譯器下都會爆掉。
//   而 class 是 Bjarne Stroustrup 新造的關鍵字，沒有相容包袱，於是選擇了
//   「預設封裝」這個對 OOP 更安全的預設值 —— 要開放必須明講。
//
// 【3. private 擋的是「編譯期的名稱存取」，不是記憶體】
//   本例中 ClassExample::x 是 private，外部寫 c.x 會編譯錯誤。但要注意：
//   private 是編譯期的存取控制（access control），不是執行期的記憶體保護。
//   物件的記憶體佈局跟存取權無關；private 成員一樣佔空間、一樣在物件裡面。
//   （用指標硬轉型去讀它在語法上擋不住你，但那是 UB，不該做。）
//
// 【4. 實務慣例：語意上的分工】
//   既然功能等價，業界就用它們來「傳達意圖」——這是慣例而非強制：
//     * struct → 純資料聚合（passive data），沒有需要維護的不變條件（invariant）
//     * class  → 有不變條件要保護、有行為、需要封裝
//   Google / LLVM / Core Guidelines（C.2、C.8）大致都是這個分法。
//
// 【概念補充 Concept Deep Dive】
//   * 存取控制不影響記憶體佈局，但「存取權區段的數量」在理論上可能影響順序：
//     標準保證『同一個 access section 內』的非靜態成員，位址依宣告順序遞增；
//     跨不同 section 之間的相對順序則是實作定義（implementation-defined）。
//     實務上主流編譯器（GCC/Clang/MSVC）仍照宣告順序排，但別寫程式去依賴它。
//   * 前向宣告可以混用：`struct X;` 之後寫 `class X { };` 在 GCC/Clang 完全合法
//     （兩者指的是同一個 class type），MSVC 早期會發出 C4099 警告。
//   * 樣板參數的 `template<class T>` 與 `template<typename T>` 也是完全等價，
//     這是另一組「同義但慣例不同」的關鍵字，別和本課的 struct/class 混為一談。
//
// 【注意事項 Pay Attention】
//   1. 「struct 不能有成員函式」是徹頭徹尾的錯誤觀念 —— 那是 C，不是 C++。
//   2. 繼承的預設存取權也不同，而且這個差異比成員存取更容易咬人：
//        struct D : Base {};   // 預設 public 繼承
//        class  D : Base {};   // 預設 private 繼承 → D* 無法隱式轉成 Base*
//      所以繼承時「一律明寫 public」是最安全的習慣。
//   3. 本檔刻意保留兩行被註解掉的錯誤示範（c.x / c.show()）。把註解拿掉會編譯
//      失敗，那正是本課要示範的重點，不是 bug。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】struct 與 class 的差異
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. C++ 中 struct 和 class 有什麼差別？
//     答：語言層面只有兩個：① 成員的預設存取權（struct=public、class=private）；
//         ② 繼承的預設存取權（struct=public、class=private）。
//         其餘一切能力完全相同 —— struct 一樣能有建構函式、virtual 函式、繼承、模板。
//     追問：那實務上怎麼選？→ 純資料聚合、無不變條件用 struct；需要封裝與維護
//           不變條件用 class。這是團隊慣例，不是編譯器強制。
//
// 🔥 Q2. private 成員能保證別人讀不到記憶體嗎？
//     答：不能。private 是編譯期的名稱存取控制，只影響「這段程式碼能不能通過編譯」，
//         不改變物件佈局，也不做任何執行期保護。private 成員照樣在物件裡佔空間。
//     追問：那 private 的價值是什麼？→ 維護不變條件與縮小改動面：編譯器幫你保證
//           沒有人能繞過你的 setter 直接破壞狀態，重構內部表示時不必擔心外部相依。
//
// ⚠️ 陷阱. 下面兩個宣告是等價的嗎？
//         struct Derived1 : Base { };
//         class  Derived2 : Base { };
//     答：不等價。Derived1 是 public 繼承、Derived2 是 private 繼承。
//         結果是 Derived2* 無法隱式轉換成 Base*，多型完全失效，而且錯誤訊息
//         （'Base' is an inaccessible base）通常出現在很遠的呼叫端，很難追。
//     為什麼會錯：多數人記住了「成員預設存取權不同」，就以為差異僅止於成員，
//         忽略了同一條規則也套用在 base-clause 上。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
using namespace std;

struct StructExample {
    int x = 10;    // 預設 public, 所以外部可以存取
    void show() { cout << "struct x = " << x << endl; }
};

class ClassExample {
    int x = 10;    // 預設 private, 所以外部無法存取
    void show() { cout << "class x = " << x << endl; }

public:
    // 補一個 public 的出口，證明「private 擋的是外部，不是成員自己」
    void reveal() { show(); }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】用 struct / class 表達不同的設計意圖
//   情境：伺服器啟動參數。
//     * ServerOptions 是純設定資料 —— 每個欄位都能獨立自由填寫，沒有任何
//       欄位間的不變條件要維護，因此用 struct，開放直接讀寫最方便。
//     * ConnectionPool 有不變條件要守（已使用連線數不可超過上限、不可為負），
//       因此用 class 把計數器藏起來，只透過 acquire()/release() 變更。
//   這正是業界「struct 裝資料、class 管狀態」慣例的實際樣貌。
// -----------------------------------------------------------------------------
struct ServerOptions {
    string host = "0.0.0.0";
    int    port = 8080;
    int    maxConnections = 100;
    bool   enableTls = false;
};

class ConnectionPool {
public:
    explicit ConnectionPool(int capacity) : m_capacity(capacity), m_inUse(0) {}

    // 不變條件：0 <= m_inUse <= m_capacity，由這兩個函式共同保證
    bool acquire() {
        if (m_inUse >= m_capacity) return false;   // 滿了就拒絕，而不是讓它超量
        ++m_inUse;
        return true;
    }
    void release() {
        if (m_inUse > 0) --m_inUse;                // 擋住「歸還比借出還多」
    }

    int inUse()     const { return m_inUse; }
    int available() const { return m_capacity - m_inUse; }

private:
    int m_capacity;   // 外部無法直接改 → 不變條件不會被繞過
    int m_inUse;
};

int main() {
    cout << "=== struct：預設 public ===" << endl;
    StructExample s;
    cout << s.x << endl;    // ✅ public，可以存取
    s.show();               // ✅ public

    cout << "\n=== class：預設 private ===" << endl;
    ClassExample c;
    // cout << c.x << endl; // ❌ 編譯錯誤！private
    // c.show();            // ❌ 編譯錯誤！private
    c.reveal();             // ✅ 透過 public 成員函式間接使用 private 成員
    cout << "外部無法寫 c.x，但 x 確實存在且佔記憶體" << endl;

    cout << "\n=== 實務：struct 裝設定 ===" << endl;
    ServerOptions opt;              // 每個欄位都有預設值
    opt.port = 443;                 // 純資料，直接改最直觀
    opt.enableTls = true;
    cout << "監聽 " << opt.host << ":" << opt.port
         << " (TLS=" << (opt.enableTls ? "on" : "off")
         << ", 上限 " << opt.maxConnections << ")" << endl;

    cout << "\n=== 實務：class 守不變條件 ===" << endl;
    ConnectionPool pool(3);
    for (int i = 1; i <= 4; ++i) {
        bool ok = pool.acquire();
        cout << "第 " << i << " 次 acquire: " << (ok ? "成功" : "失敗（已達上限）")
             << "，使用中 " << pool.inUse() << "，剩餘 " << pool.available() << endl;
    }
    pool.release();
    cout << "release 之後，使用中 " << pool.inUse()
         << "，剩餘 " << pool.available() << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 12 課：struct 與 class 的差異2.cpp" -o demo2

// === 預期輸出 ===
// === struct：預設 public ===
// 10
// struct x = 10
//
// === class：預設 private ===
// class x = 10
// 外部無法寫 c.x，但 x 確實存在且佔記憶體
//
// === 實務：struct 裝設定 ===
// 監聽 0.0.0.0:443 (TLS=on, 上限 100)
//
// === 實務：class 守不變條件 ===
// 第 1 次 acquire: 成功，使用中 1，剩餘 2
// 第 2 次 acquire: 成功，使用中 2，剩餘 1
// 第 3 次 acquire: 成功，使用中 3，剩餘 0
// 第 4 次 acquire: 失敗（已達上限），使用中 3，剩餘 0
// release 之後，使用中 2，剩餘 1
