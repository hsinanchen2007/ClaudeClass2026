// =============================================================================
//  第 16 課：初始化列表 10  —  用「會出聲的物件」量化兩種寫法的成本差
// =============================================================================
//
// 【主題資訊 Information】
//   對照：  ContainerA(const string& s) { obj = s; }        // 兩步：預設建構 + 賦值
//           ContainerB(const string& s) : obj(s) { }        // 一步：直接帶參建構
//   標準版本：C++98 起即有
//   複雜度：方式 A 多一次預設建構 + 一次賦值；方式 B 只有一次建構
//   標頭檔：<string>
//
// 【詳細解釋 Explanation】
//
// 【1. 本檔的做法：讓成本自己說話】
//   前面幾個檔案用文字說明「函數體賦值會多做一次預設建構」。
//   本檔改用實證：讓 HeavyObject 的每個特殊成員函數都印出自己被呼叫，
//   於是「多做了哪一步」直接出現在輸出裡，不必相信任何人的說法。
//   結果非常清楚：
//       方式 A：預設建構 → 賦值        （兩次操作）
//       方式 B：帶參建構                （一次操作）
//
// 【2. 為什麼方式 A 一定會先預設建構】
//   因為建構函數的函數體開始執行時，物件的所有成員都必須已經是有效狀態。
//   編譯器看到你沒在初始化列表指定 obj 怎麼建，就只好呼叫它的預設建構函數。
//   等到函數體裡的 obj = s; 執行時，obj 早就是一個建好的物件了，
//   那一行只能是**賦值**。
//   注意：如果 HeavyObject 沒有預設建構函數，方式 A 根本編譯不過
//   （這正是 4.cpp 的主題）。
//
// 【3. 成本差在哪裡（不只是「多一次函數呼叫」）】
//   對管理動態資源的型別，兩步的實際代價通常是：
//       預設建構：可能配置一塊初始緩衝區（或至少初始化內部狀態）
//       賦值：    可能需要釋放剛剛那塊、再依來源大小重新配置、然後複製
//   也就是說，多的不只是一次呼叫，還可能包含一次**多餘的配置與釋放**。
//   在每秒建立成千上萬個物件的服務裡，這是可量測的浪費。
//
// 【4. 但這仍然不是最重要的理由】
//   必須再強調一次：初始化列表的價值排序是
//       (1) 有些成員**只能**這樣初始化（const、參考、無預設建構函數、基底）
//       (2) 語意更正確：初始化就是初始化，不是「建好再改」
//       (3) 效能：省掉多餘的預設建構
//   效能排在最後。把初始化列表當成「效能技巧」是本末倒置——
//   它其實是表達「這個成員誕生時就該是這個值」的正確語法。
//
// 【5. 什麼時候「不得不」在函數體賦值】
//   ● 初始化需要複雜流程（多重 if、迴圈、try/catch）而又不適合包成函數
//   ● 成員的值必須依賴「建構函數執行過程中才知道的結果」
//   ● 兩階段初始化的既有 API（例如某些 C 函式庫的 handle 要先建再 init）
//   這些情況下，先讓成員預設建構、再在函數體設定，是合理的取捨。
//
// 【概念補充 Concept Deep Dive】
//
//   ● 為什麼編譯器不能自動幫我把方式 A 最佳化成方式 B
//     因為 HeavyObject 的預設建構函數與 operator= 都有**可觀察的副作用**
//     （本檔是印字，實務上常是記憶體配置或計數）。
//     最佳化必須保持可觀察行為不變，所以編譯器不能把它們刪掉。
//     本檔的輸出就是最直接的證據——即使開 -O2，那兩行還是會印出來。
//
//   ● 本檔的 operator= 是「從 string 賦值」而非「從 HeavyObject 賦值」
//     HeavyObject& operator=(const string& s);
//     這是一個自訂的轉換賦值運算子，讓 obj = s; 能直接吃字串。
//     實務上這種設計要小心：它讓 HeavyObject 與 string 之間多了一條
//     隱式路徑，可能造成非預期的重載決議結果。
//
//   ● 與移動語意的關係（C++11 起）
//     若參數是右值，初始化列表可以寫成 : obj(std::move(s))，
//     直接把資源搬過來，連複製都省了。方式 A 則無論如何都要先預設建構，
//     移動語意也救不回那一步。
//
//   ● 如何在真實專案中量測
//     不要靠猜測。可用的手段包括：
//       - 在特殊成員函數裡加計數器（本檔的簡化版）
//       - 用 perf / valgrind --tool=callgrind 看實際呼叫次數
//       - 覆寫 operator new 統計配置次數
//
// 【注意事項 Pay Attention】
//   1. 差異的大小取決於成員型別；對 int 幾乎沒差，對管理資源的型別才明顯。
//   2. 若成員沒有預設建構函數，方式 A 直接編譯失敗，不是效能問題而是無法編譯。
//   3. 別把初始化列表當成純效能技巧；它首先是「正確表達初始化」的語法。
//   4. 效能結論請以實測為準，不要用「應該比較快」來下判斷。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】兩種初始化方式的實際成本
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 在建構函數本體內賦值成員，比用初始化列表多做了什麼？
//     答：多了一次該成員的**預設建構**。因為進入函數體之前所有成員都必須
//         初始化完成，編譯器只好先呼叫預設建構函數；函數體內的 = 於是
//         變成對已存在物件的賦值。對管理動態資源的型別，這可能包含
//         一次多餘的配置與釋放。
//     追問：那對 int 成員有差嗎？
//         → 幾乎沒有。內建型別的「預設建構」其實什麼都不做（甚至不初始化），
//           差異僅在寫入次數，通常被最佳化掉。
//
// 🔥 Q2. 編譯器開了 -O2，能不能自動把函數體賦值最佳化成初始化列表？
//     答：一般不能。若該型別的預設建構函數或 operator= 有可觀察的副作用
//         （記憶體配置、I/O、計數），最佳化必須保持這些行為，
//         不能把它們刪除。只有在成員是簡單型別、且編譯器能證明沒有副作用時
//         才可能消掉多餘的寫入。
//     追問：怎麼驗證？
//         → 在特殊成員函數裡印字或計數，然後用 -O0 與 -O2 各跑一次比較，
//           本檔就是這麼做的。
//
// ⚠️ 陷阱. 既然只是效能差異，那我用哪種寫法都行，反正功能一樣？
//     答：功能不一定一樣。若成員是 const、是參考、或沒有預設建構函數，
//         函數體賦值**根本無法編譯**。而且就算能編，語意也不同：
//         初始化列表表達「誕生時就是這個值」，函數體賦值表達
//         「先建一個預設的，再改成這個值」——後者允許物件短暫處於
//         「已建好但還不是正確值」的中間狀態。
//     為什麼會錯：把「輸出結果相同」等同於「兩種寫法等價」。
//         實際上能否編譯、物件是否存在無效中間狀態、例外發生時的狀態，
//         三者都可能不同。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <vector>
using namespace std;

// -----------------------------------------------------------------------------
// 會出聲的成員型別：每個特殊成員函數都報告自己被呼叫
// -----------------------------------------------------------------------------
class HeavyObject {
private:
    string data;

public:
    HeavyObject() {
        data = "";
        cout << "    HeavyObject 預設建構" << endl;
    }

    HeavyObject(const string& s) {
        data = s;
        cout << "    HeavyObject 帶參建構: " << s << endl;
    }

    // 自訂的轉換賦值運算子，讓 obj = s; 能直接吃字串
    HeavyObject& operator=(const string& s) {
        data = s;
        cout << "    HeavyObject 賦值: " << s << endl;
        return *this;
    }

    const string& get() const { return data; }
};

// -----------------------------------------------------------------------------
// 方式 A：函數體賦值 → 預設建構 + 賦值（兩步）
// -----------------------------------------------------------------------------
class ContainerA {
private:
    HeavyObject obj;

public:
    ContainerA(const string& s) {
        // 進到這一行時，obj 早就被預設建構好了
        cout << "  --- 進入函數體，開始賦值 ---" << endl;
        obj = s;
        cout << "  --- 賦值完成 ---" << endl;
    }
};

// -----------------------------------------------------------------------------
// 方式 B：初始化列表 → 直接帶參建構（一步）
// -----------------------------------------------------------------------------
class ContainerB {
private:
    HeavyObject obj;

public:
    ContainerB(const string& s)
        : obj(s)  // 直接帶參建構，不經過預設建構
    {
        cout << "  --- 初始化列表已完成，進入函數體 ---" << endl;
    }
};

// -----------------------------------------------------------------------------
// 用計數器量化：建立 N 個物件時，兩種寫法各自呼叫了幾次
// -----------------------------------------------------------------------------
struct Counter {
    static int defaultCtor;
    static int paramCtor;
    static int assign;
    static void reset() { defaultCtor = paramCtor = assign = 0; }
    static void report(const string& label) {
        cout << "    " << label
             << " → 預設建構 " << defaultCtor
             << " 次, 帶參建構 " << paramCtor
             << " 次, 賦值 " << assign << " 次" << endl;
    }
};
int Counter::defaultCtor = 0;
int Counter::paramCtor = 0;
int Counter::assign = 0;

class Counted {
private:
    string data_;
public:
    Counted() { ++Counter::defaultCtor; }
    Counted(const string& s) : data_(s) { ++Counter::paramCtor; }
    Counted& operator=(const string& s) { data_ = s; ++Counter::assign; return *this; }
};

class BoxAssign {          // 函數體賦值
    Counted c_;
public:
    BoxAssign(const string& s) { c_ = s; }
};

class BoxInit {            // 初始化列表
    Counted c_;
public:
    BoxInit(const string& s) : c_(s) { }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】訊息佇列的封包物件：高頻建立時差異會累積
//   情境：網路服務每收到一個封包就建立一個 Packet 物件，
//         每秒可能數萬個。每個 Packet 內含 payload 字串與標頭欄位。
//   重點：這正是「一次多餘的預設建構」會被放大成可量測成本的場景。
//         實務寫法一律走初始化列表；若 payload 來源是右值，
//         還可以再用 std::move 連複製都省掉。
// -----------------------------------------------------------------------------
class Packet {
private:
    string topic_;
    string payload_;
    long   seq_;

public:
    // 一般版本：從 const& 複製
    Packet(const string& topic, const string& payload, long seq)
        : topic_(topic), payload_(payload), seq_(seq)
    { }

    // 移動版本：來源是右值時直接搬走資源，連複製都省
    Packet(string&& topic, string&& payload, long seq)
        : topic_(std::move(topic)), payload_(std::move(payload)), seq_(seq)
    { }

    void print() const {
        cout << "    #" << seq_ << " [" << topic_ << "] "
             << payload_.size() << " bytes" << endl;
    }
};

int main() {
    cout << "=== 方式 A：函數體賦值（兩步）===" << endl;
    { ContainerA a("Hello"); (void)a; }

    cout << "\n=== 方式 B：初始化列表（一步）===" << endl;
    { ContainerB b("Hello"); (void)b; }

    cout << "\n=== 量化：各建立 1000 個物件 ===" << endl;
    Counter::reset();
    for (int i = 0; i < 1000; ++i) { BoxAssign x("payload"); (void)x; }
    Counter::report("函數體賦值");

    Counter::reset();
    for (int i = 0; i < 1000; ++i) { BoxInit x("payload"); (void)x; }
    Counter::report("初始化列表");

    cout << "\n=== 日常實務：訊息封包 ===" << endl;
    vector<Packet> queue;
    queue.reserve(3);
    string topic = "sensor/temp";
    queue.push_back(Packet(topic, "21.7C", 1001));                    // 複製版本
    queue.push_back(Packet(string("sensor/humidity"),                 // 移動版本
                           string("48%"), 1002));
    queue.push_back(Packet(string("sensor/pressure"),
                           string("1013.2hPa"), 1003));
    for (const auto& p : queue) p.print();

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 16 課：建構函數初始化列表（Member Initializer List）10.cpp" -o demo10
//
// ※ 註：本檔的計數結果在 -O0 與 -O2 下都相同，因為特殊成員函數有可觀察的
//   副作用（計數／輸出），編譯器不能將其最佳化掉。可自行以兩種最佳化等級
//   各跑一次驗證。

// === 預期輸出 ===
// === 方式 A：函數體賦值（兩步）===
//     HeavyObject 預設建構
//   --- 進入函數體，開始賦值 ---
//     HeavyObject 賦值: Hello
//   --- 賦值完成 ---
//
// === 方式 B：初始化列表（一步）===
//     HeavyObject 帶參建構: Hello
//   --- 初始化列表已完成，進入函數體 ---
//
// === 量化：各建立 1000 個物件 ===
//     函數體賦值 → 預設建構 1000 次, 帶參建構 0 次, 賦值 1000 次
//     初始化列表 → 預設建構 0 次, 帶參建構 1000 次, 賦值 0 次
//
// === 日常實務：訊息封包 ===
//     #1001 [sensor/temp] 5 bytes
//     #1002 [sensor/humidity] 3 bytes
//     #1003 [sensor/pressure] 9 bytes
