/*
 * ================================================================
 * 【第 20 課：vector 效能分析與最佳實踐】總複習 summary.cpp
 * ================================================================
 * 編譯方式：g++ -std=c++17 -o summary summary.cpp
 *
 * 本課重點：
 * 1. 效能測量：用 chrono 計時
 * 2. reserve() 的重要性 —— 避免多次重分配
 * 3. emplace_back vs push_back 效能比較
 * 4. 避免不必要的複製：const 參考、移動語意
 * 5. 迭代遍歷的效能：range-for vs index vs iterator
 * 6. 縮減記憶體：shrink_to_fit()、swap trick
 * 7. 選擇正確的容器
 * ================================================================
 */

#include <iostream>
#include <vector>
#include <chrono>
#include <string>
#include <algorithm>
#include <numeric>
using namespace std;

// 簡單計時工具（RAII 風格）
struct Timer {
    string label;
    chrono::high_resolution_clock::time_point start;

    Timer(const string& l) : label(l), start(chrono::high_resolution_clock::now()) {}

    ~Timer() {
        auto end = chrono::high_resolution_clock::now();
        auto us = chrono::duration_cast<chrono::microseconds>(end - start).count();
        cout << "  [" << label << "]: " << us << " μs" << endl;
    }
};

// ================================================================
// 重點一：reserve() 的重要性
// ================================================================
// vector 在 push_back 超過 capacity 時會：
//   1. 分配新的（通常 2 倍）記憶體
//   2. 將所有元素移動到新位置
//   3. 釋放舊記憶體
// 若事先知道大概元素數量，reserve() 可避免重複重分配

void demoReserve() {
    cout << "\n【reserve() 效能比較】" << endl;

    const int N = 1'000'000;

    {
        Timer t("不用 reserve");
        vector<int> v;
        for (int i = 0; i < N; ++i) v.push_back(i);
    }

    {
        Timer t("reserve(N) 後 push_back");
        vector<int> v;
        v.reserve(N);  // 一次預留所有空間
        for (int i = 0; i < N; ++i) v.push_back(i);
    }

    {
        Timer t("直接用 resize + 索引賦值");
        vector<int> v(N);  // 預先建構 N 個預設值
        for (int i = 0; i < N; ++i) v[i] = i;
    }

    // 觀察 capacity 增長規律
    cout << "\n  capacity 增長規律（前 5 次重分配）:" << endl;
    vector<int> v;
    size_t lastCap = 0;
    int count = 0;
    for (int i = 0; i < 100; ++i) {
        v.push_back(i);
        if (v.capacity() != lastCap) {
            cout << "  size=" << v.size() << " -> capacity=" << v.capacity() << endl;
            lastCap = v.capacity();
            if (++count >= 5) break;
        }
    }
    cout << "  （通常每次翻倍增長）" << endl;
}

// ================================================================
// 重點二：push_back vs emplace_back 效能
// ================================================================
// 對於複雜物件，emplace_back 就地構造，省去一次移動/複製
// 對於內建類型（int、double），幾乎無差別

struct HeavyObject {
    string name;
    int value;
    vector<int> data;

    // 建構函數
    HeavyObject(const string& n, int v) : name(n), value(v), data(100, v) {}
};

void demoPushVsEmplace() {
    cout << "\n【push_back vs emplace_back 效能】" << endl;

    const int N = 10000;

    {
        Timer t("push_back（臨時物件 + 移動）");
        vector<HeavyObject> v;
        v.reserve(N);
        for (int i = 0; i < N; ++i) {
            v.push_back(HeavyObject("item", i));  // 構造臨時物件，再移動
        }
    }

    {
        Timer t("emplace_back（就地構造）");
        vector<HeavyObject> v;
        v.reserve(N);
        for (int i = 0; i < N; ++i) {
            v.emplace_back("item", i);  // 直接就地構造，無臨時物件
        }
    }
}

// ================================================================
// 重點三：遍歷方式效能比較
// ================================================================
// 對於 vector，各種遍歷方式效能相近（都是順序存取）
// 但語意清晰的方式是首選

void demoTraversal() {
    cout << "\n【遍歷方式比較】" << endl;

    const int N = 10'000'000;
    vector<int> v(N);
    iota(v.begin(), v.end(), 0);

    long long sum = 0;

    {
        Timer t("range-based for");
        sum = 0;
        for (int x : v) sum += x;
    }
    cout << "  sum = " << sum << endl;

    {
        Timer t("index-based for");
        sum = 0;
        for (size_t i = 0; i < v.size(); ++i) sum += v[i];
    }
    cout << "  sum = " << sum << endl;

    {
        Timer t("iterator-based for");
        sum = 0;
        for (auto it = v.begin(); it != v.end(); ++it) sum += *it;
    }
    cout << "  sum = " << sum << endl;

    {
        Timer t("std::accumulate");
        sum = accumulate(v.begin(), v.end(), 0LL);
    }
    cout << "  sum = " << sum << endl;
}

// ================================================================
// 重點四：縮減記憶體 —— shrink_to_fit() 與 swap trick
// ================================================================
// clear() 只清空元素，不釋放記憶體（capacity 不變）
// shrink_to_fit() 要求釋放多餘記憶體（非強制，但通常有效）
// swap trick（C++11 前）：強制釋放記憶體

void demoShrink() {
    cout << "\n【縮減記憶體】" << endl;

    vector<int> v;
    v.reserve(1000);
    for (int i = 0; i < 100; ++i) v.push_back(i);

    cout << "初始 size=" << v.size() << " capacity=" << v.capacity() << endl;

    // clear() 只刪元素，不釋放記憶體
    v.clear();
    cout << "clear() 後 size=" << v.size() << " capacity=" << v.capacity() << endl;

    // shrink_to_fit() 釋放多餘記憶體
    v.shrink_to_fit();
    cout << "shrink_to_fit() 後 size=" << v.size() << " capacity=" << v.capacity() << endl;

    // swap trick（強制釋放，C++11 前的做法）
    vector<int> v2;
    v2.reserve(1000);
    for (int i = 0; i < 100; ++i) v2.push_back(i);
    cout << "\nswap 前 size=" << v2.size() << " capacity=" << v2.capacity() << endl;

    { vector<int>(v2).swap(v2); }  // 建立一個剛好大小的副本，再 swap
    cout << "swap trick 後 size=" << v2.size() << " capacity=" << v2.capacity() << endl;
}

// ================================================================
// 重點五：避免不必要的複製
// ================================================================
// 傳遞 vector 時：
//   - 唯讀：const vector<T>&（參考，零複製）
//   - 需要修改：vector<T>&（參考）
//   - 取得所有權：vector<T>&&（移動）或 vector<T>（值傳遞+移動）

void readVector(const vector<int>& v) {    // 唯讀，零複製
    cout << "  讀取 vector，size=" << v.size() << endl;
}

void modifyVector(vector<int>& v) {        // 修改，零複製
    for (int& n : v) n *= 2;
}

vector<int> createVector(int n) {          // RVO/NRVO 通常零複製返回
    vector<int> result(n);
    iota(result.begin(), result.end(), 1);
    return result;  // 編譯器通常會做 RVO（Return Value Optimization）
}

void demoCopyAvoidance() {
    cout << "\n【避免不必要的複製】" << endl;

    vector<int> v = createVector(5);
    readVector(v);                          // 傳 const 參考，零複製
    modifyVector(v);                        // 傳參考，零複製

    cout << "  修改後: ";
    for (int n : v) cout << n << " ";
    cout << endl;

    // 移動 vector（O(1)，轉移所有權）
    vector<int> v2 = move(v);              // v 的資料轉給 v2，v 變空
    cout << "  move 後 v.size()=" << v.size()
         << " v2.size()=" << v2.size() << endl;
}

// ================================================================
// 重點六：最佳實踐總結
// ================================================================
//
// ✓ DO（應該做的）：
//   1. 事先知道元素數量時，用 reserve()
//   2. 傳遞 vector 時優先用 const 參考（唯讀）或參考（修改）
//   3. 就地構造複雜物件時用 emplace_back
//   4. 返回大型 vector 時讓 RVO/NRVO 優化（直接 return）
//   5. 不再需要 vector 時，用 shrink_to_fit() 釋放記憶體
//
// ✗ DON'T（不應該做的）：
//   1. 不要在 vector 中間頻繁插入（改用 list）
//   2. 不要傳 vector by value（除非需要複製）
//   3. 不要忽略重分配後的迭代器失效問題
//   4. 不要在大型 vector 前面插入（O(n) 位移）

int main() {
    cout << "=============================================" << endl;
    cout << "   第 20 課：vector 效能分析與最佳實踐" << endl;
    cout << "=============================================" << endl;

    demoReserve();
    demoPushVsEmplace();
    demoTraversal();
    demoShrink();
    demoCopyAvoidance();

    cout << "\n==============================================" << endl;
    cout << " 最重要的最佳實踐：" << endl;
    cout << " 1. 批量插入前 reserve()" << endl;
    cout << " 2. 傳遞時用 const ref" << endl;
    cout << " 3. 就地構造用 emplace_back" << endl;
    cout << " 4. 注意迭代器/指標失效" << endl;
    cout << "==============================================" << endl;

    return 0;
}
