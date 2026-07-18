// ============================================================
// 第 2.9 章 總結：移動語意的效能分析 — 實測比較與最佳實踐
// 編譯：g++ -std=c++17 -O2 -o summary summary.cpp
// ============================================================
// 【效能差異的來源】
//   複製 = 配置新記憶體 + 複製全部資料 → O(n)
//   移動 = 偷取指標 + 歸零 → O(1)，與資料大小無關
//
// 【效能測試結論】
//   1. string 複製 vs 移動：字串越長，差距越大
//   2. vector 複製 vs 移動：容器越大，差距越大
//   3. push_back vs emplace_back：emplace_back 最快（原地建構）
//   4. noexcept 對 vector 擴容：有 noexcept → 用移動，沒有 → 退回複製
//   5. 基本型別（int）：移動 = 複製，零差異
//   6. SSO 短字串：差異很小（資料在棧上）
//
// 【最佳實踐】
//   ✅ 移動建構/賦值一定要加 noexcept
//   ✅ 用 emplace_back 取代 push_back（原地建構）
//   ✅ 不需要的物件用 std::move 傳入
//   ✅ 優先使用 Rule of Zero（RAII 成員）
//   ❌ 不要 move const 物件（退化為複製）
//   ❌ 不要 return std::move(local)（阻止 RVO）
//   ❌ 不要 move 基本型別（無效）
// ============================================================

#include <iostream>
#include <string>
#include <vector>
#include <utility>
#include <chrono>

class Timer {
    std::chrono::high_resolution_clock::time_point start_;
    const char* label_;
public:
    Timer(const char* label) : label_(label),
        start_(std::chrono::high_resolution_clock::now()) {}
    ~Timer() {
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now() - start_).count();
        std::cout << "  " << label_ << ": " << ms << " ms\n";
    }
};

int main() {
    // ============================================================
    // 1. string 複製 vs 移動（長字串）
    // ============================================================
    std::cout << "===== 1. string 複製 vs 移動 (10000 字元) =====\n";
    {
        const int N = 2000000;
        std::string source(10000, 'x');

        {
            Timer t("複製");
            for (int i = 0; i < N; ++i) {
                std::string copy = source;
                (void)copy;
            }
        }
        {
            Timer t("移動");
            for (int i = 0; i < N; ++i) {
                std::string temp = source;
                std::string moved = std::move(temp);
                (void)moved;
            }
        }
    }
    std::cout << "\n";

    // ============================================================
    // 2. vector 複製 vs 移動（不同大小）
    // ============================================================
    std::cout << "===== 2. vector 複製 vs 移動（不同大小）=====\n";
    {
        auto bench = [](size_t size) {
            const int N = 5000;
            std::vector<int> source(size, 42);
            long long copy_us, move_us;

            {
                auto t1 = std::chrono::high_resolution_clock::now();
                for (int i = 0; i < N; ++i) {
                    std::vector<int> copy = source;
                    (void)copy;
                }
                copy_us = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::high_resolution_clock::now() - t1).count();
            }
            {
                auto t1 = std::chrono::high_resolution_clock::now();
                for (int i = 0; i < N; ++i) {
                    std::vector<int> temp = source;
                    std::vector<int> moved = std::move(temp);
                    (void)moved;
                }
                move_us = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::high_resolution_clock::now() - t1).count();
            }
            std::cout << "  size=" << size
                      << "\t複製: " << copy_us << " us"
                      << "\t移動: " << move_us << " us";
            if (move_us > 0)
                std::cout << "\t加速: " << (double)copy_us / move_us << "x";
            std::cout << "\n";
        };

        bench(100);
        bench(1000);
        bench(10000);
        bench(100000);
    }
    std::cout << "\n";

    // ============================================================
    // 3. push_back vs emplace_back
    // ============================================================
    std::cout << "===== 3. push_back vs emplace_back =====\n";
    {
        const int N = 1000000;
        {
            Timer t("push_back(copy)");
            std::vector<std::string> v;
            v.reserve(N);
            for (int i = 0; i < N; ++i) {
                std::string s = "Hello, World!!!!";
                v.push_back(s);
            }
        }
        {
            Timer t("push_back(move)");
            std::vector<std::string> v;
            v.reserve(N);
            for (int i = 0; i < N; ++i) {
                std::string s = "Hello, World!!!!";
                v.push_back(std::move(s));
            }
        }
        {
            Timer t("emplace_back");
            std::vector<std::string> v;
            v.reserve(N);
            for (int i = 0; i < N; ++i) {
                v.emplace_back("Hello, World!!!!");
            }
        }
    }
    std::cout << "\n";

    // ============================================================
    // 4. noexcept 對 vector 擴容的影響
    // ============================================================
    std::cout << "===== 4. noexcept 對擴容的影響 =====\n";
    {
        struct WithNoexcept {
            std::vector<int> data;
            WithNoexcept() : data(1000, 42) {}
            WithNoexcept(const WithNoexcept& o) : data(o.data) {}
            WithNoexcept(WithNoexcept&& o) noexcept : data(std::move(o.data)) {}
        };
        struct WithoutNoexcept {
            std::vector<int> data;
            WithoutNoexcept() : data(1000, 42) {}
            WithoutNoexcept(const WithoutNoexcept& o) : data(o.data) {}
            WithoutNoexcept(WithoutNoexcept&& o) : data(std::move(o.data)) {} // 沒 noexcept
        };

        {
            Timer t("有 noexcept（擴容用移動）");
            std::vector<WithNoexcept> v;
            for (int i = 0; i < 50000; ++i) v.emplace_back();
        }
        {
            Timer t("沒 noexcept（擴容用複製）");
            std::vector<WithoutNoexcept> v;
            for (int i = 0; i < 50000; ++i) v.emplace_back();
        }
    }
    std::cout << "\n";

    // ============================================================
    // 5. 基本型別和 SSO 短字串：移動無效
    // ============================================================
    std::cout << "===== 5. 移動無效的情況 =====\n";
    {
        const int N = 10000000;

        {
            Timer t("int 複製");
            int source = 42;
            for (int i = 0; i < N; ++i) { int copy = source; (void)copy; }
        }
        {
            Timer t("int 移動");
            int source = 42;
            for (int i = 0; i < N; ++i) { int moved = std::move(source); (void)moved; }
        }
        std::cout << "  → 基本型別：移動 = 複製，零差異\n";
    }
    {
        const int N = 10000000;
        {
            Timer t("SSO 短 string 複製");
            for (int i = 0; i < N; ++i) {
                std::string s = "Hi"; std::string c = s; (void)c;
            }
        }
        {
            Timer t("SSO 短 string 移動");
            for (int i = 0; i < N; ++i) {
                std::string s = "Hi"; std::string m = std::move(s); (void)m;
            }
        }
        std::cout << "  → SSO 短字串：差異很小\n";
    }

    std::cout << "\n=== 最佳實踐 ===\n";
    std::cout << "  ✅ 移動建構/賦值加 noexcept\n";
    std::cout << "  ✅ 用 emplace_back > push_back(move) > push_back(copy)\n";
    std::cout << "  ✅ 不需要的物件用 std::move\n";
    std::cout << "  ✅ 優先 Rule of Zero（RAII 成員）\n";
    std::cout << "  ❌ 不要 move const 物件\n";
    std::cout << "  ❌ 不要 return move(local)\n";
    std::cout << "  ❌ 不要 move 基本型別\n";

    return 0;
}
