// =============================================================================
//  第 15 課：vector 元素刪除 2  —  空容器保護：pop_back 的前置條件
// =============================================================================
//
// 【主題資訊 Information】
//   void pop_back();                      // 前置條件：!empty()
//   bool empty() const noexcept;          // O(1)，等價於 size() == 0
//   標頭檔：<vector>
//   標準版本：兩者皆 C++98（empty 的 noexcept 標註自 C++11）。
//   本檔的主題只有一句話：
//     【pop_back 不會替你檢查容器是否為空，那是呼叫端的責任。】
//   違反前置條件的後果是【未定義行為】，不是丟例外、也不是靜默無事。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼標準要把「不得為空」訂成前置條件，而不是內建檢查】
//   這是 STL 的一貫立場：容器只提供必要的操作，不強加額外成本。
//   如果 pop_back 內建 `if (empty()) throw;`：
//     * 那些「已經確定非空」的呼叫端也得付這份檢查成本
//     * 而他們往往是最在意效能的人（堆疊操作在熱迴圈裡很常見）
//   標準的選擇是：把檢查交給知道上下文的呼叫端。
//   對比 at() —— 那是【明確為了檢查而存在】的另一個函式。
//   vector 沒有提供「檢查版的 pop_back」，因為 `if (!v.empty())`
//   本身就已經足夠簡單。
//
// 【2. 這個 UB 的具體形態（libstdc++）】
//   libstdc++ 的 pop_back 大致是：
//       void pop_back() {
//           --this->_M_impl._M_finish;          // 結尾指標往前挪
//           _Alloc_traits::destroy(..., this->_M_impl._M_finish);
//       }
//   對空容器來說，_M_finish 本來就等於 _M_start，
//   往前挪一格就退到了【配置區塊之外】。接著：
//     * size() = _M_finish - _M_start 變成 -1，
//       但 size_type 是無號的 → 環繞成 18446744073709551615
//     * destroy 對一個不屬於自己的位址呼叫解構子
//   之後這個 vector 的所有操作都不可預測。
//   最惡劣的是：這【不會當場崩潰】。程式會帶著壞掉的狀態繼續跑，
//   直到很久之後某個無關的地方爆炸。
//
// 【3. 三種寫法的取捨】
//   (a) if (!v.empty()) v.pop_back();
//       最直接。適合「空了就跳過」的語意。
//   (b) assert(!v.empty()); v.pop_back();
//       表達「這裡不可能是空的，若是就是我的 bug」。
//       注意 assert 在 NDEBUG（release build）下會被【整個移除】，
//       所以它是除錯工具，不是執行期保護。
//   (c) if (v.empty()) throw std::logic_error(...);
//       表達「空容器是可處理的錯誤狀態」，讓上層決定怎麼辦。
//   選哪一種取決於「空容器對你的程式代表什麼」——
//   正常情況、程式 bug、還是可回報的錯誤。
//
// 【4. 同樣需要前置條件的還有這些】
//   vector 中「不檢查」的成員是多數，不是少數：
//       front()  back()  operator[]  pop_back()
//       erase(pos)  —— pos 必須是可解參考的合法迭代器
//   唯一會檢查的只有 at()。把「不檢查」當成 vector 的預設立場，
//   at() 當成例外，記憶起來會比較準確。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 怎麼在開發期自動抓到這類錯誤
//     libstdc++ 提供兩個層級的除錯開關（編譯時定義即可）：
//       -D_GLIBCXX_ASSERTIONS
//           輕量。對 operator[]、front、back、pop_back 加上前置條件檢查，
//           違反時直接 abort 並印出明確訊息。效能影響小，
//           很多發行版的 debug build 預設就開。
//       -D_GLIBCXX_DEBUG
//           完整的 debug mode。會換掉容器的實作，額外檢查迭代器有效性、
//           範圍合法性等。很慢，但抓得最徹底。
//     另外 -fsanitize=address 能抓到越界讀寫，
//     -fsanitize=undefined 能抓到部分未定義行為。
//     這些都應該在 CI 的測試流程裡開啟。
//
// (B) 為什麼 empty() 而不是 size() == 0
//     對 vector 而言兩者等價，都是 O(1)。
//     但對其他容器不一定——C++11 之前的 std::list::size() 在某些實作上
//     是 O(n)（要走訪整條鏈結串列），而 empty() 永遠是 O(1)。
//     C++11 起標準要求所有容器的 size() 都是 O(1)，這個差異消失了，
//     但「判斷空就用 empty()」仍是好習慣：它語意更直接，
//     而且對任何容器都保證最快。
//
// (C) 這一課的教訓可以推廣：讀函式文件時要找「前置條件」
//     cppreference 上每個函式都有一段 Preconditions 或
//     "The behavior is undefined if ..."。那一段才是真正的契約。
//     複雜度、回傳值都可以事後查，但違反前置條件的代價是無法補救的。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】前置條件與未定義行為
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 對空 vector 呼叫 pop_back() 會發生什麼事？
//     答：未定義行為。標準明訂前置條件是 !empty()。
//         以 libstdc++ 為例，它會把結尾指標往前挪到配置區塊之外，
//         導致 size() 無號環繞成天文數字、容器狀態毀壞，
//         而且【通常不會當場崩潰】——錯誤會在遠離現場的地方浮現。
//         正確做法是呼叫前先 if (!v.empty())。
//     追問：為什麼標準不乾脆讓它丟例外？
//         → 因為那會強迫所有呼叫端付出檢查成本，
//           包括那些已經確定非空的熱路徑。
//           STL 的立場是「不檢查是預設，at() 是例外」。
//
// 🔥 Q2. 開發時要怎麼自動抓出這類「違反前置條件」的錯誤？
//     答：用標準函式庫的除錯開關。libstdc++ 有兩個層級：
//         -D_GLIBCXX_ASSERTIONS（輕量，檢查 operator[]/front/back/pop_back
//         等的前置條件，違反就 abort 並印訊息）
//         -D_GLIBCXX_DEBUG（完整 debug mode，還檢查迭代器有效性，較慢）。
//         另可搭配 -fsanitize=address,undefined。
//         這些都該在 CI 的測試建置中開啟。
//     追問：那 release build 呢？
//         → release 不該依賴這些檢查。前置條件要靠程式碼結構保證，
//           而不是靠執行期偵測。
//
// ⚠️ 陷阱. 「我用 assert(!v.empty()) 保護了 pop_back，所以上線後也安全」
//         ——這個推論錯在哪？
//     答：assert 在定義了 NDEBUG 時會被前置處理器【完全移除】，
//         而 release build 幾乎一定會定義 NDEBUG（-DNDEBUG 是慣例，
//         CMake 的 Release 設定預設就加）。
//         所以上線後那行 assert 根本不存在，保護是零。
//         assert 是「開發期抓自己的 bug」的工具，
//         不是「執行期防止壞事發生」的機制。
//     為什麼會錯：把 assert 當成 if。兩者長得像，語意完全不同：
//         assert 說的是「這件事不可能發生，若發生就是我寫錯了」，
//         它的存在假設是【正確的程式永遠不會觸發它】。
//         若空容器是【可能真實發生】的狀態，就該用 if 或丟例外，
//         那是控制流程，不是除錯斷言。
// ═══════════════════════════════════════════════════════════════════════════

#include <vector>
#include <iostream>
#include <string>
#include <stdexcept>

// -----------------------------------------------------------------------------
// 【日常實務範例】工作佇列的取件迴圈（三種空容器策略的對照）
//   情境：背景執行緒從待處理佇列取出任務。「佇列空了」是完全正常的狀態，
//         不是錯誤——所以這裡用 if 判斷，不用 assert 也不丟例外。
//   為什麼用到本主題：這正是「空容器代表什麼」決定寫法的實例。
//     同一個 pop_back，在三種語意下要配三種不同的保護：
//       ① 佇列取件：空了是正常 → if (!empty())
//       ② 內部不變式：空了是 bug → assert（僅開發期）
//       ③ 外部 API：空了要回報 → throw
//   下面三個函式各示範一種。
// -----------------------------------------------------------------------------

// ① 空了是正常狀態 → 用 if，回傳是否成功
bool tryTakeTask(std::vector<std::string>& queue, std::string& out) {
    if (queue.empty()) return false;
    out = queue.back();
    queue.pop_back();
    return true;
}

// ③ 空了代表呼叫端用錯了 → 丟例外，讓上層決定怎麼處理
std::string takeTaskOrThrow(std::vector<std::string>& queue) {
    if (queue.empty()) {
        throw std::logic_error("工作佇列是空的，不能取件");
    }
    std::string t = queue.back();
    queue.pop_back();
    return t;
}

int main() {
    std::vector<int> v;

    std::cout << "=== 原始示範：空容器的安全處理 ===\n";
    std::cout << "v.size()  = " << v.size() << "\n";
    std::cout << "v.empty() = " << std::boolalpha << v.empty() << "\n";

    // 危險！未定義行為
    // v.pop_back();

    // 安全做法
    if (!v.empty()) {
        v.pop_back();
        std::cout << "刪除了一個元素\n";
    } else {
        std::cout << "容器是空的 → 沒有呼叫 pop_back（正確）\n";
    }

    std::cout << "\n=== 為什麼那行被註解掉 ===\n";
    std::cout << "對空 vector 呼叫 pop_back() 是未定義行為。\n";
    std::cout << "libstdc++ 的實作會把結尾指標往前挪到配置區塊之外：\n";
    std::cout << "  * size() = 結尾 - 開頭 變成 -1，但 size_type 是無號的\n";
    std::cout << "    → 環繞成 18446744073709551615\n";
    std::cout << "  * 接著對一個不屬於容器的位址呼叫解構子\n";
    std::cout << "最惡劣的是它【不會當場崩潰】，程式會帶著壞掉的狀態繼續跑。\n";
    std::cout << "本檔不執行它——UB 沒有正確答案，任何一次觀察都不能當教材。\n";

    std::cout << "\n=== 三種空容器策略的對照 ===\n";
    std::cout << "(a) if (!v.empty())      —— 空了是正常狀態，跳過就好\n";
    std::cout << "(b) assert(!v.empty())   —— 空了代表 bug（僅開發期有效！）\n";
    std::cout << "(c) if (empty()) throw   —— 空了是可回報的錯誤\n";
    std::cout << "選哪個取決於「空容器對你的程式代表什麼」。\n";

    std::cout << "\n=== assert 在 release build 會消失 ===\n";
#ifdef NDEBUG
    std::cout << "本次編譯【有】定義 NDEBUG → assert 已被完全移除，保護為零\n";
#else
    std::cout << "本次編譯【沒有】定義 NDEBUG → assert 有效\n";
#endif
    std::cout << "（CMake 的 Release 設定預設會加 -DNDEBUG，\n";
    std::cout << "  所以 assert 不能當成上線後的保護機制）\n";

    std::cout << "\n=== vector 中「不檢查」的成員是多數 ===\n";
    std::cout << "不檢查（違反前置條件即 UB）: operator[]  front()  back()\n";
    std::cout << "                              pop_back()  erase(pos)\n";
    std::cout << "會檢查（越界丟 out_of_range）: at()  ← 唯一的例外\n";
    std::cout << "→ 把「不檢查」當成 vector 的預設立場會比較準確。\n";

    std::cout << "\n=== 開發期的自動偵測工具 ===\n";
    std::cout << "g++ -D_GLIBCXX_ASSERTIONS  ...  # 輕量：檢查前置條件，違反即 abort\n";
    std::cout << "g++ -D_GLIBCXX_DEBUG       ...  # 完整 debug mode：另檢查迭代器有效性\n";
    std::cout << "g++ -fsanitize=address     ...  # 抓越界讀寫\n";
    std::cout << "g++ -fsanitize=undefined   ...  # 抓部分未定義行為\n";
    std::cout << "這些應該在 CI 的測試建置中開啟，而不是依賴人工檢查。\n";

    std::cout << "\n=== 日常實務：工作佇列取件 ===\n";
    {
        std::vector<std::string> queue = {"寄送通知信", "產生報表", "清理暫存檔"};
        std::cout << "佇列有 " << queue.size() << " 件工作\n";

        // ① 空了是正常狀態 → 用 if 判斷
        std::string task;
        while (tryTakeTask(queue, task)) {
            std::cout << "  處理: " << task << "（剩 " << queue.size() << " 件）\n";
        }
        std::cout << "佇列已清空，再取一次: "
                  << (tryTakeTask(queue, task) ? "取到了" : "回傳 false（正常，不是錯誤）")
                  << "\n";

        // ③ 空了要回報 → 丟例外
        std::cout << "\n改用會丟例外的版本：\n";
        try {
            takeTaskOrThrow(queue);
        } catch (const std::logic_error& e) {
            std::cout << "  捕捉到例外: " << e.what() << "\n";
        }

        queue.push_back("重新排程的工作");
        std::cout << "  補一件工作後再取: " << takeTaskOrThrow(queue) << "\n";
        std::cout << "→ 同一個 pop_back，三種語意就要配三種不同的保護。\n";
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第 15 課：vector 元素刪除2.cpp -o demo2
// 開發期建議加上前置條件檢查：
//        g++ -std=c++17 -D_GLIBCXX_ASSERTIONS -Wall -Wextra <本檔> -o demo2_chk

// === 預期輸出 ===
// === 原始示範：空容器的安全處理 ===
// v.size()  = 0
// v.empty() = true
// 容器是空的 → 沒有呼叫 pop_back（正確）
//
// === 為什麼那行被註解掉 ===
// 對空 vector 呼叫 pop_back() 是未定義行為。
// libstdc++ 的實作會把結尾指標往前挪到配置區塊之外：
//   * size() = 結尾 - 開頭 變成 -1，但 size_type 是無號的
//     → 環繞成 18446744073709551615
//   * 接著對一個不屬於容器的位址呼叫解構子
// 最惡劣的是它【不會當場崩潰】，程式會帶著壞掉的狀態繼續跑。
// 本檔不執行它——UB 沒有正確答案，任何一次觀察都不能當教材。
//
// === 三種空容器策略的對照 ===
// (a) if (!v.empty())      —— 空了是正常狀態，跳過就好
// (b) assert(!v.empty())   —— 空了代表 bug（僅開發期有效！）
// (c) if (empty()) throw   —— 空了是可回報的錯誤
// 選哪個取決於「空容器對你的程式代表什麼」。
//
// === assert 在 release build 會消失 ===
// 本次編譯【沒有】定義 NDEBUG → assert 有效
// （CMake 的 Release 設定預設會加 -DNDEBUG，
//   所以 assert 不能當成上線後的保護機制）
//
// === vector 中「不檢查」的成員是多數 ===
// 不檢查（違反前置條件即 UB）: operator[]  front()  back()
//                               pop_back()  erase(pos)
// 會檢查（越界丟 out_of_range）: at()  ← 唯一的例外
// → 把「不檢查」當成 vector 的預設立場會比較準確。
//
// === 開發期的自動偵測工具 ===
// g++ -D_GLIBCXX_ASSERTIONS  ...  # 輕量：檢查前置條件，違反即 abort
// g++ -D_GLIBCXX_DEBUG       ...  # 完整 debug mode：另檢查迭代器有效性
// g++ -fsanitize=address     ...  # 抓越界讀寫
// g++ -fsanitize=undefined   ...  # 抓部分未定義行為
// 這些應該在 CI 的測試建置中開啟，而不是依賴人工檢查。
//
// === 日常實務：工作佇列取件 ===
// 佇列有 3 件工作
//   處理: 清理暫存檔（剩 2 件）
//   處理: 產生報表（剩 1 件）
//   處理: 寄送通知信（剩 0 件）
// 佇列已清空，再取一次: 回傳 false（正常，不是錯誤）
//
// 改用會丟例外的版本：
//   捕捉到例外: 工作佇列是空的，不能取件
//   補一件工作後再取: 重新排程的工作
// → 同一個 pop_back，三種語意就要配三種不同的保護。
