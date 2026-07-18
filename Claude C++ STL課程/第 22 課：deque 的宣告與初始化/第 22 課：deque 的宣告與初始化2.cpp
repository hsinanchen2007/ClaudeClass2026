#include <iostream>
#include <vector>
using namespace std;

int main() {
    vector<int*> v1;
    v1.push_back(new int(42));
    v1.push_back(new int(99));

    // 複製建構
    vector<int*> v2(v1);

    // v1 和 v2 的指標指向同一塊記憶體
    cout << *v1[0] << endl;  // 42
    cout << *v2[0] << endl;  // 42

    // 透過 v2 修改，v1 也會看到變化
    *v2[0] = 777;
    cout << *v1[0] << endl;  // 777  ← v1 也被影響了！

    // 最危險的情況：double free
    // 如果 v1 和 v2 都嘗試 delete 同一個指標...
    // delete v1[0];  // 第一次 delete → OK
    // delete v2[0];  // 第二次 delete 同一塊 → 未定義行為！

    // 正確清理（只能 delete 一次）
    for (int* p : v1) delete p;
    // v2 就不能再 delete 了，也不能再存取了

    return 0;
}
