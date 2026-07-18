#include <iostream>
#include <string>
using namespace std;

// ============================================================
// 遊戲角色類別：展示帶參建構函數的各種技巧
// ============================================================
class GameCharacter {
private:
    string name_;         // Google 風格：底線後綴
    string classType_;    // 職業
    int level_;
    int hp_;
    int mp_;
    double attackPower_;

public:
    // 建構函數 1：完整建構（const 引用 + 預設參數）
    explicit GameCharacter(const string& name, 
                           const string& classType = "戰士",
                           int level = 1)
        : name_(name), classType_(classType), level_(level)
    {
        // 根據職業設定基礎屬性
        if (classType_ == "戰士") {
            hp_ = 150 + level_ * 20;
            mp_ = 30 + level_ * 5;
            attackPower_ = 15.0 + level_ * 3.0;
        } else if (classType_ == "法師") {
            hp_ = 80 + level_ * 10;
            mp_ = 100 + level_ * 15;
            attackPower_ = 25.0 + level_ * 5.0;
        } else if (classType_ == "弓箭手") {
            hp_ = 100 + level_ * 12;
            mp_ = 50 + level_ * 8;
            attackPower_ = 20.0 + level_ * 4.0;
        } else {
            // 未知職業，使用平均值
            hp_ = 100 + level_ * 15;
            mp_ = 50 + level_ * 10;
            attackPower_ = 15.0 + level_ * 3.5;
        }
        
        cout << "  [創建角色] " << name_ << " - " << classType_ 
             << " (Lv." << level_ << ")" << endl;
    }

    void printStatus() const {
        cout << "  ┌──────────────────────────┐" << endl;
        cout << "  │ " << name_ << " [" << classType_ << "]" << endl;
        cout << "  │ 等級: " << level_ << endl;
        cout << "  │ HP: " << hp_ << "  MP: " << mp_ << endl;
        cout << "  │ 攻擊力: " << attackPower_ << endl;
        cout << "  └──────────────────────────┘" << endl;
    }
};

// ============================================================
// 座標類別：展示 explicit 和窄化防護
// ============================================================
class Coordinate {
private:
    int x_, y_;

public:
    // explicit + 大括號初始化列表防止窄化
    explicit Coordinate(int x, int y) : x_(x), y_(y) { }
    
    void print() const {
        cout << "  (" << x_ << ", " << y_ << ")" << endl;
    }
};

int main() {
    cout << "==========================================" << endl;
    cout << "   第 15 課：帶參數的建構函數 綜合範例" << endl;
    cout << "==========================================" << endl;
    
    // --- 使用不同數量的參數 ---
    cout << "\n[1] 不同參數數量" << endl;
    GameCharacter hero1("勇者小明");                    // 預設職業和等級
    GameCharacter hero2("暗影法師", "法師");            // 預設等級
    GameCharacter hero3("神射手", "弓箭手", 10);        // 全部指定
    
    cout << "\n[2] 角色狀態" << endl;
    hero1.printStatus();
    hero2.printStatus();
    hero3.printStatus();
    
    // --- explicit 的效果 ---
    cout << "\n[3] Coordinate (explicit)" << endl;
    Coordinate c1(10, 20);         // OK：直接初始化
    Coordinate c2{30, 40};         // OK：大括號初始化
    // Coordinate c3 = {50, 60};   // 錯誤！explicit 禁止拷貝列表初始化
    c1.print();
    c2.print();
    
    // --- 大括號防窄化 ---
    cout << "\n[4] 窄化轉換測試" << endl;
    Coordinate c4(3, 4);      // OK
    // Coordinate c5{3.7, 4.2};  // 編譯錯誤！double → int 是窄化
    Coordinate c6{int(3.7), int(4.2)};  // OK：明確轉換
    c4.print();
    c6.print();
    
    // --- 陣列中使用帶參建構函數 ---
    cout << "\n[5] 陣列中的帶參建構" << endl;
    GameCharacter party[3] = {
        GameCharacter("坦克", "戰士", 5),
        GameCharacter("治癒者", "法師", 3),
        GameCharacter("輸出手", "弓箭手", 7)
    };
    
    cout << "\n隊伍成員：" << endl;
    for (int i = 0; i < 3; i++) {
        party[i].printStatus();
    }
    
    return 0;
}
