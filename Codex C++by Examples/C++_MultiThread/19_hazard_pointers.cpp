// ============================================================================
// Hazard Pointer：lock-free 結構中延後回收「可能正被讀」的節點
// ============================================================================
// lock-free pop 的核心難題不只 CAS，而是 memory reclamation：thread A load Node* 後，
// thread B 可能移除並 delete，A 再解參考便 use-after-free。hazard pointer 讓 reader
// 先發布「我可能要讀此 pointer」，再重查 shared pointer 未變；retirer 移除節點後，
// 掃 hazard slots，無人保護才 delete，否則放 retired list 稍後重試。
//
// 本檔刻意實作「單 hazard slot、單 reader/retirer」最小模型，不能冒充完整 library。
// production 需每 thread slot 註冊、retired batching、ABA 對策與離線 proof/test。

#include <atomic>
#include <cassert>
#include <iostream>
#include <optional>
#include <thread>
#include <utility>

struct Node {
    int value;
    Node* next;
};

// -----------------------------------------------------------------------------
// 【日常實務範例】單 Reader/Retirer 的 Hazard Pointer 回收示意
// 情境：reader 可能正讀 shared head，retirer 移除節點後不能立刻 delete；本教材以一個 hazard slot 示範延後回收協定。
// 為何使用本章主題：reader 先發布候選 pointer 再重查 head，retirer 只有確認 hazard 不指向 retired node 時才 delete，避免 use-after-free。
// 設計：1. reader load/publish/revalidate。2. 安全讀值後清 hazard。3. retirer exchange head 到 retired。4. scan hazard 並在安全時 CAS 後 delete。
// 成本：正常讀取/退休為期望 O(1) atomic 操作；完整多 thread hazard system 的 scan 通常與 hazard slots 數量成正比。
// 上線注意：本類只允許單 reader/retirer，demo 更先 join reader 才 retire，未測真實競態；production 應使用成熟 library 並處理 ABA、註冊與 batch reclaim。
// -----------------------------------------------------------------------------
class SingleHazardList {
public:
    explicit SingleHazardList(int value) : head_(new Node{value, nullptr}) {}

    ~SingleHazardList()
    {
        Node* remaining = head_.exchange(nullptr);
        delete remaining;
        delete retired_.exchange(nullptr);
    }

    SingleHazardList(const SingleHazardList&) = delete;
    SingleHazardList& operator=(const SingleHazardList&) = delete;

    std::optional<int> protected_read()
    {
        Node* candidate = nullptr;
        for (;;) {
            candidate = head_.load(std::memory_order_acquire);
            if (candidate == nullptr) {
                hazard_.store(nullptr, std::memory_order_release);
                reclaim_if_safe();
                return std::nullopt;
            }
            hazard_.store(candidate, std::memory_order_seq_cst);
            if (candidate == head_.load(std::memory_order_acquire)) {
                break;
            }
        }
        const int value = candidate->value;
        hazard_.store(nullptr, std::memory_order_release);
        reclaim_if_safe();
        return value;
    }

    bool retire_head()
    {
        Node* removed = head_.exchange(nullptr, std::memory_order_acq_rel);
        if (removed == nullptr) {
            return false;
        }
        retired_.store(removed, std::memory_order_release);  // 前提：只有一個 retirer。
        reclaim_if_safe();
        return true;
    }

private:
    void reclaim_if_safe()
    {
        Node* retired = retired_.load(std::memory_order_acquire);
        if (retired != nullptr &&
            hazard_.load(std::memory_order_seq_cst) != retired) {
            if (retired_.compare_exchange_strong(retired, nullptr,
                                                 std::memory_order_acq_rel)) {
                delete retired;
            }
        }
    }

    std::atomic<Node*> head_{nullptr};
    std::atomic<Node*> hazard_{nullptr};
    std::atomic<Node*> retired_{nullptr};
};

void basic_demo()
{
    SingleHazardList list(42);
    assert(list.protected_read() == std::optional<int>{42});
    assert(list.retire_head());
    assert(!list.retire_head());
    assert(!list.protected_read().has_value());
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 206. Reverse Linked List（反轉鏈結串列）
// 題目：將單向串列原地反轉並回傳新 head；例如 1->2->3 變成 3->2->1。
// 為何使用本章主題：反轉本身是單執行緒 ownership 基礎，不需要 hazard pointer；本例用來區分 pointer 重接與併發 memory reclamation 是兩件事。
// 思路：1. previous 初值 null。2. 保存 current->next。3. 將 current->next 指回 previous。4. 前進兩個 pointer 直到 null。
// 複雜度：N 個節點的時間 O(N)、額外空間 O(1)。
// 易錯點：必須先保存 next 才能覆寫連結；若其他 thread 同時存取/刪除節點，此單執行緒演算法沒有任何同步或回收保證。
// -----------------------------------------------------------------------------
Node* reverse_list(Node* head)
{
    Node* previous = nullptr;
    while (head != nullptr) {
        Node* next = head->next;
        head->next = previous;
        previous = head;
        head = next;
    }
    return previous;
}

void leetcode_demo()
{
    Node third{3, nullptr};
    Node second{2, &third};
    Node first{1, &second};
    Node* reversed = reverse_list(&first);
    assert(reversed == &third && reversed->next == &second);
    assert(second.next == &first && first.next == nullptr);
}

void practical_demo()
{
    SingleHazardList list(7);
    int observed = 0;
    std::thread reader([&] { observed = list.protected_read().value(); });
    reader.join();  // 確定教學測試的 read 完成，再退休；protocol 本身仍處理 hazard。
    std::thread retirer([&] { assert(list.retire_head()); });
    retirer.join();
    assert(observed == 7);
}

int main()
{
    basic_demo();
    leetcode_demo();
    practical_demo();
    std::cout << "hazard pointer：保護、重查與延後回收概念測試通過\n";
}

// 【陷阱】發布 hazard 後必須重查 head；否則節點可能在 load 與 publish 間已被回收。
// 【陷阱】hazard pointer 解決 reclamation，不自動解決 ABA 或資料結構線性化正確性。
// 【面試】為何 shared_ptr 不總能直接取代 hazard pointer？control block 原子成本、API
//         layout 與 lock-free progress 需求不同，但優先使用成熟 library 而非手刻。
// 【練習】設計 retired vector，累積到門檻才掃所有 hazard slots。

/*
 * 【教科書補充：本例只展示 protocol 形狀】
 * - practical_demo 先 join reader 才 retire，沒有真正建立「已發布 hazard、並行移除」的回收競態測試。
 * - 完整測試需用 latch/barrier 固定 load/publish/validate/retire/scan 的交錯，不能靠 scheduler 運氣。
 * - 本類只允許單 reader/單 retirer；解構前必須 quiescent，且沒有證明所有 atomic 實作皆 lock-free。
 * - production 應採成熟 hazard-pointer library，並另外處理 slot 註冊、batch scan、ABA 與 memory-order proof。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '19_hazard_pointers.cpp' -o '/tmp/codex_cpp_C_MultiThread_19_hazard_pointers' && '/tmp/codex_cpp_C_MultiThread_19_hazard_pointers'
//
// === 預期輸出（節錄）===
// hazard pointer：保護、重查與延後回收概念測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
