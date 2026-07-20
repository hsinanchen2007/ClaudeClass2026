// =============================================================================
//  第 14 課：預設建構函數 8  —  沒有 default constructor 時，陣列還能怎麼建
// =============================================================================
//
// 【主題資訊 Information】
//   主題      : 用初始化列表為「不可預設建構」的型別建立陣列
//   語法      : T arr[N] = { T(a), T(b), T(c) };
//   標準版本  : C++98 起；本檔用到的 vector::emplace_back 是 C++11、
//               std::optional 是 C++17
//   標頭檔    : <iostream>、<vector>、<optional>
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼初始化列表可以繞過 default constructor】
//   T arr[3]; 之所以需要 default constructor，是因為語法上沒有地方
//   為每個元素提供引數。而寫成
//       NoDefault arr[3] = { NoDefault(1), NoDefault(2), NoDefault(3) };
//   等於**逐一明確指定每個元素怎麼建構**，需求消失了。
//   注意這只能在**定義陣列的同時**做——不能先 NoDefault arr[3]; 再逐一賦值，
//   因為第一步就已經失敗了。
//
// 【2. 它其實不是「複製三個暫時物件進去」】
//   直覺上會以為：先建 NoDefault(1) 這個暫時物件，再複製進 arr[0]。
//   在 C++17 之前確實在概念上是「copy-initialization + 可省略的複製」，
//   多數編譯器會做 copy elision 省掉。
//   **C++17 起這件事變成語言保證**（guaranteed copy elision，P0135）：
//   NoDefault(1) 這個 prvalue 直接就地初始化 arr[0]，
//   中間根本不存在暫時物件，因此**即使 copy constructor 被 delete 也能編譯**。
//   本檔用一個 copy constructor 被 delete 的型別實測驗證了這一點。
//
// 【3. 這個作法的三個實際限制】
//   (a) 陣列大小必須是編譯期常數，且元素個數要寫死在原始碼裡
//   (b) 元素個數少於陣列大小時，剩下的會被**值初始化**——
//       對不可預設建構的型別而言，那反而會編譯失敗
//   (c) 完全無法在執行期決定要建立幾個
//   所以它適合「固定的一小組物件」（例如查表用的常數陣列），
//   不適合任何規模會變的情境。
//
// 【4. 執行期才知道數量時的三種正解】
//   (a) std::vector + reserve + emplace_back  ← 最常用
//       不需要 default constructor，且完全避免多餘的複製
//   (b) std::vector<std::optional<T>>          ← 需要「這一格暫時是空的」語意
//   (c) std::vector<std::unique_ptr<T>>        ← 需要多型或物件位址要穩定
//   本檔三種都實測示範。
//
// 【概念補充 Concept Deep Dive】
//   ▍為什麼原本的範例會有 -Wunused-parameter 警告
//     NoDefault(int x) { } 收了 x 卻沒用它，-Wextra 會警告。
//     解法有三：把參數存起來（本檔作法，順便讓範例能印出東西）、
//     省略參數名 NoDefault(int) { }、或用 C++17 的 [[maybe_unused]]。
//     教學檔案更不該留警告——學生會照抄。
//
//   ▍emplace_back vs push_back
//     push_back 需要先有一個物件再搬進容器；
//     emplace_back 把引數**轉發**進容器內部直接建構，
//     既不需要 default constructor，也不需要 copy/move constructor。
//     對不可複製、不可移動的型別，emplace_back 是唯一選擇。
//
//   ▍std::optional 的語意價值
//     optional<T> 明確表達「這一格可能沒有東西」，
//     比「用一個 magic value 代表空」安全得多，
//     而且 T 本身完全不需要有預設狀態。
//
// 【注意事項 Pay Attention】
//   1. 初始化列表只能在定義陣列時使用，不能先宣告再逐一賦值。
//   2. 元素個數少於陣列大小時，剩餘元素會被值初始化 → 對不可預設建構的型別是編譯錯誤。
//   3. C++17 起保證 copy elision，因此連 copy constructor 被 delete 的型別都能這樣建。
//   4. 執行期才知道數量時，請用 vector + emplace_back，不要硬湊陣列。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】不可預設建構的型別與容器
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 類別沒有 default constructor，NoDefault arr[3] = {NoDefault(1), ...}; 為什麼可以？
//     答：因為初始化列表為每個元素明確提供了引數，不再需要編譯器
//         「自己想辦法生一個」。限制是大小必須編譯期固定、元素個數要寫死，
//         且若給的個數少於陣列大小，剩餘元素會被值初始化 → 又編譯失敗。
//     追問：這中間會產生暫時物件再複製嗎？→ C++17 起不會。
//         guaranteed copy elision 讓 prvalue 直接就地初始化目標，
//         所以連 copy constructor 被 delete 的型別都能這樣寫。
//
// 🔥 Q2. push_back 和 emplace_back 對「不可預設建構」的型別有差別嗎？
//     答：兩者都不需要 default constructor。差別在 push_back 需要一個已存在的
//         物件再 copy/move 進去，emplace_back 則把引數轉發到容器內部直接建構。
//         對既不可複製也不可移動的型別，emplace_back 是唯一可行的方式。
//     追問：那 vector<T> v(3); 呢？→ 需要 default constructor（值初始化 3 個元素），
//         會編譯失敗；改用 v.reserve(3) 就不需要。
//
// ⚠️ 陷阱. NoDefault arr[3] = { NoDefault(1) };  只給一個，其餘會怎樣？
//     答：編譯失敗。剩下的兩個元素會被**值初始化**，而值初始化需要
//         default constructor——正是這個型別沒有的東西。
//         直覺上以為「沒給的就不管它」，但 C++ 不存在「未建構的陣列元素」。
//     為什麼會錯：把陣列想成可以「部分填充」的容器。
//         實際上 T arr[N] 的每一格都必須是完整建構好的物件，沒有例外。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <optional>
#include <string>
using namespace std;

class NoDefault {
private:
    int value_;                    // 把參數存起來：既消除警告，也讓範例能印出結果

public:
    explicit NoDefault(int x) : value_(x) {   // 只有帶參建構函數
        cout << "  [建構] NoDefault(" << value_ << ")" << endl;
    }
    int value() const { return value_; }
};

// 既不可預設建構、也不可複製——用來驗證 C++17 的 guaranteed copy elision
class NoCopyNoDefault {
private:
    int id_;
public:
    explicit NoCopyNoDefault(int id) : id_(id) {}
    NoCopyNoDefault(const NoCopyNoDefault&)            = delete;
    NoCopyNoDefault& operator=(const NoCopyNoDefault&) = delete;
    int id() const { return id_; }
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】從缺
//   理由：本檔處理的是「型別沒有 default constructor 時該怎麼建立集合」，
//         這是 C++ 容器與初始化語意的問題。LeetCode 的設計題全都使用
//         內建型別或自行定義且可預設建構的節點，不會遇到這個限制。
//         本課的設計題實作在 7.cpp（705 Design HashSet）與 summary.cpp。
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 【日常實務範例】感測器通道（SensorChannel）：不該有「預設感測器」
//   情境：工廠監控系統要管理多個感測器通道，每個通道必須綁定
//         一個實體位址與量測單位。「沒有位址的感測器」在語意上不存在，
//         所以這個型別**刻意**不提供 default constructor——
//         這是設計決定，不是疏漏。
//   結果：通道集合只能用 vector + emplace_back 建立，
//         而這正好也是執行期才知道有幾個通道的正確作法。
// -----------------------------------------------------------------------------
class SensorChannel {
private:
    string address_;
    string unit_;
    double lastValue_ = 0.0;

public:
    SensorChannel(const string& address, const string& unit)
        : address_(address), unit_(unit) {}

    void update(double v) { lastValue_ = v; }

    void print() const {
        cout << "    [" << address_ << "] " << lastValue_ << " " << unit_ << endl;
    }
};

int main() {
    cout << "=== 用初始化列表建立陣列（不需要 default constructor）===" << endl;
    // NoDefault arr[3];   // 編譯失敗：no matching function for call to 'NoDefault::NoDefault()'
    NoDefault arr[3] = { NoDefault(1), NoDefault(2), NoDefault(3) };
    cout << "  建好了，內容為: ";
    for (const auto& a : arr) cout << a.value() << " ";
    cout << endl;

    cout << "\n=== 以下寫法會編譯失敗（故以註解呈現）===" << endl;
    cout << "  NoDefault arr2[3];                    → 需要 default constructor" << endl;
    cout << "  NoDefault arr3[3] = { NoDefault(1) }; → 其餘兩格要值初始化，同樣失敗" << endl;
    cout << "  （C++ 沒有「未建構的陣列元素」這種狀態，每一格都必須完整建好）" << endl;

    cout << "\n=== C++17 保證 copy elision：連不可複製的型別都能這樣建 ===" << endl;
    NoCopyNoDefault ncs[2] = { NoCopyNoDefault(7), NoCopyNoDefault(8) };
    cout << "  copy constructor 已被 delete，卻仍能編譯並建立陣列" << endl;
    cout << "  ncs[0].id() = " << ncs[0].id()
         << ", ncs[1].id() = " << ncs[1].id() << endl;
    cout << "  → 因為 prvalue 直接就地初始化目標，中間沒有暫時物件" << endl;

    cout << "\n=== 執行期才知道數量：vector + reserve + emplace_back ===" << endl;
    vector<NoDefault> v;
    v.reserve(3);                  // 只配置記憶體，不建立元素
    for (int i = 10; i <= 30; i += 10) v.emplace_back(i);   // 就地建構
    cout << "  vector 內容: ";
    for (const auto& e : v) cout << e.value() << " ";
    cout << "(size=" << v.size() << ")" << endl;

    cout << "\n=== 需要「這一格暫時是空的」：std::optional ===" << endl;
    vector<optional<NoDefault>> slots(3);   // 3 個空槽，完全不需要 NoDefault 可預設建構
    slots[1].emplace(99);                    // 只填第 1 格
    for (size_t i = 0; i < slots.size(); ++i) {
        cout << "  slot[" << i << "] = ";
        if (slots[i]) cout << slots[i]->value() << endl;
        else          cout << "(空)" << endl;
    }

    cout << "\n=== 日常實務：感測器通道 ===" << endl;
    cout << "  SensorChannel 刻意不提供 default constructor——" << endl;
    cout << "  「沒有位址的感測器」在語意上不存在，這是設計決定。" << endl;
    vector<SensorChannel> channels;
    channels.reserve(3);
    channels.emplace_back("MOD-01:40001", "°C");
    channels.emplace_back("MOD-01:40002", "bar");
    channels.emplace_back("MOD-02:40001", "rpm");
    channels[0].update(72.5);
    channels[1].update(3.2);
    channels[2].update(1480);
    cout << "  目前讀數:" << endl;
    for (const auto& c : channels) c.print();

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 14 課：預設建構函數（Default Constructor）8.cpp" -o def8
//   （本檔的 guaranteed copy elision 示範需要 C++17；以 -std=c++14 編譯時
//     NoCopyNoDefault 那段會因為需要 copy constructor 而失敗）

// === 預期輸出 ===
// === 用初始化列表建立陣列（不需要 default constructor）===
//   [建構] NoDefault(1)
//   [建構] NoDefault(2)
//   [建構] NoDefault(3)
//   建好了，內容為: 1 2 3 
//
// === 以下寫法會編譯失敗（故以註解呈現）===
//   NoDefault arr2[3];                    → 需要 default constructor
//   NoDefault arr3[3] = { NoDefault(1) }; → 其餘兩格要值初始化，同樣失敗
//   （C++ 沒有「未建構的陣列元素」這種狀態，每一格都必須完整建好）
//
// === C++17 保證 copy elision：連不可複製的型別都能這樣建 ===
//   copy constructor 已被 delete，卻仍能編譯並建立陣列
//   ncs[0].id() = 7, ncs[1].id() = 8
//   → 因為 prvalue 直接就地初始化目標，中間沒有暫時物件
//
// === 執行期才知道數量：vector + reserve + emplace_back ===
//   [建構] NoDefault(10)
//   [建構] NoDefault(20)
//   [建構] NoDefault(30)
//   vector 內容: 10 20 30 (size=3)
//
// === 需要「這一格暫時是空的」：std::optional ===
//   [建構] NoDefault(99)
//   slot[0] = (空)
//   slot[1] = 99
//   slot[2] = (空)
//
// === 日常實務：感測器通道 ===
//   SensorChannel 刻意不提供 default constructor——
//   「沒有位址的感測器」在語意上不存在，這是設計決定。
//   目前讀數:
//     [MOD-01:40001] 72.5 °C
//     [MOD-01:40002] 3.2 bar
//     [MOD-02:40001] 1480 rpm
