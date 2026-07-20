// ============================================================================
// unordered_multiset：hash-based 重複值集合
// ============================================================================
// 【基本模型】unordered_multiset 是 hash-based bag：相等 key 可重複存在，沒有排序次序。
// 【模板】可指定 Key、Hash、KeyEqual、Allocator；Hash 與 KeyEqual 必須對相等 key 保持一致。
// 【API】insert/emplace 加一筆；find/contains 找存在性；count/equal_range 處理全部等價 key。
// 【刪除 overload】erase(iterator) 只刪一筆，erase(key) 會刪除所有相等項目並回傳數量。
// 【選型】需要保留重複值但不排序時使用；只存次數時 unordered_map<Key,size_t> 通常更精簡。
// 【選型】需要有序遍歷或穩定 O(log n) 上界時改用 multiset，不要依賴 hash 遍歷順序。
// 【複雜度】insert/find/erase(iterator) 平均 O(1)；count/erase(key) 還要計入匹配筆數，最壞 O(n)。
// 【生命週期】容器擁有每個 key 節點；iterator 解參考得到的 key 不可就地修改。
// 【失效】rehash 使 iterator 失效，但不使既有元素 reference/pointer 失效；erase 只使被刪者失效。
// 【例外安全】配置、Hash 或 KeyEqual 可能拋例外；外部計數應在 insert 成功後再更新。

#include <cassert>
#include <cstddef>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

void basic_demo()
{
    std::unordered_multiset<std::string> votes{"yes", "no", "yes"};
    assert(votes.count("yes") == 2U);
    assert(votes.contains("yes"));
    const auto one_yes = votes.find("yes");
    assert(one_yes != votes.end());
    votes.erase(one_yes);  // 只刪 iterator 指向的一票。
    assert(votes.count("yes") == 1U);
    votes.insert("no");
    // erase 是必要操作，不能藏在 assert；-DNDEBUG 會把整個 assert expression 移除。
    const std::size_t erased_no_votes = votes.erase("no");
    assert(erased_no_votes == 2U);  // key overload 會刪除全部同值項目。
    assert(!votes.contains("no"));
}

// ----------------------------------------------------------------------------
// LeetCode 350：Intersection of Two Arrays II
// ----------------------------------------------------------------------------
std::vector<int> intersect(const std::vector<int>& left,
                           const std::vector<int>& right)
{
    std::unordered_multiset<int> remaining(right.begin(), right.end());
    std::vector<int> result;
    for (const int value : left) {
        const auto found = remaining.find(value);
        if (found != remaining.end()) {
            result.push_back(value);
            remaining.erase(found);
        }
    }
    return result;
}

void leetcode_demo()
{
    const auto result = intersect({1, 2, 2, 1}, {2, 2});
    assert((result == std::vector<int>{2, 2}));
    assert(intersect({}, {1, 2}).empty());
    assert((intersect({4, 4, 4}, {4, 4}) == std::vector<int>{4, 4}));
}

// ----------------------------------------------------------------------------
// 實務：每個實體序號代表一件可出貨庫存
// ----------------------------------------------------------------------------
class InventoryBag {
public:
    void receive(std::string sku) { stock_.insert(std::move(sku)); }

    bool ship_one(const std::string& sku)
    {
        const auto found = stock_.find(sku);
        if (found == stock_.end()) {
            return false;
        }
        stock_.erase(found);
        return true;
    }

    std::size_t count(const std::string& sku) const { return stock_.count(sku); }

private:
    std::unordered_multiset<std::string> stock_;
};

void practical_demo()
{
    InventoryBag stock;
    stock.receive("GPU");
    stock.receive("GPU");
    const bool shipped_first_gpu = stock.ship_one("GPU");
    assert(shipped_first_gpu);
    assert(stock.count("GPU") == 1U);
    const bool shipped_second_gpu = stock.ship_one("GPU");
    assert(shipped_second_gpu);
    assert(stock.count("GPU") == 0U);
    const bool shipped_missing_gpu = stock.ship_one("GPU");
    const bool shipped_missing_cpu = stock.ship_one("CPU");
    assert(!shipped_missing_gpu);
    assert(!shipped_missing_cpu);
}

int main()
{
    basic_demo();
    leetcode_demo();
    practical_demo();
    std::cout << "unordered_multiset：bag、重複交集與庫存測試通過\n";
}

// 【容量】reserve 預估元素數、max_load_factor 控制負載；兩者都可能觸發或影響 rehash。
// 【易錯】erase(value) 回傳刪除數量但會移除所有相等項目；出貨一件應 find 後 erase(iterator)。
// 【易錯】遍歷順序未指定，rehash 後尤其可能改變；測試應比較 multiset 語意而非顯示順序。
// 【LeetCode 契約】輸出保留 min(count_left,count_right) 次；本實作順序跟隨 left 的掃描順序。
// 【LeetCode 成本】平均時間 O(n+m)、額外空間 O(m)；惡意碰撞下最壞可能退化為平方級。
// 【實務契約】每個重複 SKU 代表一件實體庫存；ship_one 成功恰好移除一件，失敗不改庫存。
// 【實務生命週期】receive 以傳值取得 SKU，再移入容器；呼叫結束後不借用 caller 字串。
// 【面試追問】若只需 count，為何 map 可能更好？每個 key 只需一個節點，更新數字更直接。
// 【面試追問】何時 reference 還有效但 iterator 失效？rehash 後元素節點仍在，但 bucket 走訪位置重建。
// 【練習】為 InventoryBag 加 ship_many(sku,n)，不足時不得部分出貨。
