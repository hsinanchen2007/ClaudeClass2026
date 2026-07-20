// =============================================================================
//  第 17 課-7：接住 erase 的回傳值 —— 刪除迴圈唯一安全的寫法
// =============================================================================
//
// 【主題資訊 Information】
//   iterator vector<T>::erase(const_iterator pos);
//     回傳：被刪元素之後那個元素的迭代器；若刪的是最後一個，回傳 end()
//   標準版本：C++98 起；C++11 起參數型別改為 const_iterator
//   複雜度：O(end() - pos)，也就是「刪除點之後的元素個數」
//   失效範圍：刪除點及其之後的 iterator / pointer / reference 全部失效；
//             之前的仍然有效（因為 erase 不會重新配置，capacity 不變）
//   標頭檔：<vector>
//
// 【詳細解釋 Explanation】
//
// 【1. erase 為什麼要回傳一個迭代器】
//   這不是為了方便，是為了「可能性」。
//   erase(pos) 必然使 pos 失效，於是呼叫端手上再也沒有任何有效的
//   走訪位置——除非容器把新的位置還給你。
//   所以標準規定 erase 回傳「下一個有效元素」的迭代器，
//   這是刪除迴圈能夠繼續寫下去的唯一依據。
//   換句話說：不接住回傳值，就沒有安全的方式繼續走訪。
//
// 【2. 關鍵在把 ++it 移出 for 的第三格】
//     for (auto it = v.begin(); it != v.end(); /* 這裡刻意留空 */) {
//         if (要刪) it = v.erase(it);   // erase 已經幫我們「前進」了
//         else      ++it;               // 只有不刪時才需要自己前進
//     }
//   心法一句話：「刪就不前進，不刪才前進。」
//   若把 ++it 留在第三格，刪除後 erase 已經指向下一個、再 ++ 一次
//   就會跳過一個元素（而且那是在使用失效迭代器之後，本身已是 UB）。
//
// 【3. 為什麼 erase 不會使「前面」的迭代器失效】
//   erase 只做兩件事：把後方元素往前 move-assign 一格、把 end_ 指標減一。
//   它完全不碰 capacity、不重新配置、不搬動前段元素。
//   所以緩衝區的起始位址不變，[begin, pos) 這段的元素位址原封不動。
//   這一點和 push_back 觸發重新配置時「連 begin() 都失效」截然不同——
//   同樣叫「失效」，範圍差很多。
//
// 【4. 這個寫法的複雜度是 O(n²)，要知道代價】
//   每次 erase 都要把後方元素整批往前搬。
//   n 個元素刪掉一半，總搬移次數約 n²/4。
//   n = 100000 時是 25 億次搬移——這在正式環境會直接變成效能事故。
//   純粹「依條件批次刪除」請改用 erase-remove（見同課 8.cpp），那是 O(n)。
//   本寫法的適用時機是：刪除時還要對被刪元素做額外處理
//   （寫 log、釋放資源、通知外部），因為 remove_if 會直接把它們覆蓋掉。
//
// 【5. erase 之後 capacity 不會縮小】
//   刪除元素只減少 size，不歸還記憶體。
//   一個曾經裝過一百萬筆的 vector，清空後仍然佔著那塊記憶體。
//   要真正歸還必須呼叫 shrink_to_fit()（見同課 10.cpp）。
//   這是刻意的設計：避免「刪一個就縮一次、加一個又長回去」的反覆配置。
//
// 【概念補充 Concept Deep Dive】
//   ▸ erase 的回傳值在數值上等於傳進去的 pos
//     因為後方元素往前搬了一格，原本 pos 的位置現在住著「下一個」元素。
//     所以回傳值的位址和 pos 相同，但它指向的元素已經換人了。
//     理解這點，就明白為什麼「回傳值有效、傳入值失效」聽起來矛盾卻正確——
//     失效講的是「這個迭代器物件的語意保證」，不是「這個位址還能不能讀」。
//   ▸ 為什麼刪最後一個元素不需要特判
//     此時 erase 回傳 end()，迴圈條件 it != v.end() 立刻不成立，
//     自然結束。半開區間的設計讓邊界自動正確。
//   ▸ 倒著走也是一種解法
//     for (int i = (int)v.size() - 1; i >= 0; --i)
//         if (要刪) v.erase(v.begin() + i);
//     由後往前刪，不會影響尚未走到的索引。
//     但要小心迴圈變數必須用有號型別——用 size_t 時 i >= 0 恆真，
//     而且 i-- 會在 0 之後回繞成天文數字。
//
// 【注意事項 Pay Attention】
//   1. 一定要接住 erase 的回傳值；丟掉它就沒有安全的續走方式。
//   2. for 的第三格必須留空，否則會「刪一個又跳一個」。
//   3. erase 使刪除點及其之後失效；之前的仍有效（不重新配置）。
//   4. 本寫法最壞是 O(n²)；純批次刪除請改用 erase-remove（O(n)）。
//   5. erase 不縮小 capacity，記憶體不會自動歸還。
//   6. 倒序索引刪除時，迴圈變數務必用有號型別。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】erase 的回傳值與刪除迴圈
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. erase 回傳的迭代器，指向的是「被刪的那個位置」還是「下一個元素」？
//     答：語意上是「下一個元素」；位址上則與傳入的 pos 相同。
//         因為 erase 把後方元素往前搬了一格，原本 pos 的位置
//         現在住的正是下一個元素。這也是為什麼
//         it = v.erase(it) 之後不可以再 ++it——那會跳過一個。
//     追問：如果刪的是最後一個元素呢？
//         → 回傳 end()，迴圈條件 it != v.end() 立即不成立而正常結束，
//           不需要任何特判。這是半開區間帶來的紅利。
//
// 🔥 Q2. erase 會不會讓「刪除點之前」的迭代器也失效？為什麼？
//     答：不會。erase 只把後方元素往前搬並把 end_ 減一，
//         完全不重新配置、capacity 不變，緩衝區起始位址也不變，
//         所以 [begin, pos) 這段元素的位址原封不動。
//         這和 push_back 觸發重新配置時「連 begin() 都失效」不同。
//     追問：那 insert 呢？
//         → 若沒觸發重新配置，同樣只有插入點之後失效；
//           一旦觸發重新配置，就是全部失效——包含 begin()。
//
// ⚠️ 陷阱. 這樣寫刪除迴圈，為什麼結果會少刪？
//          for (auto it = v.begin(); it != v.end(); ++it)
//              if (*it % 2 == 0) it = v.erase(it);
//     答：兩個問題疊在一起。erase 回傳的已經是「下一個元素」，
//         但 for 第三格又執行了一次 ++it，於是每刪一個就額外跳過一個。
//         而且 it = v.erase(it) 之後那個 ++it 若剛好把 it 推過 end()，
//         迴圈條件就再也不成立不了，直接衝出邊界。
//     為什麼會錯：以為「接住回傳值」就萬事俱備，
//         卻忘了回傳值本身已經包含了「前進一步」的語意。
//         接住回傳值和把 ++it 移出第三格，是必須同時做的一組動作。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <vector>

// -----------------------------------------------------------------------------
// 【日常實務範例】排程器：移除已完成的任務，並在移除時歸還其佔用的資源
//   情境：排程器每個 tick 掃描一次任務清單，把狀態為 Done 的任務移出，
//         而且移出時必須「先關掉它持有的檔案句柄、再記一筆稽核日誌」。
//   為什麼用本主題（而不是 erase-remove）：
//         erase-remove 會直接用後面的元素覆蓋掉被刪的元素，
//         你根本來不及在它消失前對它做任何事。
//         凡是「刪除時還要處理被刪物件」的場景，
//         就只能用「接住 erase 回傳值」這個逐一刪除的寫法。
// -----------------------------------------------------------------------------
struct Task {
    int         id;
    std::string state;      // "Running" / "Done" / "Failed"
    int         fd;         // 假想的檔案句柄
};

std::vector<std::string> reapFinishedTasks(std::vector<Task>& tasks) {
    std::vector<std::string> auditLog;

    for (auto it = tasks.begin(); it != tasks.end(); /* 不在此 ++it */) {
        if (it->state == "Done" || it->state == "Failed") {
            // 關鍵：在元素被移除「之前」處理它
            auditLog.push_back("task#" + std::to_string(it->id) +
                               " state=" + it->state +
                               " closed fd=" + std::to_string(it->fd));
            it = tasks.erase(it);       // 刪 → 不前進
        } else {
            ++it;                        // 不刪 → 才前進
        }
    }
    return auditLog;
}

int main() {
    std::cout << "=== 一、正確的刪除迴圈 ===" << std::endl;
    std::vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8};

    std::cout << "刪除前：";
    for (int x : v) std::cout << x << " ";
    std::cout << std::endl;

    // 正確寫法：接住回傳值，且 ++it 不在 for 的第三格
    for (auto it = v.begin(); it != v.end(); /* 不在這裡 ++it */) {
        if (*it % 2 == 0) {
            it = v.erase(it);  // erase 回傳下一個有效迭代器
        } else {
            ++it;              // 只在不刪除時才前進
        }
    }

    std::cout << "刪除後：";
    for (int x : v) std::cout << x << " ";
    std::cout << std::endl;

    std::cout << "\n=== 二、erase 不會縮小 capacity ===" << std::endl;
    std::vector<int> w(1000, 7);
    std::cout << "初始     size=" << w.size() << ", capacity=" << w.capacity() << std::endl;
    w.erase(w.begin() + 10, w.end());
    std::cout << "刪到剩 10 size=" << w.size() << ", capacity=" << w.capacity()
              << "  ← 記憶體沒有歸還" << std::endl;
    w.shrink_to_fit();
    std::cout << "shrink 後 size=" << w.size() << ", capacity=" << w.capacity() << std::endl;

    std::cout << "\n=== 三、刪除點之前的迭代器仍然有效 ===" << std::endl;
    std::vector<int> u = {10, 20, 30, 40, 50};
    auto first = u.begin();               // 指向 10（在刪除點之前）
    u.erase(u.begin() + 2);               // 刪掉 30
    std::cout << "刪除索引 2 之後，*first = " << *first
              << "（仍然有效，因為 erase 不重新配置）" << std::endl;
    std::cout << "目前內容：";
    for (int x : u) std::cout << x << " ";
    std::cout << std::endl;

    std::cout << "\n=== 四、倒序索引刪除（另一種安全寫法）===" << std::endl;
    std::vector<int> r = {1, 2, 3, 4, 5, 6, 7, 8};
    for (int i = static_cast<int>(r.size()) - 1; i >= 0; --i) {   // 必須用有號型別
        if (r[static_cast<std::size_t>(i)] % 2 == 0) {
            r.erase(r.begin() + i);
        }
    }
    std::cout << "倒序刪除偶數後：";
    for (int x : r) std::cout << x << " ";
    std::cout << std::endl;

    std::cout << "\n=== 五、日常實務：回收已結束的任務 ===" << std::endl;
    std::vector<Task> tasks = {
        {101, "Running", 3}, {102, "Done", 4}, {103, "Running", 5},
        {104, "Failed", 6},  {105, "Done", 7}
    };
    std::cout << "回收前任務數：" << tasks.size() << std::endl;
    std::vector<std::string> log = reapFinishedTasks(tasks);
    std::cout << "稽核日誌：" << std::endl;
    for (const std::string& line : log) std::cout << "  " << line << std::endl;
    std::cout << "回收後仍在執行：";
    for (const Task& t : tasks) std::cout << "task#" << t.id << " ";
    std::cout << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 17 課：vector 的記憶體重新配置機制7.cpp" -o safe_erase
//
// 【關於下方預期輸出的但書】
//   第二段的 capacity 數值屬「實作定義」：本機為 GCC 15.2 / libstdc++，
//   vector<int>(1000, 7) 的初始 capacity 恰為 1000，
//   shrink_to_fit() 後縮為 10。其他標準庫實作的數值可能不同，
//   而且 shrink_to_fit 在標準上只是「非強制性請求」，允許不做任何事。
//
// 【本檔未附 LeetCode 範例的理由】
//   本檔的重點是「刪除時還需要對被刪元素做額外處理」這個
//   erase-remove 無法取代的場景（資源歸還、稽核日誌）。
//   LeetCode 的原地移除題（27、26、283）只關心最終陣列內容，
//   不存在「被刪元素還要善後」的需求，且已在第 15 課對應檔案中示範過；
//   在這裡重複同一題只會稀釋本檔真正想講的差異。

// === 預期輸出 ===
// === 一、正確的刪除迴圈 ===
// 刪除前：1 2 3 4 5 6 7 8
// 刪除後：1 3 5 7
//
// === 二、erase 不會縮小 capacity ===
// 初始     size=1000, capacity=1000
// 刪到剩 10 size=10, capacity=1000  ← 記憶體沒有歸還
// shrink 後 size=10, capacity=10
//
// === 三、刪除點之前的迭代器仍然有效 ===
// 刪除索引 2 之後，*first = 10（仍然有效，因為 erase 不重新配置）
// 目前內容：10 20 40 50
//
// === 四、倒序索引刪除（另一種安全寫法）===
// 倒序刪除偶數後：1 3 5 7
//
// === 五、日常實務：回收已結束的任務 ===
// 回收前任務數：5
// 稽核日誌：
//   task#102 state=Done closed fd=4
//   task#104 state=Failed closed fd=6
//   task#105 state=Done closed fd=7
// 回收後仍在執行：task#101 task#103
