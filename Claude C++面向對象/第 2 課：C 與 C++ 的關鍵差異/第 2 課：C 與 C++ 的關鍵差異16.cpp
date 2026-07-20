// =============================================================================
//  第 2 課：C 與 C++ 的關鍵差異 16  —  C++ 的 struct：資料與行為可以放在一起
// =============================================================================
//
// 【主題資訊 Information】
//   語法：struct Point { int x; int y; void print(); };
//         宣告變數直接寫  Point p1;  （不需要 struct 關鍵字）
//   標準版本：C++98 起即如此。C 語言的 struct 至今仍只能放資料。
//   標頭檔：本檔用 <iostream>、<vector>、<string>。
//   複雜度：成員函式的呼叫成本與自由函式相同——非虛擬成員函式在編譯後
//           就是一個普通函式呼叫，只是多傳一個隱藏的 this 指標。
//
// 【詳細解釋 Explanation】
//
// 【1. C++ 的 struct 和 class 差在哪？只差一個字】
//   唯一的差異是「預設存取權限」與「預設繼承方式」：
//       struct → 預設 public
//       class  → 預設 private
//   除此之外完全等價：struct 可以有建構/解構函式、可以繼承、可以有虛擬函式、
//   可以寫 private 區段、可以多載運算子。
//   慣例（不是規則）是：純資料聚合用 struct，需要維護不變式（invariant）
//   並對外隱藏內部狀態的用 class。本檔的 Point 屬於前者。
//
// 【2. 成員函式底層發生了什麼事？—— 隱藏的 this】
//   編譯器把
//       void Point::print() { std::cout << x; }
//   大致轉換成一個帶額外參數的自由函式：
//       void Point_print(Point* this) { std::cout << this->x; }
//   而 p1.print(); 則轉換成 Point_print(&p1);
//   由此可推出幾個重要結論：
//     (a) 成員函式不佔物件的空間。sizeof(Point) 仍然只有兩個 int，
//         不會因為多加幾個成員函式而變大——因為程式碼只有一份，
//         放在程式的 text 區段，所有物件共用。
//     (b) 非虛擬成員函式沒有任何額外執行期成本，可以被 inline。
//     (c) static 成員函式沒有 this，所以不能存取非靜態成員。
//   ★ 只有「虛擬函式」才會讓物件變大（多一個 vptr），這是常見的混淆點，
//     本檔會實測 sizeof 印出來證明。
//
// 【3. 為什麼「資料 + 行為」放在一起是進步？】
//   在 C 裡，資料與操作它的函式是分離的：
//       struct Point p;
//       point_print(&p);         /* 函式在哪？叫什麼名字？要自己記 */
//   問題不只是打字麻煩：
//     ● 沒有命名空間，所有函式擠在全域，只好用 point_ 前綴人工分群。
//     ● 沒有存取控制，任何人都能直接改欄位，破壞資料的一致性。
//     ● 沒有建構函式，「物件建立後一定處於合法狀態」無法被保證。
//   C++ 把行為收進型別裡，讓「這個型別能做什麼」自我說明（p1.print()），
//   並且能用 private + 建構函式強制維持不變式。這就是封裝（encapsulation）
//   的起點，也是整個物件導向的基礎。
//
// 【4. const 成員函式：把「唯讀」寫進型別】
//       void print() const;      // 承諾不修改成員
//   加了 const 之後，const 物件與 const 參考才能呼叫它。
//   這是 C 完全沒有的表達力——C 只能靠 const 指標參數表達「不會改」，
//   而且無法阻止函式內部把 const 轉掉。
//   實務守則：只要成員函式不修改狀態，就一律加 const。
//   本檔刻意保留原始的非 const print()，並在下面補上 const 版本示範。
//
// 【概念補充 Concept Deep Dive】
//   ● 記憶體佈局相容性：只要 struct 是 standard-layout（沒有虛擬函式、
//     沒有虛擬繼承、非靜態成員存取權限一致…），它在 C 和 C++ 的佈局相同，
//     可以安全地傳給 C 函式庫或用 memcpy 序列化。加了成員函式並不會
//     破壞 standard-layout；加了虛擬函式才會。
//   ● 聚合初始化（aggregate initialization）：
//         Point p{10, 20};
//     這在 struct 沒有使用者定義建構函式、沒有 private 非靜態成員時可用。
//     C++11 起支援大括號初始化，C++14 起支援有預設成員初始器的聚合。
//   ● 空類別的 sizeof 至少為 1：因為每個物件必須有唯一位址，
//     兩個相鄰的空物件不能佔同一個地址。
//
// 【注意事項 Pay Attention】
//   1. struct 與 class 只差預設存取權限，其餘完全相同——
//      「struct 不能有成員函式」是 C 的規則，不是 C++ 的。
//   2. 非虛擬成員函式不會增加 sizeof；虛擬函式才會（多一個 vptr）。
//      本檔實測值 8 / 16 bytes 是本機 g++ 15.2 x86-64 的結果，
//      屬實作定義，不是標準保證。
//   3. 不修改狀態的成員函式請加 const，否則 const 物件無法呼叫它。
//   4. 在類別定義內直接寫本體的成員函式，隱含具有 inline 語意，
//      可以安全地放在標頭檔而不違反單一定義規則（ODR）。
//   5. 本檔原始的 print() 未加 const，這是為了保留原教材樣貌；
//      實務上應寫成 void print() const。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】C++ 的 struct 與成員函式
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. C++ 的 struct 和 class 有什麼差別？
//     答：只有兩個預設值不同——struct 的成員與繼承預設是 public，
//         class 預設是 private。其餘能力完全相同：兩者都能有建構/解構函式、
//         虛擬函式、繼承、運算子多載。慣例上純資料用 struct、
//         需要封裝不變式的用 class，但那是風格而非語言規則。
//     追問：那 C 的 struct 和 C++ 的 struct 呢？
//         → 差很多。C 的 struct 只能放資料，沒有成員函式、存取控制、
//           建構/解構函式；且宣告變數必須寫 struct 關鍵字。
//
// 🔥 Q2. 在 struct 裡加了 5 個成員函式，sizeof 會變大嗎？
//     答：不會。成員函式的程式碼只有一份，放在程式的 text 區段，
//         所有物件共用；呼叫時只是多傳一個隱藏的 this 指標。
//         唯一會讓物件變大的是「虛擬函式」——它需要一個 vptr 指向虛擬表。
//     追問：那 static 成員變數呢？
//         → 也不佔物件空間。它只有一份，儲存在靜態儲存區，
//           所有物件共享，不屬於任何一個物件。
//
// ⚠️ 陷阱. 成員函式是「每個物件各有一份」嗎？
//     答：不是。程式碼只有一份，被所有物件共用。所謂「不同物件呼叫
//         得到不同結果」，是因為每次呼叫傳進去的 this 指標不同，
//         函式透過 this 存取到各自的資料。
//     為什麼會錯：因為語法寫成 p1.print()、p2.print()，看起來像是
//         「函式屬於物件」，就順勢想成每個物件都帶著自己的一份函式。
//         實際上物件裡只有資料；函式屬於「型別」，不屬於「實例」。
//         把 p1.print() 心裡翻譯成 print(&p1)，這個誤解就消失了。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <string>

struct Point {
    int x;
    int y;
    
    // C++ 的 struct 可以有成員函數！
    void print() {
        std::cout << "(" << x << ", " << y << ")" << std::endl;
    }
};

// -----------------------------------------------------------------------------
// 【示範】非虛擬成員函式不佔空間，虛擬函式才會
//   用來實測上面【概念補充】的說法，而不是只用嘴巴講。
// -----------------------------------------------------------------------------
struct PlainData {                 // 只有兩個 int
    int x;
    int y;
};

struct WithMethods {               // 兩個 int + 三個非虛擬成員函式
    int x;
    int y;
    void print() const {}
    int  sum() const { return x + y; }
    void reset() { x = 0; y = 0; }
};

struct WithVirtual {               // 兩個 int + 一個虛擬函式
    int x;
    int y;
    virtual ~WithVirtual() = default;
    virtual void print() const {}
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 155. Min Stack
//   題目：設計一個堆疊，支援 push / pop / top，以及在「常數時間」內
//         取得堆疊中的最小值 getMin()。
//   為什麼用到本主題：這是一道純粹的「設計一個型別」的題目——
//         把資料（兩個堆疊）與行為（push/pop/top/getMin）封裝在同一個
//         struct 裡，正是本檔的主題。在 C 裡你得寫成一堆
//         minstack_push(&s, v) 的自由函式加一個裸露的 struct；
//         在 C++ 則能讓型別自己說明它會做什麼。
//   關鍵想法：主堆疊存所有值，另一個「最小值堆疊」在每次 push 時
//         同步存入「目前為止的最小值」。如此 getMin() 只要看
//         minStack 的頂端，就是 O(1)。
//   複雜度：push / pop / top / getMin 全部 O(1)；空間 O(n)。
// -----------------------------------------------------------------------------
struct MinStack {
    std::vector<int> data;      // 主堆疊
    std::vector<int> minData;   // 與 data 同步成長，記錄「到此為止的最小值」

    void push(int val) {
        data.push_back(val);
        if (minData.empty() || val < minData.back()) {
            minData.push_back(val);        // 新的最小值
        } else {
            minData.push_back(minData.back());  // 沿用舊的最小值
        }
    }

    void pop() {
        if (data.empty()) return;          // 防禦：空堆疊 pop 不做事
        data.pop_back();
        minData.pop_back();                // 兩個堆疊必須同步
    }

    int top() const { return data.back(); }
    int getMin() const { return minData.back(); }
    bool empty() const { return data.empty(); }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】把「資料 + 驗證邏輯」封裝在同一個 struct
//   情境：處理一批感測器讀數，每筆包含裝置 id、溫度、時間戳。
//   在 C 裡，「這筆讀數是否合法」的判斷會散落在各個呼叫點；
//   在 C++ 可以讓型別自己回答 isValid()，規則只寫一次、改一次就全部生效。
// -----------------------------------------------------------------------------
struct SensorReading {
    std::string deviceId;
    double      celsius;
    long        timestamp;

    // 業務規則集中在型別內部：溫度必須在合理範圍，且 id 不可為空
    bool isValid() const {
        return !deviceId.empty() && celsius > -50.0 && celsius < 150.0;
    }

    std::string describe() const {
        return deviceId + " => " + std::to_string(celsius) + "C"
             + (isValid() ? "  [OK]" : "  [REJECTED]");
    }
};

int main() {
    // C++ 不需要 struct 關鍵字
    Point p1;
    p1.x = 10;
    p1.y = 20;
    p1.print();  // 調用成員函數

    std::cout << "\n=== 成員函式佔不佔空間？（本機 g++ 15.2 / x86-64 實測）===" << std::endl;
    std::cout << "  sizeof(PlainData)   = " << sizeof(PlainData)
              << "  (2 個 int)" << std::endl;
    std::cout << "  sizeof(WithMethods) = " << sizeof(WithMethods)
              << "  (2 個 int + 3 個非虛擬成員函式 → 大小不變)" << std::endl;
    std::cout << "  sizeof(WithVirtual) = " << sizeof(WithVirtual)
              << " (2 個 int + 虛擬函式 → 多了一個 vptr)" << std::endl;
    std::cout << "  註：以上為實作定義的數值，非標準保證" << std::endl;

    std::cout << "\n=== LeetCode 155. Min Stack ===" << std::endl;
    {
        MinStack st;
        st.push(-2);
        st.push(0);
        st.push(-3);
        std::cout << "  push(-2), push(0), push(-3)" << std::endl;
        std::cout << "  getMin() = " << st.getMin() << std::endl;   // -3
        st.pop();
        std::cout << "  pop() 之後" << std::endl;
        std::cout << "  top()    = " << st.top() << std::endl;      // 0
        std::cout << "  getMin() = " << st.getMin() << std::endl;   // -2
    }

    std::cout << "\n=== 日常實務：資料與驗證邏輯封裝在一起 ===" << std::endl;
    {
        std::vector<SensorReading> readings = {
            {"sensor-A1", 23.5, 1721000000},
            {"sensor-B2", 999.0, 1721000060},   // 溫度超出合理範圍
            {"",          21.0, 1721000120},    // 缺 deviceId
            {"sensor-C3", -12.25, 1721000180}
        };

        int okCount = 0;
        for (const SensorReading& r : readings) {
            std::cout << "  " << r.describe() << std::endl;
            if (r.isValid()) ++okCount;
        }
        std::cout << "  合法筆數: " << okCount << " / " << readings.size() << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 2 課：C 與 C++ 的關鍵差異16.cpp" -o diff16

// === 預期輸出 ===
// (10, 20)
//
// === 成員函式佔不佔空間？（本機 g++ 15.2 / x86-64 實測）===
//   sizeof(PlainData)   = 8  (2 個 int)
//   sizeof(WithMethods) = 8  (2 個 int + 3 個非虛擬成員函式 → 大小不變)
//   sizeof(WithVirtual) = 16 (2 個 int + 虛擬函式 → 多了一個 vptr)
//   註：以上為實作定義的數值，非標準保證
//
// === LeetCode 155. Min Stack ===
//   push(-2), push(0), push(-3)
//   getMin() = -3
//   pop() 之後
//   top()    = 0
//   getMin() = -2
//
// === 日常實務：資料與驗證邏輯封裝在一起 ===
//   sensor-A1 => 23.500000C  [OK]
//   sensor-B2 => 999.000000C  [REJECTED]
//    => 21.000000C  [REJECTED]
//   sensor-C3 => -12.250000C  [OK]
//   合法筆數: 2 / 4
