// ============================================================================
// 課題 5：Custom deleter - 管理非 delete 資源
// ============================================================================
//
// unique_ptr<T,Deleter> 可在解構時呼叫 fclose/free/CloseHandle/cudaFree 等配對 release。
// deleter 是 unique_ptr 型別的一部分；stateful/function-pointer deleter 可能讓 pointer
// object 變大，stateless functor 常可 empty-base optimize。shared_ptr 的 deleter 放 control
// block，不是 shared_ptr 靜態型別的一部分。
//
// acquire/release 必須配對：malloc/free、fopen/fclose、new/delete。不可拿 malloc pointer
// 給 default_delete，也不可對 new pointer 呼叫 free。失敗 acquire 通常回 nullptr，owner
// 仍可安全持有 null。
// ============================================================================

#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>

struct FileCloser {
    void operator()(std::FILE* file) const noexcept
    {
        if (file != nullptr) std::fclose(file);
    }
};
using FileHandle = std::unique_ptr<std::FILE, FileCloser>;

void basic_example()
{
    FileHandle file(std::tmpfile());
    if (file == nullptr) throw std::runtime_error("tmpfile failed");
    // fputs/fflush 是必要 I/O；NDEBUG 會移除整個 assert expression，不能把寫入藏在裡面。
    [[maybe_unused]] const int write_result = std::fputs("CUDA", file.get());
    assert(write_result >= 0);
    [[maybe_unused]] const int flush_result = std::fflush(file.get());
    assert(flush_result == 0);
    std::cout << "[基礎] FILE* will fclose on every scope-exit path\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 705. Design HashSet（設計雜湊集合）
// 題目：不使用內建 hash set 實作 add/remove/contains；例如加入 1、2 後包含 1，移除 2 後不包含 2。
// 為何使用本章主題：本例以 calloc 直接位址表簡化題目，custom FreeDeleter 確保 allocation 與 free 正確配對。
// 思路：constructor calloc 並驗失敗；checked 驗 key；三操作讀寫 byte slot；owner 解構自動呼叫 free。
// 複雜度：初始化/空間 O(R)，每次操作 O(1)，R 為 key range 1,000,001。
// 易錯點：這不是一般 collision hash table；calloc 必須配 free；deleter 型別要接受實際 pointer 且不得拋例外。
// -----------------------------------------------------------------------------
struct FreeDeleter {
    void operator()(void* pointer) const noexcept { std::free(pointer); }
};

class MyHashSet {
public:
    MyHashSet() : table_(static_cast<unsigned char*>(std::calloc(kSize, 1U)))
    {
        if (table_ == nullptr) throw std::bad_alloc();
    }
    void add(int key) { table_.get()[checked(key)] = 1U; }
    void remove(int key) { table_.get()[checked(key)] = 0U; }
    bool contains(int key) const { return table_.get()[checked(key)] != 0U; }
private:
    static constexpr std::size_t kSize = 1'000'001U;
    static std::size_t checked(int key)
    {
        if (key < 0 || key > 1'000'000) throw std::out_of_range("key");
        return static_cast<std::size_t>(key);
    }
    std::unique_ptr<unsigned char, FreeDeleter> table_;
};

void leetcode_705_example()
{
    MyHashSet set;
    set.add(1);
    set.add(2);
    assert(set.contains(1) && !set.contains(3));
    set.remove(2);
    assert(!set.contains(2));
    std::cout << "[LeetCode 705] calloc table will be released by free deleter\n";
}

// -----------------------------------------------------------------------------
// 【日常實務範例】帶釋放狀態的 C API handle owner
// 情境：外部 handle 離開 scope 時除釋放資源，還要把 release_count 加一供測試與可觀測性驗證。
// 為何使用本章主題：stateful custom deleter 將 release context 與 owner 綁定，比散落的手動 close 路徑可靠。
// 設計：deleter 保存 counter pointer；owner 接管 handle；scope exit 呼叫 delete；隨後累加一次計數。
// 成本：owner 操作 O(1)，釋放成本由外部 API 決定；stateful deleter 通常增加 unique_ptr 大小。
// 上線注意：release_count 必須比 owner 活得久且並行時需 atomic；deleter 應處理 API 錯誤但不可讓例外逃出。
// -----------------------------------------------------------------------------
struct HandleDeleter {
    int* release_count;
    void operator()(int* handle) const noexcept
    {
        delete handle;
        ++(*release_count);
    }
};

void practical_example()
{
    int releases = 0;
    {
        std::unique_ptr<int, HandleDeleter> handle(new int(42), HandleDeleter{&releases});
        assert(*handle == 42);
    }
    assert(releases == 1);
    std::cout << "[實務] stateful deleter invoked exactly once\n";
}

int main()
{
    basic_example();
    leetcode_705_example();
    practical_example();
}

// 練習：比較 sizeof(unique_ptr<int>) 與帶 function-pointer/stateful deleter 的大小。
// 複雜度：pointer 操作通常 O(1)，真正釋放成本由 fclose/free/外部 API deleter 決定。
// 生命週期：smart pointer 從成功取得 handle 起擁有資源，move 轉移 owner，deleter 必須活在其內。

/*
【本課面試問答】
Q1：custom deleter 為何是 `unique_ptr` 型別的一部分，卻不是 `shared_ptr` 型別的一部分？
A：`unique_ptr<T,D>` 直接把 deleter 存在物件內，因此 `D` 影響型別、move 性質與 `sizeof`；
`shared_ptr<T>` 把 type-erased deleter 放在 control block，所以不同 deleter 的 shared_ptr 型別相同。

Q2：空 deleter 一定讓 `unique_ptr` 變大嗎？
A：不一定。實作通常利用 empty-base/`[[no_unique_address]]` 類最佳化，使無狀態 deleter 不增加大小，
但標準不保證 `sizeof(unique_ptr<T,D>) == sizeof(T*)`。function pointer 或有狀態 deleter 通常會增加大小。

Q3：deleter 應滿足哪些工程條件？
A：必須與取得資源的 API 成對（`malloc/free`、`fopen/fclose` 等），其捕捉狀態要活在 smart pointer
內，且通常不得讓清理例外逃出 destructor。錯配 `new[]/delete` 或 `malloc/delete` 是未定義行為。
*/

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '05_custom_deleter.cpp' -o '/tmp/codex_cpp_C_SmartPointers_05_custom_deleter' && '/tmp/codex_cpp_C_SmartPointers_05_custom_deleter'
//
// === 預期輸出（節錄）===
// [實務] stateful deleter invoked exactly once
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
