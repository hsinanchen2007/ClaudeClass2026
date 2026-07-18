#include <iostream>
using namespace std;

class Buffer {
private:
    int* data_;
    int size_;

public:
    Buffer(int size) : size_(size), data_(new int[size]) {
        for (int i = 0; i < size; i++) data_[i] = i;
        cout << "  [建構] 大小:" << size_ << endl;
    }

    ~Buffer() {
        delete[] data_;
        cout << "  [解構]" << endl;
    }

    // 拷貝賦值運算子——必須檢查自我賦值
    Buffer& operator=(const Buffer& other) {
        cout << "  [賦值] ";

        // 自我賦值檢查：this == &other 嗎？
        if (this == &other) {
            cout << "偵測到自我賦值，跳過" << endl;
            return *this;
        }

        // 正常賦值邏輯
        delete[] data_;
        size_ = other.size_;
        data_ = new int[size_];
        for (int i = 0; i < size_; i++) data_[i] = other.data_[i];
        cout << "完成" << endl;

        return *this;
    }

    void print() const {
        cout << "  [";
        for (int i = 0; i < size_; i++) {
            if (i > 0) cout << ", ";
            cout << data_[i];
        }
        cout << "]" << endl;
    }
};

int main() {
    cout << "=== 場景四：自我賦值檢查 ===" << endl;

    Buffer buf(5);
    buf.print();

    // 自我賦值：buf = buf
    buf = buf;   // this == &other，被安全攔截
    buf.print();

    return 0;
}
