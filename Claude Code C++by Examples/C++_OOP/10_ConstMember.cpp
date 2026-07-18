/*=============================================================================
 * 檔名：10_ConstMember.cpp
 * 主題：const 成員函式、const 物件、const 正確性 (const-correctness)
 * 適合：已會寫類別，前面範例已經看過 const 但沒搞清楚意思的人
 *
 * 【課題介紹】
 *   const 在 C++ 是個「保證不會改變」的承諾。物件導向中常出現三個地方：
 *
 *     A. const 成員函式：     int getX() const;
 *        承諾「這個函式不會改動本物件的成員變數」。
 *
 *     B. const 物件 / const 參考：    const Point p; / const Point& p;
 *        這個物件被視為唯讀，從它身上「只能呼叫 const 成員函式」。
 *
 *     C. const 參數：         void f(const std::string& s);
 *        承諾「函式內不會修改 s」，且能避免複製字串成本。
 *
 *   這三者結合起來叫做「const-correctness」(常數正確性) — 在大型專案非常重要。
 *
 * 【const 成員函式怎麼寫？】
 *   把 const 寫在「函式宣告的右大括號之前」：
 *
 *       int getX() const { return x_; }
 *                  ↑↑↑↑
 *                  注意位置
 *
 *   它做了兩件事：
 *     (1) 編譯器會檢查函式內「沒有」對成員變數做修改 (修改就編譯錯誤)。
 *     (2) const 物件 / const 參考 / const 指標 → 只能呼叫 const 版本。
 *
 *   背後機理：const 成員函式中，this 的型別變成 const ClassName*，
 *   所以你不能透過 this 去改成員。
 *
 * 【const 物件只能呼叫 const 函式 - 為什麼？】
 *   const 物件保證「我不能被修改」，如果它能呼叫一個「會改自己」的函式，
 *   那就跟「你說你不會改我，又改了我」自相矛盾了。所以編譯器不允許。
 *
 *   實務影響：你寫的函式如果參數是 const Foo&，那這個函式只能呼叫 Foo 的
 *   const 成員函式。「忘記加 const」的成員函式，常常導致這類編譯錯誤。
 *
 * 【const / 非 const 多載 (overload)】
 *   有時候同一個 getter 想要兩種版本：
 *       T&       data();          // 給可改的物件用
 *       const T& data() const;    // 給 const 物件用
 *   編譯器會根據呼叫者的 constness 決定走哪個版本。
 *
 * 【mutable 關鍵字 (進階小知識)】
 *   想在「const 成員函式」內修改某個成員 (例如紀錄被讀過幾次)，
 *   可以把該成員宣告成 mutable，編譯器就會放行。
 *
 * 【日常實用範例】
 *   一個 Calculator 類別，含「狀態 (上次結果)」與一些只讀方法。
 *   再示範 const Calculator 物件能/不能做什麼。
 *
 * 【參考】
 *   https://en.cppreference.com/w/cpp/language/cv     (const, volatile)
 *   https://isocpp.org/wiki/faq/const-correctness
 *=============================================================================*/

/*
補充筆記：ConstMember
  - ConstMember 這類 OOP 範例要追蹤物件狀態：建構後是否有效、操作後是否仍符合類別承諾。
  - 如果類別擁有資源，就要檢查 destructor、copy、move 是否表達同一套所有權規則。
  - 繼承、friend、static、operator overload 都應服務於清楚的物件語意，而不是只展示語法。
  - const 成員函式承諾不修改物件的可觀察狀態，語法是在參數列後加 const，例如 int size() const。
  - 同一個物件若是 const，只能呼叫 const 成員函式；因此查詢類函式若忘記標 const，會讓 const 物件無法使用。
  - const 不是只給編譯器看的裝飾，它能幫讀者判斷函式是否會改狀態，也讓 API 更容易安全組合。
  - mutable 成員可在 const 函式中修改，常用於 cache、mutex、lazy computation；但濫用 mutable 會破壞 const 語意。
  - const reference 參數避免複製並承諾不改輸入；回傳 const reference 則要確認被參考的物件生命週期長於呼叫者使用期間。
  - const int*、int* const、const int* const 位置不同意義不同：分別是指向 const int 的指標、不能改指向的指標、兩者都不可改。
  - const 成員變數必須在初始化列表初始化；進入建構子本體後就不能再被賦值。
  - const_cast 可以移除 const，但只有原始物件本來不是 const 時才可安全修改；修改真正 const 物件是未定義行為。
*/
#include <iostream>
#include <string>
#include <vector>

class Calculator {
private:
    double          last_;        // 上次運算結果
    mutable size_t  readCount_;   // 被讀取過幾次 (用 mutable 才能在 const 函式裡 ++)

public:
    Calculator() : last_(0.0), readCount_(0) {}

    // 會修改物件 → 不寫 const
    void setLast(double v) { last_ = v; }

    double add(double a, double b) {
        last_ = a + b;            // 改了成員 → 不能宣告為 const
        return last_;
    }

    // 只讀，不改物件 → 寫成 const 成員函式
    double getLast() const {
        ++readCount_;             // mutable 讓這行在 const 函式內合法
        return last_;
    }

    // 只讀
    size_t getReadCount() const { return readCount_; }

    // 只讀
    void show() const {
        std::cout << "Calculator{ last=" << last_
                  << ", reads=" << readCount_ << " }" << std::endl;
    }
};

// 用 const 參考傳入：只能呼叫 const 成員函式
void readOnlyShow(const Calculator& c) {
    // c.add(1, 2);     // ← 編譯錯誤：add() 不是 const 成員函式
    c.show();           // ← OK：show() 是 const
    std::cout << "(經過 readOnlyShow) last=" << c.getLast() << std::endl;
}

// -----------------------------------------------------------------------------
// 範例 2：const 與非 const 多載
// -----------------------------------------------------------------------------
class IntArray {
private:
    int data_[3];
public:
    IntArray() : data_{10, 20, 30} {}

    // 非 const 版本：回傳「可寫」的參考，呼叫者可以修改
    int& at(size_t i)             { return data_[i]; }

    // const 版本：回傳「唯讀」的 const 參考
    const int& at(size_t i) const { return data_[i]; }
    // 兩個只差在尾巴的 const 與回傳型別，編譯器會根據呼叫者的 constness 自動挑選。

    size_t size() const { return 3; }
};

int main() {
    std::cout << "----- 範例 1：Calculator -----" << std::endl;
    Calculator c;
    c.add(3, 4);
    std::cout << "last = " << c.getLast() << std::endl;     // 7
    c.show();
    readOnlyShow(c);

    // const 物件示範：建立後不能修改
    const Calculator constC;
    // constC.add(1, 2);          // 編譯錯誤：add() 非 const，不能呼叫
    constC.show();                // OK：show() 是 const
    std::cout << "constC.getLast() = " << constC.getLast() << std::endl;  // 0

    std::cout << "----- 範例 2：const / 非 const 多載 -----" << std::endl;
    IntArray a;
    a.at(1) = 222;                              // 走非 const 版本，可以寫入
    std::cout << "a.at(0) = " << a.at(0) << "\n";
    std::cout << "a.at(1) = " << a.at(1) << "\n";

    const IntArray ca;
    // ca.at(0) = 99;                            // 編譯錯誤：const 版本回傳 const&，不能寫
    std::cout << "ca.at(0) = " << ca.at(0) << "\n";   // 走 const 版本，可以讀

    std::cout << "----- 範例 3：Leetcode 1672 + const 正確性 -----" << std::endl;
    // 題目簡述：給多個客戶在各家銀行的存款，回傳最富有客戶的總財富。
    // 重點：wealth() / richest() 都是只讀方法 → 標 const 才能被 const 物件使用。
    class Wallets {
    private:
        std::vector<std::vector<int>> accounts_;
    public:
        explicit Wallets(std::vector<std::vector<int>> v) : accounts_(std::move(v)) {}

        // 只讀：計算單一客戶總財富
        int wealthOf(size_t i) const {
            int sum = 0;
            for (int x : accounts_[i]) sum += x;
            return sum;
        }

        // 只讀：找最富有客戶的財富
        int richest() const {
            int best = 0;
            for (size_t i = 0; i < accounts_[0].size() && i < accounts_.size(); ++i) {
                // 上面的 short-circuit 只是讓 unused 變數較合理；下面是正解：
            }
            for (size_t i = 0; i < accounts_.size(); ++i) {
                int w = wealthOf(i);
                if (w > best) best = w;
            }
            return best;
        }
    };

    const Wallets W({{1, 2, 3}, {3, 2, 1}, {1, 5, 7}});
    std::cout << "最富有: " << W.richest() << std::endl;   // 13

    std::cout << "----- 範例 4：日常實用 - ReadOnlyView -----" << std::endl;
    // 工作上常見：對外暴露 const reference，避免呼叫端修改內部資料。
    class Inventory {
    private:
        std::vector<std::string> items_;
    public:
        void add(const std::string& s) { items_.push_back(s); }
        // 只讀視圖：標 const，回傳 const reference
        const std::vector<std::string>& view() const { return items_; }
        size_t size() const { return items_.size(); }
    };

    Inventory inv;
    inv.add("螺絲"); inv.add("墊片"); inv.add("螺母");
    const Inventory& cRef = inv;          // 用 const reference 拿到唯讀視圖
    std::cout << "庫存共 " << cRef.size() << " 項，首項: "
              << cRef.view()[0] << std::endl;
    // cRef.add("X");                      // 編譯錯誤：add() 非 const
    return 0;
}

/* 預期輸出：
 * ----- 範例 1：Calculator -----
 * last = 7
 * Calculator{ last=7, reads=1 }
 * Calculator{ last=7, reads=1 }
 * (經過 readOnlyShow) last=7
 * Calculator{ last=0, reads=0 }
 * constC.getLast() = 0
 * ----- 範例 2：const / 非 const 多載 -----
 * a.at(0) = 10
 * a.at(1) = 222
 * ca.at(0) = 10
 * ----- 範例 3：Leetcode 1672 + const 正確性 -----
 * 最富有: 13
 * ----- 範例 4：日常實用 - ReadOnlyView -----
 * 庫存共 3 項，首項: 螺絲
 */

/*=============================================================================
 * 【本篇重點回顧】
 *   1. const 成員函式：在函式宣告右大括號前加 const，承諾不改物件。
 *   2. const 物件 / const& 只能呼叫 const 成員函式。
 *   3. 「該標 const 就標 const」是好的習慣，能讓 API 對 const 友善。
 *   4. 同名的 const 與非 const 函式可以多載，常用於回傳引用的 getter。
 *   5. mutable 修飾的成員，可以在 const 成員函式內被修改 (用於計數、cache 等)。
 *
 * 【下一篇預告】
 *   11_StaticMember.cpp
 *   static 成員 — 屬於「整個類別」而不是某個物件的東西，
 *   並用 Leetcode 1396. Design Underground System 作為應用情境。
 *=============================================================================*/
