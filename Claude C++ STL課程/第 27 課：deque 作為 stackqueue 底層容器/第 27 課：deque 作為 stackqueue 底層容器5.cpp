#include <iostream>
#include <queue>
#include <vector>
using namespace std;

// 簡單的圖遍歷（BFS）
void bfs(const vector<vector<int>>& graph, int start) {
    vector<bool> visited(graph.size(), false);
    queue<int> q;   // 底層是 deque<int>

    visited[start] = true;
    q.push(start);

    cout << "BFS 順序：";
    while (!q.empty()) {
        int node = q.front();   // 取出最早加入的節點
        q.pop();
        cout << node << " ";

        for (int neighbor : graph[node]) {
            if (!visited[neighbor]) {
                visited[neighbor] = true;
                q.push(neighbor);
            }
        }
    }
    cout << endl;
}

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

    bfs(graph, 0);
    // BFS 順序：0 1 2 3 4

    return 0;
}
