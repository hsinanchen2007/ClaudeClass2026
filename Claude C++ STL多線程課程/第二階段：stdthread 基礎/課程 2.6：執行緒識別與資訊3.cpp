// =============================================================================
//  課程 2.6：執行緒識別與資訊3.cpp  —  std::thread::hardware_concurrency()
// =============================================================================
//
// 【主題資訊 Information】
//   static unsigned int std::thread::hardware_concurrency() noexcept;   // C++11
//   標頭檔：<thread>
//   回傳值：實作對「可真正並行執行的執行緒數」的估計值；
//           【允許回傳 0】，代表無法取得或未定義。
//   複雜度：標準未規定；實務上是一次查詢（libstdc++ 走 sched_getaffinity）。
//
// 【詳細解釋 Explanation】
//
// 【1. 它回傳的不是「CPU 核心數」，而是一個「提示」】
//   標準的措辭是 hint（提示），不是保證。它可能等於：
//     * 實體核心數（physical cores）
//     * 邏輯核心數（logical processors，含 Hyper-Threading / SMT）
//     * 容器 / cgroup CPU affinity 限制下的可用 CPU 數
//     * 0（無法判定）
//   libstdc++ 在 Linux 上實作為 sched_getaffinity() 的 CPU set 大小，
//   所以它反映的是【本行程實際被允許跑在幾顆 CPU 上】，
//   而不是主機板上插了幾顆 CPU。
//   本機實測值：16（與 nproc 一致）。這是【實作定義】的結果，換機器必然不同。
//
// 【2. 為什麼「允許回傳 0」這件事很重要】
//   標準明文允許回傳 0。任何直接拿它去做除法或當迴圈上界的程式碼，
//   在回傳 0 的平台上會退化成「一個執行緒都不建立」，或直接除以零。
//   所以慣用寫法永遠是：
//       unsigned int n = std::thread::hardware_concurrency();
//       if (n == 0) n = 2;          // 給一個保守的預設值
//   這不是防禦性過度，而是標準明確要求你處理的路徑。
//
// 【3. 為什麼開「剛好 hardware_concurrency 個」執行緒不一定最快】
//   這個數字只適合【CPU-bound】工作負載：每條執行緒都在燒 CPU，
//   開超過核心數只會增加 context switch 成本。
//   但如果工作是【I/O-bound】（讀檔、等網路、等資料庫），
//   執行緒大部分時間阻塞在等待上，開 2~10 倍於核心數反而吞吐量更高。
//   決定執行緒數的是「工作型態」，不是這個函式。
//
// 【概念補充 Concept Deep Dive】
//   * 容器環境的經典陷阱：在 Docker 裡用 --cpus=2 限制 CPU 配額時，
//     cgroup 的 cpu.max 限制的是 CPU 時間配額，【不會】改變 affinity mask。
//     此時 hardware_concurrency() 可能仍回傳宿主機的核心數，
//     程式開了滿滿的執行緒卻只有 2 顆 CPU 的配額 → 大量爭搶與節流。
//     要真正縮小可見核心數必須用 --cpuset-cpus，那才會影響 affinity。
//   * 它是 static 成員函式：不需要任何 std::thread 物件就能呼叫，
//     寫成 std::thread::hardware_concurrency() 而不是 t.hardware_concurrency()。
//   * noexcept：查詢失敗以「回傳 0」表示，不會丟例外。
//
// 【注意事項 Pay Attention】
//   1. 回傳型別是 unsigned int。與 int 混用比較時注意 signed/unsigned 警告。
//   2. 【不可】假設回傳值 > 0，標準允許 0。
//   3. 【不可】把它當成「實體核心數」——含 SMT 時通常是邏輯核心數。
//   4. 這個數值是實作定義的：本機 16，在別台機器上幾乎一定是別的數字。
//   5. 它不反映當下系統負載；別的行程正在吃滿 CPU，它仍回傳同樣的值。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::thread::hardware_concurrency()
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. hardware_concurrency() 保證回傳 CPU 核心數嗎？
//     答：不保證。標準只說它是「可並行執行緒數的提示」，而且【明文允許回傳 0】。
//         在 Linux/libstdc++ 上它反映的是本行程的 CPU affinity 集合大小，
//         含 Hyper-Threading 時通常是邏輯核心數而非實體核心數。
//     追問：那回傳 0 時你的程式怎麼辦？→ 一律 fallback 到保守預設值
//           （常見是 2 或 4），絕不能拿 0 去做除數或迴圈上界。
//
// 🔥 Q2. 執行緒池的大小應該直接設成 hardware_concurrency() 嗎？
//     答：只有 CPU-bound 工作適合。I/O-bound 工作因為執行緒大多在阻塞，
//         設成核心數會讓 CPU 閒置，通常要開更多。
//         正規做法是量測後決定，或用「核心數 / (1 - 阻塞比例)」之類的模型估算。
//     追問：混合型工作怎麼辦？→ 拆成兩個池（CPU 池 + I/O 池），
//           不要用同一個池同時扛計算與阻塞。
//
// ⚠️ 陷阱. 程式在 Docker 裡跑，下了 --cpus=2，為什麼 hardware_concurrency()
//        還是回傳宿主機的核心數？
//     答：--cpus 設定的是 cgroup 的 CPU 頻寬配額（cpu.max），
//         它限制的是「每個週期能用多少 CPU 時間」，不是「能跑在哪幾顆 CPU 上」。
//         libstdc++ 查的是 sched_getaffinity()，affinity mask 沒被改動，
//         所以仍回傳原本的邏輯核心數。
//     為什麼會錯：多數人腦中把「限制 CPU」和「限制可見核心數」畫上等號，
//         但 cgroup 的配額限制與 CPU affinity 是兩套獨立機制。
//         要縮小可見核心數要用 --cpuset-cpus。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】—— 本檔【略過】，理由如下：
//   hardware_concurrency() 是「查詢執行環境」的 API。
//   LeetCode 的並行題（1114 / 1115 / 1116 / 1117 / 1195）考的是執行緒間的
//   【順序同步】，評測環境也不允許依機器核心數改變行為。
//   硬套一題只會誤導讀者，故此處從缺。

#include <iostream>
#include <thread>

// -----------------------------------------------------------------------------
// 【日常實務範例】依工作型態決定 worker 數量（影片轉檔服務的排程器）
//   情境：一個轉檔服務同時有兩類工作——
//         (a) 影像編碼：純 CPU 運算，開超過核心數只會互搶；
//         (b) 上傳 / 下載素材：大部分時間阻塞在網路 I/O。
//   這裡示範正確的三步驟：查詢 → 處理 0 → 依工作型態調整。
// -----------------------------------------------------------------------------
enum class WorkloadKind { CpuBound, IoBound };

unsigned int decideWorkerCount(WorkloadKind kind, unsigned int hw) {
    // 步驟 1：標準允許回傳 0，必須先兜底
    if (hw == 0) {
        hw = 2;  // 無法偵測時的保守預設值
    }

    // 步驟 2：依工作型態調整
    switch (kind) {
        case WorkloadKind::CpuBound:
            // CPU-bound：留一條給主執行緒 / OS，避免整台機器沒有餘裕
            return (hw > 1) ? (hw - 1) : 1;
        case WorkloadKind::IoBound:
            // I/O-bound：執行緒多半在等待，超額訂閱可提高吞吐；設上限避免爆量
            return (hw * 4 < 64) ? (hw * 4) : 64;
    }
    return hw;
}

int main() {
    std::cout << "=== 原始查詢 ===" << std::endl;

    unsigned int cores = std::thread::hardware_concurrency();
    std::cout << "硬體支援的並行執行緒數: " << cores << std::endl;

    // 標準允許回傳 0，代表「無法偵測」——必須處理這條路徑
    if (cores == 0) {
        std::cout << "無法偵測，使用預設值" << std::endl;
        cores = 2;
    }
    std::cout << "實際採用的核心數估計: " << cores << std::endl;

    std::cout << "\n=== 日常實務: 依工作型態決定 worker 數 ===" << std::endl;
    std::cout << "CPU-bound（影像編碼）建議 worker 數: "
              << decideWorkerCount(WorkloadKind::CpuBound, cores) << std::endl;
    std::cout << "I/O-bound（素材上傳下載）建議 worker 數: "
              << decideWorkerCount(WorkloadKind::IoBound, cores) << std::endl;

    std::cout << "\n=== 邊界檢查: 平台回傳 0 時 ===" << std::endl;
    std::cout << "CPU-bound  fallback: "
              << decideWorkerCount(WorkloadKind::CpuBound, 0) << std::endl;
    std::cout << "I/O-bound  fallback: "
              << decideWorkerCount(WorkloadKind::IoBound, 0) << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread '課程 2.6：執行緒識別與資訊3.cpp' -o hw_concurrency

// -----------------------------------------------------------------------------
// 【輸出但書】以下數字是【實作定義】的，取決於執行機器：
//   本機（Ubuntu 26.04 / g++ 15.2）實測 hardware_concurrency() = 16，
//   因此衍生的 15 與 64 也隨之而來。換一台機器這些數字都會不同。
//   只有「邊界檢查」區段的 1 與 8 是固定的——那段直接餵 0 進去測 fallback。
// -----------------------------------------------------------------------------

// === 預期輸出 ===
// === 原始查詢 ===
// 硬體支援的並行執行緒數: 16
// 實際採用的核心數估計: 16
//
// === 日常實務: 依工作型態決定 worker 數 ===
// CPU-bound（影像編碼）建議 worker 數: 15
// I/O-bound（素材上傳下載）建議 worker 數: 64
//
// === 邊界檢查: 平台回傳 0 時 ===
// CPU-bound  fallback: 1
// I/O-bound  fallback: 8
