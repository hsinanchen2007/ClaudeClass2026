// ============================================================================
// 課題 5：Destructor（解構子）與物件生命週期
// ============================================================================
//
// 解構子寫成 `~ClassName()`，沒有參數、沒有回傳型別。物件離開 scope、被 delete，
// 或其 owner 被銷毀時會自動執行。它應釋放「這個物件擁有」的資源，例如 heap
// memory、file descriptor、mutex lock；這正是 RAII 的基礎。
//
// 成員的解構順序與建構相反：先執行 destructor body，再依「宣告反序」解構 member，
// 最後解構 base class。destructor 通常不可丟 exception；stack unwinding 時再丟一個
// exception 會呼叫 std::terminate。
//
// 現代 C++ 優先讓 vector/string/unique_ptr 管資源，便可使用 compiler 產生的解構子
//（Rule of Zero）。本課手動 new[]/delete[] 是為了看清 ownership，不代表日常首選。
//
// 【面試】delete 與 delete[] 必須配對；base pointer 刪 derived object 時 base destructor
// 必須 virtual（第 18 課）。
// 【陷阱】手寫 destructor 後，compiler 產生的 shallow copy 可能造成 double free；
// 第 8、9、23 課會完整處理。
// ============================================================================

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <stdexcept>

class IntBuffer {
public:
    explicit IntBuffer(std::size_t size) : size_(size), data_(new int[size]{})
    {
        if (size == 0U) {
            delete[] data_;
            data_ = nullptr;
            throw std::invalid_argument("buffer must not be empty");
        }
    }

    ~IntBuffer() noexcept
    {
        delete[] data_;  // delete[] nullptr 也合法。
    }

    IntBuffer(const IntBuffer&) = delete;             // 本課先禁止危險 shallow copy。
    IntBuffer& operator=(const IntBuffer&) = delete;

    int& at(std::size_t index)
    {
        if (index >= size_) {
            throw std::out_of_range("IntBuffer index");
        }
        return data_[index];
    }

private:
    std::size_t size_;
    int* data_;
};

void basic_example()
{
    IntBuffer samples(3);
    samples.at(0) = 10;
    samples.at(1) = 20;
    assert(samples.at(0) + samples.at(1) == 30);
    // 離開函式時自動呼叫 ~IntBuffer，不需記得手動 cleanup。
    std::cout << "[基礎] IntBuffer 會在 scope 結束時釋放陣列\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 705. Design HashSet（設計雜湊集合）
// 題目：不使用內建 hash set，支援 add/remove/contains；例如加入 1、2 後包含 1，不包含 3。
// 為何使用本章主題：本例以直接位址 bool array 簡化題目，重點是 destructor 正確釋放 class 擁有的動態陣列。
// 思路：constructor 配置並清零 key range；checked 驗 key；三個操作讀寫對應 slot；destructor 用 delete[]。
// 複雜度：建構與空間 O(R)，每次操作 O(1)，R 為可接受 key 範圍 1,000,001。
// 易錯點：這不是一般碰撞式 hash table；new[] 必須配 delete[]；禁止預設 shallow copy 才不會 double free。
// -----------------------------------------------------------------------------
class MyHashSet {
public:
    MyHashSet() : present_(new bool[kRange]{}) {}
    ~MyHashSet() noexcept { delete[] present_; }

    MyHashSet(const MyHashSet&) = delete;
    MyHashSet& operator=(const MyHashSet&) = delete;

    void add(int key) { present_[checked(key)] = true; }
    void remove(int key) { present_[checked(key)] = false; }
    bool contains(int key) const { return present_[checked(key)]; }

private:
    static constexpr std::size_t kRange = 1'000'001U;

    static std::size_t checked(int key)
    {
        if (key < 0 || key > 1'000'000) {
            throw std::out_of_range("hash-set key");
        }
        return static_cast<std::size_t>(key);
    }

    bool* present_;
};

void leetcode_705_example()
{
    MyHashSet set;
    set.add(1);
    set.add(2);
    assert(set.contains(1));
    assert(!set.contains(3));
    set.remove(2);
    assert(!set.contains(2));
    std::cout << "[LeetCode 705] add/remove/contains 通過，離開時釋放 table\n";
}

// -----------------------------------------------------------------------------
// 【日常實務範例】追蹤作用域內的活躍 session 數
// 情境：測試或診斷需要在 session guard 建立時加一、離開 scope 時減一，確認 acquire/release 成對。
// 為何使用本章主題：destructor 在正常 return 與例外展開都會執行，比每條控制路徑手動遞減可靠。
// 設計：constructor 保存外部計數 reference 並遞增；destructor 遞減；禁止 copy 避免重複扣減。
// 成本：建立與解構皆 O(1)，無配置；同步成本目前未包含。
// 上線注意：外部 int 必須比 guard 活得久；跨執行緒需 atomic/mutex，destructor 不應拋例外。
// -----------------------------------------------------------------------------
class ScopeCounter {
public:
    explicit ScopeCounter(int& active) : active_(active) { ++active_; }
    ~ScopeCounter() noexcept { --active_; }
    ScopeCounter(const ScopeCounter&) = delete;
    ScopeCounter& operator=(const ScopeCounter&) = delete;

private:
    int& active_;  // 呼叫端須保證被參照物件活得比 ScopeCounter 久。
};

void practical_example()
{
    int active_sessions = 0;
    {
        ScopeCounter first(active_sessions);
        ScopeCounter second(active_sessions);
        assert(active_sessions == 2);
    }
    assert(active_sessions == 0);
    std::cout << "[實務] scope 結束後 active sessions=0\n";
}

int main()
{
    basic_example();
    leetcode_705_example();
    practical_example();
}

// 練習：把 IntBuffer 改用 std::vector<int>，刪除自訂 destructor，體會 Rule of Zero。
// 複雜度：destructor 成本為成員/資源釋放總和，container 解構通常 O(element count)。
// 生命週期：destructor body 後才逆序解構 members/bases；不要從 destructor 洩漏 this 給非同步工作。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '5_Destructor.cpp' -o '/tmp/codex_cpp_C_OOP_5_Destructor' && '/tmp/codex_cpp_C_OOP_5_Destructor'
//
// === 預期輸出（節錄）===
// [實務] scope 結束後 active sessions=0
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
