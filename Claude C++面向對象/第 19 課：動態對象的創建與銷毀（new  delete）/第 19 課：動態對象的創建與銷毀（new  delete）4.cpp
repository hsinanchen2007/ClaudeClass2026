#include <iostream>
using namespace std;

class Item {
private:
    int id;
    static int nextId;
public:
    Item() : id(nextId++) {
        cout << "  [+] Item #" << id << endl;
    }
    ~Item() {
        cout << "  [-] Item #" << id << endl;
    }
};

int Item::nextId = 1;

int main() {
    cout << "=== 正確配對 ===" << endl;
    
    // 正確：new 配 delete, 會調用建構函數和解構函數，內部資源被正確管理
    Item* single = new Item;
    delete single;
    
    // 正確：new[] 配 delete[], 會調用陣列中每個元素的建構函數和解構函數，內部資源被正確管理
    Item* array = new Item[3];
    delete[] array;
    
    cout << "\n=== 錯誤配對（千萬不要這樣做！）===" << endl;
    cout << "  // Item* p = new Item[3];" << endl;
    cout << "  // delete p;      ← 錯誤！應該用 delete[]" << endl;
    cout << "  // 後果：只解構第一個元素，其餘洩漏" << endl;
    cout << "  //        或者直接崩潰（未定義行為）" << endl;
    
    cout << "\n  // Item* q = new Item;" << endl;
    cout << "  // delete[] q;    ← 錯誤！應該用 delete" << endl;
    cout << "  // 後果：未定義行為，可能崩潰" << endl;
    
    return 0;
}
