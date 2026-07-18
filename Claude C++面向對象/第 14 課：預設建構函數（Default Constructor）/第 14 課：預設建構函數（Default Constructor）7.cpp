#include <iostream>
#include <string>
using namespace std;

class Enemy {
private:
    string type;
    int health;

public:
    // 必須有預設建構函數，否則無法創建陣列
    // 預設建構函數會自動被編譯器生成，但如果我們定義了其他建構函數，就必須自己寫出預設建構函數
    // 預設建構函數可以有預設值，但這裡直接給出固定的預設值
    // 預設建構函數會在創建陣列時被自動調用，為每個元素初始化
    Enemy() {
        type = "小怪";
        health = 100;
    }
    
    Enemy(string t, int hp) {
        type = t;
        health = hp;
    }

    void print() const {
        cout << "  " << type << " (HP: " << health << ")" << endl;
    }
    
    void setType(string t) { type = t; }
    void setHealth(int hp) { health = hp; }
};

int main() {
    cout << "=== 對象陣列（需要預設建構函數）===" << endl;
    
    // 創建 5 個 Enemy，每個都調用預設建構函數
    // 如果沒有預設建構函數，這裡會編譯錯誤，因為編譯器不知道如何初始化陣列中的每個元素
    // 預設建構函數會為每個 Enemy 設定 type 為 "小怪" 和 health 為 100
    // 這裡創建了一個 Enemy 類型的陣列，陣列中的每個元素都會自動調用預設建構函數來初始化
    // 如果 Enemy 類沒有預設建構函數，這裡會出現編譯錯誤，因為編譯器無法為陣列中的每個元素找到合適的建構函數來初始化
    // 預設建構函數的存在使得我們可以輕鬆地創建對象陣列，並且每個對象都會有合理的初始狀態
    Enemy enemies[5];
    
    for (int i = 0; i < 5; i++) {
        enemies[i].print();
    }
    
    cout << "\n=== 修改後 ===" << endl;
    enemies[0].setType("Boss");
    enemies[0].setHealth(5000);
    enemies[2].setType("精英怪");
    enemies[2].setHealth(500);
    
    for (int i = 0; i < 5; i++) {
        enemies[i].print();
    }
    
    return 0;
}
