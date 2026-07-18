#include <iostream>
#include <string>
using namespace std;

class Score {
private:
    string playerName_;
    int points_;

public:
    Score(const string& name, int pts)
        : playerName_(name), points_(pts)
    {
    }

    // 用法 1：返回自身引用（鏈式調用）
    Score& addPoints(int pts) {
        points_ += pts;
        return *this;    // *this 是 Score&
    }

    // 用法 2：返回自身的拷貝（不修改原物件）
    Score doubled() const {
        Score copy = *this;     // 拷貝自身
        copy.points_ *= 2;
        return copy;
    }

    // 用法 3：比較自身與另一個對象
    bool isHigherThan(const Score& other) const {
        // 先檢查是不是和自己比
        if (this == &other) return false;
        return points_ > other.points_;
    }

    // 用法 4：把自身傳給外部函數
    void printVia(void (*printFunc)(const Score&)) const {
        printFunc(*this);    // 把自身傳出去
    }

    const string& getName() const { return playerName_; }
    int getPoints() const { return points_; }
};

// 外部的列印函數
void fancyPrint(const Score& s) {
    cout << "  ★ " << s.getName() << "：" << s.getPoints() << " 分 ★" << endl;
}

int main() {
    cout << "=== *this 的各種用法 ===" << endl;

    Score s("陳信安", 100);

    // 用法 1：鏈式調用
    cout << "\n--- 鏈式加分 ---" << endl;
    s.addPoints(50).addPoints(30).addPoints(20);
    cout << "  總分：" << s.getPoints() << endl;

    // 用法 2：拷貝自身
    cout << "\n--- 翻倍（不影響原物件）---" << endl;
    Score doubled = s.doubled();
    cout << "  原始：" << s.getPoints() << endl;
    cout << "  翻倍：" << doubled.getPoints() << endl;

    // 用法 3：比較
    cout << "\n--- 比較 ---" << endl;
    Score other("對手", 150);
    cout << "  " << s.getName() << " 高於 " << other.getName() << "？ "
         << (s.isHigherThan(other) ? "是" : "否") << endl;
    cout << "  和自己比？ "
         << (s.isHigherThan(s) ? "是" : "否") << endl;

    // 用法 4：傳給外部函數
    cout << "\n--- 傳遞 *this ---" << endl;
    s.printVia(fancyPrint);

    return 0;
}
