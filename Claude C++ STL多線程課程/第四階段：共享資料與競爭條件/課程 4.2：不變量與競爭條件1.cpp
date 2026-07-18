#include <iostream>

struct Node {
    int data;
    Node* prev = nullptr;
    Node* next = nullptr;
};

void insertAfter(Node* a, Node* newNode, Node* c) {
    // 不變量暫時被破壞，但沒關係
    // 因為沒有其他人會看到中間狀態
    newNode->next = c;
    newNode->prev = a;
    a->next = newNode;
    c->prev = newNode;
    // 不變量恢復
}

int main() {
    Node a{1}, b{2}, c{3};
    a.next = &c;
    c.prev = &a;
    
    insertAfter(&a, &b, &c);  // 安全：單執行緒
    
    std::cout << a.next->data << std::endl;  // 2
    return 0;
}
