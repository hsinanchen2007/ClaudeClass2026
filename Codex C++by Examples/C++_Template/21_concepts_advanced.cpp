/*
 * 第 21 章：進階 Concepts 與 requires-expression
 *
 * requires-expression 可包含四種 requirement：
 * 1. simple：expr; 只求可形成。
 * 2. type：typename T::value_type;
 * 3. compound：{ expr } noexcept -> Concept;
 * 4. nested：requires BooleanConstraint<T>;
 * Concept 可組合，但要避免兩個語意相同、語法不同的限制造成 overload subsumption 意外。
 */

/*
 * 【離線教材：模型與取捨】
 * 1. requires-expression 有 simple、type、compound、nested 四種 requirement，可組成精確結構契約。
 * 2. Simple requirement `expr;` 只問 expression 能否形成，不檢查結果值，也不會真的執行。
 * 3. Type requirement `typename R::value_type;` 只驗證名稱是型別，不證明它和 Entity 相同。
 * 4. Compound requirement `{ expr } -> same_as<X>` 同時檢查 expression 與精確結果型別。
 * 5. Nested requirement `requires C<T>;` 可嵌入其他 constraint，本例用它連結 repository 元素型別。
 * 6. `noexcept` 可寫在 compound requirement 中，但只有 API 真承諾不丟例外時才應限制。
 * 7. Hashable 只證明 std::hash<T> 呼叫可形成且結果可轉 size_t，不證明分布品質或一致性。
 * 8. unordered_map 還需 key 可複製；把 copy_constructible 放進 concept 可避免錯誤掉進 emplace 本體。
 * 9. 相等的 key 必須產生相同 hash 是語意契約，編譯器與 requires-expression 都無法證明。
 * 10. Two Sum 平均時間 O(n)、最壞 O(n^2)、空間 O(n)；reserve 可降低一般情況 rehash 次數。
 * 11. `target-values[i]` 對 T 必須有定義且不得 overflow；same_as<T> 只檢查結果型別，不保證安全。
 * 12. LeetCode 契約要求恰有一組解；本泛型版無解時以 {size,size} 回傳，呼叫端必須辨識。
 * 13. vector 與 target 以 const/value 借用或複製；unordered_map 在函式內擁有 key，回傳只含索引。
 * 14. WritableRepository 分開使用 mutable repository 做 save、const repository 做 find，契約更貼近讀寫邊界。
 * 15. find 回傳 pointer 的生命週期屬 repository；repository 銷毀或該元素被 erase 後不可再解參照。
 * 16. unordered_map rehash 會使 iterator 失效，但不使元素 reference/pointer 失效；erase 才會失效。
 * 17. practical_register_user 按值接收字串並 move 進 User，適合呼叫端移交所有權的 API。
 * 18. Concept 分派本身零 runtime 成本；每個 Repository/T 仍可能產生 specialization 與 code size。
 * 19. Constraint normalization/subsumption 影響 overload 優先序；重用同一命名 concept 最可靠。
 * 20. 兩個文字上等價但由不同 token 寫出的 requires，不一定建立編譯器可證明的 subsumption。
 * 21. 診斷時由最外層 concept 往內找第一個 false requirement，必要時提高 compiler 診斷深度。
 * 22. 面試追問：concept 是否 duck typing？它能檢查結構，但演算法公理與副作用仍靠文件契約。
 * 23. 面試追問：何時改用 abstract interface？需要 runtime 注入異質實作或穩定 binary ABI 時。
 */

#include <cassert>
#include <concepts>
#include <cstddef>
#include <functional>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

template <typename T>
concept Hashable = requires(const T& value) {
    { std::hash<T>{}(value) } -> std::convertible_to<std::size_t>;
};

template <typename T>
concept EqualityComparableHashKey =
    Hashable<T> && std::equality_comparable<T> && std::copy_constructible<T>;

template <typename T>
concept TwoSumKey = EqualityComparableHashKey<T> && requires(const T& left, const T& right) {
    { left - right } -> std::same_as<T>;
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1. Two Sum（兩數之和）
// 題目：從 values 找兩個相異索引使其值相加為 target；[2,7,11,15]、9 得 [0,1]。
// 為何使用本章主題：TwoSumKey 組合可雜湊、相等比較、複製與減法 requirement，讓不合格 T
// 在呼叫邊界被拒絕；原題只需 int，這是進階 concept 的泛化版。
// 思路：reserve 雜湊表；逐項算 wanted；先查互補值，命中回索引 pair，否則保存目前值。
// 複雜度：平均時間 O(N)、額外空間 O(N)，N 是 values 長度；碰撞時最壞可到 O(N^2)。
// 易錯點：concept 只驗減法型別，不能防 signed overflow 或證明 hash/equality 語意一致；無解回 {N,N}。
// -----------------------------------------------------------------------------
template <TwoSumKey T>
std::pair<std::size_t, std::size_t> leetcode_two_sum(const std::vector<T>& values, T target) {
    std::unordered_map<T, std::size_t> seen;
    seen.reserve(values.size());
    for (std::size_t i = 0; i < values.size(); ++i) {
        const T wanted = target - values[i];
        if (const auto found = seen.find(wanted); found != seen.end()) {
            return {found->second, i};
        }
        seen.emplace(values[i], i);
    }
    return {values.size(), values.size()};
}

// -----------------------------------------------------------------------------
// 【日常實務範例】受約束的使用者 Repository 註冊
// 情境：服務要接受可儲存 User 且可依 id 查詢的 repository，支援建立或更新使用者。
// 為何使用本章主題：WritableRepository 同時使用 type、nested、compound requirements，
// 把 value_type、save、const find 的完整結構契約放在函式簽名，便於替換測試 repository。
// 設計：concept 驗 Entity 一致；UserRepository 以 map 實作 save/find；register 移動 id/name 後呼叫 save。
// 成本：此實作平均 save/find O(1)、儲存 O(U)，U 是使用者數；concept 本身無 runtime 成本。
// 上線注意：find 回傳 pointer 受 repository/erase 生命週期限制；還需驗證欄位、同步寫入與回報失敗原因。
// -----------------------------------------------------------------------------
template <typename Repository, typename Entity>
concept WritableRepository = requires(Repository& repository, const Repository& read_only,
                                      const Entity& entity, const std::string& id) {
    typename Repository::value_type;                 // type requirement
    requires std::same_as<typename Repository::value_type, Entity>; // nested
    { repository.save(entity) } -> std::same_as<bool>;               // compound
    { read_only.find(id) } -> std::same_as<const Entity*>;
};

struct User {
    std::string id;
    std::string name;
};

class UserRepository {
public:
    using value_type = User;

    bool save(const User& user) {
        users_.insert_or_assign(user.id, user);
        return true;
    }

    const User* find(const std::string& id) const {
        const auto found = users_.find(id);
        return found == users_.end() ? nullptr : &found->second;
    }

private:
    std::unordered_map<std::string, User> users_;
};

template <typename Repository>
requires WritableRepository<Repository, User>
bool practical_register_user(Repository& repository, std::string id, std::string name) {
    return repository.save(User{std::move(id), std::move(name)});
}

int main() {
    static_assert(EqualityComparableHashKey<int>);
    static_assert(TwoSumKey<int>);
    [[maybe_unused]] const auto answer =
        leetcode_two_sum(std::vector<int>{2, 7, 11, 15}, 9);
    assert(answer.first == 0U && answer.second == 1U);
    assert((leetcode_two_sum(std::vector<int>{3, 3}, 6) ==
            std::pair<std::size_t, std::size_t>(0U, 1U)));
    assert((leetcode_two_sum(std::vector<int>{1, 2}, 99) ==
            std::pair<std::size_t, std::size_t>(2U, 2U)));

    static_assert(WritableRepository<UserRepository, User>);
    static_assert(!WritableRepository<int, User>);
    UserRepository repository;
    [[maybe_unused]] const bool created =
        practical_register_user(repository, "u-1", "Ada");
    assert(created);
    [[maybe_unused]] const User* user = repository.find("u-1");
    assert(user != nullptr && user->name == "Ada");
    [[maybe_unused]] const bool updated =
        practical_register_user(repository, "u-1", "Grace");
    assert(updated);
    user = repository.find("u-1");
    assert(user != nullptr && user->name == "Grace");
    assert(repository.find("missing") == nullptr);

    std::cout << "進階 concepts 測試完成\n";
}

/*
 * 【陷阱】Hashable 只證明語法可用，不證明 hash 與 equality 遵守一致性契約。
 * 【陷阱】requires-expression 回傳 bool；constraint failure 不是 static_assert error，候選會被排除。
 * 【面試】subsumption 是什麼？較受約束 overload 可優先，但依 constraint 正規化而非人類直覺。
 * 【練習】加入 ReadOnlyRepository concept，設計 overload 讓 WritableRepository 版本更特化。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '21_concepts_advanced.cpp' -o '/tmp/codex_cpp_C_Template_21_concepts_advanced' && '/tmp/codex_cpp_C_Template_21_concepts_advanced'
//
// === 預期輸出（節錄）===
// 進階 concepts 測試完成
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
