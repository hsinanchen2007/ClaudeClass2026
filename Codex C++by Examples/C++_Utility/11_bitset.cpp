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

// LeetCode 191：Number of 1 Bits。bitset::count 通常映射到高效 popcount。
int leetcode_hamming_weight(unsigned int value) {
    return static_cast<int>(std::bitset<sizeof(unsigned int) * 8U>(value).count());
}

// 【實務情境】固定四種權限以一個小型 bit mask 儲存與稽核。
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
