// =============================================================================
//  課程 5.4：互斥鎖的常見錯誤2.cpp  —  提前 return：鎖沒有「函式範圍」的概念
// =============================================================================
//
// 【主題資訊 Information】
//   本檔示範：函式提前 return 跳過 unlock() → 鎖被【自己】留著沒放
//             → 同一條執行緒下次再 lock() → 【未定義行為】
//   標頭檔：<mutex>
//   標準依據：對已被【呼叫執行緒本身】持有的 std::mutex 呼叫 lock()，
//             行為未定義（std::mutex 不可重入）。
//
//   ⚠️ 本檔與「常見錯誤1.cpp」的關鍵差異——這是最容易混淆的地方：
//       檔 1：忘記解鎖 →【別的】執行緒 lock() → 阻塞等待（【不是 UB】）
//       本檔：忘記解鎖 →【同一條】執行緒 lock() → 【是 UB】
//   同樣是「忘記解鎖」，下一個來取鎖的是誰，決定了性質完全不同。
//
// 【詳細解釋 Explanation】
//
// 【1. 鎖的所有權屬於「執行緒」，不屬於「函式」】
//   這是本檔要建立的核心心智模型。很多人下意識覺得
//   「lock 是在 getValue 裡做的，函式結束了鎖應該就沒事了」——
//   完全不是這樣。mutex 不知道有函式這回事，它只記得
//   「現在是哪一條執行緒持有我」。
//   getValue(-5) 提前 return 之後：
//       * 函式的堆疊框架消失了
//       * 區域變數消失了
//       * 但那把鎖【仍然被主執行緒持有】，而且沒有任何東西記得要放開它
//   接著同一條主執行緒再呼叫 getValue(20)，第一行就是 mtx.lock()——
//   對自己已持有的鎖再上鎖，UB 成立。
//
// 【2. 為什麼是 UB 而不是「保證死結」】
//   標準只說「未定義」，因為不同實作可以有不同反應：
//     * 預設的 PTHREAD_MUTEX_NORMAL（glibc 的 std::mutex 走這條）→ 實際上卡死
//     * PTHREAD_MUTEX_ERRORCHECK → 回傳 EDEADLK 錯誤碼
//     * PTHREAD_MUTEX_RECURSIVE → 遞迴計數 +1，正常繼續
//   標準不想把任何一種行為寫死，所以統一規定為 UB。
//   → 【重要的思維訓練】：
//     「本機實測會卡住」是【觀察】，「標準保證會卡住」是【承諾】。
//     UB 的定義就是「標準不承諾任何事」，包括不承諾它會出錯。
//     一段 UB 程式碼在你的機器上跑十年都正常，也完全不構成正確性的證據。
//   本機實測（g++ 15.2 / glibc、Ubuntu 26.04）：確實永久停住（exit=124）。
//   這是【本平台的實作結果】，不是標準保證的結果。
//
// 【3. 為什麼「提前 return」特別容易出事】
//   函式離開的方式遠比多數人想的多：
//       * 正常執行到底
//       * 任何一條 return（本檔有兩條）
//       * break / continue / goto 跳出臨界區段
//       * 【拋出例外】——原始碼裡完全看不見的分支
//   本檔的 getValue 只有兩條提前 return 就出事了。
//   真實程式碼裡，一個函式有五、六條 return 是常態，
//   而新人加第七條時，幾乎不可能記得「這裡有把鎖要放」。
//   ⚠️ 更糟的是：這個 bug【只在特定輸入下觸發】。
//      getValue(10) 走正常路徑，一切正常；只有 input < 0 才會洩漏鎖。
//      也就是說，測試沒覆蓋到那條分支的話，這個 bug 就會活到上線。
//
// 【4. 唯一的正解】
//       int getValue(int input) {
//           std::lock_guard<std::mutex> lock(mtx);   // 建構即 lock
//           if (input < 0) return -1;                 // ✓ 自動解鎖
//           if (input == 0) return 0;                 // ✓ 自動解鎖
//           return input * 2;                         // ✓ 自動解鎖
//       }
//   不需要記得任何事，也不需要在每條 return 前加 unlock()——
//   後者不但醜，還會在新增分支時再次被遺忘。
//
// 【概念補充 Concept Deep Dive】
//   * 怎麼把這類 UB 變成明確的錯誤訊息？
//     用 PTHREAD_MUTEX_ERRORCHECK 型別的 mutex，重複 lock 會回傳 EDEADLK。
//     C++ 標準沒有暴露這個選項，但可以在【除錯建置】中用
//     ThreadSanitizer 達到類似效果：
//         g++ -std=c++17 -pthread -g -fsanitize=thread ...
//     TSan 會直接報出 "double lock of a mutex" 並附上兩次上鎖的堆疊。
//   * 為什麼 glibc 的預設不做這個檢查？效能。
//     檢查「持有者是不是自己」需要多讀一次執行緒 id 並比對，
//     那是每一次 lock 都要付的成本。標準選擇讓正確的程式碼跑得最快，
//     把錯誤的程式碼交給 sanitizer 處理。
//   * ⚠️ 本檔的 mtx 是全域變數，所以洩漏的鎖會影響整個程式的後續執行。
//     若是類別成員 mutex，影響範圍限於該物件——但同樣致命。
//
// 【注意事項 Pay Attention】
//   1. 鎖的所有權屬於【執行緒】，不屬於函式。函式返回不會釋放鎖。
//   2. 同一條執行緒對已持有的 std::mutex 再 lock() 是【UB】，
//      不是「保證死結」——換平台可能是別的症狀。
//   3. 這類 bug 常只在【特定輸入】下觸發，測試容易漏掉。
//   4. 每條 return 前手動 unlock() 不是解法——新增分支時一定會漏。
//   5. 用 -fsanitize=thread 可以在測試階段直接抓出重複上鎖。
//   6. 真的需要同一執行緒重入時，用 std::recursive_mutex（見常見錯誤6），
//      但那通常是重構的訊號（見常見錯誤7）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】提前 return 與鎖的所有權
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 函式裡 mtx.lock() 之後提前 return 了，函式的堆疊框架都銷毀了，
//        鎖會不會自動釋放？
//     答：不會。鎖的所有權屬於【執行緒】，不屬於函式或堆疊框架。
//         mutex 只記錄「現在是哪條執行緒持有我」，它完全不知道
//         有函式這回事。函式返回後，那把鎖仍然被同一條執行緒持有，
//         而且已經沒有任何程式碼記得要放開它。
//     追問：那接下來會發生什麼？→ 看下一個來取鎖的是誰。
//           若是【別的】執行緒 → 阻塞等待（定義明確，不是 UB）；
//           若是【同一條】執行緒 → 對已持有的 mutex 再上鎖 → UB。
//
// 🔥 Q2. 同一條執行緒對 std::mutex 連續 lock() 兩次，標準說會怎樣？
//        實際上會怎樣？
//     答：標準說是【未定義行為】，沒有承諾任何特定結果。
//         實際行為取決於實作：glibc 的預設 mutex 會卡死；
//         PTHREAD_MUTEX_ERRORCHECK 型別會回傳 EDEADLK；
//         PTHREAD_MUTEX_RECURSIVE 則會正常遞增計數。
//         正因為各實作可以合理地有不同反應，標準才規定為 UB。
//     追問：那「我測過會卡住」能當成正確性的依據嗎？
//           → 不能。UB 的定義就是標準不保證任何事，
//             包括不保證它會出錯。程式在你的機器上跑十年都正常，
//             也不代表它是對的。
//
// ⚠️ 陷阱. 「那我在每一條 return 前面都加上 mtx.unlock() 就好了。」
//     答：這能讓目前的程式碼正確，但它不是解法，只是把問題延後。
//         第一，例外路徑仍然漏掉——那條分支在原始碼裡看不見，
//         你無法在「它」前面加 unlock()。
//         第二，下一個維護者新增第七條 return 時，
//         沒有任何機制提醒他要加 unlock()，bug 會原封不動地回來。
//     為什麼會錯：把問題理解成「我忘記寫了」，
//         於是解法就是「記得寫」。但真正的問題是
//         【正確性依賴於人類的記憶力】——
//         而這在任何有多人維護、會持續演化的程式碼裡都不可靠。
//         RAII 的價值正是把正確性從「記得做」變成「不可能不做」。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】—— 本檔【略過】，理由如下：
//   本檔示範的是「提前 return 導致鎖洩漏，進而觸發 UB」這個錯誤模式。
//   LeetCode 的並行題（1114 / 1115 / 1116 / 1117 / 1195）的解法通常只有
//   一兩條執行路徑，不存在複雜的提前返回結構，評測也只看輸出是否正確。
//   硬掛一題無法示範本檔重點，故從缺。
//   （RAII 在多返回路徑下的正確用法見同課「常見錯誤3.cpp」。）
//
// -----------------------------------------------------------------------------
// 【本檔是「刻意示範錯誤」的範例，開啟示範後程式會停住，不會自己結束】
//
// ⚠️ 注意這裡的錯誤性質與「兩個執行緒互等」不同：
//    getValue(-5) 提前 return 時沒有 unlock，鎖仍被【主執行緒自己】持有；
//    接著 getValue(20) 又在【同一個執行緒】對 std::mutex 呼叫 lock()。
//    對已被自己持有的 std::mutex 再次 lock()，依標準是【未定義行為】，
//    而不是「保證死結」。標準並未規定它一定會卡住。
//
//    實測（g++ 15.2 / glibc、Ubuntu 26.04）：確實永久停住，
//    程式不會自行終止（timeout 觀察到 exit=124）。
//    但這是本平台的實作結果，不是標準保證的結果 —— 換平台可能是別的症狀。
//
// 為了不讓整份課程的批次執行卡死，這個示範預設【關閉】。
// 要親眼觀察，請自行加上 -DDEMONSTRATE_UB：
//    g++ -std=c++17 -pthread -DDEMONSTRATE_UB -o early_return '課程 5.4：互斥鎖的常見錯誤2.cpp'
//    timeout 5 ./early_return ; echo "exit=$?"   # 本平台預期 exit=124
//
// 也可以用 ThreadSanitizer 直接指出這個誤用：
//    g++ -std=c++17 -pthread -g -fsanitize=thread -DDEMONSTRATE_UB -o early_return_tsan '課程 5.4：互斥鎖的常見錯誤2.cpp'
//
// ✅ 正確作法：用 std::lock_guard，讓每一條 return 路徑都自動解鎖。
// -----------------------------------------------------------------------------

#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

std::mutex mtx;

int getValue(int input) {
    mtx.lock();

    if (input < 0) {
        std::cout << "無效輸入" << std::endl;
        return -1;  // 💀 提前返回，沒有 unlock！
    }

    if (input == 0) {
        std::cout << "零值" << std::endl;
        return 0;   // 💀 又一個提前返回！
    }

    int result = input * 2;
    mtx.unlock();
    return result;
}

// -----------------------------------------------------------------------------
// 【正確對照組】同樣的多返回路徑，用 RAII 就不可能漏解鎖
// -----------------------------------------------------------------------------
std::mutex safeMtx;

int getValueSafe(int input) {
    std::lock_guard<std::mutex> lock(safeMtx);   // 建構即 lock

    if (input < 0)  return -1;                   // ✓ 自動解鎖
    if (input == 0) return 0;                    // ✓ 自動解鎖
    return input * 2;                            // ✓ 自動解鎖
}

// -----------------------------------------------------------------------------
// 【日常實務範例】參數驗證：錯誤路徑才是鎖洩漏的高發區
//   情境：一個設定服務的 update() 方法，要先做多項驗證才真正寫入。
//         驗證失敗的路徑通常有五、六條，而且【平常都不會走到】——
//         正是最容易漏掉解鎖、也最不容易被測試覆蓋的地方。
//   本例示範正確寫法：所有驗證路徑都由 lock_guard 保證解鎖，
//   並用「連續呼叫多次，涵蓋每一條錯誤路徑」來證明鎖從未洩漏——
//   只要任何一條路徑漏了解鎖，後續呼叫就會立刻卡死。
// -----------------------------------------------------------------------------
class ConfigService {
public:
    enum class Status { Ok, EmptyKey, ValueTooLong, ReservedKey, ReadOnly };

private:
    std::mutex mtx_;
    bool readOnly_ = false;
    int  writes_ = 0;

public:
    void setReadOnly(bool v) {
        std::lock_guard<std::mutex> lock(mtx_);
        readOnly_ = v;
    }

    // 五條離開路徑，每一條都由 RAII 保證解鎖
    Status update(const std::string& key, const std::string& value) {
        std::lock_guard<std::mutex> lock(mtx_);

        if (key.empty())            return Status::EmptyKey;      // 路徑 1
        if (value.size() > 256)     return Status::ValueTooLong;  // 路徑 2
        if (key.rfind("__", 0) == 0) return Status::ReservedKey;  // 路徑 3
        if (readOnly_)              return Status::ReadOnly;      // 路徑 4

        ++writes_;
        return Status::Ok;                                        // 路徑 5
    }

    int writes() {
        std::lock_guard<std::mutex> lock(mtx_);
        return writes_;
    }
};

const char* statusName(ConfigService::Status s) {
    switch (s) {
        case ConfigService::Status::Ok:           return "Ok";
        case ConfigService::Status::EmptyKey:     return "EmptyKey";
        case ConfigService::Status::ValueTooLong: return "ValueTooLong";
        case ConfigService::Status::ReservedKey:  return "ReservedKey";
        case ConfigService::Status::ReadOnly:     return "ReadOnly";
    }
    return "?";
}

int main() {
    std::cout << getValue(10) << std::endl;   // OK：有走到 unlock()
    std::cout << getValue(-5) << std::endl;   // 💀 提前 return，鎖沒釋放

#ifdef DEMONSTRATE_UB
    // 💀 同一執行緒對已持有的 std::mutex 再 lock() → 未定義行為。
    //    本平台實測：停在這裡不再往下走。
    std::cout << getValue(20) << std::endl;

    std::cout << "（本平台上這一行不會被執行）" << std::endl;
#else
    std::cout << "已略過會卡住的第三次呼叫（預設關閉）。" << std::endl;
    std::cout << "要重現請加上 -DDEMONSTRATE_UB 重新編譯。" << std::endl;
#endif

    std::cout << "\n=== 正確對照組: lock_guard 涵蓋所有返回路徑 ===" << std::endl;
    std::cout << "getValueSafe(10) = " << getValueSafe(10) << std::endl;
    std::cout << "getValueSafe(-5) = " << getValueSafe(-5) << std::endl;
    std::cout << "getValueSafe(0)  = " << getValueSafe(0)  << std::endl;
    // 連續走完三條不同的返回路徑後，鎖仍能正常取得
    std::cout << "三條路徑走完後鎖仍可取得: "
              << (safeMtx.try_lock() ? "是" : "否") << std::endl;
    safeMtx.unlock();

    std::cout << "\n=== 鎖的所有權屬於執行緒，不屬於函式 ===" << std::endl;
    std::cout << "getValue(-5) 已經讓主執行緒持有一把永遠不會釋放的鎖。" << std::endl;
    std::cout << "此時若由【別的執行緒】去 lock()，是阻塞等待（不是 UB）；"
              << std::endl;
    std::cout << "若由【同一條執行緒】再 lock()，才是未定義行為。" << std::endl;
    std::cout << "同樣是「忘記解鎖」，下一個取鎖的是誰決定了性質完全不同。"
              << std::endl;

    std::cout << "\n=== 日常實務: 參數驗證的五條錯誤路徑 ===" << std::endl;
    {
        ConfigService config;

        std::string longValue(300, 'x');
        std::cout << "update(\"\", \"v\")           -> "
                  << statusName(config.update("", "v")) << std::endl;
        std::cout << "update(\"k\", <300 字元>)    -> "
                  << statusName(config.update("k", longValue)) << std::endl;
        std::cout << "update(\"__internal\", \"v\") -> "
                  << statusName(config.update("__internal", "v")) << std::endl;
        std::cout << "update(\"timeout\", \"30\")   -> "
                  << statusName(config.update("timeout", "30")) << std::endl;

        config.setReadOnly(true);
        std::cout << "唯讀模式下 update(\"a\",\"b\") -> "
                  << statusName(config.update("a", "b")) << std::endl;

        std::cout << "成功寫入次數: " << config.writes()
                  << "  (預期 1——只有一次通過全部驗證)" << std::endl;
        std::cout << "重點: 五條路徑全部走過，鎖一次都沒洩漏——"
                     "若漏了任何一條，上面的呼叫早就卡死了" << std::endl;

        // 多執行緒驗證：錯誤路徑在高併發下也不會洩漏鎖
        std::vector<std::thread> workers;
        for (int t = 0; t < 8; ++t) {
            workers.emplace_back([&config]() {
                for (int i = 0; i < 2000; ++i) {
                    config.update("", "v");              // 一直走錯誤路徑
                }
            });
        }
        for (auto& t : workers) t.join();

        config.setReadOnly(false);   // 解除唯讀，讓下面的驗證能走到成功路徑
        std::cout << "8 執行緒各走 2000 次錯誤路徑後，服務仍正常: "
                  << statusName(config.update("k2", "v2")) << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread '課程 5.4：互斥鎖的常見錯誤2.cpp' -o early_return
// 觀察 UB（本平台會卡住，務必加 timeout）:
//   g++ -std=c++17 -Wall -Wextra -pthread -DDEMONSTRATE_UB '課程 5.4：互斥鎖的常見錯誤2.cpp' -o early_return_ub
//   timeout 5 ./early_return_ub ; echo "exit=$?"     # 本機實測 exit=124
// 讓 ThreadSanitizer 直接指出誤用:
//   g++ -std=c++17 -pthread -g -fsanitize=thread -DDEMONSTRATE_UB '課程 5.4：互斥鎖的常見錯誤2.cpp' -o early_return_tsan

// -----------------------------------------------------------------------------
// 【輸出但書】
//   1. 以下是【預設編譯】（不加 -DDEMONSTRATE_UB）的輸出。
//      加上 -DDEMONSTRATE_UB 後，第三次 getValue 會觸發【未定義行為】；
//      本機實測是永久停住（timeout 5 觀察到 exit=124），
//      但這是本平台的實作結果，【不是】標準保證——換平台可能是別的症狀。
//   2. 其餘輸出完全確定：正確對照組與實務範例都不依賴排程順序，
//      多執行緒區段只驗證「服務仍正常」這個不變量。
// -----------------------------------------------------------------------------

// === 預期輸出 ===
// 20
// 無效輸入
// -1
// 已略過會卡住的第三次呼叫（預設關閉）。
// 要重現請加上 -DDEMONSTRATE_UB 重新編譯。
//
// === 正確對照組: lock_guard 涵蓋所有返回路徑 ===
// getValueSafe(10) = 20
// getValueSafe(-5) = -1
// getValueSafe(0)  = 0
// 三條路徑走完後鎖仍可取得: 是
//
// === 鎖的所有權屬於執行緒，不屬於函式 ===
// getValue(-5) 已經讓主執行緒持有一把永遠不會釋放的鎖。
// 此時若由【別的執行緒】去 lock()，是阻塞等待（不是 UB）；
// 若由【同一條執行緒】再 lock()，才是未定義行為。
// 同樣是「忘記解鎖」，下一個取鎖的是誰決定了性質完全不同。
//
// === 日常實務: 參數驗證的五條錯誤路徑 ===
// update("", "v")           -> EmptyKey
// update("k", <300 字元>)    -> ValueTooLong
// update("__internal", "v") -> ReservedKey
// update("timeout", "30")   -> Ok
// 唯讀模式下 update("a","b") -> ReadOnly
// 成功寫入次數: 1  (預期 1——只有一次通過全部驗證)
// 重點: 五條路徑全部走過，鎖一次都沒洩漏——若漏了任何一條，上面的呼叫早就卡死了
// 8 執行緒各走 2000 次錯誤路徑後，服務仍正常: Ok
