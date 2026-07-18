class Broken {
    int* data_;
public:
    Broken(int val) : data_(new int(val)) {}
    ~Broken() { delete data_; }
    // 只寫了解構子，其他四個都沒寫
};

int main() {
    Broken a(42);
    Broken b = a;    // 淺拷貝！a.data_ 和 b.data_ 指向同一塊記憶體
}   // a 和 b 都 delete 同一個指標 → double free → 程式崩潰
