// =============================================================================
//  07_stringstream.cpp  —  istringstream / ostringstream 字串流
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/io/basic_stringstream
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、為什麼要 stringstream？                                │
//  └────────────────────────────────────────────────────────────┘
//
//  把「字串當作 stream 來讀／寫」 — 既能用 iostream 的格式化機制，又不必
//  真的開檔案或 stdin/stdout。
//
//  典型場景：
//   * 解析「以空白／逗號分隔的字串」 → istringstream + while(in >> x)
//   * 把多個資料拼成一個格式化字串 → ostringstream
//   * 數字 ↔ 字串轉換（雖然 to_string / from_chars 更快，但 stringstream
//     對「複雜型別 + 格式化」更靈活）
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、三個 class                                             │
//  └────────────────────────────────────────────────────────────┘
//
//      std::istringstream   只讀
//      std::ostringstream   只寫
//      std::stringstream    讀寫
//
//  常用 API：
//      .str()       取得 / 設定底下的 string
//      << / >>      跟一般 stream 一樣
//      .clear()     清掉 fail bit
//      .seekg/seekp  移動讀/寫位置
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、本檔示範                                               │
//  └────────────────────────────────────────────────────────────┘
//
//   * Demo 1：用 ostringstream 組合複雜字串
//   * Demo 2：用 istringstream 解析空白分隔
//   * Demo 3：解析 CSV-like（逗號分隔）— getline + delim
//   * Demo 4：把整數陣列「join」成 "1,2,3"
//   * Demo 5：split 字串成 token（簡易 tokenizer）
// =============================================================================

/*
補充筆記：std::stringstream
  - std::stringstream 屬於 iostream；stream 同時保存資料流位置、格式設定和錯誤狀態。
  - operator>> 會跳過空白並遇空白停止，getline 會讀到換行；混用時常需要先處理殘留 newline。
  - failbit、badbit、eofbit 代表不同狀態；讀取失敗後要 clear() 才能繼續使用 stream。
  - fstream 開檔模式要明確：in/out/app/binary/trunc 組合不同會影響是否覆蓋、追加或以二進位處理。
  - sync_with_stdio(false) 和 cin.tie(nullptr) 可提升競賽輸入速度，但混用 C stdio 和 iostream 時要小心順序。
  - stringstream 適合字串解析與格式化，但大量資料轉換時可考慮 charconv 降低 locale 和配置成本。
  - stringstream 把字串當 stream，可重用 operator>> 的解析規則。
  - 解析失敗後要 clear() 才能繼續使用同一個 stringstream。
  - 大量數字轉換若在效能熱點，可考慮 std::from_chars/to_chars 避免 locale 和配置成本。
*/
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// ─── helper：用 stringstream 把容器 join 成 "a,b,c" ───
template <class It>
static std::string join(It first, It last, const std::string& sep) {
    std::ostringstream os;
    bool firstItem = true;
    for (auto it = first; it != last; ++it) {
        if (!firstItem) os << sep;
        os << *it;
        firstItem = false;
    }
    return os.str();
}

// ─── helper：用 getline 把 "a,b,c" 切成 vector<string> ───
static std::vector<std::string> split(const std::string& s, char delim) {
    std::vector<std::string> out;
    std::istringstream is{s};
    std::string token;
    while (std::getline(is, token, delim)) {
        out.push_back(token);
    }
    return out;
}

int main() {
    // ─────────────────────────────────────────────────────────
    // Demo 1：ostringstream 組字串（含 manipulator）
    // ─────────────────────────────────────────────────────────
    std::ostringstream os;
    os << "user=" << "alice"
       << ", id=" << std::setw(5) << std::setfill('0') << 42
       << ", score=" << std::fixed << std::setprecision(2) << 91.5;
    std::cout << "[Demo1] " << os.str() << '\n';

    // ─────────────────────────────────────────────────────────
    // Demo 2：istringstream 解析空白分隔
    // ─────────────────────────────────────────────────────────
    std::istringstream is{"alice 30 91.5"};
    std::string name;
    int age;
    double score;
    is >> name >> age >> score;
    std::cout << "[Demo2] name=" << name
              << " age=" << age
              << " score=" << score << '\n';

    // ─────────────────────────────────────────────────────────
    // Demo 3：CSV-like 解析 — getline 用 delim 取代換行
    // ─────────────────────────────────────────────────────────
    auto fields = split("alice,30,Taipei,Engineer", ',');
    std::cout << "[Demo3]";
    for (auto& f : fields) std::cout << " [" << f << "]";
    std::cout << '\n';

    // ─────────────────────────────────────────────────────────
    // Demo 4：整數 join
    // ─────────────────────────────────────────────────────────
    std::vector<int> nums{1, 2, 3, 4, 5};
    std::cout << "[Demo4] " << join(nums.begin(), nums.end(), ", ") << '\n';

    // ─────────────────────────────────────────────────────────
    // Demo 5：用 stringstream 重複 use 一個 buffer
    //   重置：os.str("") + os.clear()
    // ─────────────────────────────────────────────────────────
    std::ostringstream buf;
    for (int i = 1; i <= 3; ++i) {
        buf.str("");                // 清空底層字串
        buf.clear();                // 清掉 fail bit（保險）
        buf << "iteration-" << i;
        std::cout << "[Demo5] " << buf.str() << '\n';
    }

    // ─────────────────────────────────────────────────────────
    // 實用範例 A：把 vector<int> 轉成「總和 / 平均 / 最大 / 最小」一行報告
    //   工作上常見：要 print 一段資料的統計摘要。ostringstream 攢起來再
    //   一次輸出，比多次 cout 友善（測試也好斷言整段字串）。
    // ─────────────────────────────────────────────────────────
    {
        std::vector<int> data{4, 8, 15, 16, 23, 42};
        int mn = data.front(), mx = data.front();
        long long sum = 0;
        for (int x : data) {
            sum += x;
            if (x < mn) mn = x;
            if (x > mx) mx = x;
        }
        double avg = static_cast<double>(sum) / static_cast<int>(data.size());

        std::ostringstream report;
        report << std::fixed << std::setprecision(2)
               << "n=" << data.size()
               << " sum=" << sum
               << " avg=" << avg
               << " min=" << mn
               << " max=" << mx;
        std::cout << "[report] " << report.str() << '\n';
    }

    // ─────────────────────────────────────────────────────────
    // 實用範例 B：parse "key1=v1; key2=v2; key3=v3" 形式的字串
    //   工作上常見：HTTP Cookie / Header 內容、簡單序列化的設定字串。
    // ─────────────────────────────────────────────────────────
    {
        std::string raw = "user=alice; sid=ABC123; ttl=3600";
        std::istringstream is{raw};
        std::string item;
        std::cout << "[parse-pairs]\n";
        while (std::getline(is, item, ';')) {
            // 去掉前後空白（簡易版）
            size_t start = item.find_first_not_of(' ');
            if (start == std::string::npos) continue;
            item = item.substr(start);

            auto eq = item.find('=');
            if (eq == std::string::npos) continue;
            std::cout << "  [" << item.substr(0, eq)
                      << "] = [" << item.substr(eq + 1) << "]\n";
        }
    }

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：std::to_string vs ostringstream？
    //    A：to_string 簡單、快；但只支援基本數值、無格式化。複雜格式化
    //       (寬度、進制、精度組合) 還是 ostringstream 比較靈活。
    //       C++17 std::to_chars 是更快的選項，但 API 較底層。
    //
    //  Q2：怎麼把 stringstream 「reset」？
    //    A：os.str("") 把字串清空；os.clear() 清掉 io state。
    //       兩個都要做才完全乾淨。
    //
    //  Q3：getline + delim 跟 split 為什麼夠用？
    //    A：getline(is, s, ',') 讀到逗號為止 → 一個 token；
    //       這樣寫出來的 split 對單字元 delim 非常乾淨。多字元 delim
    //       要自己 substr / find，或用 std::regex。
    //
    return 0;
}
