// =============================================================================
//  第 18 課 總複習：vector<bool> 的特殊性與陷阱
//                 —— 一個「實作沒錯、名字錯了」的標準庫案例
// =============================================================================
//
// 【主題資訊 Information】
//   template<class Allocator> class vector<bool, Allocator>;   // 標準規定的特化
//     reference       operator[](size_type);      // 回傳代理物件，不是 bool&
//     const_reference operator[](size_type) const;// C++11 起明文規定為 bool
//     void flip() noexcept;                       // 特化獨有：翻轉全部位元
//     （沒有 data()）
//   標準版本：C++98 起即有此特化（[vector.bool]）
//   記憶體：每個 bool 佔 1 bit（vector<char> 為 1 byte，差 8 倍）
//   標頭檔：<vector>
//
// 【詳細解釋 Explanation】
//
// 【1. 這個特化到底改了什麼】
//   一般的 vector<T> 每個元素佔 sizeof(T) bytes，operator[] 回傳 T&。
//   vector<bool> 則把每個元素壓成 1 個 bit，
//   底層是一串字組（libstdc++ 在 x86-64 上用 64-bit 的 unsigned long）。
//   問題來了：C++ 的最小可定址單位是 byte，
//   一個 bit 沒有自己的位址，所以 operator[] 回傳不了 bool&。
//   標準的解法是回傳一個「代理物件」vector<bool>::reference，
//   它內含「指向字組的指標 + 位元遮罩」，並提供
//   operator bool() 與 operator=(bool) 來模仿引用的行為。
//
// 【2. 五大陷阱，全部源自同一個原因】
//   (a) auto val = vb[0];  → val 是代理物件，不是 bool。
//       之後修改 vb[0]，val 會「跟著變」——因為它連結著同一個 bit。
//   (b) bool* p = &vb[0];  → 編譯錯誤。&vb[0] 的型別是 reference*。
//   (c) bool& r = vb[0];   → 編譯錯誤。代理物件既非 bool，也是右值。
//   (d) vb.data();         → 編譯錯誤。根本沒有這個成員函式。
//   (e) 泛型模板中的 T& elem = v[i]; → 對 bool 實例化時編譯失敗。
//   五個看似無關的症狀，根因都是「單一 bit 沒有位址」。
//
// 【3. 為什麼 (e) 最嚴重】
//   前四個都要你「直接對 vector<bool> 動手」才會遇到，
//   (e) 卻會炸在一份「完全沒想過 bool」的既有模板上，
//   而且錯誤訊息出現在模板內部、離呼叫點很遠、又長又難讀。
//   它違反的是 C++ 最基本的期待：
//   同一個模板對所有型別應該有一致的介面語意。
//
// 【4. 它換到了什麼：flip() 與記憶體】
//   公平地說，這個特化也帶來兩個好處：
//     ▸ 記憶體省 8 倍（一百萬個旗標：約 125 KB vs 約 1 MB）
//     ▸ flip() 是逐「字組」做按位取反，一次翻 64 個 bit，
//       比逐元素的迴圈快得多；一般的 vector<T> 根本沒有這個成員。
//   問題不在這些好處，而在它用了 vector 這個名字，
//   讓所有人以為它滿足容器的泛型契約。
//
// 【5. 四個替代方案怎麼選】
//     deque<bool>      沒被特化 → bool&/bool* 正常，但每元素 1 byte，
//                      而且分段配置，同樣沒有 data()
//     vector<uint8_t>  最務實 → 引用、指標、data()、泛型全部正常，
//                      代價是 1 byte/元素
//     std::bitset<N>   1 bit/元素 + 完整位元運算子（& | ^ ~ << >>），
//                      但 N 必須是編譯期常數，而且它不是容器
//     自訂位元打包     用 vector<uint8_t> 自己管理 bit，
//                      同時擁有壓縮與 data()——記憶體真的關鍵時的最佳解
//
// 【概念補充 Concept Deep Dive】
//   ▸ 代理物件是一個通用模式，不是 vector<bool> 的怪招
//     當「底層表示」與「介面型別」不一致時，代理物件是標準做法。
//     std::bitset::operator[] 同樣回傳代理；
//     線性代數庫（Eigen）用它實作表達式模板以避免暫時矩陣。
//     差別在於那些函式庫從不假裝自己是 std::vector，
//     因此沒有人對它們抱持錯誤的泛型期待。
//   ▸ 一個少被提及卻致命的並行問題
//     標準保證「同時修改容器的不同元素」是安全的——
//     但 vector<bool> 是例外。相鄰元素共用同一個字組，
//     執行緒 A 寫 v[0]、執行緒 B 寫 v[1] 時，
//     兩者對同一個 word 做讀改寫，形成貨真價實的 data race。
//   ▸ 歷史與現況
//     C++98 制定時記憶體昂貴，委員會想提供開箱即用的位元陣列，
//     卻把它塞進 vector<bool> 而非另立名字。
//     Herb Sutter 曾提案移除（因相容性未果），
//     Scott Meyers 在《Effective STL》第 18 條直接寫「避開它」。
//     現在的共識是：它應該叫 bit_vector。
//
// 【注意事項 Pay Attention】
//   1. 存取一律明確寫 bool val = vb[i];，絕不用 auto。
//   2. 取不到 bool*、綁不到 bool&、沒有 data()——這三件事是設計使然。
//   3. 不要把 vector<bool> 傳進你沒讀過原始碼的泛型模板。
//   4. 模板中請用 auto&& 取代 T& 來接元素，才能同時相容 bool。
//   5. 相鄰元素共用字組，多執行緒寫入「不同元素」也會產生 data race。
//   6. const vector<bool> 的 operator[] 回傳純 bool（C++11 起明文規定）。
//   7. 需要 data() → vector<uint8_t>；需要位元運算 → std::bitset<N>。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】vector<bool> 的特殊性
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. vector<bool> 和其他 vector<T> 有什麼根本差異？請說明它的五個陷阱。
//     答：它是標準規定的模板特化，每個元素壓成 1 bit 而非 sizeof(T) bytes。
//         因為單一 bit 沒有位址，operator[] 只能回傳代理物件而非 bool&，
//         由此衍生五個陷阱：auto 推導成代理物件、無法取 bool*、
//         無法綁 bool&、沒有 data()、破壞泛型模板。
//         五個症狀的根因是同一個：C++ 的最小可定址單位是 byte。
//     追問：這個特化有帶來任何好處嗎？
//         → 有兩個：記憶體省 8 倍；以及 flip() 是逐字組做按位取反，
//           一次翻 64 個 bit，比逐元素迴圈快得多。
//           問題不在這些好處，而在它用了 vector 這個名字。
//
// 🔥 Q2. auto val = vb[0]; 之後修改 vb[0]，val 會不會跟著變？為什麼？
//     答：會跟著變。因為 auto 推導出的是 vector<bool>::reference
//         這個代理物件，它內部持有「指向該字組的指標 + 位元遮罩」，
//         始終連結著容器裡的那個 bit。
//         要取得獨立副本必須明確寫 bool val = vb[0];，
//         觸發 operator bool() 轉換。
//     追問：那 const vector<bool> 上的 auto 呢？
//         → C++11 起標準規定 const_reference 就是 bool 純值，
//           所以在 const vector<bool> 上 auto 會推導成 bool，行為正常。
//
// 🔥 Q3. 你要在專案裡存一千萬個布林旗標，會怎麼選？
//     答：先問三個問題。需要位元運算且大小編譯期固定 → std::bitset；
//         需要整塊交給 C API → vector<uint8_t>；
//         會進泛型模板或需要 T&/T* → 避開 vector<bool>。
//         若三題皆否且記憶體確實是瓶頸，才用 vector<bool>，
//         而且要嚴守「存取一律寫 bool 不用 auto」的紀律。
//     追問：既要省記憶體又要能交給 C API 怎麼辦？
//         → 用 vector<uint8_t> 自己做位元打包（每 byte 存 8 個旗標）。
//           壓縮格式就是輸出格式，data() 照樣可用，
//           比 vector<bool> 全面更好。
//
// ⚠️ 陷阱. 「我用兩個執行緒分別寫 vb[0] 和 vb[1]，
//          它們是不同的元素，應該不需要加鎖吧？」
//     答：需要。標準保證「同時修改容器的不同元素」安全，
//         但 vector<bool> 正是這條保證的例外——
//         相鄰元素被壓在同一個字組裡，兩個執行緒都在對同一個 word
//         做讀改寫，這是貨真價實的 data race（undefined behavior），
//         ThreadSanitizer 會直接報出來。
//     為什麼會錯：以為「不同索引 = 不同記憶體位置」。
//         對所有正常容器這都成立，唯獨 vector<bool> 因為位元壓縮，
//         多個邏輯元素共用同一個實體位置。
//         這是它最少被提及、卻在並行程式中最致命的一點。
// ═══════════════════════════════════════════════════════════════════════════

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
    std::cout << "&vb[0] 與 data() 相同: " << std::boolalpha << (ptr == data)
              << "（記憶體連續，所以兩者必然相等）" << std::endl;
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
              << static_cast<int>(safe_flags[0])
              << "（用 vector<uint8_t>；data() 首元素 = "
              << static_cast<int>(ptr[0]) << "）" << std::endl;

    // 3. 固定大小 + 需要位元運算 → std::bitset
    std::bitset<16> bs;
    bs[0] = bs[5] = bs[10] = 1;
    std::cout << "場景 3：位元運算，bs = " << bs
              << "（用 std::bitset）" << std::endl;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 204. Count Primes
//   題目：統計小於 n 的質數個數。
//   為什麼用到本主題：埃拉托斯特尼篩法（Sieve of Eratosthenes）需要一個
//         「長度到執行期才知道」的布林標記陣列，這正是 vector<bool>
//         少數真正合理的使用場景——大小動態、只做讀寫、
//         而且 n 很大時（例如 5,000,000）記憶體差距是實打實的：
//         vector<bool> 約 610 KB，vector<char> 約 4.8 MB。
//         注意程式中一律寫 bool 而不用 auto，正是本課的紀律。
// -----------------------------------------------------------------------------
int countPrimes(int n) {
    if (n < 3) return 0;

    // 大小到執行期才知道 → 不能用 bitset<N>，vector<bool> 正好合適
    std::vector<bool> isComposite(static_cast<std::size_t>(n), false);

    int count = 0;
    for (long long i = 2; i < n; ++i) {
        bool composite = isComposite[static_cast<std::size_t>(i)];  // 明確寫 bool
        if (composite) continue;
        ++count;
        // 從 i*i 開始標記，前面的倍數已被更小的質因數處理過
        for (long long j = i * i; j < n; j += i) {
            isComposite[static_cast<std::size_t>(j)] = true;
        }
    }
    return count;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】黑名單過濾器：用 vector<bool> 做 O(1) 的 ID 命中查詢
//   情境：風控系統維護一份「被封鎖的使用者 ID」清單，ID 範圍是 0..N-1，
//         每筆請求進來都要判斷是否命中黑名單，QPS 很高。
//   為什麼 vector<bool> 在這裡合理：
//         (a) N 到執行期才知道（讀設定或查 DB），排除了 bitset；
//         (b) 只做讀寫、不取指標、不進泛型模板，五個陷阱一個都碰不到；
//         (c) N 若是千萬級，1 bit/元素與 1 byte/元素的差距是
//             約 1.2 MB 對上約 10 MB——這時記憶體差距是真的。
//   範例同時示範必須遵守的紀律：存取一律寫 bool，不用 auto。
// -----------------------------------------------------------------------------
class BlocklistFilter {
    std::vector<bool> blocked_;
public:
    explicit BlocklistFilter(std::size_t maxUserId) : blocked_(maxUserId, false) {}

    void block(std::size_t userId)   { blocked_[userId] = true; }
    void unblock(std::size_t userId) { blocked_[userId] = false; }

    bool isBlocked(std::size_t userId) const {
        if (userId >= blocked_.size()) return false;
        bool hit = blocked_[userId];     // 紀律：明確寫 bool，不用 auto
        return hit;
    }

    std::size_t blockedCount() const {
        std::size_t c = 0;
        for (std::size_t i = 0; i < blocked_.size(); ++i) {
            bool hit = blocked_[i];
            if (hit) ++c;
        }
        return c;
    }
    std::size_t capacityBytes() const { return blocked_.capacity() / 8; }
};

void demo_leetcode_and_practice() {
    std::cout << "\n--- 十二、LeetCode 204. Count Primes ---" << std::endl;
    std::cout << "countPrimes(10)      = " << countPrimes(10)      << std::endl;
    std::cout << "countPrimes(100)     = " << countPrimes(100)     << std::endl;
    std::cout << "countPrimes(1000000) = " << countPrimes(1000000) << std::endl;
    std::cout << "  （篩法陣列長度到執行期才知道 → bitset 用不了，"
              << "vector<bool> 正好合適）" << std::endl;

    std::cout << "\n--- 十三、日常實務：黑名單過濾器 ---" << std::endl;
    const std::size_t kMaxUserId = 10'000'000;
    BlocklistFilter filter(kMaxUserId);

    // 封鎖若干使用者
    for (std::size_t id = 0; id < kMaxUserId; id += 100000) filter.block(id);
    filter.block(42);
    filter.block(777);

    std::cout << "使用者上限 " << kMaxUserId << "，已封鎖 "
              << filter.blockedCount() << " 人" << std::endl;
    std::cout << "查詢 id=42     → " << filter.isBlocked(42)     << std::endl;
    std::cout << "查詢 id=43     → " << filter.isBlocked(43)     << std::endl;
    std::cout << "查詢 id=777    → " << filter.isBlocked(777)    << std::endl;
    std::cout << "查詢超出範圍   → " << filter.isBlocked(99'999'999) << std::endl;
    std::cout << "記憶體約 " << (filter.capacityBytes() / 1024) << " KB"
              << "（若改用 vector<uint8_t> 會是約 "
              << (kMaxUserId / 1024) << " KB）" << std::endl;

    filter.unblock(42);
    std::cout << "解除封鎖 42 後 → " << filter.isBlocked(42) << std::endl;
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
    demo_leetcode_and_practice();

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

// 編譯: g++ -std=c++17 -Wall -Wextra summary.cpp -o summary18
//
// 【關於下方預期輸出的但書】
//   ▸ 第二段用 typeid(...).name() 印出的型別名稱是「編譯器專屬」的
//     修飾名（本機 GCC 為 i 與 St14_Bit_reference），
//     標準並未規定其格式，換編譯器（例如 MSVC）會完全不同。
//   ▸ sizeof(vector<bool>::reference)（本機為 16）屬實作定義。
//   ▸ 記憶體量級（capacity()/8 等）為約略值，未計入字組對齊的
//     向上取整與配置器開銷，僅供比較。
//   ▸ 其餘內容皆為確定性，每次執行都應相同。

// === 預期輸出 ===
// ================================================================
//    第 18 課：vector<bool> 的特殊性與陷阱 總複習
// ================================================================
//
// --- 一、記憶體效率對比 ---
// bool[1000] 大小：1000 bytes
// vector<char>(1000)：約 1000 bytes
// vector<bool>(1000)：約 128 bytes（節省約 8 倍）
//
// --- 二、代理物件（Proxy Object）型別觀察 ---
// vi[0] 的型別（auto）：i
// vb[0] 的型別（auto）：St14_Bit_reference
// sizeof(ri) = 4 bytes
// sizeof(rb) = 16 bytes（不是 1！）
//
// --- 三、陷阱一：auto 推導錯誤 ---
// val_proxy（代理）= false（跟著 vb[0] 變為 false！）
// val_copy（副本）= true（不受 vb[0] 改變的影響）
//
// --- 四、陷阱二：無法取元素指標 ---
// *pi = 10（正常取到 int 指標）
// vector<bool> 無法取 bool* 指標（編譯期阻止）
//
// --- 五、陷阱三：無法綁定 bool& 引用 ---
// vi[0] = 42（透過 int& 修改成功）
// vector<bool> 元素無法綁定到 bool&（編譯期阻止）
//
// --- 六、陷阱四：data() 不存在 ---
// vi.data()[0] = 10（正常）
// vector<bool> 沒有 data() 成員函數（底層不是 bool 陣列）
//
// --- 七、vector<bool> 的特有操作：flip() ---
// 翻轉前：truefalsetruetruefalse
// vb[0].flip()後：falsefalsetruetruefalse
// vb.flip() 後：truetruefalsefalsetrue
//
// --- 八、替代方案一：deque<bool>（行為完全正常）---
// db[0]（透過 bool& 修改）= false
// *ptr（bool* 指標）= false
// deque<bool> 有完整容器行為，但每元素仍佔 1 byte
//
// --- 九、替代方案二：vector<uint8_t>（完整容器行為）---
// vb[0]（透過 ref 修改）= 0
// &vb[0] 與 data() 相同: true（記憶體連續，所以兩者必然相等）
// data[2] = 1
// vector<uint8_t>：完整容器行為，1 byte/bool，可與 C API 互操作
//
// --- 十、替代方案三：std::bitset（位元壓縮 + 完整位元運算）---
// bs = 10001001（最右為第 0 位）
// count（1 的個數）= 3
// flip 後：01110110
// bs AND bs2 = 01000100
// bs OR  bs2 = 11111110
// bitset：1 bit/bool，位元運算豐富，但大小必須是編譯期常數
//
// --- 十一、實務建議 ---
// 場景 1：僅讀寫，flags[42] = true（請明確寫 bool，不用 auto）
// 場景 2：完整行為，safe_flags[0] = 1（用 vector<uint8_t>；data() 首元素 = 1）
// 場景 3：位元運算，bs = 0000010000100001（用 std::bitset）
//
// --- 十二、LeetCode 204. Count Primes ---
// countPrimes(10)      = 4
// countPrimes(100)     = 25
// countPrimes(1000000) = 78498
//   （篩法陣列長度到執行期才知道 → bitset 用不了，vector<bool> 正好合適）
//
// --- 十三、日常實務：黑名單過濾器 ---
// 使用者上限 10000000，已封鎖 102 人
// 查詢 id=42     → true
// 查詢 id=43     → false
// 查詢 id=777    → true
// 查詢超出範圍   → false
// 記憶體約 1220 KB（若改用 vector<uint8_t> 會是約 9765 KB）
// 解除封鎖 42 後 → false
//
// ================================================================
// 課程重點回顧：
//   1. vector<bool> 是模板特化，每元素 1 bit，節省 8 倍記憶體
//   2. operator[] 回傳代理物件（proxy），不是 bool&
//   3. 五大陷阱：auto 推導、取指標、綁 bool&、data()、模板相容
//   4. 特有操作：flip()（單元素翻轉、全部翻轉）
//   5. 替代方案：deque<bool>（完整行為）、vector<uint8_t>（推薦）、bitset（位元運算）
//   6. 實務原則：存取時明確寫 bool 不用 auto；需完整容器行為請改用其他型別
//   7. 歷史教訓：被 Scott Meyers 等專家認為是 C++ 標準庫的設計失誤
// ================================================================
