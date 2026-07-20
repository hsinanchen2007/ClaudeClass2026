// =============================================================================
//  第 17 課：解構函數 3  —  用解構函數釋放動態記憶體（並帶出 Rule of Three）
// =============================================================================
//
// 【主題資訊 Information】
//   配對規則：new  ↔ delete
//             new[] ↔ delete[]      （**絕不可交叉使用**）
//   標準版本：C++98 起即有；std::vector 是更好的替代方案（C++98 就有）
//   複雜度：配置 O(n)（本檔還要初始化 n 個元素）；釋放 O(1)（對 int 這種平凡型別）
//   標頭檔：<cstring>
//
// 【詳細解釋 Explanation】
//
// 【1. 這就是 RAII 的原型】
//   DynamicArray 在建構函數裡 new[]、在解構函數裡 delete[]。
//   於是「記憶體的生命週期」被綁定到「物件的生命週期」：
//       {
//           DynamicArray arr(5);   // 這裡配置
//           ...
//       }                          // 這裡自動釋放，不管中間怎麼離開
//   相較之下，C 語言必須自己記得 malloc 之後要 free，
//   而且每一條 return、每一個錯誤分支都要記得——這正是記憶體洩漏的來源。
//   RAII 把「記得釋放」從人的責任變成語言的保證。
//
// 【2. new[] 一定要配 delete[]】
//   ● new  配 delete
//   ● new[] 配 delete[]
//   交叉使用（new[] 卻用 delete）是**未定義行為**。
//   原因是實作通常會在配置區塊前額外儲存「元素個數」，
//   delete[] 會據此逐一呼叫每個元素的解構函數再整塊釋放；
//   delete 則不會。對 int 這種沒有解構函數的平凡型別，
//   某些實作上「看起來沒事」，但那不是保證，仍然是 UB。
//
// 【3. 有了解構函數，就要想到 Rule of Three（本檔最重要的延伸）】
//   這個類別自己管理一塊 heap 記憶體，於是編譯器自動生成的
//   複製建構函數與複製指派運算子就變得**危險**：它們只做逐成員複製，
//   也就是把 data 這個**指標值**照抄一份。結果兩個物件指向同一塊記憶體，
//   當兩者都解構時，同一塊記憶體會被 delete[] 兩次——重複釋放，
//   屬於未定義行為。
//   規則是：**只要你需要自訂解構函數、複製建構函數、複製指派運算子中的
//   任何一個，通常三個都需要**，這就是 Rule of Three。
//   C++11 之後再加上移動建構與移動指派，稱為 Rule of Five。
//
//   本檔的處理方式：**明確把複製功能停用**（= delete）。
//   這樣任何嘗試複製的程式碼都會在**編譯期**被擋下，
//   而不是留一個執行期才爆炸的地雷。這是很實用的防禦手法：
//   如果暫時不想實作深複製，就先明確禁止複製。
//   （第 27 課會專門示範「淺複製造成重複釋放」的完整過程。）
//
// 【4. 但正確答案通常是「不要自己管」】
//   本檔示範的一切，std::vector<int> 都已經做好了，而且做得更完整：
//   深複製、移動、例外安全、動態成長全部具備。
//   自己寫 new[]/delete[] 的唯一理由是**學習它的原理**。
//   實務上的優先順序是：
//       std::vector／std::string  >  std::unique_ptr  >  自己 new/delete
//   這就是 **Rule of Zero**：讓成員自己管理資源，
//   你的類別就不需要寫任何解構函數、複製、移動函數。
//
// 【概念補充 Concept Deep Dive】
//
//   ● delete 空指標是安全的
//     delete nullptr; 與 delete[] nullptr; 都是合法的無操作。
//     所以解構函數裡不需要寫 if (data != nullptr) 再 delete。
//     但**重複 delete 同一個非空指標**是未定義行為——
//     這正是上面 Rule of Three 要防的事。
//
//   ● 為什麼標準要區分 delete 與 delete[]
//     因為兩者的釋放方式不同：delete[] 需要知道元素個數才能逐一解構。
//     這個個數通常存在配置區塊的前方（**實作定義**的作法，
//     不同編譯器與平台可能不同，不可依賴其佈局）。
//
//   ● 建構函數拋出例外時，解構函數不會被呼叫
//     若 new int[size] 之後、建構函數結束之前拋出例外，
//     這個物件視為從未建構成功，它的解構函數**不會**執行，
//     那塊記憶體就洩漏了。
//     這是「一個建構函數裡配置多個資源」很危險的原因，
//     也是應該讓每個資源各自由一個 RAII 成員持有的理由。
//
//   ● size 應該用什麼型別
//     本檔沿用 int 以貼近初學者的直覺，但配置大小的正統型別是
//     std::size_t（無號）。用 int 的話，傳入負數會造成
//     new int[負數] 這種問題——本檔在建構函數中夾住了下限以避免此情況。
//
// 【注意事項 Pay Attention】
//   1. new[] 必須配 delete[]，交叉使用是未定義行為。
//   2. 自訂了解構函數，就要一併考慮複製／移動（Rule of Three／Five）；
//      不想實作就明確 = delete 停用。
//   3. delete 空指標安全；重複 delete 同一個非空指標是未定義行為。
//   4. 實務上請優先用 std::vector；自己管理記憶體只適合學習與極特殊場合。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】解構函數與資源管理
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 什麼是 RAII？為什麼說它比「記得呼叫 free」可靠？
//     答：RAII 是把資源的生命週期綁定到物件的生命週期——建構時取得，
//         解構時釋放。可靠的原因是：物件離開作用域時，語言**保證**呼叫
//         解構函數，不論是正常結束、提早 return，還是拋出例外而堆疊展開。
//         手動釋放則必須在每一條離開路徑上都記得，只要漏一條就洩漏。
//     追問：有沒有 RAII 也保證不了的情況？
//         → 有。std::abort()、std::_Exit() 或行程被強制終止時，
//           解構函數不會執行；std::exit() 也不會解構堆疊上的區域物件。
//
// 🔥 Q2. 什麼是 Rule of Three？為什麼一寫解構函數就要想到它？
//     答：只要類別需要自訂「解構函數、複製建構函數、複製指派運算子」
//         其中任何一個，通常三個都需要。因為需要自訂解構函數，
//         代表這個類別**自己管理某種資源**；而編譯器生成的複製只做
//         逐成員的淺複製，會讓兩個物件持有同一份資源，最後重複釋放。
//     追問：C++11 之後呢？
//         → 加上移動建構與移動指派，成為 Rule of Five；
//           而最理想的是 Rule of Zero——讓成員（vector、unique_ptr）
//           自己管理資源，這五個函數一個都不用寫。
//
// ⚠️ 陷阱. 對 int 陣列用 new[] 配置卻用 delete 釋放，反正 int 沒有解構函數，
//          應該沒差吧？
//     答：不對，那是未定義行為。delete[] 與 delete 是不同的釋放路徑，
//         實作通常會在配置區塊前額外記錄元素個數供 delete[] 使用。
//         對平凡型別在某些實作上「看起來能跑」，但那是巧合而非保證，
//         換個編譯器、換個標準函式庫就可能損壞 heap。
//     為什麼會錯：把「delete[] 的用途只是為了呼叫每個元素的解構函數」
//         當成完整理由。實際上配置與釋放的**記帳方式**本身就不同，
//         與元素有沒有解構函數無關。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <cstring>
#include <vector>
using namespace std;

class DynamicArray {
private:
    int size;      // 宣告在前：data 的初值要用到它（第 16 課的順序規則）
    int* data;

public:
    DynamicArray(int sz)
        : size(sz > 0 ? sz : 1)          // 夾住下限，避免 new int[負數]
        , data(new int[static_cast<size_t>(size)])   // 此時 size 已初始化
    {
        for (int i = 0; i < size; i++) {
            data[i] = 0;
        }
        cout << "  [建構] 分配了 " << size << " 個 int 的記憶體" << endl;
    }

    ~DynamicArray() {
        delete[] data;         // new[] 配 delete[]，絕不可寫成 delete
        cout << "  [解構] 釋放了 " << size << " 個 int 的記憶體" << endl;
    }

    // ---- Rule of Three：因為自訂了解構函數，複製就必須一併處理 ----
    // 這裡選擇「明確停用」，讓任何複製嘗試在編譯期就失敗，
    // 而不是留下一個執行期才會重複釋放的地雷。
    // （若要支援複製，就得實作深複製：另外配置一塊並逐元素複製。）
    DynamicArray(const DynamicArray&) = delete;
    DynamicArray& operator=(const DynamicArray&) = delete;

    void set(int index, int value) {
        if (index >= 0 && index < size) {
            data[index] = value;
        }
    }

    int get(int index) const {
        if (index >= 0 && index < size) {
            return data[index];
        }
        return -1;
    }

    void print() const {
        cout << "  [";
        for (int i = 0; i < size; i++) {
            if (i > 0) cout << ", ";
            cout << data[i];
        }
        cout << "]" << endl;
    }
};

// -----------------------------------------------------------------------------
// 對照組：實作了深複製的版本（Rule of Three 的完整寫法）
//   每個物件都持有自己獨立的一塊記憶體，複製時另外配置並逐元素搬過去，
//   因此兩個物件各自解構時不會重複釋放。
// -----------------------------------------------------------------------------
class DeepArray {
private:
    int size_;
    int* data_;

public:
    explicit DeepArray(int sz)
        : size_(sz > 0 ? sz : 1)
        , data_(new int[static_cast<size_t>(size_)])
    {
        for (int i = 0; i < size_; ++i) data_[i] = 0;
    }

    // (1) 複製建構函數：另外配置一塊，再逐元素複製
    DeepArray(const DeepArray& other)
        : size_(other.size_)
        , data_(new int[static_cast<size_t>(other.size_)])
    {
        memcpy(data_, other.data_, sizeof(int) * static_cast<size_t>(size_));
        cout << "    [深複製] 建構出獨立的 " << size_ << " 個元素" << endl;
    }

    // (2) 複製指派運算子：先自我指派檢查，再重新配置
    DeepArray& operator=(const DeepArray& other) {
        if (this == &other) return *this;          // 自我指派保護
        int* fresh = new int[static_cast<size_t>(other.size_)];  // 先配置成功再動舊的
        memcpy(fresh, other.data_, sizeof(int) * static_cast<size_t>(other.size_));
        delete[] data_;
        data_ = fresh;
        size_ = other.size_;
        cout << "    [深指派] 重新配置為 " << size_ << " 個元素" << endl;
        return *this;
    }

    // (3) 解構函數
    ~DeepArray() { delete[] data_; }

    void set(int i, int v) { if (i >= 0 && i < size_) data_[i] = v; }
    int  get(int i) const  { return (i >= 0 && i < size_) ? data_[i] : -1; }

    void print(const string& tag) const {
        cout << "    " << tag << " [";
        for (int i = 0; i < size_; ++i) {
            if (i) cout << ", ";
            cout << data_[i];
        }
        cout << "]" << endl;
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】固定容量的取樣緩衝區：先自己寫，再對照 std::vector
//   情境：韌體或訊號處理常需要一塊「建立時就決定大小、之後不再變動」的
//         數值緩衝區，用來存放最近 N 筆取樣值並計算平均。
//   重點：先看自己管理記憶體要處理多少細節（配置、釋放、禁止複製），
//         再看同樣的需求用 std::vector 寫出來有多短——這就是 Rule of Zero
//         的價值：不是「不會寫」，而是「知道有人已經寫得更好」。
// -----------------------------------------------------------------------------
class SampleBufferVec {
private:
    vector<double> samples_;   // 由 vector 自己管理記憶體
    size_t count_ = 0;

public:
    explicit SampleBufferVec(size_t capacity) : samples_(capacity, 0.0) { }
    // 不需要解構函數、不需要複製建構、不需要複製指派 —— Rule of Zero

    bool push(double v) {
        if (count_ >= samples_.size()) return false;
        samples_[count_++] = v;
        return true;
    }

    double average() const {
        if (count_ == 0) return 0.0;
        double sum = 0.0;
        for (size_t i = 0; i < count_; ++i) sum += samples_[i];
        return sum / static_cast<double>(count_);
    }

    void print() const {
        cout << "  取樣 " << count_ << "/" << samples_.size()
             << " 筆，平均 = " << average() << endl;
    }
};

int main() {
    cout << "=== 動態陣列範例（自己管理記憶體）===" << endl;
    {
        DynamicArray arr(5);
        arr.set(0, 10);
        arr.set(1, 20);
        arr.set(2, 30);
        arr.print();

        // DynamicArray copy = arr;   // 編譯錯誤：複製已被 = delete 停用
        //                            // 這正是我們要的：問題在編譯期就被擋下

        cout << "  --- 即將離開區塊 ---" << endl;
    }
    cout << "  --- 記憶體已自動釋放 ---" << endl;

    cout << "\n=== Rule of Three 完整版：深複製 ===" << endl;
    {
        DeepArray a(3);
        a.set(0, 1); a.set(1, 2); a.set(2, 3);
        a.print("a =");

        DeepArray b = a;         // 呼叫複製建構函數 → 各自獨立
        b.set(0, 999);           // 改 b 不會影響 a
        a.print("a =");
        b.print("b =");

        DeepArray c(1);
        c = a;                   // 呼叫複製指派運算子
        c.print("c =");
        cout << "  --- 三個物件各自解構，不會重複釋放 ---" << endl;
    }

    cout << "\n=== 日常實務：改用 std::vector（Rule of Zero）===" << endl;
    SampleBufferVec buf(4);
    buf.push(21.5);
    buf.push(22.0);
    buf.push(22.9);
    buf.print();
    cout << "  ↑ 這個類別完全不需要解構函數、複製建構、複製指派" << endl;

    return 0;
}
// 離開作用域時，DynamicArray 的解構函數自動被呼叫，釋放先前配置的記憶體

// 編譯: g++ -std=c++17 -Wall -Wextra "第 17 課：解構函數（Destructor）3.cpp" -o demo3
// 建議另外用 AddressSanitizer 確認沒有洩漏與重複釋放:
//   g++ -std=c++17 -Wall -Wextra -fsanitize=address -g <檔名> -o demo3

// === 預期輸出 ===
// === 動態陣列範例（自己管理記憶體）===
//   [建構] 分配了 5 個 int 的記憶體
//   [10, 20, 30, 0, 0]
//   --- 即將離開區塊 ---
//   [解構] 釋放了 5 個 int 的記憶體
//   --- 記憶體已自動釋放 ---
//
// === Rule of Three 完整版：深複製 ===
//     a = [1, 2, 3]
//     [深複製] 建構出獨立的 3 個元素
//     a = [1, 2, 3]
//     b = [999, 2, 3]
//     [深指派] 重新配置為 3 個元素
//     c = [1, 2, 3]
//   --- 三個物件各自解構，不會重複釋放 ---
//
// === 日常實務：改用 std::vector（Rule of Zero）===
//   取樣 3/4 筆，平均 = 22.1333
//   ↑ 這個類別完全不需要解構函數、複製建構、複製指派
