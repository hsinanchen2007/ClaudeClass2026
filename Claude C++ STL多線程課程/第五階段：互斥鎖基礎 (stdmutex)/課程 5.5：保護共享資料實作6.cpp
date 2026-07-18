// 檔案：lesson_5_5_thread_safe_logger.cpp
// 說明：執行緒安全的日誌記錄器

#include <iostream>
#include <thread>
#include <mutex>
#include <string>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <vector>
#include <fstream>

class ThreadSafeLogger {
public:
    enum class Level {
        DEBUG,
        INFO,
        WARNING,
        ERROR
    };
    
private:
    mutable std::mutex mtx;
    std::ostream& output;
    Level minLevel;
    bool includeTimestamp;
    bool includeThreadId;
    
    std::string levelToString(Level level) const {
        switch (level) {
            case Level::DEBUG:   return "DEBUG";
            case Level::INFO:    return "INFO ";
            case Level::WARNING: return "WARN ";
            case Level::ERROR:   return "ERROR";
            default:             return "?????";
        }
    }
    
    std::string getTimestamp() const {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        std::ostringstream oss;
        oss << std::put_time(std::localtime(&time), "%H:%M:%S");
        oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
        return oss.str();
    }
    
public:
    ThreadSafeLogger(std::ostream& out = std::cout, 
                     Level level = Level::DEBUG,
                     bool timestamp = true,
                     bool threadId = true)
        : output(out)
        , minLevel(level)
        , includeTimestamp(timestamp)
        , includeThreadId(threadId) {}
    
    // 禁止複製
    ThreadSafeLogger(const ThreadSafeLogger&) = delete;
    ThreadSafeLogger& operator=(const ThreadSafeLogger&) = delete;
    
    void setLevel(Level level) {
        std::lock_guard<std::mutex> lock(mtx);
        minLevel = level;
    }
    
    void log(Level level, const std::string& message) {
        if (level < minLevel) {
            return;  // 過濾低級別日誌
        }
        
        // 在鎖外準備訊息（減少臨界區段）
        std::ostringstream oss;
        
        if (includeTimestamp) {
            oss << "[" << getTimestamp() << "] ";
        }
        
        oss << "[" << levelToString(level) << "] ";
        
        if (includeThreadId) {
            oss << "[T:" << std::this_thread::get_id() << "] ";
        }
        
        oss << message << std::endl;
        
        std::string formatted = oss.str();
        
        // 只在輸出時鎖定
        std::lock_guard<std::mutex> lock(mtx);
        output << formatted;
        output.flush();
    }
    
    void debug(const std::string& msg)   { log(Level::DEBUG, msg); }
    void info(const std::string& msg)    { log(Level::INFO, msg); }
    void warning(const std::string& msg) { log(Level::WARNING, msg); }
    void error(const std::string& msg)   { log(Level::ERROR, msg); }
};

// 全域 Logger（實際專案中可能使用單例模式）
ThreadSafeLogger logger;

void worker(int id) {
    logger.info("Worker " + std::to_string(id) + " 開始");
    
    for (int i = 0; i < 3; ++i) {
        logger.debug("Worker " + std::to_string(id) + 
                     " 執行任務 " + std::to_string(i));
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    logger.info("Worker " + std::to_string(id) + " 結束");
}

int main() {
    logger.info("=== 程式開始 ===");
    
    std::vector<std::thread> threads;
    
    for (int i = 1; i <= 3; ++i) {
        threads.emplace_back(worker, i);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    logger.info("=== 程式結束 ===");
    
    return 0;
}
