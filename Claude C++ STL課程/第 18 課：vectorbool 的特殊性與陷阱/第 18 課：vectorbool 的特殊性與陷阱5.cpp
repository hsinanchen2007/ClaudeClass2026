// =============================================================================
//  第 18 課-5：陷阱二 —— 為什麼 &vb[0] 拿不到 bool*
// =============================================================================
//
// 【主題資訊 Information】
//   template<class Allocator>
//   class vector<bool, Allocator>;              // 標準規定的「特化」，非一般 vector
//     reference       operator[](size_type n);        // 回傳代理物件，不是 bool&
//     const_reference operator[](size_type n) const;  // C++11 起規定為 bool
//   對照：vector<T>（T != bool）的 operator[] 回傳 T&，&v[0] 是貨真價實的 T*
//   標準版本：C++98 起即有此特化（[vector.bool]）
//   標頭檔：<vector>
//
// 【詳細解釋 Explanation】
//
// 【1. 問題的根源：C++ 最小可定址單位是 byte】
//   vector<bool> 為了省記憶體，把每個 bool 壓成 1 個 bit 存放
//   （1000 個 bool 只要約 125 bytes，而不是 1000 bytes）。
//   但 C++ 的指標模型建立在「byte 定址」之上：
//   sizeof(bool) 至少是 1，指標的最小粒度就是一個 byte。
//   一個 bit 沒有自己的位址，物理上就無法產生指向它的 bool*。
//   這不是標準庫偷懶，是語言的定址模型使然。
//
// 【2. 於是 operator[] 只能回傳「代理物件」】
//   既然回傳不了 bool&，vector<bool>::operator[] 改為回傳一個
//   vector<bool>::reference 型別的小物件，內部大致是：
//       struct reference {
//           uint64_t* word_;   // 指向存放這個 bit 的那個字組
//           uint64_t  mask_;   // 標出是字組裡的第幾個 bit
//           operator bool() const { return *word_ & mask_; }
//           reference& operator=(bool b);   // 寫回那一個 bit
//       };
//   它「模仿」bool& 的行為：可以隱式轉成 bool、可以被賦值。
//   但它終究是個獨立的類別型別，不是引用。
//
// 【3. 所以 &vb[0] 的型別是什麼】
//   &vb[0] 取的是「那個暫時代理物件的位址」，
//   型別是 vector<bool>::reference*，而不是 bool*。
//   兩者之間沒有任何轉換關係，賦值給 bool* 會直接編譯失敗。
//   更糟的是：那個代理物件是個暫時物件，
//   即使硬拿到它的位址，敘述結束後它就消失了——
//   所以編譯期擋下來反而是好事。
//
// 【4. 這個特化為什麼被視為設計失誤】
//   C++ 有一條核心原則：泛型程式碼中，vector<T> 對所有 T 的行為應該一致。
//   vector<bool> 打破了它——同一份模板程式碼，
//   對 vector<int> 編得過、對 vector<bool> 編不過。
//   Scott Meyers 在《Effective STL》第 18 條明講「避開 vector<bool>」，
//   Herb Sutter 也曾提議把它從標準移除（因相容性未果）。
//   標準委員會後來的態度是：它應該叫別的名字（例如 bit_vector），
//   而不該偽裝成 vector 的一個實例化。
//
// 【5. 什麼時候還是可以用它】
//   如果你只做「讀值、寫值、計數」這類操作，
//   而且真的在意那 8 倍的記憶體差距（例如百萬級的旗標陣列），
//   vector<bool> 是可以用的——只要記得存取時明確寫 bool 而不要用 auto。
//   一旦需要取位址、取引用、傳給泛型模板、或與 C API 互通，
//   就該換成 vector<uint8_t>（完整容器行為）或 std::bitset（固定大小）。
//
// 【概念補充 Concept Deep Dive】
//   ▸ 代理物件的 sizeof 不是 1
//     它內含一根指標加一個遮罩，本機（x86-64 / GCC 15.2 / libstdc++）
//     實測 sizeof(std::vector<bool>::reference) = 16 bytes，
//     而 sizeof(bool) = 1。這個差距本身就說明了它不是 bool。
//     （16 這個數字屬實作定義。）
//   ▸ 為什麼 const 版本反而正常
//     C++11 起標準規定 vector<bool> 的 const_reference 就是 bool（純值），
//     因為唯讀時不需要「寫回某個 bit」的能力，直接回傳值即可。
//     所以 const vector<bool> 的 operator[] 拿到的是普通 bool。
//   ▸ 同樣是代理，std::bitset 也有 reference
//     bitset<N>::operator[] 也回傳代理物件，原因完全相同。
//     差別在於 bitset 從一開始就不叫 vector，
//     沒有人期待它滿足容器的泛型契約，因此不構成陷阱。
//
// 【注意事項 Pay Attention】
//   1. &vb[0] 的型別是 vector<bool>::reference*，不是 bool*，無法轉換。
//   2. vector<bool> 沒有 data() 成員函式，無法取得底層陣列指標。
//   3. 存取元素時明確寫 bool val = vb[i]; 不要用 auto（auto 會推導出代理）。
//   4. 代理物件是暫時物件，其生命週期只到該敘述結束。
//   5. 需要取位址/引用/與 C API 互通時，改用 vector<uint8_t> 或 std::bitset。
//   6. const vector<bool> 的 operator[] 回傳的是純 bool（C++11 起明文規定）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】vector<bool> 為什麼取不到元素指標
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 bool* p = &vb[0]; 編譯不過，但 int* p = &vi[0]; 沒問題？
//     答：vector<bool> 是標準規定的模板特化，把每個 bool 壓成 1 個 bit。
//         C++ 的最小可定址單位是 byte，單一個 bit 沒有自己的位址，
//         所以 operator[] 無法回傳 bool&，只能回傳一個代理物件
//         vector<bool>::reference。&vb[0] 取到的是那個代理物件的位址
//         （型別為 reference*），與 bool* 之間沒有任何轉換關係。
//     追問：那 vb.data() 呢？
//         → vector<bool> 根本沒有 data() 成員函式，
//           因為底層不是 bool 陣列，沒有一個「連續 bool 序列」可以回傳。
//
// 🔥 Q2. vector<bool> 的代理物件內部長什麼樣？sizeof 是多少？
//     答：概念上包含「指向該 bit 所在字組的指標」加上「標出第幾個 bit 的遮罩」，
//         並提供 operator bool() 與 operator=(bool) 來模仿引用行為。
//         本機（x86-64 / GCC 15.2 / libstdc++）實測
//         sizeof(vector<bool>::reference) = 16 bytes（實作定義），
//         而 sizeof(bool) = 1——這個差距本身就說明它不是 bool。
//     追問：為什麼 const 版本沒有這個問題？
//         → C++11 起標準規定 const_reference 就是 bool 純值。
//           唯讀時不需要「寫回某個 bit」的能力，直接回傳值即可。
//
// ⚠️ 陷阱. 「那我用 auto p = &vb[0]; 讓編譯器自己推導，不就繞過去了？」
//     答：編譯是會過，但 p 的型別是 vector<bool>::reference*，
//         而且它指向的是一個「暫時的代理物件」——
//         那個物件在該敘述結束時就已經被銷毀了。
//         之後解參考 p 就是使用懸空指標，屬於 undefined behavior。
//     為什麼會錯：以為編譯錯誤是「型別寫錯」這種表面問題，
//         用 auto 就能繞開。實際上編譯器擋下來是在保護你——
//         底層根本不存在一個可以被指向的 bool 物件。
//         用 auto 只是把編譯期的錯誤，換成了執行期的懸空指標。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <vector>

// -----------------------------------------------------------------------------
// 【日常實務範例】特徵旗標（feature flags）：兩種存法的取捨
//   情境：服務啟動時載入上萬個使用者的「是否啟用某功能」旗標，
//         需要快速查詢，偶爾也要把整批旗標交給 C 寫的統計函式庫。
//   為什麼用本主題：這正是 vector<bool> 看似最適合、實際上卻會卡住的場景。
//         查詢與設定沒問題，但只要需要把資料交給 C API，
//         沒有 data() 就完全無解——只能整批轉存成 vector<uint8_t>。
//         這個範例把「省記憶體」與「可互通」的取捨具體呈現出來。
// -----------------------------------------------------------------------------

// 模擬一個 C 寫的統計函式庫：只接受連續的 unsigned char 陣列
extern "C" int count_enabled(const unsigned char* flags, std::size_t n) {
    int c = 0;
    for (std::size_t i = 0; i < n; ++i) c += (flags[i] != 0);
    return c;
}

// vector<bool> 省記憶體，但無法直接餵給 C API，必須先轉存
int countViaBitPacked(const std::vector<bool>& flags) {
    // 沒有 data() 可用，只能逐一取值複製到一塊連續的 byte 緩衝區
    std::vector<unsigned char> bridge(flags.size());
    for (std::size_t i = 0; i < flags.size(); ++i) {
        bridge[i] = flags[i] ? 1u : 0u;      // 隱式轉成 bool 再轉 byte
    }
    return count_enabled(bridge.data(), bridge.size());
}

// vector<uint8_t> 多用 8 倍記憶體，但可以零成本直接交給 C API
int countViaBytes(const std::vector<unsigned char>& flags) {
    return count_enabled(flags.data(), flags.size());   // data() 直接可用
}

int main() {
    std::cout << "=== 一、vector<int> 可以取元素位址 ===" << std::endl;
    std::vector<int> vi = {10, 20, 30};
    int* pi = &vi[0];  // 完全合法，取得元素的位址
    std::cout << "*pi = " << *pi << std::endl;
    std::cout << "vi.data() 與 &vi[0] 相同: " << std::boolalpha
              << (vi.data() == &vi[0]) << std::endl;

    std::cout << "\n=== 二、vector<bool> 不行（以下皆為編譯錯誤，故註解）===" << std::endl;
    std::vector<bool> vb = {true, false, true};
    // bool* pb = &vb[0];   // 編譯錯誤！
    //   vb[0] 回傳的是代理物件 vector<bool>::reference，不是 bool
    //   &vb[0] 的型別因此是 reference*，與 bool* 無任何轉換關係
    // bool* pd = vb.data();  // 編譯錯誤！vector<bool> 根本沒有 data()
    std::cout << "vb 的內容（讀值完全正常）: ";
    for (std::size_t i = 0; i < vb.size(); ++i) {
        bool val = vb[i];          // 正確寫法：明確宣告為 bool
        std::cout << val << " ";
    }
    std::cout << std::endl;

    std::cout << "\n=== 三、代理物件的大小不是 1 ===" << std::endl;
    std::cout << "sizeof(bool)                        = " << sizeof(bool) << " byte" << std::endl;
    std::cout << "sizeof(std::vector<bool>::reference) = "
              << sizeof(std::vector<bool>::reference)
              << " bytes（實作定義，本機 GCC 15.2 / libstdc++）" << std::endl;

    std::cout << "\n=== 四、記憶體用量對比 ===" << std::endl;
    const std::size_t N = 100000;
    std::vector<bool>          bits(N, true);
    std::vector<unsigned char> bytes(N, 1);
    std::cout << N << " 個旗標：" << std::endl;
    std::cout << "  vector<bool>          約 " << (bits.capacity() / 8) << " bytes" << std::endl;
    std::cout << "  vector<unsigned char> 約 " << (bytes.capacity() * sizeof(unsigned char))
              << " bytes（多 8 倍，但換來完整容器行為）" << std::endl;

    std::cout << "\n=== 五、日常實務：把旗標交給 C API 的兩條路 ===" << std::endl;
    std::vector<bool> flagBits(1000, false);
    std::vector<unsigned char> flagBytes(1000, 0);
    for (std::size_t i = 0; i < 1000; i += 3) {     // 每 3 個啟用一個
        flagBits[i]  = true;
        flagBytes[i] = 1;
    }
    std::cout << "vector<bool>  →（必須先轉存一份 byte 緩衝區）啟用數 = "
              << countViaBitPacked(flagBits) << std::endl;
    std::cout << "vector<uint8> →（data() 直接餵給 C API）      啟用數 = "
              << countViaBytes(flagBytes) << std::endl;
    std::cout << "結論：省 8 倍記憶體的代價，是與 C API 互通時要多一次完整複製" << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 18 課：vectorbool 的特殊性與陷阱5.cpp" -o vb_no_pointer
//
// 【關於下方預期輸出的但書】
//   sizeof(std::vector<bool>::reference) 的值（本機為 16）屬於實作定義，
//   標準只規定它是一個能隱式轉為 bool 的類別型別，未規定其大小。
//   vector<bool> 的 capacity()/8 只是「約略」的 byte 數，
//   實際配置會以字組（word）為單位對齊，故僅供量級比較。
//
// 【本檔未附 LeetCode 範例的理由】
//   本檔的主題是「C++ 的 byte 定址模型導致 bit 無法被指向」這個
//   語言層面的限制，以及它如何破壞容器的泛型契約。
//   LeetCode 不會考容器特化造成的型別問題（那是 API 設計議題，不是演算法），
//   硬加一題只會離題。真正會被它咬到的是與 C API 互通的實務場景，
//   因此改以上面的特徵旗標範例呈現這個取捨。

// === 預期輸出 ===
// === 一、vector<int> 可以取元素位址 ===
// *pi = 10
// vi.data() 與 &vi[0] 相同: true
//
// === 二、vector<bool> 不行（以下皆為編譯錯誤，故註解）===
// vb 的內容（讀值完全正常）: true false true
//
// === 三、代理物件的大小不是 1 ===
// sizeof(bool)                        = 1 byte
// sizeof(std::vector<bool>::reference) = 16 bytes（實作定義，本機 GCC 15.2 / libstdc++）
//
// === 四、記憶體用量對比 ===
// 100000 個旗標：
//   vector<bool>          約 12504 bytes
//   vector<unsigned char> 約 100000 bytes（多 8 倍，但換來完整容器行為）
//
// === 五、日常實務：把旗標交給 C API 的兩條路 ===
// vector<bool>  →（必須先轉存一份 byte 緩衝區）啟用數 = 334
// vector<uint8> →（data() 直接餵給 C API）      啟用數 = 334
// 結論：省 8 倍記憶體的代價，是與 C API 互通時要多一次完整複製
