#include <iostream>
#include <vector>

int main() {
    std::vector<bool> vb = {true, false, true, true, false};

    for (bool b : vb) {
        std::cout << b << " ";
    }
    std::cout << std::endl;

    return 0;
}
