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
