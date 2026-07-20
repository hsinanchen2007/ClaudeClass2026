// =============================================================================
//  第 15 課：帶參數的建構函數 6  —  初始化列表：同名參數的最佳解法
// =============================================================================
//
// 【主題資訊 Information】
//   語法：  Class(const T& name, int age) : name(name), age(age) { }
//                                          └─括號外＝成員─┘└括號內＝參數┘
//   標準版本：C++98 起即有；本檔用到的 const& 傳參也是 C++98
//   複雜度：O(1) 的語法成本；相對「函數體內賦值」少一次預設建構
//   標頭檔：<string>
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼同名在這裡不會衝突】
//   這是本檔唯一但最關鍵的知識點。初始化列表中每個項目長成 成員(初值)，
//   標準對這兩個位置規定了**不同的名稱查找規則**：
//     ● 括號外的名字（被初始化的對象）：只在**類別作用域**查找
//         → 一定是成員，不可能找到參數。
//     ● 括號內的運算式：依**一般查找規則**（由內而外）
//         → 先找到建構函數的參數。
//   所以 : name(name) 讀作「用參數 name 來初始化成員 name」，語意毫無歧義。
//   這跟函數體內的 name = name;（兩邊都是參數，自我賦值）是完全不同的機制。
//
// 【2. 這解決了前面幾個檔案的所有問題】
//   ● 不必替參數改名（風格 A 的底線）
//   ● 不必替成員加前綴／後綴（風格 B、C）
//   ● 不必每行都寫 this->（風格 D）
//   ● 而且順便升級成「初始化」而非「賦值」，效率更好
//   換句話說，同名參數配初始化列表是最省事、也最安全的組合。
//
// 【3. 初始化 vs 賦值：省下來的到底是什麼】
//   函數體內賦值的流程是兩步：
//       (a) 進入函數體前，成員 name 先被**預設建構**成空字串
//       (b) 函數體內再以 operator= 把參數的內容**複製**進去
//   初始化列表只有一步：
//       (a) 直接以**複製建構函數**用參數建出成員
//   對 int 這種內建型別差異微乎其微；但對 std::string、std::vector 這類
//   要配置記憶體的型別，等於省掉一次「先建一個空的、再丟掉重來」。
//
// 【4. 什麼時候「非用不可」】
//   有三種成員根本不能在函數體內賦值，只能靠初始化列表：
//     ● const 成員 —— 建出來就不能改，必須誕生時給值
//     ● 參考成員（T&）—— 必須在誕生時綁定對象
//     ● 沒有預設建構函數的類別型別成員 —— 進函數體前沒有東西可以先建
//   另外，base class 的建構也只能寫在初始化列表。這些在第 16 課會展開。
//
// 【概念補充 Concept Deep Dive】
//
//   ● 初始化的真正順序由「宣告順序」決定
//     成員一律按照在 class 內的**宣告順序**初始化，與初始化列表的書寫順序
//     無關。本檔 name 宣告在 age 前面，所以 name 先初始化。
//     若書寫順序與宣告順序不一致，g++ 會發出 -Wreorder 警告——它只是提醒你
//     「你寫的順序不是真正發生的順序」，並不會改變執行順序。
//     這件事在成員之間有依賴時會變成真正的 bug（本課第 16 課 7.cpp 有專門示範）。
//
//   ● 為什麼不寫成 : name(std::move(name))
//     本檔參數是 const string&，move 一個 const 物件會退化成複製，沒有效果。
//     若要吃到移動，參數要改成值傳遞 string name，再 : name(std::move(name))。
//     那是 C++11 之後的另一種常見寫法，取捨見本課 2.cpp 的說明。
//
//   ● 空的函數體 { } 不是浪費
//     初始化列表做完所有初始化後，函數體常常就沒事可做了。空函數體代表
//     「這個建構函數只負責初始化，沒有額外邏輯」，是好事而非缺陷。
//
// 【注意事項 Pay Attention】
//   1. 同名只在**初始化列表**安全；一旦進了函數體，name 仍然是參數。
//   2. 初始化列表的書寫順序請與宣告順序一致，避免 -Wreorder 警告與誤解。
//   3. 初始化列表裡不要放複雜運算；需要計算或驗證的值，先用 helper 函數
//      算好再傳進來，可讀性與例外安全都比較好。
//   4. 若成員之間有依賴（例如用長度去配置陣列），務必確認被依賴者宣告在前。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】初始化列表與同名參數
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. : name(name), age(age) 這種同名寫法為什麼是對的？
//     答：初始化列表中括號外的名字只在類別作用域查找，必定是成員；
//         括號內依一般規則查找，找到的是建構函數參數。兩個位置的查找規則
//         不同，所以同名不會像函數體內的 name = name; 那樣變成自我賦值。
//     追問：那在函數體裡寫 name = name; 呢？
//         → 兩邊都是參數，是自我賦值，成員不會被設定，而且 -Wall -Wextra
//           抓不到，要開 -Wshadow。
//
// 🔥 Q2. 初始化列表和在建構函數本體內賦值，差別只有效能嗎？
//     答：不只。效能上初始化列表少一次預設建構；但更關鍵的是**功能差異**：
//         const 成員、參考成員、沒有預設建構函數的成員，以及 base class，
//         都只能在初始化列表初始化，函數體內根本寫不出來。
//     追問：那什麼情況還是得把邏輯寫在函數體？
//         → 需要條件判斷、迴圈、驗證後拋例外這類流程時；
//           初始化列表只適合單一運算式。
//
// ⚠️ 陷阱. 初始化列表的執行順序，是照我寫的順序嗎？
//     答：不是。成員一律按照在 class 內的**宣告順序**初始化，
//         書寫順序完全不影響實際順序，只會影響編譯器要不要給你 -Wreorder 警告。
//     為什麼會錯：把初始化列表當成一般的敘述序列，以為「由上往下執行」。
//         實際上它是「宣告式」的——你只是在指定每個成員該怎麼初始化，
//         真正的順序由類別佈局決定（這樣才能保證解構順序剛好相反）。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <vector>
using namespace std;

class Solution3 {
private:
    string name;   // 宣告在前 → 先初始化
    int age;       // 宣告在後 → 後初始化

public:
    // 初始化列表中，括號外是成員變數，括號內是參數，即使同名也不會衝突
    // 這是四種解法中最推薦的：不必改名、不必加前綴、不必寫 this->
    Solution3(const string& name, int age)
        : name(name), age(age) { }

    void print() const {
        cout << "  name = " << name << ", age = " << age << endl;
    }
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1656. Design an Ordered Stream
//   題目：設計一個資料流，建構時給定總數 n；insert(idKey, value) 依 id 放入
//         資料，並回傳「從目前指標開始、連續已就緒」的那一段值。
//   為什麼用到本主題：建構函數 OrderedStream(int n) 拿到參數 n 之後，
//         必須用它去決定內部容器的大小——這正是「用建構函數參數初始化成員」
//         的典型場景，而且 data_(n + 1) 只能在初始化列表做才不會浪費一次
//         預設建構（先建空 vector 再 resize）。
//   複雜度：每個元素最多被指標掃過一次，整體攤平 O(n)。
// -----------------------------------------------------------------------------
class OrderedStream {
private:
    vector<string> data_;   // 宣告在前 → 先初始化
    int ptr_;               // 目前掃描到的位置（題目用 1-based）

public:
    // 用參數 n 直接把 vector 建成 n+1 格（索引 0 不用，方便 1-based）
    explicit OrderedStream(int n) : data_(static_cast<size_t>(n) + 1), ptr_(1) { }

    vector<string> insert(int idKey, const string& value) {
        data_[static_cast<size_t>(idKey)] = value;

        vector<string> chunk;
        // 從 ptr_ 開始，把已經填好的連續段一次吐出來
        while (ptr_ < static_cast<int>(data_.size())
               && !data_[static_cast<size_t>(ptr_)].empty()) {
            chunk.push_back(data_[static_cast<size_t>(ptr_)]);
            ++ptr_;
        }
        return chunk;
    }
};

// 小工具：把回傳的 chunk 印成 [a, b, c] 的樣子
static void printChunk(const vector<string>& v) {
    cout << "  [";
    for (size_t i = 0; i < v.size(); ++i) {
        if (i) cout << ", ";
        cout << v[i];
    }
    cout << "]" << endl;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】環形緩衝區（ring buffer）：容量由建構參數決定
//   情境：收集最近 N 筆感測器讀數或 log 行，滿了就覆蓋最舊的。
//         監控代理程式、韌體、遊戲的最近事件紀錄都常用這個結構。
//   重點：capacity 是 const 成員（建好就不該改），**只能**用初始化列表設定；
//         buf_ 的大小也必須用參數初始化。這兩點都印證了本檔主題。
// -----------------------------------------------------------------------------
class RingBuffer {
private:
    const size_t capacity_;    // const 成員 → 非用初始化列表不可
    vector<double> buf_;
    size_t count_;             // 已寫入總筆數（用來算目前位置）

public:
    explicit RingBuffer(size_t capacity)
        : capacity_(capacity), buf_(capacity, 0.0), count_(0) { }

    void push(double v) {
        buf_[count_ % capacity_] = v;
        ++count_;
    }

    void print() const {
        size_t n = (count_ < capacity_) ? count_ : capacity_;
        cout << "  最近 " << n << " 筆: ";
        // 從最舊的一筆開始印
        size_t start = (count_ < capacity_) ? 0 : (count_ % capacity_);
        for (size_t i = 0; i < n; ++i) {
            cout << buf_[(start + i) % capacity_] << " ";
        }
        cout << "(總寫入 " << count_ << " 筆)" << endl;
    }
};

int main() {
    cout << "=== 初始化列表：同名參數也不衝突 ===" << endl;
    Solution3 s("王五", 25);
    s.print();

    cout << "\n=== LeetCode 1656. Design an Ordered Stream ===" << endl;
    OrderedStream os(5);
    cout << "  insert(3, ccccc):"; printChunk(os.insert(3, "ccccc"));
    cout << "  insert(1, aaaaa):"; printChunk(os.insert(1, "aaaaa"));
    cout << "  insert(2, bbbbb):"; printChunk(os.insert(2, "bbbbb"));
    cout << "  insert(5, eeeee):"; printChunk(os.insert(5, "eeeee"));
    cout << "  insert(4, ddddd):"; printChunk(os.insert(4, "ddddd"));

    cout << "\n=== 日常實務：環形緩衝區（容量由建構參數決定）===" << endl;
    RingBuffer rb(4);
    rb.push(21.5); rb.push(21.7); rb.push(22.0);
    rb.print();                       // 還沒滿
    rb.push(22.3); rb.push(23.1);     // 滿了之後開始覆蓋最舊的
    rb.print();

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 15 課：帶參數的建構函數6.cpp" -o demo6

// === 預期輸出 ===
// === 初始化列表：同名參數也不衝突 ===
//   name = 王五, age = 25
//
// === LeetCode 1656. Design an Ordered Stream ===
//   insert(3, ccccc):  []
//   insert(1, aaaaa):  [aaaaa]
//   insert(2, bbbbb):  [bbbbb, ccccc]
//   insert(5, eeeee):  []
//   insert(4, ddddd):  [ddddd, eeeee]
//
// === 日常實務：環形緩衝區（容量由建構參數決定）===
//   最近 3 筆: 21.5 21.7 22 (總寫入 3 筆)
//   最近 4 筆: 21.7 22 22.3 23.1 (總寫入 5 筆)
