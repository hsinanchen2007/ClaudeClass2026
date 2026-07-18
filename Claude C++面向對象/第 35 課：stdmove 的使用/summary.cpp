// ============================================================
// 第 35 課 總結：std::move 的使用
// 編譯：g++ -std=c++17 -o summary summary.cpp
// ============================================================
// 【核心觀念】
// std::move(x) 本身不移動任何東西！
// 它只是 static_cast<T&&>(x)，把左值轉成右值引用
// 真正的移動發生在接收端的移動建構/移動賦值中
//
// 【五大使用場景】
//   1. 放棄所有權：明確不再需要的局部變數 → 移動給別人
//   2. 建構函數 pass-by-value + move：成員初始化的推薦寫法
//   3. push_back(move(obj))：避免不必要的拷貝
//   4. 從容器提取元素：move(container.back()) + pop_back()
//   5. swap 實現：3 次移動交換兩個物件（零拷貝）
//
// 【注意事項】
//   ❌ 不要 move const 物件 → const T&& 不觸發移動，退化為拷貝
//   ❌ 不要 move 函數回傳的局部變數 → 阻止 NRVO 優化
//      return move(local);  // 不好！
//      return local;        // 好！讓編譯器做 NRVO
//   ❌ move 後不要再使用物件的值（可以重新賦值或解構）
//
// 【push_back vs emplace_back】
//   push_back(obj)        → 拷貝
//   push_back(move(obj))  → 移動
//   emplace_back(args...) → 原地建構（最佳，零拷貝零移動）
// ============================================================

#include <iostream>
#include <string>
#include <vector>
#include <utility>

// ============================================================
// 追蹤拷貝/移動次數的字串類別
// ============================================================
class TrackedString {
    std::string m_data;
    static int s_copyCount;
    static int s_moveCount;
public:
    TrackedString(const char* s = "") : m_data(s) {
        std::cout << "    [建構] \"" << m_data << "\"\n";
    }

    TrackedString(const TrackedString& other) : m_data(other.m_data) {
        ++s_copyCount;
        std::cout << "    [拷貝💰] \"" << m_data << "\"\n";
    }

    TrackedString(TrackedString&& other) noexcept : m_data(std::move(other.m_data)) {
        ++s_moveCount;
        std::cout << "    [移動⚡] \"" << m_data << "\"\n";
    }

    TrackedString& operator=(const TrackedString& other) {
        m_data = other.m_data; ++s_copyCount;
        std::cout << "    [拷貝賦值] \"" << m_data << "\"\n";
        return *this;
    }

    TrackedString& operator=(TrackedString&& other) noexcept {
        m_data = std::move(other.m_data); ++s_moveCount;
        std::cout << "    [移動賦值] \"" << m_data << "\"\n";
        return *this;
    }

    const std::string& str() const { return m_data; }

    static void reset() { s_copyCount = s_moveCount = 0; }
    static void report() {
        std::cout << "    >>> 拷貝 " << s_copyCount
                  << " 次，移動 " << s_moveCount << " 次\n";
    }
};
int TrackedString::s_copyCount = 0;
int TrackedString::s_moveCount = 0;

// ============================================================
// 場景 2：建構函數 pass-by-value + move
// ============================================================
class Hero {
    TrackedString m_name;
    TrackedString m_title;
public:
    // ★ 推薦寫法：傳值 + move 進成員
    // 傳左值：拷貝到參數 → move 到成員（1 拷貝 + 1 移動）
    // 傳右值：移動到參數 → move 到成員（2 移動，零拷貝）
    Hero(TrackedString name, TrackedString title)
        : m_name(std::move(name))     // ← move 傳值參數到成員
        , m_title(std::move(title))
    {}

    void print() const {
        std::cout << "    Hero: " << m_name.str() << " - " << m_title.str() << "\n";
    }
};

// ============================================================
// 場景 4：Inventory — 從容器提取元素
// ============================================================
class GameItem {
    std::string m_name;
    int m_power;
public:
    GameItem(std::string name, int power)
        : m_name(std::move(name)), m_power(power) {}

    GameItem(const GameItem& o) : m_name(o.m_name), m_power(o.m_power) {
        std::cout << "    [GameItem 拷貝] " << m_name << "\n";
    }
    GameItem(GameItem&& o) noexcept
        : m_name(std::move(o.m_name)), m_power(o.m_power) {
        o.m_power = 0;
        std::cout << "    [GameItem 移動] " << m_name << "\n";
    }
    GameItem& operator=(GameItem o) {
        std::swap(m_name, o.m_name);
        std::swap(m_power, o.m_power);
        return *this;
    }
    ~GameItem() = default;

    const std::string& name() const { return m_name; }
    int power() const { return m_power; }
};

class Inventory {
    std::vector<GameItem> m_items;
public:
    // const T& 版本（拷貝路徑）
    void addItem(const GameItem& item) {
        std::cout << "    addItem (拷貝路徑):\n";
        m_items.push_back(item);
    }
    // T&& 版本（移動路徑）
    void addItem(GameItem&& item) {
        std::cout << "    addItem (移動路徑):\n";
        m_items.push_back(std::move(item));
    }
    // 從容器移出最後一個
    GameItem takeLastItem() {
        GameItem item = std::move(m_items.back());
        m_items.pop_back();
        return item;
    }

    void print() const {
        std::cout << "    背包 (" << m_items.size() << " 個): ";
        for (const auto& i : m_items)
            std::cout << "[" << i.name() << " +" << i.power() << "] ";
        std::cout << "\n";
    }
};

int main() {
    // ============================================================
    // 場景 1：放棄所有權
    // ============================================================
    std::cout << "===== 場景 1：放棄所有權 =====\n";
    TrackedString::reset();
    {
        TrackedString a("Excalibur");
        std::cout << "  不用 move（拷貝）：\n";
        TrackedString b = a;                // 拷貝
        std::cout << "  用 move（移動）：\n";
        TrackedString c = std::move(a);     // 移動
        std::cout << "  a 移動後：\"" << a.str() << "\"（通常為空）\n";
    }
    TrackedString::report();
    std::cout << "\n";

    // ============================================================
    // 場景 2：建構函數 pass-by-value + move
    // ============================================================
    std::cout << "===== 場景 2：pass-by-value + move =====\n";
    {
        std::cout << "  傳入左值（1拷貝 + 1移動 per 參數）：\n";
        TrackedString::reset();
        TrackedString name("Arthur"), title("King");
        Hero h1(name, title);
        TrackedString::report();

        std::cout << "\n  傳入右值（2移動 per 參數，零拷貝）：\n";
        TrackedString::reset();
        Hero h2(TrackedString("Merlin"), TrackedString("Wizard"));
        TrackedString::report();

        std::cout << "\n  用 std::move 傳入（也是移動路徑）：\n";
        TrackedString::reset();
        TrackedString n("Lancelot"), t("Knight");
        Hero h3(std::move(n), std::move(t));
        TrackedString::report();
    }
    std::cout << "\n";

    // ============================================================
    // 場景 3：push_back vs emplace_back
    // ============================================================
    std::cout << "===== 場景 3：push_back vs emplace_back =====\n";
    TrackedString::reset();
    {
        std::vector<TrackedString> vec;
        vec.reserve(3);

        TrackedString s1("Alpha");
        std::cout << "  push_back 拷貝：\n";
        vec.push_back(s1);                // 拷貝

        std::cout << "  push_back 移動：\n";
        vec.push_back(std::move(s1));     // 移動

        std::cout << "  emplace_back 原地建構：\n";
        vec.emplace_back("Delta");        // 原地建構（零拷貝零移動）
    }
    TrackedString::report();
    std::cout << "\n";

    // ============================================================
    // 場景 4：從容器提取元素
    // ============================================================
    std::cout << "===== 場景 4：從容器提取元素 =====\n";
    {
        Inventory bag;
        GameItem sword("Fire Sword", 50);
        bag.addItem(sword);                          // 拷貝加入
        bag.addItem(GameItem("Ice Shield", 30));     // 移動加入
        bag.addItem(GameItem("Thunder Staff", 70));  // 移動加入
        bag.print();

        std::cout << "\n  取出最後一個：\n";
        GameItem taken = bag.takeLastItem();  // move(back()) + pop_back()
        std::cout << "    取出：" << taken.name() << " +" << taken.power() << "\n";
        bag.print();
    }
    std::cout << "\n";

    // ============================================================
    // 場景 5：swap（3 次移動，0 次拷貝）
    // ============================================================
    std::cout << "===== 場景 5：swap =====\n";
    TrackedString::reset();
    {
        TrackedString x("Left"), y("Right");
        std::cout << "  swap 前：x=\"" << x.str() << "\" y=\"" << y.str() << "\"\n";
        // std::swap 內部：
        //   T temp = move(x);  ① x → temp
        //   x = move(y);       ② y → x
        //   y = move(temp);    ③ temp → y
        std::swap(x, y);
        std::cout << "  swap 後：x=\"" << x.str() << "\" y=\"" << y.str() << "\"\n";
    }
    TrackedString::report();
    std::cout << "\n";

    // ============================================================
    // 注意事項：const move 退化為拷貝
    // ============================================================
    std::cout << "===== 注意：const move 退化為拷貝 =====\n";
    TrackedString::reset();
    {
        const TrackedString constStr("Immovable");
        TrackedString copied = std::move(constStr);  // const → 無法移動 → 拷貝！
    }
    TrackedString::report();
    std::cout << "\n";

    // ============================================================
    // 重點整理
    // ============================================================
    std::cout << "=== 重點整理 ===\n";
    std::cout << "  std::move 只是 static_cast<T&&>，不做實際移動\n";
    std::cout << "  五大場景：放棄所有權、ctor 傳值+move、push_back、容器提取、swap\n";
    std::cout << "  push_back(obj) 拷貝 / push_back(move(obj)) 移動 / emplace_back 原地建構\n";
    std::cout << "  ❌ 不要 move const 物件（退化為拷貝）\n";
    std::cout << "  ❌ 不要 return move(local)（阻止 NRVO）\n";
    std::cout << "  ❌ move 後不要再使用物件的值\n";

    return 0;
}
