// =============================================================================
//  第 15 課：vector 元素刪除 4  —  erase 的回傳值：迴圈刪除的關鍵
// =============================================================================
//
// 【主題資訊 Information】
//   iterator erase(const_iterator pos);
//   iterator erase(const_iterator first, const_iterator last);
//   標頭檔：<vector>
//   標準版本：C++98（C++11 起參數改為 const_iterator）。
//   回傳值的定義：
//     * erase(pos)        → 指向「被刪元素之後那個元素」的 iterator
//     * erase(first,last) → 指向「last 原本所指元素」的 iterator
//     * 若刪到容器結尾    → 回傳 end()
//   關鍵：回傳的是【重新計算過、保證有效】的迭代器。
//         這正是它存在的理由——因為 erase 使原本的迭代器全部失效。
//
// 【詳細解釋 Explanation】
//
// 【1. 這個回傳值不是方便，是必需】
//   erase 使 pos 及其之後的所有迭代器失效。於是「刪完之後要從哪繼續」
//   就成了一個真問題：你手上的 it 已經不能用了。
//   標準的解法是讓 erase 自己回傳一個保證有效的新位置。
//   這使得下面這個慣用法成為唯一正確的迴圈刪除寫法：
//       for (auto it = v.begin(); it != v.end(); ) {
//           if (shouldDelete(*it)) it = v.erase(it);   // 刪除 → 用回傳值續行
//           else                   ++it;               // 不刪 → 自己前進
//       }
//   注意 for 的第三格是【空的】。遞增必須寫在 else 裡，
//   因為刪除的那一輪不該再 ++（erase 已經把 it 指到下一個了）。
//
// 【2. 三種錯誤寫法，各錯在哪裡】
//   (a) for (auto it = v.begin(); it != v.end(); ++it)
//           if (cond) v.erase(it);
//       → erase 之後 it 已失效，接著 ++it 是未定義行為。
//         實務上常常「看起來能跑」，因為 vector 的迭代器就是指標，
//         但這是實作細節，不是保證（debug mode 下會直接 abort）。
//
//   (b) for (auto it = v.begin(); it != v.end(); )
//           if (cond) { v.erase(it); }       // 忘了接回傳值
//           else ++it;
//       → 無窮迴圈。it 沒有前進，若條件恆真就永遠卡住。
//
//   (c) for (size_t i = 0; i < v.size(); ++i)
//           if (cond) v.erase(v.begin() + i);
//       → 會【跳過】元素。刪掉索引 i 之後，原本 i+1 的元素補到了 i，
//         但迴圈接著 ++i 跳到 i+1，那個補位過來的元素就沒被檢查到。
//         連續兩個都該刪時，第二個會存活。
//         （若真的要用索引，刪除時就【不要】遞增。）
//
// 【3. 為什麼 erase(first, last) 回傳的是「last 原本指的元素」】
//   刪掉 [first, last) 之後，原本在 last 位置的元素往前補到了 first。
//   所以回傳值等於「更新後的 first」。這讓範圍刪除也能用在迴圈裡：
//       it = v.erase(rangeStart, rangeEnd);   // it 指向被刪範圍之後的第一個
//   一個常見的推論：erase(v.begin(), v.end()) 回傳 end()，
//   因為刪光之後 begin() == end()。
//
// 【4. 但迴圈逐一 erase 仍然是 O(n²)】
//   即使寫法正確，這個 pattern 的複雜度也是 O(n²)——
//   每次 erase 都要搬移後半段。刪 n/2 個元素就是 n/2 次 O(n)。
//   它適合「只刪少數幾個」的場合。要批次刪除請用
//   erase-remove 慣用法（第 10 檔）或 C++20 的 std::erase_if（第 11 檔），
//   那才是 O(n)。本課第 12 檔有實測對照。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 其他容器的 erase 也回傳迭代器，但意義稍有不同
//     std::map / std::set 的 erase(pos) 在 C++11 起也回傳下一個迭代器
//     （C++98 時 map::erase 回傳 void，所以當年的慣用法是
//      `m.erase(it++);` —— 利用後置遞增先取值再前進）。
//     std::list 的 erase 一樣回傳下一個。
//     所以 `it = c.erase(it)` 這個慣用法對三大類容器都通用，
//     值得當成肌肉記憶。
//
// (B) 為什麼不設計成「erase 自動幫你前進迭代器」
//     因為 erase 收到的是【值】不是【參考】——它無法修改呼叫端的變數。
//     即使能改，那也會讓「刪除」與「走訪」兩件事耦合在一起，
//     違反 STL 一貫的正交性。回傳新位置、由呼叫端決定怎麼用，
//     是更乾淨的介面。
//
// (C) C++20 的 std::erase_if 讓這整個 pattern 消失
//     C++20 起可以直接寫：
//         std::erase_if(v, [](int x){ return x % 2 == 0; });
//     一行，O(n)，而且完全不會寫錯迭代器。
//     這是標準函式庫「把容易寫錯的慣用法收編成演算法」的典型例子。
//     詳見本課第 11 檔。
//
// 【注意事項 Pay Attention】
//   1. erase 之後絕不能沿用舊迭代器。一律用回傳值。
//   2. 迴圈刪除時，for 的第三格要留空，遞增寫在 else 裡。
//   3. 用索引迴圈刪除時，刪掉之後【不要】遞增，否則會跳過元素。
//   4. 這個 pattern 是 O(n²)。批次刪除請改用 erase-remove 或 erase_if。
//   5. `it = c.erase(it)` 對 vector / list / map / set 都通用。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】erase 的回傳值與迴圈刪除
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 寫一個「刪除 vector 中所有偶數」的迴圈，並說明為什麼要這樣寫。
//     答：
//         for (auto it = v.begin(); it != v.end(); ) {
//             if (*it % 2 == 0) it = v.erase(it);
//             else              ++it;
//         }
//         關鍵有兩點：① for 的第三格必須留空，因為刪除的那一輪
//         erase 已經把 it 指到下一個元素了，再 ++ 就會跳過；
//         ② 必須接住 erase 的回傳值，因為原本的 it 已經失效。
//     追問：這樣寫的複雜度是多少？有更好的做法嗎？
//         → O(n²)，每次 erase 都搬移後半段。
//           批次刪除該用 erase-remove 慣用法（O(n)）：
//           v.erase(std::remove_if(v.begin(), v.end(), pred), v.end());
//           C++20 更簡單：std::erase_if(v, pred);
//
// 🔥 Q2. erase(first, last) 的回傳值指向哪裡？
//     答：指向「last 原本所指的那個元素」——因為刪除後，
//         原本在 last 位置的元素會往前補到 first 的位置。
//         等價的說法是「更新後的 first」。
//         特例：erase(v.begin(), v.end()) 回傳 end()（容器已空）。
//     追問：那 erase(pos) 呢？
//         → 指向被刪元素之後那個元素；刪最後一個時回傳 end()。
//
// ⚠️ 陷阱. 用索引迴圈刪除：
//         for (size_t i = 0; i < v.size(); ++i)
//             if (v[i] % 2 == 0) v.erase(v.begin() + i);
//         測 {1,2,3,4} 得到 {1,3}，看起來完全正確。為什麼它其實有 bug？
//     答：因為它會【跳過】元素。刪掉索引 i 之後，原本 i+1 的元素
//         補到了 i 的位置，但迴圈接著 ++i 跳到 i+1，
//         那個補位過來的元素就沒被檢查到。
//         {1,2,3,4} 剛好偶數不相鄰所以看不出來；
//         換成 {1,2,4,3} 就會得到 {1,4,3}——中間那個 4 逃過了檢查。
//         修法是刪除時不要遞增：
//             for (size_t i = 0; i < v.size(); )
//                 if (v[i] % 2 == 0) v.erase(v.begin() + i);
//                 else ++i;
//     為什麼會錯：測試資料剛好避開了觸發條件。
//         「連續兩個都該刪」是這個 bug 的唯一觸發條件，
//         而人工挑的小測資很容易不含這種情況。
//         本檔的 main 會用 {1,2,4,3} 把它實際印出來。
// ═══════════════════════════════════════════════════════════════════════════

#include <vector>
#include <iostream>
#include <string>

// -----------------------------------------------------------------------------
// 【日常實務範例】清理過期的 session
//   情境：Web 伺服器持有一份 session 清單，定期掃過去把過期的踢掉。
//         過期的 session 通常是【連續好幾筆】（同一批使用者同時登入，
//         也就同時過期），這正好會觸發「索引迴圈跳過元素」的 bug。
//   為什麼用到本主題：這是 `it = v.erase(it)` 慣用法最典型的實務場景。
//     這裡刻意把「正確版」與「會跳過的錯誤版」都寫出來對照，
//     讓那個 bug 在真實資料下現形。
//   效能提醒：若一次要清掉大量 session，這個 O(n²) 的寫法會很慢，
//     該改用 erase-remove（見第 10 檔）。這裡用它是為了示範迭代器語意。
// -----------------------------------------------------------------------------
struct Session {
    std::string id;
    int         expiresInSec;    // <= 0 表示已過期
};

// 正確版：用 erase 的回傳值續行
size_t purgeExpired(std::vector<Session>& sessions) {
    size_t removed = 0;
    for (auto it = sessions.begin(); it != sessions.end(); /* 這裡不遞增 */) {
        if (it->expiresInSec <= 0) {
            it = sessions.erase(it);    // erase 回傳下一個有效位置
            ++removed;
        } else {
            ++it;                        // 只有不刪除時才前進
        }
    }
    return removed;
}

// 錯誤版（僅供對照）：索引迴圈 + 無條件 ++i → 會跳過補位過來的元素
size_t purgeExpiredBuggy(std::vector<Session>& sessions) {
    size_t removed = 0;
    for (size_t i = 0; i < sessions.size(); ++i) {     // ← bug 在這個 ++i
        if (sessions[i].expiresInSec <= 0) {
            sessions.erase(sessions.begin() + static_cast<long>(i));
            ++removed;
        }
    }
    return removed;
}

int main() {
    std::vector<int> v = {10, 20, 30, 40, 50};

    std::cout << "=== 原始示範：erase 的回傳值 ===\n";
    std::cout << "初始: ";
    for (int x : v) std::cout << x << " ";
    std::cout << "\n";

    auto it = v.erase(v.begin() + 2);  // 刪除 30

    std::cout << "刪除後 it 指向: " << *it << std::endl;  // 40
    std::cout << "  （it 指向「被刪元素之後那個元素」）\n";
    std::cout << "  it 的索引: " << (it - v.begin()) << "\n";

    // 如果刪除的是最後一個元素，回傳 end()
    it = v.erase(v.end() - 1);  // 刪除 50
    if (it == v.end()) {
        std::cout << "it 現在是 end()" << std::endl;
        std::cout << "  （刪到結尾時回傳 end()，不能解參考）\n";
    }
    std::cout << "目前內容: ";
    for (int x : v) std::cout << x << " ";
    std::cout << "\n";

    std::cout << "\n=== erase(first, last) 的回傳值 ===\n";
    {
        std::vector<int> w = {1, 2, 3, 4, 5, 6, 7};
        auto r = w.erase(w.begin() + 2, w.begin() + 5);   // 刪掉 3,4,5
        std::cout << "刪除 [begin+2, begin+5) 後: ";
        for (int x : w) std::cout << x << " ";
        std::cout << "\n";
        std::cout << "回傳的 it 指向: " << *r
                  << "（原本 last 所指的元素，現在補到了 first 的位置）\n";
        std::cout << "it 的索引: " << (r - w.begin()) << "\n";

        auto e = w.erase(w.begin(), w.end());
        std::cout << "erase(begin, end) 後 size=" << w.size()
                  << "，回傳值 == end(): " << std::boolalpha << (e == w.end()) << "\n";
    }

    std::cout << "\n=== 正確的迴圈刪除慣用法 ===\n";
    {
        std::vector<int> nums = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        std::cout << "刪除前: ";
        for (int x : nums) std::cout << x << " ";
        std::cout << "\n";

        for (auto i = nums.begin(); i != nums.end(); /* 第三格留空 */) {
            if (*i % 2 == 0) i = nums.erase(i);   // 刪除 → 接回傳值
            else             ++i;                  // 不刪 → 自己前進
        }

        std::cout << "刪除偶數後: ";
        for (int x : nums) std::cout << x << " ";
        std::cout << "\n";
    }

    std::cout << "\n=== 陷阱實測：索引迴圈 + ++i 會跳過元素 ===\n";
    {
        // {1,2,3,4}：偶數不相鄰 → 看起來正確
        std::vector<int> a = {1, 2, 3, 4};
        for (size_t i = 0; i < a.size(); ++i) {
            if (a[i] % 2 == 0) a.erase(a.begin() + static_cast<long>(i));
        }
        std::cout << "{1,2,3,4} 用錯誤寫法 -> ";
        for (int x : a) std::cout << x << " ";
        std::cout << "  ← 看起來正確\n";

        // {1,2,4,3}：兩個偶數相鄰 → bug 現形
        std::vector<int> b = {1, 2, 4, 3};
        for (size_t i = 0; i < b.size(); ++i) {
            if (b[i] % 2 == 0) b.erase(b.begin() + static_cast<long>(i));
        }
        std::cout << "{1,2,4,3} 用錯誤寫法 -> ";
        for (int x : b) std::cout << x << " ";
        std::cout << "  ← 4 逃掉了！\n";

        // 正確的索引版：刪除時不遞增
        std::vector<int> c = {1, 2, 4, 3};
        for (size_t i = 0; i < c.size(); /* 不遞增 */) {
            if (c[i] % 2 == 0) c.erase(c.begin() + static_cast<long>(i));
            else ++i;
        }
        std::cout << "{1,2,4,3} 用正確寫法 -> ";
        for (int x : c) std::cout << x << " ";
        std::cout << "\n";
        std::cout << "→ 觸發條件是「連續兩個都該刪」，人工小測資很容易漏掉。\n";
    }

    std::cout << "\n=== 日常實務：清理過期的 session ===\n";
    {
        auto makeSessions = []() {
            return std::vector<Session>{
                {"sess-a1", 3600},
                {"sess-b2",    0},   // 過期
                {"sess-c3",   -5},   // 過期（與上一筆相鄰！）
                {"sess-d4",  120},
                {"sess-e5",   -1},   // 過期
                {"sess-f6",  900},
            };
        };

        auto good = makeSessions();
        size_t n1 = purgeExpired(good);
        std::cout << "正確版：清掉 " << n1 << " 筆，剩下 " << good.size() << " 筆：";
        for (const auto& s : good) std::cout << s.id << " ";
        std::cout << "\n";

        auto bad = makeSessions();
        size_t n2 = purgeExpiredBuggy(bad);
        std::cout << "錯誤版：清掉 " << n2 << " 筆，剩下 " << bad.size() << " 筆：";
        for (const auto& s : bad) std::cout << s.id << " ";
        std::cout << "\n";
        std::cout << "→ 錯誤版漏掉了 sess-c3：它補位到 b2 的索引，卻被 ++i 跳過。\n";
        std::cout << "  真實系統中「同一批同時過期」很常見，這個 bug 幾乎必然被觸發。\n";
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第 15 課：vector 元素刪除4.cpp -o demo4

// === 預期輸出 ===
// === 原始示範：erase 的回傳值 ===
// 初始: 10 20 30 40 50
// 刪除後 it 指向: 40
//   （it 指向「被刪元素之後那個元素」）
//   it 的索引: 2
// it 現在是 end()
//   （刪到結尾時回傳 end()，不能解參考）
// 目前內容: 10 20 40
//
// === erase(first, last) 的回傳值 ===
// 刪除 [begin+2, begin+5) 後: 1 2 6 7
// 回傳的 it 指向: 6（原本 last 所指的元素，現在補到了 first 的位置）
// it 的索引: 2
// erase(begin, end) 後 size=0，回傳值 == end(): true
//
// === 正確的迴圈刪除慣用法 ===
// 刪除前: 1 2 3 4 5 6 7 8 9 10
// 刪除偶數後: 1 3 5 7 9
//
// === 陷阱實測：索引迴圈 + ++i 會跳過元素 ===
// {1,2,3,4} 用錯誤寫法 -> 1 3   ← 看起來正確
// {1,2,4,3} 用錯誤寫法 -> 1 4 3   ← 4 逃掉了！
// {1,2,4,3} 用正確寫法 -> 1 3
// → 觸發條件是「連續兩個都該刪」，人工小測資很容易漏掉。
//
// === 日常實務：清理過期的 session ===
// 正確版：清掉 3 筆，剩下 3 筆：sess-a1 sess-d4 sess-f6
// 錯誤版：清掉 2 筆，剩下 4 筆：sess-a1 sess-c3 sess-d4 sess-f6
// → 錯誤版漏掉了 sess-c3：它補位到 b2 的索引，卻被 ++i 跳過。
//   真實系統中「同一批同時過期」很常見，這個 bug 幾乎必然被觸發。
