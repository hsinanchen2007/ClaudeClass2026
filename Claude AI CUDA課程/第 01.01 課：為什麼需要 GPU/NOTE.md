# 課 1.1 — 為什麼需要 GPU：平行運算的本質

> **Part 1 — GPU 與 CUDA 硬體基礎**　｜　難度：入門　｜　本機（sm_75）：✅ 可實跑
> **前置**：無（這是第一課）
> **授課版本**：Claude　｜　**大綱對應**：`CUDA_AI_完整課程大綱_v2.0.6.md` 課 1.1

---

## 1. 學什麼

搞懂 **CPU 與 GPU 為什麼長得完全不一樣**，以及為什麼 AI 這個特定的工作負載，剛好落在 GPU 的甜蜜點上。

這課不寫 kernel（那是 Part 3），但會用**你這台機器的真實規格**算出兩者的差距，讓「GPU 比較快」從一句空話變成可驗證的數字。

> **給資深 C++ 工程師的定位**：你已經懂 cache、pipeline、記憶體階層。這課不是重教那些，而是把你的直覺**重新對焦**——從「怎麼讓一件事做得更快」轉成「怎麼讓一萬件事同時做」。這個轉換是後面 184 課的地基。

---

## 2. 核心概念

### 2.1 兩種設計哲學：latency-oriented vs throughput-oriented

這是整課最重要的一句話：

> **CPU 最佳化的是「單一工作的完成時間」；GPU 最佳化的是「單位時間內完成的工作總量」。**

兩者不是「誰比較先進」，而是**針對不同目標，把電晶體預算花在不同地方**：

| | **CPU（latency-oriented）** | **GPU（throughput-oriented）** |
|---|---|---|
| 電晶體主要花在 | 大 cache、分支預測、亂序執行、暫存器重新命名 | **算術單元（ALU）** |
| 核心數 | 少（個位數到數十） | 多（數千） |
| 單核能力 | 極強，深度管線、高時脈 | 相對弱、時脈較低 |
| 面對記憶體延遲 | **想辦法避免它**（cache、預取、亂序執行） | **想辦法掩蓋它**（切換到別的 warp） |
| 適合 | 分支多、相依性強、不規則的邏輯 | 資料量大、同樣的運算重複很多次 |

**類比（EDA 背景可能有共鳴）**：CPU 像一位資深工程師 debug 一個複雜的時序違例——路徑曲折、要不斷回頭、靠經驗跳躍。GPU 像對整個 netlist 做 DRC——一億個相同的檢查規則，彼此無關，越多人手越快。

⚠️ **常見誤解**：「GPU 比 CPU 快」是錯的說法。**單一工作的延遲，GPU 往往比 CPU 慢**——GPU 時脈更低、記憶體延遲更高（數百 cycle）。GPU 贏在「同時處理幾萬件」的總吞吐。

### 2.2 為什麼會演化成這樣：Dennard scaling 終結與暗矽

歷史脈絡，決定了 2005 年後所有硬體的走向：

- **Moore's Law**：電晶體密度每約兩年翻倍 —— 這條**還大致成立**。
- **Dennard scaling**：電晶體變小，**功率密度維持不變** —— 這條在 **2005 年前後失效**了。

Dennard scaling 一垮，後果是：電晶體還能繼續變多，但**不能全部同時開著跑**，否則晶片會燒掉。這個「有電晶體卻不能用」的現象叫 **dark silicon（暗矽）**。

於是產業被迫轉向：

1. **提升時脈** ❌ 撞上功耗牆（所以 CPU 時脈卡在 3–5 GHz 快 20 年了）
2. **加核心數** ✅ → 多核 CPU
3. **針對特定工作做專用硬體** ✅ → GPU、TPU、Tensor Core、NPU

> 👉 **GPU 的存在本身就是 Dennard scaling 終結的產物。** 既然不能讓所有電晶體高速運轉，那就讓「大量低速、簡單、高能效」的單元一起工作。這也解釋了為什麼 GPU 時脈比 CPU 低——**能效比才是設計目標**。

### 2.3 SIMD vs SIMT

| | **SIMD**（CPU 的 AVX/SSE） | **SIMT**（GPU） |
|---|---|---|
| 全名 | Single Instruction, Multiple **Data** | Single Instruction, Multiple **Threads** |
| 程式模型 | 你要**手動**（或靠編譯器）把資料塞進向量暫存器 | 你寫**單一執行緒**的程式碼，硬體自動讓 32 條一起跑 |
| 分支處理 | 靠 mask，寫起來痛苦 | 硬體處理，但會有 **divergence 代價**（課 4.2） |
| 寬度 | 固定（AVX2 = 8 個 float） | 一個 **warp = 32 threads** |

**SIMT 是 CUDA 好寫的關鍵**：你寫的 kernel 看起來像是在描述「一條執行緒做什麼」，硬體負責讓 32 條 lockstep 執行。這比手刻 AVX intrinsics 人性化太多——但**代價是你必須知道 warp 存在**，否則會寫出效能災難（Part 4 的主題）。

### 2.4 Amdahl vs Gustafson：GPU 幾萬條執行緒到底有沒有意義

這是本課**最容易被略過、但最該懂**的一段。

**Amdahl 定律（1967）**：問題規模**固定**，加處理器
$$S(N) = \frac{1}{(1-p) + p/N} \quad\xrightarrow{N \to \infty}\quad \frac{1}{1-p}$$

**Gustafson 定律（1988）**：時間預算**固定**，問題規模隨處理器變大
$$S(N) = (1-p) + p \cdot N$$

同樣 `p = 0.9`、`N = 40960`（正是你 GPU 的常駐執行緒數）：

| 定律 | 加速比 | 為什麼 |
|---|---|---|
| Amdahl | **10×**（撞上 1/(1-p) 天花板） | 序列的那 10% 吃掉幾乎全部時間 |
| Gustafson | **36864×** | 問題跟著變大，序列部分佔比被稀釋 |

**兩者都對，只是問的問題不同**：
- Amdahl 問：「**同一張** 512×512 影像，能算多快？」→ 很快撞牆
- Gustafson 問：「**同樣時間內**，能算多大的模型？」→ 持續受益

👉 **深度學習是 Gustafson 的教科書案例**：硬體變強時，業界不是把同一個模型訓練得更快，而是**把模型做更大**（參數量、batch size、序列長度一起長）。所以 GPU 的幾萬條執行緒不是虛胖，是真的用得掉。

⚠️ **但 Amdahl 沒有失效，它在推論時會回來咬人**：單一 request 的**延遲**就是固定規模問題。LLM decode 一次只能吐一個 token，那是**本質序列**的——這正是 Part 9 的核心矛盾（throughput vs latency）。你未來的職涯主軸，很大一部分就是在跟這條線搏鬥。

### 2.5 Heterogeneous computing：不是取代，是分工

CUDA 程式永遠是 **CPU + GPU 協同**：

```
CPU（host）                     GPU（device）
  ├─ 作業系統、I/O、檔案                ├─ 大規模平行運算
  ├─ 複雜控制流、分支                   ├─ 資料平行的 kernel
  ├─ 決定「做什麼」                     └─ 負責「大量地做」
  └─ 啟動 kernel、搬資料
```

⚠️ **這個分工帶來 CUDA 最常見的效能陷阱**：CPU↔GPU 之間的資料傳輸走 PCIe，**頻寬遠低於 GPU 自己的記憶體頻寬**。很多「GPU 版本反而更慢」的案例，都是敗在搬資料而不是算得慢（課 3.3、8.8 會處理）。

---

## 3. 動手做

### 3.1 程式一：用你的真實硬體算出設計差異

完整程式：[`src/gpu_vs_cpu_profile.cu`](src/gpu_vs_cpu_profile.cu)

核心片段——查詢裝置屬性並換算峰值：

```cuda
cudaDeviceProp p{};
cudaGetDeviceProperties(&p, dev);

// ⚠️ 踩雷：舊教材都寫 p.clockRate，但 CUDA 13.x 已把它從 cudaDeviceProp 移除！
int smClockKHz = 0, memClockKHz = 0;
cudaDeviceGetAttribute(&smClockKHz,  cudaDevAttrClockRate,       dev);
cudaDeviceGetAttribute(&memClockKHz, cudaDevAttrMemoryClockRate, dev);

// 峰值算力：每個 core 每 cycle 做 1 次 FMA = 2 FLOP
double tflops = totalCores * 2.0 * (smClockKHz/1e6) / 1000.0;

// 峰值頻寬：bus width(bit)÷8 × 記憶體時脈 × 2 (DDR)
double bwGBs = p.memoryBusWidth / 8.0 * (memClockKHz/1e6) * 2.0;
```

**編譯與執行**：
```bash
cd "第 01.01 課：為什麼需要 GPU"
nvcc -std=c++20 -O3 -arch=native src/gpu_vs_cpu_profile.cu -o profile
./profile
```

**確認未來桌機也能編**（本課程的硬規則：所有程式都要能編到 sm_120）：
```bash
nvcc -std=c++20 -O3 -arch=sm_120 src/gpu_vs_cpu_profile.cu -o profile_5090   # ✅ 已驗證通過
# 或用課程工具一次稽核全部架構：
../../tools/check_build.sh --lesson "第 01.01 課：為什麼需要 GPU"
```

### 3.2 實際輸出（本機 Quadro RTX 4000 / sm_75 / CUDA 13.3）

```
═══ GPU 0：Quadro RTX 4000 with Max-Q Design ═══
  Compute capability : 7.5 (sm_75)
  SM 數量            : 40
  FP32 CUDA core     : 2560  (40 SM × 64 core/SM)
  SM 時脈            : 1.380 GHz
  記憶體             : 7.6 GB, 256-bit @ 6.001 GHz(effective)
  L2 cache           : 4.0 MB
  Shared mem / block : 48 KB
  Warp size          : 32
  每 SM 最大常駐執行緒: 1024
  → 全 GPU 可常駐執行緒: 40960

  峰值 FP32 算力     : 7.07 TFLOPS
    = 2560 core × 2 FLOP/cycle(FMA) × 1.380 GHz

  峰值記憶體頻寬     : 384.1 GB/s
    = 256 bit ÷ 8 × 6.001 GHz × 2 (DDR)

  Machine balance    : 18.4 FLOP/byte
```

### 3.3 程式二：Amdahl vs Gustafson

完整程式：[`src/amdahl_gustafson.cpp`](src/amdahl_gustafson.cpp)（純 CPU，不需 GPU）

```bash
g++ -std=c++20 -O2 src/amdahl_gustafson.cpp -o amdahl && ./amdahl
```

實際輸出（節錄）：
```
═══ Amdahl 定律：問題規模固定 ═══
  可平行比例 p | N=8        N=64       N=512      N=2560     N=40960    |  理論上限
  p = 0.900    |      4.7×       8.8×       9.8×      10.0×      10.0×  |     10.0×
  p = 0.990    |      7.5×      39.3×      83.8×      96.3×      99.8×  |    100.0×
  p = 0.999    |      7.9×      60.2×     338.8×     719.3×     976.2×  |   1000.0×

═══ Gustafson 定律：時間預算固定 ═══
  p = 0.900    |      7.3×      57.7×     460.9×    2304.1×   36864.1×
  p = 0.990    |      7.9×      63.4×     506.9×    2534.4×   40550.4×
```

**看懂這兩張表就懂了半課**：同樣 40960 條執行緒，Amdahl 說「只能快 10 倍」，Gustafson 說「能做 36864 倍的事」。差別純粹在於**問題規模有沒有跟著長**。

---

## 4. 本機 CPU vs GPU 對照（真實數字）

| | **CPU**：Xeon W-10885M | **GPU**：Quadro RTX 4000 (Max-Q) | 比值 |
|---|---|---|---|
| 核心 | 8 核 / 16 執行緒 | **2560** CUDA core（40 SM） | 320× |
| 時脈 | ~4.3 GHz（全核） | 1.38 GHz | 0.32× |
| **峰值 FP32** | ~1.10 TFLOPS | **7.07 TFLOPS** | **6.4×** |
| **記憶體頻寬** | ~25–31 GB/s（實測 memcpy） | **384 GB/s**（理論峰值） | **~13×** |
| 同時「在飛」的執行緒 | 16 | **40960** | 2560× |
| 快取 | L3 16 MB | L2 4 MB | 0.25× |
| 功耗 | ~45 W | 80 W | — |

**CPU 峰值算力怎麼算的**（AVX2 + FMA，無 AVX-512）：
```
每核每 cycle = 2 個 FMA 單元 × 8 個 float(AVX2 256-bit) × 2 FLOP = 32 FLOP
8 核 × 32 FLOP × 4.3 GHz ≈ 1101 GFLOPS ≈ 1.10 TFLOPS
```

**這張表的三個重點**：

1. **GPU 時脈更低**（0.32×）卻算力更高（6.4×）——完全靠**平行度**堆出來，不是靠跑更快。這就是 2.1 講的設計哲學。
2. **快取反而更小**（L2 4MB vs L3 16MB）——GPU 不靠 cache 藏延遲，靠**切換 warp**。這是 2.1 那張表最關鍵的一行。
3. **頻寬差距（13×）比算力差距（6.4×）還大**——所以 GPU 特別適合**記憶體密集**的工作，而深度學習大量的層（LayerNorm、activation、attention 的部分階段）正是 memory-bound。

### 4.1 未來桌機（RTX 5090）預期值 —— 拿到卡時的對照基準

本課程的程式**已驗證可編譯到 sm_120**（`nvcc -arch=sm_120` 通過，產出真正的 Blackwell SASS），
所以拿到 5090 後**不用改任何一行**，直接編、直接跑即可。

以官方規格推算，同一支 `gpu_vs_cpu_profile` 在 5090 上應該印出接近這些數字：

| 項目 | 本機 Quadro RTX 4000（sm_75，**實測**） | RTX 5090（sm_120，**預測**） | 倍數 |
|---|---|---|---|
| SM 數 | 40 | 170 | 4.3× |
| FP32 core | 2560（64/SM） | 21760（**128**/SM） | 8.5× |
| 峰值 FP32 | 7.07 TFLOPS | **~104.9 TFLOPS** | **14.8×** |
| 記憶體頻寬 | 384 GB/s | **1792 GB/s**（512-bit GDDR7） | 4.7× |
| **machine balance** | **18.4 FLOP/byte** | **~58.5 FLOP/byte** | **3.2×** |
| VRAM | 8 GB | 32 GB | 4× |

⚠️ **這一欄是預測值，不是實測**（本機沒有 5090）。取得桌機後請重跑本課程式核對。
兩個要特別留意的點：

1. **每 SM 的 core 數從 64 變 128** —— 程式裡的 `fp32CoresPerSM()` 已處理這個分支，
   但這正是「不能假設所有架構都一樣」的例子（課 1.4 會深入 compute capability）。
2. **GDDR7 的頻寬換算可能對不上**：程式用的公式是 `bus width ÷ 8 × 記憶體時脈 × 2 (DDR)`，
   而 GDDR7 改用 PAM3 訊號，`cudaDevAttrMemoryClockRate` 回報的語意可能不同。
   **若算出來與官方 1792 GB/s 有出入，以官方規格為準**，並把這件事當成
   「規格換算公式會隨世代失效」的實例——這在整個課程會反覆出現。

👉 **machine balance 從 18.4 漲到 58.5，是這張表最重要的一行**：
新卡的算力成長（14.8×）遠快於頻寬成長（4.7×），
代表**演算法要餵飽 5090，資料重用率必須比現在高 3 倍**。
這就是為什麼「越新的 GPU 越吃 kernel 優化功力」——硬體給你更多算力，
但你得寫得出足夠 compute-intensive 的 kernel 才拿得到（整個 Part 8 的動機）。

### 4.2 Machine balance：18.4 FLOP/byte

這個數字會貫穿整個 Part 8，現在先建立直覺：

> 你的 GPU 每從記憶體搬 **1 byte**，有能力做 **18.4 次**浮點運算。

意思是：**你的演算法每讀 1 byte，至少要做 18.4 次運算，才可能餵飽算力單元**。做不到就是 memory-bound——這時再強的 Tensor Core 也是閒著。

- 向量加法 `c[i] = a[i] + b[i]`：讀 8 byte、寫 4 byte，只做 1 次加法 → **0.08 FLOP/byte**，遠低於 18.4 → **重度 memory-bound**
- 矩陣乘法（有好好 tiling）：可達 **數十到數百 FLOP/byte** → 才有機會 compute-bound

👉 這就是為什麼 GEMM 是 GPU 的甜蜜點，而 elementwise 操作永遠在浪費算力（課 8.13 的 kernel fusion 就是在解這件事）。

---

## 5. 圖解：電晶體預算花在哪

```
        CPU（latency-oriented）                    GPU（throughput-oriented）
   ┌─────────────────────────────┐         ┌─────────────────────────────┐
   │ ┌────┐ ┌────┐               │         │ ▣▣▣▣▣▣▣▣ ▣▣▣▣▣▣▣▣ ▣▣▣▣▣▣▣▣ │
   │ │Core│ │Core│  ┌──────────┐ │         │ ▣▣▣▣▣▣▣▣ ▣▣▣▣▣▣▣▣ ▣▣▣▣▣▣▣▣ │
   │ │+控制│ │+控制│  │          │ │         │ ▣▣▣▣▣▣▣▣ ▣▣▣▣▣▣▣▣ ▣▣▣▣▣▣▣▣ │
   │ └────┘ └────┘  │          │ │         │ ─────────────────────────── │
   │ ┌────┐ ┌────┐  │  L3 快取  │ │         │ ▣▣▣▣▣▣▣▣ ▣▣▣▣▣▣▣▣ ▣▣▣▣▣▣▣▣ │
   │ │Core│ │Core│  │  16 MB   │ │         │ ▣▣▣▣▣▣▣▣ ▣▣▣▣▣▣▣▣ ▣▣▣▣▣▣▣▣ │
   │ │+控制│ │+控制│  │          │ │         │ ▣▣▣▣▣▣▣▣ ▣▣▣▣▣▣▣▣ ▣▣▣▣▣▣▣▣ │
   │ └────┘ └────┘  └──────────┘ │         │ ─────────────────────────── │
   │  分支預測│亂序執行│預取器     │         │      L2 快取（僅 4 MB）      │
   └─────────────────────────────┘         └─────────────────────────────┘
     8 核，大量面積給「控制與快取」            2560 個 ALU（▣），面積幾乎全給「算」

   延遲策略：想辦法【避免】記憶體延遲          延遲策略：想辦法【掩蓋】記憶體延遲
            （cache 命中、預取、亂序）                （某 warp 等資料時，換另一個 warp 算）
```

**「掩蓋延遲」是 GPU 的靈魂**——40960 條常駐執行緒的真正用途不是「同時算 40960 件事」，而是**永遠有 warp 準備好可以執行**，讓算術單元不要空轉。這個觀念在課 4.1、4.8（occupancy）會被反覆使用。

---

## 6. 自我檢核

完成這課後，你應該能回答：

- [ ] 為什麼說「GPU 比 CPU 快」是不精確的說法？什麼情況下 GPU 反而比較慢？
- [ ] Dennard scaling 終結跟 GPU 的興起有什麼因果關係？
- [ ] SIMT 和 SIMD 差在哪？為什麼 SIMT 比較好寫？
- [ ] 同樣 40960 條執行緒，為什麼 Amdahl 說 10 倍、Gustafson 說 36864 倍？兩者誰對？
- [ ] 你的 GPU machine balance 是 18.4 FLOP/byte，這對「該寫什麼樣的 kernel」有什麼含意？
- [ ] 為什麼 GPU 的 cache（4 MB）比 CPU（16 MB）小這麼多，卻不是設計缺陷？

---

## 7. 踩雷紀錄（本課實測遇到的）

**`cudaDeviceProp::clockRate` 在 CUDA 13.x 已被移除。**

網路上與舊教材幾乎都寫：
```cuda
double ghz = p.clockRate / 1e6;   // ❌ CUDA 13.x 編不過
```
實際錯誤：
```
error: class "cudaDeviceProp" has no member "clockRate"
```
它在 CUDA 12.x 標為 deprecated、13.x 直接拿掉。正確寫法：
```cuda
int khz;
cudaDeviceGetAttribute(&khz, cudaDevAttrClockRate, dev);   // ✅
```

👉 **這是本課程會反覆出現的模式**：CUDA 生態演進很快，網路資料常是舊的。**以本機實際編譯結果為準**，別相信「看起來很合理」的程式碼。

---

## 8. 延伸參考

- **PMPP 5e** Ch.1（Introduction）— 本課程主教材
- [CUDA C++ Programming Guide §1 — Introduction](https://docs.nvidia.com/cuda/cuda-programming-guide/)
- [CUDA Runtime API — Device Management](https://docs.nvidia.com/cuda/cuda-runtime-api/group__CUDART__DEVICE.html)（`cudaDeviceGetAttribute` 的完整屬性清單）
- Gustafson, J. "Reevaluating Amdahl's Law", CACM 1988（兩頁短文，值得一讀）
- 下一課 **1.2 GPU 硬體架構總覽** 會把「SM 裡面有什麼」拆開講

---

**我證明了什麼**：用本機實測算出 GPU 峰值 7.07 TFLOPS／384 GB/s／machine balance 18.4 FLOP/byte，並用 Amdahl vs Gustafson 的數值對照解釋了 40960 條執行緒為何用得掉；同時踩到並修正了 CUDA 13.x 移除 `cudaDeviceProp::clockRate` 的真實 API 變更。
