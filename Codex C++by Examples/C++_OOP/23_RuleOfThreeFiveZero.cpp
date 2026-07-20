// ============================================================================
// 課題 23：Rule of Three / Five / Zero
// ============================================================================
//
// 若 class 手動擁有 resource，通常要一起考慮：
//   Rule of Three：destructor、copy constructor、copy assignment。
//   Rule of Five：再加 move constructor、move assignment。
// 少寫一個常造成 leak、double free、昂貴 copy 或被刪除的 operation。
//
// 最推薦 Rule of Zero：把 ownership 交給 vector/string/unique_ptr 等 RAII member，class
// 不自行宣告五個 special members，compiler 逐 member 行為自然正確。只有 ownership
// semantics 特殊時才手寫，並用 tests/sanitizers 驗證。
//
// 【面試】user-declared destructor 會影響 implicit move generation；不要只為印 log 就
// 隨便加入 destructor。
// 【設計】polymorphic base 常需 virtual destructor，copy/move 則可 `= default/delete`。
// ============================================================================

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

class LegacyBuffer {
public:
    explicit LegacyBuffer(std::size_t size)
        : size_(size), data_(size == 0U ? nullptr : new int[size]{}) {}

    ~LegacyBuffer() noexcept { delete[] data_; }

    LegacyBuffer(const LegacyBuffer& other) : LegacyBuffer(other.size_)
    {
        if (other.size_ > 0U) {
            std::copy(other.data_, other.data_ + other.size_, data_);
        }
    }

    LegacyBuffer& operator=(const LegacyBuffer& other)
    {
        if (this != &other) {
            LegacyBuffer copy(other);
            swap(copy);
        }
        return *this;
    }

    LegacyBuffer(LegacyBuffer&& other) noexcept
        : size_(std::exchange(other.size_, 0U)),
          data_(std::exchange(other.data_, nullptr)) {}

    LegacyBuffer& operator=(LegacyBuffer&& other) noexcept
    {
        if (this != &other) {
            delete[] data_;
            size_ = std::exchange(other.size_, 0U);
            data_ = std::exchange(other.data_, nullptr);
        }
        return *this;
    }

    void swap(LegacyBuffer& other) noexcept
    {
        using std::swap;
        swap(size_, other.size_);
        swap(data_, other.data_);
    }
    int& at(std::size_t index) { return data_[index]; }
    int at(std::size_t index) const { return data_[index]; }

private:
    std::size_t size_;
    int* data_;
};

void basic_example()
{
    LegacyBuffer first(1U);
    first.at(0U) = 7;
    LegacyBuffer copy(first);
    LegacyBuffer moved(std::move(copy));
    assert(first.at(0U) == 7 && moved.at(0U) == 7);
    std::cout << "[基礎] 手寫 Rule of Five copy/move 都保有 ownership\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 208. Implement Trie (Prefix Tree)（實作 Trie 前綴樹）
// 題目：支援 insert、完整字 search、prefix startsWith；例如插入 apple 後 search(app)=false、startsWith(app)=true。
// 為何使用本章主題：Node 以 array<unique_ptr> 擁有子節點，Trie 不手寫 destructor/copy/move 即由 RAII 完成 Rule of Zero。
// 思路：insert 逐字建立缺少 child；末節點標 word；find 逐字沿 child；兩種查詢再判節點/word 狀態。
// 複雜度：每次操作時間 O(L)，新增最多 O(L) 節點，L 為字串長度；總空間 O(總節點數*26 pointers)。
// 易錯點：只接受 a..z，assert 在 release 不可當輸入驗證；search 與 startsWith 的 word flag 契約不同。
// -----------------------------------------------------------------------------
class Trie {
public:
    void insert(const std::string& word)
    {
        Node* node = &root_;
        for (const char character : word) {
            const std::size_t index = letter_index(character);
            if (node->children[index] == nullptr) {
                node->children[index] = std::make_unique<Node>();
            }
            node = node->children[index].get();
        }
        node->word = true;
    }

    bool search(const std::string& word) const
    {
        const Node* node = find(word);
        return node != nullptr && node->word;
    }

    bool startsWith(const std::string& prefix) const { return find(prefix) != nullptr; }

private:
    struct Node {
        std::array<std::unique_ptr<Node>, 26> children{};
        bool word = false;
    };

    static std::size_t letter_index(char character)
    {
        assert(character >= 'a' && character <= 'z');
        return static_cast<std::size_t>(character - 'a');
    }

    const Node* find(const std::string& text) const
    {
        const Node* node = &root_;
        for (const char character : text) {
            const auto& child = node->children[letter_index(character)];
            if (child == nullptr) return nullptr;
            node = child.get();
        }
        return node;
    }

    Node root_;
};

void leetcode_208_example()
{
    Trie trie;
    trie.insert("apple");
    assert(trie.search("apple"));
    assert(!trie.search("app"));
    assert(trie.startsWith("app"));
    trie.insert("app");
    assert(trie.search("app"));
    std::cout << "[LeetCode 208] Rule-of-Zero Trie operations 通過\n";
}

// -----------------------------------------------------------------------------
// 【日常實務範例】可安全複製的部署設定值
// 情境：從 production 設定建立 staging 副本，修改 staging 名稱後，原設定與 limits 都應保持獨立且有效。
// 為何使用本章主題：Config 只含 string/vector RAII members，compiler-generated special members 已提供正確 value semantics。
// 設計：定義純值 members；以 production copy-construct staging；修改副本；比較原名稱與 limits。
// 成本：copy 時間與額外空間 O(N+S)，N 為 limits 數、S 為字串長度；move 通常較便宜。
// 上線注意：加入 raw pointer、iterator 或外部 handle 後要重新審視 copy 語意；設定驗證仍須由 constructor/factory 負責。
// -----------------------------------------------------------------------------
struct Config {
    std::string name;
    std::vector<int> limits;
};

void practical_example()
{
    Config production{"prod", {4, 8}};
    Config staging = production;
    staging.name = "staging";
    assert(production.name == "prod" && staging.limits == production.limits);
    std::cout << "[實務] Config 採 Rule of Zero，不需 boilerplate\n";
}

int main()
{
    basic_example();
    leetcode_208_example();
    practical_example();
}

// 易錯與面試：自行宣告 destructor 會影響 implicit move generation；不要機械式寫五個 member。
// 優先 Rule of Zero，以 vector/string/unique_ptr 讓成員自然決定 copy/move 語意。
// 練習：把 LegacyBuffer 改成 vector<int>，刪掉全部 five special members。
// 複雜度：手寫 deep copy O(N)、move O(1)；Rule of Zero 讓 member type 自己提供相同語意。
// 生命週期：每個 resource 應只有清楚 owner；自行宣告 destructor 會影響 implicit move generation。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '23_RuleOfThreeFiveZero.cpp' -o '/tmp/codex_cpp_C_OOP_23_RuleOfThreeFiveZero' && '/tmp/codex_cpp_C_OOP_23_RuleOfThreeFiveZero'
//
// === 預期輸出（節錄）===
// [實務] Config 採 Rule of Zero，不需 boilerplate
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
