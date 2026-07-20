// =============================================================================
//  第 32 課 -2  —  移動建構函數的 noexcept：不寫它，vector 就當它不存在
// =============================================================================
//
// 【主題資訊 Information】
//   主題       ： 為什麼移動建構函數一定要標 noexcept
//   關鍵機制   ： std::move_if_noexcept —— vector 重新配置時的取捨依據
//   判斷依據   ： std::is_nothrow_move_constructible<T>（編譯期 trait）
//   標準版本   ： C++11（移動語義與 noexcept 同期引入）
//   標頭檔     ： <vector>、<utility>；本檔另用 <cstring> 管裸 char*
//   影響範圍   ： vector 擴容、resize、insert；以及其他要求強例外保證的操作
//
// 【詳細解釋 Explanation】
//
// 【1. 問題的起點：vector 擴容時的「半途而廢」風險】
//   vector 容量滿了要擴充時，必須做三件事：
//       配置新的、更大的記憶體 → 把既有元素搬過去 → 釋放舊記憶體
//   關鍵在中間那步。假設有 10 個元素，搬到第 6 個時拋出例外：
//       * 前 5 個已經被「搬空」了 —— 原本的資料不在舊緩衝區裡了
//       * 新緩衝區只有 5 個完整元素，其餘是未建構的空間
//   此時舊的回不去、新的不完整，vector 無法回復到操作前的狀態。
//   而標準要求 push_back 提供「強例外保證」：要嘛成功，要嘛完全沒發生。
//
// 【2. 解法：move_if_noexcept —— 用型別資訊在編譯期做選擇】
//   標準庫的處理方式不是執行期 try/catch，而是編譯期查詢型別：
//       移動建構是 noexcept  → 用移動。快，而且保證不會拋，不可能半途失敗。
//       移動建構可能拋例外  → 改用拷貝。慢，但拷貝不會破壞來源；
//                              搬到一半失敗時舊緩衝區仍然完整，直接放棄新的即可。
//   這就是 std::move_if_noexcept 的全部邏輯，且完全在編譯期決定，
//   執行期沒有任何判斷成本。
//   結論很直接：移動建構函數不標 noexcept，vector 根本不會採用它。
//   你寫了移動建構函數、也確實跑得比較快，卻永遠不會被呼叫到 —— 等於白寫。
//
// 【3. 本檔怎麼證明這件事】
//   兩個類別的差別只有「移動建構函數後面那個 noexcept」，其他一字不差。
//   同樣的 reserve(2) + 三次 push_back，第三次觸發擴容：
//       有 noexcept  → 擴容時搬 Alpha / Beta 印出 [移動]
//       沒有 noexcept → 同樣的位置改印 [拷貝]
//   這是同一份程式邏輯下、只因一個關鍵字而分岔的兩條路徑。
//
// 【4. 一個容易看漏的細節：Gamma 為什麼兩邊都是「移動」】
//   看輸出會發現，即使在沒有 noexcept 的版本，
//   新元素 Gamma 仍然印 [移動]，只有既有的 Alpha / Beta 變成 [拷貝]。
//   原因是這兩件事的性質不同：
//       * 把 push_back 收到的暫時物件放進新槽位 —— 來源是個右值暫時物件，
//         就算失敗也沒有「原本的資料」需要保護，直接移動即可。
//       * 重新安置既有元素 —— 這些是 vector 已經對外承諾存在的資料，
//         破壞它們就回不去了，所以才需要 move_if_noexcept 把關。
//   看懂這個差別，就真的懂 noexcept 在這裡保護的是什麼了。
//
// 【概念補充 Concept Deep Dive】
//
// (A) noexcept 不是「請求」，是「承諾」
//   noexcept 不會讓編譯器幫你檢查函式內部真的不拋例外。
//   它是你對編譯器與標準庫下的保證。若標了 noexcept 的函式真的拋出例外，
//   不會轉成其他例外、也不是未定義行為 —— 標準規定會呼叫 std::terminate()
//   直接結束程式（這點與 UB 不同，是有明確定義的行為）。
//   所以只在「確實不會拋」時才標：移動建構通常只搬指標、不配置記憶體，
//   天生符合條件；而拷貝建構要配置記憶體、可能拋 bad_alloc，不能標。
//
// (B) 為什麼「拷貝」在失敗時是安全的
//   拷貝不修改來源。搬到一半拋例外時，舊緩衝區裡每個元素都還完好，
//   vector 只要銷毀新緩衝區已建好的部分、釋放它，
//   然後把例外往外丟即可 —— 對外看起來就像什麼都沒發生。
//   這就是為什麼「慢但安全」的拷貝會被優先於「快但可能半途失敗」的移動。
//
// (C) 自動生成的移動建構函數會自己推導 noexcept
//   若你不手寫、讓編譯器生成，它會依據所有成員與基底類別的移動操作
//   是否都是 noexcept 來決定自己標不標。成員都是 std::string / int 這類
//   noexcept 可移動的型別時，生成的版本就是 noexcept。
//   這也是 Rule of Zero 的另一個好處：連 noexcept 都不用自己煩惱。
//   （見本課 -3 檔：一旦自己宣告了解構函數，這個自動生成就消失了。）
//
// (D) 本檔的 operator=(Widget other) 是「統一賦值」
//   傳值 + swap 的寫法，一個函式同時處理拷貝與移動：
//   傳左值時參數由拷貝建構、傳右值時由移動建構，之後一律 swap。
//   細節見第 33 課；本檔保留它是為了讓 Widget 不違反 Rule of Three/Five。
//
// 【注意事項 Pay Attention】
//   1. 移動建構函數務必標 noexcept，否則 vector 擴容時不會採用它。
//   2. noexcept 是承諾不是檢查；標了卻拋例外，標準規定呼叫 std::terminate()
//      （這是有定義的行為，不是未定義行為）。
//   3. 拷貝建構函數通常「不能」標 noexcept —— 它要配置記憶體，可能拋 bad_alloc。
//   4. 擴容時「新元素」與「既有元素」走的路徑不同，
//      只有既有元素的重新安置受 move_if_noexcept 管轄（見上方第 4 點）。
//   5. 本檔輸出中，vector 何時擴容取決於 reserve(2) 這個明確的容量，
//      不依賴實作的成長倍率，因此輸出是決定性的。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】移動建構函數與 noexcept
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼移動建構函數一定要加 noexcept？不加會怎樣？
//     答：std::vector 擴容時用 std::move_if_noexcept 決定怎麼搬既有元素。
//         移動建構若沒標 noexcept，vector 會認定它可能拋例外，
//         為了維持 push_back 的強例外保證而改用拷貝。
//         結果是：你寫的移動建構函數完全不會被呼叫到，等於白寫。
//         本檔兩個類別只差這個關鍵字，擴容時一個印 [移動]、一個印 [拷貝]。
//     追問：為什麼「拷貝」在這裡反而比較安全？
//         → 拷貝不修改來源。搬到一半拋例外時舊緩衝區還完整，
//           vector 可以直接放棄新緩衝區、回到原狀；
//           而移動已經把前幾個元素搬空了，回不去。
//
// 🔥 Q2. 標了 noexcept 的函式如果真的拋出例外，會發生什麼事？
//     答：標準規定呼叫 std::terminate() 直接結束程式。
//         注意這不是未定義行為，而是有明確定義的結果 ——
//         不會被轉成別的例外，也不會有堆疊回溯保證。
//         noexcept 是你對編譯器的承諾，編譯器不會替你驗證。
//     追問：那編譯器完全不檢查嗎？
//         → 不檢查函式內部實際會不會拋。但它會用這個承諾做最佳化，
//           例如省略例外處理的相關程式碼；標準庫也會用它做演算法選擇。
//
// ⚠️ 陷阱. 「我寫了移動建構函數，所以 vector 擴容一定會用移動，效能沒問題。」
//     答：不成立。少了 noexcept，vector 會完全忽略你的移動建構函數而改用拷貝。
//         而且這件事沒有任何錯誤或警告 —— 程式完全正確，只是慢，
//         慢到你以為移動語義沒用。本檔輸出就是實測證據。
//     為什麼會錯：以為「有沒有移動建構函數」是唯一的判斷條件。
//         實際上標準庫問的是更嚴格的問題：
//         「這個移動保證不會拋嗎？」（is_nothrow_move_constructible）
//         答案是否定時，有等於沒有。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】—— 本檔從缺，理由如下
//   noexcept 對 vector 擴容策略的影響屬於例外安全與標準庫實作細節，
//   LeetCode 只驗證輸出正確與整體時間限制，
//   不會、也無法觀察你的元素型別是走移動還是拷貝重新安置。
//   硬掛一題只會誤導。本檔改以「同一份程式碼、只差一個關鍵字」
//   的對照實驗呈現，證據比題號更有說服力。
//
// -----------------------------------------------------------------------------
// lesson32_noexcept.cpp
// -----------------------------------------------------------------------------

#include <iostream>
#include <vector>
#include <cstring>
#include <type_traits>

// =============================================================
// 實驗組：移動建構函數有 noexcept
// =============================================================
class Widget {
private:
    char* m_name;

public:
    Widget(const char* name) : m_name(new char[std::strlen(name) + 1]) {
        std::strcpy(m_name, name);
    }

    // 拷貝建構 —— 要配置記憶體，可能拋 bad_alloc，所以「不能」標 noexcept
    Widget(const Widget& other) : m_name(new char[std::strlen(other.m_name) + 1]) {
        std::strcpy(m_name, other.m_name);
        std::cout << "    [拷貝] \"" << m_name << "\"\n";
    }

    // 移動建構 —— 有 noexcept！只搬指標、不配置記憶體，天生不會拋
    Widget(Widget&& other) noexcept : m_name(other.m_name) {
        other.m_name = nullptr;
        std::cout << "    [移動] \"" << m_name << "\"\n";
    }

    ~Widget() { delete[] m_name; }   // delete[] nullptr 是合法的空操作

    // 統一賦值（傳值 + swap），細節見第 33 課
    Widget& operator=(Widget other) {
        std::swap(m_name, other.m_name);
        return *this;
    }
};

// =============================================================
// 對照組：一模一樣，只是移動建構函數「沒有」noexcept
// =============================================================
class FragileWidget {
private:
    char* m_name;

public:
    FragileWidget(const char* name) : m_name(new char[std::strlen(name) + 1]) {
        std::strcpy(m_name, name);
    }

    FragileWidget(const FragileWidget& other)
        : m_name(new char[std::strlen(other.m_name) + 1]) {
        std::strcpy(m_name, other.m_name);
        std::cout << "    [拷貝] \"" << m_name << "\"\n";
    }

    // ★ 唯一的差別：少了 noexcept
    FragileWidget(FragileWidget&& other) : m_name(other.m_name) {
        other.m_name = nullptr;
        std::cout << "    [移動] \"" << m_name << "\"\n";
    }

    ~FragileWidget() { delete[] m_name; }

    FragileWidget& operator=(FragileWidget other) {
        std::swap(m_name, other.m_name);
        return *this;
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】封包緩衝區佇列的擴容成本
//   情境：網路服務把收到的封包先暫存在 std::vector 裡，滿了就整批處理。
//         流量尖峰時 vector 會反覆擴容，每次都要重新安置既有元素。
//         這正是移動語義該發揮作用的地方 —— 只搬指標，不複製整塊 payload。
//   為什麼用到本主題：能不能享受到這個好處，取決於元素型別的移動建構
//         有沒有標 noexcept。少一個關鍵字，尖峰時的每次擴容
//         就從「搬指標」變成「逐位元組深拷貝」。
//   下面用同一段流程跑兩個型別，唯一變因就是那個 noexcept。
// -----------------------------------------------------------------------------
template <typename T>
void runScenario(const char* tag) {
    std::cout << tag << "（is_nothrow_move_constructible = "
              << std::is_nothrow_move_constructible_v<T> << "）\n";

    std::vector<T> vec;
    vec.reserve(2);  // 先預留 2 個空間，讓第 3 次 push_back 必定觸發擴容

    std::cout << "  push_back #1:\n";
    vec.push_back(T("Alpha"));

    std::cout << "  push_back #2:\n";
    vec.push_back(T("Beta"));

    std::cout << "  push_back #3（觸發擴容，需要重新安置 Alpha 與 Beta）:\n";
    vec.push_back(T("Gamma"));
}

int main() {
    std::cout << std::boolalpha;

    std::cout << "=== 1. 有 noexcept：擴容時用移動 ===\n";
    runScenario<Widget>("Widget");

    std::cout << "\n=== 2. 沒有 noexcept：擴容時退回拷貝 ===\n";
    runScenario<FragileWidget>("FragileWidget");

    std::cout << "\n=== 3. 結論 ===\n";
    std::cout << "  兩個類別只差一個 noexcept 關鍵字。\n";
    std::cout << "  新元素 Gamma 兩邊都是移動；差別在「重新安置既有元素」那兩行。\n";
    std::cout << "  沒有 noexcept 時，vector 為了強例外保證寧可深拷貝。\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -Wreorder "第 32 課：移動建構函數（Move Constructor）2.cpp" -o lesson32b

// ── 輸出說明（讀之前先看這裡）────────────────────────────────────────────
// * 本檔輸出完全決定性：沒有位址、沒有耗時、沒有執行緒，
//   本機實測連跑 5 次逐位元組相同。
// * 擴容時機不依賴實作的成長倍率：這裡用 reserve(2) 明確固定容量，
//   所以「第 3 次 push_back 觸發擴容」在任何實作上都成立。
//   （若不 reserve 而靠自動成長，1→2→4 的序列是 libstdc++ 的
//     2 倍策略，屬實作定義，MSVC 的 1.5 倍會給出不同時機。）
// * 請對照兩段的第 3 次 push_back：
//       有 noexcept  → [移動] Gamma、[移動] Alpha、[移動] Beta
//       沒有 noexcept → [移動] Gamma、[拷貝] Alpha、[拷貝] Beta
//   Gamma 兩邊都是移動並非筆誤：把暫時物件放進新槽位時，
//   失敗也沒有既有資料會被破壞，不需要 move_if_noexcept 把關；
//   只有「重新安置既有元素」才受它管轄。
// * 解構函數刻意不印任何東西，以免蓋過本檔要對比的重點。
// ─────────────────────────────────────────────────────────────────────────

// === 預期輸出 ===
// === 1. 有 noexcept：擴容時用移動 ===
// Widget（is_nothrow_move_constructible = true）
//   push_back #1:
//     [移動] "Alpha"
//   push_back #2:
//     [移動] "Beta"
//   push_back #3（觸發擴容，需要重新安置 Alpha 與 Beta）:
//     [移動] "Gamma"
//     [移動] "Alpha"
//     [移動] "Beta"
//
// === 2. 沒有 noexcept：擴容時退回拷貝 ===
// FragileWidget（is_nothrow_move_constructible = false）
//   push_back #1:
//     [移動] "Alpha"
//   push_back #2:
//     [移動] "Beta"
//   push_back #3（觸發擴容，需要重新安置 Alpha 與 Beta）:
//     [移動] "Gamma"
//     [拷貝] "Alpha"
//     [拷貝] "Beta"
//
// === 3. 結論 ===
//   兩個類別只差一個 noexcept 關鍵字。
//   新元素 Gamma 兩邊都是移動；差別在「重新安置既有元素」那兩行。
//   沒有 noexcept 時，vector 為了強例外保證寧可深拷貝。
