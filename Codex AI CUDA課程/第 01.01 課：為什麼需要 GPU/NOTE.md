# 第 01.01 課：為什麼需要 GPU：平行運算的本質

> **授課軌**：Codex AI CUDA 課程<br>
> **所屬章節**：Part 1 — GPU 與 CUDA 硬體基礎<br>
> **大綱版本**：v2.0.6<br>
> **難度**：入門，但會建立後續 184 課都要使用的效能心智模型<br>
> **本機驗證**：Dell Precision 7550、Quadro RTX 4000 Mobile（Turing，sm_75，8 GB）、CUDA 13.3、GCC 15<br>
> **前置知識**：會閱讀基本 C/C++；本課不要求先懂 CUDA 語法

---

## 0. 這一課要解決什麼問題

AI 常被簡化成一句話：「GPU 核心很多，所以比 CPU 快。」這句話不夠精確，也會導致錯誤設計。本課要建立更可操作的版本：

> **CPU 主要把有限資源用在降低單一工作延遲；GPU 主要把資源用在提高大量相似工作的總吞吐量。**

這不是「CPU 慢、GPU 快」的排名，也不是不可逾越的分類。現代 CPU 有寬向量指令與多核心，現代 GPU 也有快取、控制邏輯和複雜排程器。真正的問題是：

1. 工作中有多少部分能同時執行？
2. 平行部分是否規則、資料量是否足夠？
3. 資料已在 GPU，還是每次都要跨 PCIe 搬運？
4. 計算、記憶體與同步成本，哪一個才是瓶頸？
5. 加速的是核心計算，還是使用者真正等待的端到端流程？

本課結束後，你應該能：

- 分清 **latency（延遲）**與 **throughput（吞吐量）**。
- 解釋 CPU 與 GPU 為何採取不同的晶片資源配置。
- 用 Dennard scaling 終結與 dark silicon 解釋專用加速器興起的背景。
- 分清 SIMD 與 SIMT，不把 GPU thread 當成 CPU thread。
- 正確套用 Amdahl 與 Gustafson 定律，知道兩者回答不同問題。
- 從 GPU kernel 時間與 end-to-end 時間判斷一次加速是否真的有價值。

---

## 1. 延遲與吞吐量是兩個不同目標

### 1.1 定義

- **延遲（latency）**：完成一件工作需要多久，例如一個請求 5 ms 完成。
- **吞吐量（throughput）**：單位時間能完成多少工作，例如每秒處理 50,000 個請求。

若一次處理 1 件工作花 1 ms，延遲是 1 ms，理論吞吐量是 1,000 件/秒。若某個系統把 1,000 件工作分批處理，每批要 5 ms，單件工作可能等得更久，但整體吞吐量可以遠高於 1,000 件/秒。

因此：

> **增加平行度通常直接提高吞吐量，卻不保證降低每一件工作的延遲。**

這就是 AI inference 中 batch size 的核心取捨：較大的 batch 通常提高 GPU 利用率與 tokens/s，卻可能增加單一請求的排隊時間或 time-to-first-token。

### 1.2 CPU 與 GPU 的主要設計取向

| 面向 | CPU 的典型取向 | GPU 的典型取向 |
|---|---|---|
| 主要優化目標 | 少量執行緒的低延遲 | 大量相似工作的高吞吐 |
| 執行資源 | 少數較複雜核心 | 大量執行 lane，組成多個 SM |
| 控制流 | 大型分支預測、亂序執行、推測執行 | 讓許多 warp 輪替；同 warp 分歧會降低效率 |
| 快取策略 | 大型、多層、偏重降低單執行緒等待 | 快取加高頻寬記憶體；用大量可執行 warp 隱藏延遲 |
| 適合工作 | OS、編譯器、複雜分支、低延遲服務、序列工作 | dense tensor、影像、向量/矩陣、規則資料平行工作 |
| 不代表 | CPU 不能平行 | GPU 的每個工作都更低延遲 |

這張表描述的是**主要設計取捨**，不是絕對規則。實際效能必須量測。

### 1.3 概念性晶片面積圖

下面只表達資源配置方向，**不是任何真實晶片的比例圖**：

```text
CPU（偏低延遲）                         GPU（偏高吞吐）
┌─────────────────────────┐            ┌─────────────────────────┐
│ 大型 cache               │            │ 執行 lane  執行 lane     │
├────────────┬────────────┤            │ 執行 lane  執行 lane     │
│ 複雜核心   │ 複雜核心    │            │ 執行 lane  執行 lane     │
│ OoO/branch │ OoO/branch  │            │ 執行 lane  執行 lane     │
├────────────┴────────────┤            ├─────────────────────────┤
│ coherency / I/O / control│            │ warp 排程 / cache / shared│
└─────────────────────────┘            └─────────────────────────┘
   用更多資源縮短少量工作等待                 用更多資源同時推進大量工作
```

GPU 並不是沒有控制與快取；CPU 也不是沒有大量 ALU。差異在於功耗與面積預算的分配優先序。

---

## 2. 為什麼不能只靠 CPU 頻率一直變快

### 2.1 Dennard scaling 的原始承諾

Dennard scaling 描述一組 MOSFET 等比例縮小的理想關係：尺寸與電壓下降時，可以提高電晶體密度與頻率，同時讓單位面積功率密度大致受控。早期製程演進因此能同時帶來更多電晶體與更高時脈。

動態功耗可用下式建立直覺：

\[
P_{dynamic} \propto \alpha C V^2 f
\]

- \(\alpha\)：切換活動因子（switching activity）
- \(C\)：切換電容
- \(V\)：電壓
- \(f\)：時脈

關鍵在 \(V^2\)。當電壓不再能依相同比例下降，繼續提高頻率會快速碰到功耗與散熱牆；漏電功耗也愈來愈重要。「電晶體變多」不再自動等於「所有電晶體都能全速同時打開」。

### 2.2 Dark silicon（暗矽）

**Dark silicon** 不是說晶片上真的有一塊黑色區域，而是：在既定功耗與溫度預算下，某些電路不能一直同時以最高性能運作。

這推動了兩種方向：

1. **多核心與平行化**：用較多執行單元處理可平行工作。
2. **專用化**：同一面積與功耗下，用 GPU、Tensor Core 或其他加速器更有效率地執行特定模式。

專用化不是免費午餐。若工作模式不符合加速器，硬體再多也可能閒置。

---

## 3. AI 為什麼特別適合 GPU

許多 AI 核心操作都能表示成大規模 tensor 運算，例如：

\[
C_{ij} = \sum_k A_{ik} B_{kj}
\]

矩陣中的大量輸出元素可以平行計算，且常具備以下特徵：

- **資料平行**：同一公式套用到大量元素。
- **規則控制流**：大部分執行緒走相似路徑。
- **大量工作**：足以攤平 kernel launch 與排程成本。
- **資料重用**：tile 放進 register/shared memory 後可重複參與運算。
- **高算術強度的可能性**：每搬一個 byte，可進行多次運算。

但「AI」這個標籤本身不保證適合 GPU。以下工作可能仍由 CPU 更有效：

- tokenizer、檔案 I/O、網路協定與複雜 request orchestration。
- 很小的 tensor 或 batch，GPU 啟動成本占比過高。
- 高度不規則、分支發散、pointer chasing 的工作。
- 每做一次簡單運算就跨 PCIe 搬回資料。

成熟系統通常是 CPU 與 GPU 的**異質協作**，不是二選一。

---

## 4. SIMD 與 SIMT：相似，但不能混為一談

### 4.1 SIMD

SIMD（Single Instruction, Multiple Data）是一條向量指令同時操作多個資料 lane。概念例：一條指令同時計算 8 個 float 的加法。

程式設計者或編譯器通常直接看到向量寬度、mask 與向量暫存器。

### 4.2 SIMT

SIMT（Single Instruction, Multiple Threads）讓程式設計者撰寫許多**邏輯 scalar threads**。NVIDIA GPU 會把 thread 組成 warp；目前 CUDA NVIDIA GPU 的一個 warp 是 32 threads，硬體以 warp 為排程單位。

優點是每個 thread 可有自己的 index、register state 與控制流。代價是同一 warp 若走不同分支，硬體通常必須分段執行各條路徑：

```cpp
if (thread_id % 2 == 0) {
    path_a();
} else {
    path_b();
}
```

這段程式合法，但同一 warp 內一半 thread 走 A、一半走 B 時，可能發生 **warp divergence**。SIMT 提供比裸 SIMD 友善的程式模型，卻沒有取消底層 lockstep 執行的效能代價。

> 本課只建立區別；warp、occupancy 與 divergence 會在後續課程量化。

---

## 5. Heterogeneous computing：CPU 與 GPU 的分工

CUDA 應用程式由 CPU 的 **host code** 開始，GPU 是 **device**。典型流程為：

```text
CPU host memory                         GPU device memory
┌──────────────┐   H2D copy            ┌──────────────┐
│ 準備/讀取資料 │ ────────────────────> │ 輸入 tensor   │
└──────────────┘                        └──────┬───────┘
                                             │ launch kernel
CPU 可做其他工作                              v
                                      ┌──────────────┐
                                      │ 大量 threads │
                                      └──────┬───────┘
                                             │ D2H copy（需要時）
                                             v
                                      CPU 消費結果
```

三個重要結論：

1. **傳輸是工作**：H2D/D2H 會消耗時間與頻寬。
2. **啟動是工作**：每次 kernel launch 都有固定成本。
3. **資料駐留很重要**：把多個算子留在 GPU 執行，通常比每個算子後都搬回 CPU 更合理。

後續課程會用 stream、pinned memory、async copy、kernel fusion 與 CUDA Graphs 降低這些成本；現在先學會在報告數字時分開：

- **kernel time**：只計算 GPU 上的核心執行。
- **end-to-end time**：從真正輸入到真正輸出的使用者等待時間。

---

## 6. Amdahl 與 Gustafson：兩種問題，兩個答案

兩個模型共用 \(P\)（理想化的平行處理器數量），但它們的串行比例採用**不同的正規化基準**，因此下面分別寫成 \(s_A\) 與 \(s_G\)。

### 6.1 Amdahl's Law：固定問題大小

對固定問題，令 \(s_A\) 為單處理器基準時間中不能平行的比例：

\[
S_A(P) = \frac{1}{s_A + \frac{1-s_A}{P}}
\]

當 \(P \to \infty\)：

\[
S_A(\infty) = \frac{1}{s_A}
\]

若 5% 無法平行，即使平行硬體無限多，固定工作最多也只接近 20 倍加速。這會迫使我們處理資料搬運、同步、CPU 前後處理與小 kernel，而不是只增加 GPU 核心。

### 6.2 Gustafson's Law：隨硬體擴大問題

對隨 \(P\) 擴大的問題，先把在 \(P\) 個處理器上的平行執行時間正規化為 1，令其中不能平行的時間比例為 \(s_G\)：

\[
S_G(P) = P - s_G(P-1)
\]

它問的是另一件事：若允許問題規模隨處理器增加，在固定執行時間內能完成多少更大的工作？科學運算與 AI 訓練常會把更多算力換成更大模型、更多 token、較大 batch 或更高解析度，而不是只把原問題縮短。

### 6.3 同樣數值的示意比較，為什麼答案差很多

為了並列兩條曲線，暫時分別令 \(s_A=0.05\)、\(s_G=0.05\)、\(P=64\)：

- Amdahl：\(S_A \approx 15.42\)
- Gustafson：\(S_G = 60.85\)

兩者沒有互相矛盾：

| 定律 | 不變的是什麼 | 問題 |
|---|---|---|
| Amdahl | 問題大小 | 原本這份工作最多能縮短多少？ |
| Gustafson | 大致執行時間 | 增加硬體後能處理多大的工作？ |

這裡把兩個比例都設成 5% 只是教學比較，**不表示同一工作量測出的 \(s_A\) 可以原封不動當作 \(s_G\)**；若要互換，必須根據固定問題與 scaled problem 的實際時間重新正規化。

另外，真實 GPU 的 \(P\) 不能簡單等同 CUDA Core 數。記憶體、排程、occupancy、同步與資料依賴都會讓有效平行度低於理想模型。這兩條定律是思考工具，不是效能預測器。

---

## 7. 實驗 A：用程式探索 scaling law

### 7.1 檔案

`src/scaling_laws.cpp` 會把同一個**示意百分比**分別當作 \(s_A\) 與 \(s_G\)，列出兩種理論加速比，也能接受自訂參數。輸出會再次提醒兩個比例不是同一個實測量。它是純 C++，即使尚未完成 CUDA Toolkit 課程也能執行。

### 7.2 編譯與執行

在本課目錄執行：

```bash
g++ -std=c++20 -O3 -Wall -Wextra -Wpedantic \
  src/scaling_laws.cpp -o /tmp/scaling_laws

/tmp/scaling_laws
/tmp/scaling_laws 0.05 64
```

自訂案例輸出：

```text
  串行比例    平行比例          P      Amdahl 加速比     Gustafson 加速比
------------------------------------------------------------------------
    5.00%      95.00%            64             15.42               60.85

解讀：Amdahl 固定問題大小；Gustafson 隨處理器增加問題規模。

模型邊界：表中同一百分比只是並列示意；Amdahl 的 s_A 來自固定問題基準，Gustafson 的 s_G 來自擴張問題的平行執行，兩者不是同一個實測量。
```

輸入錯誤（例如串行比例 `1.2`、處理器數 `1.5`，或超出本工具的教學上限 \(10^{12}\)）會顯示原因並回傳 exit code 2，而不是輸出沒有意義的數字。

### 7.3 觀察題

依序執行：

```bash
/tmp/scaling_laws 0.10 1024
/tmp/scaling_laws 0.01 1024
```

把 \(s_A\) 從 10% 降到 1%，對 Amdahl 上限的影響，遠比把 \(P\) 從 64 增加到 1024 更值得注意。優化加速器系統時，降低 serial/overhead 往往比盲目增加平行資源更有效。

---

## 8. 實驗 B：核心很快，不代表端到端很快

### 8.1 實驗設計

`src/latency_throughput.cu` 在單一 CPU host thread 與 GPU 上執行相同 SAXPY：

\[
y_i = 2.5x_i + y_i
\]

程式分開量測：

- CPU 計算時間。
- GPU kernel-only 時間。
- GPU end-to-end：H2D 輸入 + kernel + D2H 輸出；不含一次性配置。
- CPU/GPU 結果逐元素比對。

CPU 端以 `-O3` 編譯，編譯器可以自動向量化；「單一 CPU thread」不等於刻意關閉 SIMD。程式在計時區尾端加入零指令的 compiler barrier，確保陣列結果在停止計時前可被觀察，避免 benchmark 被最佳化掉。每組取數次測量中的最短值，以降低背景干擾，但正式效能工程還應報 median、分位數、暖機策略與誤差。

### 8.2 編譯與執行

本機課程環境已完成 CUDA 13.3，可用共用工具自動選目前 GPU 架構：

```bash
../../tools/build.sh src/latency_throughput.cu --native \
  -o /tmp/latency_throughput

/tmp/latency_throughput
```

若你正在一台尚未完成 Part 2 CUDA 安裝的機器，只讀本節即可；完成課 2.3 後再回來執行。

### 8.3 黃金筆電實測

以下是 **2026-07-18** 在 Quadro RTX 4000 Mobile（sm_75）、CUDA 13.3 的一次實測。這些數字只證明程式在此環境的行為，不是其他 CPU/GPU 的保證：

```text
裝置：Quadro RTX 4000 with Max-Q Design（sm_75）
實驗：SAXPY y = 2.5*x + y；每欄取多次測量的最短時間。
GPU 核心時間不含傳輸；端到端時間包含 H2D + kernel + D2H，但不含配置。

           N         CPU ms      kernel ms         E2E ms     CPU/kernel        CPU/E2E     check
-------------------------------------------------------------------------------------------------
        1024       0.000131       0.004096       0.017234      0.031982x      0.007601x      PASS
     1048576       0.200422       0.037088       1.364479      5.403958x      0.146885x      PASS
    16777216       7.303802       0.573248      22.397756     12.741085x      0.326095x      PASS
```

### 8.4 如何解讀

1. **1,024 元素**：工作太小。`CPU/kernel = 0.031982x`，表示 GPU kernel 只有約 3.20% 的 CPU 速度、耗時約 31.3 倍；固定啟動成本已壓過有效計算。
2. **1,048,576 元素**：kernel-only 約快 5.40 倍，證明大量元素能利用 GPU 吞吐量。
3. **16,777,216 元素**：kernel-only 約快 12.74 倍，但完整 H2D + kernel + D2H 只有 CPU 速度的約 0.326 倍，仍較慢。
4. **全部 PASS**：先確定結果正確，再討論速度。錯誤答案的加速比沒有價值。

為什麼大型工作端到端仍輸？SAXPY 每個元素只做少量算術，卻要搬很多 bytes，屬低算術強度工作；本實驗還刻意讓每次操作都跨 PCIe 往返。若資料長時間留在 GPU，連續執行許多算子，或能 fusion 多個操作，傳輸成本就能被更多計算攤平。

本課最重要的量測規則是：

> **可以報 kernel speedup，但不能把它冒充 application speedup。**

---

## 9. 一個可重複使用的 GPU 適用性檢查表

面對任何候選工作，先回答：

| 問題 | 偏向適合 GPU 的訊號 | 風險訊號 |
|---|---|---|
| 有多少獨立工作？ | 數萬到數百萬相似元素 | 只有少量工作 |
| 控制流規則嗎？ | 多數 thread 同路徑 | 高度分支、pointer chasing |
| 每 byte 做多少運算？ | 高資料重用、高算術強度 | 一搬進來只做一兩次運算 |
| 資料在哪裡？ | 已在 GPU，接著有多個算子 | 每步都 H2D/D2H |
| serial fraction 多大？ | CPU orchestration 很小 | 前後處理與同步占比高 |
| 真正目標？ | 吞吐量、批次工作 | 單一極小請求最低延遲 |
| 正確性可驗證嗎？ | 有 CPU reference / tolerance | 只看速度、不比答案 |

「有風險訊號」不等於不能用 GPU，而是需要不同演算法、批次化、資料布局、fusion 或 CPU/GPU pipeline。

---

## 10. 練習

### 練習 1：Amdahl 上限

某個**固定問題**在單處理器基準中有 \(s_A=10\%\) 不能平行，理想平行資源增加到 1,024。先手算，再用程式驗證：

```bash
/tmp/scaling_laws 0.10 1024
```

回答：為什麼硬體增加 1,024 倍，固定問題卻不到 10 倍加速？

### 練習 2：把資料留在 GPU

目前 end-to-end 每次都複製 `x`、`y` 到 GPU，再把結果複製回來。先不要改碼，預測以下變更會如何影響結果：

1. H2D 一次。
2. 連續執行 100 次 kernel。
3. 最後才 D2H 一次。

判斷 kernel time、總傳輸時間與端到端加速比各會怎麼變。

### 練習 3：問題大小的 crossover

在 `latency_throughput.cu` 的 `sizes` 增加 `1U << 12U`、`1U << 16U`、`1U << 22U`，重新執行。找出這台機器上 kernel-only 從輸變贏的大致範圍。

這個 crossover 只屬於這支程式與這台機器；不要把它寫成固定常數。

### 練習 4：工作分類

對下列工作判斷先放 CPU、GPU，或混合 pipeline，並用本課檢查表說明：

- 編譯 10,000 個彼此不同的 C++ source files。
- 對 4K 影像每個 pixel 做相同濾鏡。
- Transformer 大 batch 的矩陣乘法。
- 解析高度分支化、大小不一的 JSON request。
- CPU tokenizer + GPU model inference。

---

## 11. 自我檢查與答案

### 11.1 快問快答

1. **GPU 核心很多，是否代表任何單一工作延遲都較低？**
   否。小工作可能被 launch、排程與傳輸固定成本主導。

2. **Amdahl 與 Gustafson 哪一個正確？它們的串行比例能直接共用嗎？**
   兩者都正確；前者固定問題大小，後者允許問題規模隨硬體擴張。\(s_A\) 與 \(s_G\) 的正規化基準不同，不能把同一筆量測不經轉換直接共用。

3. **SIMT 是否等於每個 thread 完全獨立執行？**
   程式模型提供獨立 thread state，但硬體以 warp 排程；同 warp 分歧可能序列化路徑。

4. **kernel 快 10 倍，應用程式就快 10 倍嗎？**
   不一定。必須把 serial CPU、傳輸、同步、配置與其他 stage 算入端到端時間。

5. **GPU 最適合的訊號是什麼？**
   大量、規則、可獨立執行且有足夠資料重用的工作。

### 11.2 練習 1 數值

對 Amdahl 固定問題令 \(s_A=0.10\)、\(P=1024\)；另以 \(s_G=0.10\) 作 Gustafson 的獨立示意：

- Amdahl：\(1/(0.1+0.9/1024) \approx 9.91\)
- Amdahl 理論極限：\(1/s_A=10\)
- Gustafson：\(1024-0.1\times1023=921.70\)

### 11.3 練習 2 預期

kernel 執行時間本身大致不因是否反覆傳輸而改變；但 100 次計算共用一次 H2D/D2H，傳輸占總時間的比例會下降，端到端加速比通常顯著改善。實際結果仍受記憶體頻寬、cache、thermal throttling 與數值操作影響，後續課程會正式量測。

---

## 12. 本課結論

請保留五句話：

1. CPU 與 GPU 是不同資源取捨，不是簡單快慢排名。
2. GPU 以大量可執行工作隱藏延遲，追求整體吞吐量。
3. AI 的規則 tensor 運算與資料重用，使它常能符合 GPU 的設計方向。
4. Amdahl 提醒你消除 serial fraction；Gustafson 解釋為何更多算力常被拿來解更大問題。
5. **永遠分開 kernel time 與 end-to-end time，並先驗證正確性。**

本課完成證明：

> 我能用 Amdahl/Gustafson 定量解釋平行化上限，並以本機 SAXPY 實測區分 GPU kernel throughput 與包含 PCIe 傳輸的端到端成本。

---

## 13. 延伸閱讀

以下優先使用原始論文與官方文件：

1. NVIDIA, [CUDA Programming Guide — Programming Model](https://docs.nvidia.com/cuda/cuda-programming-guide/01-introduction/programming-model.html)：host/device 異質程式模型。
2. NVIDIA, [CUDA C++ Programming Guide](https://docs.nvidia.com/cuda/cuda-c-programming-guide/)：CUDA C++ 的權威規格；本大綱指定 §1。
3. David B. Kirk, Wen-mei W. Hwu, Izzat El Hajj, *Programming Massively Parallel Processors*, 5th ed., Ch. 1。
4. Gene M. Amdahl, [Validity of the Single Processor Approach to Achieving Large Scale Computing Capabilities](https://doi.org/10.1145/1465482.1465560), AFIPS 1967。
5. John L. Gustafson, [Reevaluating Amdahl's Law](https://doi.org/10.1145/42411.42415), Communications of the ACM, 1988。
6. Robert H. Dennard et al., [Design of Ion-Implanted MOSFET's with Very Small Physical Dimensions](https://doi.org/10.1109/JSSC.1974.1050511), IEEE JSSC, 1974。
7. Hadi Esmaeilzadeh et al., [Dark Silicon and the End of Multicore Scaling](https://doi.org/10.1145/2000064.2000108), ISCA 2011。
8. NVIDIA, [CUDA Runtime API — `cudaDeviceProp`](https://docs.nvidia.com/cuda/cuda-runtime-api/structcudaDeviceProp.html)：本課程式使用的 `name`、`major`、`minor` 欄位定義。
9. NVIDIA, [CUDA Runtime API — Event Management](https://docs.nvidia.com/cuda/cuda-runtime-api/group__CUDART__EVENT.html)：`cudaEventRecord` 與 `cudaEventElapsedTime` 的官方語意。
