// =============================================================================
//  課程 2.3：傳遞參數給執行緒2.cpp  —  為什麼 thread(f, v) 配 f(int&) 編譯不過
// =============================================================================
//
// 【本檔性質】本檔是上一位工程師的**更正檔**：原始教材寫的程式碼根本無法編譯，
//   卻宣稱它會印出「value = 1」。下方既有的 ⚠️ 區塊記錄了那次更正，
//   請保留閱讀，本段只是在其上補充完整的原理與面試觀點。
//
// 【主題資訊 Information】
//   template<class F, class... Args>
//   explicit thread(F&& f, Args&&... args);                       // C++11 起
//   標頭檔：<thread>、<functional>（std::ref / std::cref）
//
//   關鍵規則（[thread.thread.constr]）：新執行緒執行
//       INVOKE( decay-copy(f), decay-copy(args)... )
//   其中 decay-copy 出來的副本，在呼叫函式時是以 **rvalue** 傳入。
//   libstdc++ 對此有明確的 static_assert，訊息為：
//       "std::thread arguments must be invocable after conversion to rvalues"
//
// 【詳細解釋 Explanation】
//
// 【1. 錯誤的心智模型：「參數被複製，所以引用參數會看到副本」】
// 很多教材（包含本課原始版本）是這樣講的：
//     void modify(int& x) { x = 100; }
//     std::thread t(modify, value);      // 據稱：value 被複製，所以印出 1
// 這個說法**兩層都錯**：
//   * 它不會印出 1，因為它**根本不會編譯成功**。
//   * 錯誤也不是「型別不符」這麼單純，而是「副本以 rvalue 傳入」造成的綁定失敗。
//
// 【2. 真正的原因：rvalue 綁不到非 const 左值引用】
// 執行緒儲存區裡那份 int 副本，在被交給 modify 時是一個 rvalue（標準規定以
// std::move 的形式轉發）。C++ 的引用綁定規則是：
//     int&        ← 只能綁 lvalue                    → 綁不到，編譯失敗
//     const int&  ← 可以綁 lvalue 也可以綁 rvalue    → 綁得到，但綁的是副本
//     int&&       ← 只能綁 rvalue                    → 綁得到副本
// 所以 f(int&) 這個組合是**編譯期就被擋下來**的。
// 本機實測（GCC 15.2.0 / libstdc++）錯誤訊息：
//     error: static assertion failed:
//            std::thread arguments must be invocable after conversion to rvalues
//
// 【3. 為什麼標準要故意設計成「以 rvalue 傳入」？】
// 這是刻意的安全設計，不是實作偷懶。理由有二：
//   (a) 讓 move-only 型別（如 std::unique_ptr）可以傳進執行緒：副本必須能被移動走，
//       否則 thread(f, std::move(p)) 這種寫法無法成立（見本課第 6 個檔案）。
//   (b) **把「假引用」變成編譯錯誤**。如果副本以 lvalue 傳入，那 f(int&) 會安靜地
//       通過編譯、安靜地改到副本、而呼叫端的原值永遠不變——一個沒有任何診斷訊息的
//       邏輯 bug。標準寧可讓你在編譯期就撞牆，逼你明確表態要 std::ref 還是要傳值。
//
// 【4. 兩種正確寫法，先決定語意再選工具】
//   (A) 只是要「執行緒有自己的一份資料」→ 參數就收 by value：void f(int x)
//       語意清楚：呼叫端與執行緒互不干擾，沒有任何生命週期問題。
//   (B) 真的要改到原值 → 參數收 int&，呼叫端寫 std::ref(v)
//       此時你拿回了引用語意，但也**拿回了生命週期責任**：必須自己保證 v 活得比
//       執行緒久（本檔用 join() 達成，見下方 main）。
//
// 【概念補充 Concept Deep Dive】
//
// (A) static_assert 而不是一長串 SFINAE 錯誤
//   libstdc++ 在 std_thread.h 裡先用 __is_invocable 檢查，不通過就丟一行人話錯誤。
//   若沒有這個 static_assert，你看到的會是 tuple/_Invoker 深處數十行的模板展開錯誤。
//   這是「函式庫作者主動改善錯誤訊息」的典型例子，也是為什麼記住這句訊息很有價值：
//   看到它就知道是「參數綁定」問題，而不是別的。
//
// (B) 為什麼 std::ref 能繞過這個限制
//   std::ref(v) 產生 std::reference_wrapper<int>。decay-copy 複製的是這個包裝物件
//   （內部只有一個指標），複製它完全合法。呼叫時 reference_wrapper 透過
//   隱式轉換運算子 operator T&() 還原成 int&——而**這個轉換的結果是 lvalue**，
//   所以可以順利綁到 int&。整條鏈路上沒有任何一步違反引用綁定規則。
//
// (C) 編譯器的 -Wunused-but-set-parameter 警告
//   本檔 modify_copy(int x) 中 x = 100 之後就沒被讀取，GCC 會warn：
//       warning: parameter 'x' set but not used [-Wunused-but-set-parameter]
//   這個警告**不是缺陷，正是本檔要示範的事實的證據**：那次賦值確實沒有任何
//   對外可見的效果，連編譯器都看得出來這是一次「寫了等於沒寫」的操作。
//   刻意保留不消音，讓警告本身成為教材的一部分。
//
// 【注意事項 Pay Attention】
// 1. 「編譯不過」與「行為不如預期」是兩件事，不可混講。本例是前者。
// 2. 換成 const int& 就會編譯成功——但綁到的是副本，不是原值（見第 1 個檔案的實測）。
//    從「編不過」變成「編得過但沒有引用語意」，危險程度其實更高。
// 3. 用了 std::ref 就等於放棄了 std::thread 提供的生命週期保護，
//    必須自行確保被引用物件的存活期涵蓋整個執行緒生命期。
// 4. 這個 static_assert 是 libstdc++ 的診斷措辭；MSVC / libc++ 的訊息文字不同，
//    但「不可編譯」這個結果是標準要求的，不是實作差異。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】thread 參數的 rvalue 轉換與 std::ref
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. void modify(int& x) 配 std::thread t(modify, value)，程式會印出什麼？
//     答：什麼都印不出來——**它編譯失敗**。執行緒儲存區的副本以 rvalue 傳入，
//         rvalue 綁不到 int&，libstdc++ 直接 static_assert：
//         "std::thread arguments must be invocable after conversion to rvalues"。
//     追問：那把參數改成 const int& 呢？
//         → 會編譯成功，但 x 綁到的是執行緒內部的副本，不是原本的 value；
//           原值一樣不會被修改，而且這次連編譯器都不會提醒你。
//
// 🔥 Q2. 標準為什麼要規定「以 rvalue 傳入」？直接以 lvalue 傳入不是更方便？
//     答：兩個理由。一是為了支援 move-only 型別（unique_ptr 必須能被移動進函式）；
//         二是為了把「以為傳了引用、其實傳了副本」這個沉默的邏輯 bug
//         提升成編譯期錯誤，強迫你用 std::ref 明確表態。
//     追問：那 std::ref 為什麼就能通過？
//         → 複製的是 reference_wrapper（合法可複製），呼叫時經 operator T&()
//           還原成 lvalue，因此能綁到 int&。
//
// ⚠️ 陷阱. 「std::thread 會複製參數」→ 所以把大物件改成引用參數就能避免複製？
//     答：不能。無論函式簽名是 T、T& 還是 const T&，std::thread 一律對**你傳進去的
//         實參**做 decay-copy。改函式簽名完全不影響是否複製；
//         要避免複製只能改**呼叫端**：std::ref / std::cref / std::move / 傳指標。
//     為什麼會錯：把「函式參數的傳遞方式」和「thread 建構子的參數儲存」搞混成同一件事。
//         它們是兩層獨立的傳遞：先由 thread 建構子複製到儲存區（這層你只能用
//         std::ref 等工具控制），再由儲存區以 rvalue 交給函式（這層才輪到函式簽名）。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <thread>
#include <functional>   // std::ref

// ⚠️ 本檔更正（原版根本編不過）：
//    原本寫 void modify(int& x) 配 std::thread t(modify, value);
//    這【不是】「value 被複製所以印出 1」——而是**編譯失敗**：
//    std::thread 會先 decay-copy 參數存起來,再以 rvalue 去呼叫,
//    rvalue 綁不到 non-const lvalue reference,於是 libstdc++ 直接
//    static_assert:「std::thread arguments must be invocable after
//    conversion to rvalues」。
//
//    所以要分成兩種寫法,想示範哪一種就用哪一種:

// (A) 想示範「執行緒拿到的是副本、改不到原值」→ 參數就收 by value
void modify_copy(int x) {
    x = 100;                       // 只改到副本
}

// (B) 想真的改到原值 → 參數收 reference,呼叫端必須用 std::ref 包起來
void modify_ref(int& x) {
    x = 100;
}

int main() {
    // (A) 傳值:原值不變
    int value = 1;
    std::thread t1(modify_copy, value);
    t1.join();
    std::cout << "傳值   後 value = " << value << std::endl;   // 1

    // (B) std::ref:原值被改
    int value2 = 1;
    std::thread t2(modify_ref, std::ref(value2));
    t2.join();
    std::cout << "std::ref 後 value = " << value2 << std::endl; // 100

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread "課程 2.3：傳遞參數給執行緒2.cpp" -o pass_args2
//
// 編譯時會出現一則刻意保留的警告（見上方【概念補充 (C)】）：
//   warning: parameter 'x' set but not used [-Wunused-but-set-parameter]
// 它正是「改到副本等於沒改」的證據，不是缺陷，故不消音。

// === 預期輸出 ===
// 傳值   後 value = 1
// std::ref 後 value = 100
//
// 對照：原始教材版本（void modify(int&) 配 std::thread t(modify, value)）
// 在本機 GCC 15.2.0 / libstdc++ 下的實測結果是**編譯失敗**，沒有任何輸出：
//   /usr/include/c++/15/bits/std_thread.h:168:72: error: static assertion failed:
//     std::thread arguments must be invocable after conversion to rvalues
// 這就是本檔存在的原因——原版宣稱它會印出「value = 1」，那個輸出從來不存在。
