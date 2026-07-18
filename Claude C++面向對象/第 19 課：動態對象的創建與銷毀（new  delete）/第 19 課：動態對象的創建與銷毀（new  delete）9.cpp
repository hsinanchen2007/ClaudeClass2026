#include <iostream>
#include <string>
#include <memory>    // 智能指標
using namespace std;

class Monster {
private:
    string name;
public:
    Monster(const string& n) : name(n) {
        cout << "  [+] " << name << " 出現" << endl;
    }
    ~Monster() {
        cout << "  [-] " << name << " 消失" << endl;
    }
    void roar() const {
        cout << "  " << name << " 吼叫！" << endl;
    }
};

int main() {
    cout << "=== 裸 new/delete vs 智能指標 ===" << endl;
    
    // ====== 舊方式：裸 new/delete ======
    cout << "\n--- 舊方式（裸指標）---" << endl;
    {
        Monster* m = new Monster("哥布林");
        m->roar();
        delete m;       // 必須手動 delete！否則會洩漏記憶體
    }
    
    // ====== 新方式：unique_ptr ======
    cout << "\n--- 新方式（unique_ptr）---" << endl;
    {
        // make_unique 會自動調用 new，unique_ptr 離開作用域時自動 delete
        // 這裡我們創建了一個 unique_ptr，指向一個 Monster 對象
        // unique_ptr 是一種智能指標，確保在離開作用域時自動釋放資源
        unique_ptr<Monster> m = make_unique<Monster>("史萊姆");
        m->roar();
        // 不需要 delete！unique_ptr 自動處理！即使在這裡發生異常，unique_ptr 也會確保記憶體被釋放，避免洩漏。
    }
    
    // ====== 異常安全 ======
    cout << "\n--- 異常安全示範 ---" << endl;
    try {
        unique_ptr<Monster> m = make_unique<Monster>("龍");
        m->roar();
        throw runtime_error("勇者逃跑了！");
        // m 在堆疊展開時自動解構，不會洩漏記憶體, 即使發生異常也能確保資源被釋放。
    } catch (const exception& e) {
        cout << "  " << e.what() << endl;
    }
    
    // ====== 動態陣列 ======
    cout << "\n--- 動態陣列（unique_ptr）---" << endl;
    {
        unique_ptr<int[]> arr = make_unique<int[]>(5);
        for (int i = 0; i < 5; i++) {
            arr[i] = (i + 1) * 10;
        }
        
        cout << "  ";
        for (int i = 0; i < 5; i++) {
            cout << arr[i] << " ";
        }
        cout << endl;
        // 不需要 delete[]！unique_ptr 自動處理！即使在這裡發生異常，unique_ptr 也會確保記憶體被釋放，避免洩漏。
    }
    
    cout << "\n=== 所有記憶體已自動管理 ===" << endl;
    return 0;
}
