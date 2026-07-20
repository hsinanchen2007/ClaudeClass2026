// =============================================================================
//  05_noexcept.cpp  —  noexcept 與「移動式效能優化」
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/language/noexcept_spec
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、noexcept 是什麼？                                      │
//  └────────────────────────────────────────────────────────────┘
//
//  函式聲明後綴 — 承諾「這函式不會 throw」。如果真的 throw 了，runtime
//  直接呼叫 std::terminate（不會 unwind 到呼叫端）。
//
//      void f() noexcept;
//      void g() noexcept(condition);    // 編譯期 bool 條件
//
//  關鍵詞作為「型別系統訊號」 — STL 容器會「看你的 move 是不是 noexcept」
//  決定要不要用它，這直接影響效能。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、最重要場景：std::vector 重新配置時的「move vs copy」  │
//  └────────────────────────────────────────────────────────────┘
//
//  vector 容量不夠時要把所有元素「搬到新 buffer」。要選 move 還是 copy？
//
//   * 若 element 的 move constructor 是 noexcept：用 move（快、O(n) 單次）
//   * 若不是 noexcept：用 copy（慢、可能多倍時間）
//
//  原因：vector 為了「強例外保證」(strong exception guarantee) — 萬一搬到
//  一半某個 move throw，可能搬完一半的 buffer 就壞了。如果是 copy，原
//  buffer 還在，搬失敗也能還原。move 不能還原（原物件已被掏空），所以
//  vector 只敢用 noexcept 的 move。
//
//  結論：自己寫 class 時，如果 move ctor / move assignment 【真的不會拋】
//  （純粹轉移 handle／指標、不配置記憶體），就【明確標上 noexcept】，否則
//  丟進 vector 擴充時可能悄悄走 copy 路徑，效能差一個數量級。
//
//  ⚠️ 但這【不是】「一律標上去」的意思。noexcept 是對 runtime 的硬承諾：
//  一旦標了卻真的丟出例外，直接 std::terminate，沒有 catch 的機會。把一個
//  會配置記憶體的 move 硬標 noexcept，等於把「可回復的 bad_alloc」變成
//  「不可回復的當場死亡」—— 那比多幾次 copy 嚴重得多。
//  標不了就兩條路：改實作讓它不需要配置（例如改持有指標／pimpl），
//  或就接受 vector 走 copy。詳見下方 Q3 的原則 (d)。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、noexcept 的副作用                                     │
//  └────────────────────────────────────────────────────────────┘
//
//   * 編譯器可能省掉 unwinding metadata → 較小 binary
//   * caller 可能更積極內聯
//   * destructor 預設 noexcept (隱式)
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 四、noexcept 運算子                                        │
//  └────────────────────────────────────────────────────────────┘
//
//      noexcept(expr)
//
//  作為 operator 在編譯期回傳 bool — 「expr 編譯期判斷會不會 throw」。
//
//      static_assert(noexcept(0 + 0));            // 算術不 throw
//      static_assert(noexcept(std::declval<int>() + 0));
//
//  常用搭配條件 noexcept：
//
//      template <class T>
//      void swap(T& a, T& b) noexcept(noexcept(T(std::move(a))) &&
//                                     noexcept(a = std::move(b)));
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 本檔示範                                                   │
//  └────────────────────────────────────────────────────────────┘
//
//   * Demo 1：兩個版本的 class（移動式 noexcept vs not）→ 看 vector 差別
//   * Demo 2：noexcept 運算子的編譯期判斷
// =============================================================================

/*
補充筆記：noexcept
  - noexcept 屬於例外處理；例外用來把錯誤從偵測處傳到能處理的地方，而不是取代一般 if 流程。
  - throw by value, catch by const reference 是常見規則，可避免 slicing 並保留多型例外資訊。
  - 解構子和 noexcept 函式不應讓例外逃出；違反 noexcept 會呼叫 std::terminate。
  - RAII 是例外安全基礎，因為 stack unwinding 會自動解構已建立物件。
  - 例外安全常分為 basic guarantee、strong guarantee、nothrow guarantee；修改資料結構時要知道失敗後物件狀態是否仍有效。
  - 不要丟裸指標或字串字面值作為主要錯誤通道；標準例外或自訂 exception 型別較能保存語意。
  - noexcept 是函式介面承諾；若例外逃出 noexcept 函式會呼叫 std::terminate。
  - move constructor 標 noexcept 可讓 vector 等容器在搬移元素時選擇更有效且安全的路徑。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】noexcept 與移動語意的效能連動
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. noexcept 是什麼？違反了會怎樣？
//     答：它宣告函式不會拋出例外。若例外真的試圖逃出 noexcept 函式，會直接呼叫
//     std::terminate()，而且不保證做 stack unwinding。兩種用法：說明符
//     void f() noexcept;，以及運算子 noexcept(expr)——編譯期 bool，用來查詢某個運算式
//     是否宣告為不拋，可寫出條件式的 noexcept(noexcept(x.swap(y)))。它的價值是：讓標準庫
//     選擇更快的路徑、讓編譯器省略 unwinding 相關程式碼、作為契約與文件。
//     追問：哪些函式應該標 noexcept？（移動建構、移動賦值、swap、解構函式（隱含）、
//     記憶體釋放函式；以及確實簡單不拋的葉節點函式。反過來說，標了之後再移除是契約與
//     ABI 的破壞，不要隨便亂標）
//
// 🔥 Q2. std::vector 擴容為什麼要看 move 建構是不是 noexcept？（move_if_noexcept）
//     答：push_back 承諾強例外保證。擴容時要把舊元素搬到新緩衝區：若用 move，搬到一半
//     拋例外的話舊 buffer 已被掏空、新 buffer 不完整，無法回滾，強保證破功；若用 copy，
//     搬到一半拋例外時舊 buffer 完好無損，丟掉新 buffer 即可回滾。所以標準庫用
//     std::move_if_noexcept：當元素的移動建構函式是 noexcept（或該型別根本不可複製）
//     時才 move，否則退回 copy。實務衝擊是——忘了在自己的類別上標 noexcept，vector 擴容
//     會默默退回複製，效能差好幾倍，而且沒有任何警告。
//     追問：怎麼檢查？（static_assert(std::is_nothrow_move_constructible_v<T>);）
//
// ⚠️ 陷阱. 在 noexcept 函式裡呼叫可能拋例外的東西，能通過編譯嗎？
//     答：能。noexcept 不是編譯期強制檢查，編譯器最多給警告（GCC 的 -Wterminate 只在
//     能靜態判定必然拋出時才警告）。void f() noexcept { throw 1; } 編得過，執行到那行
//     直接 std::terminate()。所以 noexcept 是你對編譯器與使用者的承諾，違約的懲罰是
//     執行期直接死亡，而不是編譯錯誤——這也是「亂標 noexcept 很危險」的原因：它不會
//     幫你檢查，只會讓出事時更難除錯（例外根本無法被 catch）。
//     為什麼會錯：把它類比成 Java 的 throws 那種編譯期檢查機制。
//
// 🔥 Q. 「zero-cost exception」的 zero-cost 是相對什麼而言？
//     答：相對於舊的 **SjLj（setjmp/longjmp）** 模型。SjLj 要在每個有 cleanup 的
//         scope 進出時執行 setjmp 註冊／註銷，**沒有例外也要付錢**。
//         現代 table-based（DWARF/Itanium ABI）改成把 unwind 資訊放進獨立的
//         .eh_frame / LSDA 表，正常路徑一條指令都不用多跑，代價轉移到「真的丟出
//         例外時」要查表 unwind（很慢）與「binary 變大」。
//     追問：所以什麼時候不該用例外？（高頻錯誤路徑——例外一旦變常態，查表成本
//         遠超回傳錯誤碼；以及 binary size 受限的嵌入式環境）
//
// Q. `void f() throw();` 和 `void f() noexcept;` 差在哪？throw() 現在還能用嗎？
//     答：throw(...) 是 C++98 的「動態例外規格」，可以列出允許拋出的型別；但實務上
//         幾乎不可最佳化（編譯器得插入執行期檢查），違規時呼叫 std::unexpected 的
//         行為也難以使用，因此被 C++11 的 noexcept 取代。完整時間線（別只答
//         「C++11 取代」，那會被追問）：
//           C++11：動態例外規格 deprecated，引入 noexcept
//           C++17：移除 throw(T,...)；throw() 保留並改為等價於 noexcept(true)
//                  （違規行為也從 unexpected() 改成 terminate()）
//           C++20：連 throw() 也移除
//         本機實測：throw(int) 在 -std=c++14 可編，-std=c++17 直接報
//         「ISO C++17 does not allow dynamic exception specifications」。
//     追問：那 throw() 在 -std=c++20 下編得過嗎？（標準已移除，但本機 GCC 15 即使加
//         -pedantic-errors 仍接受它——這是編譯器的寬容，不是標準允許。也提醒
//         「編譯得過」不等於「符合標準」，驗證版本分界時要小心）
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <type_traits>
#include <utility>
#include <vector>

// 版本 A：move ctor 標 noexcept — 進 vector 會走 move
struct WidgetA {
    int* data;
    explicit WidgetA(int v) : data(new int(v)) {}
    ~WidgetA() { delete data; }

    WidgetA(const WidgetA& o) : data(new int(*o.data)) { ++copyCount; }
    WidgetA(WidgetA&& o) noexcept : data(o.data) { o.data = nullptr; ++moveCount; }
    WidgetA& operator=(const WidgetA&) = delete;
    WidgetA& operator=(WidgetA&&) = delete;

    static inline int copyCount = 0;
    static inline int moveCount = 0;
};

// 版本 B：move ctor 沒 noexcept — 進 vector 會走 copy
struct WidgetB {
    int* data;
    explicit WidgetB(int v) : data(new int(v)) {}
    ~WidgetB() { delete data; }

    WidgetB(const WidgetB& o) : data(new int(*o.data)) { ++copyCount; }
    WidgetB(WidgetB&& o) /*no noexcept*/ : data(o.data) { o.data = nullptr; ++moveCount; }
    WidgetB& operator=(const WidgetB&) = delete;
    WidgetB& operator=(WidgetB&&) = delete;

    static inline int copyCount = 0;
    static inline int moveCount = 0;
};

int main() {
    // ─────────────────────────────────────────────────────────
    // Demo 1：vector reallocation — move vs copy
    //   reserve 不夠時 vector 會搬家；標 noexcept 才能 move
    // ─────────────────────────────────────────────────────────
    {
        std::vector<WidgetA> v;
        for (int i = 0; i < 8; ++i) v.emplace_back(i);   // 多次重配
        std::cout << "[Demo1-A] copy=" << WidgetA::copyCount
                  << " move=" << WidgetA::moveCount << '\n';
    }
    {
        std::vector<WidgetB> v;
        for (int i = 0; i < 8; ++i) v.emplace_back(i);
        std::cout << "[Demo1-B] copy=" << WidgetB::copyCount
                  << " move=" << WidgetB::moveCount << '\n';
    }
    // 預期：A 主要走 move，B 主要走 copy

    // ─────────────────────────────────────────────────────────
    // Demo 2：noexcept 運算子
    // ─────────────────────────────────────────────────────────
    static_assert(noexcept(0 + 0), "arithmetic shouldn't throw");
    static_assert(std::is_nothrow_move_constructible_v<WidgetA>,
                  "WidgetA move should be noexcept");
    static_assert(!std::is_nothrow_move_constructible_v<WidgetB>,
                  "WidgetB move is NOT noexcept");

    std::cout << "[Demo2] static_assert all passed at compile time\n";

    // ─────────────────────────────────────────────────────────
    // 實用範例 A：自訂 move + member swap 應加 noexcept
    //   STL 容器在強例外保證下會檢查 swap / move 是否 noexcept；自家型別實作
    //   時，move ctor、move assignment 與 swap 都應該 noexcept。
    // ─────────────────────────────────────────────────────────
    {
        struct Box {
            int* p;
            Box() : p(new int(0)) {}
            ~Box() { delete p; }
            Box(Box&& o) noexcept : p(o.p) { o.p = nullptr; }
            Box& operator=(Box&& o) noexcept { std::swap(p, o.p); return *this; }
            Box(const Box&) = delete;
            Box& operator=(const Box&) = delete;

            // member swap：noexcept；外部可直接呼叫
            void swap(Box& other) noexcept { std::swap(p, other.p); }
        };
        static_assert(std::is_nothrow_move_constructible_v<Box>,
                      "Box move should be noexcept");
        Box a, b;
        *a.p = 7; *b.p = 11;
        a.swap(b);
        std::cout << "[swap] a=" << *a.p << " b=" << *b.p
                  << "  (預期 a=11 b=7)\n";
    }

    // ─────────────────────────────────────────────────────────
    // 實用範例 B：條件式 noexcept — 只有當 T 的 swap 不丟才聲明 noexcept
    //   這是 STL 的常見寫法，讓「noexcept 屬性沿著型別傳遞」。
    // ─────────────────────────────────────────────────────────
    {
        // 簡化版 swapper：noexcept 屬性跟著 T 走
        auto safeSwap = [](auto& a, auto& b)
            noexcept(noexcept(std::swap(a, b)))
        {
            std::swap(a, b);
        };
        int x = 1, y = 2;
        safeSwap(x, y);
        std::cout << "[condNoexcept] x=" << x << " y=" << y << '\n';
        // noexcept(safeSwap(x, y)) 對 int 一定是 true（int swap 不丟）
        static_assert(noexcept(std::swap(x, y)), "int swap should be noexcept");
    }

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：destructor 一定是 noexcept 嗎？
    //    A：C++11 起，destructor 預設 noexcept（隱式）。如果你真的要在
    //       destructor 內 throw，要顯式寫 ~T() noexcept(false) — 但極不建議
    //       (見 09_pitfalls.cpp)。
    //
    //  Q2：throw 在 noexcept 函式裡會發生什麼？
    //    A：runtime 立刻 std::terminate()，預設行為 abort 程式。所以 noexcept
    //       是個「強承諾」 — 你必須保證真的不丟。
    //
    //  Q3：應該對所有函式都加 noexcept 嗎？
    //    A：不用。原則：
    //         (a) move ctor / move assignment / swap 應該 noexcept
    //         (b) destructor 預設就是
    //         (c) 「真的不會丟」的小函式（getter、簡單運算）值得加
    //         (d) 內部會呼叫可能 throw 的函式 → 不要假裝 noexcept
    //
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra 05_noexcept.cpp -o 05_noexcept

// === 預期輸出 ===
// [Demo1-A] copy=0 move=7
// [Demo1-B] copy=7 move=0
// [Demo2] static_assert all passed at compile time
// [swap] a=11 b=7  (預期 a=11 b=7)
// [condNoexcept] x=2 y=1
