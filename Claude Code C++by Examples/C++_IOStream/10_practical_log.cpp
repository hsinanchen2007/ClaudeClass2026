// =============================================================================
//  10_practical_log.cpp  —  工作小工具：簡單 logger
// =============================================================================
//  參考：
//    - https://en.cppreference.com/w/cpp/io/basic_ofstream
//    - https://en.cppreference.com/w/cpp/io/basic_ostringstream
//    - https://en.cppreference.com/w/cpp/io/manip/put_time
//    - https://en.cppreference.com/w/cpp/thread/mutex
//
//  把前面學到的東西實際組合：
//   * std::ostream& 寫格式化輸出
//   * stringstream 預先組合 message
//   * fstream 寫檔
//   * manipulators 對齊 / 時間戳
//   * 多個 sink（cout + file）
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 設計目標                                                   │
//  └────────────────────────────────────────────────────────────┘
//
//  Logger 應該：
//   1) 對使用者像「stream」 — 直接 logger << "x=" << x; 寫入
//   2) 加上等級（INFO/WARN/ERROR）跟時間戳
//   3) 同時輸出到「stdout 與 file」（fan-out 寫入）
//   4) thread-safe（本範例用 mutex 簡單包，避免細節糾結）
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 關鍵技巧：用 RAII 物件「在析構時 flush 一行」              │
//  └────────────────────────────────────────────────────────────┘
//
//  常見 logger 模式：
//
//      LOG(INFO) << "x=" << x << " y=" << y;
//
//  LOG(...) 回傳一個臨時 RAII 物件，operator<< 寫進它的 ostringstream；
//  本行結束時臨時物件析構，把 buffer 一次 flush 到 sink，加 prefix（時間
//  + 等級）+ 加結尾換行。
//
//  好處：
//   * 「一條 log = 一行輸出」自動保證
//   * 多 thread 下用 mutex 包住「flush」這一步即可，不必每個 << 都鎖
//
//  本檔提供一個極簡實作。
// =============================================================================

/*
補充筆記：std::practical_log
  - std::practical_log 屬於 iostream；stream 同時保存資料流位置、格式設定和錯誤狀態。
  - operator>> 會跳過空白並遇空白停止，getline 會讀到換行；混用時常需要先處理殘留 newline。
  - failbit、badbit、eofbit 代表不同狀態；讀取失敗後要 clear() 才能繼續使用 stream。
  - fstream 開檔模式要明確：in/out/app/binary/trunc 組合不同會影響是否覆蓋、追加或以二進位處理。
  - sync_with_stdio(false) 和 cin.tie(nullptr) 可提升競賽輸入速度，但混用 C stdio 和 iostream 時要小心順序。
  - stringstream 適合字串解析與格式化，但大量資料轉換時可考慮 charconv 降低 locale 和配置成本。
  - log 輸出應考慮時間戳、嚴重等級、thread id 和是否需要立即 flush。
  - 多執行緒寫同一個 stream 需要同步，否則多行 log 可能交錯。
  - 正式 logger 通常把格式化和寫入目的地分開，方便改成檔案、console 或 ring buffer。
*/
#include <chrono>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>

enum class Level { Info, Warn, Error };

static const char* levelStr(Level l) {
    switch (l) {
        case Level::Info:  return "INFO ";
        case Level::Warn:  return "WARN ";
        case Level::Error: return "ERROR";
    }
    return "?    ";
}

class Logger {
public:
    explicit Logger(const std::string& filePath)
        : file_(filePath, std::ios::app) {}

    // 提供一個 RAII 「行 logger」 — 析構時 flush 整行
    class Line {
    public:
        Line(Logger& l, Level lv) : owner_(l), level_(lv) {}
        ~Line() { owner_.flush(level_, oss_.str()); }

        // 鏈式 operator<<：什麼都吃，丟到內部的 stringstream
        template <class T>
        Line& operator<<(const T& v) { oss_ << v; return *this; }

    private:
        Logger& owner_;
        Level level_;
        std::ostringstream oss_;
    };

    Line info()  { return Line{*this, Level::Info}; }
    Line warn()  { return Line{*this, Level::Warn}; }
    Line error() { return Line{*this, Level::Error}; }

private:
    void flush(Level lv, const std::string& msg) {
        std::lock_guard<std::mutex> g{mtx_};

        // 取現在時間 → "YYYY-MM-DD HH:MM:SS"
        auto now = std::chrono::system_clock::now();
        std::time_t tt = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};
#if defined(_WIN32)
        localtime_s(&tm, &tt);
#else
        localtime_r(&tt, &tm);
#endif
        // 組 prefix 一次（不要 << 多次省得 thread 切換中插隊）
        std::ostringstream prefix;
        prefix << '[' << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "] "
               << '[' << levelStr(lv) << "] ";
        std::string full = prefix.str() + msg + '\n';

        // fan-out 兩個 sink
        std::cout << full;
        if (file_) file_ << full;
    }

    std::ofstream file_;
    std::mutex    mtx_;
};

int main() {
    Logger logger{"tmp_app.log"};

    int x = 42;
    double pi = 3.14;

    logger.info()  << "starting up, x=" << x;
    logger.warn()  << "queue depth=" << 8 << " (>5)";
    logger.error() << "could not parse value=" << pi;

    // 用 stream-like 寫法可以混型別、含 manipulator
    logger.info() << "hex(255)=" << std::hex << 255
                  << " precision: " << std::fixed << std::setprecision(2) << 1.0/3.0;

    std::cout << "[main] log saved to tmp_app.log\n";

    // ─────────────────────────────────────────────────────────
    // 實用範例 A：簡易 log rotation — 檔案超過 N bytes 就改名 + 重開
    //   工作上 log 不可以無限大；這裡示範最簡單的 rotation 概念。
    //   實務生產用 spdlog 之類的成熟函式庫，本範例只展示概念。
    // ─────────────────────────────────────────────────────────
    {
        const std::string path = "tmp_rot.log";
        const std::streampos LIMIT = 80;        // 故意設小，方便 demo
        // 假裝寫了幾行
        {
            std::ofstream out{path, std::ios::trunc};
            out << "first line of log\n";
            out << "second line of log\n";
            out << "third line of log\n";
        }
        // 檢查大小、超過就 rotate
        std::ifstream check{path, std::ios::ate};   // 開檔就跳到尾
        auto sz = check.tellg();
        check.close();
        if (sz > LIMIT) {
            std::rename(path.c_str(), (path + ".1").c_str());
            std::ofstream fresh{path, std::ios::trunc};
            fresh << "(rotated)\n";
            std::cout << "[rotate] rotated: old size=" << sz << " > "
                      << LIMIT << " bytes\n";
        } else {
            std::cout << "[rotate] no rotate, size=" << sz << '\n';
        }
    }

    // ─────────────────────────────────────────────────────────
    // 實用範例 B：用 ostringstream 預組訊息 — 多 thread 下避免交錯
    //   重點：「攢成一塊 string 再一次寫」是 thread-safe-ish 的基本作法。
    //   配合 C++20 std::osyncstream 可達到真正的 atomic 行級輸出。
    // ─────────────────────────────────────────────────────────
    {
        auto threadLikeWrite = [](int tid, int n) {
            std::ostringstream line;
            line << "[thread " << tid << "] iteration " << n
                 << " value=" << (tid * 100 + n);
            // 一次寫到 cout（雖然這裡是單 thread，但姿勢一致）
            std::cout << line.str() << '\n';
        };
        for (int t = 1; t <= 2; ++t)
            for (int n = 1; n <= 2; ++n)
                threadLikeWrite(t, n);
    }

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：為什麼 RAII 行物件比每行都呼叫 logger.write 好？
    //    A：使用者語法跟 stream 一致 (LOG << "..." << x)；中間 << 操作不用
    //       鎖，只有最後 flush 一次取 mutex，鎖時間最短。
    //
    //  Q2：要支援 file rotation 怎麼做？
    //    A：在 flush 內檢查 file size / 日期，超過上限就關閉、改名、重開。
    //       工業級 logger（spdlog）有現成 rotating sink、async sink 可選。
    //
    //  Q3：thread 之間 cout 會交錯嗎？
    //    A：在 sync_with_stdio(true) 下，operator<< 一次呼叫對應一次寫，但
    //       「跨多次 <<」中間沒鎖會交錯。我們把 prefix + msg 拼成一個字串
    //       再 cout << full 一次寫，能大幅降低交錯機率（不是百分百）。
    //       要絕對隔離可用 std::osyncstream（C++20）。
    //
    return 0;
}
