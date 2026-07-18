/*
 * ================================================================
 * 【第 13 課：vector 元素新增：push_back、emplace_back】
 * 總複習 summary.cpp
 * ================================================================
 * 編譯方式：g++ -std=c++17 -o summary summary.cpp
 *
 * 本課重點：
 * 1. push_back()   —— 複製或移動元素到尾端
 * 2. emplace_back()—— 就地構造元素（C++11），效能更優
 * 3. push_back vs emplace_back 的差異與選擇
 * 4. 移動語意（move semantics）在 push_back 的應用
 * 5. reserve() 搭配批量新增以避免重分配
 * ================================================================
 */

#include <iostream>
#include <vector>
#include <string>
using namespace std;

// 用於觀察建構/複製/移動行為的測試類別
struct Tracker {
    string name;

    Tracker(const string& n) : name(n) {
        cout << "  [建構] " << name << endl;
    }
    Tracker(const Tracker& other) : name(other.name) {
        cout << "  [複製] " << name << endl;
    }
    Tracker(Tracker&& other) noexcept : name(move(other.name)) {
        cout << "  [移動] " << name << endl;
    }
    ~Tracker() {
        // 解構時不印出，避免干擾輸出
    }
};

// ================================================================
// 重點一：push_back() 的基本用法
// ================================================================
// push_back(value)：將 value 複製（或移動）到 vector 尾端
// 若 vector 空間不足，會自動重新分配記憶體（所有元素被移動）

void demoPushBack() {
    cout << "\n【push_back 基本用法】" << endl;

    vector<int> v;

    v.push_back(10);
    v.push_back(20);
    v.push_back(30);

    cout << "元素: ";
    for (int n : v) cout << n << " ";
    cout << endl;

    // push_back 字串
    vector<string> words;
    string hello = "Hello";

    words.push_back(hello);             // 複製 hello 進入 vector
    words.push_back("World");           // 臨時字串物件：移動進入
    words.push_back(move(hello));       // 移動 hello 進入（hello 後可能為空）

    cout << "hello 字串後的狀態: \"" << hello << "\"" << endl;  // 可能為空
    cout << "vector 內容: ";
    for (const string& w : words) cout << "\"" << w << "\" ";
    cout << endl;
}

// ================================================================
// 重點二：push_back 的複製 vs 移動行為
// ================================================================
// 傳遞左值（lvalue）：複製
// 傳遞右值（rvalue）或用 move()：移動

void demoPushBackCopyMove() {
    cout << "\n【push_back 複製 vs 移動】" << endl;

    vector<Tracker> v;
    v.reserve(3);  // 預留空間，避免重分配影響觀察

    cout << "--- push_back(lvalue)（複製）---" << endl;
    Tracker t1("T1");
    v.push_back(t1);      // 複製：t1 仍然有效

    cout << "--- push_back(rvalue)（移動）---" << endl;
    v.push_back(Tracker("T2"));  // 臨時物件：移動

    cout << "--- push_back(move(lvalue))（移動）---" << endl;
    Tracker t3("T3");
    v.push_back(move(t3));  // 明確移動：t3 之後不能再用
}

// ================================================================
// 重點三：emplace_back() —— 就地構造（C++11）
// ================================================================
// emplace_back(args...)：直接在 vector 末尾構造物件
// 傳入的是「建構函數的參數」，而非物件本身
// 好處：省去一次複製或移動，效能更好
// 注意：C++17 前不保證例外安全；C++17 後回傳最後插入元素的參考

void demoEmplaceBack() {
    cout << "\n【emplace_back 就地構造】" << endl;

    vector<Tracker> v;
    v.reserve(2);

    cout << "--- emplace_back（直接用建構參數）---" << endl;
    v.emplace_back("E1");  // 直接傳建構函數的參數，就地構造！
    v.emplace_back("E2");  // 完全省去複製/移動
}

// ================================================================
// 重點四：push_back vs emplace_back 比較
// ================================================================
// 對於內建類型（int、double 等）：幾乎沒有差別
// 對於複雜物件（有建構函數的類別）：emplace_back 可能更快
// 對於已有的物件：push_back 語意更清晰
// 對於需要就地構造：emplace_back 更優

struct Point {
    int x, y;
    Point(int x, int y) : x(x), y(y) {}
};

void demoCompare() {
    cout << "\n【push_back vs emplace_back 比較】" << endl;

    vector<Point> v;
    v.reserve(4);

    // push_back：需要先構造 Point 物件
    v.push_back(Point(1, 2));       // 構造臨時物件，再移動進入
    v.push_back({3, 4});            // 初始化列表，也是先構造

    // emplace_back：直接傳參數，就地構造，少一次移動
    v.emplace_back(5, 6);           // 直接傳 (5, 6)，在 vector 內構造
    v.emplace_back(7, 8);

    cout << "Points: ";
    for (const Point& p : v) cout << "(" << p.x << "," << p.y << ") ";
    cout << endl;
}

// ================================================================
// 重點五：reserve() 搭配批量新增
// ================================================================
// 每次 push_back 時若 capacity 不足，vector 會重分配記憶體
// 重分配 = 申請新空間 + 移動所有元素 + 釋放舊空間
// 若知道大概需要多少元素，先 reserve() 可大幅提升效能

void demoReserveWithPushBack() {
    cout << "\n【reserve() 搭配 push_back 最佳實踐】" << endl;

    // 不使用 reserve（可能多次重分配）
    vector<int> v1;
    int realloc1 = 0;
    size_t lastCap1 = 0;
    for (int i = 0; i < 20; ++i) {
        v1.push_back(i);
        if (v1.capacity() != lastCap1) {
            realloc1++;
            lastCap1 = v1.capacity();
        }
    }
    cout << "不用 reserve，重分配次數: " << realloc1 << endl;

    // 使用 reserve（零重分配）
    vector<int> v2;
    v2.reserve(20);
    int realloc2 = 0;
    size_t lastCap2 = v2.capacity();
    for (int i = 0; i < 20; ++i) {
        v2.push_back(i);
        if (v2.capacity() != lastCap2) {
            realloc2++;
            lastCap2 = v2.capacity();
        }
    }
    cout << "用 reserve(20)，重分配次數: " << realloc2 << endl;
}

// ================================================================
// 重點六：常見錯誤與陷阱
// ================================================================
// 陷阱一：push_back 後迭代器失效（若觸發重分配）
// 陷阱二：在循環中 push_back 並同時遍歷同一 vector

void demoTrap() {
    cout << "\n【常見陷阱】" << endl;

    vector<int> v = {1, 2, 3};

    // 陷阱：push_back 後迭代器可能失效
    auto it = v.begin();
    v.push_back(4);  // 可能觸發重分配！
    // cout << *it;  // 危險！it 可能已失效

    // 正確做法：push_back 後重新取迭代器
    it = v.begin();
    cout << "重新取迭代器後，*it = " << *it << endl;

    // 正確：先 reserve 再 push_back，迭代器在 capacity 範圍內不失效
    vector<int> v2;
    v2.reserve(10);
    auto it2 = v2.begin();
    for (int i = 0; i < 5; ++i) {
        v2.push_back(i * 10);  // 不超過 capacity，迭代器安全
    }
    cout << "reserve 後 push_back，安全不失效" << endl;
}

int main() {
    cout << "=============================================" << endl;
    cout << "   第 13 課：push_back / emplace_back 總複習" << endl;
    cout << "=============================================" << endl;

    demoPushBack();
    demoPushBackCopyMove();
    demoEmplaceBack();
    demoCompare();
    demoReserveWithPushBack();
    demoTrap();

    cout << "\n==============================================" << endl;
    cout << " 選擇建議：" << endl;
    cout << " - 一般用 push_back（語意清晰）" << endl;
    cout << " - 就地構造複雜物件時用 emplace_back" << endl;
    cout << " - 批量新增前先 reserve()" << endl;
    cout << "==============================================" << endl;

    return 0;
}
