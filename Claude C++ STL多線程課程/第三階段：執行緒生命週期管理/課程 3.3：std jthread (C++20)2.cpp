// =============================================================================
//  課程 3.3：std::jthread (C++20) — 第 2 部分：例外安全與 stack unwinding
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<thread>、<stop_token>、<stdexcept>
//   標準版本：C++20
//
//   核心規則（[thread.jthread.cons]）：
//       ~jthread() {
//           if (joinable()) { request_stop(); join(); }
//       }
//   關鍵在於：解構子是在 stack unwinding 期間被自動呼叫的，
//   所以「函式中途拋出例外」與「函式正常返回」走的是同一條回收路徑。
//
// 【詳細解釋 Explanation】
//
// 【1. 問題的根源：例外會跳過 join()】
//   考慮這段用 std::thread 的程式碼：
//       void risky() {
//           std::thread t(work);
//           mayThrow();      // ← 如果這裡拋出例外
//           t.join();        // ← 這行【永遠不會執行】
//       }                    // ← t 解構時仍 joinable → std::terminate()
//   例外會讓控制流直接跳到 catch，中間所有「敘述」都被跳過。
//   t.join() 只是一個敘述，當然也被跳過。於是 t 帶著 joinable 狀態走進解構子，
//   標準規定此時呼叫 std::terminate()，程式立刻中止 —— 而且外層的
//   catch (...) 【攔不住】，因為 terminate() 不是例外，是直接中止。
//
// 【2. 為什麼解構子沒被跳過？——stack unwinding】
//   這是 C++ 例外機制最重要的保證：當例外往外傳播時，
//   執行期會逐層「解開」呼叫堆疊，並對每一個已完整建構的自動儲存期物件
//   呼叫其解構子。這個過程叫 stack unwinding。
//   敘述會被跳過，但【解構子不會】。
//   所以把「必須執行的收尾動作」放進解構子，是 C++ 唯一可靠的手段 ——
//   這就是 RAII 的全部精髓，也是 C++ 沒有 finally 關鍵字的原因：
//   有了 RAII，finally 就是多餘的。
//
// 【3. jthread 如何解決】
//   jthread 把 join() 從「一個敘述」變成「解構子的一部分」。
//   敘述會被例外跳過，解構子不會。所以：
//       void risky() {
//           std::jthread jt(work);
//           mayThrow();      // 拋出例外
//       }                    // unwinding → ~jthread() → request_stop() + join()
//   例外可以順利往外傳，執行緒也被正確回收，兩件事互不干擾。
//
// 【4. 執行順序的細節：先 join 完，例外才繼續往外傳】
//   本檔的輸出順序是理解這點的關鍵證據：
//       執行緒完成            ← 背景執行緒的輸出（jt 解構時 join 等它跑完）
//       捕獲: 發生錯誤！      ← main 的 catch 才收到例外
//   例外【不會】在解構子執行到一半時就跳到 catch。
//   正確的順序是：拋出 → unwinding（跑完 ~jthread()，含 join 阻塞 100ms）
//   → 才抵達 catch。所以「執行緒完成」一定印在「捕獲」之前，這是確定的。
//
// 【概念補充 Concept Deep Dive】
//   * 解構子在 unwinding 期間拋出例外會怎樣？
//     C++11 起解構子預設是 noexcept(true)。若 unwinding 期間解構子又拋例外，
//     等於在 noexcept 函式中拋例外 → 直接 std::terminate()。
//     這就是「解構子絕對不要拋例外」這條鐵律的來源。
//     ~jthread() 內部的 join() 理論上可能丟 system_error，
//     但在正常情況（執行緒有效、非自我 join）下不會發生。
//
//   * 為什麼 catch (...) 攔不住 std::terminate()？
//     terminate() 不是「拋出一個例外」，它是直接呼叫 terminate_handler，
//     預設實作是 std::abort() → 送出 SIGABRT。這條路徑完全繞過例外機制，
//     所以任何 catch 都攔不到。這是【標準保證的中止】，不是未定義行為 ——
//     說「一定會 abort」在這裡是正確的描述。
//
//   * 例外物件本身存在哪裡？
//     不在堆疊上（堆疊正在被解開），而是由執行期在專屬的例外儲存區配置
//     （Itanium ABI 的 __cxa_allocate_exception）。所以例外物件在 unwinding
//     期間一直有效，這也是它能安全傳過多層解構子的原因。
//
// 【注意事項 Pay Attention】
//   1. jthread 只保證「執行緒會被 join」，【不保證】執行緒內部的例外會傳給你。
//      執行緒函式若讓例外逃逸出去，會直接 std::terminate()（見課程 3.4）。
//      本檔的例外是在 main 這一側拋的，不是在執行緒內拋的，兩者完全不同。
//   2. 解構子的 join() 會阻塞。若背景執行緒要跑 10 秒，這個函式就會多花 10 秒
//      才把例外傳出去。要避免就得用 stop_token 讓執行緒能提早收工。
//   3. 「例外安全」不等於「執行緒安全」。這裡談的是資源不外洩，
//      與資料競爭是兩個獨立的議題。
//   4. 本檔必須用 -std=c++20（已用 -pedantic-errors 驗證，C++17 會編譯失敗）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】例外安全與 RAII 執行緒
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 一個函式建立了 std::thread，還沒 join 就拋出例外，會發生什麼事？
//     答：thread 物件解構時仍是 joinable，標準規定呼叫 std::terminate()，
//         程式直接中止。這是標準【保證】的行為，而且外層 catch (...)
//         攔不住，因為 terminate() 走的是 abort 路徑、不是例外傳播路徑。
//     追問：那要怎麼修？
//         → 用 RAII 包裝：C++20 直接用 std::jthread；C++11/14/17 自己寫
//           ThreadGuard，在解構子裡 if (t.joinable()) t.join()。
//
// 🔥 Q2. 為什麼例外能跳過 t.join() 這行，卻跳不過解構子？
//     答：因為兩者機制不同。join() 是一個【敘述】，例外傳播時控制流直接
//         跳離，敘述自然不會執行。解構子則由 stack unwinding 機制負責，
//         執行期會逐層對每個已完整建構的自動物件呼叫解構子，這是標準
//         保證的。所以「必須執行的收尾」只能放解構子，不能放敘述。
//     追問：這跟其他語言的 finally 有什麼關係？
//         → finally 是把收尾動作綁在「控制流」上；RAII 是綁在「物件生命週期」上。
//           C++ 選了後者，所以不需要 finally；代價是你必須先有一個物件。
//
// ⚠️ 陷阱. 本檔的輸出，「執行緒完成」和「捕獲: 發生錯誤！」哪個先印？
//     答：一定是「執行緒完成」先。因為 ~jthread() 是在 unwinding 期間執行的，
//         它的 join() 會阻塞到執行緒真的跑完（含那 100ms 的 sleep），
//         之後例外才繼續往外傳到 main 的 catch。
//     為什麼會錯：很多人以為 throw 會「立刻」跳到 catch，把 unwinding 想成
//         瞬間完成的事。實際上 unwinding 會同步執行沿路所有解構子，
//         這些解構子可以做任意耗時的工作（這裡就是 join 等了 100ms）。
//         把 throw 想成 goto 是錯誤的心智模型。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】—— 本檔從缺
//   本檔主題是「例外傳播與解構子的互動」，屬於 C++ 語言機制。
//   LeetCode 的並行題（1114/1115/1116/1117/1195）全部聚焦在執行緒間的
//   順序協調，測試框架也不會讓你的解法拋例外。兩者沒有真實交集，故從缺。

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <stop_token>
#include <string>
#include <thread>
#include <vector>

// -----------------------------------------------------------------------------
// 【基本示範】函式中途拋例外，jthread 仍被正確回收
// -----------------------------------------------------------------------------
void riskyFunction() {
    std::jthread jt([]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "執行緒完成" << std::endl;
    });

    throw std::runtime_error("發生錯誤！");

    // 不需要擔心！jt 解構時會自動 join
    // （上面拋出例外後，stack unwinding 會執行 ~jthread()）
}

// -----------------------------------------------------------------------------
// 【對照示範】多層巢狀：unwinding 會逐層執行解構子
//   證明 RAII 的回收是「逐層、有序」的，不是一次跳到最外層。
// -----------------------------------------------------------------------------
struct Tracer {
    const char* name;
    explicit Tracer(const char* n) : name(n) {
        std::cout << "  進入 " << name << std::endl;
    }
    ~Tracer() { std::cout << "  離開 " << name << "（解構子執行）" << std::endl; }
};

void innerLevel() {
    Tracer t("innerLevel");
    std::jthread jt([]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        std::cout << "  innerLevel 的背景工作結束" << std::endl;
    });
    throw std::logic_error("來自最內層的例外");
}

void outerLevel() {
    Tracer t("outerLevel");
    innerLevel();
}

// -----------------------------------------------------------------------------
// 【日常實務範例】批次匯入器：解析失敗要回報錯誤，但工作執行緒必須先收乾淨
//   情境：從 CSV 匯入訂單，主執行緒負責驗證欄位，背景執行緒負責把已驗證的
//         資料寫入資料庫。遇到格式錯誤時要拋例外讓上層記錄失敗原因，
//         但【絕不能】在背景執行緒還在寫 DB 時就讓物件消失。
//   為何用 jthread：不論是驗證成功走到函式結尾、還是驗證失敗拋出例外，
//         都保證背景寫入執行緒被 join 完才離開，資料不會寫到一半。
// -----------------------------------------------------------------------------
struct ImportError : std::runtime_error {
    explicit ImportError(const std::string& msg) : std::runtime_error(msg) {}
};

void importOrders(const std::vector<std::string>& rows) {
    int flushed = 0;

    std::jthread dbWriter([&flushed](std::stop_token st) {
        while (!st.stop_requested()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            ++flushed;
        }
        std::cout << "  [db] 收到停止請求，flush 收尾完成（共 " << flushed
                  << " 批）" << std::endl;
    });

    for (const auto& row : rows) {
        std::cout << "  [parse] 檢查: " << row << std::endl;
        if (row.find(',') == std::string::npos) {
            // 拋出後：dbWriter 解構 → request_stop() + join()，DB 寫入乾淨收尾
            throw ImportError("欄位分隔符缺失: " + row);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }
    std::cout << "  [parse] 全部通過" << std::endl;
}

int main() {
    std::cout << "=== 基本示範：拋例外仍自動 join ===" << std::endl;
    try {
        riskyFunction();
    } catch (const std::exception& e) {
        std::cout << "捕獲: " << e.what() << std::endl;
    }

    std::cout << "\n=== 多層 unwinding：解構子逐層執行 ===" << std::endl;
    try {
        outerLevel();
    } catch (const std::exception& e) {
        std::cout << "  main 捕獲: " << e.what() << std::endl;
    }

    std::cout << "\n=== 日常實務：批次匯入失敗時的乾淨收尾 ===" << std::endl;
    try {
        importOrders({"1001,ACME,3", "1002,GLOBEX,7", "1003-BADROW"});
    } catch (const ImportError& e) {
        std::cout << "  匯入中止，原因: " << e.what() << std::endl;
    }
    std::cout << "  （背景寫入執行緒已確定結束，可以安全回報失敗）" << std::endl;

    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra -pthread "課程 3.3：std jthread (C++20)2.cpp" -o jthread2
//   注意：本檔【必須】用 -std=c++20（std::jthread / std::stop_token 為 C++20 新增，
//   已用 -std=c++17 -pedantic-errors 實測確認會編譯失敗）。

// 註:
//   （實務範例中的 flush 批次數取決於執行緒排程與計時，每次執行都可能不同）

// === 預期輸出 ===
// === 基本示範：拋例外仍自動 join ===
// 執行緒完成
// 捕獲: 發生錯誤！
//
// === 多層 unwinding：解構子逐層執行 ===
//   進入 outerLevel
//   進入 innerLevel
//   innerLevel 的背景工作結束
//   離開 innerLevel（解構子執行）
//   離開 outerLevel（解構子執行）
//   main 捕獲: 來自最內層的例外
//
// === 日常實務：批次匯入失敗時的乾淨收尾 ===
//   [parse] 檢查: 1001,ACME,3
//   [parse] 檢查: 1002,GLOBEX,7
//   [parse] 檢查: 1003-BADROW
//   [db] 收到停止請求，flush 收尾完成（共 3 批）
//   匯入中止，原因: 欄位分隔符缺失: 1003-BADROW
//   （背景寫入執行緒已確定結束，可以安全回報失敗）
