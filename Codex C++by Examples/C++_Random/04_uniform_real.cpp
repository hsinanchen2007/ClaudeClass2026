// ============================================================================
// 課題 4：std::uniform_real_distribution
// ============================================================================
//
// uniform_real_distribution<Real>(a,b) 通常產生 [a,b)；因 floating representation 與實作
// 細節，永遠以 API contract/range 處理，不用 `value==b` 作核心邏輯。它給數值區間均勻，
// 但「半徑均勻」不等於「圓面積均勻」：圓內取點需 radius=sqrt(U)*R。
//
// 隨機 floating test 不應斷言精確 sequence/decimal；驗 range、invariant 與寬鬆統計。
// ============================================================================

#include <cassert>
#include <cmath>
#include <iostream>
#include <numbers>
#include <random>
#include <utility>

void basic_example()
{
    std::mt19937 engine(123U);
    std::uniform_real_distribution<double> unit(0.0, 1.0);
    for (int sample = 0; sample < 1'000; ++sample) {
        const double value = unit(engine);
        assert(value >= 0.0 && value < 1.0);
    }
    std::cout << "[基礎] 1000 real samples stayed in [0,1)\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 478. Generate Random Point in a Circle（在圓內產生隨機點）
// 題目：給定圓心與半徑，反覆回傳圓面積內均勻分布的點；本例驗證半徑 2、圓心 (1,-1) 的 1000 個樣本。
// 為何使用本章主題：兩個 [0,1) 均勻實數分別產生角度與面積比例；半徑用 sqrt(U)*R 才不會讓點集中在圓心。
// 思路：1. 抽 U 計算 sqrt(U)*R。2. 再抽 U 轉成 [0,2pi) 角度。3. 用 cos/sin 轉笛卡兒座標。4. 平移到圓心。
// 複雜度：每點固定兩次 draw 與常數次數學運算，時間 O(1)、額外空間 O(1)。
// 易錯點：直接用 U*R 只會讓半徑均勻而非面積均勻；production 建構器還應拒絕負半徑與非有限座標。
// -----------------------------------------------------------------------------
class CircleSampler {
public:
    CircleSampler(double radius, double center_x, double center_y, unsigned seed)
        : radius_(radius), x_(center_x), y_(center_y), engine_(seed) {}
    std::pair<double, double> randPoint()
    {
        std::uniform_real_distribution<double> unit(0.0, 1.0);
        const double radius = std::sqrt(unit(engine_)) * radius_;
        const double angle = unit(engine_) * 2.0 * std::numbers::pi;
        return {x_ + radius * std::cos(angle), y_ + radius * std::sin(angle)};
    }
private:
    double radius_;
    double x_;
    double y_;
    std::mt19937 engine_;
};

void leetcode_478_example()
{
    CircleSampler circle(2.0, 1.0, -1.0, 42U);
    for (int sample = 0; sample < 1'000; ++sample) {
        const auto [x, y] = circle.randPoint();
        const double dx = x - 1.0;
        const double dy = y + 1.0;
        assert(dx * dx + dy * dy <= 4.0 + 1e-12);
    }
    std::cout << "[LeetCode 478] all points are inside radius-2 circle\n";
}

// -----------------------------------------------------------------------------
// 【日常實務範例】分散大量客戶端的重試時間
// 情境：基準重試延遲是 10 秒，為避免服務恢復瞬間所有 client 同步重送，要在 80% 到 120% 間加入 jitter。
// 為何使用本章主題：uniform_real_distribution 直接產生連續倍率，比固定延遲或少量整數槽更能攤開重試時間。
// 設計：1. 接收基準秒數與持久 engine。2. 抽 [0.8,1.2) 倍率。3. 相乘得到本次 delay。4. 呼叫端負責等待。
// 成本：每次一次分布抽樣與乘法，期望時間 O(1)、空間 O(1)。
// 上線注意：應拒絕負值、NaN、Infinity，並與最大 backoff、總 deadline、取消機制結合；mt19937 不可用於安全用途。
// -----------------------------------------------------------------------------
double jittered_delay(double base_seconds, std::mt19937& engine)
{
    return base_seconds * std::uniform_real_distribution<double>(0.8, 1.2)(engine);
}

void practical_example()
{
    std::mt19937 engine(9U);
    for (int attempt = 0; attempt < 100; ++attempt) {
        const double delay = jittered_delay(10.0, engine);
        assert(delay >= 8.0 && delay < 12.0);
    }
    std::cout << "[實務] retry jitter stays within 80%-120%\n";
}

int main()
{
    basic_example();
    leetcode_478_example();
    practical_example();
}

// 易錯與面試：uniform_real_distribution 通常是 [a,b)，不要 assert 能抽到 b。把隨機 radius
// 直接設 U*R 不會在面積上均勻；要以 sqrt(U)*R 修正極座標 Jacobian。
// 練習：解釋為何 radius=U*R 會讓點過度集中在圓心。
// 複雜度：每個圓內 sample 需要固定數量 draws/math，通常 O(1)；trig 常數成本不可忽略。
// 生命週期：sampler 擁有 engine；回傳 point 是 value，不借用 distribution 或 engine 內部狀態。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '04_uniform_real.cpp' -o '/tmp/codex_cpp_C_Random_04_uniform_real' && '/tmp/codex_cpp_C_Random_04_uniform_real'
//
// === 預期輸出（節錄）===
// [實務] retry jitter stays within 80%-120%
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
