// =============================================================================
//  第 15 課：vector 元素刪除 3  —  erase(pos)：刪除指定位置
// =============================================================================
//
// 【主題資訊 Information】
//   iterator erase(const_iterator pos);                 // 刪單一元素
//   iterator erase(const_iterator first, const_iterator last);  // 刪範圍（見第 5 檔）
//   標頭檔：<vector>
//   標準版本：C++98。C++11 起參數型別由 iterator 改為 const_iterator。
//   複雜度：O(n)。被刪位置【之後】的每個元素都要往前移動一格。
//     精確地說是「size() - index(pos) - 1」次移動賦值，加一次解構。
//   回傳：指向「被刪元素之後那個元素」的 iterator。
//         刪的是最後一個時回傳 end()。
//   前置條件：pos 必須是【可解參考的】合法迭代器（不能是 end()）。
//   失效範圍：pos 及其之後的所有 iterator/reference/pointer。
//             pos 之前的完全不受影響。capacity 不變。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼是 O(n)：連續記憶體不能有洞】
//   vector 的核心保證是「元素連續存放」，operator[] 才能用
//   `*(data() + i)` 一次算出位址。若刪除中間元素時留下一個洞，
//   這個保證就破了。所以 erase 必須把後面的元素整批往前搬：
//       刪除索引 1：
//       [A][B][C][D]  →  [A][C][D][ ]  →  size 減一
//            ↑            ←──搬移──
//   搬移次數是「被刪位置之後的元素個數」。
//   所以刪第一個元素最貴（搬 n-1 個），刪最後一個最便宜（搬 0 個）。
//
// 【2. 搬移用的是「移動賦值」而不是複製】
//   C++11 起，erase 內部用 std::move 把後面的元素往前搬：
//       *it = std::move(*(it + 1));
//   對 std::string、std::vector 這類元素，這代表只搬指標、不深複製，
//   成本遠低於複製。但注意這要求元素【可移動賦值】——
//   若你的型別把 operator=(T&&) 刪掉了，erase 會退回複製賦值；
//   若連複製賦值都沒有，就編譯失敗。
//
// 【3. 刪除最後一個元素：erase(end()-1) vs pop_back()】
//   兩者結果相同，成本也相同（都不必搬移任何元素）。
//   差別只在表達力：
//       v.pop_back();          明說「拿掉最後一個」
//       v.erase(v.end() - 1);  要讀者自己在腦中算出那是最後一個
//   另外 erase(end()-1) 對空容器是 UB，因為 end()-1 已經越界。
//   結論：要刪最後一個就用 pop_back。
//
// 【4. erase(pos) 不能傳 end()】
//   end() 不指向任何元素，對它 erase 是未定義行為。
//   這個錯誤最常見的形態是：
//       auto it = std::find(v.begin(), v.end(), target);
//       v.erase(it);        // 危險！find 找不到時 it == end()
//   正確寫法一定要先判斷：
//       if (it != v.end()) v.erase(it);
//   本檔的實務範例正是在示範這個 pattern。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼 C++11 把參數改成 const_iterator
//     C++98 的簽章是 erase(iterator)，於是你不能拿一個 const_iterator
//     去 erase——即使你只是「用它指出位置」，並不打算透過它修改元素。
//     這造成 `v.erase(v.cbegin())` 在 C++98 編譯不過。
//     C++11 改成 const_iterator 之後，指出位置與修改權限就分開了：
//     erase 的修改權限來自它是 v 的成員函式，不是來自迭代器。
//
// (B) 刪除多個元素時，不要在迴圈裡逐一 erase
//     `for (...) if (cond) v.erase(it);` 是 O(n²)——
//     每次 erase 都搬移一次後半段。
//     刪 n/2 個元素就是 n/2 次 O(n) 的搬移。
//     正確做法是 erase-remove 慣用法（見第 10 檔），
//     它只掃一次、每個元素最多搬一次，總共 O(n)。
//     本課第 12 檔有兩者的實測對照。
//
// (C) erase 的例外安全保證
//     標準規定：若元素的移動賦值運算子與解構子都不拋例外，
//     erase 不會拋出任何例外。
//     若移動賦值可能拋例外，那麼在搬移途中拋出時，
//     erase 只提供【基本保證】（容器仍然有效，但內容未指定），
//     不是強保證。這又是一個「移動操作應該 noexcept」的理由。
//
// 【注意事項 Pay Attention】
//   1. erase(pos) 是 O(n)，不是 O(1)。刪第一個元素最貴。
//   2. pos 不能是 end()。搭配 std::find 時務必先檢查 it != end()。
//   3. erase 之後，pos 及其後的所有 iterator/reference/pointer 全部失效。
//      不要沿用舊迭代器繼續操作——要用【回傳值】（見第 4 檔）。
//   4. capacity 不變，記憶體不會歸還。
//   5. 迴圈中逐一 erase 是 O(n²)，批次刪除請用 erase-remove 慣用法。
//   6. 刪最後一個請用 pop_back()，語意更清楚。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】vector::erase(pos)
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. vector 的 erase(pos) 複雜度是多少？為什麼不是 O(1)？
//     答：O(n)。因為 vector 保證元素連續存放（operator[] 才能靠
//         `data() + i` 一次算出位址），刪除中間元素不能留洞，
//         必須把後面所有元素往前搬一格。
//         搬移次數 = 被刪位置之後的元素個數，所以刪第一個最貴
//         （搬 n-1 個）、刪最後一個免費（搬 0 個）。
//     追問：那 std::list 的 erase 為什麼是 O(1)？
//         → list 是節點式的，刪除只要改前後節點的指標，
//           不必搬動任何資料。代價是它沒有隨機存取、
//           而且每個元素多兩個指標的記憶體開銷、cache 局部性也差。
//
// 🔥 Q2. `v.erase(std::find(v.begin(), v.end(), x));` 這行有什麼問題？
//     答：find 找不到 x 時會回傳 v.end()，而 erase(end()) 是未定義行為
//         （end() 不指向任何元素）。
//         必須先檢查：
//             auto it = std::find(v.begin(), v.end(), x);
//             if (it != v.end()) v.erase(it);
//     追問：如果要刪除【所有】等於 x 的元素呢？
//         → 別用迴圈逐一 find + erase（那是 O(n²)）。
//           用 erase-remove 慣用法：
//           v.erase(std::remove(v.begin(), v.end(), x), v.end());
//           C++20 起可直接寫 std::erase(v, x)。
//
// ⚠️ 陷阱. 「erase 之後那個位置的元素往前補上了，所以我手上的迭代器
//         現在自然指向下一個元素，可以繼續用」——這個推論錯在哪？
//     答：錯在把「觀察到的行為」當成「標準的保證」。
//         標準明訂 erase 會使 pos 及其之後的所有迭代器失效。
//         實務上對 vector 而言，那個迭代器的【位址】確實還在容器內、
//         而且剛好指向補位過來的元素——所以它「看起來能用」。
//         但這是實作細節，不是契約：
//           * 換成 debug mode（-D_GLIBCXX_DEBUG）會直接 abort
//           * 換成別的容器（deque）行為完全不同
//           * 若這次 erase 剛好刪到最後一個，迭代器就等於 end()，
//             再解參考就是實實在在的越界
//         正確做法是使用 erase 的【回傳值】：it = v.erase(it);
//     為什麼會錯：把「迭代器」想成單純的指標，於是推論「記憶體位置沒變
//         所以還能用」。但迭代器是一個抽象契約，失效與否由標準規定，
//         不由記憶體佈局決定。依賴未定義行為寫出來的程式，
//         會在換編譯器、開最佳化、或開除錯模式時突然壞掉。
// ═══════════════════════════════════════════════════════════════════════════

#include <vector>
#include <iostream>
#include <algorithm>
#include <string>

// -----------------------------------------------------------------------------
// 計數用的輔助型別：記錄「賦值」被呼叫幾次，用來實測 erase 的搬移成本。
// （必須定義在檔案層級：區域類別不能有 static 資料成員。）
// -----------------------------------------------------------------------------
struct Tracer {
    int value;
    static int moves;
    Tracer(int v = 0) : value(v) {}
    Tracer(const Tracer&) = default;
    Tracer(Tracer&&) noexcept = default;
    Tracer& operator=(const Tracer& o) { value = o.value; ++moves; return *this; }
    Tracer& operator=(Tracer&& o) noexcept { value = o.value; ++moves; return *this; }
};
int Tracer::moves = 0;

// -----------------------------------------------------------------------------
// 【日常實務範例】連線管理：移除一個已斷線的 client
//   情境：伺服器維護一份目前連線的 client 清單。某個 client 斷線時，
//         要把它從清單中移除。
//   為什麼用到本主題：這是 find + erase 最典型的樣子，
//     也是「忘記檢查 end()」這個 bug 最常出現的地方——
//     斷線通知可能重複送達，第二次就找不到了。
//   複雜度提醒：單次移除是 O(n)。若斷線很頻繁且清單很大，
//     應該改用 unordered_map，或用 swap-and-pop（見第 13 檔）
//     ——前提是不需要維持順序。
// -----------------------------------------------------------------------------
struct Connection {
    int         fd;
    std::string peer;
};

// 回傳 true 表示真的移除了某筆
bool removeConnection(std::vector<Connection>& conns, int fd) {
    auto it = std::find_if(conns.begin(), conns.end(),
                           [fd](const Connection& c) { return c.fd == fd; });
    if (it == conns.end()) {
        return false;            // 關鍵：找不到就不能 erase（那會是 UB）
    }
    conns.erase(it);
    return true;
}

int main() {
    std::vector<int> v = {10, 20, 30, 40, 50};

    std::cout << "=== 原始示範：erase(pos) ===\n";
    std::cout << "初始: ";
    for (int x : v) std::cout << x << " ";
    std::cout << "  (capacity=" << v.capacity() << ")\n";

    // 刪除第一個元素
    v.erase(v.begin());
    std::cout << "刪除第一個: ";
    for (int x : v) std::cout << x << " ";  // 20 30 40 50
    std::cout << std::endl;

    // 刪除索引 2 的元素（現在是 40）
    v.erase(v.begin() + 2);
    std::cout << "刪除索引 2: ";
    for (int x : v) std::cout << x << " ";  // 20 30 50
    std::cout << std::endl;

    // 刪除最後一個元素（等同 pop_back）
    v.erase(v.end() - 1);
    std::cout << "刪除最後: ";
    for (int x : v) std::cout << x << " ";  // 20 30
    std::cout << std::endl;
    std::cout << "capacity 全程不變: " << v.capacity() << "\n";

    std::cout << "\n=== 搬移次數：刪第一個最貴、刪最後一個免費 ===\n";
    {
        // 用一個會計數移動賦值的型別（定義在檔案層級，見上方 Tracer），
        // 把「搬了幾次」實際量出來
        auto countMoves = [](size_t eraseIndex) {
            std::vector<Tracer> t;
            t.reserve(10);
            for (int i = 0; i < 10; ++i) t.emplace_back(i);
            Tracer::moves = 0;
            t.erase(t.begin() + static_cast<long>(eraseIndex));
            return Tracer::moves;
        };

        std::cout << "10 個元素的 vector：\n";
        std::cout << "  erase(begin()+0) 搬移次數: " << countMoves(0) << "（最貴）\n";
        std::cout << "  erase(begin()+5) 搬移次數: " << countMoves(5) << "\n";
        std::cout << "  erase(begin()+9) 搬移次數: " << countMoves(9) << "（最後一個，免費）\n";
        std::cout << "  規律：搬移次數 = size() - 索引 - 1\n";
    }

    std::cout << "\n=== erase(end()) 是未定義行為 ===\n";
    {
        std::vector<int> w = {1, 2, 3};
        auto it = std::find(w.begin(), w.end(), 99);   // 找一個不存在的值
        std::cout << "std::find 找 99 -> "
                  << (it == w.end() ? "回傳 end()" : "找到了") << "\n";
        std::cout << "此時若直接 w.erase(it) 就是 erase(end()) → 未定義行為\n";

        // 正確寫法
        if (it != w.end()) {
            w.erase(it);
            std::cout << "刪除成功\n";
        } else {
            std::cout << "先檢查 it != end() → 不刪除（正確）\n";
        }

        // 找得到的情況
        auto it2 = std::find(w.begin(), w.end(), 2);
        if (it2 != w.end()) {
            w.erase(it2);
            std::cout << "找到 2 並刪除，剩下: ";
            for (int x : w) std::cout << x << " ";
            std::cout << "\n";
        }
    }

    std::cout << "\n=== 刪最後一個：pop_back 比 erase(end()-1) 清楚 ===\n";
    {
        std::vector<int> a = {1, 2, 3};
        std::vector<int> b = {1, 2, 3};
        a.pop_back();
        b.erase(b.end() - 1);
        std::cout << "pop_back()       後: ";
        for (int x : a) std::cout << x << " ";
        std::cout << "\n";
        std::cout << "erase(end() - 1) 後: ";
        for (int x : b) std::cout << x << " ";
        std::cout << "\n";
        std::cout << "結果與成本都相同，但 pop_back 直接說出意圖。\n";
        std::cout << "（另外 erase(end()-1) 對空容器是 UB，因為 end()-1 已越界）\n";
    }

    std::cout << "\n=== 日常實務：移除斷線的 client ===\n";
    {
        std::vector<Connection> conns = {
            {101, "10.0.0.5:44321"},
            {102, "10.0.0.9:51002"},
            {103, "192.168.1.4:8080"},
            {104, "10.0.0.5:44999"},
        };
        std::cout << "目前連線數: " << conns.size() << "\n";

        std::cout << "fd=102 斷線 -> "
                  << (removeConnection(conns, 102) ? "已移除" : "找不到") << "\n";
        std::cout << "剩餘連線: ";
        for (const auto& c : conns) std::cout << c.fd << " ";
        std::cout << "\n";

        // 關鍵情境：重複的斷線通知
        std::cout << "fd=102 斷線通知重複送達 -> "
                  << (removeConnection(conns, 102) ? "已移除" : "找不到（安全回傳 false）")
                  << "\n";
        std::cout << "→ 若沒有 it == end() 的檢查，這裡就是 erase(end()) 的 UB。\n";
        std::cout << "  而且「重複的斷線通知」在真實網路程式中很常見。\n";

        std::cout << "最終連線: ";
        for (const auto& c : conns) std::cout << c.fd << "(" << c.peer << ") ";
        std::cout << "\n";
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第 15 課：vector 元素刪除3.cpp -o demo3

// === 預期輸出 ===
// === 原始示範：erase(pos) ===
// 初始: 10 20 30 40 50   (capacity=5)
// 刪除第一個: 20 30 40 50
// 刪除索引 2: 20 30 50
// 刪除最後: 20 30
// capacity 全程不變: 5
//
// === 搬移次數：刪第一個最貴、刪最後一個免費 ===
// 10 個元素的 vector：
//   erase(begin()+0) 搬移次數: 9（最貴）
//   erase(begin()+5) 搬移次數: 4
//   erase(begin()+9) 搬移次數: 0（最後一個，免費）
//   規律：搬移次數 = size() - 索引 - 1
//
// === erase(end()) 是未定義行為 ===
// std::find 找 99 -> 回傳 end()
// 此時若直接 w.erase(it) 就是 erase(end()) → 未定義行為
// 先檢查 it != end() → 不刪除（正確）
// 找到 2 並刪除，剩下: 1 3
//
// === 刪最後一個：pop_back 比 erase(end()-1) 清楚 ===
// pop_back()       後: 1 2
// erase(end() - 1) 後: 1 2
// 結果與成本都相同，但 pop_back 直接說出意圖。
// （另外 erase(end()-1) 對空容器是 UB，因為 end()-1 已越界）
//
// === 日常實務：移除斷線的 client ===
// 目前連線數: 4
// fd=102 斷線 -> 已移除
// 剩餘連線: 101 103 104
// fd=102 斷線通知重複送達 -> 找不到（安全回傳 false）
// → 若沒有 it == end() 的檢查，這裡就是 erase(end()) 的 UB。
//   而且「重複的斷線通知」在真實網路程式中很常見。
// 最終連線: 101(10.0.0.5:44321) 103(192.168.1.4:8080) 104(10.0.0.5:44999)
