#include <iostream>
#include <thread>

struct Node {
    int data;
    Node* prev = nullptr;
    Node* next = nullptr;
};

Node a{1}, b{2}, c{3};

void writer() {
    // 插入 b 到 a 和 c 之間
    b.next = &c;
    b.prev = &a;
    // ← 此時另一個執行緒可能讀取！
    a.next = &b;
    c.prev = &b;
}

void reader() {
    // 嘗試遍歷鏈結串列
    Node* current = &a;
    while (current != nullptr) {
        std::cout << current->data << " ";
        current = current->next;
    }
    std::cout << std::endl;
}

int main() {
    a.next = &c;
    c.prev = &a;
    
    std::thread t1(writer);
    std::thread t2(reader);
    
    t1.join();
    t2.join();
    
    return 0;
}
