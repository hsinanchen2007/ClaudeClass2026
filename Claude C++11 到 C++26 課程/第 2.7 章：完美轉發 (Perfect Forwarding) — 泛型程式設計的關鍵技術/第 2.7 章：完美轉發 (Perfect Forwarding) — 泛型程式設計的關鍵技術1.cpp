#include <iostream>
#include <memory>
#include <string>
#include <utility>

class Connection {
    std::string host_;
    int port_;
    std::string protocol_;
public:
    template<typename H, typename P>
    Connection(H&& h, int p, P&& proto)
        : host_(std::forward<H>(h)), port_(p), protocol_(std::forward<P>(proto)) {
        std::cout << "  [建構] host=" << host_ << " port=" << port_
                  << " proto=" << protocol_ << "\n";
    }
    void print() const {
        std::cout << "  Connection(" << host_ << ":" << port_
                  << ", " << protocol_ << ")\n";
    }
};

// 一個函式涵蓋所有情況
template<typename... Args>
std::unique_ptr<Connection> make_connection(Args&&... args) {
    return std::unique_ptr<Connection>(
        new Connection(std::forward<Args>(args)...)
    );
}

int main() {
    std::string host = "example.com";
    std::string proto = "https";

    std::cout << "--- 全部左值 ---\n";
    auto c1 = make_connection(host, 8080, proto);
    c1->print();

    std::cout << "\n--- 全部右值 ---\n";
    auto c2 = make_connection(std::string("localhost"), 3000, std::string("http"));
    c2->print();

    std::cout << "\n--- 混合 ---\n";
    auto c3 = make_connection(host, 443, std::string("wss"));
    c3->print();

    return 0;
}
