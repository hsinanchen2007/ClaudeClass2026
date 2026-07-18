#include <iostream>
#include <iterator>
#include <vector>
#include <list>
#include <forward_list>
 
// 查詢迭代器類別
template <typename Iterator>
void print_iterator_category(const std::string& name) {
    using category = typename std::iterator_traits<Iterator>::iterator_category;
    
    std::cout << name << " 的迭代器類別: ";
    
    if constexpr (std::is_same_v<category, std::input_iterator_tag>) {
        std::cout << "Input Iterator";
    } else if constexpr (std::is_same_v<category, std::output_iterator_tag>) {
        std::cout << "Output Iterator";
    } else if constexpr (std::is_same_v<category, std::forward_iterator_tag>) {
        std::cout << "Forward Iterator";
    } else if constexpr (std::is_same_v<category, std::bidirectional_iterator_tag>) {
        std::cout << "Bidirectional Iterator";
    } else if constexpr (std::is_same_v<category, std::random_access_iterator_tag>) {
        std::cout << "Random Access Iterator";
    }
    
    std::cout << std::endl;
}
 
int main() {
    print_iterator_category<std::vector<int>::iterator>("vector");
    print_iterator_category<std::list<int>::iterator>("list");
    print_iterator_category<std::forward_list<int>::iterator>("forward_list");
    print_iterator_category<int*>("原生指標");
    print_iterator_category<std::istream_iterator<int>>("istream_iterator");
    print_iterator_category<std::ostream_iterator<int>>("ostream_iterator");
    
    return 0;
}
 