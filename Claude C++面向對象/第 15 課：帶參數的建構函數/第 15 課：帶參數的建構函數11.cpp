#include <iostream>
#include <string>
using namespace std;

class HttpRequest {
private:
    string url;
    string method;
    int timeout;      // 秒
    bool followRedirect;

public:
    // 只有 url 是必需的，其他都有合理的預設值
    // 使用預設參數來簡化建構函數的使用
    // 預設 method 是 GET，預設 timeout 是 30 秒，預設 followRedirect 是 true
    // 這樣用戶可以根據需要只提供部分參數，剩下的會自動使用預設值
    // 預設參數必須從右到左依次提供，不能跳過中間的參數
    // 例如，如果要指定 timeout，但不想指定 method，就必須提供 method 的預設值
    // 這樣的設計使得建構函數非常靈活，適用於多種使用場景
    HttpRequest(const string& url, 
                const string& method = "GET",
                int timeout = 30,
                bool followRedirect = true)
        : url(url), method(method), timeout(timeout), 
          followRedirect(followRedirect) 
    {
        cout << "  創建請求: " << method << " " << url << endl;
    }

    void print() const {
        cout << "  [" << method << "] " << url 
             << " (timeout=" << timeout << "s"
             << ", redirect=" << (followRedirect ? "是" : "否") 
             << ")" << endl;
    }
};

int main() {
    cout << "=== 只提供 URL ===" << endl;
    HttpRequest r1("https://example.com");
    r1.print();
    
    cout << "\n=== 指定 Method ===" << endl;
    HttpRequest r2("https://api.example.com/data", "POST");
    r2.print();
    
    cout << "\n=== 指定 Method 和 Timeout ===" << endl;
    HttpRequest r3("https://slow-server.com", "GET", 120);
    r3.print();
    
    cout << "\n=== 全部指定 ===" << endl;
    HttpRequest r4("https://redirect.com", "GET", 10, false);
    r4.print();
    
    return 0;
}
