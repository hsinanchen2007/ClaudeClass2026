// ============================================================================
// 課題 8：Copy constructor（複製建構子）與 deep copy
// ============================================================================
//
// `T copy(original)`、按值傳參、按值回傳（未被 elision）都可能使用 copy constructor：
// `T(const T&)`。compiler 的預設行為是逐 member copy；對 vector/string 很安全，對
// raw owning pointer 只會複製位址，兩物件會 delete 同一資源，因此必須 deep copy。
//
// copy constructor 建立「新物件」；copy assignment（下一課）修改「既有物件」。
// copy elision/RVO 可能讓觀察不到 copy 呼叫，這不是語意失效，而是標準允許或要求
// 直接在目的地建構。
//
// 【面試】參數為何是 `const T&`？若按值傳入，為了建立參數又要先 copy，無限遞迴。
// 【設計】能用 vector/unique_ptr 的 class 優先 Rule of Zero，不必自己維護 new/delete。
// ============================================================================

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <unordered_map>
#include <utility>
#include <vector>

class IntArray {
public:
    explicit IntArray(std::size_t size) : size_(size), data_(new int[size]{}) {}

    IntArray(const IntArray& other)
        : size_(other.size_), data_(new int[other.size_])
    {
        std::copy(other.data_, other.data_ + other.size_, data_);
    }

    ~IntArray() noexcept { delete[] data_; }
    IntArray& operator=(const IntArray&) = delete; // 下一課補齊。

    int& operator[](std::size_t index) { return data_[index]; }
    int operator[](std::size_t index) const { return data_[index]; }

private:
    std::size_t size_;
    int* data_;
};

void basic_example()
{
    IntArray first(2);
    first[0] = 7;
    IntArray second(first); // deep copy：second.data_ 指向不同 allocation。
    second[0] = 99;
    assert(first[0] == 7 && second[0] == 99);
    std::cout << "[基礎] deep copy 後修改副本不影響原物件\n";
}

// LeetCode 138：Copy List with Random Pointer。
// RandomList 的 copy constructor 實作完整 deep copy：next 與 random 都指向新 nodes。
class RandomList {
public:
    struct Node {
        int value;
        Node* next = nullptr;
        Node* random = nullptr;
    };

    RandomList() = default;

    RandomList(const RandomList& other)
    {
        try {
            std::unordered_map<const Node*, Node*> mapping;
            Node* tail = nullptr;
            for (const Node* source = other.head_; source != nullptr; source = source->next) {
                Node* clone = new Node{source->value};
                if (head_ == nullptr) {
                    head_ = clone;
                } else {
                    tail->next = clone;
                }
                tail = clone;
                mapping.emplace(source, clone);
            }
            const Node* source = other.head_;
            Node* clone = head_;
            while (source != nullptr) {
                clone->random = source->random == nullptr ? nullptr : mapping.at(source->random);
                source = source->next;
                clone = clone->next;
            }
        } catch (...) {
            // constructor 丟例外時 ~RandomList 不會被呼叫，必須自行清掉已配置節點。
            clear();
            throw;
        }
    }

    ~RandomList() noexcept { clear(); }
    RandomList& operator=(const RandomList&) = delete;

    Node* append(int value)
    {
        Node* node = new Node{value};
        if (head_ == nullptr) {
            head_ = node;
            return node;
        }
        Node* tail = head_;
        while (tail->next != nullptr) {
            tail = tail->next;
        }
        tail->next = node;
        return node;
    }
    const Node* head() const { return head_; }

private:
    void clear() noexcept
    {
        while (head_ != nullptr) {
            Node* next = head_->next;
            delete head_;
            head_ = next;
        }
    }
    Node* head_ = nullptr;
};

void leetcode_138_example()
{
    RandomList original;
    RandomList::Node* first = original.append(7);
    RandomList::Node* second = original.append(13);
    first->random = second;
    second->random = first;

    RandomList clone(original);
    assert(clone.head() != original.head());
    assert(clone.head()->value == 7);
    assert(clone.head()->random == clone.head()->next);
    assert(clone.head()->random != second);
    std::cout << "[LeetCode 138] next/random 全部指向複製後節點\n";
}

// 實務案例：設定 snapshot 按值複製，後續更新 live config 不改歷史版本。
struct ConfigSnapshot {
    std::vector<int> worker_limits;
};

void practical_example()
{
    ConfigSnapshot live{{4, 8}};
    const ConfigSnapshot audit_copy(live);
    live.worker_limits[0] = 16;
    assert(audit_copy.worker_limits[0] == 4);
    std::cout << "[實務] audit snapshot 保留舊設定 4\n";
}

int main()
{
    basic_example();
    leetcode_138_example();
    practical_example();
}

// 練習：為 RandomList 加 copy assignment；注意 exception safety 與 self-assignment。
// 複雜度：deep copy 是 O(owned nodes/elements)，額外空間同樣線性；shallow pointer copy 才 O(1)。
// 生命週期與易錯：副本必須擁有獨立資源；兩個 owner 指同一 raw allocation 會 double delete。
