// 問題：在傳統語法中，回傳型別寫在函式名稱前，
// 此時參數 a, b 還不存在，無法使用它們推導型別

// C++11 解法：後置回傳型別
template<typename T, typename U>
auto add(T a, U b) -> decltype(a + b)
{
    return a + b;
}

int main()
{
    auto result1 = add(1, 2);       // int
    auto result2 = add(1, 2.5);     // double
    auto result3 = add(1.5f, 2.5);  // double
    
    return 0;
}
