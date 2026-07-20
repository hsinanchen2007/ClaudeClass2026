// =============================================================================
//  第 19 課：動態對象的創建與銷毀 3  —  new[] 與 delete[]：動態陣列
// =============================================================================
//
// 【主題資訊 Information】
//   語法：T* p = new T[n];        // n 個元素，預設初始化
//         T* p = new T[n]();      // 值初始化（純量會全部歸零）
//         T* p = new T[n]{...};   // C++11 初始化列表
//         delete[] p;             // 反序解構全部元素 + 釋放記憶體
//   標準版本：new[]/delete[] 自 C++98；大括號初始化列表為 C++11。
//   複雜度：配置 O(1)（加上 n 次建構）；delete[] 為 n 次解構 + O(1) 釋放。
//   ★ 元素以「順序」建構，但以「反序」解構——與區域物件的規則一致。
//
// 【詳細解釋 Explanation】
//
// 【1. delete[] 怎麼知道要解構幾個元素？】
//   這是本主題最核心的機制。對「有非平凡解構函式」的型別，
//   編譯器會在配置時多要一點記憶體，把元素個數存在使用者資料「前面」：
//       [ 元素個數 n ][ elem0 ][ elem1 ] ... [ elem(n-1) ]
//                     ↑
//                     new[] 回傳的指標指這裡
//   delete[] 收到指標後往前偏移讀出 n，反序呼叫 n 次解構函式，
//   再把「真正的起始位址」還給配置器。
//   ★ 由此立刻推出兩個重要結論：
//     (a) 對 int 這種沒有解構函式的型別，實作通常「不需要」存 n
//         （沒有東西要解構），所以 new int[5] 可能沒有這個標頭。
//     (b) 用 delete（沒有 []）釋放 new[] 來的指標，會把「使用者資料位址」
//         直接交給配置器，而真正的區塊起點在它前面 → 未定義行為。
//         這正是 4.cpp 要講的配對規則。
//
// 【2. 三種初始化方式的差別（延續 1.cpp）】
//       int* a = new int[5];                  // 預設初始化 → 值不確定
//       int* b = new int[5]();                // 值初始化 → 全部為 0
//       int* c = new int[5]{10,20,30,40,50};  // C++11 → 指定值
//       int* d = new int[5]{10, 20};          // 前兩個指定，其餘值初始化為 0
//   ★ 本檔刻意「不印出」未初始化陣列的內容：
//     讀取不確定值是未定義行為，且每次執行結果都不同
//     （原始教材印出的那串數字正是這種不可重現的值），
//     不能寫成可驗證的預期輸出。
//
// 【3. new[] 對類別的要求：必須有可用的預設建構函式】
//       Soldier* squad = new Soldier[3];   // 呼叫 3 次「預設」建構函式
//   因為 new[] 無法為不同元素傳不同的引數。若類別沒有預設建構函式，
//   這行就編譯失敗。想要逐一給不同引數，只能：
//     ● 用 std::vector<Soldier> 搭配 emplace_back
//     ● 或用 C++11 的初始化列表 new Soldier[3]{Soldier(1), Soldier(2), Soldier(3)}
//   這是 new[] 相對於 vector 的一個明顯限制。
//
// 【4. 為什麼實務上幾乎不該用 new[]？】
//   std::vector 在每一個面向都勝過裸 new[]：
//     ● 自動釋放，不可能忘記 delete[]
//     ● 例外安全（中途拋例外也會正確清理）
//     ● 知道自己的大小（裸指標不知道，你得自己另外傳 n）
//     ● 可以成長（new[] 的大小固定，要變大只能重新配置 + 手動搬移）
//     ● 支援迭代器、range-based for、標準演算法
//   而且 vector 沒有額外成本——它內部就是一塊連續記憶體。
//   ★ 學 new[] 的價值在於「理解 vector 底層在做什麼」，而非拿來日常使用。
//
// 【概念補充 Concept Deep Dive】
//   ● 為什麼元素要「反序」解構？理由與區域物件相同：後建構的可能依賴
//     先建構的，反序才能保證解構時相依對象仍存活。本檔輸出可見
//     士兵 #3 → #2 → #1 的順序。
//   ● 建構到一半失敗：若第 k 個元素的建構函式拋出例外，
//     已建好的前 k-1 個會被反序解構，然後記憶體被釋放，例外繼續傳播。
//     這是標準保證的，不會洩漏。
//   ● operator new[] 與 operator new 是不同的函式，可以分別多載。
//   ● 陣列大小為 0 是合法的：new int[0] 回傳一個唯一的、不可解參考的指標，
//     仍然必須 delete[]。
//
// 【注意事項 Pay Attention】
//   1. new[] 必須配 delete[]，new 必須配 delete。混用是未定義行為（見 4.cpp）。
//   2. `new T[n]` 不初始化（對純量而言），讀取其值是未定義行為；
//      `new T[n]()` 才保證歸零。
//   3. new[] 對類別要求「可用的預設建構函式」。
//   4. 裸指標不知道自己的長度，你必須自行保存 n——這是常見的越界來源。
//   5. 實務上請用 std::vector；本檔的 new[] 主要是為了理解底層機制。
//   6. 元素以順序建構、反序解構。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】new[] 與 delete[]
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. delete[] 怎麼知道要呼叫幾次解構函式？
//     答：對有非平凡解構函式的型別，編譯器在配置時會多要一塊空間，
//         把元素個數存在使用者資料的前面（cookie/標頭）。
//         delete[] 拿到指標後往前偏移讀出 n，反序解構 n 次，
//         再把真正的區塊起點交還配置器。
//     追問：那 new int[5] 也有這個標頭嗎？
//         → 通常沒有。int 沒有解構函式，不需要知道個數，
//           實作可以省掉這份額外開銷。這也是為什麼「混用 delete
//           釋放 new int[] 有時看起來沒事」——但它仍然是未定義行為。
//
// 🔥 Q2. 為什麼實務上建議用 std::vector 取代 new[]？
//     答：vector 在每個面向都更好——自動釋放、例外安全、知道自己的大小、
//         可以成長、支援迭代器與標準演算法，而且沒有額外的執行期成本
//         （內部同樣是一塊連續記憶體）。裸 new[] 大小固定、不知長度、
//         必須手動配對 delete[]，任何提前 return 或例外都會洩漏。
//     追問：那還需要學 new[] 嗎？
//         → 需要，因為 vector 底層正是用「配置一大塊 + 逐一建構」實作的。
//           理解 new[] 才能理解 capacity 與 size 為什麼是兩件事。
//
// ⚠️ 陷阱. 對 new Soldier[3] 誤用 delete（沒有 []），會怎樣？
//     答：未定義行為。實務上通常有兩個後果：只有第一個元素被解構
//         （其餘兩個的資源洩漏），而且交還給配置器的位址是錯的
//         （真正的區塊起點在使用者指標前面，那裡存著元素個數），
//         很可能直接讓堆積管理結構損毀而崩潰。
//     為什麼會錯：以為「delete 會依指標自動判斷是不是陣列」。
//         實際上 delete 與 delete[] 是兩個不同的運算子，
//         編譯器完全依「你寫了哪一個」產生程式碼，不做任何執行期判斷——
//         型別 Soldier* 上並沒有「我是陣列」這個資訊。
//         把它想成「你必須告訴編譯器當初是怎麼配置的」，就不會弄錯。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <vector>
using namespace std;

class Soldier {
private:
    int id;
    static int nextId;

public:
    Soldier() : id(nextId++) {
        cout << "  [建構] 士兵 #" << id << endl;
    }
    
    ~Soldier() {
        cout << "  [解構] 士兵 #" << id << endl;
    }
    
    void report() const {
        cout << "  士兵 #" << id << " 報到！" << endl;
    }
};

int Soldier::nextId = 1;

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 705. Design HashSet
//   題目：不使用內建雜湊表，自行實作 HashSet，支援 add / remove / contains。
//   為什麼用到本主題：雜湊表的核心就是一個「桶陣列（bucket array）」——
//         大小在建構時決定、必須動態配置、且要在解構時整批釋放，
//         這正是 new[] / delete[] 的教科書用途。
//         本例用 new bucket[BUCKETS] 配置桶陣列，並在解構函式中 delete[]，
//         示範「配置與釋放必須嚴格配對」。
//   設計：固定 769 個桶（質數可降低碰撞），每個桶是一條 vector 鏈。
//   複雜度：平均 O(1)，最壞 O(n/桶數)；空間 O(桶數 + 元素數)。
// -----------------------------------------------------------------------------
class MyHashSet {
    static const int BUCKETS = 769;      // 用質數當桶數，分佈較均勻
    vector<int>* table;                  // 動態配置的桶陣列

    int hash(int key) const { return key % BUCKETS; }

public:
    MyHashSet() {
        table = new vector<int>[BUCKETS];   // ★ new[]：配置並預設建構 769 個 vector
    }

    ~MyHashSet() {
        delete[] table;                     // ★ delete[]：反序解構 769 個 vector 後釋放
    }

    // 本例持有裸指標，禁止複製以免淺複製造成 double delete
    MyHashSet(const MyHashSet&) = delete;
    MyHashSet& operator=(const MyHashSet&) = delete;

    void add(int key) {
        vector<int>& bucket = table[hash(key)];
        for (int v : bucket) if (v == key) return;   // 已存在就不重複加
        bucket.push_back(key);
    }

    void remove(int key) {
        vector<int>& bucket = table[hash(key)];
        for (size_t i = 0; i < bucket.size(); ++i) {
            if (bucket[i] == key) {
                bucket.erase(bucket.begin() + static_cast<long>(i));
                return;
            }
        }
    }

    bool contains(int key) const {
        const vector<int>& bucket = table[hash(key)];
        for (int v : bucket) if (v == key) return true;
        return false;
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】固定大小的環形緩衝區（ring buffer）
//   情境：收集最近 N 筆感測器讀數，超過就覆蓋最舊的。
//   容量在執行期才決定，且大小固定不變——這是 new[] 少數仍算合理的場景
//   （雖然 std::vector 同樣做得到，且更安全）。
//   這裡用 new T[n]() 值初始化，保證所有格子一開始都是 0，
//   避免讀到不確定值。
// -----------------------------------------------------------------------------
class RingBuffer {
    double* data;
    size_t  cap;
    size_t  head;    // 下一個要寫入的位置
    size_t  count;   // 目前有效筆數

public:
    explicit RingBuffer(size_t n)
        : data(new double[n]()),      // ★ 加 () 值初始化，全部為 0
          cap(n), head(0), count(0) {}

    ~RingBuffer() { delete[] data; }

    RingBuffer(const RingBuffer&) = delete;
    RingBuffer& operator=(const RingBuffer&) = delete;

    void push(double v) {
        data[head] = v;
        head = (head + 1) % cap;
        if (count < cap) ++count;
    }

    double average() const {
        if (count == 0) return 0.0;
        double sum = 0.0;
        for (size_t i = 0; i < count; ++i) sum += data[i];
        return sum / static_cast<double>(count);
    }

    size_t size() const { return count; }
    size_t capacity() const { return cap; }
};

int main() {
    cout << "=== new[] 與 delete[] ===" << endl;
    
    // ====== 基本型別陣列 ======
    cout << "\n--- 基本型別陣列 ---" << endl;
    int* nums = new int[5];           // 預設初始化：值不確定，讀取是未定義行為
    int* zeros = new int[5]();        // 5 個 int，全部初始化為 0, 因為 () 會值初始化
    int* init = new int[5]{10, 20, 30, 40, 50};  // C++11 初始化列表, 初始化為指定值
    int* partial = new int[5]{7, 8};  // 前兩個指定，其餘值初始化為 0
    
    // ★ 刻意不印出 nums 的內容：讀取未初始化記憶體是未定義行為，
    //   其值每次執行都不同，無法寫成可驗證的預期輸出。
    cout << "  nums:  (new int[5] 未初始化，讀取是未定義行為，故不印出)" << endl;
    for (int i = 0; i < 5; i++) nums[i] = i * 100;   // 先寫入才能安全讀取
    cout << "  nums:  ";
    for (int i = 0; i < 5; i++) cout << nums[i] << " ";
    cout << "(寫入之後才讀取)" << endl;
    
    cout << "  zeros: ";
    for (int i = 0; i < 5; i++) cout << zeros[i] << " ";
    cout << "(new int[5]() 保證全為 0)" << endl;
    
    cout << "  init:  ";
    for (int i = 0; i < 5; i++) cout << init[i] << " ";
    cout << endl;

    cout << "  部分:  ";
    for (int i = 0; i < 5; i++) cout << partial[i] << " ";
    cout << "(未列出的元素值初始化為 0)" << endl;
    
    delete[] nums;
    delete[] zeros;
    delete[] init;
    delete[] partial;
    
    // ====== 類別物件陣列 ======
    cout << "\n--- 類別物件陣列 ---" << endl;
    cout << "  創建 3 個士兵：" << endl;
    Soldier* squad = new Soldier[3];   // 調用 3 次預設建構函數, 返回指向陣列首元素的指標
    
    cout << "\n  點名：" << endl;
    for (int i = 0; i < 3; i++) {
        squad[i].report();
    }
    
    cout << "\n  解散（注意是反序解構）：" << endl;
    delete[] squad;   // 調用 3 次解構函數，然後釋放記憶體, 注意要用 delete[] 來對應 new[]，
                      // 否則只會調用第一個元素的解構函數，導致資源洩漏

    // ====== LeetCode 705 ======
    cout << "\n=== LeetCode 705. Design HashSet ===" << endl;
    {
        MyHashSet st;
        st.add(1);
        st.add(2);
        cout << "  add(1), add(2)" << endl;
        cout << "  contains(1) = " << (st.contains(1) ? "true" : "false") << endl;
        cout << "  contains(3) = " << (st.contains(3) ? "true" : "false") << endl;
        st.add(2);                       // 重複加入不應產生第二筆
        cout << "  再 add(2)（重複），contains(2) = "
             << (st.contains(2) ? "true" : "false") << endl;
        st.remove(2);
        cout << "  remove(2) 之後 contains(2) = "
             << (st.contains(2) ? "true" : "false") << endl;
        cout << "  離開作用域，解構函式以 delete[] 釋放 769 個桶" << endl;
    }

    // ====== 實務範例 ======
    cout << "\n=== 日常實務：固定大小環形緩衝區 ===" << endl;
    {
        RingBuffer rb(4);
        cout << "  容量 = " << rb.capacity() << "，初始筆數 = " << rb.size() << endl;
        rb.push(10.0);
        rb.push(20.0);
        rb.push(30.0);
        cout << "  push 10, 20, 30 -> 筆數 = " << rb.size()
             << "，平均 = " << rb.average() << endl;
        rb.push(40.0);
        rb.push(50.0);                   // 覆蓋最舊的 10.0
        cout << "  再 push 40, 50 -> 筆數 = " << rb.size()
             << "，平均 = " << rb.average() << endl;
        cout << "  （50,20,30,40 的平均 = 35）" << endl;
    }
    
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 19 課：動態對象的創建與銷毀（new  delete）3.cpp" -o newdel3

// 【輸出說明】本檔刻意「不印出」未初始化陣列（new int[5]）的內容：
//   讀取不確定值是未定義行為，每次執行結果都不同，
//   不能寫成可驗證的預期輸出。改為先寫入再讀取，故下列輸出完全可重現。
//   另注意士兵是「順序建構、反序解構」（#1#2#3 建構，#3#2#1 解構）。

// === 預期輸出 ===
// === new[] 與 delete[] ===
//
// --- 基本型別陣列 ---
//   nums:  (new int[5] 未初始化，讀取是未定義行為，故不印出)
//   nums:  0 100 200 300 400 (寫入之後才讀取)
//   zeros: 0 0 0 0 0 (new int[5]() 保證全為 0)
//   init:  10 20 30 40 50 
//   部分:  7 8 0 0 0 (未列出的元素值初始化為 0)
//
// --- 類別物件陣列 ---
//   創建 3 個士兵：
//   [建構] 士兵 #1
//   [建構] 士兵 #2
//   [建構] 士兵 #3
//
//   點名：
//   士兵 #1 報到！
//   士兵 #2 報到！
//   士兵 #3 報到！
//
//   解散（注意是反序解構）：
//   [解構] 士兵 #3
//   [解構] 士兵 #2
//   [解構] 士兵 #1
//
// === LeetCode 705. Design HashSet ===
//   add(1), add(2)
//   contains(1) = true
//   contains(3) = false
//   再 add(2)（重複），contains(2) = true
//   remove(2) 之後 contains(2) = false
//   離開作用域，解構函式以 delete[] 釋放 769 個桶
//
// === 日常實務：固定大小環形緩衝區 ===
//   容量 = 4，初始筆數 = 0
//   push 10, 20, 30 -> 筆數 = 3，平均 = 20
//   再 push 40, 50 -> 筆數 = 4，平均 = 35
//   （50,20,30,40 的平均 = 35）
