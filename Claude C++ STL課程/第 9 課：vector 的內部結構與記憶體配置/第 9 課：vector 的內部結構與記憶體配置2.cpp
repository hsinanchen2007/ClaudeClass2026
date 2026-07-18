#include <vector>
#include <iostream>

int main() {
    std::vector<int> v;
    
    size_t prev_cap = 0;
    for (int i = 0; i < 100; ++i) {
        v.push_back(i);
        if (v.capacity() != prev_cap) {
            std::cout << "size: " << v.size() 
                      << ", capacity: " << v.capacity() << std::endl;
            prev_cap = v.capacity();
        }
    }
    
    return 0;
}
