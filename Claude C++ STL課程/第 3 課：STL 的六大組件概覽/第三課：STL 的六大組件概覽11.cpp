#include <iostream>
#include <vector>
#include <algorithm>
#include <functional>


int main() {
    std::vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    
    // std::bind：綁定參數, 這裡綁定了 std::greater<int>() 的第一個參數為 _1（代表待測試的元素），第二個參數為 5，
    //形成一個新的可調用對象 is_greater_than_5
    
    /**
     * @brief 演示 std::bind() 的用法和參數綁定機制
     * 
     * 本函數展示了如何使用 std::bind() 來綁定函數的參數，以創建新的可調用對象。
     * 
     * std::bind() 的工作原理：
     * - std::bind(func, args...) 返回一個新的可調用對象
     * - std::placeholders::_1, _2, ... 代表調用時傳入的參數位置
     * - 非佔位符的參數會被綁定為固定值
     * 
     * 示例解析：
     * - std::bind(std::greater<int>(), std::placeholders::_1, 5)
     *   綁定了 std::greater<int>() 的兩個參數：
     *   * 第一個參數為 _1（待測試的元素，調用時傳入）
     *   * 第二個參數為 5（綁定的固定值）
     * - 形成的可調用對象等價於：[](int n) { return n > 5; }
     * 
     * @note 雖然 std::bind() 功能強大，但在 C++11 之後，使用 Lambda 表達式
     *       通常更簡潔、更易讀，除非在需要複雜參數重排或多次複用時才推薦使用。
     * 
     * @return int 總是返回 0（程序成功執行）
     */
    auto is_greater_than_5 = std::bind(std::greater<int>(), std::placeholders::_1, 5);
    
    int count = std::count_if(vec.begin(), vec.end(), is_greater_than_5);
    std::cout << "大於5的個數: " << count << std::endl;
    
    // 等價的 Lambda 寫法（更推薦）
    int count2 = std::count_if(vec.begin(), vec.end(),
        [](int n) { return n > 5; }
    );
    std::cout << "大於5的個數（Lambda）: " << count2 << std::endl;
    
    return 0;
}
