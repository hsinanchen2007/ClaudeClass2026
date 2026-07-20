// =============================================================================
//  第 17 課：vector 的記憶體重新配置機制 3  —  重新配置時的「移動 vs 拷貝」抉擇
// =============================================================================
//
// 【主題資訊 Information】
//   template <class T>
//   constexpr typename std::conditional<
//         !std::is_nothrow_move_constructible<T>::value
//      &&  std::is_copy_constructible<T>::value,
//         const T&,        // → 退回拷貝
//         T&&              // → 使用移動
//   >::type move_if_noexcept(T& x) noexcept;
//
//   標頭檔  ：<utility>（move_if_noexcept）、<vector>
//   標準版本：std::move_if_noexcept 為 C++11（本機以 -pedantic-errors 驗證：
//             c++98 失敗、c++11 起通過）
//   相關特徵：std::is_nothrow_move_constructible<T>（<type_traits>, C++11）
//
// 【詳細解釋 Explanation】
//
// 【1. 問題：重新配置搬到一半失敗了怎麼辦】
//   vector 的 push_back 提供「強例外保證（strong exception guarantee）」：
//     要嘛整個操作成功，要嘛 vector 完全維持原狀，不存在中間狀態。
//   重新配置時要把 n 個元素從舊區搬到新區。假設搬到第 k 個時丟出例外：
//     * 若用「拷貝」搬：舊區的元素從頭到尾都沒被動過，
//       只要把新區已建構的 k 個銷毀、釋放新區，vector 就完好如初 → 保證達成。
//     * 若用「移動」搬：前 k 個舊元素已經被掏空（moved-from）了，
//       而「把它們搬回去」本身也可能再丟例外 → 無法復原 → 保證破功。
//   所以：只有在「移動絕對不會丟例外」時，用移動才是安全的。
//
// 【2. 解法：std::move_if_noexcept 的三分支邏輯】
//   vector 內部不是直接用 std::move，而是用 std::move_if_noexcept，規則是：
//     (a) T 的移動建構子是 noexcept          → 回傳 T&&      → 移動（快，且安全）
//     (b) 移動不是 noexcept，但 T 可以拷貝    → 回傳 const T& → 拷貝（慢，但安全）
//     (c) 移動不是 noexcept，且 T 不可拷貝    → 回傳 T&&      → 只能移動
//   分支 (c) 常被忽略：對 move-only 型別（如 std::unique_ptr 的容器、
//   含 std::mutex 成員的型別）根本沒有拷貝可退，實作只好用移動——
//   此時強例外保證會降級為 basic guarantee。標準對此的措辭是：
//   「若元素型別不是 CopyInsertable 且移動建構子非 noexcept，效果未指定」。
//
// 【3. 這一個 noexcept 造成的效能差距有多大】
//   假設 vector<std::string> 有 100 萬個字串，發生一次重新配置：
//     * 移動：每個字串只搬指標 + 長度 + 容量（本機 sizeof(std::string)==32），
//       且不觸碰堆積上的字元資料 → 100 萬次淺層搬移。
//     * 拷貝：每個字串都要 new 一塊新記憶體 + memcpy 字元內容
//       → 100 萬次 heap 配置 + 100 萬次 memcpy。
//   差距經常是一個數量級以上。而觸發它的，只是你有沒有在移動建構子
//   後面多打七個字母 noexcept。
//   標準庫型別（std::string、std::vector、std::unique_ptr…）都已標好
//   noexcept，本機實測 is_nothrow_move_constructible<std::string> 為 true。
//
// 【4. 什麼時候「編譯器自動產生的」移動建構子會是 noexcept】
//   = default 或編譯器隱式產生的移動建構子，其 noexcept 是「推導」出來的：
//   只要所有成員與基底的移動建構都是 noexcept，它就是 noexcept。
//   所以純由 std::string / int / std::vector 組成的聚合型別，
//   通常不用手寫就自動獲得 noexcept 移動。
//   但只要你「手寫」了移動建構子而忘了寫 noexcept，就會掉進分支 (b)——
//   這正是本檔要示範的陷阱。
//
// 【概念補充 Concept Deep Dive】
//   為什麼「移動」對 std::string 這麼便宜？因為 std::string 的移動只是
//   把三個欄位搬過來、再把來源設成空殼：
//
//       來源 s1                       目的 s2（移動後）
//       ┌────────┬──────┬──────┐      ┌────────┬──────┬──────┐
//       │ ptr    │ size │ cap  │      │ ptr    │ size │ cap  │
//       └───┬────┴──────┴──────┘      └───┬────┴──────┴──────┘
//           │  (移動後被設為空)             │  (直接接手同一塊堆積記憶體)
//           X ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─▶ heap: "hello world..."
//
//   堆積上的字元資料【完全沒有被複製】，只是換了一個主人。
//   這種「只搬所有權、不搬資料」的操作天生不需要配置記憶體，
//   因此可以、也應該標記為 noexcept。
//   反過來說，如果你的移動建構子裡真的會配置記憶體或呼叫可能丟例外的
//   函式，那它就不該被標成 noexcept——標了卻丟出例外會直接 std::terminate。
//
// 【注意事項 Pay Attention】
//   1. 自訂型別的移動建構子「務必」標記 noexcept，否則 vector 擴容時
//      會默默退化成拷貝。這是效能問題，不是編譯錯誤，很難靠測試發現。
//   2. 不要為了效能亂標 noexcept。若函式實際上可能丟例外，標了之後
//      一旦丟出，程式會直接 std::terminate（不是 UB，是明確定義的終止）。
//   3. 移動建構子若有可能丟例外，正解是「讓它不會丟」（例如改成只搬指標），
//      而不是硬標 noexcept。
//   4. 下方輸出中的建構／移動／銷毀「順序與次數」取決於本機 libstdc++ 的
//      實作細節（例如 push_back(暫存物件) 會先建構暫存再移入），
//      換實作可能略有差異；但「Tracker 走移動、CopyableSlow 走拷貝」
//      這個結論是由 move_if_noexcept 的規則保證的，不因實作而異。
//   5. 本檔的 Tracker 移動後 name_ 被掏空，解構時印不出名字——這正是
//      moved-from 狀態「有效但未指定」的具體表現。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】noexcept 移動與 move_if_noexcept
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼自訂類別的移動建構子一定要加 noexcept？
//     答：vector 重新配置時用 std::move_if_noexcept 決定搬移方式。
//         移動建構子若沒標 noexcept，而型別又可拷貝，vector 會退回「拷貝」
//         以維持強例外保證。結果是：你以為有移動語意，實際上每次擴容
//         都在做深拷貝，效能差一個數量級，而且完全不會編譯錯誤。
//     追問：那 vector 為什麼不乾脆一律用移動？
//         → 因為移動搬到一半丟例外時，舊元素已被掏空、無法復原，
//           push_back 承諾的強例外保證就無法兌現。
//
// 🔥 Q2. 一個「移動建構子非 noexcept」且「刪除了拷貝建構子」的型別，
//        vector 擴容時會怎麼做？
//     答：只能用移動（move_if_noexcept 的第三個分支：無拷貝可退）。
//         此時強例外保證降級為 basic guarantee。本機實測確認：
//         move-only 且 move 非 noexcept 的型別，擴容時仍走移動。
//     追問：那這樣安全嗎？→ 不會 UB，但擴容中途丟例外時 vector 的內容
//         就不保證還原了，所以更該把移動寫成 noexcept。
//
// ⚠️ 陷阱. 「我寫了移動建構子，所以 vector 擴容時一定會用移動」——錯在哪？
//     答：只有標了 noexcept（或型別不可拷貝）才會用移動。
//         少寫 noexcept + 型別可拷貝 = vector 靜靜地改用拷貝。
//         本檔的 CopyableSlow 就是這個情形：明明有移動建構子，
//         輸出卻全是「拷貝」。
//     為什麼會錯：以為「有沒有移動語意」是看「有沒有寫移動建構子」。
//         實際上 vector 看的是 is_nothrow_move_constructible——
//         是「會不會丟例外」，不是「有沒有」。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <string>
#include <utility>
#include <type_traits>

// ── 示範型別 1：移動建構子有標 noexcept（正確做法）────────────────────────
class Tracker {
    std::string name_;
public:
    Tracker(const std::string& name) : name_(name) {
        std::cout << "  建構: " << name_ << std::endl;
    }

    // 拷貝建構子
    Tracker(const Tracker& other) : name_(other.name_) {
        std::cout << "  拷貝: " << name_ << std::endl;
    }

    // 移動建構子（標記 noexcept）→ vector 擴容時會選它
    Tracker(Tracker&& other) noexcept : name_(std::move(other.name_)) {
        std::cout << "  移動: " << name_ << std::endl;
    }

    ~Tracker() {
        std::cout << "  銷毀: " << name_ << std::endl;
    }
};

// ── 示範型別 2：移動建構子「忘了」標 noexcept（常見錯誤）──────────────────
class CopyableSlow {
    std::string name_;
public:
    CopyableSlow(const std::string& name) : name_(name) {}
    CopyableSlow(const CopyableSlow& other) : name_(other.name_) {
        std::cout << "  拷貝: " << name_ << "（← 擴容時被迫走這裡）" << std::endl;
    }
    // 注意：這裡沒有 noexcept！
    CopyableSlow(CopyableSlow&& other) : name_(std::move(other.name_)) {
        std::cout << "  移動: " << name_ << std::endl;
    }
};

// ── 示範型別 3：move-only，且移動非 noexcept（第三分支）────────────────────
class MoveOnly {
    std::string name_;
public:
    MoveOnly(const std::string& name) : name_(name) {}
    MoveOnly(const MoveOnly&) = delete;              // 不可拷貝
    MoveOnly(MoveOnly&& other) : name_(std::move(other.name_)) {   // 非 noexcept
        std::cout << "  移動: " << name_ << "（無拷貝可退，只能移動）" << std::endl;
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】批次載入交易紀錄：一個 noexcept 決定要不要做百萬次深拷貝
//   情境：每筆交易含商品名稱等字串欄位，從資料來源逐筆 push_back 進 vector。
//         若 Record 的移動建構子沒標 noexcept，每次擴容都會把所有字串
//         深拷貝一次；標了就只搬指標。
//   本函式用 type traits 在「編譯期」把這個判斷印出來，
//   讓這種靜默的效能問題在 code review 時看得見。
// -----------------------------------------------------------------------------
struct RecordGood {
    std::string sku;
    std::string buyer;
    long        amount = 0;
    // 全部成員的移動都是 noexcept → 隱式產生的移動建構子自動是 noexcept
};

struct RecordBad {
    std::string sku;
    std::string buyer;
    long        amount = 0;
    RecordBad() = default;
    // 手寫移動建構子卻忘了 noexcept → vector 擴容時退化成拷貝
    RecordBad(RecordBad&& o) : sku(std::move(o.sku)), buyer(std::move(o.buyer)),
                               amount(o.amount) {}
    RecordBad(const RecordBad&) = default;
};

template <typename T>
void report_realloc_strategy(const char* type_name) {
    const bool nothrow_move = std::is_nothrow_move_constructible<T>::value;
    const bool copyable     = std::is_copy_constructible<T>::value;
    const char* strategy =
        nothrow_move ? "移動 (快)"
                     : (copyable ? "拷貝 (慢！退化了)" : "移動 (無拷貝可退)");
    std::cout << "  " << type_name
              << "：is_nothrow_move_constructible=" << (nothrow_move ? "true" : "false")
              << ", is_copy_constructible=" << (copyable ? "true" : "false")
              << " → 擴容時採用 " << strategy << std::endl;
}

int main() {
    std::cout << "=== 一、Tracker（移動建構子有 noexcept）===" << std::endl;
    {
        std::cout << "--- 建立 vector，reserve(2) ---" << std::endl;
        std::vector<Tracker> v;
        v.reserve(2);

        std::cout << "\n--- push_back A ---" << std::endl;
        v.push_back(Tracker("A"));

        std::cout << "\n--- push_back B ---" << std::endl;
        v.push_back(Tracker("B"));

        std::cout << "\n--- push_back C（size==capacity，觸發重新配置）---" << std::endl;
        v.push_back(Tracker("C"));
        std::cout << "  ↑ 舊有的 A、B 是被「移動」到新緩衝區的" << std::endl;

        std::cout << "\n--- 離開作用域，銷毀 vector ---" << std::endl;
    }

    std::cout << "\n=== 二、CopyableSlow（移動建構子忘了 noexcept）===" << std::endl;
    {
        std::vector<CopyableSlow> v;
        v.reserve(2);
        v.emplace_back("X");
        v.emplace_back("Y");
        std::cout << "--- 觸發重新配置 ---" << std::endl;
        v.emplace_back("Z");
        std::cout << "  ↑ 明明寫了移動建構子，擴容卻走「拷貝」" << std::endl;
    }

    std::cout << "\n=== 三、MoveOnly（不可拷貝，移動也非 noexcept）===" << std::endl;
    {
        std::vector<MoveOnly> v;
        v.reserve(2);
        v.emplace_back("P");
        v.emplace_back("Q");
        std::cout << "--- 觸發重新配置 ---" << std::endl;
        v.emplace_back("R");
        std::cout << "  ↑ 沒有拷貝可退，只好用移動（強例外保證降級）" << std::endl;
    }

    std::cout << "\n=== 四、move_if_noexcept 的回傳型別（編譯期驗證）===" << std::endl;
    {
        Tracker      t("probe");
        CopyableSlow c("probe");
        // 有 noexcept 移動 → 回傳右值參考（會被移動）
        static_assert(std::is_same<decltype(std::move_if_noexcept(t)), Tracker&&>::value,
                      "Tracker 應回傳 Tracker&&");
        // 無 noexcept 移動但可拷貝 → 回傳 const 左值參考（會被拷貝）
        static_assert(std::is_same<decltype(std::move_if_noexcept(c)),
                                   const CopyableSlow&>::value,
                      "CopyableSlow 應回傳 const CopyableSlow&");
        std::cout << "  static_assert 全數通過："
                  << "Tracker → T&&（移動）、CopyableSlow → const T&（拷貝）" << std::endl;
    }

    std::cout << "\n=== 五、日常實務：交易紀錄型別的擴容策略體檢 ===" << std::endl;
    report_realloc_strategy<RecordGood>("RecordGood（全部交給編譯器產生）");
    report_realloc_strategy<RecordBad> ("RecordBad （手寫移動但漏 noexcept）");
    report_realloc_strategy<std::string>("std::string             ");
    std::cout << "建議：把這種 static_assert 放進單元測試，"
              << "讓「漏寫 noexcept」變成編譯期錯誤而不是靜默的效能退化。" << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 17 課：vector 的記憶體重新配置機制3.cpp" -o move_vs_copy

// 註：下方建構／移動／銷毀的「順序與次數」屬本機 GCC 15.2 / libstdc++ 的
//     實作細節（例如暫存物件的建構時機），換標準函式庫可能略有出入；
//     但「有 noexcept → 移動、無 noexcept 且可拷貝 → 拷貝」是標準保證的行為。

// === 預期輸出 ===
// === 一、Tracker（移動建構子有 noexcept）===
// --- 建立 vector，reserve(2) ---
//
// --- push_back A ---
//   建構: A
//   移動: A
//   銷毀:
//
// --- push_back B ---
//   建構: B
//   移動: B
//   銷毀:
//
// --- push_back C（size==capacity，觸發重新配置）---
//   建構: C
//   移動: C
//   移動: A
//   銷毀:
//   移動: B
//   銷毀:
//   銷毀:
//   ↑ 舊有的 A、B 是被「移動」到新緩衝區的
//
// --- 離開作用域，銷毀 vector ---
//   銷毀: A
//   銷毀: B
//   銷毀: C
//
// === 二、CopyableSlow（移動建構子忘了 noexcept）===
// --- 觸發重新配置 ---
//   拷貝: X（← 擴容時被迫走這裡）
//   拷貝: Y（← 擴容時被迫走這裡）
//   ↑ 明明寫了移動建構子，擴容卻走「拷貝」
//
// === 三、MoveOnly（不可拷貝，移動也非 noexcept）===
// --- 觸發重新配置 ---
//   移動: P（無拷貝可退，只能移動）
//   移動: Q（無拷貝可退，只能移動）
//   ↑ 沒有拷貝可退，只好用移動（強例外保證降級）
//
// === 四、move_if_noexcept 的回傳型別（編譯期驗證）===
//   建構: probe
//   static_assert 全數通過：Tracker → T&&（移動）、CopyableSlow → const T&（拷貝）
//   銷毀: probe
//
// === 五、日常實務：交易紀錄型別的擴容策略體檢 ===
//   RecordGood（全部交給編譯器產生）：is_nothrow_move_constructible=true, is_copy_constructible=true → 擴容時採用 移動 (快)
//   RecordBad （手寫移動但漏 noexcept）：is_nothrow_move_constructible=false, is_copy_constructible=true → 擴容時採用 拷貝 (慢！退化了)
//   std::string             ：is_nothrow_move_constructible=true, is_copy_constructible=true → 擴容時採用 移動 (快)
// 建議：把這種 static_assert 放進單元測試，讓「漏寫 noexcept」變成編譯期錯誤而不是靜默的效能退化。
