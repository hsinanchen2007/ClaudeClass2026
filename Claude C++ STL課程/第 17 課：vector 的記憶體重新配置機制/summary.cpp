/*
 * ================================================================
 * 【第17課：vector 的記憶體重新配置機制】總複習 summary.cpp
 * ================================================================
 * 編譯方式：g++ -std=c++17 -o summary summary.cpp
 * 本課重點：
 * 1. 擴容策略：size 滿時配置更大的新記憶體，複製/移動元素，釋放舊記憶體
 * 2. 成長倍率：通常是 1.5 倍（MSVC）或 2 倍（GCC/Clang），均攤 O(1) 的保證
 * 3. push_back 觸發重新配置時的物件搬移：移動建構子（若有 noexcept）優先
 * 4. 迭代器失效：重新配置後所有迭代器/指標/引用全部失效
 * 5. insert/erase 也可能造成迭代器失效（元素搬移即使不重新配置）
 * 6. 在迴圈中安全刪除元素的正確寫法（利用 erase 的回傳值）
 * 7. erase-remove 慣用法（std::remove_if + erase）
 * 8. reserve() 避免不必要的重新配置
 * 9. shrink_to_fit() 釋放多餘的容量
 * 10. 使用 chrono 量測 reserve 對效能的影響
 * ================================================================
 */

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>  // std::remove_if
#include <chrono>

// ===== 重點一：擴容策略（Reallocation Strategy）=====
// 當 vector 的 size == capacity 時，再呼叫 push_back 就會觸發「重新配置」：
//   1. 配置一塊「更大」的新記憶體（通常 1.5 或 2 倍）
//   2. 把所有現有元素從舊記憶體複製（或移動）到新記憶體
//   3. 銷毀舊記憶體中的元素
//   4. 釋放舊記憶體
//   5. 在新記憶體尾端建構新元素
//
// 這個過程是「透明的」——使用者感覺 push_back 就是在尾端加一個元素。

// ===== 重點二：均攤 O(1) 的數學原理 =====
// 倍率成長策略保證 n 次 push_back 的總拷貝次數是 O(n)，
// 因此「每次 push_back 的均攤成本」是 O(1)。
//
// 以倍率 2 為例（0→1→2→4→8→16→...）：
//   加入第 1 個：拷貝 0 次
//   加入第 2 個：拷貝 1 次（1→2，搬 1 個舊元素）
//   加入第 3 個：拷貝 2 次（2→4，搬 2 個舊元素）
//   加入第 5 個：拷貝 4 次（4→8，搬 4 個舊元素）
//   ...
//   加入 n 個元素，總拷貝次數 ≤ 2n，均攤每次 O(1)

// ===== 重點三：noexcept 移動建構子的重要性 =====
// 重新配置時，vector 需要把舊元素搬到新位址。
//   - 若元素型別有「標記 noexcept 的移動建構子」→ 使用移動（高效）
//   - 若沒有 noexcept 的移動建構子                → 使用拷貝（保守，以防例外安全）
// 結論：自訂類別的移動建構子務必標記 noexcept。

// ===== 重點四：迭代器失效的根本原因 =====
// 重新配置後，舊記憶體被釋放，指向舊記憶體的迭代器就變成「懸空迭代器」。
// 繼續使用懸空迭代器 = 未定義行為（UB），可能崩潰或出現奇怪數值。
//
// insert 和 erase 也會使「插入/刪除點之後」的迭代器失效，
// 因為後方元素被搬移了，原本的地址存的是不同的值。

// ===== 重點五：在迴圈中安全刪除元素 =====
// 錯誤寫法：
//   for (auto it = v.begin(); it != v.end(); ++it) {
//       if (條件) v.erase(it);  // erase 後 it 失效，++it 是 UB！
//   }
//
// 正確寫法：利用 erase() 的回傳值（下一個有效迭代器）
//   for (auto it = v.begin(); it != v.end(); /* 不在此處 ++it */) {
//       if (條件) it = v.erase(it);  // 取回有效的下一個迭代器
//       else      ++it;
//   }

// ===== 重點六：erase-remove 慣用法（推薦）=====
// std::remove_if 把不滿足條件的元素「移到前面」，回傳邏輯新尾端。
// v.erase(new_end, v.end()) 才真正刪除尾部的無用元素。
// 這個「先 remove_if 再 erase」的模式叫做「erase-remove idiom」。

// ===== 重點七：reserve() 與 shrink_to_fit() =====
// reserve(n)：至少保留 n 個元素的容量，避免後續重新配置
//   - 只影響 capacity，不影響 size
//   - 若 n <= 目前 capacity，無任何效果
//   - 若 n > 目前 capacity，觸發一次重新配置（現有迭代器失效！）
//
// shrink_to_fit()：請求把 capacity 縮減到 size 大小
//   - 是「非綁定性請求」（hint），編譯器/STL 實作可忽略，但通常會執行
//   - 使用時機：大量刪除元素後，想釋放多餘記憶體

// 用於觀察物件搬移行為的追蹤類別
class Tracker {
    std::string name_;
public:
    explicit Tracker(const std::string& name) : name_(name) {
        std::cout << "  建構: " << name_ << std::endl;
    }
    Tracker(const Tracker& other) : name_(other.name_) {
        std::cout << "  拷貝: " << name_ << std::endl;
    }
    // noexcept 讓 vector 在重新配置時優先使用移動而非拷貝
    Tracker(Tracker&& other) noexcept : name_(std::move(other.name_)) {
        std::cout << "  移動: " << name_ << std::endl;
    }
    ~Tracker() {
        if (!name_.empty())
            std::cout << "  銷毀: " << name_ << std::endl;
    }
    const std::string& name() const { return name_; }
};

void demo_reallocation_observation() {
    std::cout << "\n--- 一、觀察重新配置的時機與位址變化 ---" << std::endl;
    std::vector<int> v;

    std::cout << "初始：size=" << v.size()
              << ", capacity=" << v.capacity() << std::endl;

    for (int i = 1; i <= 10; ++i) {
        size_t old_cap = v.capacity();
        const int* old_data = v.data();

        v.push_back(i);

        if (v.capacity() != old_cap) {
            std::cout << "  *** 重新配置！舊容量=" << old_cap
                      << " → 新容量=" << v.capacity()
                      << "（位址從 " << old_data << " 變為 " << v.data() << "）" << std::endl;
        }
    }
}

void demo_growth_ratio() {
    std::cout << "\n--- 二、成長倍率觀察（push_back 100 次）---" << std::endl;
    std::vector<int> v;
    size_t prev_cap = 0;

    for (int i = 0; i < 100; ++i) {
        v.push_back(i);
        if (v.capacity() != prev_cap) {
            if (prev_cap > 0) {
                double ratio = static_cast<double>(v.capacity()) / prev_cap;
                std::cout << "  " << prev_cap << " → " << v.capacity()
                          << "（倍率約 " << ratio << "）" << std::endl;
            }
            prev_cap = v.capacity();
        }
    }
    // 通常看到的倍率：GCC/Clang 約 2 倍，MSVC 約 1.5 倍
}

void demo_move_vs_copy_on_realloc() {
    std::cout << "\n--- 三、重新配置時：移動 vs 拷貝（需要 noexcept）---" << std::endl;
    std::vector<Tracker> v;
    v.reserve(2);  // 故意只留 2 個位置

    std::cout << "push_back A：" << std::endl;
    v.push_back(Tracker("A"));

    std::cout << "push_back B：" << std::endl;
    v.push_back(Tracker("B"));

    std::cout << "push_back C（觸發重新配置）：" << std::endl;
    v.push_back(Tracker("C"));
    // 由於 Tracker 的移動建構子有 noexcept，vector 會使用移動而非拷貝
}

void demo_iterator_invalidation_by_push_back() {
    std::cout << "\n--- 四、push_back 觸發重新配置使迭代器失效 ---" << std::endl;
    std::vector<int> v = {10, 20, 30};
    v.reserve(3);  // 確保容量剛好是 3，下一次 push_back 必定重新配置

    auto it = v.begin() + 1;  // 指向 20
    std::cout << "push_back 前 *it = " << *it << std::endl;  // 20

    v.push_back(40);  // 觸發重新配置，it 失效

    // 絕對不可以：std::cout << *it;  // 未定義行為！

    // 安全做法：重新取得迭代器
    it = v.begin() + 1;
    std::cout << "重新取得後 *it = " << *it << std::endl;  // 20
}

void demo_iterator_invalidation_by_insert_erase() {
    std::cout << "\n--- 五、insert/erase 也會使插入/刪除點之後的迭代器失效 ---" << std::endl;
    std::vector<int> v = {10, 20, 30, 40, 50};
    v.reserve(100);  // 預留足夠容量，排除重新配置的干擾

    auto it_10 = v.begin();      // 指向 10（插入點之前）
    auto it_30 = v.begin() + 2;  // 指向 30（插入點之後）

    v.insert(v.begin() + 1, 15);  // 在索引 1 插入 15 → {10, 15, 20, 30, 40, 50}

    // 插入點之前的迭代器仍然有效
    std::cout << "*it_10（插入前）= " << *it_10 << "（仍有效，值=" << *it_10 << "）" << std::endl;

    // 插入點之後的迭代器已失效（後方元素被往後移）
    // std::cout << *it_30;  // 危險！it_30 指向的位置內容已改變

    std::cout << "目前 v = ";
    for (int x : v) std::cout << x << " ";
    std::cout << std::endl;
}

void demo_safe_erase_in_loop() {
    std::cout << "\n--- 六、迴圈中安全刪除元素（正確 vs 錯誤）---" << std::endl;
    std::vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8};

    std::cout << "刪除前：";
    for (int x : v) std::cout << x << " ";
    std::cout << std::endl;

    // 正確做法：利用 erase() 回傳值取得下一個有效迭代器
    for (auto it = v.begin(); it != v.end(); /* 不在此 ++it */) {
        if (*it % 2 == 0) {
            it = v.erase(it);  // erase 回傳指向下一個元素的迭代器
        } else {
            ++it;              // 只在不刪除時才手動前進
        }
    }

    std::cout << "刪除偶數後：";
    for (int x : v) std::cout << x << " ";
    std::cout << std::endl;
}

void demo_erase_remove_idiom() {
    std::cout << "\n--- 七、erase-remove 慣用法（推薦方式）---" << std::endl;
    std::vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8};

    std::cout << "刪除前：";
    for (int x : v) std::cout << x << " ";
    std::cout << std::endl;

    // std::remove_if：把不滿足條件的元素移到前面，回傳邏輯新尾端
    // v.erase(new_end, v.end())：真正刪除「尾部的無用元素」
    auto new_end = std::remove_if(v.begin(), v.end(),
                                  [](int x) { return x % 2 == 0; });
    v.erase(new_end, v.end());

    std::cout << "刪除偶數後（erase-remove）：";
    for (int x : v) std::cout << x << " ";
    std::cout << std::endl;
}

void demo_reserve_performance() {
    std::cout << "\n--- 八、reserve 對效能的影響 ---" << std::endl;

    {
        std::vector<int> v;
        int realloc_count = 0;
        size_t prev_cap = 0;
        for (int i = 0; i < 10000; ++i) {
            v.push_back(i);
            if (v.capacity() != prev_cap) {
                ++realloc_count;
                prev_cap = v.capacity();
            }
        }
        std::cout << "不用 reserve：重新配置 " << realloc_count
                  << " 次，最終容量 " << v.capacity() << std::endl;
    }

    {
        std::vector<int> v;
        v.reserve(10000);  // 預先配置，避免後續重新配置
        int realloc_count = 0;
        size_t prev_cap = v.capacity();
        for (int i = 0; i < 10000; ++i) {
            v.push_back(i);
            if (v.capacity() != prev_cap) {
                ++realloc_count;
                prev_cap = v.capacity();
            }
        }
        std::cout << "使用 reserve：重新配置 " << realloc_count
                  << " 次，最終容量 " << v.capacity() << std::endl;
    }
}

void demo_shrink_to_fit() {
    std::cout << "\n--- 九、shrink_to_fit：釋放多餘容量 ---" << std::endl;
    std::vector<int> v;

    for (int i = 0; i < 10000; ++i) v.push_back(i);
    std::cout << "插入 10000 個後：size=" << v.size()
              << ", capacity=" << v.capacity() << std::endl;

    // 刪除大部分，只保留前 10 個
    v.erase(v.begin() + 10, v.end());
    std::cout << "刪除後（未 shrink）：size=" << v.size()
              << ", capacity=" << v.capacity() << std::endl;

    // 要求把多餘容量還給系統
    v.shrink_to_fit();
    std::cout << "shrink_to_fit 後：size=" << v.size()
              << ", capacity=" << v.capacity() << std::endl;
}

void demo_timing_reserve() {
    std::cout << "\n--- 十、計時對比：使用/不使用 reserve ---" << std::endl;
    const int N = 5'000'000;

    auto time_it = [&](const std::string& label, auto fn) {
        auto start = std::chrono::high_resolution_clock::now();
        fn();
        auto end = std::chrono::high_resolution_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << label << ": " << ms << " ms" << std::endl;
    };

    time_it("不用 reserve", [&]() {
        std::vector<int> v;
        for (int i = 0; i < N; ++i) v.push_back(i);
    });

    time_it("使用 reserve", [&]() {
        std::vector<int> v;
        v.reserve(N);
        for (int i = 0; i < N; ++i) v.push_back(i);
    });
}

int main() {
    std::cout << "================================================================" << std::endl;
    std::cout << "   第 17 課：vector 的記憶體重新配置機制 總複習" << std::endl;
    std::cout << "================================================================" << std::endl;

    demo_reallocation_observation();
    demo_growth_ratio();
    demo_move_vs_copy_on_realloc();
    demo_iterator_invalidation_by_push_back();
    demo_iterator_invalidation_by_insert_erase();
    demo_safe_erase_in_loop();
    demo_erase_remove_idiom();
    demo_reserve_performance();
    demo_shrink_to_fit();
    demo_timing_reserve();

    std::cout << "\n================================================================" << std::endl;
    std::cout << "課程重點回顧：" << std::endl;
    std::cout << "  1. size == capacity 時 push_back 觸發重新配置" << std::endl;
    std::cout << "  2. 成長倍率（約 1.5~2 倍）保證均攤 O(1)" << std::endl;
    std::cout << "  3. 移動建構子加 noexcept → 重新配置時使用移動，效率更高" << std::endl;
    std::cout << "  4. 重新配置後所有迭代器/指標/引用全部失效" << std::endl;
    std::cout << "  5. insert/erase 使插入/刪除點之後的迭代器失效" << std::endl;
    std::cout << "  6. 迴圈中安全刪除：用 erase() 的回傳值取回有效迭代器" << std::endl;
    std::cout << "  7. erase-remove 慣用法：O(n) 批量刪除，推薦使用" << std::endl;
    std::cout << "  8. reserve() 預先配置，避免重新配置" << std::endl;
    std::cout << "  9. shrink_to_fit() 釋放多餘容量" << std::endl;
    std::cout << "================================================================" << std::endl;

    return 0;
}
