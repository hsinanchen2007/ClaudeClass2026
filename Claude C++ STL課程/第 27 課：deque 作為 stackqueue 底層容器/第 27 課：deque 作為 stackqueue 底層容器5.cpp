// =============================================================================
//  第 27 課：deque 作為 stack/queue 底層容器 5  —  queue 經典應用：BFS
// =============================================================================
//
// 【主題資訊 Information】
//   std::queue<int> q;   // 底層預設 deque<int>
//   q.push(v);  q.front();  q.pop();  q.empty();
//
//   標頭檔：<queue>、<vector>
//   複雜度：時間 O(V + E)（每個節點入列一次、每條邊檢查一次）；空間 O(V)
//   本檔標準：C++17
//
// 【詳細解釋 Explanation】
//
// 【1. BFS 為什麼一定是 queue】
//   廣度優先搜尋的定義是「先把距離起點 k 步的節點全部走完，再走 k+1 步的」。
//   要做到這件事，就必須讓「先被發現的節點先被處理」——這正是 FIFO。
//   把 queue 換成 stack（LIFO），演算法立刻變成 DFS（深度優先），
//   走訪順序完全不同。
//   **資料結構的選擇本身就決定了演算法的行為**，這是本檔最重要的一課。
//   本檔第 3 節會實際跑同一份程式碼、只換容器，展示兩種截然不同的順序。
//
// 【2. visited 一定要在「入列時」標記，不是「出列時」】
//   這是 BFS 最常見的 bug。看正確與錯誤的差別：
//       正確：發現鄰居 → 立刻標記 visited → 入列
//       錯誤：發現鄰居 → 入列 → 等它出列時才標記
//   錯誤版本的問題：同一個節點可能在出列前被多個鄰居重複發現，
//   於是被重複入列。結果不只效能變差（佇列膨脹），
//   在稠密圖上還可能導致指數級的重複處理。
//   本檔第 4 節用「入列次數」的計數實測這個差異。
//
// 【3. BFS 的核心保證：最短路徑】
//   在**無權圖**（每條邊權重都是 1）上，BFS 第一次抵達某節點時，
//   走過的邊數必定是最少的。理由：BFS 嚴格按距離分層推進，
//   距離 k 的節點全部處理完才會碰到距離 k+1 的。
//   ⚠️ 這個保證只在無權圖成立。邊有不同權重時要用 Dijkstra
//   （priority_queue，不是 queue）。
//
// 【4. 為什麼不能用 vector 當 queue 的底層】
//   BFS 的主迴圈是「取出最前面的、處理、把鄰居放到最後面」——
//   也就是 front + pop_front + push_back。
//   vector 沒有 pop_front；即使自己實作，那也是 O(n)，
//   會讓整個 BFS 從 O(V+E) 退化成 O(V² + E)。
//   deque 的 pop_front 是攤銷 O(1)，這就是 std::queue 選它當預設底層的原因
//   （同課 2.cpp 已用 detection idiom 在編譯期驗證 vector 確實沒有 pop_front）。
//
// 【概念補充 Concept Deep Dive】
//   ● 分層 BFS：怎麼知道「現在走到第幾層」
//     在主迴圈開頭記下 int levelSize = q.size()，
//     然後只處理這麼多個節點——它們剛好就是同一層的全部節點。
//     這個技巧是所有「求最短步數」「逐層輸出」類題目的標準寫法，
//     本檔第 5 節有完整示範。
//
//   ● BFS vs DFS 的空間特性
//     BFS 的佇列最大長度是「最寬那一層的節點數」，
//     在寬而淺的圖上可能非常大；
//     DFS 的堆疊深度是「最長路徑長度」，在深而窄的圖上才會爆。
//     兩者最壞都是 O(V)，但實際峰值取決於圖的形狀。
//
//   ● queue 只能看兩端，這裡剛好夠用
//     BFS 全程只需要 front()、push()、pop()、empty()，
//     完全不需要遍歷或隨機存取——所以用 queue（而非直接用 deque）
//     不但沒有損失，還向讀者宣告了「這裡只有 FIFO 行為」。
//
// 【注意事項 Pay Attention】
//   1. visited 必須在**入列時**標記，不是出列時，否則節點會重複入列。
//   2. 起點也要標記 visited 並入列，否則可能被自己的鄰居再次加入。
//   3. 對空 queue 呼叫 front() / pop() 是 UB；主迴圈的 while (!q.empty())
//      已經保證了這一點，但若在迴圈內另外 pop 就要重新檢查。
//   4. BFS 的最短路徑保證只適用於**無權圖**；有權重請用 Dijkstra。
//   5. 圖若可能不連通，單次 BFS 只會走訪起點所在的連通分量。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】BFS 與 queue
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. BFS 為什麼用 queue？把 queue 換成 stack 會發生什麼事？
//     答：BFS 要求「先發現的節點先處理」，才能保證按距離分層推進——這就是 FIFO。
//         換成 stack（LIFO）之後，會變成一路往深處走、回頭再走另一條，
//         也就是 **DFS**。程式碼幾乎一模一樣，只因容器不同就變成另一個演算法。
//     追問：那 BFS 的最短路徑保證還在嗎？→ 不在了。
//         DFS 第一次抵達某節點時，路徑不保證最短。
//         最短路徑的保證來自「按層推進」，而按層推進來自 FIFO。
//
// 🔥 Q2. visited 應該在什麼時候標記？入列時還是出列時？
//     答：**入列時**。若等到出列才標記，同一個節點可能在出列前被多個鄰居
//         重複發現、重複入列，佇列會膨脹、同一節點被處理多次。
//         在稠密圖上這會嚴重影響效能，甚至改變複雜度。
//     追問：起點需要標記嗎？→ 需要。否則起點的鄰居在展開時
//         會把起點當成未訪問的節點再次加入佇列。
//
// ⚠️ 陷阱. 「BFS 找到的路徑一定是最短的」——這句話在什麼情況下是錯的？
//     答：在**有權圖**上就是錯的。BFS 的最短保證來自「每走一步代價相同」，
//         所以按層推進等價於按距離推進。
//         一旦邊有不同權重，「邊數最少」就不等於「總權重最小」——
//         走 5 條權重 1 的邊（總和 5）比走 1 條權重 100 的邊更短，
//         但 BFS 會先找到後者。此時要用 Dijkstra（priority_queue）。
//     為什麼會錯：把「BFS 找最短路」當成無條件的定理背下來，
//         而沒有記住它的前提——「無權圖，或所有邊權重相等」。
//         面試時只要追問一句「邊有權重呢」，就能區分是理解還是背誦。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <queue>
#include <stack>
#include <vector>
#include <string>
using namespace std;

// 簡單的圖遍歷（BFS）
void bfs(const vector<vector<int>>& graph, int start) {
    vector<bool> visited(graph.size(), false);
    queue<int> q;   // 底層是 deque<int>

    visited[start] = true;      // ★ 起點也要標記，否則會被鄰居再次加入
    q.push(start);

    cout << "BFS 順序：";
    while (!q.empty()) {
        int node = q.front();   // 取出最早加入的節點
        q.pop();
        cout << node << " ";

        for (int neighbor : graph[node]) {
            if (!visited[neighbor]) {
                visited[neighbor] = true;   // ★ 入列時就標記，不是出列時
                q.push(neighbor);
            }
        }
    }
    cout << endl;
}

// 對照組：只把 queue 換成 stack，演算法立刻變成 DFS
void dfsWithStack(const vector<vector<int>>& graph, int start) {
    vector<bool> visited(graph.size(), false);
    stack<int> s;               // 唯一的差別就在這一行

    visited[start] = true;
    s.push(start);

    cout << "DFS 順序：";
    while (!s.empty()) {
        int node = s.top();
        s.pop();
        cout << node << " ";

        for (int neighbor : graph[node]) {
            if (!visited[neighbor]) {
                visited[neighbor] = true;
                s.push(neighbor);
            }
        }
    }
    cout << endl;
}

// 錯誤示範：在「出列時」才標記 visited → 節點會重複入列
// 回傳總入列次數，用來量化這個 bug 的代價
long bfsWrongMarking(const vector<vector<int>>& graph, int start) {
    vector<bool> visited(graph.size(), false);
    queue<int> q;
    q.push(start);
    long enqueued = 1;

    while (!q.empty()) {
        int node = q.front();
        q.pop();
        if (visited[node]) continue;    // 只好在這裡補救，但已經多入列了
        visited[node] = true;           // ✗ 太晚了

        for (int neighbor : graph[node]) {
            if (!visited[neighbor]) {
                q.push(neighbor);       // 同一個節點可能被多個鄰居重複推入
                ++enqueued;
            }
        }
    }
    return enqueued;
}

// 正確版本：入列時標記，回傳總入列次數
long bfsCorrectMarking(const vector<vector<int>>& graph, int start) {
    vector<bool> visited(graph.size(), false);
    queue<int> q;
    visited[start] = true;
    q.push(start);
    long enqueued = 1;

    while (!q.empty()) {
        int node = q.front();
        q.pop();
        for (int neighbor : graph[node]) {
            if (!visited[neighbor]) {
                visited[neighbor] = true;   // ✓ 入列前就標記
                q.push(neighbor);
                ++enqueued;
            }
        }
    }
    return enqueued;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】社群網路的「共同好友距離」與人脈分層
//   情境：社群平台要顯示「你和某人相隔幾層關係」（1 層 = 直接好友、
//         2 層 = 朋友的朋友），並列出每一層各有多少人。
//         這是 BFS 分層走訪的直接應用——而且必須是 BFS，
//         因為我們要的是「最少幾層」，DFS 找到的路徑不保證最短。
//   關鍵技巧：在迴圈開頭記下 q.size()，那就是「這一層的全部人數」。
// -----------------------------------------------------------------------------
struct SocialResult {
    int distance = -1;                  // -1 代表不可達
    vector<int> layerSizes;             // 每一層的人數
};

SocialResult socialDistance(const vector<vector<int>>& friends,
                            int me, int target) {
    SocialResult r;
    vector<bool> visited(friends.size(), false);
    queue<int> q;
    visited[me] = true;
    q.push(me);

    int level = 0;
    while (!q.empty()) {
        int levelSize = static_cast<int>(q.size());   // ★ 這一層有幾個人
        r.layerSizes.push_back(levelSize);

        for (int i = 0; i < levelSize; ++i) {
            int person = q.front();
            q.pop();
            if (person == target) {
                r.distance = level;
                return r;
            }
            for (int f : friends[person]) {
                if (!visited[f]) {
                    visited[f] = true;
                    q.push(f);
                }
            }
        }
        ++level;
    }
    return r;                            // 走完仍沒找到 → distance 維持 -1
}

// 註：本檔不附 LeetCode 範例。BFS 相關的 LeetCode 題目（如 994 Rotting Oranges、
//     1091 Shortest Path in Binary Matrix）都不在本次指定的題庫範圍內，
//     而題庫內的題目沒有一題真正用得上 BFS；
//     硬掛一題不相關的只會模糊「queue 決定了演算法行為」這個重點。

int main() {
    //     0 --- 1 --- 3
    //     |     |
    //     2 --- 4
    vector<vector<int>> graph = {
        {1, 2},     // 0 的鄰居
        {0, 3, 4},  // 1 的鄰居
        {0, 4},     // 2 的鄰居
        {1},        // 3 的鄰居
        {1, 2}      // 4 的鄰居
    };

    cout << "=== 1. BFS 基本走訪 ===" << endl;
    cout << "圖的結構：" << endl;
    cout << "     0 --- 1 --- 3" << endl;
    cout << "     |     |" << endl;
    cout << "     2 --- 4" << endl;
    bfs(graph, 0);
    // BFS 順序：0 1 2 3 4

    cout << "\n=== 2. 同一份程式碼，只換容器就變成另一個演算法 ===" << endl;
    bfs(graph, 0);
    dfsWithStack(graph, 0);
    cout << "→ 兩個函式的差別只有一行：queue<int> vs stack<int>。" << endl;
    cout << "  FIFO 讓它按距離分層推進（BFS）；" << endl;
    cout << "  LIFO 讓它一路往深處走（DFS）。" << endl;
    cout << "  **資料結構的選擇本身就決定了演算法的行為。**" << endl;

    cout << "\n=== 3. BFS 的分層性質 ===" << endl;
    cout << "從節點 0 出發：" << endl;
    cout << "  第 0 層（距離 0）：0" << endl;
    cout << "  第 1 層（距離 1）：1, 2" << endl;
    cout << "  第 2 層（距離 2）：3, 4" << endl;
    cout << "→ BFS 保證距離 k 的節點全部處理完，才會碰到距離 k+1 的，" << endl;
    cout << "  這就是「第一次抵達即最短路徑」的來源（僅限無權圖）。" << endl;

    cout << "\n=== 4. 實測：visited 標記時機造成的重複入列 ===" << endl;
    // 造一個較稠密的圖來放大差異：完全圖 K8
    const int N = 8;
    vector<vector<int>> dense(N);
    for (int i = 0; i < N; ++i)
        for (int j = 0; j < N; ++j)
            if (i != j) dense[i].push_back(j);

    long correct = bfsCorrectMarking(dense, 0);
    long wrong   = bfsWrongMarking(dense, 0);
    cout << "在 " << N << " 個節點的完全圖上：" << endl;
    cout << "  入列時標記（正確）：總入列 " << correct << " 次" << endl;
    cout << "  出列時標記（錯誤）：總入列 " << wrong << " 次" << endl;
    cout << "→ 正確版每個節點剛好入列一次（" << correct << " = 節點數）；" << endl;
    cout << "  錯誤版多入列 " << (wrong - correct) << " 次，圖越稠密差距越大。" << endl;
    cout << "（這組數字每次執行完全相同，可重現）" << endl;

    cout << "\n=== 5. 分層 BFS：怎麼知道走到第幾層 ===" << endl;
    cout << "技巧：在迴圈開頭記下 q.size()，那就是「這一層的全部節點數」。" << endl;
    cout << "下面的實務範例就用這個技巧算出人脈距離與每層人數。" << endl;

    cout << "\n=== 日常實務：社群網路的人脈距離 ===" << endl;
    // 0=我, 1..3=直接好友, 4..6=朋友的朋友, 7=更遠, 8=孤立節點
    vector<vector<int>> friends = {
        {1, 2, 3},      // 0 我
        {0, 4},         // 1
        {0, 4, 5},      // 2
        {0, 6},         // 3
        {1, 2, 7},      // 4
        {2},            // 5
        {3, 7},         // 6
        {4, 6},         // 7
        {}              // 8 孤立（沒有任何好友）
    };

    struct { int target; const char* label; } queries[] = {
        {3, "直接好友"}, {5, "朋友的朋友"}, {7, "三層外"}, {8, "孤立節點"}
    };
    for (auto& qy : queries) {
        SocialResult r = socialDistance(friends, 0, qy.target);
        cout << "  我 → 使用者 " << qy.target << "（" << qy.label << "）：";
        if (r.distance < 0) {
            cout << "不可達（不在同一個人脈網路）" << endl;
        } else {
            cout << "相隔 " << r.distance << " 層";
            cout << "，各層人數 ";
            for (size_t i = 0; i < r.layerSizes.size(); ++i) {
                cout << "[第" << i << "層:" << r.layerSizes[i] << "人]";
            }
            cout << endl;
        }
    }
    cout << "→ 必須用 BFS 而非 DFS：我們要的是「最少幾層」，" << endl;
    cout << "  DFS 找到的路徑不保證最短。" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 27 課：deque 作為 stackqueue 底層容器5.cpp" -o demo27_5

// === 預期輸出 ===
// === 1. BFS 基本走訪 ===
// 圖的結構：
//      0 --- 1 --- 3
//      |     |
//      2 --- 4
// BFS 順序：0 1 2 3 4
//
// === 2. 同一份程式碼，只換容器就變成另一個演算法 ===
// BFS 順序：0 1 2 3 4
// DFS 順序：0 2 4 1 3
// → 兩個函式的差別只有一行：queue<int> vs stack<int>。
//   FIFO 讓它按距離分層推進（BFS）；
//   LIFO 讓它一路往深處走（DFS）。
//   **資料結構的選擇本身就決定了演算法的行為。**
//
// === 3. BFS 的分層性質 ===
// 從節點 0 出發：
//   第 0 層（距離 0）：0
//   第 1 層（距離 1）：1, 2
//   第 2 層（距離 2）：3, 4
// → BFS 保證距離 k 的節點全部處理完，才會碰到距離 k+1 的，
//   這就是「第一次抵達即最短路徑」的來源（僅限無權圖）。
//
// === 4. 實測：visited 標記時機造成的重複入列 ===
// 在 8 個節點的完全圖上：
//   入列時標記（正確）：總入列 8 次
//   出列時標記（錯誤）：總入列 29 次
// → 正確版每個節點剛好入列一次（8 = 節點數）；
//   錯誤版多入列 21 次，圖越稠密差距越大。
// （這組數字每次執行完全相同，可重現）
//
// === 5. 分層 BFS：怎麼知道走到第幾層 ===
// 技巧：在迴圈開頭記下 q.size()，那就是「這一層的全部節點數」。
// 下面的實務範例就用這個技巧算出人脈距離與每層人數。
//
// === 日常實務：社群網路的人脈距離 ===
//   我 → 使用者 3（直接好友）：相隔 1 層，各層人數 [第0層:1人][第1層:3人]
//   我 → 使用者 5（朋友的朋友）：相隔 2 層，各層人數 [第0層:1人][第1層:3人][第2層:3人]
//   我 → 使用者 7（三層外）：相隔 3 層，各層人數 [第0層:1人][第1層:3人][第2層:3人][第3層:1人]
//   我 → 使用者 8（孤立節點）：不可達（不在同一個人脈網路）
// → 必須用 BFS 而非 DFS：我們要的是「最少幾層」，
//   DFS 找到的路徑不保證最短。
