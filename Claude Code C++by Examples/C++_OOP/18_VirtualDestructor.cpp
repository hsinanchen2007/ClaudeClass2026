/*=============================================================================
 * 檔名：18_VirtualDestructor.cpp
 * 主題：虛擬解構子 (Virtual Destructor) - 父類別解構子為什麼幾乎都要 virtual
 * 適合：學完繼承與 virtual function，準備搞清楚一個常見大坑的人
 *
 * 【課題介紹】
 *   想像一個常見場景：
 *
 *       Animal* a = new Dog("旺財", 3);
 *       // ... 用 a 操作 dog ...
 *       delete a;        // 我以為這樣就會把 dog 整個釋放
 *
 *   問題：如果 Animal 的解構子「不是」virtual，C++ 看到 a 的型別是 Animal*，
 *   就會「只」呼叫 Animal::~Animal()，「不會」呼叫 Dog::~Dog()。
 *   結果：Dog 自己擁有的資源 (例如 Dog 內部的 vector / 動態配置) 沒被釋放，
 *   形成「資源洩漏 (resource leak)」。嚴重時還會 undefined behavior。
 *
 *   解法：把父類別解構子標 virtual，C++ 就會在 delete 時走動態 dispatch，
 *   依物件實際型別呼叫正確順序的解構子 (Dog::~Dog → Animal::~Animal)。
 *
 *       「只要這個類別會被別人繼承、且可能透過父類別指標 delete，
 *        就把它的解構子寫成 virtual。」
 *
 *   實務上：「只要打算被當父類別」就標 virtual，幾乎沒有錯。
 *
 * 【為什麼預設不是 virtual？】
 *   因為 virtual 會讓物件多一個 vptr，多 8 bytes 開銷。對單純的「pure data」型別
 *   (例如 std::pair, std::string 內部 helper)，加 virtual 是浪費。
 *   C++ 的設計哲學是「不付不需要的代價」(zero-overhead principle)。
 *
 * 【用 = default 的好習慣】
 *   現代 C++ 常這樣寫：
 *       virtual ~Animal() = default;
 *   意思是「我什麼都不需要在解構子裡做，但我要它是 virtual」。
 *   既清楚，又不會少做事。
 *
 * 【小提醒：純虛擬解構子 (有實作)】
 *   有時你想讓一個類別變抽象 (不能 new) 但又沒有別的純虛擬函式能放，
 *   那就用「純虛擬解構子 + 給實作」：
 *       virtual ~Animal() = 0;
 *       Animal::~Animal() {}
 *   注意「= 0」表示純虛擬，但仍然必須給實作 (子類別解構結束後一定會呼叫到它)。
 *
 * 【日常實用】
 *   這個範例會用一個「會配置記憶體的子類別」 + 「父類別有/無 virtual 解構子」的
 *   兩種版本，讓你直接看到「資源沒被釋放」的差別。
 *
 * 【對應 Leetcode】1480. Running Sum of 1d Array (用多型實作 + 容器自動釋放)
 *   為什麼選這題：用一個 IRunningOp 抽象類別示範「多型容器一定要 virtual 解構子」。
 *   我們把 std::vector<std::unique_ptr<IRunningOp>> 當作運算管線，每個元素都
 *   是介面型別，銷毀時必須走到具體子類別的解構子才能正確釋放。
 *   這是「為什麼父類別解構子要 virtual」最常見的實際情境。
 *
 * 【參考】
 *   https://en.cppreference.com/w/cpp/language/destructor   (Virtual destructors 段落)
 *   https://cplusplus.com/doc/tutorial/polymorphism/
 *=============================================================================*/

/*
補充筆記：VirtualDestructor
  - 若會透過 base pointer 刪除 derived 物件，base destructor 必須是 virtual。
  - 沒有 virtual destructor 時，derived 資源可能不會釋放，行為可能未定義。
  - 有 virtual function 的 polymorphic base 幾乎都應明確考慮 virtual destructor。
  - 當 base class 可能被透過 Base* delete 時，base destructor 必須是 virtual，否則 derived destructor 不會被正確呼叫。
  - virtual destructor 讓 delete basePtr 時先呼叫最底層 derived destructor，再一路往上呼叫 base destructor。
  - 若 base class 不打算被多型刪除，可以把 destructor 設為 protected non-virtual，讓外部不能透過 base pointer delete。
  - 只要 class 有任何 virtual function，幾乎就表示它是 polymorphic base；此時 virtual destructor 是基本安全規則。
  - defaulted virtual destructor 寫法 virtual ~Base() = default; 很常見，表示需要 virtual 語意但沒有額外清理邏輯。
  - 解構子內不要呼叫需要 derived 行為的 virtual function，因為 derived 部分已經開始解構，不再是完整 derived 物件。
  - 智慧指標也不能替你修正錯誤的 base destructor；std::unique_ptr<Base> 刪除 Derived 仍需要 Base destructor virtual。
  - 學到這裡要把「繼承」和「所有權」一起想：誰負責 delete，就必須看到正確的解構介面。
*/
#include <iostream>
#include <string>
#include <memory>
#include <vector>

// -----------------------------------------------------------------------------
// (反例) 父類別解構子「不是」virtual → 子類別解構子不會被呼叫
// -----------------------------------------------------------------------------
class BadBase {
public:
    BadBase()           { std::cout << "  BadBase() 建構\n"; }
    ~BadBase()          { std::cout << "  ~BadBase() 解構\n"; }   // 注意沒有 virtual
};

class BadDerived : public BadBase {
private:
    int* big_;          // 假裝這是動態配置、需要在解構子釋放的資源
public:
    BadDerived() : big_(new int[100]) {
        std::cout << "  BadDerived() 建構，配置陣列\n";
    }
    ~BadDerived() {
        std::cout << "  ~BadDerived() 解構，釋放陣列\n";
        delete[] big_;
    }
};

// -----------------------------------------------------------------------------
// (好範例) 父類別解構子是 virtual → 兩個解構子都會被呼叫
// -----------------------------------------------------------------------------
class GoodBase {
public:
    GoodBase()                  { std::cout << "  GoodBase() 建構\n"; }
    virtual ~GoodBase()         { std::cout << "  ~GoodBase() 解構\n"; }
};

class GoodDerived : public GoodBase {
private:
    int* big_;
public:
    GoodDerived() : big_(new int[100]) {
        std::cout << "  GoodDerived() 建構，配置陣列\n";
    }
    ~GoodDerived() override {
        std::cout << "  ~GoodDerived() 解構，釋放陣列\n";
        delete[] big_;
    }
};

int main() {
    std::cout << "===== 反例：父類別解構子非 virtual =====" << std::endl;
    {
        BadBase* p = new BadDerived;
        // 透過父類別指標 delete → 因為解構子非 virtual，
        // 只會呼叫 ~BadBase()，~BadDerived() 不會跑 → big_ 沒被 delete[] → 洩漏
        delete p;
        std::cout << "(注意上面看不到 ~BadDerived()，記憶體洩漏！)\n";
    }

    std::cout << "===== 好範例：父類別解構子為 virtual =====" << std::endl;
    {
        GoodBase* p = new GoodDerived;
        delete p;
        // 預期看到先 ~GoodDerived()，再 ~GoodBase()，順序正確且資源完整釋放
    }

    std::cout << "===== 直接以子類別型別操作則沒事 =====" << std::endl;
    {
        BadDerived d;
        // 離開區塊時，編譯器知道 d 真實型別是 BadDerived，
        // 會正確呼叫兩個解構子。問題只發生在「透過父類別指標 delete」
    }

    std::cout << "===== Leetcode 1480 變體：多型容器 + virtual dtor =====" << std::endl;
    // 設計一個小框架：vector<unique_ptr<IRunningOp>> 串接多種「逐元素運算」，
    // 每種運算是一個子類別，內部可能持有自己的資源。
    // 因為 IRunningOp 解構子是 virtual，unique_ptr 在解構時會走具體子類別的 dtor。
    struct IRunningOp {
        virtual int apply(int acc, int x) const = 0;
        virtual ~IRunningOp() = default;        // ★ 必須 virtual ★
    };
    struct AddOp : IRunningOp {
        int apply(int acc, int x) const override { return acc + x; }
    };
    struct MaxOp : IRunningOp {
        int apply(int acc, int x) const override { return acc > x ? acc : x; }
    };

    std::vector<std::unique_ptr<IRunningOp>> ops;
    ops.push_back(std::make_unique<AddOp>());
    ops.push_back(std::make_unique<MaxOp>());

    // LC 1480 風格：對 nums 跑 running 累計
    std::vector<int> nums = {1, 2, 3, 4};
    int acc = 0;
    std::cout << "Running Sum: ";
    for (int x : nums) { acc = ops[0]->apply(acc, x); std::cout << acc << " "; }
    std::cout << "\n";
    int cur = nums[0];
    std::cout << "Running Max: " << cur;
    for (size_t i = 1; i < nums.size(); ++i) {
        cur = ops[1]->apply(cur, nums[i]);
        std::cout << " " << cur;
    }
    std::cout << "\n";
    // ops 在這裡解構 → unique_ptr 透過 IRunningOp* 呼叫 delete →
    // 因為 ~IRunningOp() 是 virtual，AddOp/MaxOp 各自的 dtor 都會被叫到。

    std::cout << "===== Leetcode 1672 - 多型容器處理顧客財富 =====" << std::endl;
    // 用 IWealthOp 抽象介面 + 兩種子類別 (Sum, Max)
    // 子類別內部持有 std::string label_ (RAII 物件)，
    // 仰賴 virtual 解構子才能正確釋放。
    struct IWealthOp {
        virtual int compute(const std::vector<int>& row) const = 0;
        virtual std::string label() const = 0;
        virtual ~IWealthOp() = default;
    };
    struct SumOp : IWealthOp {
        std::string label_ = "總和";
        int compute(const std::vector<int>& row) const override {
            int s = 0; for (int x : row) s += x; return s;
        }
        std::string label() const override { return label_; }
    };
    struct MaxRowOp : IWealthOp {
        std::string label_ = "最大值";
        int compute(const std::vector<int>& row) const override {
            int m = row.empty() ? 0 : row[0];
            for (int x : row) if (x > m) m = x;
            return m;
        }
        std::string label() const override { return label_; }
    };

    std::vector<std::unique_ptr<IWealthOp>> wops;
    wops.push_back(std::make_unique<SumOp>());
    wops.push_back(std::make_unique<MaxRowOp>());

    std::vector<int> row = {1, 5, 3, 9};
    for (const auto& op : wops) {
        std::cout << op->label() << " = " << op->compute(row) << std::endl;
    }
    // wops 解構時，每個 std::string label_ 都會被正確釋放。

    std::cout << "===== 日常實用：Plugin 系統 =====" << std::endl;
    // 工作上 plugin / 處理管線常用多型容器；virtual 解構子保證 cleanup 正確。
    struct IPlugin {
        virtual void run() = 0;
        virtual ~IPlugin() { std::cout << "  ~IPlugin (base)\n"; }
    };
    struct LogPlugin : IPlugin {
        std::string buffer_;
        void run() override { buffer_ = "logged"; std::cout << "LogPlugin run\n"; }
        ~LogPlugin() override { std::cout << "  ~LogPlugin (清空 buffer_=\"" << buffer_ << "\")\n"; }
    };

    std::unique_ptr<IPlugin> plugin = std::make_unique<LogPlugin>();
    plugin->run();
    // plugin 在 main 結束時被釋放，會先呼叫 ~LogPlugin，再呼叫 ~IPlugin
    return 0;
}

/* 預期輸出：
 * ===== 反例：父類別解構子非 virtual =====
 *   BadBase() 建構
 *   BadDerived() 建構，配置陣列
 *   ~BadBase() 解構
 * (注意上面看不到 ~BadDerived()，記憶體洩漏！)
 * ===== 好範例：父類別解構子為 virtual =====
 *   GoodBase() 建構
 *   GoodDerived() 建構，配置陣列
 *   ~GoodDerived() 解構，釋放陣列
 *   ~GoodBase() 解構
 * ===== 直接以子類別型別操作則沒事 =====
 *   BadBase() 建構
 *   BadDerived() 建構，配置陣列
 *   ~BadDerived() 解構，釋放陣列
 *   ~BadBase() 解構
 * ===== Leetcode 1480 變體：多型容器 + virtual dtor =====
 * Running Sum: 1 3 6 10
 * Running Max: 1 2 3 4
 * ===== Leetcode 1672 - 多型容器處理顧客財富 =====
 * 總和 = 18
 * 最大值 = 9
 * ===== 日常實用：Plugin 系統 =====
 * LogPlugin run
 *   ~LogPlugin (清空 buffer_="logged")
 *   ~IPlugin (base)
 */

/*=============================================================================
 * 【本篇重點回顧】
 *   1. 父類別解構子要設成 virtual，才能在 delete (父類別指標) 時正確走子類別解構子。
 *   2. 不寫 virtual → 子類別資源不會被釋放，造成洩漏甚至未定義行為。
 *   3. 沒實際工作要做的解構子，可以寫 virtual ~Class() = default;
 *   4. 經驗法則：「打算當父類別」的類別，就把解構子標 virtual，幾乎不會錯。
 *   5. 編譯器若編成 -Wnon-virtual-dtor 警告，可以幫你抓這類問題。
 *
 * 【下一篇預告】
 *   19_RAII.cpp
 *   RAII (Resource Acquisition Is Initialization) — 把資源管理交給 C++ 物件生命週期，
 *   是現代 C++ 防洩漏 / 防忘記釋放最重要的觀念。
 *=============================================================================*/
