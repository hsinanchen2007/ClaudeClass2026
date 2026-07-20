// =============================================================================
//  第 17 課：解構函數 5  —  作用域計時器：解構函數不只用來釋放資源
// =============================================================================
//
// 【主題資訊 Information】
//   主題：利用「離開作用域必定呼叫解構函數」的保證，做自動化的量測與收尾
//   標準版本：<chrono> 為 C++11；本檔用到的 steady_clock 亦為 C++11
//   複雜度：計時本身 O(1)；量測到的工作量另計
//   標頭檔：<chrono>、<string>
//
// 【詳細解釋 Explanation】
//
// 【1. 換個角度看解構函數】
//   前面幾個檔案把解構函數當成「歸還資源」的地方。
//   但它真正的語意其實更廣：**「這個作用域結束時，一定要做的事」**。
//   計時就是最好的例子——它不釋放任何資源，只是想知道
//   「從這裡到離開這個作用域，花了多久」。
//   因為解構函數的呼叫是語言保證的，所以不論函數從哪裡 return、
//   甚至拋出例外，這個量測都不會漏掉。
//
// 【2. 這個模式的其他常見用途】
//   ● 效能量測（本檔）
//   ● 進入／離開的日誌配對（trace log，永遠成對，不會漏掉 exit）
//   ● 巢狀深度縮排的自動增減
//   ● 交易的自動 rollback（沒有明確 commit 就回復）
//   ● 暫時修改全域狀態，離開時自動還原（例如暫時提高 log level）
//   共同點都是「有進必有出，而且出的那一步不能被跳過」。
//
// 【3. 為什麼用 steady_clock 而不是 high_resolution_clock】
//   量測「經過多久」應該用 **std::chrono::steady_clock**：
//     ● steady_clock 保證單調遞增，不會因為系統校時（NTP、使用者調時鐘、
//       日光節約）而倒退或跳動
//     ● system_clock 代表「牆上時間」，可能被調整，不適合量測區間
//     ● high_resolution_clock 在標準中只是個別名，**實作定義**它到底
//       指向哪一個。在 libstdc++ 上它是 system_clock 的別名，
//       所以用它量測區間其實有被校時影響的風險
//   原始版本用的是 high_resolution_clock，本檔改為 steady_clock，
//   這是量測經過時間的正確選擇。
//
// 【4. 為什麼耗時數字要輸出到 stderr】
//   耗時是**非決定性**的：同一支程式每次執行、每台機器、每種最佳化等級
//   量到的數字都不一樣。本檔把量測結果寫到 **stderr**（標準錯誤），
//   而把可重現的流程訊息寫到 stdout（標準輸出）。
//   這不只是為了讓教材的預期輸出可重現，實務上也是正確做法：
//     ● 診斷／計時資訊屬於 stderr，才不會污染程式的正式輸出
//     ● 使用者可以用 2>/dev/null 關掉診斷，或用 2>timing.log 單獨收集
//     ● 若程式的 stdout 要接到管線給下一個程式吃，混入計時數字會破壞格式
//
// 【5. 量測本身的注意事項】
//   ● 編譯器可能把「沒有副作用的迴圈」整個最佳化掉，量到 0 毫秒。
//     本檔把累加結果印出來，讓那個迴圈有可觀察的效果，避免被消除。
//   ● 單次量測的雜訊很大（快取狀態、CPU 排程、頻率調節）。
//     要比較效能請多次量測取統計值，不要只跑一次就下結論。
//   ● 累加用 long long 而非 int：0 加到 1 億的總和約 5×10^15，
//     遠超過 32 位元 int 的範圍，用 int 會造成有號整數溢位（未定義行為）。
//     這是原始版本潛藏的一個真正問題，本檔已修正。
//
// 【概念補充 Concept Deep Dive】
//
//   ● chrono 的型別安全設計
//     duration_cast<milliseconds> 明確表達「我要毫秒」，
//     而不是拿一個裸 long 猜單位。chrono 用型別把單位帶在身上，
//     混用不同單位會編譯失敗，這是很好的型別設計範例。
//
//   ● 為什麼解構函數裡做輸出要小心
//     若解構發生在堆疊展開（例外傳播）過程中，此時再拋出例外會
//     直接 std::terminate。輸出操作理論上可能拋例外
//     （若對該串流呼叫過 exceptions()），所以嚴謹的實作會把輸出包在
//     try/catch 裡。本檔維持簡潔未包，但實務上值得注意。
//
//   ● 這個模式與「函數進入／離開」的關係
//     ScopedTimer 建立在函數開頭、解構在函數結束，等於自動配對了
//     「進入」與「離開」兩個事件。這比手動在每個 return 前插入
//     結束訊息可靠得多——因為你不可能漏掉任何一條路徑。
//
// 【注意事項 Pay Attention】
//   1. 量測經過時間請用 steady_clock；high_resolution_clock 指向誰是實作定義。
//   2. 耗時數字每次執行都不同，屬非決定性輸出，本檔寫到 stderr。
//   3. 空迴圈可能被最佳化掉；要量測就讓結果有可觀察的用途。
//   4. 累加大量整數請注意型別範圍，有號整數溢位是未定義行為。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】用解構函數做自動化收尾
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. ScopedTimer 這種寫法的價值是什麼？跟手動記錄開始／結束時間差在哪？
//     答：解構函數的呼叫是語言保證的，所以不論函數從哪一條路徑離開
//         （正常 return、提早 return、拋出例外導致堆疊展開），
//         結束時間都一定會被記錄。手動記錄則必須在每一條離開路徑上重複，
//         只要漏一條，那條路徑就沒有量測資料。
//     追問：同樣的模式還能用在哪裡？
//         → 進入／離開日誌配對、縮排深度自動增減、未 commit 就自動 rollback、
//           暫時改變全域狀態並在離開時還原。
//
// 🔥 Q2. 量測一段程式碼耗時，該用哪一個 clock？為什麼？
//     答：std::chrono::steady_clock。它保證單調遞增，不受系統校時
//         （NTP 同步、手動調時鐘、日光節約）影響。system_clock 是牆上時間、
//         可能被往回調；high_resolution_clock 在標準中只是別名，
//         實際指向哪個 clock 是**實作定義**（libstdc++ 上是 system_clock），
//         所以用它量測區間並不安全。
//     追問：那什麼時候該用 system_clock？
//         → 需要「真實世界的時間點」時，例如寫入時間戳、與外部系統對時。
//
// ⚠️ 陷阱. 我用計時器量了一次，發現優化後的版本比較快，這樣就能下結論了吧？
//     答：不能。單次量測的雜訊來源非常多：CPU 頻率調節、快取冷熱、
//         作業系統排程、其他程序競爭。而且編譯器可能把沒有副作用的
//         測試迴圈整個最佳化掉，讓你量到接近 0 的假數字。
//     為什麼會錯：把單次觀測值當成穩定的量測結果。正確做法是多次量測、
//         看中位數與變異，並確認被量測的程式碼真的有被執行
//         （讓結果有可觀察的用途，或使用專門的 benchmark 框架）。
// ═══════════════════════════════════════════════════════════════════════════
//
// VERIFY_STDOUT_ONLY
//   ↑ 此標記告訴驗證工具：本檔的預期輸出只涵蓋 stdout。
//     耗時數字寫到 stderr，因為它每次執行都不同、無法列為固定預期輸出。

#include <iostream>
#include <chrono>
#include <string>
using namespace std;

class ScopedTimer {
private:
    string taskName;
    // 量測經過時間要用 steady_clock（單調遞增，不受系統校時影響）
    chrono::steady_clock::time_point startTime;

public:
    explicit ScopedTimer(const string& name)
        : taskName(name)
        , startTime(chrono::steady_clock::now())
    {
        cout << "  [計時開始] " << taskName << endl;
    }

    ~ScopedTimer() {
        auto endTime = chrono::steady_clock::now();
        auto duration = chrono::duration_cast<chrono::milliseconds>(
            endTime - startTime
        ).count();

        // 流程訊息 → stdout（可重現）
        cout << "  [計時結束] " << taskName << "（耗時數字見 stderr）" << endl;
        // 量測數字 → stderr（每次執行都不同，屬診斷資訊）
        cerr << "  [計時] " << taskName << " 耗時: " << duration << " ms" << endl;
    }
};

// -----------------------------------------------------------------------------
// 被量測的工作
//   注意兩件事：
//   (1) sum 用 long long —— 0 累加到 1 億的總和約 5×10^15，
//       超過 32 位元 int 的範圍，用 int 會造成有號整數溢位（未定義行為）
//   (2) 把 sum 印出來，讓這個迴圈有可觀察的效果，避免被編譯器整個最佳化掉
// -----------------------------------------------------------------------------
void simulateWork() {
    ScopedTimer timer("模擬工作");

    long long sum = 0;
    for (long long i = 0; i < 100000000LL; i++) {
        sum += i;
    }
    cout << "  計算結果: " << sum << endl;

    // timer 在函數返回時自動解構，印出耗時
}

// -----------------------------------------------------------------------------
// 示範：即使提早 return 或拋出例外，計時器一樣會結算
// -----------------------------------------------------------------------------
static void earlyReturnWork(bool bail) {
    ScopedTimer timer(bail ? "提早return的工作" : "正常完成的工作");
    if (bail) {
        cout << "    條件不符，提早 return" << endl;
        return;                       // 解構函數照樣執行
    }
    cout << "    正常跑完" << endl;
}

static void throwingWork() {
    ScopedTimer timer("拋出例外的工作");
    cout << "    即將拋出例外" << endl;
    throw runtime_error("工作失敗");
    // 堆疊展開時 timer 仍會被解構
}

// -----------------------------------------------------------------------------
// 【日常實務範例】巢狀追蹤日誌：進入／離開自動配對，縮排自動還原
//   情境：追查一個多層呼叫的流程為什麼變慢或走錯分支時，最有用的是
//         「每一層進入與離開都成對記錄、並用縮排表現層次」的 trace log。
//   重點：手寫的話，每個 return 前都要記得印「離開」並把縮排減回去，
//         漏一條路徑整份 log 的縮排就全亂了。
//         交給解構函數就不可能漏——這正是 RAII 用在「狀態還原」的典型案例。
// -----------------------------------------------------------------------------
class TraceScope {
private:
    string label_;
    static int depth_;      // 目前巢狀深度（全類別共用）

    static void indent() {
        for (int i = 0; i < depth_; ++i) cout << "  ";
    }

public:
    explicit TraceScope(const string& label) : label_(label) {
        indent();
        cout << "→ 進入 " << label_ << endl;
        ++depth_;                     // 進入時加深
    }

    ~TraceScope() {
        --depth_;                     // 離開時一定還原，不論走哪條路徑
        indent();
        cout << "← 離開 " << label_ << endl;
    }
};
int TraceScope::depth_ = 0;

static bool validateOrder(int qty) {
    TraceScope ts("validateOrder");
    if (qty <= 0) {
        cout << "    數量不合法，提早 return" << endl;
        return false;                 // 縮排仍會被正確還原
    }
    return true;
}

static void chargePayment() {
    TraceScope ts("chargePayment");
    cout << "    扣款完成" << endl;
}

static void handleOrder(int qty) {
    TraceScope ts("handleOrder");
    if (!validateOrder(qty)) {
        cout << "    訂單被拒絕" << endl;
        return;
    }
    chargePayment();
}

int main() {
    cout << "=== 自動計時器範例 ===" << endl;
    simulateWork();

    cout << "\n=== 提早 return 也會結算 ===" << endl;
    earlyReturnWork(false);
    earlyReturnWork(true);

    cout << "\n=== 拋出例外也會結算（堆疊展開）===" << endl;
    try {
        throwingWork();
    } catch (const exception& e) {
        cout << "  攔到例外: " << e.what() << endl;
    }

    cout << "\n=== 日常實務：巢狀追蹤日誌 ===" << endl;
    cout << "  正常訂單：" << endl;
    handleOrder(3);
    cout << "  異常訂單（提早 return，縮排仍正確還原）：" << endl;
    handleOrder(0);

    cout << "\n=== 完成 ===" << endl;
    return 0;
}
// ScopedTimer 的解構函數在離開作用域時自動被呼叫，印出耗時

// 編譯: g++ -std=c++17 -Wall -Wextra "第 17 課：解構函數（Destructor）5.cpp" -o demo5
// 只看流程（隱藏耗時診斷）:  ./demo5 2>/dev/null
// 只收集耗時到檔案:          ./demo5 2>timing.log
//
// ※ 重要說明（放在預期輸出標記之前）：
//   1. 下方預期輸出**只涵蓋 stdout**。每個計時器實際量到的毫秒數會寫到
//      **stderr**，因為那個數字每次執行、每台機器都不同，無法列為固定預期值。
//   2. 「計算結果: 4999999950000000」是 0 累加到 99999999 的確定總和，
//      與執行速度無關，因此是可重現的。

// === 預期輸出 ===
// === 自動計時器範例 ===
//   [計時開始] 模擬工作
//   計算結果: 4999999950000000
//   [計時結束] 模擬工作（耗時數字見 stderr）
//
// === 提早 return 也會結算 ===
//   [計時開始] 正常完成的工作
//     正常跑完
//   [計時結束] 正常完成的工作（耗時數字見 stderr）
//   [計時開始] 提早return的工作
//     條件不符，提早 return
//   [計時結束] 提早return的工作（耗時數字見 stderr）
//
// === 拋出例外也會結算（堆疊展開）===
//   [計時開始] 拋出例外的工作
//     即將拋出例外
//   [計時結束] 拋出例外的工作（耗時數字見 stderr）
//   攔到例外: 工作失敗
//
// === 日常實務：巢狀追蹤日誌 ===
//   正常訂單：
// → 進入 handleOrder
//   → 進入 validateOrder
//   ← 離開 validateOrder
//   → 進入 chargePayment
//     扣款完成
//   ← 離開 chargePayment
// ← 離開 handleOrder
//   異常訂單（提早 return，縮排仍正確還原）：
// → 進入 handleOrder
//   → 進入 validateOrder
//     數量不合法，提早 return
//   ← 離開 validateOrder
//     訂單被拒絕
// ← 離開 handleOrder
//
// === 完成 ===
