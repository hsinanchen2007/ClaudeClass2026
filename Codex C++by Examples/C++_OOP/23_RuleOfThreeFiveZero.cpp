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

// LeetCode 208：Implement Trie。
// Node 用 array<unique_ptr>，Trie 不需 destructor/copy/move 實作，就是 Rule of Zero。
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

// 實務案例：Config 只含 Rule-of-Zero members，compiler-generated operations 已正確。
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
