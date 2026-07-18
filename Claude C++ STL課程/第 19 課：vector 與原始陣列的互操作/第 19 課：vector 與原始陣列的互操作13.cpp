#include <iostream>
#include <vector>
#include <cstring>

// 模擬一個作業系統 API（類似 Windows 的 GetComputerName 或 POSIX 的 read）
// 把字串寫入 buffer，回傳寫入的位元組數
int os_get_hostname(char* buffer, int buffer_size) {
    const char* hostname = "my-workstation";
    int len = std::strlen(hostname);

    if (buffer_size < len + 1) {
        return -1;  // 緩衝區太小
    }

    std::strcpy(buffer, hostname);
    return len;
}

int main() {
    // 用 vector<char> 當作動態緩衝區
    std::vector<char> buffer(256, '\0');

    int len = os_get_hostname(buffer.data(), buffer.size());

    if (len > 0) {
        // 可以直接用 buffer.data() 當作 C 字串
        std::cout << "主機名稱：" << buffer.data() << std::endl;
        std::cout << "長度：" << len << std::endl;

        // 也可以轉成 std::string
        std::string hostname(buffer.data(), len);
        std::cout << "std::string：" << hostname << std::endl;
    }

    return 0;
}
