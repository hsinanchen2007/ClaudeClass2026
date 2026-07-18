#include <iostream>
#include <deque>
#include <vector>
using namespace std;

void print(const string& label, const deque<int>& dq) {
    cout << label << ": ";
    for (int val : dq) {
        cout << val << " ";
    }
    cout << "(size=" << dq.size() << ")" << endl;
}

int main() {
    // 1. 預設建構
    deque<int> d1;
    print("d1 空deque", d1);
    // d1 空deque: (size=0)

    // 2. 指定數量與預設值
    deque<int> d2(5);
    print("d2 五個0 ", d2);
    // d2 五個0 : 0 0 0 0 0 (size=5)

    // 3. 指定數量與指定值
    deque<int> d3(4, 77);
    print("d3 四個77", d3);
    // d3 四個77: 77 77 77 77 (size=4)

    // 4. 初始化列表
    deque<int> d4 = {10, 20, 30, 40, 50};
    print("d4 列表  ", d4);
    // d4 列表  : 10 20 30 40 50 (size=5)

    // 5. 複製建構
    deque<int> d5(d4);
    print("d5 複製d4", d5);
    // d5 複製d4: 10 20 30 40 50 (size=5)

    // 驗證深複製
    d5[0] = 999;
    print("修改d5後 ", d5);
    print("d4不受影響", d4);
    // 修改d5後 : 999 20 30 40 50 (size=5)
    // d4不受影響: 10 20 30 40 50 (size=5)

    // 6. 移動建構
    deque<int> d6(move(d5));
    print("d6 移動d5", d6);
    print("d5 被移走 ", d5);
    // d6 移動d5: 999 20 30 40 50 (size=5)
    // d5 被移走 : (size=0)

    // 7. 從 vector 的迭代器範圍建構
    vector<int> vec = {100, 200, 300};
    deque<int> d7(vec.begin(), vec.end());
    print("d7 從vec ", d7);
    // d7 從vec : 100 200 300 (size=3)

    // 8. assign 重新指定
    d7.assign({7, 8, 9, 10});
    print("d7 assign", d7);
    // d7 assign: 7 8 9 10 (size=4)

    // 9. 小括號 vs 大括號
    deque<int> d8(5, 10);
    deque<int> d9{5, 10};
    print("d8 (5,10)", d8);
    print("d9 {5,10}", d9);
    // d8 (5,10): 10 10 10 10 10 (size=5)
    // d9 {5,10}: 5 10 (size=2)

    return 0;
}
