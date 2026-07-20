// =============================================================================
//  第 32 課 -3  —  移動建構函數的「自動生成」條件：一個解構函數就能讓它消失
// =============================================================================
//
// 【主題資訊 Information】
//   主題       ： 編譯器何時「自動生成」移動建構函數，何時「不生成」
//   偵測工具   ： std::is_move_constructible_v<T>          （C++17 變數模板）
//                 std::is_nothrow_move_constructible_v<T>  （C++17 變數模板）
//                 —— C++11 起有 ::value 形式，_v 後綴是 C++17 才加的
//   標頭檔     ： <type_traits>
//   標準版本   ： 移動語義本身 C++11；本檔用的 _v 變數模板需 C++17
//   複雜度     ： 全部在編譯期完成，執行期零成本
//
// 【詳細解釋 Explanation】
//
// 【1. 本檔要打破的那個誤解】
//   多數人第一次看到 is_move_constructible_v<NoAutoMove> 印出 true，
//   會以為「它有移動建構函數」。這是本課最大的陷阱：
//       is_move_constructible 問的不是「有沒有移動建構函數」，
//       而是「T t(std::move(x)) 這行能不能編譯過」。
//   而 const T& 的拷貝建構函數是可以接住右值的 —— 右值能繫結到 const 左值參考。
//   所以即使移動建構函數完全不存在，這個 trait 一樣回答 true。
//   真正能分辨兩者的是 is_nothrow_move_constructible_v：
//   自動生成的移動建構會沿用成員的 noexcept 性質（string 的移動是 noexcept）
//   → true；而退回拷貝時，string 的拷貝要配置記憶體、可能拋 bad_alloc
//   → false。本檔的輸出正是這組對比。
//
// 【2. 什麼會「殺掉」自動生成的移動建構函數】
//   只要你自己宣告了下列任何一項，編譯器就不再自動生成移動建構函數：
//       * 解構函數（本檔的 NoAutoMove 就是這一項）
//       * 拷貝建構函數
//       * 拷貝賦值運算子
//       * 另一個移動操作（移動賦值）
//   注意「宣告」就夠了，不需要寫出內容 —— NoAutoMove 的 ~NoAutoMove() {}
//   是空的，照樣讓移動建構函數消失。連 = default 都算宣告。
//   這條規則的動機是安全：你既然要親手管解構，編譯器就不敢假設
//   「逐成員搬移」對你的類別是正確的（見第 27 課淺拷貝的教訓）。
//
// 【3. 消失之後不會報錯，只會變慢 —— 這才是危險的地方】
//   移動建構函數不見了，std::move(x) 依然編得過，
//   只是重載解析安靜地退回 const T&（拷貝）。
//   沒有錯誤、沒有警告，只有效能默默掉下去。
//   本檔的【日常實務範例】把這個代價量化出來：
//   同樣的 4 次 push_back，有移動的版本做 7 次移動，
//   被解構函數殺掉移動的版本做 7 次深拷貝。
//
// 【4. 為什麼 vector 特別在意「noexcept」而不只是「有沒有移動」】
//   std::vector 擴充容量時必須把舊元素搬到新記憶體。
//   若搬到一半拋例外，舊緩衝區已經被破壞、新的又不完整 —— 無法回復。
//   所以 vector 用 std::move_if_noexcept：
//       移動建構是 noexcept  → 用移動（快，且不可能失敗）
//       移動建構可能拋例外  → 寧可用拷貝（慢，但失敗時舊資料還在，可回復）
//   結論：移動建構函數不寫 noexcept，等於白寫 —— vector 不會用它。
//   這就是為什麼本檔要同時印出兩個 trait，而不是只印 is_move_constructible。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 「Rule of Zero」為什麼是預設建議
//   NoAutoMove 的問題根源是它宣告了解構函數。但它其實沒有需要手動釋放的資源
//   （成員是 std::string，會自己管自己）。這種空解構函數是純粹的負債：
//   沒有帶來任何好處，卻關掉了移動、還讓類別不再是 trivially destructible。
//   Rule of Zero：讓成員自己管資源，五個特殊成員函式一個都別寫。
//   對照第 34 課 —— 真的持有裸資源時才適用 Rule of Five，
//   而那時你必須把五個都寫齊，正因為寫了一個就會關掉其他的。
//
// (B) 拷貝操作在這裡是「已棄用但仍生成」
//   宣告解構函數之後，拷貝建構與拷貝賦值仍會被自動生成，
//   但自 C++11 起這個行為已被標準標記為 deprecated。
//   所以 NoAutoMove 才會 is_move_constructible = true（靠這個拷貝接住右值）。
//   未來標準若真的移除，這類程式碼會直接編不過 —— 又一個別留空解構的理由。
//
// (C) 這些 trait 都不會「真的呼叫」建構函數
//   is_*_constructible 只做重載解析與 noexcept 查詢，全在編譯期。
//   它不會實例化函式本體，所以就算建構函數的實作內部有錯，trait 也照樣回答。
//   換句話說：trait 回答 true 只代表「這行語法上選得到某個建構函數」，
//   不代表那個建構函數的語義是對的。
//
// 【注意事項 Pay Attention】
//   1. is_move_constructible_v 為 true ≠ 有移動建構函數。
//      它只是說「能用右值建構」，拷貝建構也算數。
//   2. 想確認「真的會走移動」，看 is_nothrow_move_constructible_v，
//      或直接數呼叫次數（本檔實務範例的做法）。
//   3. 自己宣告解構／拷貝建構／拷貝賦值／移動賦值任一項，
//      移動建構就不再自動生成。宣告即可，不必有內容。
//   4. 移動建構函數務必加 noexcept，否則 vector 擴充時不會採用它。
//   5. 本檔輸出中 vector 的 capacity 序列（1→2→4）是 libstdc++ 的
//      2 倍成長策略，屬實作定義；MSVC 用 1.5 倍，數字會不同。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】移動建構函數的自動生成
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 哪些情況下編譯器不會自動生成移動建構函數？
//     答：自己宣告了解構函數、拷貝建構函數、拷貝賦值運算子、
//         或移動賦值運算子 —— 四者任一即可。「宣告」就足夠，
//         空的 ~T() {} 甚至 ~T() = default; 都會讓它消失。
//         理由是安全：你既然要手動介入資源管理，
//         編譯器就不敢假設逐成員搬移對你的類別是正確的。
//     追問：那沒有移動建構函數之後，std::move(x) 會編譯錯誤嗎？
//         → 不會。它安靜退回 const T& 的拷貝版本，
//           沒有錯誤也沒有警告，只有效能悄悄變差。
//
// 🔥 Q2. 移動建構函數為什麼一定要加 noexcept？
//     答：std::vector 擴充容量時用 std::move_if_noexcept 搬移元素。
//         若移動可能拋例外，搬到一半就無法回復（舊緩衝已破壞、新的不完整），
//         所以 vector 會保守地改用拷貝以保住強例外保證。
//         結論：移動建構不標 noexcept，vector 根本不會採用它，等於白寫。
//     追問：那 std::vector 怎麼知道會不會拋？
//         → 靠 std::is_nothrow_move_constructible 這個編譯期 trait 查詢，
//           完全在編譯期決定，執行期沒有任何判斷成本。
//
// ⚠️ 陷阱 1. is_move_constructible_v<T> 是 true，所以 T 有移動建構函數？
//     答：不成立。這個 trait 問的是「T t(std::move(x)) 能不能編譯」，
//         而 const T& 的拷貝建構函數可以接住右值，所以拷貝也算數。
//         本檔的 NoAutoMove 沒有任何移動建構函數，
//         is_move_constructible_v 仍然是 true，
//         但 is_nothrow_move_constructible_v 是 false —— 那才露出馬腳。
//     為什麼會錯：把 trait 的名字當成「存在性檢查」，
//         但它其實是「可行性檢查」—— 問的是這個語法成不成立，
//         不是問哪個函式被選中。
//
// ⚠️ 陷阱 2. 「加一個空的解構函數無傷大雅，反正它什麼都沒做。」
//     答：它做了一件大事：關掉移動建構與移動賦值的自動生成。
//         本檔實務範例中，同樣 4 次 push_back，
//         有移動的版本做 7 次移動，加了空解構的版本做 7 次深拷貝。
//         而且這個類別的成員是 std::string，本來就會自己釋放，
//         那個解構函數純粹是負債。
//     為什麼會錯：以為特殊成員函式的影響僅限於自己，
//         實際上它們互相牽動 —— 這正是 Rule of Zero／Rule of Five
//         要處理的問題（見第 34 課）。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】—— 本檔從缺，理由如下
//   「特殊成員函式的自動生成規則」是編譯期的語言規則，
//   LeetCode 判的是演算法的輸入輸出正確性與時間複雜度，
//   不會去檢查你的類別有沒有生成移動建構函數。
//   硬套一題設計類題目（例如 146. LRU Cache）只會讓讀者誤以為
//   兩者有關，反而模糊焦點。這裡改以「vector 擴充時的搬移成本」
//   這個真實可量測的場景呈現，比湊題目誠實。
//
// -----------------------------------------------------------------------------
// lesson32_auto_generation.cpp
// -----------------------------------------------------------------------------

#include <iostream>
#include <string>
#include <vector>
#include <type_traits>

// 情況 1：什麼都沒寫 → 編譯器自動生成移動建構
class AutoMove {
    std::string m_name;
    int m_value;
public:
    AutoMove(std::string name, int val) : m_name(std::move(name)), m_value(val) {}
};

// 情況 2：寫了解構函數 → 編譯器不自動生成移動建構
class NoAutoMove {
    std::string m_name;
    int m_value;
public:
    NoAutoMove(std::string name, int val) : m_name(std::move(name)), m_value(val) {}
    ~NoAutoMove() {}  // 自定義解構 → 移動建構不會自動生成
};

// -----------------------------------------------------------------------------
// 【日常實務範例】把 log 緩衝區放進 std::vector
//   情境：服務把每一批要落盤的 log 先收在 std::vector<LogBuffer> 裡，
//         批次滿了再一次寫檔。vector 沒有事先 reserve 時會反覆擴充容量，
//         每次擴充都要把既有元素「搬」到新的記憶體。
//   為什麼用到本主題：搬移用移動還是拷貝，取決於元素型別有沒有
//         「noexcept 的移動建構函數」。下面兩個類別的差別只有一行 ——
//         BadBuffer 多了一個空的解構函數 —— 就足以讓移動消失、
//         整批 log 內容被逐字元深拷貝。
//   量測方式：刻意用「計數」而不是「計時」。計時受 CPU 排程與快取影響，
//         每次跑都不一樣；計數是決定性的，可以直接寫進預期輸出。
// -----------------------------------------------------------------------------
namespace practice {

int g_copies = 0;
int g_moves  = 0;

// 真正持有 payload 的成員型別，負責計數。
// 注意：GoodBuffer / BadBuffer 自己都「沒有」寫拷貝或移動建構函數，
//       它們的拷貝／移動是編譯器生成的，再轉呼叫到這裡 —— 這才是本課主題。
struct Payload {
    std::string bytes;
    explicit Payload(std::string s) : bytes(std::move(s)) {}
    Payload(const Payload& o) : bytes(o.bytes)                { ++g_copies; }
    Payload(Payload&& o) noexcept : bytes(std::move(o.bytes)) { ++g_moves;  }
};

// 沒有自訂解構 → 移動建構自動生成，且因成員的移動是 noexcept 而同為 noexcept
class GoodBuffer {
    Payload m_payload;
public:
    explicit GoodBuffer(std::string s) : m_payload(std::move(s)) {}
};

// 只多了一個空解構函數 → 移動建構不再生成，vector 擴充時退回深拷貝
class BadBuffer {
    Payload m_payload;
public:
    explicit BadBuffer(std::string s) : m_payload(std::move(s)) {}
    ~BadBuffer() {}          // ← 罪魁禍首，而且它什麼都沒做
};

template <typename T>
void collectBatch(const char* tag) {
    g_copies = 0;
    g_moves  = 0;
    std::vector<T> batch;                 // 刻意不 reserve，讓它自然擴充
    std::cout << "  " << tag
              << "  nothrow 可移動建構 = "
              << std::is_nothrow_move_constructible_v<T> << "\n";
    for (int i = 0; i < 4; ++i) {
        batch.push_back(T("log-line-" + std::to_string(i)));
        std::cout << "    第 " << (i + 1) << " 筆後 capacity=" << batch.capacity()
                  << "  累計 拷貝=" << g_copies
                  << " 移動=" << g_moves << "\n";
    }
}

}  // namespace practice

int main() {
    std::cout << std::boolalpha;

    std::cout << "=== 1. 兩個 trait 問的其實不是同一件事 ===\n";
    std::cout << "AutoMove 可移動建構？ "
              << std::is_move_constructible_v<AutoMove> << "\n";
    std::cout << "AutoMove 有 nothrow 移動建構？ "
              << std::is_nothrow_move_constructible_v<AutoMove> << "\n";

    std::cout << "\nNoAutoMove 可移動建構？ "
              << std::is_move_constructible_v<NoAutoMove> << "\n";
    std::cout << "NoAutoMove 有 nothrow 移動建構？ "
              << std::is_nothrow_move_constructible_v<NoAutoMove> << "\n";
    // NoAutoMove 仍然「可移動建構」，但會退回使用拷貝建構（const T&）
    // 沒有真正的移動建構函數 —— 上面兩行 true / false 的落差就是證據。

    std::cout << "\n=== 2. 日常實務：一個空解構函數的實際代價 ===\n";
    practice::collectBatch<practice::GoodBuffer>("GoodBuffer（無自訂解構）");
    std::cout << "\n";
    practice::collectBatch<practice::BadBuffer>("BadBuffer （有空解構）  ");

    std::cout << "\n=== 3. 結論 ===\n";
    std::cout << "  同樣 4 次 push_back：GoodBuffer 全走移動，BadBuffer 全走深拷貝。\n";
    std::cout << "  差別只有一行空的 ~BadBuffer() {}，而且它什麼事都沒做。\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -Wreorder "第 32 課：移動建構函數（Move Constructor）3.cpp" -o lesson32c

// ── 輸出說明（讀之前先看這裡）────────────────────────────────────────────
// * 本檔輸出完全決定性：全部是編譯期 trait 與計數器，沒有位址、
//   沒有耗時、沒有執行緒（本機實測連跑 5 次逐位元組相同）。
//   刻意用「數呼叫次數」而非「量時間」—— 計時每次都不同，計數可重現。
// * capacity 序列 1→2→4 是 libstdc++ 的 2 倍成長策略，屬實作定義：
//   MSVC 採 1.5 倍，數列會變成 1→2→3→4，累計次數也會跟著不同。
//   會變的是數字，不變的是「Good 全走移動、Bad 全走拷貝」這個對比。
// * 7 次的組成：4 次是把 push_back 的暫時物件搬進容器槽位，
//   3 次是擴充容量時重新安置既有元素（1 + 2）。
// * NoAutoMove 那組 true / false 不是矛盾：
//   is_move_constructible 問「能不能用右值建構」（拷貝建構也接得住右值，故 true），
//   is_nothrow_move_constructible 才問「那條路徑保證不拋嗎」
//   （走拷貝要配置記憶體、可能拋 bad_alloc，故 false）。
// ─────────────────────────────────────────────────────────────────────────

// === 預期輸出 ===
// === 1. 兩個 trait 問的其實不是同一件事 ===
// AutoMove 可移動建構？ true
// AutoMove 有 nothrow 移動建構？ true
//
// NoAutoMove 可移動建構？ true
// NoAutoMove 有 nothrow 移動建構？ false
//
// === 2. 日常實務：一個空解構函數的實際代價 ===
//   GoodBuffer（無自訂解構）  nothrow 可移動建構 = true
//     第 1 筆後 capacity=1  累計 拷貝=0 移動=1
//     第 2 筆後 capacity=2  累計 拷貝=0 移動=3
//     第 3 筆後 capacity=4  累計 拷貝=0 移動=6
//     第 4 筆後 capacity=4  累計 拷貝=0 移動=7
//
//   BadBuffer （有空解構）    nothrow 可移動建構 = false
//     第 1 筆後 capacity=1  累計 拷貝=1 移動=0
//     第 2 筆後 capacity=2  累計 拷貝=3 移動=0
//     第 3 筆後 capacity=4  累計 拷貝=6 移動=0
//     第 4 筆後 capacity=4  累計 拷貝=7 移動=0
//
// === 3. 結論 ===
//   同樣 4 次 push_back：GoodBuffer 全走移動，BadBuffer 全走深拷貝。
//   差別只有一行空的 ~BadBuffer() {}，而且它什麼事都沒做。
