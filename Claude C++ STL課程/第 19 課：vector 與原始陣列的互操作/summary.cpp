/*
 * ================================================================
 * 【第 19 課：vector 與原始陣列的互操作】總複習 summary.cpp
 * ================================================================
 * 編譯方式：g++ -std=c++17 -o summary summary.cpp
 *
 * 本課重點：
 * 1. data() —— 取得 vector 底層原始指標，與 C API 介接
 * 2. 從原始陣列建立 vector
 * 3. 將 vector 資料傳給需要 C 陣列的函數
 * 4. 指標算術與 vector 底層的關係
 * 5. 記憶體連續性保證（C++11 起正式保證）
 * 6. 安全注意事項：迭代器失效與指標失效
 * ================================================================
 */

#include <iostream>
#include <vector>
#include <algorithm>
#include <cstring>
#include <string>
using namespace std;

// ================================================================
// 重點一：data() 取得底層指標
// ================================================================
// vector<T>::data() 回傳 T*（或 const T* for const vector）
// 指向 vector 的第一個元素
// C++11 正式保證 vector 的元素在記憶體中是「連續」的
// 這讓 vector 可以直接替代 C 風格陣列

void demoData() {
    cout << "\n【data() 取得底層指標】" << endl;

    vector<int> v = {10, 20, 30, 40, 50};

    int* ptr = v.data();   // 取得底層指標
    cout << "v.data() = " << static_cast<void*>(ptr) << endl;
    cout << "&v[0]    = " << static_cast<void*>(&v[0]) << endl;
    cout << "兩者相同: " << (ptr == &v[0] ? "是" : "否") << endl;

    // 透過指標讀取元素（指標算術）
    cout << "ptr[0]=" << ptr[0] << ", ptr[2]=" << ptr[2] << ", ptr[4]=" << ptr[4] << endl;

    // 透過指標修改元素
    ptr[1] = 200;
    cout << "ptr[1] 改為 200，v[1] = " << v[1] << endl;  // 同步修改

    // const vector：data() 回傳 const T*
    const vector<int> cv = {1, 2, 3};
    const int* cptr = cv.data();
    cout << "const vector data()[0] = " << cptr[0] << endl;
    // cptr[0] = 99;  // 錯誤！const 指標不能修改
}

// ================================================================
// 重點二：傳給 C API 函數
// ================================================================
// 大量 C 函數（如 memcpy、POSIX API、網路 API）需要 T* 指標
// 用 v.data() 可以直接傳入，無需手動複製到 C 陣列

// 模擬 C 函數庫的函數
void c_print_array(const int* arr, int size) {
    printf("C 函數印出: ");
    for (int i = 0; i < size; ++i) {
        printf("%d ", arr[i]);
    }
    printf("\n");
}

void c_double_elements(int* arr, int size) {
    for (int i = 0; i < size; ++i) {
        arr[i] *= 2;
    }
}

void demoCInterface() {
    cout << "\n【與 C API 介接】" << endl;

    vector<int> v = {1, 2, 3, 4, 5};

    // 傳給 const int* 的函數（唯讀）
    c_print_array(v.data(), static_cast<int>(v.size()));

    // 傳給 int* 的函數（可修改）
    c_double_elements(v.data(), static_cast<int>(v.size()));
    cout << "乘以 2 後: ";
    for (int n : v) cout << n << " ";
    cout << endl;
}

// ================================================================
// 重點三：從原始陣列建立 vector
// ================================================================
// 三種方式將 C 陣列轉為 vector：
// 1. 迭代器範圍建構：vector<T>(arr, arr + size)
// 2. assign()
// 3. C++11 起用 begin(arr)/end(arr)

void demoArrayToVector() {
    cout << "\n【從原始陣列建立 vector】" << endl;

    int arr[] = {10, 20, 30, 40, 50};
    int size = sizeof(arr) / sizeof(arr[0]);

    // 方式一：迭代器範圍建構（最常用）
    vector<int> v1(arr, arr + size);
    cout << "方式一（迭代器範圍）: ";
    for (int n : v1) cout << n << " ";
    cout << endl;

    // 方式二：assign()
    vector<int> v2;
    v2.assign(arr, arr + size);
    cout << "方式二（assign）: ";
    for (int n : v2) cout << n << " ";
    cout << endl;

    // 方式三：C++11 std::begin / std::end
    vector<int> v3(begin(arr), end(arr));
    cout << "方式三（std::begin/end）: ";
    for (int n : v3) cout << n << " ";
    cout << endl;
}

// ================================================================
// 重點四：memcpy 與 vector 的配合
// ================================================================
// 由於 vector 記憶體連續，可以使用 memcpy 進行批量複製
// 注意：只適用於 trivially copyable 的類型（int、char 等）
// 對於有建構函數的複雜類型，應使用 std::copy 或迭代器

void demoMemcpy() {
    cout << "\n【memcpy 與 vector 配合】" << endl;

    vector<int> src = {1, 2, 3, 4, 5};
    vector<int> dst(src.size());

    // 用 memcpy 複製（只適用於 trivially copyable 類型）
    memcpy(dst.data(), src.data(), src.size() * sizeof(int));

    cout << "memcpy 複製結果: ";
    for (int n : dst) cout << n << " ";
    cout << endl;

    // 對於複雜類型，應使用 std::copy
    vector<string> words_src = {"hello", "world", "cpp"};
    vector<string> words_dst(words_src.size());
    copy(words_src.begin(), words_src.end(), words_dst.begin());
    cout << "std::copy 結果: ";
    for (const string& s : words_dst) cout << s << " ";
    cout << endl;
}

// ================================================================
// 重點五：span（C++20）—— 更安全的陣列視圖
// ================================================================
// std::span 提供對連續記憶體的非擁有視圖（view）
// 可以接受 vector、原生陣列、data()+size 等
// 避免傳遞「指標+大小」兩個分開的參數

// 使用傳統方式（指標 + 大小）
void printOldStyle(const int* data, size_t size) {
    for (size_t i = 0; i < size; ++i) cout << data[i] << " ";
    cout << endl;
}

void demoSpanLike() {
    cout << "\n【指標+大小 vs span 風格】" << endl;

    vector<int> v = {10, 20, 30, 40, 50};

    // 傳統：指標 + 大小（容易傳錯大小）
    printOldStyle(v.data(), v.size());

    // 現代（C++20 span）：單一參數包含指標和大小
    // span<int> s(v.data(), v.size());  // C++20 需要 #include <span>
    cout << "（C++20 std::span 提供更安全的介面）" << endl;
}

// ================================================================
// 重點六：指標失效警告
// ================================================================
// 重要！當 vector 重新分配記憶體時，data() 返回的指標會失效
// 任何導致 capacity 增加的操作都可能使指標失效：
//   - push_back（超過 capacity）
//   - insert
//   - resize（增大）
//   - reserve（增大）

void demoPointerInvalidation() {
    cout << "\n【指標失效警告】" << endl;

    vector<int> v = {1, 2, 3};
    int* ptr = v.data();
    cout << "v.data() 取得指標，ptr[0] = " << ptr[0] << endl;

    // 警告！push_back 可能導致重分配，使 ptr 失效
    v.push_back(4);  // 可能觸發重分配！

    // ptr 此時可能已失效！
    // cout << ptr[0];  // 危險的未定義行為！

    // 正確做法：push_back 後重新取指標
    int* new_ptr = v.data();
    cout << "重新取指標後 new_ptr[0] = " << new_ptr[0] << endl;

    cout << "規則：任何可能改變 capacity 的操作後，都要重新取 data()" << endl;
}

int main() {
    cout << "=============================================" << endl;
    cout << "   第 19 課：vector 與原始陣列的互操作" << endl;
    cout << "=============================================" << endl;

    demoData();
    demoCInterface();
    demoArrayToVector();
    demoMemcpy();
    demoSpanLike();
    demoPointerInvalidation();

    cout << "\n==============================================" << endl;
    cout << " 重點：vector 記憶體連續，可透過 data() 與 C API 互操作" << endl;
    cout << " 警告：重分配後指標失效，操作後需重新取 data()" << endl;
    cout << "==============================================" << endl;

    return 0;
}
