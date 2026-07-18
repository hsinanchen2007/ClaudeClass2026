/*
 * ================================================================
 * 【第18課：vector<bool> 的特殊性與陷阱】總複習 summary.cpp
 * ================================================================
 * 編譯方式：g++ -std=c++17 -o summary summary.cpp
 * 本課重點：
 * 1. vector<bool> 是模板特化：每個 bool 只佔 1 bit，節省 8 倍記憶體
 * 2. operator[] 回傳代理物件（proxy object），不是 bool&
 * 3. 陷阱一：auto 推導出 vector<bool>::reference，不是 bool
 * 4. 陷阱二：無法取得元素的 bool* 指標（&vb[0] 編譯錯誤）
 * 5. 陷阱三：無法將元素綁定到 bool& 引用
 * 6. 陷阱四：data() 成員函數不存在
 * 7. 陷阱五：破壞泛型模板程式碼的相容性
 * 8. 特有操作：flip()（單一元素翻轉、全部翻轉）
 * 9. 替代方案：deque<bool>、vector<uint8_t>、std::bitset
 * 10. 實務建議：何時用 vector<bool>，何時避開
 * ================================================================
 */

#include <iostream>
#include <vector>
#include <deque>
#include <bitset>
#include <cstdint>   // uint8_t
#include <typeinfo>  // typeid（型別名稱顯示，可能因平台而異）

// ===== 重點一：vector<bool> 是模板特化 =====
// C++ 標準對 vector<bool> 進行了「模板特化（template specialization）」。
// 普通 vector<T>：每個元素佔 sizeof(T) bytes
// vector<bool>：每個 bool 只佔 1 bit，用位元壓縮（bit packing）儲存
//
// 記憶體佈局比較：
//   普通 vector<int>：┌──┬──┬──┬──┐  每個 int 獨立佔 4 bytes
//                    │10│20│30│40│
//                    └──┴──┴──┴──┘
//
//   vector<bool>：   ┌─┬─┬─┬─┬─┬─┬─┬─┐  一個 byte 存 8 個 bool
//                    │1│0│1│1│0│0│1│0│
//                    └─┴─┴─┴─┴─┴─┴─┴─┘
//
// 記憶體節省：1000 個 bool 只需約 125 bytes，而 vector<char>(1000) 需要 1000 bytes

// ===== 重點二：代理物件（Proxy Object）=====
// 因為 C++ 不能有「指向單一位元」的引用或指標（最小可定址單位是 1 byte），
// vector<bool>::operator[] 無法回傳 bool&。
// 取而代之，它回傳一個 vector<bool>::reference「代理物件」。
// 代理物件通常包含：
//   - 指向底層 byte 的指標
//   - 一個位元偏移量（0~7）
//   透過這兩個資訊，可以定位到那個 bit，並進行讀/寫操作。
// 代理物件可隱式轉型為 bool，但它本身不是 bool，sizeof 也不是 1。

// ===== 重點三：五大陷阱詳解 =====

// 陷阱一：auto 推導出錯誤型別
//   auto val = vb[0];  → val 是 vector<bool>::reference（代理物件）
//                         不是 bool，因此後來修改 vb[0] 會影響 val！
//   修正：bool val = vb[0];  → 強制轉型，取得獨立 bool 副本

// 陷阱二：無法取元素位址
//   bool* pb = &vb[0];  → 編譯錯誤！
//   &vb[0] 是「代理物件的位址」，型別是 vector<bool>::reference*，
//   不能轉成 bool*。

// 陷阱三：無法綁定到 bool&
//   bool& ref = vb[0];     → 編譯錯誤！
//   set_true(vb[0]);       → 編譯錯誤！（若函數參數為 bool&）
//   代理物件可以隱式轉型為 bool（右值），但不能當作 bool& 的綁定目標。

// 陷阱四：data() 不可用
//   vb.data();  → 編譯錯誤！
//   vector<bool> 底層不是 bool 陣列，沒有 data() 成員函數。

// 陷阱五：泛型模板相容性問題
//   template<typename T> void process(std::vector<T>& v) {
//       T& elem = v[i];  // 對 bool 編譯錯誤！
//   }
//   本來對 vector<int> 完美運作的模板，碰到 vector<bool> 就壞掉了。

// ===== 重點四：特有操作 flip() =====
// vb[i].flip()  → 翻轉第 i 個位元（0→1 或 1→0）
// vb.flip()     → 翻轉整個 vector 中的所有位元

// ===== 重點五：替代方案比較 =====
// deque<bool>       → 沒有被特化，bool& 和 bool* 均可用，但記憶體效率低（每元素 1 byte）
// vector<uint8_t>   → 完整容器行為（引用、指標、data() 全可用）、泛型相容，但 1 byte/bool
// std::bitset<N>    → 位元壓縮（1 bit/bool），支援完整位元運算，但大小必須是編譯期常數

void demo_memory_efficiency() {
    std::cout << "\n--- 一、記憶體效率對比 ---" << std::endl;
    const int N = 1000;

    bool arr[N];
    std::cout << "bool[1000] 大小：" << sizeof(arr) << " bytes" << std::endl;

    std::vector<char> vc(N);
    std::cout << "vector<char>(1000)：約 "
              << vc.capacity() * sizeof(char) << " bytes" << std::endl;

    std::vector<bool> vb(N);
    std::cout << "vector<bool>(1000)：約 "
              << vb.capacity() / 8 << " bytes（節省約 8 倍）" << std::endl;
}

void demo_proxy_object() {
    std::cout << "\n--- 二、代理物件（Proxy Object）型別觀察 ---" << std::endl;
    std::vector<int> vi = {1, 2, 3};
    std::vector<bool> vb = {true, false, true};

    auto ri = vi[0];  // auto 推導為 int（普通值）
    auto rb = vb[0];  // auto 推導為 vector<bool>::reference（代理物件！）

    std::cout << "vi[0] 的型別（auto）：" << typeid(ri).name() << std::endl;
    std::cout << "vb[0] 的型別（auto）：" << typeid(rb).name() << std::endl;
    std::cout << "sizeof(ri) = " << sizeof(ri) << " bytes" << std::endl;
    std::cout << "sizeof(rb) = " << sizeof(rb) << " bytes（不是 1！）" << std::endl;
}

void demo_trap_auto_deduction() {
    std::cout << "\n--- 三、陷阱一：auto 推導錯誤 ---" << std::endl;
    std::vector<bool> vb = {true, false, true};

    // 危險：val 是代理物件，連結到 vb 內部
    auto val_proxy = vb[0];
    vb[0] = false;  // 修改原容器
    std::cout << "val_proxy（代理）= " << val_proxy
              << "（跟著 vb[0] 變為 false！）" << std::endl;

    // 安全：明確宣告為 bool，強制取得副本
    vb[0] = true;  // 還原
    bool val_copy = vb[0];  // 隱式轉型，取得獨立副本
    vb[0] = false;
    std::cout << "val_copy（副本）= " << val_copy
              << "（不受 vb[0] 改變的影響）" << std::endl;
}

void demo_trap_no_pointer() {
    std::cout << "\n--- 四、陷阱二：無法取元素指標 ---" << std::endl;
    std::vector<int> vi = {10, 20, 30};
    int* pi = &vi[0];  // OK：取得真實的 int*
    std::cout << "*pi = " << *pi << "（正常取到 int 指標）" << std::endl;

    std::vector<bool> vb = {true, false, true};
    // bool* pb = &vb[0];  // 編譯錯誤！&vb[0] 型別是 vector<bool>::reference*
    std::cout << "vector<bool> 無法取 bool* 指標（編譯期阻止）" << std::endl;
}

void demo_trap_no_bool_ref() {
    std::cout << "\n--- 五、陷阱三：無法綁定 bool& 引用 ---" << std::endl;
    std::vector<int> vi = {0, 0, 0};
    int& ref = vi[0];  // OK
    ref = 42;
    std::cout << "vi[0] = " << vi[0] << "（透過 int& 修改成功）" << std::endl;

    // std::vector<bool> vb = {false, false};
    // bool& ref_b = vb[0];  // 編譯錯誤！代理物件不能綁定到 bool&
    std::cout << "vector<bool> 元素無法綁定到 bool&（編譯期阻止）" << std::endl;
}

void demo_trap_no_data() {
    std::cout << "\n--- 六、陷阱四：data() 不存在 ---" << std::endl;
    std::vector<int> vi = {10, 20, 30};
    int* data_i = vi.data();  // OK
    std::cout << "vi.data()[0] = " << data_i[0] << "（正常）" << std::endl;

    // std::vector<bool> vb = {true, false};
    // bool* data_b = vb.data();  // 編譯錯誤！vector<bool> 無 data()
    std::cout << "vector<bool> 沒有 data() 成員函數（底層不是 bool 陣列）" << std::endl;
}

void demo_flip() {
    std::cout << "\n--- 七、vector<bool> 的特有操作：flip() ---" << std::endl;
    std::vector<bool> vb = {true, false, true, true, false};

    std::cout << "翻轉前：";
    for (bool b : vb) std::cout << b;
    std::cout << std::endl;

    vb[0].flip();  // 翻轉單一元素（透過代理物件的 flip() 方法）
    std::cout << "vb[0].flip()後：";
    for (bool b : vb) std::cout << b;
    std::cout << std::endl;

    vb.flip();  // 翻轉整個容器的所有位元
    std::cout << "vb.flip() 後：";
    for (bool b : vb) std::cout << b;
    std::cout << std::endl;
}

void demo_deque_bool_alternative() {
    std::cout << "\n--- 八、替代方案一：deque<bool>（行為完全正常）---" << std::endl;
    std::deque<bool> db = {true, false, true};

    bool& ref = db[0];  // OK：正常的 bool 引用
    ref = false;
    std::cout << "db[0]（透過 bool& 修改）= " << db[0] << std::endl;

    bool* ptr = &db[1];  // OK：正常的 bool 指標
    std::cout << "*ptr（bool* 指標）= " << *ptr << std::endl;
    std::cout << "deque<bool> 有完整容器行為，但每元素仍佔 1 byte" << std::endl;
}

void demo_uint8_alternative() {
    std::cout << "\n--- 九、替代方案二：vector<uint8_t>（完整容器行為）---" << std::endl;
    std::vector<uint8_t> vb = {1, 0, 1, 1, 0};  // 用 1/0 代表 true/false

    uint8_t& ref  = vb[0];   // 正常引用
    uint8_t* ptr  = &vb[0];  // 正常指標
    uint8_t* data = vb.data();  // data() 可用，方便與 C API 互操作

    ref = 0;  // 透過引用修改
    std::cout << "vb[0]（透過 ref 修改）= " << static_cast<int>(vb[0]) << std::endl;
    std::cout << "data[2] = " << static_cast<int>(data[2]) << std::endl;
    std::cout << "vector<uint8_t>：完整容器行為，1 byte/bool，可與 C API 互操作" << std::endl;
}

void demo_bitset_alternative() {
    std::cout << "\n--- 十、替代方案三：std::bitset（位元壓縮 + 完整位元運算）---" << std::endl;
    // 大小必須是編譯期常數
    std::bitset<8> bs;

    bs[0] = 1;
    bs[3] = 1;
    bs[7] = 1;

    std::cout << "bs = " << bs << "（最右為第 0 位）" << std::endl;
    std::cout << "count（1 的個數）= " << bs.count() << std::endl;

    bs.flip();  // 翻轉所有位元
    std::cout << "flip 後：" << bs << std::endl;

    // 支援位元運算
    std::bitset<8> bs2("11001100");
    std::cout << "bs AND bs2 = " << (bs & bs2) << std::endl;
    std::cout << "bs OR  bs2 = " << (bs | bs2) << std::endl;
    std::cout << "bitset：1 bit/bool，位元運算豐富，但大小必須是編譯期常數" << std::endl;
}

void demo_practical_advice() {
    std::cout << "\n--- 十一、實務建議 ---" << std::endl;

    // 1. 需要大量布林值且只做簡單讀寫 → vector<bool> 可接受，但注意：
    std::vector<bool> flags(1'000'000, false);
    flags[42] = true;

    bool val = flags[42];  // 正確：明確宣告為 bool，取得副本
    // auto val2 = flags[42];  // 危險！得到代理物件
    std::cout << "場景 1：僅讀寫，flags[42] = " << val
              << "（請明確寫 bool，不用 auto）" << std::endl;

    // 2. 需要完整容器行為 → 使用 vector<uint8_t>
    std::vector<uint8_t> safe_flags(1000, 0);
    uint8_t& ref  = safe_flags[0];
    uint8_t* ptr  = safe_flags.data();
    ref = 1;
    std::cout << "場景 2：完整行為，safe_flags[0] = "
              << static_cast<int>(safe_flags[0]) << "（用 vector<uint8_t>）" << std::endl;

    // 3. 固定大小 + 需要位元運算 → std::bitset
    std::bitset<16> bs;
    bs[0] = bs[5] = bs[10] = 1;
    std::cout << "場景 3：位元運算，bs = " << bs
              << "（用 std::bitset）" << std::endl;
}

int main() {
    std::cout << "================================================================" << std::endl;
    std::cout << "   第 18 課：vector<bool> 的特殊性與陷阱 總複習" << std::endl;
    std::cout << "================================================================" << std::endl;

    std::cout << std::boolalpha;  // 讓 bool 顯示為 true/false

    demo_memory_efficiency();
    demo_proxy_object();
    demo_trap_auto_deduction();
    demo_trap_no_pointer();
    demo_trap_no_bool_ref();
    demo_trap_no_data();
    demo_flip();
    demo_deque_bool_alternative();
    demo_uint8_alternative();
    demo_bitset_alternative();
    demo_practical_advice();

    std::cout << "\n================================================================" << std::endl;
    std::cout << "課程重點回顧：" << std::endl;
    std::cout << "  1. vector<bool> 是模板特化，每元素 1 bit，節省 8 倍記憶體" << std::endl;
    std::cout << "  2. operator[] 回傳代理物件（proxy），不是 bool&" << std::endl;
    std::cout << "  3. 五大陷阱：auto 推導、取指標、綁 bool&、data()、模板相容" << std::endl;
    std::cout << "  4. 特有操作：flip()（單元素翻轉、全部翻轉）" << std::endl;
    std::cout << "  5. 替代方案：deque<bool>（完整行為）、vector<uint8_t>（推薦）、bitset（位元運算）" << std::endl;
    std::cout << "  6. 實務原則：存取時明確寫 bool 不用 auto；需完整容器行為請改用其他型別" << std::endl;
    std::cout << "  7. 歷史教訓：被 Scott Meyers 等專家認為是 C++ 標準庫的設計失誤" << std::endl;
    std::cout << "================================================================" << std::endl;

    return 0;
}
