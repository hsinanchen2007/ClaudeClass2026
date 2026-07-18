// =============================================================================
//  07_seeding.cpp  —  正確的種子姿勢
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/numeric/random/seed_seq
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、為什麼種子重要？                                       │
//  └────────────────────────────────────────────────────────────┘
//
//  Engine 是「狀態機」 — 完全由「初始狀態 (種子)」決定整條序列。
//  種子品質決定「隨機程度」：
//   * 永遠用同一種子（如 std::mt19937{}） → 每次跑出一模一樣序列
//   * 用 time(0) → 一秒內啟動兩次會完全相同；多 thread 開始時間相近也撞
//   * 用 random_device → 拿 OS / 硬體 entropy（最佳）
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、mt19937 的「弱種子」問題                              │
//  └────────────────────────────────────────────────────────────┘
//
//  mt19937 內部狀態是 19937 bits ≈ 624 個 32-bit 字。但建構子只接受一個
//  uint32_t — 它會把這 32 bits 「擴張」成 624 字 (用一個 hash-like
//  routine)。這在大部分情況夠用，但理論上 32-bit 種子無法達到引擎完整熵。
//
//  解決：用 std::seed_seq 接受多個 uint32_t，內部做 hash + mix；mt19937
//  的建構子也接受 seed_seq：
//
//      std::random_device rd;
//      std::seed_seq sseq{rd(), rd(), rd(), rd(), rd(), rd(), rd(), rd()};
//      std::mt19937 rng{sseq};
//
//  這樣大致用 256 bits entropy 把 mt19937 fully seeded。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、可重現實驗 vs 真隨機                                  │
//  └────────────────────────────────────────────────────────────┘
//
//   * 測試 / debug：明寫 mt19937{42}、結果穩定可重現
//   * Production：用 random_device 種一次、後續用 mt19937 跑
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 四、本檔示範                                               │
//  └────────────────────────────────────────────────────────────┘
//
//   * Demo 1：明寫 seed → 重現
//   * Demo 2：random_device 一次種子
//   * Demo 3：seed_seq 多源混合種子（最佳）
// =============================================================================

/*
補充筆記：std::seeding
  - std::seeding 使用 <random>；現代 C++ 把亂數引擎和分佈分開，引擎產生位元，分佈把位元轉成需要的機率模型。
  - std::rand 品質與範圍有限，也常被錯誤取模造成偏差；新程式應優先使用 <random>。
  - seed 決定可重現性；測試常用固定 seed，正式隨機可用 random_device 或更完整的 seed sequence。
  - uniform_int_distribution 端點通常包含上下界，uniform_real_distribution 常見語意是 [a,b)；使用前要確認範圍。
  - shuffle 應搭配亂數引擎；不要使用 random_shuffle，因為它已被移除。
  - 加權抽樣、常態分佈、伯努力分佈等應使用對應 distribution，不要手寫容易有偏差的算法。
  - 固定 seed 讓測試可重現；非固定 seed 適合正式隨機或模擬多樣性。
  - random_device 不一定是真硬體亂數，部分平台可能是 deterministic；安全需求要查平台保證。
  - seed_seq 可用多個整數初始化大型引擎狀態，比單一 time 值更完整。
*/
#include <iostream>
#include <random>
#include <string>
#include <vector>

// ─────────────────────────────────────────────────────────
// 實用範例：generate_token — 產生隨機 token / nonce
//   工作中常見：給 API 產生隨機字串作為 idempotency key / request id。
//   注意：這裡的 mt19937 不是 cryptographic-secure；正式安全 token 應走
//   /dev/urandom 或 OS API。但作為「不可預測但碰撞極低的 trace id」夠用。
//   重點：用 seed_seq + 多 random_device 字 entropy 完整 seed。
// ─────────────────────────────────────────────────────────
static std::string generate_token(int len) {
    static thread_local std::mt19937 rng{[]() {
        std::random_device rd;
        std::seed_seq sseq{rd(), rd(), rd(), rd(), rd(), rd(), rd(), rd()};
        return std::mt19937{sseq}();   // 取 seed_seq 灌注後第一個輸出當示意
    }()};
    static const char* CHARSET =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    std::uniform_int_distribution<int> d{0, 61};
    std::string out;
    out.reserve(len);
    for (int i = 0; i < len; ++i) out += CHARSET[d(rng)];
    return out;
}

static void demo_practical_token() {
    std::cout << "[Practical] 產生 3 個 16-char token:\n";
    for (int i = 0; i < 3; ++i)
        std::cout << "  " << generate_token(16) << '\n';
}

int main() {
    // ─────────────────────────────────────────────────────────
    // Demo 1：明寫 seed
    // ─────────────────────────────────────────────────────────
    {
        std::mt19937 a{42};
        std::mt19937 b{42};
        std::cout << "[Demo1] same seed=42:";
        for (int i = 0; i < 3; ++i) std::cout << ' ' << a();
        std::cout << " | ";
        for (int i = 0; i < 3; ++i) std::cout << ' ' << b();
        std::cout << '\n';
    }

    // ─────────────────────────────────────────────────────────
    // Demo 2：random_device 種子（單值）
    // ─────────────────────────────────────────────────────────
    {
        std::random_device rd;
        std::mt19937 rng{rd()};
        std::cout << "[Demo2] rd seed:";
        for (int i = 0; i < 3; ++i) std::cout << ' ' << rng();
        std::cout << '\n';
    }

    // ─────────────────────────────────────────────────────────
    // Demo 3：seed_seq 多 word entropy（推薦正式用法）
    // ─────────────────────────────────────────────────────────
    {
        std::random_device rd;
        std::seed_seq sseq{rd(), rd(), rd(), rd(), rd(), rd(), rd(), rd()};
        std::mt19937 rng{sseq};
        std::cout << "[Demo3] seed_seq seed:";
        for (int i = 0; i < 3; ++i) std::cout << ' ' << rng();
        std::cout << '\n';
    }

    // ─────────────────────────────────────────────────────────
    // Demo 4：thread-local pattern
    //   每個 thread 一份 engine — 沒有互相干擾、不需鎖
    // ─────────────────────────────────────────────────────────
    {
        auto& localRng = []() -> std::mt19937& {
            // 注意：mt19937 的 seed_seq 建構子接受 Sseq&（lvalue 參考） —
            // 所以要先用一個具名變數，不能傳 std::seed_seq{...} 這種臨時值
            thread_local std::seed_seq sseq{
                std::random_device{}(), std::random_device{}(),
                std::random_device{}(), std::random_device{}()};
            thread_local std::mt19937 rng{sseq};
            return rng;
        }();
        std::cout << "[Demo4] thread_local 3 samples:";
        for (int i = 0; i < 3; ++i) std::cout << ' ' << localRng();
        std::cout << '\n';
    }

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：seed_seq 內部做了什麼？
    //    A：把所有給的數值放進一個內部 buffer，然後做一系列 mixing：
    //       初始化 entropy → mix（與後面的 word XOR + shift + multiply）
    //       → 產生 generate() 的輸出。標準演算法保證良好分散。
    //
    //  Q2：用 std::chrono::steady_clock 種子可以嗎？
    //    A：可以，但比 random_device 差 — 啟動時間相近時容易撞。建議優先
    //       random_device，clock 當 fallback。
    //
    //  Q3：seed 出去後可以「再 seed 一次」嗎？
    //    A：可以呼叫 rng.seed(...) 重置狀態。但會丟失之前已產生的歷史；
    //       測試中重新 seed 是常見手段。
    //
    demo_practical_token();
    return 0;
}
