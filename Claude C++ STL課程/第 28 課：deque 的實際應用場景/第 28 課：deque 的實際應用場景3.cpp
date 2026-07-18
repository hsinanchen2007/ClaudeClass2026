#include <iostream>
#include <deque>
#include <string>
#include <ctime>
using namespace std;

template <typename T>
class RingBuffer {
    deque<T> buffer;
    size_t maxSize;

public:
    RingBuffer(size_t max) : maxSize(max) {}

    void push(const T& item) {
        if (buffer.size() >= maxSize) {
            buffer.pop_front();    // 丟棄最舊的
        }
        buffer.push_back(item);    // 加入最新的
    }

    // 隨機存取：0 是最舊的，size()-1 是最新的
    const T& operator[](size_t i) const { return buffer[i]; }

    // 取得最新的
    const T& latest() const { return buffer.back(); }

    // 取得最舊的
    const T& oldest() const { return buffer.front(); }

    size_t size() const { return buffer.size(); }
    bool full() const { return buffer.size() >= maxSize; }

    // 遍歷（從舊到新）
    void forEach(void (*fn)(const T&)) const {
        for (const T& item : buffer) fn(item);
    }
};

int main() {
    // 最多保留 5 筆日誌
    RingBuffer<string> log(5);

    log.push("09:00 系統啟動");
    log.push("09:01 使用者登入");
    log.push("09:05 查詢資料庫");
    log.push("09:10 匯出報表");
    log.push("09:15 發送郵件");

    cout << "--- 5 筆日誌 ---" << endl;
    for (size_t i = 0; i < log.size(); i++) {
        cout << "  [" << i << "] " << log[i] << endl;
    }

    // 再加入 2 筆，最舊的 2 筆會被丟棄
    log.push("09:20 備份資料");
    log.push("09:25 系統更新");

    cout << "\n--- 新增 2 筆後 ---" << endl;
    for (size_t i = 0; i < log.size(); i++) {
        cout << "  [" << i << "] " << log[i] << endl;
    }

    cout << "\n最舊：" << log.oldest() << endl;
    cout << "最新：" << log.latest() << endl;

    return 0;
}
