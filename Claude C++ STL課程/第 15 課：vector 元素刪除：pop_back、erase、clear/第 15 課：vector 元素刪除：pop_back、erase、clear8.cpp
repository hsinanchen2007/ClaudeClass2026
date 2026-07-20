// =============================================================================
//  第 15 課-8：在迴圈中刪除 vector 元素 —— 最容易寫錯的地方
// =============================================================================
//
// 【主題資訊 Information】
//   iterator erase(const_iterator pos);                 // 刪一個，回傳「下一個」
//   iterator erase(const_iterator first, const_iterator last);  // 刪一段
//   標準版本：C++98 起即有；C++11 起參數型別由 iterator 改為 const_iterator
//   複雜度：O(刪除點之後的元素個數)——後面的元素要整批往前搬
//   標頭檔：<vector>
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼「邊走邊刪」這麼容易錯】
//   vector 的元素是連續配置的。刪掉中間某個元素後，它後面的所有元素
//   都必須往前搬一格來填補空缺。這件事有兩個直接後果：
//     (a) 索引會位移——原本在 i+1 的元素，刪完之後跑到了 i；
//     (b) 指向刪除點及其之後位置的迭代器全部失效。
//   一般人寫刪除迴圈時，腦中的模型是「集合裡拿掉一個東西，其他不動」，
//   但 vector 的實際行為是「拿掉一個，後面全部往前擠」。
//   這個模型落差就是所有 bug 的來源。
//
// 【2. 錯誤做法一：用索引遞增，會「跳過」元素】
//   for (size_t i = 0; i < v.size(); ++i)
//       if (v[i] % 2 == 0) v.erase(v.begin() + i);
//   刪掉 v[i] 之後，原本的 v[i+1] 遞補到了 v[i]，
//   但迴圈的 ++i 仍然照走，於是遞補上來的那個元素「完全沒被檢查」。
//   對 {2, 4, 6} 這種連續符合條件的資料，會漏刪一半。
//   （這個 bug 特別陰險：對 {1,2,3,4} 這種交錯資料看起來是對的。）
//
// 【3. 錯誤做法二：erase 後繼續 ++it，是 undefined behavior】
//   for (auto it = v.begin(); it != v.end(); ++it)
//       if (*it % 2 == 0) v.erase(it);
//   erase(it) 之後 it 已經失效，接著對失效的迭代器做 ++ 與比較都是 UB。
//   注意這不是「值會不對」而是「整個程式的行為未定義」——
//   它可能崩潰、可能安靜地跑完、也可能只在 Release 版才出事。
//
// 【4. 正確做法：接住 erase 的回傳值】
//   erase 回傳「被刪元素的下一個位置」的有效迭代器，這正是為了讓
//   刪除迴圈能寫得下去而設計的。關鍵是把 ++it 從 for 的第三格移出來：
//     for (auto it = v.begin(); it != v.end(); ) {   // ← 這裡刻意留空
//         if (要刪) it = v.erase(it);                 // erase 已幫我們前進
//         else      ++it;                             // 只有不刪時才手動前進
//     }
//   「刪就不前進、不刪才前進」是這個慣用法的一句話心法。
//
// 【5. 複雜度警告：這種寫法是 O(n²)】
//   每次 erase 都要搬移後方元素，n 個元素刪掉一半就是約 n²/4 次搬移。
//   資料量大時應改用 erase-remove 慣用法（std::remove_if + 一次 erase），
//   那是 O(n)。本檔先把「逐一刪除」的正確性講清楚，
//   erase-remove 見同課的另一個範例檔。
//
// 【概念補充 Concept Deep Dive】
//   ▸ erase 到底回傳什麼
//     實作上，erase(pos) 會把 [pos+1, end) 的元素往前 move-assign 一格，
//     再 destroy 最後一個元素、把 size 減一，然後回傳 pos 本身
//     （因為原本 pos 的位置現在住著「下一個」元素）。
//     所以回傳值在數值上等於傳進去的 pos，但語意上是「新的下一個」。
//   ▸ 為什麼 erase 不會讓「刪除點之前」的迭代器失效
//     前面的元素完全沒有被搬動，記憶體位址不變。erase 也不會縮小
//     capacity（不重新配置），因此 [begin, pos) 區間的迭代器仍然有效。
//     這一點和 push_back 觸發重新配置時「全部失效」不同。
//   ▸ 刪最後一個元素時的邊界
//     若 pos 是最後一個元素，erase 回傳的就是 end()。
//     迴圈條件 it != v.end() 立刻不成立，自然結束，不需要特判。
//
// 【注意事項 Pay Attention】
//   1. erase 之後，原本那個迭代器就失效了，一定要用回傳值覆寫它。
//   2. for 的第三格必須留空，否則會變成「刪一個又跳一個」。
//   3. erase 使「刪除點及其之後」的迭代器/指標/引用失效；之前的仍有效。
//   4. 用索引寫也可以，但迴圈變數只在「不刪」時才 ++，邏輯和迭代器版相同。
//   5. 大量刪除請改用 erase-remove（O(n)），本檔的寫法最壞是 O(n²)。
//   6. C++20 起可直接用 std::erase_if(v, pred)，一行取代整個迴圈。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】在迴圈中刪除 vector 元素
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 for (auto it = v.begin(); it != v.end(); ++it) v.erase(it);
//        是錯的？正確寫法長什麼樣？
//     答：erase(it) 之後 it 立即失效，接下來的 ++it 與 it != v.end()
//         都在操作失效迭代器，屬於 undefined behavior。
//         正解是接住 erase 的回傳值（下一個有效位置），並把 ++it
//         移出 for 的第三格：刪就 it = v.erase(it)，不刪才 ++it。
//     追問：erase 的回傳值到底是「被刪的位置」還是「下一個元素」？
//         → 數值上等於傳進去的 pos，語意上是「下一個元素」——
//           因為後面的元素已經往前搬，現在住在那個位置了。
//
// 🔥 Q2. 用索引寫 for (size_t i = 0; i < v.size(); ++i) 並在裡面 erase，
//        會出什麼問題？
//     答：刪除後後方元素往前遞補，但 ++i 照樣執行，
//         導致遞補上來的元素被跳過、完全沒被檢查。
//         對 {2,4,6,8} 這種連續命中的資料會只刪掉一半。
//     追問：那要怎麼修？
//         → 刪除時不要 ++i（用 while 或在 else 分支才 ++i），
//           或者乾脆倒著跑迴圈——由後往前刪不會影響尚未走到的索引。
//
// ⚠️ 陷阱. 「erase 只會讓被刪的那個迭代器失效，其他的都還能用。」
//     答：不對。erase 會讓「刪除點及其之後」的所有迭代器、指標、引用失效，
//         因為那些元素都被往前搬過了，原位址現在存的是別的值。
//         只有「刪除點之前」的迭代器仍然有效。
//     為什麼會錯：把 vector 想成了 list。
//         list 的節點各自獨立，刪一個真的只影響那一個；
//         但 vector 是一塊連續記憶體，刪除必然伴隨後方元素的整批搬移。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <vector>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 27. Remove Element
//   題目：原地移除陣列中所有等於 val 的元素，回傳新長度；
//         超過新長度的部分內容不重要。
//   為什麼用到本主題：這題就是「在容器中刪除符合條件的元素」的最小化版本。
//         這裡刻意用本課的 erase 迴圈寫法，讓你看到它為什麼是 O(n²)——
//         LeetCode 的標準解答改用雙指標（快慢指標）把它壓到 O(n)，
//         而雙指標正是 std::remove_if 的內部做法。
// -----------------------------------------------------------------------------
int removeElementByErase(std::vector<int>& nums, int val) {
    for (auto it = nums.begin(); it != nums.end(); ) {
        if (*it == val) {
            it = nums.erase(it);   // 刪 → 不前進
        } else {
            ++it;                  // 不刪 → 才前進
        }
    }
    return static_cast<int>(nums.size());
}

// 對照組：LeetCode 官方期待的雙指標寫法，O(n)、零搬移
int removeElementTwoPointer(std::vector<int>& nums, int val) {
    size_t write = 0;
    for (size_t read = 0; read < nums.size(); ++read) {
        if (nums[read] != val) {
            nums[write++] = nums[read];
        }
    }
    nums.resize(write);
    return static_cast<int>(write);
}

// -----------------------------------------------------------------------------
// 【日常實務範例】連線池：移除已逾時中斷的連線
//   情境：伺服器維護一份 active connection 清單，健康檢查發現某些連線
//         心跳逾時，要把它們從清單中拿掉並記錄下來。
//   為什麼用本主題：這正是典型的「一邊走訪一邊刪除」場景，
//         而且刪除時還要順便做點事（記 log），所以不能只用 erase-remove。
// -----------------------------------------------------------------------------
struct Connection {
    std::string ip;
    int idleSeconds;
};

std::vector<std::string> reapIdleConnections(std::vector<Connection>& pool,
                                             int timeoutSeconds) {
    std::vector<std::string> reaped;
    for (auto it = pool.begin(); it != pool.end(); ) {
        if (it->idleSeconds >= timeoutSeconds) {
            reaped.push_back(it->ip);   // 先記錄，再刪除
            it = pool.erase(it);
        } else {
            ++it;
        }
    }
    return reaped;
}

int main() {
    std::cout << "=== 一、錯誤示範的原理說明（不實際執行）===" << std::endl;
    std::cout << "錯誤1：for(i) 內 erase → 後方元素遞補後被 ++i 跳過" << std::endl;
    std::cout << "錯誤2：erase(it) 後 ++it → 操作失效迭代器，屬 UB" << std::endl;

    std::cout << "\n=== 二、正確做法：接住 erase 的回傳值 ===" << std::endl;
    std::vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    std::cout << "刪除前: ";
    for (int x : v) std::cout << x << " ";
    std::cout << std::endl;

    for (auto it = v.begin(); it != v.end(); ) {
        if (*it % 2 == 0) {
            it = v.erase(it);   // erase 回傳下一個元素的迭代器
        } else {
            ++it;
        }
    }
    std::cout << "刪除偶數後: ";
    for (int x : v) std::cout << x << " ";
    std::cout << std::endl;

    std::cout << "\n=== 三、證明「錯誤做法一」真的會漏刪 ===" << std::endl;
    std::vector<int> bad = {2, 4, 6, 8};
    for (size_t i = 0; i < bad.size(); ++i) {
        if (bad[i] % 2 == 0) bad.erase(bad.begin() + static_cast<long>(i));
    }
    std::cout << "對 {2,4,6,8} 用錯誤寫法刪偶數，剩下: ";
    for (int x : bad) std::cout << x << " ";
    std::cout << "（應為空，卻漏刪一半）" << std::endl;

    std::cout << "\n=== 四、LeetCode 27. Remove Element ===" << std::endl;
    std::vector<int> nums1 = {3, 2, 2, 3};
    int len1 = removeElementByErase(nums1, 3);
    std::cout << "erase 迴圈版  nums={3,2,2,3}, val=3 → 長度 " << len1 << ", 內容: ";
    for (int x : nums1) std::cout << x << " ";
    std::cout << std::endl;

    std::vector<int> nums2 = {0, 1, 2, 2, 3, 0, 4, 2};
    int len2 = removeElementTwoPointer(nums2, 2);
    std::cout << "雙指標版      nums={0,1,2,2,3,0,4,2}, val=2 → 長度 " << len2 << ", 內容: ";
    for (int x : nums2) std::cout << x << " ";
    std::cout << std::endl;

    std::cout << "\n=== 五、日常實務：回收逾時連線 ===" << std::endl;
    std::vector<Connection> pool = {
        {"10.0.0.1", 5}, {"10.0.0.2", 120}, {"10.0.0.3", 30},
        {"10.0.0.4", 300}, {"10.0.0.5", 1}
    };
    std::cout << "回收前連線數: " << pool.size() << std::endl;
    std::vector<std::string> reaped = reapIdleConnections(pool, 60);
    std::cout << "已回收(閒置 >= 60s): ";
    for (const std::string& ip : reaped) std::cout << ip << " ";
    std::cout << std::endl;
    std::cout << "回收後剩餘: ";
    for (const Connection& c : pool) std::cout << c.ip << "(" << c.idleSeconds << "s) ";
    std::cout << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 15 課：vector 元素刪除：pop_back、erase、clear8.cpp" -o erase_loop

// === 預期輸出 ===
// === 一、錯誤示範的原理說明（不實際執行）===
// 錯誤1：for(i) 內 erase → 後方元素遞補後被 ++i 跳過
// 錯誤2：erase(it) 後 ++it → 操作失效迭代器，屬 UB
//
// === 二、正確做法：接住 erase 的回傳值 ===
// 刪除前: 1 2 3 4 5 6 7 8 9 10
// 刪除偶數後: 1 3 5 7 9
//
// === 三、證明「錯誤做法一」真的會漏刪 ===
// 對 {2,4,6,8} 用錯誤寫法刪偶數，剩下: 4 8 （應為空，卻漏刪一半）
//
// === 四、LeetCode 27. Remove Element ===
// erase 迴圈版  nums={3,2,2,3}, val=3 → 長度 2, 內容: 2 2
// 雙指標版      nums={0,1,2,2,3,0,4,2}, val=2 → 長度 5, 內容: 0 1 3 0 4
//
// === 五、日常實務：回收逾時連線 ===
// 回收前連線數: 5
// 已回收(閒置 >= 60s): 10.0.0.2 10.0.0.4
// 回收後剩餘: 10.0.0.1(5s) 10.0.0.3(30s) 10.0.0.5(1s)
