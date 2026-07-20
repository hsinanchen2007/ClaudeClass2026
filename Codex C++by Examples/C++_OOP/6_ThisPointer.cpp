// ============================================================================
// 課題 6：this pointer、名稱遮蔽與 fluent API
// ============================================================================
//
// 每個 non-static member function 都有隱含的 `this`，指向「本次被呼叫的物件」。
// 在 `box.set_width(3)` 中，set_width 內的 this 就是 &box。常見用途：
//   1. 區分同名參數：`this->value_ = value`。
//   2. 回傳目前物件：`return *this`，形成鏈式呼叫。
//   3. 把自己傳給合作物件，但必須小心生命週期。
//
// const member function 中 this 的型態效果近似 `const Class*`；static member function
// 不屬於任何 object，所以沒有 this。constructor/destructor 內雖有 this，但不要在
// 尚未完整建構或已部分解構時呼叫依賴 dynamic dispatch 的外部程式。
//
// 【面試】`return this` 回 pointer；`return *this` 回 object/reference，取決於簽章。
// 【陷阱】把 this 存到長壽 callback，物件先死後 callback 再用會 dangling。
// ============================================================================

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

class Rectangle {
public:
    Rectangle& set_width(int width)
    {
        this->width_ = width;  // 明寫 this 可讀，但沒有名稱衝突時通常省略。
        return *this;
    }

    Rectangle& set_height(int height)
    {
        this->height_ = height;
        return *this;
    }

    int area() const { return this->width_ * this->height_; }

private:
    int width_ = 0;
    int height_ = 0;
};

void basic_example()
{
    Rectangle rectangle;
    rectangle.set_width(4).set_height(3);  // 每次都回同一物件的 reference。
    assert(rectangle.area() == 12);
    std::cout << "[基礎] fluent Rectangle area=12\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1472. Design Browser History（設計瀏覽器歷史記錄）
// 題目：支援 visit、back、forward；新造訪會刪除 forward 分支，例如回退後造訪 linkedin 即不能再前進。
// 為何使用本章主題：每次 member call 的 this 指向同一 BrowserHistory，讓 history_ 與 current_ 一起維持狀態。
// 思路：visit 截斷目前位置後方再追加；back 將索引向零夾限；forward 將索引向尾端夾限。
// 複雜度：back/forward O(1)，visit 最壞 O(N) 因 resize/配置，空間 O(N)，N 為保留頁面數。
// 易錯點：steps 契約需非負；新 visit 必須清 forward history；回傳 const reference 只在物件與該元素未失效時有效。
// -----------------------------------------------------------------------------
class BrowserHistory {
public:
    explicit BrowserHistory(std::string homepage)
        : history_{std::move(homepage)}, current_(0U) {}

    void visit(std::string url)
    {
        history_.resize(current_ + 1U);  // 新分支會丟掉 forward history。
        history_.push_back(std::move(url));
        ++current_;
    }

    const std::string& back(int steps)
    {
        const std::size_t count = static_cast<std::size_t>(steps);
        current_ = count > current_ ? 0U : current_ - count;
        return history_.at(current_);
    }

    const std::string& forward(int steps)
    {
        const std::size_t count = static_cast<std::size_t>(steps);
        current_ = std::min(current_ + count, history_.size() - 1U);
        return history_.at(current_);
    }

private:
    std::vector<std::string> history_;
    std::size_t current_;
};

void leetcode_1472_example()
{
    BrowserHistory browser("leetcode.com");
    browser.visit("google.com");
    browser.visit("facebook.com");
    browser.visit("youtube.com");
    const std::string first_back = browser.back(1);
    const std::string second_back = browser.back(1);
    const std::string first_forward = browser.forward(1);
    assert(first_back == "facebook.com");
    assert(second_back == "google.com");
    assert(first_forward == "facebook.com");
    browser.visit("linkedin.com");
    const std::string after_new_visit = browser.forward(2);
    assert(after_new_visit == "linkedin.com");
    std::cout << "[LeetCode 1472] browser state 由同一個 this 維護\n";
}

// -----------------------------------------------------------------------------
// 【日常實務範例】HTTP 請求 URL 的鏈式建構器
// 情境：呼叫端依序設定 api.example.com 與 /v1/jobs，最後產生 https://api.example.com/v1/jobs。
// 為何使用本章主題：每個 setter 回傳 *this reference，可在同一物件上串接；build 標 const 表明只讀取 state。
// 設計：host/path 各自 move 進 member；回傳目前 builder；build 串接 scheme、host 與 path。
// 成本：設定與 build 為 O(H+P) 字串搬移/配置，H/P 為 host/path 長度。
// 上線注意：鏈式回傳是 borrow，不能超過物件生命；需驗空 host、路徑編碼與 URL injection，並避免共用可變 builder。
// -----------------------------------------------------------------------------
class RequestBuilder {
public:
    RequestBuilder& host(std::string host)
    {
        host_ = std::move(host);
        return *this;
    }
    RequestBuilder& path(std::string path)
    {
        path_ = std::move(path);
        return *this;
    }
    std::string build() const { return "https://" + host_ + path_; }

private:
    std::string host_;
    std::string path_ = "/";
};

void practical_example()
{
    RequestBuilder request;
    const std::string url = request.host("api.example.com").path("/v1/jobs").build();
    assert(url == "https://api.example.com/v1/jobs");
    std::cout << "[實務] " << url << '\n';
}

int main()
{
    basic_example();
    leetcode_1472_example();
    practical_example();
}

// 練習：讓 Rectangle 拒絕負數；思考 setter 回 reference 時 exception safety。
// 複雜度：取得/回傳 this 是 O(1)；fluent chain 的成本是各 member call 成本相加。
// 生命週期：回傳 `*this` 是 borrow，不能對 temporary chain 保存 reference 到 full-expression 之後。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '6_ThisPointer.cpp' -o '/tmp/codex_cpp_C_OOP_6_ThisPointer' && '/tmp/codex_cpp_C_OOP_6_ThisPointer'
//
// === 預期輸出（節錄）===
// [LeetCode 1472] browser state 由同一個 this 維護
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
