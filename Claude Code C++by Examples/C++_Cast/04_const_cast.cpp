// =============================================================================
//  04_const_cast.cpp  —  const_cast 詳解
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/language/const_cast
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、const_cast 能做什麼？                                  │
//  └────────────────────────────────────────────────────────────┘
//
//  唯一作用：「拿掉 const 或 volatile」（也能反向「加上」，但加 const 一般
//  不需要 cast，直接賦值就會 implicit conversion）。
//
//      const T*  → T*           ✅
//      T*        → const T*     ✅（隱式轉換即可，少數例外才需要明寫）
//      const T&  → T&           ✅
//
//  限制：const_cast 「只能改 const 屬性」，不能改型別本身。`const int*` 不
//  能 const_cast 成 `double*` — 那種要 reinterpret_cast。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、什麼時候算合法？什麼時候是 UB？                        │
//  └────────────────────────────────────────────────────────────┘
//
//  關鍵規則：
//
//    「對『原本就是 const』的物件做寫入 → UB」
//
//  也就是說：
//   * 原本物件就是 `const T x;`（或 string literal 之類） — 拿掉 const 後
//     寫入 → UB（編譯器可能把 x 放唯讀區段，crash；或可能依 const 假設做
//     優化導致行為不一致）
//   * 原本物件是 non-const，只是「現在拿在手上的指標被宣告 const」 —
//     const_cast 拿掉後可以安全寫
//
//  簡而言之：「我能不能寫」要看「真正的物件」是不是 const，不是看「我手
//  上的指標 / 參考型別」。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、實務上 const_cast 的合法用法                          │
//  └────────────────────────────────────────────────────────────┘
//
//   1. 跟「沒人 update」的舊 C API 互動。例如 strtok 接受 char* 但你只有
//      const char*；你保證該 API 不寫，可以 const_cast。
//   2. 在類別中「同時提供 const 與 non-const 版的 getter」 — 一邊呼叫另
//      一邊避免複製邏輯：
//
//         T& at(int i) {
//             return const_cast<T&>(static_cast<const Self&>(*this).at(i));
//         }
//
//   3. 解掉「以 by-ref 傳入卻有 const 標記」造成的 generic code 阻塞。
//
//  危險用法：對真正的 const object 做寫入 → 永遠是 UB，不要這麼做。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 本檔示範                                                   │
//  └────────────────────────────────────────────────────────────┘
//
//   * Demo 1：non-const 物件 → const ref → const_cast 可改
//   * Demo 2：真 const 物件被 const_cast 寫入 → UB（程式碼註解，不執行）
//   * Demo 3：替舊 C API 拆 const
// =============================================================================

/*
補充筆記：const cast
  - const_cast 只能改變 const/volatile 限定，不會改變物件原本是否真的是 const。
  - 若原物件宣告為 const，透過 const_cast 寫入是 undefined behavior。
  - 常見用途是和舊 C API 介接；新程式應優先修正 API const-correctness。
  - const cast 要先分清楚四種命名轉型：static_cast、dynamic_cast、const_cast、reinterpret_cast 各自解決不同問題。
  - static_cast 用於明確且語意合理的轉換，例如數值轉型、base/derived 已知方向；它不做執行期型別檢查。
  - dynamic_cast 用於 polymorphic base 上的安全向下轉型；失敗時 pointer 得到 nullptr，reference 會丟 std::bad_cast。
  - const_cast 只能調整 const/volatile 屬性；若原物件本來就是 const，移除 const 後修改是未定義行為。
  - reinterpret_cast 是低階重新解讀位元或位址，最容易違反 aliasing、alignment 和生命週期規則；能不用就不用。
  - C++ 風格轉型 (T)x 太模糊，可能偷偷做 const_cast 或 reinterpret_cast；教材應優先使用具名 cast 表達意圖。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】const_cast
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. const_cast 什麼時候是安全的？什麼時候是 UB？
//     答：要分清楚兩件事：【const_cast 本身只是改變型別的 cv 限定，單純轉型永遠
//         不是 UB】。真正的 UB 是「【修改一個原本就宣告為 const 的物件】」（透過
//         任何存取路徑都一樣）。所以安全的情況是：底層物件本身是 non-const，你只是
//         【透過一條 const 的路徑】（const 參考/指標）拿到它，此時 cast 掉再修改
//         完全合法。判斷標準是「真正的物件是不是 const」，不是「我手上的指標型別
//         是不是 const」。
//     追問：典型的正當場景？（呼叫設計不良、少寫 const 的舊 C API；你確定它不會
//           真的寫入時）
//
// 🔥 Q2. mutable 和 const_cast 該選哪個？
//     答：要在 const 成員函式中修改快取、mutex、統計計數這類「不影響物件邏輯狀態」
//         的成員，【一律用 mutable】 — 它是設計層面的正解，而且在該成員的宣告處
//         就明寫了「這東西可以在 const 物件裡被改」，reviewer 一眼看得到。
//         const_cast 是繞過型別系統的補救措施，而且一旦原物件真的是 const 就是 UB。
//
// Q3. const_cast 不能做什麼？它有哪些正當用途？
//     答：不能做的：【只能改變 cv 限定，不能改變型別本身】
//         （const_cast<int*>(charPtr) 編譯錯誤），也不能用在函式指標與成員函式
//         指標上。正當用途：① 介接沒寫 const 的第三方/舊 C API ② 消除 const 與
//         non-const 兩個版本 getter 的重複實作 —— non-const 版把 *this 轉成
//         const 呼叫 const 版，再 const_cast 掉回傳值的 const。後者是安全的，
//         因為呼叫端持有的物件本來就是 non-const。
//     追問：C++23 有更好的寫法嗎？（有，deducing this 可用一份 template 成員函式
//           同時涵蓋 const 與 non-const 版，不需要 const_cast）
//
// ⚠️ 陷阱. const_cast 掉 const 之後修改一個 const int，會發生什麼事？
//     答：【Undefined behavior】。實務上最經典的表現是「看起來成功了但值沒變」 —
//         編譯器可能把該 const 變數的讀取直接常數摺疊（constant folding）成字面值，
//         或把它放進唯讀記憶體段（.rodata）而導致 segfault。結果就是印出來的東西
//         自相矛盾：透過指標讀 *p 是新值，直接讀 x 還是舊值。
//     為什麼會錯：多數人的錯誤模型是「const 只是編譯器的檢查，執行期沒有這回事，
//         所以只要騙過編譯器就能改」。實際上 const 是給【最佳化器的承諾】 — 編譯器
//         會據此假設值不變並改寫程式碼，甚至決定變數的實體存放位置。
// ═══════════════════════════════════════════════════════════════════════════

#include <cstring>
#include <iostream>

// 模擬一個老 C API，要求 char* 但承諾不寫
static int legacyLength(char* s) {
    return static_cast<int>(std::strlen(s));
}

// 前置宣告：附加範例
static void demo_const_nonconst_getter_pair();
static void demo_legacy_api_strtok();

int main() {
    // ─────────────────────────────────────────────────────────
    // Demo 1：合法 — 原物件是 non-const
    // ─────────────────────────────────────────────────────────
    int x = 10;
    const int& cref = x;
    int& mref = const_cast<int&>(cref);   // 拿掉 const ref
    mref = 99;                              // 安全：x 本來就 non-const
    std::cout << "[Demo1] x = " << x << " (改寫成功)\n";

    // ─────────────────────────────────────────────────────────
    // Demo 2：UB 範例 — 不執行，只看程式碼
    //
    //     const int y = 7;
    //     int& bad = const_cast<int&>(y);   // 拿掉 const
    //     bad = 99;                          // ❌ UB：y 真的是 const
    //
    //   編譯器可能把 y 放唯讀記憶體（crash），或假設 y 永遠是 7、優化掉
    //   「讀 y」的指令（讀到的不是 99）。
    // ─────────────────────────────────────────────────────────
    std::cout << "[Demo2] (此處 UB 範例僅以註解保留，請看程式碼)\n";

    // ─────────────────────────────────────────────────────────
    // Demo 3：替老 C API 拆 const
    //   string literal "hello" 本身不可改 — 所以 legacyLength 不能寫它。
    //   但這裡只是「讀長度」，不會違規。
    // ─────────────────────────────────────────────────────────
    const char* msg = "hello world";
    int len = legacyLength(const_cast<char*>(msg));
    std::cout << "[Demo3] legacy length = " << len << '\n';

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：const_cast 比 C-style cast 安全嗎？
    //    A：const_cast 不會「順便做」其他奇怪的 cast；C-style cast 會悄悄
    //       同時做 reinterpret_cast。const_cast 表達意圖更窄、更安全。
    //
    //  Q2：mutable 跟 const_cast 有什麼關係？
    //    A：mutable 是「這個成員雖然在 const 物件裡也可以改」 — 用來實作
    //       cache / mutex。比 const_cast 更乾淨，因為它在「該成員的宣告處」
    //       就明寫。能用 mutable 就不要 const_cast。
    //
    //  Q3：在 const member function 裡修改成員怎麼做？
    //    A：兩條路：
    //         (a) 把該成員宣告 mutable（首選）
    //         (b) const_cast<This*>(this)->member = new_value;（會繞過 const，
    //             不建議；如果原物件是 const，行為是 UB）
    //
    demo_const_nonconst_getter_pair();
    demo_legacy_api_strtok();
    return 0;
}

// =============================================================================
//  附加 1：實用範例 — 避免「const 版與 non-const 版 getter」重複程式碼
// =============================================================================
//  Effective C++ Item 3 的經典 idiom：non-const 版內部呼叫 const 版，再
//  const_cast 把 const 拿掉。能避免「兩個 getter 內容幾乎一樣」的重複維護。
//  安全性：呼叫者一定是經由 non-const 物件呼叫的 non-const 版，所以原物件
//  本來就是 non-const，const_cast 是合法的。
// =============================================================================
class Container {
public:
    Container() { for (int i = 0; i < 4; ++i) buf_[i] = i * 10; }

    const int& at(int i) const { return buf_[i]; }     // const 版
    int& at(int i) {                                    // non-const 版
        // 把 *this 暫時當成 const，呼叫 const 版的 at；再把 const 拿掉
        return const_cast<int&>(
                   static_cast<const Container&>(*this).at(i));
    }
private:
    int buf_[4];
};
static void demo_const_nonconst_getter_pair() {
    Container c;
    c.at(2) = 999;                       // 走 non-const 版
    const Container& rc = c;
    std::cout << "[getter_pair] rc.at(2) = " << rc.at(2)
              << " (走 const 版，內容 999)\n";
}

// =============================================================================
//  附加 2：實用範例 — strtok 風格 API 整合
// =============================================================================
//  POSIX 的 strtok 需要 char*（會內部修改字串放 '\0'）。當你的字串是
//  std::string，「想呼叫舊 API、但又不想做拷貝」 — 這時 const_cast 是務實
//  選擇（前提：原字串是 non-const，且接受 API 真的會寫）。
//  注意：對 string literal 不可這麼做（會 UB）。
// =============================================================================
static void demo_legacy_api_strtok() {
    char buf[] = "a,b,c";              // 真實 non-const 陣列
    char* p = std::strtok(buf, ",");   // strtok 會就地改寫成 '\0'
    int cnt = 0;
    while (p) { ++cnt; p = std::strtok(nullptr, ","); }
    std::cout << "[strtok] split count = " << cnt << " (= 3)\n";
}

// 編譯: g++ -std=c++20 -Wall -Wextra 04_const_cast.cpp -o 04_const_cast

// === 預期輸出 ===
// [Demo1] x = 99 (改寫成功)
// [Demo2] (此處 UB 範例僅以註解保留，請看程式碼)
// [Demo3] legacy length = 11
// [getter_pair] rc.at(2) = 999 (走 const 版，內容 999)
// [strtok] split count = 3 (= 3)
