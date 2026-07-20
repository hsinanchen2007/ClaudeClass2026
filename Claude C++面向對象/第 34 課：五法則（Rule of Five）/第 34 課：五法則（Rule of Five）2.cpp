// =============================================================================
//  第 34 課 -2  —  用 type traits 檢查五法則：你的類別到底具備哪些能力
// =============================================================================
//
// 【主題資訊 Information】
//   主題       ： 以編譯期 trait 檢視一個類別的五個特殊成員函式狀態
//   使用的 trait：
//       std::is_destructible_v<T>                  可解構
//       std::is_copy_constructible_v<T>            可拷貝建構
//       std::is_copy_assignable_v<T>               可拷貝賦值
//       std::is_move_constructible_v<T>            可移動建構
//       std::is_nothrow_move_constructible_v<T>    移動建構且保證不拋
//       std::is_move_assignable_v<T>               可移動賦值
//       std::is_nothrow_move_assignable_v<T>       移動賦值且保證不拋
//   標準版本   ： trait 本體 C++11；本檔用的 _v 變數模板後綴是 C++17
//   標頭檔     ： <type_traits>（另用 <memory> 的 unique_ptr、<string>）
//   複雜度     ： 全部編譯期求值，執行期零成本
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼需要「檢查」而不是「用看的」】
//   五個特殊成員函式的生成規則彼此牽動：寫了其中一個，
//   可能就關掉另外幾個的自動生成（見本課 -1 與第 32 課 -3）。
//   規則複雜到光看程式碼很容易判斷錯，
//   所以直接問編譯器最可靠 —— 這就是本檔這張表的用意。
//   四個受測型別各代表一種典型狀態：
//       FullFive    五個都自己寫齊 —— Rule of Five 的標準做法
//       OnlyThree   只寫了三法則（解構＋拷貝兩件）—— C++98 時代的寫法
//       MoveOnly    成員是 unique_ptr —— 拷貝被隱式刪除，只能移動
//       std::string 標準庫的對照組
//
// 【2. 這張表最重要的一行：OnlyThree 的 true / false 落差】
//   OnlyThree 沒有寫任何移動操作，但表上「可移動建構」仍然是 true。
//   這不是錯誤，而是本課最容易被誤解的地方：
//       is_move_constructible 問的是「T t(std::move(x)) 能不能編譯」，
//       而 const T& 的拷貝建構函數可以接住右值 —— 所以拷貝也算數。
//   真正露出馬腳的是下一行：nothrow 移動建構是 false。
//   因為它實際走的是拷貝，而拷貝要 new 一塊記憶體、可能拋 bad_alloc。
//   結論：OnlyThree 的所有「移動」其實都是深拷貝，
//   而且沒有任何錯誤或警告會告訴你這件事（對照第 32 課 -3 的同一個陷阱）。
//
// 【3. MoveOnly：為什麼拷貝會「消失」】
//   MoveOnly 一行特殊成員函式都沒寫，能力卻完全不同：
//   拷貝兩項是 false、移動兩項是 true。
//   原因在成員 std::unique_ptr —— 它的拷貝建構與拷貝賦值被標記為 = delete。
//   類別的隱式拷貝操作若「無法合法生成」（因為某個成員不能拷貝），
//   就會被定義為 deleted，而不是編譯錯誤。
//   於是 MoveOnly 自動變成 move-only 型別，
//   而它的移動操作由 unique_ptr 逐成員生成，且沿用其 noexcept 性質。
//   這就是 Rule of Zero 的威力：一行都不寫，語義卻完全正確。
//
// 【4. FullFive 全 true 代表什麼、不代表什麼】
//   代表：五個能力都在，而且兩個移動操作都標了 noexcept，
//         所以放進 std::vector 擴容時會走移動而非拷貝（見第 32 課 -2）。
//   不代表：實作是正確的。trait 只做重載解析與 noexcept 查詢，
//         它不會實例化函式本體，更不會檢查語義。
//         本檔原始版本的 FullFive 就藏了一個真實缺陷（見下方【概念補充 D】），
//         而那張全 true 的表完全看不出來 —— 這點本身就值得記住。
//
// 【概念補充 Concept Deep Dive】
//
// (A) trait 不會實例化函式本體
//   is_*_constructible / is_*_assignable 只在編譯期做重載解析，
//   確認「這個運算式合不合法」以及「宣告上有沒有 noexcept」。
//   函式內部寫了什麼、會不會踩到未定義行為，trait 一律不管。
//   所以「trait 全 true」只說明介面齊備，跟正確性是兩回事。
//
// (B) nothrow 那兩行才是效能相關的關鍵
//   標準庫（尤其 vector 擴容）用 is_nothrow_move_constructible
//   決定要移動還是拷貝既有元素。
//   所以看這張表時，真正該盯的不是「可不可以移動」，
//   而是「移動保證不拋嗎」。前者幾乎永遠是 true（拷貝可以頂替），
//   後者才分得出真移動與假移動。
//
// (C) 為什麼 is_destructible 四個都是 true
//   只要解構函數可存取且沒被刪除就是 true。
//   private 或 = delete 的解構函數才會讓它變 false
//   （例如某些只能透過工廠函式釋放的型別）。這一行在這張表裡是對照基準。
//
// (D) ★ 本檔修正的一個真實缺陷：copy-assign 沒有處理「被移動過的自己」
//   原始版本的 FullFive 拷貝賦值寫成：
//       if (this != &o) { *m_data = *o.m_data; }
//   問題在於 m_data 可能是 nullptr —— 移動建構／移動賦值會把來源的
//   m_data 設成 nullptr。於是這段程式碼會踩到未定義行為：
//       FullFive a, b;
//       FullFive c = std::move(a);   // a.m_data 變成 nullptr
//       a = b;                       // *nullptr = ... → 未定義行為
//   本機以 -fsanitize=undefined 實測，UBSan 明確報出
//   「runtime error: store to null pointer of type 'int'」。
//   （未定義行為沒有固定結果：不加 sanitizer 時可能看似正常、
//     可能當掉、也可能默默寫壞別處記憶體，不能假設任一種。）
//   為什麼這是真缺陷：第 31 課講過，被移動後的物件處於
//   valid but unspecified 狀態，「重新賦值」是標準保證安全的操作之一。
//   一個類別若在自己被移動之後就不能再被賦值，等於違背了這個契約。
//   本檔已修正為先判斷雙方的 m_data 是否為空再決定要賦值、配置或釋放。
//   這也正是五法則要「五個一起想」的理由：移動操作留下的狀態，
//   拷貝賦值必須接得住。
//
// 【注意事項 Pay Attention】
//   1. is_move_constructible 為 true ≠ 有移動建構函數；
//      拷貝建構可以接住右值，所以幾乎永遠是 true。
//      要分辨真假移動請看 nothrow 那兩行。
//   2. trait 只驗介面、不驗實作。全 true 不代表類別寫對了。
//   3. unique_ptr 成員會讓拷貝操作被隱式定義為 deleted（不是編譯錯誤），
//      類別自動變成 move-only。
//   4. 寫了移動操作，就必須確保拷貝賦值能處理「被移動過」的物件，
//      否則會違反 valid but unspecified 的契約（見【概念補充 D】）。
//   5. 本檔所有輸出皆為編譯期常數，執行期不做任何實際的建構或賦值 ——
//      正因如此，(D) 的缺陷才不會在本檔的輸出中顯現。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】用 traits 檢視五法則
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 一個只寫了「三法則」的類別，is_move_constructible 會是 true 還是 false？
//     答：true。因為這個 trait 問的是「能不能用右值建構」，
//         而 const T& 的拷貝建構函數接得住右值，所以拷貝頂替了移動。
//         但 is_nothrow_move_constructible 是 false ——
//         它實際走的是拷貝，要配置記憶體、可能拋 bad_alloc。
//         本檔 OnlyThree 那一組 true/false 就是這個現象。
//     追問：那實務上會有什麼後果？
//         → vector 擴容時因為不是 nothrow 而改用拷貝，
//           所有「移動」其實都是深拷貝，而且沒有任何警告會提醒你。
//
// 🔥 Q2. 為什麼成員換成 unique_ptr 之後，類別就不能拷貝了？
//     答：unique_ptr 的拷貝建構與拷貝賦值被標記為 = delete。
//         當類別的隱式拷貝操作因為某個成員無法拷貝而生成不出來時，
//         標準規定把它定義為 deleted，而不是報編譯錯誤。
//         於是類別自動成為 move-only，移動操作則照常逐成員生成。
//     追問：這算 Rule of Five 還是 Rule of Zero？
//         → Rule of Zero。讓成員自己管資源，五個特殊成員函式一個都不用寫，
//           而且連 noexcept 都會正確地自動推導出來。
//
// ⚠️ 陷阱. 「trait 全部印 true，代表我的五法則寫對了。」
//     答：不成立。trait 只做重載解析與 noexcept 宣告查詢，
//         它不會實例化函式本體，也完全不檢查語義。
//         本檔的 FullFive 原始版本七項全 true，
//         卻藏著一個真實缺陷：拷貝賦值沒有處理 m_data 為 nullptr 的情況，
//         對「被移動過的物件」再賦值就會踩到未定義行為
//         （本機以 -fsanitize=undefined 實測會報 store to null pointer）。
//     為什麼會錯：把 trait 當成正確性檢查。
//         它檢查的是「這個操作在型別系統上成不成立」，
//         不是「這個操作做得對不對」。前者編譯器能答，後者只能靠測試與審查。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】—— 本檔從缺，理由如下
//   本檔的內容是編譯期的型別能力查詢，
//   LeetCode 只驗證演算法的輸入輸出與時間限制，
//   評測系統既不會拷貝你的物件，也無從觀察特殊成員函式的生成狀態。
//   即使是設計類題目（146. LRU Cache、707. Design Linked List），
//   評測過程也只會建立一個實例並呼叫其方法，
//   完全碰不到拷貝／移動語義。硬掛題號只會誤導，因此從缺。
//
// -----------------------------------------------------------------------------
// lesson34_traits.cpp
// -----------------------------------------------------------------------------

#include <iostream>
#include <type_traits>
#include <string>
#include <memory>

// 五法則完整：解構、拷貝建構、拷貝賦值、移動建構、移動賦值
class FullFive {
    int* m_data;
public:
    FullFive() : m_data(new int(0)) {}
    ~FullFive() { delete m_data; }

    FullFive(const FullFive& o)
        : m_data(o.m_data ? new int(*o.m_data) : nullptr) {}

    // ★ 已修正：必須能處理「自己或來源曾被移動過」（m_data 為 nullptr）的情況。
    //    原始寫法是 if (this != &o) { *m_data = *o.m_data; }，
    //    對被移動過的物件再賦值會解參考空指標 → 未定義行為
    //    （本機以 -fsanitize=undefined 實測會報 store to null pointer）。
    //    第 31 課講過：被移動後的物件「重新賦值」必須是安全的，
    //    這正是五法則要五個一起設計的原因。
    FullFive& operator=(const FullFive& o) {
        if (this != &o) {
            if (!o.m_data) {                 // 來源是空的 → 自己也變成空的
                delete m_data;
                m_data = nullptr;
            } else if (m_data) {             // 雙方都有 → 直接覆寫，不必重新配置
                *m_data = *o.m_data;
            } else {                         // 自己是空的 → 配置一塊新的
                m_data = new int(*o.m_data);
            }
        }
        return *this;
    }

    FullFive(FullFive&& o) noexcept : m_data(o.m_data) { o.m_data = nullptr; }

    FullFive& operator=(FullFive&& o) noexcept {
        if (this != &o) { delete m_data; m_data = o.m_data; o.m_data = nullptr; }
        return *this;
    }
};

// 只有三法則：解構 + 拷貝建構 + 拷貝賦值，沒有任何移動操作
class OnlyThree {
    int* m_data;
public:
    OnlyThree() : m_data(new int(0)) {}
    ~OnlyThree() { delete m_data; }
    OnlyThree(const OnlyThree& o) : m_data(new int(*o.m_data)) {}
    OnlyThree& operator=(const OnlyThree& o) {
        if (this != &o) { *m_data = *o.m_data; }
        return *this;
    }
    // 沒有移動操作！所有「移動」都會安靜地退回拷貝。
    // 註：這個類別沒有移動操作，m_data 因此永遠不會變成 nullptr，
    //     所以上面的拷貝賦值不會踩到 FullFive 原本那個空指標問題。
};

// Rule of Zero：一個特殊成員函式都沒寫，語義卻完全正確
class MoveOnly {
    std::unique_ptr<int> m_data;
public:
    MoveOnly() : m_data(std::make_unique<int>(0)) {}
    // unique_ptr 讓拷貝被隱式刪除（deleted，不是編譯錯誤），移動自動生成
};

template <typename T>
void checkTraits(const char* name) {
    std::cout << name << ":\n";
    std::cout << "  可解構？           " << std::is_destructible_v<T> << "\n";
    std::cout << "  可拷貝建構？       " << std::is_copy_constructible_v<T> << "\n";
    std::cout << "  可拷貝賦值？       " << std::is_copy_assignable_v<T> << "\n";
    std::cout << "  可移動建構？       " << std::is_move_constructible_v<T> << "\n";
    std::cout << "  nothrow 移動建構？ " << std::is_nothrow_move_constructible_v<T> << "\n";
    std::cout << "  可移動賦值？       " << std::is_move_assignable_v<T> << "\n";
    std::cout << "  nothrow 移動賦值？ " << std::is_nothrow_move_assignable_v<T> << "\n";
    std::cout << "\n";
}

int main() {
    std::cout << std::boolalpha;
    checkTraits<FullFive>("FullFive (五法則完整)");
    checkTraits<OnlyThree>("OnlyThree (只有三法則)");
    checkTraits<MoveOnly>("MoveOnly (只能移動)");
    checkTraits<std::string>("std::string (標準庫)");
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -Wreorder "第 34 課：五法則（Rule of Five）2.cpp" -o lesson34b

// ── 輸出說明（讀之前先看這裡）────────────────────────────────────────────
// * 本檔輸出完全決定性：七項全部是編譯期常數，
//   執行期不做任何實際的建構、賦值或配置，
//   沒有位址、沒有耗時（本機實測連跑 5 次逐位元組相同）。
// * 請特別對照 OnlyThree 的這兩行：
//       可移動建構？       true
//       nothrow 移動建構？ false
//   它一個移動操作都沒寫。true 是因為拷貝建構接得住右值，
//   false 才揭露它實際走的是會配置記憶體、可能拋 bad_alloc 的拷貝。
//   這就是「假移動」在 trait 上的長相。
// * MoveOnly 的拷貝兩項是 false：unique_ptr 成員讓隱式拷貝被定義為
//   deleted（不是編譯錯誤），類別自動成為 move-only。
// * std::string 七項全 true 屬 libstdc++ 的實作結果；
//   標準只保證 string 可拷貝、可移動，並要求移動操作為 noexcept。
// * 提醒：trait 只驗介面不驗實作。本檔已修正 FullFive 拷貝賦值的空指標缺陷
//   （見【概念補充 D】），而修正前後這張表完全一樣 ——
//   這正說明 trait 全 true 不能當成正確性的證明。
// ─────────────────────────────────────────────────────────────────────────

// === 預期輸出 ===
// FullFive (五法則完整):
//   可解構？           true
//   可拷貝建構？       true
//   可拷貝賦值？       true
//   可移動建構？       true
//   nothrow 移動建構？ true
//   可移動賦值？       true
//   nothrow 移動賦值？ true
//
// OnlyThree (只有三法則):
//   可解構？           true
//   可拷貝建構？       true
//   可拷貝賦值？       true
//   可移動建構？       true
//   nothrow 移動建構？ false
//   可移動賦值？       true
//   nothrow 移動賦值？ false
//
// MoveOnly (只能移動):
//   可解構？           true
//   可拷貝建構？       false
//   可拷貝賦值？       false
//   可移動建構？       true
//   nothrow 移動建構？ true
//   可移動賦值？       true
//   nothrow 移動賦值？ true
//
// std::string (標準庫):
//   可解構？           true
//   可拷貝建構？       true
//   可拷貝賦值？       true
//   可移動建構？       true
//   nothrow 移動建構？ true
//   可移動賦值？       true
//   nothrow 移動賦值？ true
//
