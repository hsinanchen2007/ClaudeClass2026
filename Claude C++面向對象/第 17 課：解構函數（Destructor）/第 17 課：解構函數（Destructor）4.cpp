// =============================================================================
//  第 17 課：解構函數 4  —  用解構函數管理檔案（RAII 的經典應用）
// =============================================================================
//
// 【主題資訊 Information】
//   主題：在建構函數開檔、在解構函數關檔，讓「檔案一定會被關閉」
//   標準版本：<fstream> 為 C++98；std::filesystem 為 C++17
//   複雜度：O(1)（開關檔本身），寫入為 O(資料量)
//   標頭檔：<fstream>、<string>、<cstdio>（本檔用 std::remove 清理測試檔）
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼「忘記關檔」比「忘記釋放記憶體」更麻煩】
//   記憶體洩漏至少只是浪費空間；沒關檔則可能造成**資料遺失**：
//     ● 寫入的內容多半停留在使用者空間的緩衝區，尚未真正落到磁碟
//     ● 檔案描述元（file descriptor）是有限資源，用完會導致無法再開檔
//     ● 在 Windows 上，未關閉的檔案還可能被鎖住，別的程式打不開
//   RAII 把關檔綁在解構函數上，就把「記得關」變成語言保證的事。
//
// 【2. 本檔其實示範了兩層 RAII】
//   ● 外層：FileWriter 自己在解構函數裡呼叫 file.close()
//   ● 內層：成員 std::ofstream **本身就是 RAII 型別**，
//     它的解構函數也會自動關檔
//   所以嚴格說，就算 FileWriter 不寫解構函數，檔案一樣會被關閉。
//   那為什麼還要寫？兩個理由：
//     (a) 教學上要看到「解構函數被呼叫」這件事
//     (b) 實務上常需要在關檔前後做額外的事（記錄、檢查寫入是否成功、
//         把暫存檔改名成正式檔）
//   如果**只是要關檔**，那就不要寫解構函數——這正是 Rule of Zero。
//
// 【3. 建構函數失敗了怎麼辦（本檔刻意示範）】
//   開檔可能失敗（路徑不存在、沒有權限、磁碟滿）。
//   本檔的處理方式是：記錄失敗、讓物件進入「無效狀態」，
//   並提供 isOpen() 讓呼叫端查詢。
//   另一種常見設計是在建構函數**拋出例外**，讓物件根本無法被建立出來——
//   這樣就不會有「半殘物件」。兩種都合理，重點是**不要沉默地失敗**。
//   注意：若建構函數拋出例外，該物件的解構函數**不會**被呼叫
//   （物件從未建構成功），但已初始化完成的成員仍會被正常解構。
//
// 【4. 解構函數裡不可以讓例外逃出去】
//   C++11 起解構函數預設為 noexcept，例外逃出去會直接 std::terminate。
//   但關檔是**可能失敗**的操作（例如 flush 時磁碟滿）。
//   所以正確的設計是：
//     ● 解構函數只做「盡力而為」的清理，失敗就記錄，絕不拋出
//     ● 若呼叫端需要知道結果，另外提供明確的 close() 讓它在正常流程中呼叫
//   本檔的解構函數就只印訊息、不拋例外。
//
// 【5. 為什麼這個模式在例外情境下特別重要】
//   若寫檔過程中拋出例外，堆疊展開（stack unwinding）會逐一解構
//   作用域內已建構完成的物件——FileWriter 的解構函數會被呼叫，檔案被關閉。
//   手動 close() 則會被例外直接跳過。這是 RAII 相對於手動管理最大的優勢。
//
// 【概念補充 Concept Deep Dive】
//
//   ● close() 與 flush() 的差別
//     flush() 把使用者空間緩衝區的內容交給作業系統；
//     close() 會先 flush 再釋放檔案描述元。
//     但要注意：**兩者都不保證資料已經實際寫入實體磁碟**，
//     作業系統可能還壓在它自己的快取裡。要求真正落盤需要
//     fsync（POSIX）或 FlushFileBuffers（Windows）這類平台專屬呼叫，
//     C++ 標準函式庫沒有提供。
//
//   ● ofstream 的狀態旗標
//     開檔失敗時 is_open() 回傳 false，同時 failbit 會被設定。
//     可以用 file.good()／file.fail() 檢查更細的狀態。
//     預設情況下 ofstream 不會因為錯誤而丟例外，除非你呼叫
//     file.exceptions(...) 明確要求。
//
//   ● 本檔會在目前工作目錄產生檔案
//     為了不留下垃圾，main 最後會用 std::remove 把測試檔刪掉，
//     所以重複執行的輸出是一致的。實務上處理暫存檔更嚴謹的做法是
//     使用 std::filesystem::temp_directory_path()（C++17）。
//
//   ● 為什麼成員宣告順序是 filename 在前、file 在後
//     這樣建構函數的初始化列表可以先設好 filename，
//     訊息輸出與後續使用都比較自然（呼應第 16 課的順序規則）。
//
// 【注意事項 Pay Attention】
//   1. 解構函數中絕不可讓例外逃出（C++11 起預設 noexcept）。
//   2. 需要知道關檔是否成功時，另外提供明確的 close()，別依賴解構函數回報。
//   3. close() 不保證資料已落到實體磁碟；那需要平台專屬的 fsync 類呼叫。
//   4. 若只是要關檔，成員 ofstream 已經會自己處理——不必寫解構函數。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】RAII 與檔案資源
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼用 RAII 管理檔案，比在函數結尾呼叫 close() 可靠？
//     答：因為若中途 return 或拋出例外，手動的 close() 會被跳過；
//         而區域物件的解構函數在堆疊展開時**保證**會被呼叫。
//         把 close 放進解構函數，等於讓語言替你保證每條離開路徑都會關檔。
//     追問：那 std::ofstream 本身需要包一層嗎？
//         → 通常不需要，它自己就是 RAII 型別。只有在關檔前後還要做別的事
//           （記錄、驗證、暫存檔改名）時才值得包裝。
//
// 🔥 Q2. 解構函數裡可以拋出例外嗎？如果清理動作本身可能失敗怎麼辦？
//     答：不可以。C++11 起解構函數預設 noexcept，例外逃出會直接
//         std::terminate。若清理可能失敗，解構函數應該只做「盡力而為」
//         的處理並記錄錯誤；同時另外提供明確的 close()／commit()，
//         讓呼叫端在正常流程中處理錯誤。
//     追問：為什麼標準要這樣規定？
//         → 因為解構可能發生在堆疊展開的過程中。若此時又拋出第二個例外，
//           語言無法決定該傳播哪一個，所以直接終止程式。
//
// ⚠️ 陷阱. 檔案 close() 成功回來了，資料就一定安全寫進磁碟了吧？
//     答：不一定。close() 只保證把使用者空間的緩衝區交給作業系統並釋放
//         描述元，資料可能仍在作業系統的快取中。若此時斷電，仍可能遺失。
//         要求真正落盤必須用 fsync（POSIX）或 FlushFileBuffers（Windows），
//         這些不在 C++ 標準函式庫內。
//     為什麼會錯：把「函式回傳成功」等同於「持久化完成」。
//         實際上寫入路徑有多層緩衝（C++ 串流緩衝 → 作業系統頁快取 →
//         磁碟自身快取），每一層都要明確要求才會往下推。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <fstream>
#include <string>
#include <cstdio>     // std::remove
using namespace std;

class FileWriter {
private:
    string filename;    // 宣告在前，訊息輸出較自然
    ofstream file;
    bool ok = false;    // NSDMI：預設視為未開啟成功

public:
    explicit FileWriter(const string& fname) : filename(fname) {
        file.open(filename);
        ok = file.is_open();
        if (ok) {
            cout << "  [建構] 打開檔案: " << filename << endl;
        } else {
            // 不要沉默失敗：明確回報，並讓物件進入可查詢的無效狀態
            cout << "  [建構] 無法打開檔案: " << filename << endl;
        }
    }

    // 解構函數：盡力而為的清理，絕不拋出例外
    ~FileWriter() {
        if (file.is_open()) {
            file.close();    // 自動關閉檔案
            cout << "  [解構] 關閉檔案: " << filename << endl;
        }
    }

    bool isOpen() const { return ok; }

    void writeLine(const string& text) {
        if (file.is_open()) {
            file << text << endl;
            cout << "  寫入: " << text << endl;
        }
    }
};

// -----------------------------------------------------------------------------
// 示範：拋出例外時，解構函數仍然會被呼叫（堆疊展開）
//   手寫的 close() 會被例外跳過，RAII 則不會。
// -----------------------------------------------------------------------------
static void writeThenThrow(const string& fname) {
    FileWriter w(fname);
    w.writeLine("例外發生前寫入的內容");
    cout << "  即將拋出例外……" << endl;
    throw runtime_error("模擬處理過程中發生錯誤");
    // 這行以下不會執行，但 w 的解構函數仍會在堆疊展開時被呼叫
}

// -----------------------------------------------------------------------------
// 【日常實務範例】原子性寫檔：先寫暫存檔，成功才改名成正式檔
//   情境：更新設定檔或資料檔時，如果直接覆寫原檔，一旦寫到一半程式當掉，
//         就會留下一個「壞掉的半成品」，連原本正確的內容都沒了。
//   做法：先寫到 <目標>.tmp，全部成功後才 rename 成正式檔名。
//         rename 在同一個檔案系統上通常是原子操作，
//         所以讀取端看到的要嘛是舊檔、要嘛是完整的新檔，不會是半成品。
//   RAII 的角色：commit() 未被呼叫時（例如中途 return 或拋例外），
//         解構函數負責把暫存檔刪掉，不留垃圾。
// -----------------------------------------------------------------------------
class AtomicFileWriter {
private:
    string target_;
    string tmp_;
    ofstream out_;
    bool committed_ = false;

public:
    explicit AtomicFileWriter(const string& target)
        : target_(target), tmp_(target + ".tmp")
    {
        out_.open(tmp_);
        cout << "    開啟暫存檔: " << tmp_ << endl;
    }

    void write(const string& line) {
        if (out_.is_open()) out_ << line << "\n";
    }

    // 明確的提交點：呼叫端在正常流程中決定「這次寫入算數」
    bool commit() {
        if (!out_.is_open()) return false;
        out_.close();
        if (std::rename(tmp_.c_str(), target_.c_str()) != 0) {
            cout << "    改名失敗，正式檔維持原狀" << endl;
            return false;
        }
        committed_ = true;
        cout << "    已提交: " << tmp_ << " → " << target_ << endl;
        return true;
    }

    // 解構函數：沒提交就清掉暫存檔，不留垃圾；絕不拋出例外
    ~AtomicFileWriter() {
        if (!committed_) {
            if (out_.is_open()) out_.close();
            std::remove(tmp_.c_str());
            cout << "    未提交 → 已刪除暫存檔，正式檔未被破壞" << endl;
        }
    }
};

// 小工具：把檔案內容印出來（用來驗證結果）
static void dumpFile(const string& fname) {
    ifstream in(fname);
    if (!in.is_open()) {
        cout << "    （檔案不存在: " << fname << "）" << endl;
        return;
    }
    string line;
    while (getline(in, line)) cout << "    | " << line << endl;
}

int main() {
    cout << "=== 檔案寫入範例（RAII 自動關檔）===" << endl;
    {
        FileWriter writer("test_output.txt");
        writer.writeLine("第一行");
        writer.writeLine("第二行");
        writer.writeLine("第三行");
        cout << "  --- 即將離開區塊 ---" << endl;
    }
    // writer 離開作用域，解構函數自動關閉檔案
    cout << "  --- 檔案已自動關閉 ---" << endl;

    cout << "\n=== 開檔失敗時不沉默：明確回報 ===" << endl;
    {
        // 這個路徑的目錄不存在，開檔一定失敗
        FileWriter bad("no_such_dir_zz/out.txt");
        cout << "  isOpen() = " << (bad.isOpen() ? "true" : "false") << endl;
    }

    cout << "\n=== 拋出例外時，解構函數仍會被呼叫 ===" << endl;
    try {
        writeThenThrow("test_throw.txt");
    } catch (const exception& e) {
        cout << "  攔到例外: " << e.what() << endl;
        cout << "  （注意上面已經印出「關閉檔案」——堆疊展開時解構了）" << endl;
    }

    cout << "\n=== 日常實務：原子性寫檔 ===" << endl;
    cout << "  情況 A：正常提交" << endl;
    {
        AtomicFileWriter w("config.txt");
        w.write("retries=3");
        w.write("timeout=5000");
        w.commit();
    }
    cout << "  config.txt 內容：" << endl;
    dumpFile("config.txt");

    cout << "  情況 B：中途放棄（未提交）" << endl;
    {
        AtomicFileWriter w("config.txt");
        w.write("retries=999");        // 寫到一半就放棄
        // 沒有呼叫 commit()
    }
    cout << "  config.txt 內容（應維持情況 A 的結果）：" << endl;
    dumpFile("config.txt");

    // 清理本次產生的測試檔，讓重複執行的結果一致
    std::remove("test_output.txt");
    std::remove("test_throw.txt");
    std::remove("config.txt");
    cout << "\n=== 測試檔案已清理 ===" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 17 課：解構函數（Destructor）4.cpp" -o demo4
//
// ※ 說明（放在預期輸出標記之前）：
//   本程式會在目前工作目錄暫時建立 test_output.txt、test_throw.txt、
//   config.txt（及其 .tmp），並在結束前全部刪除，因此重複執行的輸出一致。
//   若目前目錄沒有寫入權限，開檔會失敗，輸出會與下方不同。

// === 預期輸出 ===
// === 檔案寫入範例（RAII 自動關檔）===
//   [建構] 打開檔案: test_output.txt
//   寫入: 第一行
//   寫入: 第二行
//   寫入: 第三行
//   --- 即將離開區塊 ---
//   [解構] 關閉檔案: test_output.txt
//   --- 檔案已自動關閉 ---
//
// === 開檔失敗時不沉默：明確回報 ===
//   [建構] 無法打開檔案: no_such_dir_zz/out.txt
//   isOpen() = false
//
// === 拋出例外時，解構函數仍會被呼叫 ===
//   [建構] 打開檔案: test_throw.txt
//   寫入: 例外發生前寫入的內容
//   即將拋出例外……
//   [解構] 關閉檔案: test_throw.txt
//   攔到例外: 模擬處理過程中發生錯誤
//   （注意上面已經印出「關閉檔案」——堆疊展開時解構了）
//
// === 日常實務：原子性寫檔 ===
//   情況 A：正常提交
//     開啟暫存檔: config.txt.tmp
//     已提交: config.txt.tmp → config.txt
//   config.txt 內容：
//     | retries=3
//     | timeout=5000
//   情況 B：中途放棄（未提交）
//     開啟暫存檔: config.txt.tmp
//     未提交 → 已刪除暫存檔，正式檔未被破壞
//   config.txt 內容（應維持情況 A 的結果）：
//     | retries=3
//     | timeout=5000
//
// === 測試檔案已清理 ===
