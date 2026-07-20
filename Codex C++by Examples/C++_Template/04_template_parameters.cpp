/*
 * 第 04 章：模板參數的三大類
 *
 * 1. type parameter：typename T。
 * 2. non-type template parameter (NTTP)：std::size_t N，值在編譯期已知。
 * 3. template-template parameter：把另一個模板當參數（第 10 章深入）。
 * 多個參數可同時使用；參數順序應讓可推導者優先、常用預設值放後面。
 */

/*
 * 【離線教材：模型與取捨】
 * 1. 型別參數 T 決定元素可做的操作與儲存配置；它不是執行期的 type id。
 * 2. 值參數 N/Policy 是型別的一部分，必須是可用於模板引數的編譯期值。
 * 3. 模板模板參數 Storage 接受「型別族」；本例預設傳入 std::array，而非某個 array 物件。
 * 4. 語法 `template <typename, size_t> typename Storage` 描述被傳模板的參數形狀。
 * 5. 呼叫 running_sum 時，T 與 N 都可由 std::array<T, N> 推導，不必手寫尖括號。
 * 6. 容量若來自設定檔或使用者輸入，應選 vector；只有容量真是編譯期契約才選 NTTP。
 * 7. FixedBuffer<int, 3> 與 FixedBuffer<int, 4> 不可互相指定，也會有不同 ABI 名稱。
 * 8. FixedBuffer::at 是 O(1) 且越界丟例外；直接索引雖可更快診斷少，卻沒有邊界保護。
 * 9. running_sum 時間 O(N)、額外空間 O(1)，但按值參數會複製一份 N 個元素的輸入。
 * 10. 每組 T/N/Policy 都可能產生一份機器碼；大量不同 N 會增加 binary code size。
 * 11. 編譯器也必須逐組實體化與最佳化，模板組合數增加時會拉長 compile time。
 * 12. data_ 直接擁有 N 個 T；RingLog 即使 used()==0，也已建構全部槽位的 T。
 * 13. 因此簡化版要求 T 可預設建構且能由 T&& 指定（通常走 move assignment）。
 * 14. append(T) 先按值取得所有權，再 move 進槽位；呼叫者傳 lvalue 時會先複製一次。
 * 15. RingLog 的 N=0 會讓覆寫路徑取模為零，所以用 static_assert 在錯誤實體化處診斷。
 * 16. LeetCode 契約：輸入 array 長度固定，元素的 `+=` 必須有效，且累加不得溢位。
 * 17. 空 array 是合法邊界，迴圈不執行；有號整數 overflow 則是未定義行為，不會自動飽和。
 * 18. 實務契約：reject 不修改已滿資料；overwrite_oldest 接受新值並循環覆蓋最舊槽位。
 * 19. 編譯期 policy 消除 runtime 分支，但若部署時才決定策略，enum 成員或 virtual 較合適。
 * 20. 面試追問：NTTP 換速度的代價是什麼？答案應涵蓋型別爆炸、編譯時間與指令快取。
 * 21. 面試追問：何時改用 span？當函式只借用既有連續資料且不應擁有固定容量時。
 */

#include <array>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

template <typename T, std::size_t N,
          template <typename, std::size_t> typename Storage = std::array>
class FixedBuffer {
public:
    constexpr std::size_t size() const noexcept { return N; }

    T& at(std::size_t index) {
        if (index >= N) {
            throw std::out_of_range("FixedBuffer index");
        }
        return data_[index];
    }

    const T& at(std::size_t index) const {
        if (index >= N) {
            throw std::out_of_range("FixedBuffer index");
        }
        return data_[index];
    }

private:
    Storage<T, N> data_{};
};

// LeetCode 1480：Running Sum，以 array 大小作 NTTP，回傳型別保留相同長度。
template <typename T, std::size_t N>
constexpr std::array<T, N> leetcode_running_sum(std::array<T, N> values) {
    for (std::size_t i = 1; i < N; ++i) {
        values[i] += values[i - 1];
    }
    return values;
}

enum class OverflowPolicy { reject, overwrite_oldest };

// 實務：編譯期容量與策略；物件不需額外保存 capacity/policy 欄位。
template <typename T, std::size_t N, OverflowPolicy Policy>
class RingLog {
    static_assert(N > 0U, "RingLog 容量必須大於零");

public:
    bool append(T value) {
        if (used_ < N) {
            data_[used_++] = std::move(value);
            return true;
        }
        if constexpr (Policy == OverflowPolicy::reject) {
            return false;
        } else {
            data_[next_] = std::move(value);
            next_ = (next_ + 1U) % N;
            return true;
        }
    }

    [[nodiscard]] std::size_t used() const noexcept { return used_; }

    const T& newest() const {
        if (used_ == 0U) {
            throw std::out_of_range("RingLog is empty");
        }
        const std::size_t index = used_ < N ? used_ - 1U : (next_ + N - 1U) % N;
        return data_[index];
    }

private:
    std::array<T, N> data_{};
    std::size_t used_{};
    std::size_t next_{};
};

void practical_ring_log_test() {
    RingLog<int, 2, OverflowPolicy::reject> audit;
    [[maybe_unused]] const bool audit_first = audit.append(10);
    [[maybe_unused]] const bool audit_second = audit.append(20);
    [[maybe_unused]] const bool audit_overflow = audit.append(30);
    assert(audit_first);
    assert(audit_second);
    assert(!audit_overflow);
    assert(audit.used() == 2U);
    assert(audit.newest() == 20);

    RingLog<int, 2, OverflowPolicy::overwrite_oldest> recent;
    [[maybe_unused]] const bool recent_first = recent.append(10);
    [[maybe_unused]] const bool recent_second = recent.append(20);
    [[maybe_unused]] const bool recent_overwrite = recent.append(30);
    assert(recent_first);
    assert(recent_second);
    assert(recent_overwrite);
    assert(recent.used() == 2U);
    assert(recent.newest() == 30);
}

int main() {
    FixedBuffer<std::string, 3> names;
    names.at(0) = "Ada";
    names.at(1) = "Bjarne";
    assert(names.size() == 3U);
    assert(names.at(1) == "Bjarne");

    [[maybe_unused]] bool out_of_range = false;
    try {
        static_cast<void>(names.at(names.size()));
    } catch (const std::out_of_range&) {
        out_of_range = true;
    }
    assert(out_of_range);

    constexpr auto sums = leetcode_running_sum(std::array{1, 2, 3, 4});
    static_assert(sums == std::array{1, 3, 6, 10});
    constexpr auto empty_sums = leetcode_running_sum(std::array<int, 0>{});
    static_assert(empty_sums.empty());

    practical_ring_log_test();

    std::cout << "型別參數與值參數測試完成\n";
}

/*
 * 【重點】FixedBuffer<int, 3> 與 FixedBuffer<int, 4> 是完全不同型別。
 * 【陷阱】N=0 時 ring 演算法會出現除以零；本例已用 static_assert 拒絕該型別。
 * 【面試】NTTP 的優點？尺寸可進入型別系統、利於最佳化；缺點是每個值可能產生新實體。
 * 【練習】為 RingLog 加 static_assert(N > 0) 與 newest() 查詢。
 */

/*
 * 【教科書補充：固定容量不等於自動強例外保證】
 * - `data_[used_++] = value` 會先增加 used_；若 T 的 assignment 拋出，logical size 可能與內容不一致。
 * - overwrite 路徑同樣取決於 T 的 assignment guarantee，不能無條件宣稱 transaction/rollback。
 * - 本檔以 C++20 建置；其中 std::array 的 constexpr equality 不應被誤認為所有舊標準都支援。
 * - production container 應先完成可能失敗的寫入，再 commit 索引，或以 guard 回復 metadata。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '04_template_parameters.cpp' -o '/tmp/codex_cpp_C_Template_04_template_parameters' && '/tmp/codex_cpp_C_Template_04_template_parameters'
//
// === 預期輸出（節錄）===
// 型別參數與值參數測試完成
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
