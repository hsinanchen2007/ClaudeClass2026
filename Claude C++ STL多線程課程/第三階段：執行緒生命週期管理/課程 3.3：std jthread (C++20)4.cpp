#include <iostream>
#include <thread>

int main() {
    std::jthread jt([](std::stop_token stoken) {
        while (!stoken.stop_requested()) {
            std::cout << "Running..." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        }
    });
    
    // 取得 stop_source
    // ⚠️ 更正：get_stop_source() 是【回傳值】,不是回傳 reference,
    //    寫成 std::stop_source& 會編譯失敗（non-const lvalue ref 綁不到 prvalue）。
    //    直接用 auto 接就好——stop_source 本身就是共享狀態的把手,複製不影響語意。
    auto source = jt.get_stop_source();
    
    // 取得 stop_token
    std::stop_token token = jt.get_stop_token();
    
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    // 兩種方式都可以請求停止
    // source.request_stop();
    jt.request_stop();
    
    return 0;
}
