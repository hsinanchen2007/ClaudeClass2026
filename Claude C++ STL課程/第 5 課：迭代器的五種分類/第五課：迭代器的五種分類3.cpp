#include <iostream>
#include <iterator>
#include <vector>
#include <algorithm>
 
int main() {
    std::vector<int> numbers = {10, 20, 30, 40, 50};
    
    // ostream_iterator 是 Output Iterator
    std::cout << "=== ostream_iterator ===" << std::endl;
    std::cout << "輸出: ";

    // ostream_iterator 讓你可以「寫入」到輸出串流
    std::copy(numbers.begin(), numbers.end(),
              std::ostream_iterator<int>(std::cout, " "));
    std::cout << std::endl;
    
    // back_inserter 也是 Output Iterator
    std::cout << "\n=== back_inserter ===" << std::endl;
    std::vector<int> source = {1, 2, 3};
    std::vector<int> dest;
    
    // back_inserter 讓你可以「寫入」到 vector 尾端
    std::copy(source.begin(), source.end(),
              std::back_inserter(dest));
    
    std::cout << "dest: ";
    for (int n : dest) std::cout << n << " ";
    std::cout << std::endl;
    
    return 0;
}
 