/*
 * 第 11 章：std::bitset<N>
 *
 * bitset 是固定 N bits 的值型別，N 在編譯期決定。它支援 &,|,^,~,<<,>>、set/reset/flip、
 * test/count/any/all/none，通常比 vector<bool> 更適合固定協定旗標與權限。若位數執行期才知，
 * 可用 dynamic bitset（第三方）或自己以 vector<uint64_t> 表示。
 */

#include <bitset>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>

std::bitset<8> basic_flags() {
    std::bitset<8> flags;
    flags.set(0U);  // 00000001
    flags.set(3U);  // 00001001
    flags.flip(0U); // 00001000
    return flags;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 191. Number of 1 Bits（位元 1 的個數）
// 題目：計算無號整數二進位表示中 1 的數量；二進位 ...1011 的答案是 3。
// 為何使用本章主題：bitset 在編譯期固定為 unsigned int 的位寬，count 直接計算已設定位元；
// 原題明定 32-bit，這個版本使用平台 unsigned int 寬度，是教學上的可攜型別差異。
// 思路：用 value 建構同位寬 bitset；呼叫 count；把 size_t 結果轉成 int。
// 複雜度：令 W 為 unsigned int 位數，時間約 O(W/word_bits)、額外空間 O(W)，W 為編譯期常數。
// 易錯點：不要假設所有平台 unsigned int 都是 32 位；若要符合題目介面應改用 uint32_t。
// -----------------------------------------------------------------------------
int leetcode_hamming_weight(unsigned int value) {
    return static_cast<int>(std::bitset<sizeof(unsigned int) * 8U>(value).count());
}

// -----------------------------------------------------------------------------
// 【日常實務範例】固定角色權限集合
// 情境：服務要授予、撤銷與查詢 read/write/deploy/admin 四種固定權限，並輸出位元供稽核。
// 為何使用本章主題：bitset<4> 緊湊表示封閉權限集合，set/reset/test 對應明確；
// 相較 unordered_set 沒有配置與雜湊成本，且可直接輸出固定寬度快照。
// 設計：enum 值映射到 bit index；grant/revoke 更新單 bit；has 查詢，audit_bits 輸出四位字串。
// 成本：四個固定 bit 的操作與空間皆 O(1)，稽核字串固定長度 4。
// 上線注意：enum 數值與持久化協定必須穩定；新增權限要同步調整 N，並驗證外部轉入的 index。
// -----------------------------------------------------------------------------
enum class Permission : std::size_t {
    read = 0,
    write = 1,
    deploy = 2,
    admin = 3
};

class Permissions {
public:
    void grant(Permission permission) { bits_.set(index(permission)); }
    void revoke(Permission permission) { bits_.reset(index(permission)); }
    bool has(Permission permission) const { return bits_.test(index(permission)); }
    std::string audit_bits() const { return bits_.to_string(); }

private:
    static constexpr std::size_t index(Permission permission) {
        return static_cast<std::size_t>(permission);
    }
    std::bitset<4> bits_;
};

void practical_permissions_test() {
    Permissions permissions;
    permissions.grant(Permission::read);
    permissions.grant(Permission::deploy);
    assert(permissions.has(Permission::read));
    assert(!permissions.has(Permission::write));
    assert(permissions.audit_bits() == "0101"); // to_string 由最高 bit 印到最低 bit

    permissions.revoke(Permission::deploy);
    assert(!permissions.has(Permission::deploy));
}

int main() {
    const std::bitset<8> flags = basic_flags();
    assert(flags.to_ulong() == 8UL);
    assert(flags.count() == 1U);
    assert(flags.any() && !flags.all() && !flags.none());

    assert(leetcode_hamming_weight(0b00000000000000000000000000001011U) == 3);
    assert(leetcode_hamming_weight(0U) == 0);
    assert(leetcode_hamming_weight(~0U) == static_cast<int>(sizeof(unsigned int) * 8U));

    practical_permissions_test();

    const std::bitset<4> left("1100");
    const std::bitset<4> right("1010");
    assert((left & right) == std::bitset<4>("1000"));
    assert((left | right) == std::bitset<4>("1110"));

    std::cout << "bitset 測試完成\n";
}

/*
 * 【常見陷阱】
 * - operator[] 不做邊界檢查；test(position) 超界會丟 out_of_range。
 * - to_ulong/to_ullong 若高位無法容納會丟 overflow_error。
 * - bitset 字串左邊是最高位，index 0 卻是最低位，初學時最常看反。
 * - `bitset<N>::reference` 是 proxy，不是 bool&；auto 推導後生命週期依附原 bitset。
 *
 * 【面試段落】bitset.count 的複雜度標準不保證常數；實作通常按 machine words 掃描。
 * 【練習】實作 Permission mask 的 union/intersection，測試角色權限合併。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '11_bitset.cpp' -o '/tmp/codex_cpp_C_Utility_11_bitset' && '/tmp/codex_cpp_C_Utility_11_bitset'
//
// === 預期輸出（節錄）===
// bitset 測試完成
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
