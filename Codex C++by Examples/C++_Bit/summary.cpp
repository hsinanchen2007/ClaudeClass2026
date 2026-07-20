// ============================================================================
// C++ 位元運算總複習：語法、<bit>、經典題型與協定旗標
// ============================================================================
//
// 【本章地圖：對應同目錄 01～12】
//   01 基本運算子       &, |, ^, ~, <<, >>
//   02 常見技巧         測試/設定/清除/切換 bit、lowbit、消除最低位 1
//   03 C++20 <bit>      popcount、countl/countr、bit_width、bit_floor/ceil、rotl/rotr
//   04 bit_cast         等大小 trivially-copyable 物件的位元表示轉換
//   05～11 LeetCode     191/136/268/338/231/201/78
//   12 實務 flags       權限、功能開關、封包欄位的 mask 設計
//
// 【運算真值與用途速查】
//   x & mask   mask 為 1 的位置才保留          測試、清除後取欄位
//   x | mask   mask 為 1 的位置強制設成 1      開啟旗標
//   x ^ mask   mask 為 1 的位置反轉            toggle、找唯一元素
//   ~x         每個 bit 反轉                    必須留意整個型別寬度
//   x << n     向高位移，對 unsigned 類似乘 2^n 但溢出高位會被捨棄
//   x >> n     unsigned 補 0；負 signed 的結果不適合拿來寫可攜協定程式
//
// 【最常背的式子：前置條件也是答案的一部分】
//   mask = U{1} << k                  建立第 k 位；0 <= k < digits(U)
//   (x & mask) != 0                   測第 k 位
//   x |= mask / x &= ~mask / x ^= mask  設定 / 清除 / 切換
//   x & (x - 1)                       清除最低位的 1；x 必須為 unsigned
//   x & (~x + 1)                      取 lowbit；unsigned 模數算術定義良好
//   x != 0 && (x & (x - 1)) == 0      判斷 2 的冪；不能漏掉 x != 0
//
// 【C++20 <bit> 選型表，複雜度通常視為 O(1)】
//   std::popcount(x)       1 的個數                    LeetCode 191
//   std::has_single_bit(x) 是否恰一個 1                LeetCode 231
//   std::countl_zero(x)    從最高位起的 0 數            normalize/hash table
//   std::countr_zero(x)    從最低位起的 0 數；x=0 有定義  對齊/lowbit index
//   std::bit_width(x)      表示 x 所需位數；x=0 -> 0
//   std::bit_floor(x)      <=x 的最大 2 次冪；x=0 -> 0
//   std::bit_ceil(x)       >=x 的最小 2 次冪；結果不可超過型別可表示範圍
//   std::rotl/rotr(x,n)    固定寬度 rotate，不會像手寫 shift 因 n=0 觸發 UB
//   std::endian            描述 native endian；協定仍須明訂 wire endian
//   std::bit_cast<T>(x)    sizeof 相同且兩端 trivially copyable；不是數值轉型
//
// 【型別與未定義行為：面試高頻】
//   1. 優先 std::uint32_t 等 unsigned 固定寬度型別；signed overflow 是 UB。
//   2. shift count 若為負，或 >= 左運算元提升後的 bit width，是 UB。
//   3. `1 << 31` 的 1 是 signed int；改 `std::uint32_t{1} << 31U`。
//   4. `~mask` 會經 integer promotion；最後應轉回目標 unsigned 型別。
//   5. 不以 bit-field struct 當 wire format：layout、padding、endian 皆非協定保證。
//   6. `bit_cast` 不處理 endian，也不保證任意 bit pattern 都是目標型別合法值。
//
// 【題型複雜度】
//   191 hamming weight：Kernighan O(k)，k=1 的個數；popcount 通常硬體指令。
//   136 single number：O(n) time / O(1) space，利用 x^x=0、x^0=x。
//   268 missing number：O(n) / O(1)，index 與 value 全 XOR；也可求和但會溢位。
//   338 counting bits：DP O(n) / O(n)，bits[i]=bits[i>>1]+(i&1)。
//   201 range AND：共同高位前綴，O(log max(left,right)) / O(1)。
//   78 subsets：O(n*2^n) output time / O(n) 暫存；bitmask 僅適合 n 小且寬度足夠。
//
// 【面試快問快答】
//   Q: XOR swap 值得用嗎？
//   A: 不值得；可讀性差、同一物件會出錯，編譯器對 std::swap 更可靠。
//   Q: 為何 range AND 能一直 right &= right-1？
//   A: 區間跨越的低位不可能維持為 1；逐次移除 right 的低位 1，直到 <= left。
//   Q: bit mask 最適合何時？
//   A: 固定且少量的布林狀態；需要動態數量、型別安全名稱或很多欄位時考慮 bitset/enum class/結構。
// ============================================================================

/*
==============================================================================
【面試深挖：Bit Operations】

B1｜位移最重要的 UB 邊界？
答：shift count 為負或 >= promoted left operand 位寬是 UB；signed negative 左移與結果不可表示
也有問題。位元演算法優先轉 unsigned 並檢查 count，不要依賴編譯器或 CPU 自動 mask。

B2｜負 signed integer 右移可攜嗎？
答：C++20 起定義為向負無限方向取整的 arithmetic right shift；較早標準為 implementation-defined。
跨版本教材應明寫版本，不可把 x86 行為當所有標準。

B3｜`x & (x-1)` 判 power of two 的陷阱？
答：必須先確認 x != 0，且最好使用 unsigned；它清掉最低 set bit。C++20 可直接用
`std::has_single_bit`，型別要求也更清楚。

B4｜`popcount`、`countl_zero`、`countr_zero` 的邊界？
答：C++20 <bit> 對 unsigned integer 提供可攜操作；countl/countr_zero(0) 有定義為位寬，
但手寫 compiler builtin 對 0 可能未定義，不能混為一談。

B5｜`bit_cast` 與 `reinterpret_cast`/union punning 的差別？
答：bit_cast 對等大小、trivially copyable types 複製 object representation，避免 aliasing
與 inactive union member 問題；它不是數值轉換，NaN/trap/padding 仍需理解目標型別規則。

B6｜如何可攜判斷 endian？
答：C++20 用 `std::endian::native`；或把 object representation 讀成 unsigned char/byte。
在 C++ 中寫 union 一個 member、讀另一個不是通用標準 type-punning 解法。

B7｜XOR single-number 技巧有哪些前提？
答：只有「其餘每個值恰出現偶數次、目標出現奇數次」才成立。它不驗證輸入契約，
也不能直接找兩個任意頻率的 unique values；先說 invariant 再寫一行 XOR。

B8｜bitmask enum 如何做得型別安全？
答：用 enum class 防止與任意 int 混算，明確提供 operator|/&/contains，底層用 unsigned type。
不要把 `~flag` 未遮罩後的所有高位直接當有效 flags。

B9｜`bitset`、dynamic bit vector、enum flags 怎麼選？
答：compile-time 固定 N 且需位元操作用 bitset；runtime N 用專用 dynamic bitset/word vector；
少量具名稱權限用 enum flags。vector<bool> 有 proxy semantics，API 必須有意接受。

B10｜`bit_ceil` 的 overflow 邊界？
答：結果若無法由型別表示，呼叫不具有效結果；容量 rounding 要先驗上限。
不要先做 `1 << bit_width(x)`，那同時有 signed 與 shift-count 風險。
==============================================================================
*/

#include <bit>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

using Word = std::uint32_t;

constexpr Word bit_mask(unsigned position)
{
    if (position >= 32U) {
        throw std::out_of_range("bit position");
    }
    return Word{1} << position;
}

constexpr bool is_set(Word value, unsigned position)
{
    return (value & bit_mask(position)) != 0U;
}

constexpr Word set_bit(Word value, unsigned position) { return value | bit_mask(position); }
constexpr Word clear_bit(Word value, unsigned position) { return value & ~bit_mask(position); }
constexpr Word toggle_bit(Word value, unsigned position) { return value ^ bit_mask(position); }

void basic_and_cpp20_demo()
{
    Word value = 0U;
    value = set_bit(value, 1U);       // 0010
    value = set_bit(value, 3U);       // 1010
    assert(value == 10U && is_set(value, 3U));
    value = toggle_bit(value, 1U);    // 1000
    assert(value == 8U);
    value = clear_bit(value, 3U);     // 0000
    assert(value == 0U);

    assert(std::popcount(0b101101U) == 4);
    assert(std::has_single_bit(64U));
    assert(std::bit_width(64U) == 7);
    assert(std::bit_floor(70U) == 64U);
    assert(std::bit_ceil(70U) == 128U);
    assert(std::rotl(std::uint8_t{0b10000001}, 1) == std::uint8_t{0b00000011});
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 338. Counting Bits（計算 0 到 n 的位元數）
// 題目：回傳 0..n 每個值的 set bit 數；n=5 得 [0,1,1,2,1,2]。
// 為何使用本章主題：`answer[i>>1]+(i&1)` 以 shift/mask 重用已算出的較小索引。
// 思路：1. 驗 n 非負；2. 建立 n+1 格；3. 從 1 起依 recurrence 填表。
// 複雜度：N 為上限；時間 O(N)、結果空間 O(N)。
// 易錯點：先驗負數再轉 size_t；迴圈以 answer.size() 為界，避免 n==INT_MAX 時 int 遞增溢位。
// -----------------------------------------------------------------------------
std::vector<int> count_bits(int n)
{
    if (n < 0) {
        throw std::invalid_argument("n must be non-negative");
    }
    std::vector<int> answer(static_cast<std::size_t>(n) + 1U, 0);
    // 用 size_t 走容器 index，避免 n==INT_MAX 時 int 的最後一次 ++value 溢位。
    for (std::size_t index = 1U; index < answer.size(); ++index) {
        answer[index] = answer[index >> 1U] + static_cast<int>(index & 1U);
    }
    return answer;
}

void leetcode_demo()
{
    assert((count_bits(5) == std::vector<int>{0, 1, 1, 2, 1, 2}));

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 136. Single Number（只出現一次的數字）
// 題目：除一值只出現一次外，其餘值恰出現兩次；[42,7,42,9,9] 回傳 7。
// 為何使用本章主題：本 summary 直接在示範段以 XOR 實作，利用相同 Word 值兩兩抵消。
// 思路：1. unique 從 0 開始；2. 逐 ID XOR；3. 回傳或驗證最後未抵消值。
// 複雜度：N 為 ID 數；時間 O(N)、額外空間 O(1)。
// 易錯點：此段沒有獨立 solver，是總複習內嵌實作；輸入若不符合成對契約，結果不可當驗證證據。
// -----------------------------------------------------------------------------
    const std::vector<Word> ids{42U, 7U, 42U, 9U, 9U};
    Word unique = 0U;
    for (const Word id : ids) {
        unique ^= id;
    }
    assert(unique == 7U);
}

// -----------------------------------------------------------------------------
// 【日常實務範例】型別安全的存取權限遮罩
// 情境：授權物件要授予、撤銷與查詢 read/write/execute/audit，並可輸出 raw bits 供儲存。
// 為何使用本章主題：enum class 命名每個單一 bit，AccessMask 封裝 |、&、~，避免 magic numbers 散落呼叫端。
// 設計：1. raw 將 Permission 轉 Word；2. grant/revoke 更新 bits_；3. has 判斷指定 bit 是否存在。
// 成本：每次操作時間與空間皆 O(1)，AccessMask 自身為單一 Word。
// 上線注意：raw_bits 跨 wire 前要定義版本與 endian，反序列化時也要遮掉或拒絕未知 bits。
// -----------------------------------------------------------------------------
enum class Permission : Word {
    read = Word{1} << 0U,
    write = Word{1} << 1U,
    execute = Word{1} << 2U,
    audit = Word{1} << 3U
};

class AccessMask {
public:
    void grant(Permission permission) { bits_ |= raw(permission); }
    void revoke(Permission permission) { bits_ &= ~raw(permission); }
    [[nodiscard]] bool has(Permission permission) const
    {
        return (bits_ & raw(permission)) != 0U;
    }
    [[nodiscard]] Word raw_bits() const { return bits_; }

private:
    static constexpr Word raw(Permission permission)
    {
        return static_cast<Word>(permission);
    }
    Word bits_{0U};
};

// -----------------------------------------------------------------------------
// 【日常實務範例】16-bit 封包 header 打包
// 情境：把 version[15:13]、encrypted[12]、payload_length[11:0] 打包成一個 uint16_t。
// 為何使用本章主題：shift 將欄位移到固定位置，OR 合併，mask 則讓解碼端取回各欄。
// 設計：1. 驗 version<=7 與 length<=0xFFF；2. 位移 version/encrypted；3. OR length 後窄化成 uint16_t。
// 成本：固定欄位值運算，時間與空間皆 O(1)。
// 上線注意：跨機傳輸還要明定 byte order；新增欄位時需版本化，不能讓位元重疊或截斷超範圍值。
// -----------------------------------------------------------------------------
std::uint16_t make_header(unsigned version, bool encrypted, unsigned payload_length)
{
    if (version > 7U || payload_length > 0x0FFFU) {
        throw std::out_of_range("packet header field");
    }
    const unsigned packed = (version << 13U)
        | (encrypted ? (1U << 12U) : 0U)
        | payload_length;
    return static_cast<std::uint16_t>(packed);
}

void practical_demo()
{
    AccessMask access;
    access.grant(Permission::read);
    access.grant(Permission::audit);
    assert(access.has(Permission::read));
    assert(!access.has(Permission::write));
    access.revoke(Permission::audit);
    assert(access.raw_bits() == 1U);

    const std::uint16_t header = make_header(5U, true, 1'024U);
    assert(((header >> 13U) & 0x7U) == 5U);
    assert(((header >> 12U) & 0x1U) == 1U);
    assert((header & 0x0FFFU) == 1'024U);
}

int main()
{
    basic_and_cpp20_demo();
    leetcode_demo();
    practical_demo();
    std::cout << "Bit summary: all assertions passed\n";
}

// 【章末自測】不用查資料，實作 popcount、single number 與 power-of-two，並說明 signed shift 風險。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'summary.cpp' -o '/tmp/codex_cpp_C_Bit_summary' && '/tmp/codex_cpp_C_Bit_summary'
//
// === 預期輸出（節錄）===
// Bit summary: all assertions passed
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
