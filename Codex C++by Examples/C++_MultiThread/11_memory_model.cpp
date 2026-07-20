// ============================================================================
// C++ Memory Model：sequenced-before、synchronizes-with、happens-before
// ============================================================================
// 編譯器/CPU 可重排，只要單執行緒 observable behavior 不變。跨 thread 可見性不能靠
// 「原始碼先寫 data 再寫 flag」猜測，必須建立 happens-before：例如 mutex unlock->lock、
// thread completion->join、atomic release store->讀到該值的 acquire load。
//
// relaxed：只保證該 atomic 不可分割與 modification order，不能發布旁邊普通資料。
// release：此前操作不可移到發布之後；acquire：此後操作不可移到取得之前。當 acquire
// 讀到 release 所發布的值，release 前普通寫入才可安全由 acquire 後讀取。
// seq_cst 再加全域單一順序，最易推理但可能限制最佳化。先求正確，再量測是否需放寬。

#include <atomic>
#include <cassert>
#include <iostream>
#include <string>
#include <thread>

void basic_demo()
{
    std::string message;
    std::atomic<bool> ready{false};
    std::thread producer([&] {
        message = "published safely";  // 普通寫入。
        ready.store(true, std::memory_order_release);
    });
    std::thread consumer([&] {
        while (!ready.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        assert(message == "published safely");
    });
    producer.join();
    consumer.join();
}

// ----------------------------------------------------------------------------
// LeetCode 1114：Print in Order（atomic stage + acquire/release）
// ----------------------------------------------------------------------------
class OrderedSteps {
public:
    void first()
    {
        output_ += "first";  // 此刻只有 first 寫；release 發布給 second。
        stage_.store(1, std::memory_order_release);
    }

    void second()
    {
        wait_for(1);
        output_ += "second";
        stage_.store(2, std::memory_order_release);
    }

    void third()
    {
        wait_for(2);
        output_ += "third";
    }

    const std::string& result() const { return output_; }  // join 後讀。

private:
    void wait_for(int expected) const
    {
        while (stage_.load(std::memory_order_acquire) < expected) {
            std::this_thread::yield();
        }
    }

    mutable std::atomic<int> stage_{0};
    std::string output_;
};

void leetcode_demo()
{
    OrderedSteps steps;
    std::thread third([&] { steps.third(); });
    std::thread second([&] { steps.second(); });
    std::thread first([&] { steps.first(); });
    first.join();
    second.join();
    third.join();
    assert(steps.result() == "firstsecondthird");
}

// ----------------------------------------------------------------------------
// 實務：單 producer 發布 immutable snapshot pointer
// ----------------------------------------------------------------------------
struct Snapshot {
    int version;
    int value;
};

void practical_demo()
{
    Snapshot snapshot{0, 0};
    std::atomic<const Snapshot*> published{nullptr};
    std::thread writer([&] {
        snapshot = {7, 42};
        published.store(&snapshot, std::memory_order_release);
    });
    std::thread reader([&] {
        const Snapshot* observed = nullptr;
        while ((observed = published.load(std::memory_order_acquire)) == nullptr) {
            std::this_thread::yield();
        }
        assert(observed->version == 7 && observed->value == 42);
    });
    writer.join();
    reader.join();
}

int main()
{
    basic_demo();
    leetcode_demo();
    practical_demo();
    std::cout << "memory model：release/acquire 發布測試通過\n";
}

// 【陷阱】把 ready 改 relaxed 後程式在 x86「看似可用」仍不具 portable happens-before。
// 【陷阱】acquire 必須讀到對應 release 或其 release sequence 才完成同步。
// 【面試】atomicity、visibility、ordering 是三件事；memory order 解決後兩者的契約。
// 【練習】改用 atomic::wait/notify，消除 busy waiting，並保持 release/acquire。

/*
 * 【教科書補充：memory model 因果鏈】
 * - 同一 thread 內是 sequenced-before；release 被 acquire 讀到時建立 synchronizes-with，兩者合成 happens-before。
 * - acquire 並非看到「任意較早 release」就同步，必須讀到對應 release 或其 release sequence 所發布的值。
 * - seq_cst 另建立只涵蓋 seq_cst atomic 操作的單一全序，不能替 non-atomic data race 擦屁股。
 * - yield busy-wait 沒有公平或最長等待保證；C++20 atomic::wait/notify 或 condition_variable 通常更合適。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '11_memory_model.cpp' -o '/tmp/codex_cpp_C_MultiThread_11_memory_model' && '/tmp/codex_cpp_C_MultiThread_11_memory_model'
//
// === 預期輸出（節錄）===
// memory model：release/acquire 發布測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
