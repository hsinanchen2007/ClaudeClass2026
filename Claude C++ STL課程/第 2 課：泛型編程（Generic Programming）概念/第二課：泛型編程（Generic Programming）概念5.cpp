#include <iostream>
#include <string>

template <typename T1, typename T2>
void print_pair(T1 first, T2 second) {
    std::cout << "(" << first << ", " << second << ")" << std::endl;
}

int main() {
    print_pair(1, 3.14);                    // int, double
    print_pair("Name", 42);                 // const char*, int
    print_pair(std::string("Hello"), 'W');  // string, char
    
    return 0;
}
