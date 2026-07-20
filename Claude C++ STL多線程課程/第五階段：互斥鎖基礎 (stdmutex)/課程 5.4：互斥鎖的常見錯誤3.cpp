// =============================================================================
//  課程 5.4：互斥鎖的常見錯誤3.cpp  —  用 std::lock_guard 根治「忘記解鎖」
// =============================================================================
//
// 【主題資訊 Information】
//   template<class Mutex> class std::lock_guard;                    // C++11，<mutex>
//       explicit lock_guard(mutex_type& m);          // 建構 → m.lock()
//       lock_guard(mutex_type& m, std::adopt_lock_t);// 建構 → 假設已持有，不再 lock
//       ~lock_guard();                               // 解構 → m.unlock()
//       lock_guard(const lock_guard&) = delete;      // 不可複製、不可移動
//   複雜度：與手寫 lock/unlock 完全相同——最佳化後不產生任何額外指令。
//   C++17 起可省略樣板參數（CTAD）：std::lock_guard lock(mtx);
//
// 【詳細解釋 Explanation】
//
// 【1. RAII 解決的不是「少打字」，而是「所有離開路徑」】
//   前兩個檔案示範了同一個錯誤的兩種面貌：忘記寫 unlock、以及
//   提前 return 跳過 unlock。它們的共同結構是：
//       解鎖這件事，被綁在「某一行程式碼有沒有被執行到」上。
//   而一個函式離開的方式遠比多數人想像的多：
//       * 正常執行到底
//       * 任何一條 return
//       * break / continue / goto 跳出臨界區段
//       * 【拋出例外】——原始碼裡完全看不見的分支
//   RAII 把解鎖綁在【物件生命週期】上，而不是綁在某一行程式碼上。
//   C++ 保證：只要物件成功建構，離開作用域時解構函式【必然】被呼叫，
//   不論是哪一種離開方式。這是語言層級的保證，不是紀律。
//
// 【2. 例外是那條看不見的分支】
//   即使一段程式碼裡完全沒有 throw，它仍可能拋出例外：
//       v.push_back(x);              // 可能重新配置記憶體 → std::bad_alloc
//       s += "text";                 // 同上
//       m.at(key);                   // 可能丟 std::out_of_range
//       foo();                       // 被呼叫者內部可能 throw
//   手寫 unlock 的程式碼在這些地方全部會漏解鎖，
//   而且【編譯器不會給任何警告】——因為語法上完全合法。
//   這是 lock_guard 最重要的價值：它讓例外安全變成預設行為，而非額外工作。
//
// 【3. lock_guard 是零成本的】
//   常見疑慮是「多包一層物件會不會變慢」。不會：
//     * 建構函式就是 m.lock()，解構函式就是 m.unlock()，兩者都是 inline；
//     * 物件本身只存一個 mutex 的參考，沒有額外狀態；
//     * 最佳化後產生的機器碼與手寫 lock/unlock 完全相同。
//   對比 unique_lock：它多存一個「是否持有」的 bool，
//   解構時多一次分支判斷——那是為了換取延遲鎖定與中途解鎖的彈性。
//   → 【選用原則】：不需要彈性就用 lock_guard，它同時也是最強的
//     意圖宣告：「這個作用域從頭到尾都持有鎖」。
//
// 【4. 作用域就是臨界區段——用大括號主動控制】
//   lock_guard 的鎖定範圍完全由作用域決定，所以可以用一對大括號
//   精確地把臨界區段縮到最小：
//       int snapshot;
//       {
//           std::lock_guard<std::mutex> lock(mtx);
//           snapshot = sharedValue;          // 只有這行需要鎖
//       }                                    // 這裡就解鎖了
//       doExpensiveWork(snapshot);           // 不持有鎖，其他執行緒可以進來
//   這個「用大括號切出臨界區段」的手法是 C++ 並行程式碼的基本功。
//
// 【概念補充 Concept Deep Dive】
//   * 為什麼 lock_guard 不可複製也不可移動：鎖的所有權必須是唯一的。
//     若能複製，兩個 lock_guard 解構時會對同一把鎖 unlock 兩次——
//     第二次是對未持有的 mutex 解鎖，是未定義行為。
//     需要轉移所有權時請用 unique_lock（它是 movable）。
//   * ⚠️ 最經典的手誤：忘了寫變數名
//       std::lock_guard<std::mutex>(mtx);    // 💀 建立【暫時物件】，
//                                            //    該行結束就立刻解構解鎖！
//       std::lock_guard<std::mutex> lock(mtx);  // ✅ 正確
//     前者在語法上完全合法，編譯器不會報錯，但等於完全沒有保護。
//     ⚠️ 這也是 C++17 CTAD 特別有價值的地方：寫成
//       std::lock_guard lock(mtx); 就不容易漏掉變數名。
//   * lock_guard 的解構函式是 noexcept 的（unlock 不會拋例外）。
//     這很重要：若解構時拋例外，而當下正在處理另一個例外，
//     程式會直接 std::terminate()。
//
// 【注意事項 Pay Attention】
//   1. ⚠️ 一定要給 lock_guard 變數名字。std::lock_guard<std::mutex>(mtx);
//      建立的是暫時物件，當場就解構了，等於沒有加鎖。
//   2. lock_guard 不可複製、不可移動；需要轉移所有權請用 unique_lock。
//   3. lock_guard 沒有 unlock() 方法——它就是要你用作用域控制範圍。
//      需要中途解鎖請用 unique_lock。
//   4. 需要同時鎖多把鎖時用 std::scoped_lock（C++17），
//      不要寫兩個 lock_guard（那會依序取得，可能死結）。
//   5. 臨界區段內仍應避免 I/O 與長時間運算——RAII 保證解鎖，
//      但不會幫你縮小範圍。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::lock_guard
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. lock_guard 相對於手寫 lock()/unlock() 的核心價值是什麼？
//     答：把解鎖從「某一行程式碼有沒有被執行到」變成「物件生命週期結束時
//         必然發生」。函式可能透過正常返回、任何一條 return、break/goto，
//         或【拋出例外】離開；例外是原始碼裡看不見的分支，
//         手寫 unlock 在那條路徑上一定會被跳過。
//         RAII 讓例外安全成為預設行為。
//     追問：它有效能代價嗎？→ 沒有。建構即 lock、解構即 unlock，都是 inline，
//           物件本身只存一個參考，最佳化後與手寫版產生相同的機器碼。
//
// 🔥 Q2. lock_guard、unique_lock、scoped_lock 各在什麼時候用？
//     答：lock_guard —— 預設選擇。「整個作用域都持有鎖」，最輕、意圖最明確。
//         unique_lock —— 需要延遲鎖定（defer_lock）、非阻塞嘗試（try_to_lock）、
//         中途 unlock、轉移所有權，或搭配 condition_variable 時使用。
//         scoped_lock（C++17）—— 需要【同時】取得多把鎖，內建死結避免演算法。
//     追問：為什麼不能用兩個 lock_guard 鎖兩把鎖？
//           → 那是「依序取得」，兩條執行緒若順序相反就會 AB-BA 死結。
//             scoped_lock 是「一起取得」，內部演算法保證不死結。
//
// ⚠️ 陷阱. 這行程式碼有什麼問題？
//        std::lock_guard<std::mutex>(mtx);
//     答：它建立的是一個【沒有名字的暫時物件】，該敘述結束時就立刻解構，
//         也就立刻解鎖了。後面的程式碼完全沒有任何保護，
//         但編譯器不會給任何警告——語法上它完全合法。
//     為什麼會錯：眼睛看到 lock_guard 和 mtx 都在，就認定「有鎖了」，
//         沒注意到少了變數名。這與 RAII 的規則一致
//         （暫時物件在完整運算式結束時解構），不是特例；
//         真正的問題是這個手誤在視覺上幾乎無法察覺。
//         防範方式：C++17 起寫 std::lock_guard lock(mtx);（CTAD），
//         少了樣板參數的雜訊，漏掉變數名會明顯很多。
// ═══════════════════════════════════════════════════════════════════════════

#include <algorithm>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

std::mutex mtx;

int getValueSafe(int input) {
    std::lock_guard<std::mutex> lock(mtx);  // ✓ RAII

    if (input < 0) {
        std::cout << "無效輸入" << std::endl;
        return -1;  // ✓ lock_guard 解構時自動 unlock
    }

    if (input == 0) {
        std::cout << "零值" << std::endl;
        return 0;   // ✓ 同樣會自動 unlock
    }

    return input * 2;
}  // ✓ 函式結束，lock_guard 解構，自動 unlock

// 例外路徑：原始碼裡看不見的第四條離開路徑
int getValueOrThrow(int input) {
    std::lock_guard<std::mutex> lock(mtx);

    if (input > 100) {
        throw std::out_of_range("input 超過上限");  // ✓ 例外路徑照樣自動解鎖
    }
    return input * 2;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 155. Min Stack
//   題目：設計一個堆疊，除了 push / pop / top 之外，
//         還要能在 O(1) 時間取得堆疊中的最小值 getMin()。
//   為什麼用到本主題：155 的核心是「兩個容器必須永遠同步」——
//         主堆疊與最小值堆疊的 push/pop 必須成對發生。
//         一旦做成多執行緒可用，每個方法就都必須是原子的：
//         若 push 到一半（主堆疊進去了、最小值堆疊還沒）就被別人看到，
//         getMin() 會回傳錯的值。
//         用 lock_guard 讓每個方法的【所有離開路徑】都自動解鎖，
//         包含 vector 重新配置時可能拋出的 std::bad_alloc——
//         那正是手寫 unlock 一定會漏掉的路徑。
//   註：LeetCode 原題是單執行緒的；這裡刻意做成執行緒安全版本，
//       正是本課要示範的重點。
// -----------------------------------------------------------------------------
class MinStack {
private:
    mutable std::mutex mtx_;
    std::vector<int> data_;
    std::vector<int> mins_;    // mins_.back() 永遠是 data_ 目前的最小值

public:
    void push(int val) {
        std::lock_guard<std::mutex> lock(mtx_);
        // ⚠️ 這兩個 push_back 都可能重新配置記憶體而拋出 std::bad_alloc。
        //    手寫 unlock 的版本會在那條路徑上洩漏鎖；lock_guard 不會。
        data_.push_back(val);
        mins_.push_back(mins_.empty() ? val : std::min(val, mins_.back()));
    }

    void pop() {
        std::lock_guard<std::mutex> lock(mtx_);
        if (data_.empty()) return;      // 提前 return，照樣自動解鎖
        data_.pop_back();
        mins_.pop_back();
    }

    int top() const {
        std::lock_guard<std::mutex> lock(mtx_);
        return data_.empty() ? 0 : data_.back();
    }

    int getMin() const {
        std::lock_guard<std::mutex> lock(mtx_);
        return mins_.empty() ? 0 : mins_.back();
    }

    std::size_t size() const {
        std::lock_guard<std::mutex> lock(mtx_);
        return data_.size();
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】用大括號把臨界區段縮到最小
//   情境：從共享的設定表讀出一個值，然後拿它去做一件耗時的事
//         （呼叫外部 API、產生報表、壓縮資料…）。
//   反例：整個函式包在 lock_guard 裡 → 耗時工作期間所有人都被擋住。
//   正解：用一對大括號讓 lock_guard 只活在「複製出快照」那幾行，
//         耗時工作在鎖外進行。
// -----------------------------------------------------------------------------
class ReportGenerator {
private:
    std::mutex mtx_;
    std::string templateName_ = "monthly-v3";
    int         maxRows_ = 5000;

public:
    void updateTemplate(const std::string& name, int maxRows) {
        std::lock_guard<std::mutex> lock(mtx_);
        templateName_ = name;
        maxRows_ = maxRows;
    }

    std::string generate() {
        // ── 臨界區段：只複製快照，盡快放手 ──
        std::string name;
        int rows;
        {
            std::lock_guard<std::mutex> lock(mtx_);
            name = templateName_;
            rows = maxRows_;
        }   // ← 鎖在這裡就釋放了

        // ── 鎖外：耗時工作，不阻擋其他執行緒 ──
        return "report[" + name + "] rows=" + std::to_string(rows);
    }
};

int main() {
    std::cout << "=== 課程示範: lock_guard 的三條離開路徑 ===" << std::endl;
    std::cout << getValueSafe(10) << std::endl;
    std::cout << getValueSafe(-5) << std::endl;
    std::cout << getValueSafe(20) << std::endl;  // ✓ 正常執行（前面沒有洩漏鎖）

    std::cout << "\n=== 第四條路徑: 例外（原始碼裡看不見的分支）===" << std::endl;
    try {
        getValueOrThrow(500);
    } catch (const std::out_of_range& e) {
        std::cout << "捕獲例外: " << e.what() << std::endl;
    }
    // 若例外路徑漏了解鎖，下面這行會【永久阻塞】
    std::cout << "例外之後仍能取得同一把鎖: " << getValueSafe(3) << std::endl;

    std::cout << "\n=== LeetCode 155. Min Stack ===" << std::endl;
    {
        MinStack st;
        st.push(-2);
        st.push(0);
        st.push(-3);
        std::cout << "getMin() = " << st.getMin() << "  (預期 -3)" << std::endl;
        st.pop();
        std::cout << "top()    = " << st.top()    << "  (預期 0)"  << std::endl;
        std::cout << "getMin() = " << st.getMin() << "  (預期 -2)" << std::endl;

        // 多執行緒壓測：8 條執行緒各 push 1000 次再全部 pop 掉
        MinStack shared;
        std::vector<std::thread> workers;
        for (int t = 0; t < 8; ++t) {
            workers.emplace_back([&shared]() {
                for (int i = 0; i < 1000; ++i) shared.push(i);
            });
        }
        for (auto& t : workers) t.join();
        std::cout << "8 執行緒各 push 1000 次後 size() = " << shared.size()
                  << "  (必須是 8000)" << std::endl;
        std::cout << "getMin() = " << shared.getMin()
                  << "  (必須是 0：每條執行緒都從 0 開始 push)" << std::endl;
    }

    std::cout << "\n=== 日常實務: 用大括號縮小臨界區段 ===" << std::endl;
    {
        ReportGenerator gen;
        std::cout << gen.generate() << std::endl;
        gen.updateTemplate("quarterly-v1", 20000);
        std::cout << gen.generate() << std::endl;
        std::cout << "重點: 耗時的產報表工作在鎖【外】執行，不阻擋其他執行緒"
                  << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread '課程 5.4：互斥鎖的常見錯誤3.cpp' -o raii_fix

// -----------------------------------------------------------------------------
// 【輸出但書】
//   本檔輸出完全確定：所有多執行緒區段都只驗證不變量（總數、最小值），
//   不依賴任何排程順序。
//   「例外之後仍能取得同一把鎖」這行若印不出來（程式卡住），
//   就代表例外路徑漏了解鎖——這正是本檔要證明不會發生的事。
// -----------------------------------------------------------------------------

// === 預期輸出 ===
// === 課程示範: lock_guard 的三條離開路徑 ===
// 20
// 無效輸入
// -1
// 40
//
// === 第四條路徑: 例外（原始碼裡看不見的分支）===
// 捕獲例外: input 超過上限
// 例外之後仍能取得同一把鎖: 6
//
// === LeetCode 155. Min Stack ===
// getMin() = -3  (預期 -3)
// top()    = 0  (預期 0)
// getMin() = -2  (預期 -2)
// 8 執行緒各 push 1000 次後 size() = 8000  (必須是 8000)
// getMin() = 0  (必須是 0：每條執行緒都從 0 開始 push)
//
// === 日常實務: 用大括號縮小臨界區段 ===
// report[monthly-v3] rows=5000
// report[quarterly-v1] rows=20000
// 重點: 耗時的產報表工作在鎖【外】執行，不阻擋其他執行緒
