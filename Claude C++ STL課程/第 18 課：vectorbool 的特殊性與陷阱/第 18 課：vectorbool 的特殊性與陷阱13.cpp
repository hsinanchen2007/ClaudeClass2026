// =============================================================================
//  第 18 課-13：替代方案三 —— std::bitset，位元壓縮又有完整位元運算
// =============================================================================
//
// 【主題資訊 Information】
//   template<std::size_t N> class std::bitset;      // N 必須是編譯期常數
//     bitset<N>& set()   / set(pos, val = true)
//     bitset<N>& reset() / reset(pos)
//     bitset<N>& flip()  / flip(pos)
//     bool  test(pos)   const;      // 有範圍檢查，越界丟 std::out_of_range
//     reference operator[](pos);    // 無範圍檢查，越界是 UB
//     size_t count() const noexcept;   // 1 的個數（popcount）
//     bool  any() / none() / all() const noexcept;   // all() 是 C++11 加入
//     unsigned long      to_ulong()  const;    // 放不下丟 std::overflow_error
//     unsigned long long to_ullong() const;    // C++11
//     std::string        to_string() const;
//     完整位元運算子：& | ^ ~ << >> 與對應的複合指派
//   標準版本：C++98 起；all()、to_ullong()、部分 noexcept 為 C++11 加入
//   記憶體：N/8 bytes 向上取整並對齊字組（本機實測 bitset<8> 為 8 bytes）
//   標頭檔：<bitset>
//
// 【詳細解釋 Explanation】
//
// 【1. bitset 補上了 vector<bool> 缺的那一半】
//   兩者都做位元壓縮（1 bit/元素），但 bitset 多了整組位元運算子：
//     bs1 & bs2、bs1 | bs2、bs1 ^ bs2、~bs、bs << 3、bs >> 3
//   vector<bool> 一個都沒有，因為它的長度是執行期決定的，
//   兩個 vector<bool> 可能不等長，雙目運算的語意會變得含糊。
//   bitset<N> 的長度寫在型別裡，兩個 bitset<8> 必定等長，
//   位元運算因此有明確定義——這是「把長度提升到型別層」換來的好處。
//
// 【2. 代價：大小必須是編譯期常數】
//   bitset<n> 的 n 必須在編譯時就已知，不能用執行期變數。
//     std::size_t n = readFromConfig();
//     std::bitset<n> bs;          // 編譯錯誤！
//   所以它只適用於「大小固定且事先已知」的場合：
//   權限旗標、狀態機、固定寬度的硬體暫存器對映、
//   或上限已知的篩法（例如質數篩到 10⁶）。
//   大小要到執行期才知道，就只能回頭用 vector<bool> 或自己位元打包。
//
// 【3. count() 通常會編譯成單一條 CPU 指令】
//   計算「有幾個 1」（population count）是硬體直接支援的操作。
//   x86-64 的 POPCNT 指令、ARM 的 CNT 指令都是一個時脈週期的事。
//   GCC/Clang 會把 bitset::count() 編譯成這些指令
//   （在有支援的目標上），比手寫迴圈快上一個數量級。
//   這是「用標準庫而不是自己寫迴圈」最划算的例子之一。
//
// 【4. 索引方向：最右邊是第 0 位】
//   bs[0] 是最低位（least significant bit），
//   但 operator<< 印出來時是「最高位在左」，
//   所以 bitset<8> 中設定了 bs[0] 與 bs[7]，印出來會是 10000001。
//   這個「索引順序與顯示順序相反」是初學者最常搞混的地方。
//   從字串建構時也遵循同一規則：
//   bitset<8>("11001100") 的 bs[0] 是最右邊那個 0。
//
// 【5. test() 與 operator[] 的差別】
//   test(pos) 會做範圍檢查，越界丟 std::out_of_range；
//   operator[](pos) 不檢查，越界是 undefined behavior。
//   這組對應關係和 vector 的 at() 與 operator[] 完全一致——
//   標準庫在「安全版」與「快速版」之間一貫的分工方式。
//
// 【概念補充 Concept Deep Dive】
//   ▸ bitset 的 operator[] 也回傳代理物件
//     和 vector<bool> 一樣，非 const 版本回傳 bitset<N>::reference，
//     原因完全相同——單一 bit 沒有位址。
//     差別在於 bitset 從一開始就不叫 vector，
//     沒有人期待它滿足容器的泛型契約，所以不構成「陷阱」。
//     這再次印證：問題出在 vector<bool> 的「命名與定位」，不是位元壓縮本身。
//   ▸ sizeof(bitset<N>) 會向上對齊到字組
//     本機（x86-64 / GCC 15.2 / libstdc++）實測
//     sizeof(bitset<8>) = 8 bytes（而非 1），
//     因為它內部用 unsigned long 陣列儲存，最少也要一個字組。
//     所以極小的 bitset 反而沒有省到記憶體——
//     它的優勢要在 N 夠大時才顯現。
//   ▸ 為什麼 to_ulong() 可能丟例外
//     若 N 大於 unsigned long 的位元數，且高位有 1，
//     轉換就會遺失資訊，標準規定此時丟 std::overflow_error。
//     若高位全是 0 則可以安全轉換。
//
// 【注意事項 Pay Attention】
//   1. bitset<N> 的 N 必須是編譯期常數；執行期才知道大小請用 vector<bool>。
//   2. 索引 0 是最低位（最右邊），與 operator<< 的顯示順序相反。
//   3. test() 有範圍檢查（丟 out_of_range），operator[] 沒有（越界是 UB）。
//   4. sizeof(bitset<N>) 會向上對齊到字組，小 N 時不會比較省（本機 bitset<8> = 8 bytes）。
//   5. to_ulong()/to_ullong() 在放不下時丟 std::overflow_error。
//   6. bitset 沒有 data()、沒有 begin()/end()，它不是容器，不能用範圍 for。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::bitset
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::bitset 和 std::vector<bool> 該怎麼選？
//     答：看大小是編譯期已知還是執行期決定。
//         bitset<N> 的 N 必須是編譯期常數，換來的是完整的位元運算子
//         （& | ^ ~ << >>）與 count()/any()/all() 等操作；
//         vector<bool> 的長度可在執行期決定、可以增刪元素，
//         但完全沒有位元運算子。
//         兩者都做位元壓縮，差別在「長度資訊放在型別裡還是物件裡」。
//     追問：為什麼 vector<bool> 不提供 operator&？
//         → 因為兩個 vector<bool> 可能不等長，雙目運算的語意會含糊
//           （要截斷？補零？還是丟例外？）。
//           bitset<N> 的長度寫在型別裡，兩個 bitset<8> 必定等長，
//           運算語意才明確。
//
// 🔥 Q2. bitset<8> bs; bs[0] = 1; 之後 std::cout << bs 印出什麼？
//     答：00000001。因為索引 0 是最低位（least significant bit），
//         而 operator<< 依照「最高位在左」的慣例輸出，
//         兩者方向相反。
//         同理 bitset<8>("11001100") 的 bs[0] 是最右邊那個 0。
//     追問：那 bs.count() 和 bs.size() 分別是什麼？
//         → count() 是「1 的個數」（本例為 1），
//           size() 是「總位元數」（本例為 8，等於模板參數 N）。
//           count() 在支援的平台上通常會編譯成單一條 POPCNT 指令。
//
// ⚠️ 陷阱. std::size_t n = getUserCount();
//          std::bitset<n> flags;        // 為什麼編譯不過？
//     答：bitset 的大小是模板的非型別參數，必須是編譯期常數運算式。
//         n 是執行期才有值的變數，無法用來實例化模板。
//         大小要到執行期才知道，只能用 vector<bool>
//         或自己用 vector<uint8_t> 做位元打包。
//     為什麼會錯：把 bitset 當成「容器」看待，
//         以為它像 vector 一樣可以動態決定大小。
//         但 bitset 根本不是容器——它沒有 begin()/end()、
//         不能用範圍 for、也沒有 data()。
//         它更接近「一個帶了很多方便方法的固定寬度整數」。
// ═══════════════════════════════════════════════════════════════════════════

#include <bitset>
#include <cstdint>            // std::uint32_t
#include <initializer_list>
#include <iostream>
#include <stdexcept>
#include <string>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 191. Number of 1 Bits
//   題目：給一個無號整數，回傳它的二進位表示中 1 的個數（Hamming weight）。
//   為什麼用到本主題：這正是 bitset::count() 的定義。
//         把整數丟進 bitset<32> 再呼叫 count()，一行解決；
//         而且在支援的平台上會編譯成單一條 POPCNT 指令，
//         比手寫的逐位元迴圈快得多。
//         這裡同時附上經典的 Brian Kernighan 手寫解法做對照。
// -----------------------------------------------------------------------------
int hammingWeightByBitset(std::uint32_t n) {
    return static_cast<int>(std::bitset<32>(n).count());
}

// 對照組：Brian Kernighan 演算法，迴圈次數等於 1 的個數（不是 32 次）
int hammingWeightManual(std::uint32_t n) {
    int c = 0;
    while (n) {
        n &= (n - 1);      // 每次消掉最低位的那個 1
        ++c;
    }
    return c;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】使用者權限旗標：固定寬度、需要位元運算
//   情境：系統定義了 8 種權限（讀、寫、刪除、分享…），
//         每個使用者一組旗標；角色（role）也是一組旗標。
//         常見操作是「使用者權限 = 角色權限 OR 個人額外授權」、
//         「檢查是否具備某組必要權限」、「撤銷某組權限」。
//   為什麼用 bitset：
//         (a) 權限種類在編譯期就固定了，完全符合 bitset 的前提；
//         (b) 這些操作本質上就是 OR / AND / ~ 的位元運算，
//             用 bitset 寫出來幾乎就是需求本身的翻譯；
//         (c) vector<bool> 沒有位元運算子，同樣的邏輯要自己寫迴圈。
// -----------------------------------------------------------------------------
enum Permission : std::size_t {
    kRead = 0, kWrite = 1, kDelete = 2, kShare = 3,
    kAdmin = 4, kBilling = 5, kAudit = 6, kExport = 7
};

using PermSet = std::bitset<8>;

PermSet makePerms(std::initializer_list<Permission> ps) {
    PermSet s;
    for (Permission p : ps) s.set(p);
    return s;
}

std::string describe(const PermSet& p) {
    static const char* names[] = {"Read", "Write", "Delete", "Share",
                                  "Admin", "Billing", "Audit", "Export"};
    std::string out;
    for (std::size_t i = 0; i < p.size(); ++i) {
        if (p.test(i)) {                       // test() 有範圍檢查
            if (!out.empty()) out += "|";
            out += names[i];
        }
    }
    return out.empty() ? "(none)" : out;
}

int main() {
    std::cout << std::boolalpha;

    std::cout << "=== 一、基本操作與索引方向 ===" << std::endl;
    std::bitset<8> bs;                 // 全部初始化為 0
    bs[0] = 1;
    bs[3] = 1;
    bs[7] = 1;

    std::cout << "bs = " << bs << "（索引 0 在最右邊，顯示時最高位在左）" << std::endl;
    std::cout << "size()  = " << bs.size()  << "（總位元數，等於模板參數 N）" << std::endl;
    std::cout << "count() = " << bs.count() << "（1 的個數）" << std::endl;
    std::cout << "any()=" << bs.any() << ", none()=" << bs.none()
              << ", all()=" << bs.all() << std::endl;

    std::cout << "\n=== 二、flip 與位元運算 ===" << std::endl;
    bs.flip();
    std::cout << "flip 後：" << bs << std::endl;

    std::bitset<8> bs2("11001100");
    std::cout << "bs2      = " << bs2 << std::endl;
    std::cout << "bs & bs2 = " << (bs & bs2) << std::endl;
    std::cout << "bs | bs2 = " << (bs | bs2) << std::endl;
    std::cout << "bs ^ bs2 = " << (bs ^ bs2) << std::endl;
    std::cout << "~bs2     = " << (~bs2)     << std::endl;
    std::cout << "bs2 << 2 = " << (bs2 << 2) << std::endl;
    std::cout << "bs2 >> 2 = " << (bs2 >> 2) << std::endl;
    std::cout << "（vector<bool> 一個位元運算子都沒有）" << std::endl;

    std::cout << "\n=== 三、test() 有範圍檢查，operator[] 沒有 ===" << std::endl;
    try {
        std::cout << "bs2.test(3) = " << bs2.test(3) << std::endl;
        std::cout << "bs2.test(99) → ";
        std::cout << bs2.test(99) << std::endl;      // 會丟例外
    } catch (const std::out_of_range& e) {
        std::cout << "捕獲 std::out_of_range（test 有檢查；operator[] 越界則是 UB）"
                  << std::endl;
    }

    std::cout << "\n=== 四、記憶體：小 bitset 不會比較省 ===" << std::endl;
    std::cout << "sizeof(std::bitset<8>)    = " << sizeof(std::bitset<8>)
              << " bytes（不是 1！內部以字組為單位）" << std::endl;
    std::cout << "sizeof(std::bitset<64>)   = " << sizeof(std::bitset<64>)  << " bytes" << std::endl;
    std::cout << "sizeof(std::bitset<1024>) = " << sizeof(std::bitset<1024>)
              << " bytes（N 夠大時才看得出壓縮效果）" << std::endl;

    std::cout << "\n=== 五、與整數互轉 ===" << std::endl;
    std::bitset<8> v(0b10110101);
    std::cout << "bitset<8>(0b10110101) = " << v << std::endl;
    std::cout << "to_ulong()  = " << v.to_ulong()  << std::endl;
    std::cout << "to_string() = " << v.to_string() << std::endl;

    std::cout << "\n=== 六、LeetCode 191. Number of 1 Bits ===" << std::endl;
    std::uint32_t cases[] = {11u, 128u, 4294967293u};
    for (std::uint32_t n : cases) {
        std::cout << "n = " << n
                  << " → bitset 版 " << hammingWeightByBitset(n)
                  << "，手寫 Kernighan 版 " << hammingWeightManual(n)
                  << "，二進位 " << std::bitset<32>(n) << std::endl;
    }

    std::cout << "\n=== 七、日常實務：使用者權限旗標 ===" << std::endl;
    PermSet editorRole = makePerms({kRead, kWrite, kShare});
    PermSet extraGrant = makePerms({kExport});
    PermSet userPerms  = editorRole | extraGrant;          // 角色 OR 個人額外授權

    std::cout << "角色權限   " << editorRole << " = " << describe(editorRole) << std::endl;
    std::cout << "額外授權   " << extraGrant << " = " << describe(extraGrant) << std::endl;
    std::cout << "合併後     " << userPerms  << " = " << describe(userPerms)  << std::endl;

    PermSet required = makePerms({kRead, kWrite});
    bool ok = (userPerms & required) == required;          // 檢查是否具備全部必要權限
    std::cout << "需要 " << describe(required) << " → 是否具備: " << ok << std::endl;

    PermSet forbidden = makePerms({kAdmin, kBilling});
    userPerms &= ~forbidden;                               // 撤銷一組權限
    std::cout << "撤銷 " << describe(forbidden) << " 後 = "
              << describe(userPerms) << std::endl;
    std::cout << "目前共有 " << userPerms.count() << " 項權限" << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 18 課：vectorbool 的特殊性與陷阱13.cpp" -o bitset_demo
//
// 【關於下方預期輸出的但書】
//   sizeof(std::bitset<N>) 的數值屬「實作定義」：
//   libstdc++ 以 unsigned long（本機 x86-64 為 8 bytes）陣列儲存，
//   因此 bitset<8> 佔 8 bytes 而非 1 byte。
//   標準只規定 bitset 的行為，未規定其儲存佈局，
//   換平台或換標準庫這三個數值都可能不同。

// === 預期輸出 ===
// === 一、基本操作與索引方向 ===
// bs = 10001001（索引 0 在最右邊，顯示時最高位在左）
// size()  = 8（總位元數，等於模板參數 N）
// count() = 3（1 的個數）
// any()=true, none()=false, all()=false
//
// === 二、flip 與位元運算 ===
// flip 後：01110110
// bs2      = 11001100
// bs & bs2 = 01000100
// bs | bs2 = 11111110
// bs ^ bs2 = 10111010
// ~bs2     = 00110011
// bs2 << 2 = 00110000
// bs2 >> 2 = 00110011
// （vector<bool> 一個位元運算子都沒有）
//
// === 三、test() 有範圍檢查，operator[] 沒有 ===
// bs2.test(3) = true
// bs2.test(99) → 捕獲 std::out_of_range（test 有檢查；operator[] 越界則是 UB）
//
// === 四、記憶體：小 bitset 不會比較省 ===
// sizeof(std::bitset<8>)    = 8 bytes（不是 1！內部以字組為單位）
// sizeof(std::bitset<64>)   = 8 bytes
// sizeof(std::bitset<1024>) = 128 bytes（N 夠大時才看得出壓縮效果）
//
// === 五、與整數互轉 ===
// bitset<8>(0b10110101) = 10110101
// to_ulong()  = 181
// to_string() = 10110101
//
// === 六、LeetCode 191. Number of 1 Bits ===
// n = 11 → bitset 版 3，手寫 Kernighan 版 3，二進位 00000000000000000000000000001011
// n = 128 → bitset 版 1，手寫 Kernighan 版 1，二進位 00000000000000000000000010000000
// n = 4294967293 → bitset 版 31，手寫 Kernighan 版 31，二進位 11111111111111111111111111111101
//
// === 七、日常實務：使用者權限旗標 ===
// 角色權限   00001011 = Read|Write|Share
// 額外授權   10000000 = Export
// 合併後     10001011 = Read|Write|Share|Export
// 需要 Read|Write → 是否具備: true
// 撤銷 Admin|Billing 後 = Read|Write|Share|Export
// 目前共有 4 項權限
