/*
 * ================================================================
 * 【第9課：vector 的內部結構與記憶體配置】總複習 summary.cpp
 * ================================================================
 * 編譯方式：g++ -std=c++17 -o summary summary.cpp
 * 本課重點：
 * 1. vector 的三個關鍵內部指標：_begin、_end、_cap
 * 2. 連續記憶體的保證與 data() 原始指標存取
 * 3. size 與 capacity 的概念與差異
 * 4. 擴容策略：MSVC 約 1.5 倍，GCC/Clang 約 2 倍
 * 5. 擴容代價：配置新空間、搬移/複製元素、釋放舊空間
 * 6. noexcept 移動建構子對擴容行為的影響
 * ================================================================
 */

#include <iostream>
#include <vector>
#include <string>

// ===== 重點一：vector 的三個內部指標 =====
// vector 內部維護三個指標，是理解其所有行為的基礎：
//
//   T* _begin;   // 指向第一個元素
//   T* _end;     // 指向最後一個元素的下一個位置
//   T* _cap;     // 指向配置空間的尾端
//
// 記憶體示意圖：
//   已配置的記憶體區塊
//   ┌───┬───┬───┬───┬───┬───┬───┬───┐
//   │ A │ B │ C │ D │ E │   │   │   │
//   └───┴───┴───┴───┴───┴───┴───┴───┘
//   ↑                   ↑           ↑
//   _begin             _end        _cap
//
//   size()     = _end - _begin = 5  （實際存放的元素數量）
//   capacity() = _cap - _begin = 8  （已配置的空間可容納的元素數量）
//
// 不變量：size() <= capacity()  永遠成立

void demo_internal_structure() {
    std::cout << "\n===== 重點一：三個內部指標 =====\n";

    std::vector<int> v = {10, 20, 30, 40, 50};
    std::cout << "size:     " << v.size()     << std::endl;  // 5
    std::cout << "capacity: " << v.capacity() << std::endl;  // >= 5
    std::cout << "empty:    " << v.empty()    << std::endl;  // 0 (false)
}

// ===== 重點二：連續記憶體的保證 =====
// C++ 標準保證 vector 元素儲存在連續的記憶體空間中。
// 可以用 v.data() 或 &v[0] 取得指向內部陣列的原始指標。
// 這使得 vector 可以直接和 C 風格 API 互動。

void demo_contiguous_memory() {
    std::cout << "\n===== 重點二：連續記憶體 =====\n";

    std::vector<int> v = {10, 20, 30, 40, 50};

    // 印出每個元素的位址（位址應連續，每個 int 相差 sizeof(int) bytes）
    std::cout << "各元素位址：\n";
    for (size_t i = 0; i < v.size(); ++i) {
        std::cout << "  v[" << i << "] 位址: " << &v[i] << std::endl;
    }

    // 透過 data() 取得原始指標存取元素
    int* ptr = v.data();  // 等同於 &v[0]
    std::cout << "ptr[2] = " << ptr[2] << std::endl;  // 30

    // 修改也會反映到 vector 中
    ptr[0] = 100;
    std::cout << "修改後 v[0] = " << v[0] << std::endl;  // 100
}

// ===== 重點三：size 與 capacity 的差異及擴容策略 =====
// 為什麼要分開 size 和 capacity？
// 若每次 push_back 都重新配置記憶體，加入 1000 個元素需要
// 0+1+2+...+999 = 499,500 次複製操作，非常低效。
//
// 解決方案：「預留空間」策略。空間不足時，一次配置更多空間：
//   - MSVC：通常約 1.5 倍成長
//   - GCC/Clang：通常約 2 倍成長
//
// 實際觀察 capacity 的變化：
//   GCC 的輸出範例：
//   size: 1, capacity: 1
//   size: 2, capacity: 2
//   size: 3, capacity: 4
//   size: 5, capacity: 8
//   size: 9, capacity: 16
//   size: 17, capacity: 32
//   ...

void demo_capacity_growth() {
    std::cout << "\n===== 重點三：capacity 擴容觀察 =====\n";

    std::vector<int> v;
    size_t prev_cap = 0;

    for (int i = 0; i < 100; ++i) {
        v.push_back(i);
        if (v.capacity() != prev_cap) {
            std::cout << "  size: " << v.size()
                      << ", capacity: " << v.capacity() << std::endl;
            prev_cap = v.capacity();
        }
    }
}

// ===== 重點四：擴容代價 — 用 Tracker 類別觀察 =====
// 當 capacity 不足時，vector 會：
//   1. 配置新的更大的記憶體區塊
//   2. 將所有舊元素移動或複製到新空間
//      （若移動建構子有 noexcept，優先用移動；否則用複製）
//   3. 銷毀舊空間中的元素
//   4. 釋放舊的記憶體區塊
//
// noexcept 的重要性：
//   若移動建構子沒有 noexcept，vector 在擴容時會改用複製（更安全但更慢）。
//   原因：若移動過程拋出例外，vector 需要能回滾操作，
//         但移動語意下原物件已被改變，無法恢復；複製則可以安全復原。

class Tracker {
public:
    int id;
    Tracker(int i) : id(i) {
        std::cout << "  建構 Tracker(" << id << ")" << std::endl;
    }
    Tracker(const Tracker& other) : id(other.id) {
        std::cout << "  複製建構 Tracker(" << id << ")" << std::endl;
    }
    Tracker(Tracker&& other) noexcept : id(other.id) {
        // noexcept 讓 vector 擴容時優先使用移動而非複製
        std::cout << "  移動建構 Tracker(" << id << ")" << std::endl;
    }
    ~Tracker() {
        std::cout << "  銷毀 Tracker(" << id << ")" << std::endl;
    }
};

void demo_reallocation_cost() {
    std::cout << "\n===== 重點四：擴容代價觀察 =====\n";

    std::vector<Tracker> v;
    // 不預留空間，讓擴容自然發生，觀察複製/移動

    std::cout << "--- 加入第 1 個（capacity 從 0 變 1）---\n";
    v.emplace_back(1);

    std::cout << "\n--- 加入第 2 個（capacity 從 1 擴容）---\n";
    v.emplace_back(2);

    std::cout << "\n--- 加入第 3 個（capacity 可能再擴容）---\n";
    v.emplace_back(3);

    std::cout << "\n--- 程式結束，所有元素被銷毀 ---\n";
}

// ===== 重點五：push_back 加入多種型別的元素 =====
// 示範 v.push_back(lvalue)  → 複製
// 示範 v.push_back(rvalue)  → 移動
// 示範 v.push_back(std::move(obj)) → 強制移動

void demo_push_back_lvalue_rvalue() {
    std::cout << "\n===== 重點五：push_back 左值 vs 右值 =====\n";

    std::vector<Tracker> v;
    v.reserve(5);  // 預留空間，避免擴容干擾觀察

    std::cout << "-- 從左值 push_back（複製）--\n";
    Tracker t1(1);
    v.push_back(t1);  // 複製，t1 仍然可用

    std::cout << "\n-- 從臨時物件 push_back（移動）--\n";
    v.push_back(Tracker(2));  // 建構臨時物件 + 移動建構 + 銷毀臨時

    std::cout << "\n-- std::move push_back（強制移動）--\n";
    Tracker t3(3);
    v.push_back(std::move(t3));  // 移動，t3 之後處於有效但未指定狀態

    std::cout << "\n-- 程式結束 --\n";
}

// ===== 重點六：摘要表 =====
// | 概念       | 說明                                          |
// |------------|-----------------------------------------------|
// | 連續記憶體 | vector 元素保證存放在連續空間，可用指標算術    |
// | size       | 目前實際存放的元素數量（_end - _begin）        |
// | capacity   | 目前配置的空間可容納的元素數量（_cap - _begin）|
// | 擴容策略   | 空間不足時，配置約 1.5x 或 2x 的新空間        |
// | 擴容代價   | 需要配置新空間、搬移所有元素、釋放舊空間       |
// | noexcept   | 移動建構子加 noexcept，擴容時優先用移動        |

int main() {
    std::cout << "====================================================\n";
    std::cout << " 第9課：vector 的內部結構與記憶體配置 — 總複習\n";
    std::cout << "====================================================\n";

    demo_internal_structure();
    demo_contiguous_memory();
    demo_capacity_growth();
    demo_reallocation_cost();
    demo_push_back_lvalue_rvalue();

    std::cout << "\n====================================================\n";
    std::cout << " 複習完畢！\n";
    std::cout << "====================================================\n";

    return 0;
}
