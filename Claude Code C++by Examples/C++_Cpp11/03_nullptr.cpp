// =============================================================================
//  03_nullptr.cpp  —  nullptr 取代 NULL / 0 (C++11)
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/language/nullptr
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、為什麼有 nullptr？                                     │
//  └────────────────────────────────────────────────────────────┘
//
//  C 與 C++98 用 NULL 表示「空指標」。NULL 通常被定義成：
//
//      #define NULL 0           // C++ 大多如此
//      #define NULL ((void*)0)  // C 大多如此
//
//  問題：NULL 本質是「整數 0」，不是真正的指標 — 在 overload 解析時會挑
//  錯函式：
//
//      void f(int);
//      void f(void*);
//      f(NULL);     // ❌ 會呼叫 f(int)！（因為 NULL = 0 = int）
//      f(nullptr); // ✅ 一定呼叫 f(void*) 版
//
//  C++11 引入 nullptr — 型別是 std::nullptr_t，可以隱式轉成「任何指標型
//  別」，但不會被當成 int。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、nullptr_t                                              │
//  └────────────────────────────────────────────────────────────┘
//
//      typedef decltype(nullptr) nullptr_t;     // <cstddef>
//
//  你可以為 nullptr_t 寫 overload：
//
//      void f(std::nullptr_t) { /* 顯式處理 nullptr 案例 */ }
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、本檔示範                                               │
//  └────────────────────────────────────────────────────────────┘
//
//   * Demo 1：overload 解析正確
//   * Demo 2：把指標檢查 if (p) 與 if (p != nullptr) 對照
//   * Demo 3：template 中 nullptr 的型別
// =============================================================================

/*
補充筆記：nullptr
  - nullptr 是現代 C++ 語法或標準庫特性；學習時要把「少寫字」和「語意更精確」分開看。
  - auto 讓型別由初始化式推導，但會丟掉 top-level const/reference；需要保留引用語意時要寫 auto&、const auto& 或 decltype(auto)。
  - brace initialization 能減少未初始化與 narrowing，但遇到 initializer_list overload 可能選到不同建構子。
  - constexpr、static_assert、if constexpr 把部分錯誤和計算提前到編譯期，能讓 template 和常數邏輯更清楚。
  - 屬性如 [[nodiscard]]、[[maybe_unused]]、[[fallthrough]] 是對編譯器和讀者的意圖標記，不應拿來掩蓋設計問題。
  - string_view、optional、variant、structured binding 等特性改善介面表達力，但也帶來生命週期或狀態檢查責任。
  - nullptr 的型別是 std::nullptr_t，可轉成任何指標但不會像 0 一樣被當成整數 overload。
  - 新程式不要再用 NULL；NULL 可能只是 0，會讓函式重載解析出錯。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】nullptr
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. nullptr 與 NULL、0 的差別？
//     答：NULL 通常只是 0 或 0L 的巨集，型別是整數，所以重載決議時會選到吃 int
//         的版本。nullptr 的型別是 std::nullptr_t，可隱式轉成任何指標型別與成員
//         指標，但不能轉成整數型別——`int n = nullptr;` 直接編譯錯。
//     追問：那 nullptr 能轉成 bool 嗎？（可以，但僅限 direct-initialization：
//         `bool b(nullptr)` 合法、`bool b = nullptr` 不合法，本機 g++ 已驗證）
//
// 🔥 Q2. 為什麼要特地給空指標一個 std::nullptr_t 型別？
//     答：為了讓「空指標」在模板推導與重載決議中有自己的身分。`decltype(nullptr)`
//         就是 nullptr_t，你可以為它寫專屬重載 `void f(std::nullptr_t)`，明確表達
//         「呼叫端是刻意不傳東西」，而不是傳了一個剛好等於 0 的整數。
//
// ⚠️ 陷阱. 同時有 f(int)、f(void*)、f(std::nullptr_t) 三個重載時，f(NULL) 選哪個？
//     答：一個都選不到——編譯錯誤 ambiguous。NULL 是整數 0，對 f(int) 是精準匹配，
//         對 f(std::nullptr_t) 也是合法的空指標常數轉換，兩者不分高下。本機 g++
//         已驗證（見 Demo 1 中刻意保留為註解、不呼叫的那一行）。
//     為什麼會錯：多數人把結論背成「NULL 會選到 f(int)」就停了。那只在「沒有
//         nullptr_t 重載」時成立；一旦有人補上 nullptr_t 版本，原本能編譯的舊呼叫
//         點會突然壞掉——這正是新程式碼不該再出現 NULL 的實際理由。
// ═══════════════════════════════════════════════════════════════════════════

#include <cstddef>
#include <iostream>

static void f(int)         { std::cout << "  f(int)\n"; }
static void f(void*)       { std::cout << "  f(void*)\n"; }
static void f(std::nullptr_t) { std::cout << "  f(nullptr_t)\n"; }

int main() {
    // ─────────────────────────────────────────────────────────
    // Demo 1：overload 解析
    // ─────────────────────────────────────────────────────────
    std::cout << "[Demo1] f(0):     "; f(0);       // f(int)
    // 注意：f(NULL) — 當有 nullptr_t overload 並存時，這行會「ambiguous」：
    //       NULL 同時能匹配 f(int) 與 f(nullptr_t)，編譯器拒絕猜。
    //       這正是「NULL 不安全」的最有力佐證 — 故保留註解、不呼叫。
    //       std::cout << "[Demo1] f(NULL):  "; f(NULL);   // ❌ ambiguous
    std::cout << "[Demo1] f(nullptr): "; f(nullptr); // f(nullptr_t)
    void* p = nullptr;
    std::cout << "[Demo1] f(p):     "; f(p);       // f(void*)

    // ─────────────────────────────────────────────────────────
    // Demo 2：條件檢查
    // ─────────────────────────────────────────────────────────
    int* q = nullptr;
    if (!q) std::cout << "[Demo2] q is null (via !q)\n";
    if (q == nullptr) std::cout << "[Demo2] q is null (via == nullptr)\n";

    // ─────────────────────────────────────────────────────────
    // Demo 3：nullptr 也能比較任何指標型別
    // ─────────────────────────────────────────────────────────
    char* cp = nullptr;
    int*  ip = nullptr;
    double* dp = nullptr;
    std::cout << "[Demo3] all null: "
              << (cp == nullptr) << ' '
              << (ip == nullptr) << ' '
              << (dp == nullptr) << '\n';

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：可以把 nullptr 賦給 int 嗎？
    //    A：不行。
    //         int n = nullptr;    // ❌ 編譯錯
    //       這是 nullptr 比 NULL 安全的核心。
    //
    //  Q2：nullptr 的 sizeof？
    //    A：sizeof(std::nullptr_t) == sizeof(void*) — 通常 8 byte（64-bit）。
    //
    //  Q3：可以 throw nullptr 嗎？
    //    A：可以但毫無意義（catch 端要 catch (std::nullptr_t)）。
    //       真要表達「空對象」的錯誤，丟具名 exception 比較合理。
    //
    // ─────────────────────────────────────────────────────────
    // LeetCode 21. Merge Two Sorted Lists
    //   題意：把兩個遞增 linked list 合併成一個遞增 linked list。
    //   為什麼放這？linked list 程式碼到處用 nullptr 判斷終點，
    //                示範 nullptr 在指標傳走鏈表時最自然的用法。
    // ─────────────────────────────────────────────────────────
    struct ListNode {
        int val;
        ListNode* next;
        ListNode(int v) : val(v), next(nullptr) {}
    };
    // 建兩個 sorted list：1->2->4 與 1->3->4
    ListNode n1(1), n2(2), n3(4);
    n1.next = &n2; n2.next = &n3;
    ListNode m1(1), m2(3), m3(4);
    m1.next = &m2; m2.next = &m3;

    auto mergeTwoLists = [](ListNode* a, ListNode* b) {
        ListNode dummy(0);                      // 用 dummy 簡化邊界處理
        ListNode* tail = &dummy;
        while (a != nullptr && b != nullptr) {   // 兩條都還沒走完
            if (a->val <= b->val) { tail->next = a; a = a->next; }
            else                  { tail->next = b; b = b->next; }
            tail = tail->next;
        }
        tail->next = (a != nullptr) ? a : b;     // 接上剩下的那一條
        return dummy.next;                       // 不會是 dummy 本身
    };
    ListNode* head = mergeTwoLists(&n1, &m1);
    std::cout << "[LC21] merged:";
    for (ListNode* p2 = head; p2 != nullptr; p2 = p2->next)
        std::cout << ' ' << p2->val;
    std::cout << '\n';

    // ─────────────────────────────────────────────────────────
    // 實用範例：可空指標 + nullptr_t overload 表達「明確不傳」
    //   工作上常見：log API 想表達「沒有 context」時呼叫 log(msg, nullptr)
    // ─────────────────────────────────────────────────────────
    struct Ctx { int id; };
    auto logMsg = [](const char* msg, const Ctx* c) {
        if (c == nullptr) std::cout << "[log] " << msg << " (no ctx)\n";
        else              std::cout << "[log] " << msg << " ctx.id=" << c->id << '\n';
    };
    Ctx ctx{99};
    logMsg("ready",   &ctx);
    logMsg("standby", nullptr);   // 明確表達「沒有 context」

    return 0;
}
