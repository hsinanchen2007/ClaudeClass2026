#include <iostream>
#include <new>       // bad_alloc
using namespace std;

int main() {
    cout << "=== new 的錯誤處理 ===" << endl;
    
    // ====== 方式 1：用 try-catch 捕獲異常（標準方式）======
    // C++ 的 new 在分配失敗時會拋出 bad_alloc 異常，我們可以捕獲它來處理錯誤
    cout << "\n--- 方式 1：try-catch ---" << endl;
    try {
        // 嘗試分配巨大的記憶體（可能失敗）
        // 這裡用一個合理大小來示範語法
        int* p = new int[100];
        cout << "  分配成功！" << endl;
        delete[] p;
    } catch (const bad_alloc& e) {
        cout << "  記憶體分配失敗: " << e.what() << endl;
    }
    
    // ====== 方式 2：使用 nothrow 版本（返回 nullptr）======
    // C++ 也提供了 nothrow 版本的 new，當分配失敗時不拋異常，而是返回 nullptr
    cout << "\n--- 方式 2：nothrow ---" << endl;
    int* p2 = new(nothrow) int[100];  // 失敗時返回 nullptr，不拋異常
    if (p2 == nullptr) {
        cout << "  記憶體分配失敗（返回 nullptr）" << endl;
    } else {
        cout << "  分配成功！" << endl;
        delete[] p2;
    }
    
    // ====== 實際的分配失敗示範 ======
    cout << "\n--- 嘗試分配超大記憶體 ---" << endl;
    try {
        // 嘗試分配大約 8TB 的記憶體（一定會失敗）
        size_t hugeSize = 1000000000000ULL;
        int* huge = new int[hugeSize];
        delete[] huge;  // 不會執行到這裡
    } catch (const bad_alloc& e) {
        cout << "  預期中的失敗: " << e.what() << endl;
    }
    
    return 0;
}
