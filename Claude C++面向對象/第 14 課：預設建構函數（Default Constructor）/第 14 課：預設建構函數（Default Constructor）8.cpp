class NoDefault {
public:
    NoDefault(int x) { }  // 只有帶參建構函數
};

int main() {
    // NoDefault arr[3];   // 編譯錯誤！沒有預設建構函數！
    
    // 解決方法：用初始化列表逐一初始化
    // 這裡我們直接給出帶參建構函數的參數，來初始化陣列中的每個元素
    // 這樣就不需要預設建構函數了，因為我們在創建陣列的同時就給每個元素提供了初始化參數
    // 注意：這種方式只能在定義陣列的同時進行初始化，不能先定義陣列再賦值
    // 這裡創建了一個 NoDefault 類型的陣列，並且每個元素都使用帶參建構函數來初始化
    NoDefault arr[3] = { NoDefault(1), NoDefault(2), NoDefault(3) };  // OK
    return 0;
}
