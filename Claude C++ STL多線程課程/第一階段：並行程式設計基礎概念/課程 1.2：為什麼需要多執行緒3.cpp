/*
# 第一階段：並行程式設計基礎概念

## 課程 1.2：為什麼需要多執行緒

---

### 引言

在上一課中，我們理解了並發與並行的基本概念。但一個很自然的問題是：**為什麼我們要費心學習多執行緒程式設計？** 單執行緒程式不是更簡單、更容易理解嗎？

這一課將深入探討多執行緒程式設計的動機與價值，讓你理解在什麼情況下應該考慮使用多執行緒，以及它能為我們的程式帶來哪些具體的好處。

---

### 一、多執行緒的三大核心價值

多執行緒程式設計主要帶來三個關鍵優勢：

```
┌─────────────────────────────────────────────────────────────┐
│              多執行緒的三大核心價值                           │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐          │
│  │   效能提升   │  │  響應性改善  │  │  資源利用   │          │
│  │ Performance │  │Responsiveness│ │ Resource    │          │
│  │             │  │             │  │ Utilization │          │
│  └─────────────┘  └─────────────┘  └─────────────┘          │
│        │               │                │                   │
│        ▼               ▼                ▼                   │
│  利用多核心CPU    保持程式互動性     充分利用等待時間        │
│  加速運算密集任務  避免介面凍結       提高硬體使用效率        │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

讓我們逐一深入探討這三個價值。

---

### 二、價值一：效能提升（Performance）

#### 2.1 運算密集型任務

當程式需要進行大量計算時，多執行緒可以讓我們利用多核心 CPU 的全部運算能力。

**典型應用場景**：
- 圖像處理與渲染
- 科學計算與數值模擬
- 資料分析與機器學習
- 密碼學運算
- 遊戲物理引擎

#### 範例程式：並行計算的效能提升

```cpp
// 檔案名稱：lesson_1_2_performance.cpp
// 課程：1.2 - 為什麼需要多執行緒
// 說明：展示多執行緒在運算密集型任務中的效能提升

#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <numeric>
#include <cmath>
#include <iomanip>

// 模擬一個運算密集型的任務：計算範圍內所有質數的數量
bool isPrime(long long n) {
    if (n < 2) return false;
    if (n == 2) return true;
    if (n % 2 == 0) return false;
    for (long long i = 3; i * i <= n; i += 2) {
        if (n % i == 0) return false;
    }
    return true;
}

// 計算指定範圍內的質數數量（單執行緒版本）
long long countPrimesInRange(long long start, long long end) {
    long long count = 0;
    for (long long i = start; i <= end; ++i) {
        if (isPrime(i)) {
            ++count;
        }
    }
    return count;
}

// 工作執行緒函式：計算指定範圍內的質數數量，並將結果存入指定位置
void countPrimesWorker(long long start, long long end, long long& result) {
    result = countPrimesInRange(start, end);
}

int main() {
    const long long RANGE_START = 1;
    const long long RANGE_END = 1000000;  // 計算 1 到 100 萬之間的質數
    
    std::cout << "========================================" << std::endl;
    std::cout << "    多執行緒效能提升示範" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "計算範圍: " << RANGE_START << " 到 " << RANGE_END << std::endl;
    
    unsigned int numCores = std::thread::hardware_concurrency();
    std::cout << "可用 CPU 核心數: " << numCores << std::endl;
    std::cout << "----------------------------------------" << std::endl;

    // ========== 方式一：單執行緒計算 ==========
    std::cout << "\n【方式一】單執行緒計算：" << std::endl;
    
    auto singleStart = std::chrono::steady_clock::now();
    long long singleResult = countPrimesInRange(RANGE_START, RANGE_END);
    auto singleEnd = std::chrono::steady_clock::now();
    
    auto singleDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
        singleEnd - singleStart).count();
    
    std::cout << "找到 " << singleResult << " 個質數" << std::endl;
    std::cout << "執行時間: " << singleDuration << " 毫秒" << std::endl;

    // ========== 方式二：多執行緒計算 ==========
    std::cout << "\n【方式二】多執行緒計算（" << numCores << " 個執行緒）：" << std::endl;
    
    auto multiStart = std::chrono::steady_clock::now();
    
    // 準備儲存每個執行緒結果的容器
    std::vector<std::thread> threads;
    std::vector<long long> results(numCores, 0);
    
    // 計算每個執行緒負責的範圍
    long long rangeSize = (RANGE_END - RANGE_START + 1) / numCores;
    
    // 建立並啟動所有執行緒
    for (unsigned int i = 0; i < numCores; ++i) {
        long long threadStart = RANGE_START + i * rangeSize;
        long long threadEnd;
        
        // 最後一個執行緒處理剩餘的所有數字
        if (i == numCores - 1) {
            threadEnd = RANGE_END;
        } else {
            threadEnd = threadStart + rangeSize - 1;
        }
        
        std::cout << "  執行緒 " << i << ": 處理範圍 [" 
                  << threadStart << ", " << threadEnd << "]" << std::endl;
        
        // 建立執行緒，使用 std::ref 傳遞引用
        threads.emplace_back(countPrimesWorker, threadStart, threadEnd, 
                            std::ref(results[i]));
    }
    
    // 等待所有執行緒完成
    for (auto& t : threads) {
        t.join();
    }
    
    // 彙總所有執行緒的結果
    long long multiResult = 0;
    for (unsigned int i = 0; i < numCores; ++i) {
        std::cout << "  執行緒 " << i << " 找到: " << results[i] << " 個質數" << std::endl;
        multiResult += results[i];
    }
    
    auto multiEnd = std::chrono::steady_clock::now();
    auto multiDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
        multiEnd - multiStart).count();
    
    std::cout << "總共找到 " << multiResult << " 個質數" << std::endl;
    std::cout << "執行時間: " << multiDuration << " 毫秒" << std::endl;

    // ========== 效能比較 ==========
    std::cout << "\n========================================" << std::endl;
    std::cout << "效能比較結果：" << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    std::cout << "單執行緒時間: " << singleDuration << " 毫秒" << std::endl;
    std::cout << "多執行緒時間: " << multiDuration << " 毫秒" << std::endl;
    
    double speedup = static_cast<double>(singleDuration) / multiDuration;
    double efficiency = speedup / numCores * 100;
    
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "加速比 (Speedup): " << speedup << " 倍" << std::endl;
    std::cout << "並行效率: " << efficiency << "%" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // 驗證結果一致性
    if (singleResult == multiResult) {
        std::cout << "✓ 結果驗證通過：兩種方式得到相同結果" << std::endl;
    } else {
        std::cout << "✗ 結果驗證失敗：結果不一致！" << std::endl;
    }

    return 0;
}
```

#### 編譯與執行

```bash
# 編譯（加上優化旗標以獲得更準確的效能數據）
g++ -std=c++17 -pthread -O2 -o lesson_1_2_perf lesson_1_2_performance.cpp

# 執行
./lesson_1_2_perf
```

#### 預期輸出

```
========================================
    多執行緒效能提升示範
========================================
計算範圍: 1 到 1000000
可用 CPU 核心數: 8
----------------------------------------

【方式一】單執行緒計算：
找到 78498 個質數
執行時間: 892 毫秒

【方式二】多執行緒計算（8 個執行緒）：
  執行緒 0: 處理範圍 [1, 125000]
  執行緒 1: 處理範圍 [125001, 250000]
  執行緒 2: 處理範圍 [250001, 375000]
  執行緒 3: 處理範圍 [375001, 500000]
  執行緒 4: 處理範圍 [500001, 625000]
  執行緒 5: 處理範圍 [625001, 750000]
  執行緒 6: 處理範圍 [750001, 875000]
  執行緒 7: 處理範圍 [875001, 1000000]
  執行緒 0 找到: 11734 個質數
  執行緒 1 找到: 10538 個質數
  執行緒 2 找到: 10050 個質數
  執行緒 3 找到: 9742 個質數
  執行緒 4 找到: 9520 個質數
  執行緒 5 找到: 9354 個質數
  執行緒 6 找到: 9185 個質數
  執行緒 7 找到: 8375 個質數
總共找到 78498 個質數
執行時間: 156 毫秒

========================================
效能比較結果：
----------------------------------------
單執行緒時間: 892 毫秒
多執行緒時間: 156 毫秒
加速比 (Speedup): 5.72 倍
並行效率: 71.47%
========================================
✓ 結果驗證通過：兩種方式得到相同結果
```

#### 2.2 程式碼關鍵點解析

**工作分配策略**：

```cpp
long long rangeSize = (RANGE_END - RANGE_START + 1) / numCores;
```

將總工作量平均分配給每個執行緒。這是最簡單的負載平衡策略。

**使用 std::ref 傳遞引用**：

```cpp
threads.emplace_back(countPrimesWorker, threadStart, threadEnd, 
                    std::ref(results[i]));
```

`std::thread` 預設會複製所有參數，要傳遞引用必須使用 `std::ref`。

**加速比與並行效率**：

```cpp
double speedup = static_cast<double>(singleDuration) / multiDuration;
double efficiency = speedup / numCores * 100;
```

- **加速比（Speedup）**：多執行緒比單執行緒快多少倍
- **並行效率（Parallel Efficiency）**：實際加速比與理論最大加速比的比值

理論上，8 個核心應該能達到 8 倍加速，但實際上由於各種開銷（執行緒建立、同步、記憶體存取等），通常達不到理論值。

---

### 三、價值二：響應性改善（Responsiveness）

#### 3.1 為什麼響應性很重要

在圖形使用者介面（GUI）程式中，如果主執行緒執行耗時操作，整個介面會「凍結」，無法回應使用者的操作。這會帶來極差的使用者體驗。

**常見場景**：
- 檔案下載時介面仍可操作
- 文件儲存時仍可繼續編輯
- 搜尋執行時仍可取消操作
- 遊戲載入時顯示進度動畫

#### 範例程式：響應性改善示範

```cpp
// 檔案名稱：lesson_1_2_responsiveness.cpp
// 課程：1.2 - 為什麼需要多執行緒
// 說明：展示多執行緒如何改善程式響應性

#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <string>

// 原子變數用於執行緒間的安全通訊
std::atomic<bool> downloadComplete{false};
std::atomic<int> downloadProgress{0};
std::atomic<bool> cancelRequested{false};

// 模擬一個耗時的下載操作
void simulateDownload(const std::string& filename) {
    std::cout << "[下載執行緒] 開始下載: " << filename << std::endl;
    
    const int totalSteps = 10;
    for (int i = 1; i <= totalSteps; ++i) {
        // 檢查是否收到取消請求
        if (cancelRequested.load()) {
            std::cout << "[下載執行緒] 收到取消請求，停止下載" << std::endl;
            return;
        }
        
        // 模擬下載一個區塊
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        // 更新進度
        downloadProgress.store(i * 100 / totalSteps);
        std::cout << "[下載執行緒] 進度: " << downloadProgress.load() << "%" << std::endl;
    }
    
    downloadComplete.store(true);
    std::cout << "[下載執行緒] 下載完成！" << std::endl;
}

// 模擬使用者介面執行緒
void simulateUIThread() {
    std::cout << "[UI 執行緒] 使用者介面已啟動" << std::endl;
    
    int lastProgress = -1;
    int uiUpdateCount = 0;
    
    while (!downloadComplete.load()) {
        // 模擬 UI 更新（例如動畫、時鐘更新等）
        ++uiUpdateCount;
        
        // 如果進度有變化，顯示更新
        int currentProgress = downloadProgress.load();
        if (currentProgress != lastProgress) {
            std::cout << "[UI 執行緒] 更新進度條顯示: " 
                      << currentProgress << "%" << std::endl;
            lastProgress = currentProgress;
        }
        
        // 模擬其他 UI 工作
        if (uiUpdateCount % 10 == 0) {
            std::cout << "[UI 執行緒] 處理使用者輸入事件 #" 
                      << uiUpdateCount / 10 << std::endl;
        }
        
        // UI 執行緒不應該佔用過多 CPU
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    std::cout << "[UI 執行緒] 下載完成，更新介面狀態" << std::endl;
    std::cout << "[UI 執行緒] 共處理了 " << uiUpdateCount << " 次 UI 更新" << std::endl;
}

void demonstrateBlockingUI() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "【情境一】單執行緒（阻塞式）" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "模擬在單執行緒中執行下載..." << std::endl;
    std::cout << "(注意：下載期間 UI 完全無法回應)" << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    
    auto start = std::chrono::steady_clock::now();
    
    // 模擬阻塞式下載
    std::cout << "[主執行緒] 開始下載..." << std::endl;
    for (int i = 1; i <= 5; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::cout << "[主執行緒] 下載進度: " << i * 20 << "%" << std::endl;
        // 在這期間，UI 完全凍結！
    }
    std::cout << "[主執行緒] 下載完成！" << std::endl;
    std::cout << "[主執行緒] ⚠️ 在下載的 2.5 秒內，UI 完全無法回應！" << std::endl;
    
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end - start).count();
    std::cout << "總時間: " << duration << " 毫秒" << std::endl;
}

void demonstrateResponsiveUI() {
    // 重設狀態
    downloadComplete.store(false);
    downloadProgress.store(0);
    cancelRequested.store(false);
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "【情境二】多執行緒（響應式）" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "使用獨立執行緒進行下載，UI 保持響應..." << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    
    auto start = std::chrono::steady_clock::now();
    
    // 在獨立執行緒中執行下載
    std::thread downloadThread(simulateDownload, "large_file.zip");
    
    // 主執行緒繼續處理 UI
    simulateUIThread();
    
    // 等待下載執行緒完成
    downloadThread.join();
    
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end - start).count();
    
    std::cout << "----------------------------------------" << std::endl;
    std::cout << "總時間: " << duration << " 毫秒" << std::endl;
    std::cout << "✓ 在下載期間，UI 持續保持響應！" << std::endl;
}

void demonstrateCancellation() {
    // 重設狀態
    downloadComplete.store(false);
    downloadProgress.store(0);
    cancelRequested.store(false);
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "【情境三】支援取消操作" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "示範使用者可以在下載中途取消操作..." << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    
    // 在獨立執行緒中執行下載
    std::thread downloadThread(simulateDownload, "another_file.zip");
    
    // 主執行緒等待一段時間後發送取消請求
    std::cout << "[主執行緒] 等待 1.5 秒後將發送取消請求..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    
    std::cout << "[主執行緒] 使用者點擊了取消按鈕！" << std::endl;
    cancelRequested.store(true);
    
    // 等待下載執行緒結束
    downloadThread.join();
    
    std::cout << "----------------------------------------" << std::endl;
    std::cout << "✓ 下載已被成功取消！" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "    多執行緒響應性改善示範" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // 情境一：展示阻塞式 UI 的問題
    demonstrateBlockingUI();
    
    // 情境二：展示響應式 UI 的優勢
    demonstrateResponsiveUI();
    
    // 情境三：展示取消操作的能力
    demonstrateCancellation();
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "示範結束" << std::endl;
    std::cout << "========================================" << std::endl;
    
    return 0;
}
```

#### 編譯與執行

```bash
g++ -std=c++17 -pthread -o lesson_1_2_resp lesson_1_2_responsiveness.cpp
./lesson_1_2_resp
```

#### 預期輸出

```
========================================
    多執行緒響應性改善示範
========================================

========================================
【情境一】單執行緒（阻塞式）
========================================
模擬在單執行緒中執行下載...
(注意：下載期間 UI 完全無法回應)
----------------------------------------
[主執行緒] 開始下載...
[主執行緒] 下載進度: 20%
[主執行緒] 下載進度: 40%
[主執行緒] 下載進度: 60%
[主執行緒] 下載進度: 80%
[主執行緒] 下載進度: 100%
[主執行緒] 下載完成！
[主執行緒] ⚠️ 在下載的 2.5 秒內，UI 完全無法回應！
總時間: 2503 毫秒

========================================
【情境二】多執行緒（響應式）
========================================
使用獨立執行緒進行下載，UI 保持響應...
----------------------------------------
[UI 執行緒] 使用者介面已啟動
[下載執行緒] 開始下載: large_file.zip
[下載執行緒] 進度: 10%
[UI 執行緒] 更新進度條顯示: 10%
[UI 執行緒] 處理使用者輸入事件 #1
[下載執行緒] 進度: 20%
[UI 執行緒] 更新進度條顯示: 20%
[UI 執行緒] 處理使用者輸入事件 #2
...
[下載執行緒] 進度: 100%
[UI 執行緒] 更新進度條顯示: 100%
[下載執行緒] 下載完成！
[UI 執行緒] 下載完成，更新介面狀態
[UI 執行緒] 共處理了 102 次 UI 更新
----------------------------------------
總時間: 5012 毫秒
✓ 在下載期間，UI 持續保持響應！

========================================
【情境三】支援取消操作
========================================
示範使用者可以在下載中途取消操作...
----------------------------------------
[下載執行緒] 開始下載: another_file.zip
[主執行緒] 等待 1.5 秒後將發送取消請求...
[下載執行緒] 進度: 10%
[下載執行緒] 進度: 20%
[下載執行緒] 進度: 30%
[主執行緒] 使用者點擊了取消按鈕！
[下載執行緒] 收到取消請求，停止下載
----------------------------------------
✓ 下載已被成功取消！

========================================
示範結束
========================================
```

#### 3.2 程式碼關鍵點解析

**std::atomic 的使用**：

```cpp
std::atomic<bool> downloadComplete{false};
std::atomic<int> downloadProgress{0};
```

`std::atomic` 提供了執行緒安全的變數存取，無需使用互斥鎖。這是執行緒間通訊的最簡單方式，我們將在後續課程中詳細介紹。

**取消機制的設計**：

```cpp
if (cancelRequested.load()) {
    std::cout << "[下載執行緒] 收到取消請求，停止下載" << std::endl;
    return;
}
```

工作執行緒定期檢查取消標記，這是一種常見的「協作式取消」模式。

---

### 四、價值三：資源利用（Resource Utilization）

#### 4.1 I/O 等待時間的利用

許多程式花費大量時間等待 I/O 操作（網路請求、檔案讀寫、資料庫查詢）。在單執行緒程式中，這些等待時間完全被浪費。多執行緒可以讓我們在等待時執行其他有用的工作。

#### 範例程式：I/O 密集型任務的並行化

```cpp
// 檔案名稱：lesson_1_2_io_utilization.cpp
// 課程：1.2 - 為什麼需要多執行緒
// 說明：展示多執行緒如何提升 I/O 密集型任務的效率

#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <string>
#include <iomanip>

// 模擬一個 I/O 密集型操作（例如：HTTP 請求或檔案讀取）
struct WebPage {
    std::string url;
    std::string content;
    int statusCode;
    long long fetchTimeMs;
};

// 模擬從網路獲取網頁內容
WebPage fetchWebPage(const std::string& url) {
    auto start = std::chrono::steady_clock::now();
    
    // 模擬網路延遲（200-500 毫秒的隨機延遲）
    int delay = 200 + (std::hash<std::string>{}(url) % 300);
    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end - start).count();
    
    return WebPage{
        url,
        "Content of " + url,
        200,
        duration
    };
}

// 循序獲取多個網頁
void fetchSequential(const std::vector<std::string>& urls) {
    std::cout << "\n【循序獲取】" << std::endl;
    std::cout << "一次只處理一個請求，等待前一個完成後才開始下一個" << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    
    auto totalStart = std::chrono::steady_clock::now();
    
    std::vector<WebPage> results;
    for (const auto& url : urls) {
        std::cout << "正在獲取: " << url << "..." << std::flush;
        WebPage page = fetchWebPage(url);
        std::cout << " 完成 (" << page.fetchTimeMs << " ms)" << std::endl;
        results.push_back(page);
    }
    
    auto totalEnd = std::chrono::steady_clock::now();
    auto totalDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
        totalEnd - totalStart).count();
    
    std::cout << "----------------------------------------" << std::endl;
    std::cout << "循序獲取總時間: " << totalDuration << " 毫秒" << std::endl;
}

// 並行獲取多個網頁
void fetchParallel(const std::vector<std::string>& urls) {
    std::cout << "\n【並行獲取】" << std::endl;
    std::cout << "同時發送所有請求，並行等待回應" << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    
    auto totalStart = std::chrono::steady_clock::now();
    
    std::vector<std::thread> threads;
    std::vector<WebPage> results(urls.size());
    
    // 為每個 URL 建立一個執行緒
    for (size_t i = 0; i < urls.size(); ++i) {
        threads.emplace_back([i, &urls, &results]() {
            results[i] = fetchWebPage(urls[i]);
        });
    }
    
    // 等待所有執行緒完成
    for (auto& t : threads) {
        t.join();
    }
    
    auto totalEnd = std::chrono::steady_clock::now();
    auto totalDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
        totalEnd - totalStart).count();
    
    // 顯示每個請求的結果
    for (const auto& page : results) {
        std::cout << "獲取完成: " << page.url 
                  << " (" << page.fetchTimeMs << " ms)" << std::endl;
    }
    
    std::cout << "----------------------------------------" << std::endl;
    std::cout << "並行獲取總時間: " << totalDuration << " 毫秒" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "    I/O 密集型任務並行化示範" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // 模擬需要獲取的網頁列表
    std::vector<std::string> urls = {
        "https://api.example.com/users",
        "https://api.example.com/products",
        "https://api.example.com/orders",
        "https://api.example.com/reviews",
        "https://api.example.com/categories",
        "https://api.example.com/inventory",
        "https://api.example.com/shipping",
        "https://api.example.com/payments"
    };
    
    std::cout << "需要獲取 " << urls.size() << " 個網頁" << std::endl;
    
    // 循序獲取
    auto seqStart = std::chrono::steady_clock::now();
    fetchSequential(urls);
    auto seqEnd = std::chrono::steady_clock::now();
    auto seqDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
        seqEnd - seqStart).count();
    
    // 並行獲取
    auto parStart = std::chrono::steady_clock::now();
    fetchParallel(urls);
    auto parEnd = std::chrono::steady_clock::now();
    auto parDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
        parEnd - parStart).count();
    
    // 效能比較
    std::cout << "\n========================================" << std::endl;
    std::cout << "效能比較結果：" << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    std::cout << "循序獲取: " << seqDuration << " 毫秒" << std::endl;
    std::cout << "並行獲取: " << parDuration << " 毫秒" << std::endl;
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "加速比: " << static_cast<double>(seqDuration) / parDuration 
              << " 倍" << std::endl;
    std::cout << "========================================" << std::endl;
    
    std::cout << "\n【關鍵觀察】" << std::endl;
    std::cout << "• 循序執行：總時間 = 所有請求時間的總和" << std::endl;
    std::cout << "• 並行執行：總時間 ≈ 最慢那個請求的時間" << std::endl;
    std::cout << "• I/O 密集型任務特別適合並行化" << std::endl;
    std::cout << "• 即使在單核心 CPU 上也能獲得顯著加速" << std::endl;
    
    return 0;
}
```

#### 編譯與執行

```bash
g++ -std=c++17 -pthread -o lesson_1_2_io lesson_1_2_io_utilization.cpp
./lesson_1_2_io
```

#### 預期輸出

```
========================================
    I/O 密集型任務並行化示範
========================================
需要獲取 8 個網頁

【循序獲取】
一次只處理一個請求，等待前一個完成後才開始下一個
----------------------------------------
正在獲取: https://api.example.com/users... 完成 (342 ms)
正在獲取: https://api.example.com/products... 完成 (287 ms)
正在獲取: https://api.example.com/orders... 完成 (425 ms)
正在獲取: https://api.example.com/reviews... 完成 (312 ms)
正在獲取: https://api.example.com/categories... 完成 (256 ms)
正在獲取: https://api.example.com/inventory... 完成 (398 ms)
正在獲取: https://api.example.com/shipping... 完成 (223 ms)
正在獲取: https://api.example.com/payments... 完成 (467 ms)
----------------------------------------
循序獲取總時間: 2710 毫秒

【並行獲取】
同時發送所有請求，並行等待回應
----------------------------------------
獲取完成: https://api.example.com/users (342 ms)
獲取完成: https://api.example.com/products (287 ms)
獲取完成: https://api.example.com/orders (425 ms)
獲取完成: https://api.example.com/reviews (312 ms)
獲取完成: https://api.example.com/categories (256 ms)
獲取完成: https://api.example.com/inventory (398 ms)
獲取完成: https://api.example.com/shipping (223 ms)
獲取完成: https://api.example.com/payments (467 ms)
----------------------------------------
並行獲取總時間: 469 毫秒

========================================
效能比較結果：
----------------------------------------
循序獲取: 2710 毫秒
並行獲取: 469 毫秒
加速比: 5.78 倍
========================================

【關鍵觀察】
• 循序執行：總時間 = 所有請求時間的總和
• 並行執行：總時間 ≈ 最慢那個請求的時間
• I/O 密集型任務特別適合並行化
• 即使在單核心 CPU 上也能獲得顯著加速
```

---

### 五、運算密集型 vs I/O 密集型

理解這兩種任務類型的差異，對於決定是否使用多執行緒至關重要：

```
┌─────────────────────────────────────────────────────────────┐
│          運算密集型 vs I/O 密集型任務比較                     │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  運算密集型 (CPU-bound)          I/O 密集型 (I/O-bound)      │
│  ─────────────────────          ─────────────────────       │
│  • CPU 持續高負載               • CPU 大部分時間閒置          │
│  • 瓶頸在運算能力               • 瓶頸在等待外部資源          │
│  • 需要多核心才能加速            • 單核心也能加速              │
│  • 例如：加密、壓縮、渲染        • 例如：網路、磁碟、資料庫    │
│                                                             │
│  最佳執行緒數 ≈ CPU 核心數       最佳執行緒數 >> CPU 核心數    │
│                                                             │
│  ┌─────────────────┐            ┌─────────────────┐         │
│  │████████████████│            │██░░░░██░░░░██░░│         │
│  │  CPU 使用率高   │            │  大量等待時間    │         │
│  └─────────────────┘            └─────────────────┘         │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 六、什麼時候不該使用多執行緒

多執行緒並非萬能藥，在某些情況下反而會帶來負面效果：

#### 6.1 不適合多執行緒的情況

```
┌─────────────────────────────────────────────────────────────┐
│              何時避免使用多執行緒                             │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  1. 任務太小                                                 │
│     執行緒建立的開銷可能超過任務本身的執行時間                 │
│                                                             │
│  2. 強烈的資料相依性                                         │
│     如果每個步驟都依賴前一步驟的結果，無法並行化              │
│                                                             │
│  3. 大量的同步需求                                           │
│     頻繁的鎖競爭會抵消並行化的收益                           │
│                                                             │
│  4. 記憶體頻寬限制                                           │
│     當瓶頸在記憶體存取而非 CPU 運算時                        │
│                                                             │
│  5. 程式碼複雜度考量                                         │
│     多執行緒會增加程式的複雜度和維護難度                      │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

#### 6.2 阿姆達爾定律（Amdahl's Law）

這是評估並行化收益的重要理論：

```
                    1
加速比 = ─────────────────────────
         (1 - P) + P/N

P = 可並行化的程式比例
N = 處理器數量
```

**實例**：
- 如果程式有 50% 可以並行化（P = 0.5）
- 使用 4 個核心（N = 4）
- 最大加速比 = 1 / (0.5 + 0.5/4) = 1 / 0.625 = 1.6 倍

這意味著即使你有無限多的處理器，如果程式只有 50% 可以並行化，加速比也不會超過 2 倍！

---

### 七、本課重點回顧

1. **效能提升**：多執行緒可以利用多核心 CPU 加速運算密集型任務

2. **響應性改善**：將耗時操作放到背景執行緒，保持 UI 的響應性

3. **資源利用**：在 I/O 等待期間執行其他工作，提高整體效率

4. **運算密集型任務**：需要多核心才能真正加速

5. **I/O 密集型任務**：即使單核心也能從多執行緒受益

6. **慎重考量**：並非所有程式都適合多執行緒，需要權衡利弊

---

### 八、課後練習

1. **修改質數計算範例**：嘗試不同的執行緒數量（2、4、8、16），記錄加速比的變化

2. **分析你的程式**：思考你過去寫過的程式，哪些可能從多執行緒中受益？

3. **阿姆達爾定律計算**：如果一個程式有 80% 可以並行化，使用 8 核心 CPU 的理論最大加速比是多少？

4. **擴展 I/O 範例**：將網頁數量增加到 20 個，觀察並行版本的加速效果

---

### 下一課預告

在 **課程 1.3：執行緒 vs 程序** 中，我們將探討：
- 程序（Process）與執行緒（Thread）的本質區別
- 各自的優缺點與適用場景
- 記憶體空間的共享與隔離
- 程序間通訊 vs 執行緒間通訊

---

準備好繼續下一課了嗎？


這個問題需要先釐清你指的是哪種情況。讓我分別解釋：

---

## 你的問題可能是指...

### 情況 A：8 個 CPU 核心，執行 16 個 threads

這是**完全正常且常見**的情況！

```
┌─────────────────────────────────────────────────────────┐
│                    OS Scheduler                          │
│                                                          │
│   16 個 Threads 等待執行                                 │
│   ┌───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬...┐  │
│   │T1 │T2 │T3 │T4 │T5 │T6 │T7 │T8 │T9 │T10│T11│T12│   │  │
│   └───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴...┘  │
│                         │                                │
│                         ▼                                │
│   ┌─────────────────────────────────────────────────┐    │
│   │              時間片輪轉 (Time Slicing)           │    │
│   └─────────────────────────────────────────────────┘    │
│                         │                                │
│                         ▼                                │
│         8 個 CPU 核心 (同一時刻最多 8 個 threads)        │
│   ┌─────┐ ┌─────┐ ┌─────┐ ┌─────┐                        │
│   │CPU0 │ │CPU1 │ │CPU2 │ │CPU3 │  ...                   │
│   │ T1  │ │ T2  │ │ T3  │ │ T4  │                        │
│   └─────┘ └─────┘ └─────┘ └─────┘                        │
│                                                          │
└─────────────────────────────────────────────────────────┘
```

**發生什麼事？**
- 每個時刻只有 8 個 threads 真正在執行
- 其他 8 個 threads 在等待
- OS 透過 Context Switch 快速輪流執行
- 看起來像是 16 個 threads「同時」在跑

---

### 情況 B：`std::thread::hardware_concurrency()` 回傳 8，但你創建 16 個 threads

```cpp
#include <iostream>
#include <thread>
#include <vector>

int main() {
    // 查詢硬體支援的並行數
    unsigned int hw_threads = std::thread::hardware_concurrency();
    std::cout << "Hardware threads: " << hw_threads << "\n";  // 假設是 8
    
    // 但我們創建 16 個 threads
    std::vector<std::thread> threads;
    
    for (int i = 0; i < 16; i++) {
        threads.emplace_back([i]() {
            std::cout << "Thread " << i << " running\n";
            // 做一些工作...
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    return 0;
}
```

**結果：完全可以正常執行！** 不會報錯或崩潰。

---

## 時間片輪轉的實際運作

```
時間軸 (假設 8 核心, 16 threads)
──────────────────────────────────────────────────────────────────►

時間片 1 (例如 10ms)
┌──────┬──────┬──────┬──────┬──────┬──────┬──────┬──────┐
│ CPU0 │ CPU1 │ CPU2 │ CPU3 │ CPU4 │ CPU5 │ CPU6 │ CPU7 │
│  T1  │  T2  │  T3  │  T4  │  T5  │  T6  │  T7  │  T8  │
└──────┴──────┴──────┴──────┴──────┴──────┴──────┴──────┘
                     T9-T16 在等待佇列中
                              │
                              ▼ Context Switch
時間片 2
┌──────┬──────┬──────┬──────┬──────┬──────┬──────┬──────┐
│ CPU0 │ CPU1 │ CPU2 │ CPU3 │ CPU4 │ CPU5 │ CPU6 │ CPU7 │
│  T9  │ T10  │ T11  │ T12  │ T13  │ T14  │ T15  │ T16  │
└──────┴──────┴──────┴──────┴──────┴──────┴──────┴──────┘
                     T1-T8 在等待佇列中
                              │
                              ▼ Context Switch
時間片 3
┌──────┬──────┬──────┬──────┬──────┬──────┬──────┬──────┐
│ CPU0 │ CPU1 │ CPU2 │ CPU3 │ CPU4 │ CPU5 │ CPU6 │ CPU7 │
│  T1  │  T2  │  T3  │  T4  │  T5  │  T6  │  T7  │  T8  │
└──────┴──────┴──────┴──────┴──────┴──────┴──────┴──────┘
                              │
                              ▼
                            ...繼續輪轉
```

---

## 影響與考量

### 1. 效能影響

```
┌─────────────────────────────────────────────────────────────┐
│                    Threads vs 效能                          │
│                                                             │
│  效能 ▲                                                     │
│       │         ╭──────────╮                                │
│       │       ╱            ╲                                │
│       │     ╱                ╲                              │
│       │   ╱                    ╲───────────────             │
│       │ ╱                                                   │
│       │╱                                                    │
│       └──────────────────────────────────────────► Threads  │
│              │                  │                           │
│          最佳點              過多threads                    │
│        (≈ 核心數)           (效能下降)                      │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

### 2. 為什麼效能可能下降？

| 因素 | 說明 |
|------|------|
| **Context Switch 開銷** | 每次切換需要保存/恢復暫存器狀態 |
| **Cache 失效** | 新 thread 可能需要重新載入 cache |
| **記憶體開銷** | 每個 thread 都有自己的 stack (通常 1-8 MB) |
| **排程開銷** | OS 需要管理更多 threads |

### 3. 簡單程式碼驗證

```cpp
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>

void work() {
    volatile long long sum = 0;
    for (long long i = 0; i < 100000000; i++) {
        sum += i;
    }
}

int main() {
    unsigned int hw = std::thread::hardware_concurrency();
    std::cout << "Hardware threads: " << hw << "\n\n";
    
    // 測試不同數量的 threads
    for (int num_threads : {1, 4, 8, 16, 32, 64}) {
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<std::thread> threads;
        for (int i = 0; i < num_threads; i++) {
            threads.emplace_back(work);
        }
        for (auto& t : threads) {
            t.join();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << num_threads << " threads: " << duration.count() << " ms\n";
    }
    
    return 0;
}
```

**可能的輸出結果（8 核心機器）：**
```
Hardware threads: 8

1 threads: 250 ms
4 threads: 260 ms
8 threads: 270 ms      <-- 接近最佳
16 threads: 290 ms     <-- 開始有額外開銷
32 threads: 350 ms
64 threads: 500 ms     <-- 明顯變慢
```

---

## 何時該用超過核心數的 threads？

### 適合超過核心數的情況

```
┌────────────────────────────────────────────────────┐
│              I/O Bound 任務                        │
│                                                    │
│   Thread 1: ████░░░░░░░░░░████░░░░░░░░░░████       │
│   Thread 2: ░░░░████░░░░░░░░░░████░░░░░░░░░░       │
│   Thread 3: ░░░░░░░░████░░░░░░░░░░████░░░░░       │
│                                                    │
│   ████ = CPU 工作    ░░░░ = 等待 I/O               │
│                                                    │
│   當一個 thread 等待 I/O 時，其他 thread 可以      │
│   使用 CPU，所以可以用更多 threads                 │
└────────────────────────────────────────────────────┘
```

### 不適合超過核心數的情況

```
┌────────────────────────────────────────────────────┐
│              CPU Bound 任務                        │
│                                                    │
│   Thread 1: ████████████████████████████████       │
│   Thread 2: ████████████████████████████████       │
│                                                    │
│   ████ = CPU 工作 (100% CPU)                       │
│                                                    │
│   每個 thread 都持續使用 CPU，                     │
│   超過核心數只會增加 Context Switch 開銷          │
└────────────────────────────────────────────────────┘
```

---

## 總結

| 問題 | 答案 |
|------|------|
| 8 核心跑 16 threads 可以嗎？ | **可以，完全正常** |
| 會報錯嗎？ | **不會** |
| 效能如何？ | **取決於任務類型** |
| CPU-bound 任務 | threads ≈ 核心數 最佳 |
| I/O-bound 任務 | 可以用更多 threads |
| 限制在哪？ | 主要是記憶體和 OS 限制 |

你想更深入了解哪個方面？比如：
- 如何決定最佳 thread 數量？
- Thread Pool 的設計？
- I/O-bound vs CPU-bound 的詳細比較？
*/


// 檔案名稱：lesson_1_2_io_utilization.cpp
// 課程：1.2 - 為什麼需要多執行緒
// 說明：展示多執行緒如何提升 I/O 密集型任務的效率

#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <string>
#include <iomanip>

// 模擬一個 I/O 密集型操作（例如：HTTP 請求或檔案讀取）
struct WebPage {
    std::string url;
    std::string content;
    int statusCode;
    long long fetchTimeMs;
};

// 模擬從網路獲取網頁內容
WebPage fetchWebPage(const std::string& url) {
    auto start = std::chrono::steady_clock::now();
    
    // 模擬網路延遲（200-500 毫秒的隨機延遲）
    int delay = 200 + (std::hash<std::string>{}(url) % 300);
    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end - start).count();
    
    return WebPage{
        url,
        "Content of " + url,
        200,
        duration
    };
}

// 循序獲取多個網頁
void fetchSequential(const std::vector<std::string>& urls) {
    std::cout << "\n【循序獲取】" << std::endl;
    std::cout << "一次只處理一個請求，等待前一個完成後才開始下一個" << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    
    auto totalStart = std::chrono::steady_clock::now();
    
    std::vector<WebPage> results;
    for (const auto& url : urls) {
        std::cout << "正在獲取: " << url << "..." << std::flush;
        WebPage page = fetchWebPage(url);
        std::cout << " 完成 (" << page.fetchTimeMs << " ms)" << std::endl;
        results.push_back(page);
    }
    
    auto totalEnd = std::chrono::steady_clock::now();
    auto totalDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
        totalEnd - totalStart).count();
    
    std::cout << "----------------------------------------" << std::endl;
    std::cout << "循序獲取總時間: " << totalDuration << " 毫秒" << std::endl;
}

// 並行獲取多個網頁
void fetchParallel(const std::vector<std::string>& urls) {
    std::cout << "\n【並行獲取】" << std::endl;
    std::cout << "同時發送所有請求，並行等待回應" << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    
    auto totalStart = std::chrono::steady_clock::now();
    
    std::vector<std::thread> threads;
    std::vector<WebPage> results(urls.size());
    
    // 為每個 URL 建立一個執行緒
    for (size_t i = 0; i < urls.size(); ++i) {
        threads.emplace_back([i, &urls, &results]() {
            results[i] = fetchWebPage(urls[i]);
        });
    }
    
    // 等待所有執行緒完成
    for (auto& t : threads) {
        t.join();
    }
    
    auto totalEnd = std::chrono::steady_clock::now();
    auto totalDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
        totalEnd - totalStart).count();
    
    // 顯示每個請求的結果
    for (const auto& page : results) {
        std::cout << "獲取完成: " << page.url 
                  << " (" << page.fetchTimeMs << " ms)" << std::endl;
    }
    
    std::cout << "----------------------------------------" << std::endl;
    std::cout << "並行獲取總時間: " << totalDuration << " 毫秒" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "    I/O 密集型任務並行化示範" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // 模擬需要獲取的網頁列表
    std::vector<std::string> urls = {
        "https://api.example.com/users",
        "https://api.example.com/products",
        "https://api.example.com/orders",
        "https://api.example.com/reviews",
        "https://api.example.com/categories",
        "https://api.example.com/inventory",
        "https://api.example.com/shipping",
        "https://api.example.com/payments"
    };
    
    std::cout << "需要獲取 " << urls.size() << " 個網頁" << std::endl;
    
    // 循序獲取
    auto seqStart = std::chrono::steady_clock::now();
    fetchSequential(urls);
    auto seqEnd = std::chrono::steady_clock::now();
    auto seqDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
        seqEnd - seqStart).count();
    
    // 並行獲取
    auto parStart = std::chrono::steady_clock::now();
    fetchParallel(urls);
    auto parEnd = std::chrono::steady_clock::now();
    auto parDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
        parEnd - parStart).count();
    
    // 效能比較
    std::cout << "\n========================================" << std::endl;
    std::cout << "效能比較結果：" << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    std::cout << "循序獲取: " << seqDuration << " 毫秒" << std::endl;
    std::cout << "並行獲取: " << parDuration << " 毫秒" << std::endl;
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "加速比: " << static_cast<double>(seqDuration) / parDuration 
              << " 倍" << std::endl;
    std::cout << "========================================" << std::endl;
    
    std::cout << "\n【關鍵觀察】" << std::endl;
    std::cout << "• 循序執行：總時間 = 所有請求時間的總和" << std::endl;
    std::cout << "• 並行執行：總時間 ≈ 最慢那個請求的時間" << std::endl;
    std::cout << "• I/O 密集型任務特別適合並行化" << std::endl;
    std::cout << "• 即使在單核心 CPU 上也能獲得顯著加速" << std::endl;
    
    return 0;
}
