/*=============================================================================
 * 檔名：6_ThisPointer.cpp
 * 主題：this 指標 - 物件如何「指到自己」
 * 適合：能寫類別、開始想知道 C++ 內部是怎麼搞清楚「我是哪個物件」的人
 *
 * 【課題介紹】
 *   想像有 100 個 BankAccount 物件，每一個都有自己的 deposit() 函式。
 *   問題：當你寫
 *
 *       acc1.deposit(100);
 *       acc2.deposit(50);
 *
 *   兩個呼叫表面上都是 deposit()，C++ 怎麼知道 acc1 修改的是 acc1.balance_、
 *   acc2 修改的是 acc2.balance_？
 *
 *   答案：每個成員函式被呼叫時，C++ 在背後偷偷多塞一個指標參數，
 *         指向「呼叫它的那個物件」。這個指標就叫 this。
 *
 *       「在成員函式內，this 是一個指向當前物件的指標。」
 *
 *   你看到的：     acc1.deposit(100);
 *   C++ 心裡的：    deposit(&acc1, 100);   ← this = &acc1
 *
 * 【this 的型別】
 *   - 在 ClassName 的成員函式內，this 的型別是 ClassName*。
 *   - 在 const 成員函式內，this 的型別是 const ClassName*  (不能改成員)。
 *   - this 不能被指派 (this = ...)，它是個常數指標 (T* const)。
 *
 * 【this 三個常見用途】
 *   1. 解決「參數名 == 成員名」造成的命名衝突
 *      void setName(std::string name) { this->name = name; }
 *
 *   2. 回傳 *this，讓函式可以「鏈式呼叫 (method chaining)」
 *      obj.setX(1).setY(2).setZ(3);
 *      設計這種 API 的關鍵就是讓 setter 回傳 *this 的 reference。
 *
 *   3. 自我參考、把自己加入容器、自我比較等場景
 *      if (this == &other) return;   // 防止自我賦值 (第 9 篇會用到)
 *
 * 【對應 Leetcode】1768. Merge Strings Alternately
 *   題目：給 word1, word2，交替拼起來，較長那邊剩下的接在後面。
 *         例：word1 = "abc", word2 = "pqr" → "apbqcr"
 *         例：word1 = "ab",  word2 = "pqrs" → "apbqrs"
 *   為什麼選這題：邏輯簡單，但很適合用「鏈式呼叫」的 setter 介面去設計，
 *   能順便練到 this 與 *this 回傳的觀念。
 *
 * 【參考】
 *   https://en.cppreference.com/w/cpp/language/this
 *=============================================================================*/

/*
補充筆記：ThisPointer
  - ThisPointer 這類 OOP 範例要追蹤物件狀態：建構後是否有效、操作後是否仍符合類別承諾。
  - 如果類別擁有資源，就要檢查 destructor、copy、move 是否表達同一套所有權規則。
  - 繼承、friend、static、operator overload 都應服務於清楚的物件語意，而不是只展示語法。
  - this 是每個非 static 成員函式裡隱含存在的指標，指向目前正在操作的那個物件；a.f() 時 this 指向 a，b.f() 時 this 指向 b。
  - 在 const 成員函式中，this 的型別近似 const Class* const，因此不能透過 this 修改一般成員；這也是 const 正確性的基礎。
  - 當參數名稱和成員名稱相同時，可用 this->member 區分；更常見做法是成員採用 name_ 命名，讓程式不用到處寫 this->。
  - return *this 常用於支援鏈式呼叫，例如 obj.setA(1).setB(2)；回傳型別通常是 Class&，避免複製並持續操作同一個物件。
  - static 成員函式沒有 this，因為它不屬於任何單一物件；它只能直接存取 static 成員或透過物件參數存取一般成員。
  - 在建構子或解構子把 this 傳出去很危險，因為物件可能還沒完全建好或已開始拆掉，外部若呼叫 virtual 或讀成員容易得到錯誤假設。
  - lambda 捕獲 this 只捕獲指標，不會延長物件生命週期；若 lambda 被延後執行，物件已死亡就會 dangling。
  - this 不能被重新指定，因為它代表目前呼叫目標；若需要讓物件改指向別的資料，應該改成員指標或使用 handle 類型。
*/
#include <iostream>
#include <string>

// -----------------------------------------------------------------------------
// 範例 1：用 this-> 解決命名衝突
// -----------------------------------------------------------------------------
class Person {
private:
    std::string name;
    int         age;

public:
    // 故意讓「參數名」與「成員變數名」一樣，需要靠 this-> 區分
    void setInfo(std::string name, int age) {
        this->name = name;     // 左邊是「成員」name，右邊是「參數」name
        this->age  = age;      // 同上
    }

    void show() const {
        // 在 const 成員函式內，this 的型別是 const Person*
        std::cout << "Person(" << this->name << ", " << this->age << ")"
                  << " 物件位址 = " << this << std::endl;
    }
};

// -----------------------------------------------------------------------------
// 範例 2：回傳 *this 達成鏈式呼叫 (Builder pattern 雛形)
// -----------------------------------------------------------------------------
class Pizza {
private:
    std::string base_  = "薄餅";
    std::string sauce_ = "番茄";
    std::string cheese_= "莫札瑞拉";

public:
    // 重點：回傳型別是 Pizza&  (參考型別)
    // 為什麼是 &？因為我們要回傳「同一個物件本身」，不是它的複製品。
    Pizza& setBase  (const std::string& s) { base_   = s; return *this; }
    Pizza& setSauce (const std::string& s) { sauce_  = s; return *this; }
    Pizza& setCheese(const std::string& s) { cheese_ = s; return *this; }

    void show() const {
        std::cout << "Pizza{餅:" << base_
                  << ", 醬:" << sauce_
                  << ", 起司:" << cheese_ << "}" << std::endl;
    }
};

// -----------------------------------------------------------------------------
// 範例 3：對應 Leetcode 1768 - Merge Strings Alternately
// -----------------------------------------------------------------------------
// 我們把「兩個字串 + 交替合併」做成一個類別，
// 並順便用鏈式介面 (setter 回傳 *this) 體驗 this 的價值。
class StringMerger {
private:
    std::string word1_;
    std::string word2_;

public:
    StringMerger& setWord1(const std::string& w) { this->word1_ = w; return *this; }
    StringMerger& setWord2(const std::string& w) { this->word2_ = w; return *this; }

    // 核心邏輯：交替取字元，較長那邊剩下的直接接到尾巴
    std::string merge() const {
        std::string result;
        // 先預留空間 (效能小技巧)，避免反覆配置記憶體
        result.reserve(word1_.size() + word2_.size());

        size_t i = 0;
        size_t n1 = word1_.size();
        size_t n2 = word2_.size();
        // 兩條都還有字元時，就一個一個交替放進去
        while (i < n1 && i < n2) {
            result.push_back(word1_[i]);
            result.push_back(word2_[i]);
            ++i;
        }
        // 把較長的那條剩下的部分一次接上去
        if (i < n1) result.append(word1_, i, std::string::npos);
        if (i < n2) result.append(word2_, i, std::string::npos);
        return result;
    }
};

// -----------------------------------------------------------------------------
// 範例 4：對應 Leetcode 1108 - Defanging an IP Address
// -----------------------------------------------------------------------------
// 題目簡述：把 IP 字串中的每個 "." 變成 "[.]"。例如 "1.1.1.1" → "1[.]1[.]1[.]1"。
// 用一個類別 + 鏈式 setter 體驗 *this 的回傳。
class IpDefanger {
private:
    std::string ip_;

public:
    IpDefanger& setIp(const std::string& ip) {
        this->ip_ = ip;            // this-> 區分參數 vs 成員
        return *this;              // 回傳 *this 支援鏈式呼叫
    }

    std::string defang() const {
        std::string out;
        out.reserve(ip_.size() + 6);   // 每個 . 多 2 個字元，最多 3 個點
        for (char c : ip_) {
            if (c == '.') out += "[.]";
            else          out.push_back(c);
        }
        return out;
    }
};

// -----------------------------------------------------------------------------
// 範例 5：日常實用 - HttpRequestBuilder (建構模式)
// -----------------------------------------------------------------------------
// 工作上 HTTP 請求物件常用鏈式 setter 來組裝參數。
class HttpRequestBuilder {
private:
    std::string method_ = "GET";
    std::string url_;
    std::string body_;

public:
    HttpRequestBuilder& setMethod(const std::string& m) { method_ = m; return *this; }
    HttpRequestBuilder& setUrl   (const std::string& u) { url_    = u; return *this; }
    HttpRequestBuilder& setBody  (const std::string& b) { body_   = b; return *this; }

    void send() const {
        std::cout << method_ << " " << url_;
        if (!body_.empty()) std::cout << " body=" << body_;
        std::cout << std::endl;
    }
};

int main() {
    std::cout << "----- 範例 1：用 this-> 解命名衝突 -----" << std::endl;
    Person alice, bob;
    alice.setInfo("Alice", 30);
    bob.setInfo  ("Bob",   25);
    alice.show();
    bob.show();
    std::cout << "(注意 alice 跟 bob 的物件位址不一樣)" << std::endl;

    std::cout << "----- 範例 2：鏈式呼叫 -----" << std::endl;
    Pizza p;
    // 因為每個 setter 都回傳 *this，可以這樣串起來：
    p.setBase("厚片").setSauce("白醬").setCheese("切達");
    p.show();

    std::cout << "----- 範例 3：Leetcode 1768 -----" << std::endl;
    StringMerger sm;

    // 案例 A：等長
    std::cout << sm.setWord1("abc").setWord2("pqr").merge() << std::endl;   // apbqcr

    // 案例 B：word2 較長
    std::cout << sm.setWord1("ab").setWord2("pqrs").merge() << std::endl;   // apbqrs

    // 案例 C：word1 較長
    std::cout << sm.setWord1("abcd").setWord2("pq").merge() << std::endl;   // apbqcd

    std::cout << "----- 範例 4：Leetcode 1108 Defanging IP -----" << std::endl;
    IpDefanger d;
    std::cout << d.setIp("1.1.1.1").defang()        << std::endl;   // 1[.]1[.]1[.]1
    std::cout << d.setIp("255.100.50.0").defang()   << std::endl;   // 255[.]100[.]50[.]0

    std::cout << "----- 範例 5：HttpRequestBuilder -----" << std::endl;
    HttpRequestBuilder req;
    req.setMethod("POST").setUrl("/api/login").setBody("{\"u\":\"a\"}").send();
    req.setMethod("GET").setUrl("/api/me").setBody("").send();

    return 0;
}

/* 預期輸出（物件位址每次執行可能不同）：
 * ----- 範例 1：用 this-> 解命名衝突 -----
 * Person(Alice, 30) 物件位址 = 0x...
 * Person(Bob, 25) 物件位址 = 0x... (與 Alice 不同)
 * (注意 alice 跟 bob 的物件位址不一樣)
 * ----- 範例 2：鏈式呼叫 -----
 * Pizza{餅:厚片, 醬:白醬, 起司:切達}
 * ----- 範例 3：Leetcode 1768 -----
 * apbqcr
 * apbqrs
 * apbqcd
 * ----- 範例 4：Leetcode 1108 Defanging IP -----
 * 1[.]1[.]1[.]1
 * 255[.]100[.]50[.]0
 * ----- 範例 5：HttpRequestBuilder -----
 * POST /api/login body={"u":"a"}
 * GET /api/me
 */

/*=============================================================================
 * 【本篇重點回顧】
 *   1. 在成員函式裡，this 是「指向呼叫者物件」的指標。
 *   2. this 的型別會根據函式是否為 const 而變動 (const 成員函式 → const T*)。
 *   3. this->member 可以避免成員與參數同名造成的衝突。
 *   4. 回傳 *this (型別 ClassName&) 可以實作鏈式呼叫 API，介面更流暢。
 *   5. 第 9 篇講賦值運算子時，會用 if (this == &other) 來防止自我賦值。
 *
 * 【下一篇預告】
 *   7_InitializerList.cpp
 *   建構子初始化列表 (Member Initializer List) — 「: x(1), y(2)」這種寫法為什麼比
 *   在大括號裡面 x = 1; y = 2; 還要好，並用 Leetcode 535. TinyURL 練習。
 *=============================================================================*/
