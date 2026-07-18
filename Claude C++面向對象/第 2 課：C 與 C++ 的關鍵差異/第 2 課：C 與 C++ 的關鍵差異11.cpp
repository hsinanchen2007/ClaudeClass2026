#include <stdio.h>

void greet(const char* name, const char* greeting) {
    printf("%s, %s!\n", greeting, name);
}

int main() {
    // 每次都必須提供所有參數
    greet("Alice", "Hello");
    greet("Bob", "Hello");  // 即使 greeting 常常是 "Hello"
    return 0;
}
