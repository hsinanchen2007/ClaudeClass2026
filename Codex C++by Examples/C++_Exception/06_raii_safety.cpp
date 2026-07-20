// ============================================================================
// 課題 6：RAII 與 exception safety guarantees
// ============================================================================
//
// 三種常見 guarantee：
//   no-throw：operation 保證不丟（destructor/swap 常需要）。
//   strong：失敗後 observable state 完全不變（transaction/copy-and-swap）。
//   basic：失敗後 object 仍合法、無 leak，但內容可能改變。
//
// RAII 解決「資源一定釋放」，不自動保證 business state rollback。Strong guarantee 常先在
// temporary 完成所有可能丟的工作，最後用 noexcept swap/commit。多個外部 side effects
//（DB+email）無法靠 C++ object rollback，需 transaction/idempotency/compensation 設計。
// ============================================================================

#include <array>
#include <cassert>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

class Settings {
public:
    explicit Settings(std::vector<int> values) : values_(std::move(values)) {}
    void replace_with_validated(std::vector<int> candidate)
    {
        for (const int value : candidate) {
            if (value < 0) throw std::invalid_argument("negative setting");
        }
        values_.swap(candidate); // noexcept commit；驗證失敗時 values_ 未動。
    }
    const std::vector<int>& values() const { return values_; }
private:
    std::vector<int> values_;
};

void basic_example()
{
    Settings settings({1, 2});
    try { settings.replace_with_validated({3, -1}); }
    catch (const std::invalid_argument&) {}
    assert((settings.values() == std::vector<int>{1, 2})); // strong guarantee。
    settings.replace_with_validated({3, 4});
    assert((settings.values() == std::vector<int>{3, 4}));
    std::cout << "[基礎] validate-then-swap provides strong guarantee\n";
}

// LeetCode 208：Trie。unique_ptr 讓 partial insertion 即使 allocation 丟 bad_alloc 也不 leak；
// operation 至少 basic guarantee。要 strong guarantee（整個 word 要嘛全插要嘛不插）需額外
// staging/rollback，LeetCode 通常不要求。
class Trie {
public:
    void insert(const std::string& word)
    {
        Node* node = &root_;
        for (const char character : word) {
            const std::size_t index = static_cast<std::size_t>(character - 'a');
            if (node->children.at(index) == nullptr) {
                node->children.at(index) = std::make_unique<Node>();
            }
            node = node->children.at(index).get();
        }
        node->word = true;
    }
    bool search(const std::string& word) const
    {
        const Node* node = &root_;
        for (const char character : word) {
            const auto& child = node->children.at(static_cast<std::size_t>(character - 'a'));
            if (child == nullptr) return false;
            node = child.get();
        }
        return node->word;
    }
private:
    struct Node { std::array<std::unique_ptr<Node>, 26> children{}; bool word = false; };
    Node root_;
};

void leetcode_208_example()
{
    Trie trie;
    trie.insert("apple");
    assert(trie.search("apple"));
    assert(!trie.search("app"));
    std::cout << "[LeetCode 208] RAII nodes prevent leaks during unwinding\n";
}

// 實務：RAII transaction 預設 rollback，explicit commit 才保留。
class Transaction {
public:
    explicit Transaction(std::vector<std::string>& rows)
        : rows_(rows), original_size_(rows.size()) {}
    ~Transaction() noexcept { if (!committed_) rows_.resize(original_size_); }
    void add(std::string row) { rows_.push_back(std::move(row)); }
    void commit() noexcept { committed_ = true; }
private:
    std::vector<std::string>& rows_;
    std::size_t original_size_;
    bool committed_ = false;
};

void practical_example()
{
    std::vector<std::string> rows{"stable"};
    try {
        Transaction transaction(rows);
        transaction.add("temporary");
        throw std::runtime_error("validation failed");
    } catch (const std::runtime_error&) {}
    assert((rows == std::vector<std::string>{"stable"}));
    std::cout << "[實務] unwinding triggered transaction rollback\n";
}

int main()
{
    basic_example();
    leetcode_208_example();
    practical_example();
}

// 易錯與面試：destructor 在 stack unwinding 期間再丟例外會 terminate；cleanup 應 noexcept。
// Strong guarantee 常用「先在 temporary 完成，成功後 swap/commit」，不是 catch 後手動補洞。
// 練習：讓 Trie::insert 具有 strong guarantee，失敗不留下 prefix nodes。
// 複雜度：RAII wrapper 本身通常 O(1)，acquire/release 與 rollback 的實際成本由資源決定。
// 生命週期：guard 的 lexical scope 就是資源有效期；成功 transfer 時必須明確 move/release ownership。
