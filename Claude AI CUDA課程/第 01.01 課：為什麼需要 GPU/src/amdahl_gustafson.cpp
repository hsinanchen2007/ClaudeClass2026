// ============================================================================
//  課 1.1 — 為什麼需要 GPU
//  amdahl_gustafson.cpp — 兩個平行運算定律的數值對照
//
//  這支程式【不需要 GPU】，純 CPU 計算，只是把兩個公式的行為印出來。
//  目的是搞懂一件事：
//      「為什麼 GPU 有幾萬條執行緒還有意義？Amdahl 定律不是說會卡住嗎？」
//
//  Amdahl（1967）：問題規模【固定】，加更多處理器
//      S(N) = 1 / ( (1-p) + p/N )              N→∞ 時上限 = 1/(1-p)
//
//  Gustafson（1988）：時間預算【固定】，問題規模【隨處理器變大】
//      S(N) = (1-p) + p·N                      隨 N 線性成長，無上限
//
//  p = 可平行化的比例（0.0 ~ 1.0）
//
//  編譯：g++ -std=c++20 -O2 amdahl_gustafson.cpp -o amdahl_gustafson
// ============================================================================

#include <cstdio>
#include <vector>

// Amdahl：固定問題規模下，用 N 個處理器能加速幾倍
static double amdahl(double p, double N)
{
    return 1.0 / ((1.0 - p) + p / N);
}

// Gustafson：固定時間預算下，用 N 個處理器能多做幾倍的事
static double gustafson(double p, double N)
{
    return (1.0 - p) + p * N;
}

int main()
{
    // 你這台筆電 GPU（Quadro RTX 4000, sm_75）可常駐 40960 條執行緒，
    // 所以 N 一路排到 40960 才有實感。
    const std::vector<double> Ns = {8, 64, 512, 2560, 40960};
    const std::vector<double> ps = {0.50, 0.90, 0.99, 0.999};

    std::printf("═══ Amdahl 定律：問題規模固定 ═══\n");
    std::printf("（把同一份工作丟給更多處理器，最多能快幾倍）\n\n");
    std::printf("  可平行比例 p |");
    for (double N : Ns) std::printf(" N=%-8.0f", N);
    std::printf(" |  理論上限 1/(1-p)\n");
    std::printf("  -------------|");
    for (size_t i = 0; i < Ns.size(); ++i) std::printf("-----------");
    std::printf("-|------------------\n");

    for (double p : ps) {
        std::printf("  p = %-8.3f |", p);
        for (double N : Ns) std::printf(" %8.1f× ", amdahl(p, N));
        std::printf(" |  %10.1f×\n", 1.0 / (1.0 - p));
    }

    std::printf("\n  ⚠️ 關鍵觀察：\n");
    std::printf("     p=0.90（九成可平行）時，就算給你 40960 條執行緒，\n");
    std::printf("     也只能快 %.1f 倍 —— 上限是 10 倍，多給的執行緒全浪費。\n",
                amdahl(0.90, 40960));
    std::printf("     序列的那 10%%，最後佔掉了幾乎全部的執行時間。\n");

    std::printf("\n═══ Gustafson 定律：時間預算固定 ═══\n");
    std::printf("（給更多處理器，就把問題做得更大 —— 這才是 GPU 的實際用法）\n\n");
    std::printf("  可平行比例 p |");
    for (double N : Ns) std::printf(" N=%-8.0f", N);
    std::printf("\n  -------------|");
    for (size_t i = 0; i < Ns.size(); ++i) std::printf("-----------");
    std::printf("\n");

    for (double p : ps) {
        std::printf("  p = %-8.3f |", p);
        for (double N : Ns) std::printf(" %8.1f× ", gustafson(p, N));
        std::printf("\n");
    }

    std::printf("\n  ✅ 關鍵觀察：\n");
    std::printf("     同樣 p=0.90、N=40960，Gustafson 給出 %.0f 倍。\n",
                gustafson(0.90, 40960));
    std::printf("     差別不在數學，而在【問題有沒有跟著變大】：\n");
    std::printf("       Amdahl   問「同一張 512×512 影像，能算多快？」  → 很快撞牆\n");
    std::printf("       Gustafson問「同樣時間內，能算多大的模型？」    → 持續受益\n");

    std::printf("\n═══ 這對 AI 意味著什麼 ═══\n");
    std::printf("  深度學習正好是 Gustafson 的教科書案例：\n");
    std::printf("    · 硬體變強 → 大家不是把同一個模型訓練得更快，而是【把模型做更大】\n");
    std::printf("    · batch size、序列長度、參數量都隨算力一起長\n");
    std::printf("    · 矩陣乘法的 p 極高（>0.99），且工作量隨規模成長\n");
    std::printf("  → 所以 GPU 的「幾萬條執行緒」不是虛胖，是真的用得掉。\n");
    std::printf("\n  但別誤會：Amdahl 沒有失效。它在推論（inference）時會回來咬人——\n");
    std::printf("  單一 request 的延遲（latency）就是固定規模問題，\n");
    std::printf("  序列的部分（例如 LLM 一次只能吐一個 token）會變成硬牆。\n");
    std::printf("  這正是 Part 9 要處理的核心矛盾：throughput 與 latency 的拉鋸。\n");

    return 0;
}
