#include <iostream>
using namespace std;

int main() {
    cout << "=== delete nullptr ===" << endl;
    
    int* p = nullptr;
    delete p;       // 完全安全！不會崩潰, 也不會有任何效果
    cout << "  delete nullptr 是安全的" << endl;
    
    int* arr = nullptr;
    delete[] arr;   // 也是安全的, 不會崩潰, 也不會有任何效果
    cout << "  delete[] nullptr 也是安全的" << endl;
    
    // 但是 delete 同一個指標兩次是未定義行為！可能會崩潰！
    cout << "\n  // int* q = new int(42);" << endl;
    cout << "  // delete q;   ← 第一次 OK" << endl;
    cout << "  // delete q;   ← 第二次：未定義行為！可能崩潰！" << endl;
    
    // 好習慣：delete 後把指標設為 nullptr, 避免誤刪同一個指標兩次, 這樣即使第二次 delete 也安全（因為是 nullptr）
    int* safe = new int(42);
    delete safe;
    safe = nullptr;    // 設為 nullptr
    delete safe;       // 再次 delete 也安全（因為是 nullptr）
    cout << "\n  好習慣：delete 後設為 nullptr" << endl;
    
    return 0;
}
