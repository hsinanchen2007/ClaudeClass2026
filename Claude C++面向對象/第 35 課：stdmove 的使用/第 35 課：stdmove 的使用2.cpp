// lesson35_comprehensive.cpp
// 編譯：g++ -std=c++17 -Wall -Wextra -g -o lesson35b lesson35_comprehensive.cpp
// 驗證：valgrind --leak-check=full ./lesson35b

#include <iostream>
#include <string>
#include <vector>
#include <utility>

class GameItem {
private:
    std::string m_name;
    int m_power;

public:
    GameItem(std::string name, int power)
        : m_name(std::move(name))   // move 進成員
        , m_power(power)
    {
        std::cout << "  [建構] " << m_name << " (power=" << m_power << ")\n";
    }

    // Rule of Zero：成員都是 string 和 int，不需要自定義任何特殊函數
    // 但為了觀察，加上追蹤訊息：

    GameItem(const GameItem& other)
        : m_name(other.m_name), m_power(other.m_power) {
        std::cout << "  [拷貝] " << m_name << "\n";
    }

    GameItem(GameItem&& other) noexcept
        : m_name(std::move(other.m_name)), m_power(other.m_power) {
        other.m_power = 0;
        std::cout << "  [移動] " << m_name << "\n";
    }

    GameItem& operator=(const GameItem& other) {
        m_name = other.m_name;
        m_power = other.m_power;
        std::cout << "  [拷貝賦值] " << m_name << "\n";
        return *this;
    }

    GameItem& operator=(GameItem&& other) noexcept {
        m_name = std::move(other.m_name);
        m_power = other.m_power;
        other.m_power = 0;
        std::cout << "  [移動賦值] " << m_name << "\n";
        return *this;
    }

    ~GameItem() = default;

    const std::string& name() const { return m_name; }
    int power() const { return m_power; }
};

class Inventory {
    std::vector<GameItem> m_items;

public:
    // 用 const T& 和 T&& 重載，分別處理拷貝和移動
    void addItem(const GameItem& item) {
        std::cout << "  addItem (拷貝路徑):\n";
        m_items.push_back(item);
    }

    void addItem(GameItem&& item) {
        std::cout << "  addItem (移動路徑):\n";
        m_items.push_back(std::move(item));
    }

    // 移出最後一個物品
    GameItem takeLastItem() {
        GameItem item = std::move(m_items.back());  // 從 vector 移出
        m_items.pop_back();
        return item;  // NRVO 或隱式移動
    }

    void print() const {
        std::cout << "  背包 (" << m_items.size() << " 個物品): ";
        for (const auto& item : m_items) {
            std::cout << "[" << item.name() << " +" << item.power() << "] ";
        }
        std::cout << "\n";
    }
};

int main() {
    std::cout << "===== 建立物品 =====\n";
    GameItem sword("Fire Sword", 50);
    GameItem shield("Ice Shield", 30);

    Inventory bag;

    std::cout << "\n===== 拷貝加入背包 =====\n";
    bag.addItem(sword);       // sword 是左值 → 拷貝
    bag.print();
    std::cout << "  sword 仍然存在：" << sword.name() << "\n";

    std::cout << "\n===== 移動加入背包 =====\n";
    bag.addItem(std::move(shield));  // 移動
    bag.print();
    std::cout << "  shield 已被移動：\"" << shield.name() << "\"\n";

    std::cout << "\n===== 直接建構加入 =====\n";
    bag.addItem(GameItem("Thunder Staff", 70));  // 暫時物件 → 移動
    bag.print();

    std::cout << "\n===== 取出最後一個物品 =====\n";
    GameItem taken = bag.takeLastItem();
    std::cout << "  取出：" << taken.name() << " +" << taken.power() << "\n";
    bag.print();

    std::cout << "\n===== 結束 =====\n";
    return 0;
}
