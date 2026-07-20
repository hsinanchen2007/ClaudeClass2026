// =============================================================================
//  第 17 課：vector 的記憶體重新配置機制 5  —  insert / erase 造成的「部分失效」
// =============================================================================
//
// 【主題資訊 Information】
//   iterator insert(const_iterator pos, const T& value);        // C++98（C++11 起 pos 為 const_iterator）
//   iterator erase (const_iterator pos);                        // C++98
//   iterator erase (const_iterator first, const_iterator last); // C++98
//
//   標頭檔  ：<vector>
//   回傳值  ：insert → 指向「新插入元素」的 iterator
//             erase  → 指向「被刪元素的下一個」的 iterator（刪到尾端則為 end()）
//   複雜度  ：兩者皆為 O(插入／刪除點到尾端的距離)，最壞 O(n)
//
//   失效規則（標準保證）：
//     insert：若造成重新配置 → 全部失效；否則 → 插入點（含）之後全部失效
//     erase ：不會重新配置 → 刪除點（含）之後全部失效；之前的仍有效
//     兩者的 end() 一律失效
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼「插入點之前」的 iterator 還活著】
//   在容量足夠、不需要重新配置的前提下，vector 的緩衝區位址完全沒變。
//   insert 做的事是「把插入點之後的元素整段往後搬一格，再把新值寫進空出來的位置」：
//
//       insert(begin()+1, 15)  對 {10, 20, 30, 40, 50}
//
//       索引:      0     1     2     3     4     5
//       之前:    [ 10 ][ 20 ][ 30 ][ 40 ][ 50 ][  ? ]
//                       └──────────────────────┘ 整段往後搬一格
//       之後:    [ 10 ][ 15 ][ 20 ][ 30 ][ 40 ][ 50 ]
//                  ▲     ▲
//                  │     └─ 這個位址的「內容」從 20 變成 15
//                  └─ 這個位址的內容沒被動過 ⇒ 指向它的 iterator 語意不變
//
//   所以「插入點之前」的 iterator 依然指著同一個元素，是有效的。
//   而「插入點之後」的 iterator 雖然位址還在合法範圍內（不會 crash），
//   但它現在指到的是「別的元素」——語意已經錯了。標準因此規定它們失效。
//
// 【2. 「失效」的兩種嚴重程度：懸空 vs 語意錯亂】
//   這是本課最容易混淆的地方，務必分清：
//     (a) 重新配置造成的失效（第 4 檔）：iterator 指向【已釋放的記憶體】。
//         解參考是 UB，且是很容易出事的那種。
//     (b) insert/erase 未重新配置造成的失效（本檔）：iterator 指向的位址
//         仍在合法緩衝區內，解參考通常不會崩潰，但讀到的是【錯誤的元素】。
//   從標準的角度看，兩者都是「失效」、都是 UB，不可使用。
//   但 (b) 更陰險——它幾乎不會當場出事，只會安靜地算出錯誤結果。
//   注意：erase 之後若原本指向的是「已被刪掉的最後一格」，那個位置的物件
//   已被解構，讀它同樣是 UB，不能因為「位址還在」就以為安全。
//
// 【3. 為什麼 erase 之後「刪除點之後」全部失效】
//   erase 的實作是「把後面的元素整段往前搬，再解構最後一個」。
//   同樣的道理：位址沒變，但每個位址上住的元素換人了。
//   還有一個常被忽略的細節：erase 之後 size() 減少，
//   原本指向最後一個元素的 iterator 現在會落在 [size, capacity) 那段
//   「已解構」的區域——那裡已經沒有物件了。
//
// 【4. 正確用法：接住回傳值】
//   insert 與 erase 都回傳一個「已經重新取得、保證有效」的 iterator。
//   這不是方便功能，而是標準為了讓你能在失效之後繼續操作而設計的唯一出口：
//       it = v.insert(it, 15);   // it 現在指向新插入的 15
//       it = v.erase(it);        // it 現在指向被刪元素的下一個
//   第 6 檔會示範不接住回傳值的後果，第 7 檔則是正確寫法。
//
// 【概念補充 Concept Deep Dive】
//   insert / erase 的搬移是用「移動指派（move assignment）」而非
//   「移動建構」完成的——因為目標位置上已經有活著的物件了。
//   這也是為什麼元素型別的 operator=(T&&) 同樣值得標記 noexcept。
//   在中間插入的成本是 O(n)：要搬移 (size - pos) 個元素。
//   若你的工作負載大量在「中間」插入或刪除，vector 就不是對的容器：
//     * 需要頻繁在中間插刪 → std::list / std::forward_list（O(1) 插刪，
//       但失去隨機存取與快取局部性，實測往往比想像中慢）
//     * 只在兩端插刪       → std::deque（頭尾皆 O(1)，且尾端插入不會使
//       既有元素的 reference 失效——但 iterator 仍會失效）
//     * 不在意順序         → 「與最後一個元素交換再 pop_back」可把
//       單筆刪除降到 O(1)（本檔第四段示範）
//
// 【注意事項 Pay Attention】
//   1. 使用失效的 iterator 是【UB】，不論它「看起來」還能不能讀。
//      本檔只用註解標示危險位置，不示範去解參考它。
//   2. reserve 只能排除「重新配置」這一種失效，
//      【不能】排除 insert/erase 造成的搬移失效。這是最常見的誤解。
//   3. erase 不會縮小 capacity，也不會歸還記憶體（見第 10 檔的 shrink_to_fit）。
//   4. erase(first, last) 傳入的區間必須是同一個 vector 的有效區間，
//      且 first <= last；否則是 UB。
//   5. 在 range-based for 迴圈（for (auto& x : v)）中插入或刪除元素同樣是
//      UB——它內部就是用 begin()/end() 的 iterator。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】insert / erase 的 iterator 失效
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 我已經 reserve 了足夠容量，是不是就不用擔心 iterator 失效？
//     答：不對。reserve 只能避免「重新配置」造成的失效。
//         insert/erase 會把插入或刪除點之後的元素整段搬移，
//         那些位置上的元素換人了，指向它們的 iterator 一樣失效。
//         只有「插入／刪除點之前」的 iterator 才仍然有效。
//     追問：那 end() 呢？→ 任何改變 size 的操作之後 end() 都失效。
//
// 🔥 Q2. insert 和 erase 的回傳值是什麼？為什麼標準要設計這個回傳值？
//     答：insert 回傳指向「新插入元素」的 iterator；
//         erase 回傳指向「被刪元素的下一個」的 iterator（刪到尾則為 end()）。
//         因為操作後舊 iterator 已失效，這個回傳值是標準提供的、
//         唯一保證有效的接續點——迴圈中刪除元素必須靠它。
//     追問：erase 刪除最後一個元素時回傳什麼？→ end()。
//         所以 `it = v.erase(it);` 之後一定要再檢查 it != v.end()。
//
// ⚠️ 陷阱. 「erase 之後那個 iterator 指的位址還在緩衝區內，讀出來也沒 crash，
//        所以應該可以用」——錯在哪？
//     答：那是【UB】。位址合法不代表語意正確：後面的元素已經整段前移，
//         你讀到的是「別的元素」；若原本指向最後一格，那裡的物件更是
//         已經被解構了。不崩潰只是運氣，不是保證。
//     為什麼會錯：把「失效」理解成「會 crash」。標準說的失效是
//         「不再保證任何意義」，靜靜給你錯誤答案正是它最常見的表現。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

static void print_vec(const std::string& label, const std::vector<int>& v) {
    std::cout << label;
    for (int x : v) std::cout << x << " ";
    std::cout << std::endl;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】使用者權限清單的增刪：用「索引」與「回傳值」避開失效
//   情境：後台管理頁面對一份權限清單做「在指定位置插入」與「移除某項」。
//         直覺寫法是先找到 iterator、放著、之後再用——那正是失效的來源。
//   做法：需要「位置」時用索引；需要「接著操作」時接住 insert/erase 的回傳值。
// -----------------------------------------------------------------------------
struct Permission {
    std::string name;
    bool        enabled;
};

// 在 name 相同的項目之前插入一個新權限；回傳新元素的索引（找不到則附加到尾端）
size_t insert_before(std::vector<Permission>& perms,
                     const std::string& anchor,
                     const Permission& p) {
    auto it = std::find_if(perms.begin(), perms.end(),
                           [&](const Permission& x) { return x.name == anchor; });
    // 接住 insert 的回傳值 —— 這是操作後唯一保證有效的 iterator
    auto inserted = perms.insert(it, p);
    return static_cast<size_t>(inserted - perms.begin());
}

// 移除所有 enabled == false 的項目，回傳移除筆數（用 erase 的回傳值走訪）
size_t drop_disabled(std::vector<Permission>& perms) {
    size_t removed = 0;
    for (auto it = perms.begin(); it != perms.end(); ) {
        if (!it->enabled) {
            it = perms.erase(it);   // 接住回傳值，取得下一個有效位置
            ++removed;
        } else {
            ++it;
        }
    }
    return removed;
}

int main() {
    std::cout << "=== 一、insert：插入點之前的 iterator 仍有效 ===" << std::endl;
    {
        std::vector<int> v = {10, 20, 30, 40, 50};
        v.reserve(100);   // 預留足夠空間，排除「重新配置」這個變因
        const int* base = v.data();

        auto it_10 = v.begin();      // 指向 10（插入點之前）
        // auto it_30 = v.begin() + 2;  // 指向 30（插入點之後）→ insert 後會失效

        print_vec("插入前： ", v);
        auto inserted = v.insert(v.begin() + 1, 15);   // 在索引 1 插入 15
        print_vec("插入後： ", v);

        std::cout << "緩衝區位址是否改變 = " << (base != v.data() ? "是" : "否")
                  << "（reserve 過，未重新配置）" << std::endl;
        std::cout << "*it_10 = " << *it_10 << "（插入點之前，仍有效）" << std::endl;
        std::cout << "insert 回傳的 iterator 指向 = " << *inserted
                  << "（永遠指向新插入的元素）" << std::endl;

        // ⚠️ 不可以在這裡讀 it_30：它現在指到的是「別的元素」，
        //    位址雖仍在緩衝區內，但語意已錯，標準規定它失效 ⇒ UB。
        std::cout << "（it_30 這類插入點之後的 iterator 已失效，本檔刻意不去讀它）"
                  << std::endl;
    }

    std::cout << "\n=== 二、erase：刪除點之前的 iterator 仍有效 ===" << std::endl;
    {
        std::vector<int> v = {10, 20, 30, 40, 50};
        v.reserve(100);

        auto it_10 = v.begin();      // 指向 10（刪除點之前）
        // auto it_40 = v.begin() + 3;  // 指向 40（刪除點之後）→ erase 後會失效

        print_vec("刪除前： ", v);
        auto next = v.erase(v.begin() + 1);   // 刪除索引 1 的 20
        print_vec("刪除後： ", v);

        std::cout << "*it_10 = " << *it_10 << "（刪除點之前，仍有效）" << std::endl;
        std::cout << "erase 回傳的 iterator 指向 = " << *next
                  << "（被刪元素的下一個）" << std::endl;
        std::cout << "（it_40 這類刪除點之後的 iterator 已失效，本檔刻意不去讀它）"
                  << std::endl;
    }

    std::cout << "\n=== 三、erase 刪到最後一個元素時，回傳的是 end() ===" << std::endl;
    {
        std::vector<int> v = {1, 2, 3};
        auto last = v.erase(v.begin() + 2);       // 刪掉最後一個
        std::cout << "刪除最後一個元素後，回傳值 == end() ? "
                  << (last == v.end() ? "是" : "否")
                  << " ⇒ 迴圈中務必先檢查 it != v.end() 才能解參考" << std::endl;
        print_vec("目前內容： ", v);
    }

    std::cout << "\n=== 四、不在意順序時：swap + pop_back 把單筆刪除降到 O(1) ===" << std::endl;
    {
        std::vector<int> v = {10, 20, 30, 40, 50};
        print_vec("刪除前： ", v);
        size_t idx = 1;                       // 想刪掉索引 1（值 20）
        std::swap(v[idx], v.back());          // 與最後一個交換
        v.pop_back();                         // 尾端彈出：O(1)，不搬移任何元素
        print_vec("刪除索引 1 後（順序被打亂，但只花 O(1)）： ", v);
        std::cout << "適用時機：元素順序不重要（例如遊戲場上的實體清單）。"
                  << std::endl;
    }

    std::cout << "\n=== 五、日常實務：權限清單的插入與批次移除 ===" << std::endl;
    {
        std::vector<Permission> perms = {
            {"read",   true},
            {"write",  true},
            {"delete", false},
            {"admin",  false},
            {"audit",  true}
        };

        std::cout << "原始清單： ";
        for (const auto& p : perms)
            std::cout << p.name << (p.enabled ? "(on) " : "(off) ");
        std::cout << std::endl;

        size_t pos = insert_before(perms, "delete", {"export", true});
        std::cout << "在 delete 之前插入 export，新元素索引 = " << pos << std::endl;

        size_t n = drop_disabled(perms);
        std::cout << "移除 " << n << " 筆停用的權限後： ";
        for (const auto& p : perms)
            std::cout << p.name << (p.enabled ? "(on) " : "(off) ");
        std::cout << std::endl;
        std::cout << "重點：全程沒有保存任何跨越 insert/erase 的 iterator——"
                  << "需要位置就用索引，需要接續就用回傳值。" << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 17 課：vector 的記憶體重新配置機制5.cpp" -o insert_erase_invalidate

// === 預期輸出 ===
// === 一、insert：插入點之前的 iterator 仍有效 ===
// 插入前： 10 20 30 40 50
// 插入後： 10 15 20 30 40 50
// 緩衝區位址是否改變 = 否（reserve 過，未重新配置）
// *it_10 = 10（插入點之前，仍有效）
// insert 回傳的 iterator 指向 = 15（永遠指向新插入的元素）
// （it_30 這類插入點之後的 iterator 已失效，本檔刻意不去讀它）
//
// === 二、erase：刪除點之前的 iterator 仍有效 ===
// 刪除前： 10 20 30 40 50
// 刪除後： 10 30 40 50
// *it_10 = 10（刪除點之前，仍有效）
// erase 回傳的 iterator 指向 = 30（被刪元素的下一個）
// （it_40 這類刪除點之後的 iterator 已失效，本檔刻意不去讀它）
//
// === 三、erase 刪到最後一個元素時，回傳的是 end() ===
// 刪除最後一個元素後，回傳值 == end() ? 是 ⇒ 迴圈中務必先檢查 it != v.end() 才能解參考
// 目前內容： 1 2
//
// === 四、不在意順序時：swap + pop_back 把單筆刪除降到 O(1) ===
// 刪除前： 10 20 30 40 50
// 刪除索引 1 後（順序被打亂，但只花 O(1)）： 10 50 30 40
// 適用時機：元素順序不重要（例如遊戲場上的實體清單）。
//
// === 五、日常實務：權限清單的插入與批次移除 ===
// 原始清單： read(on) write(on) delete(off) admin(off) audit(on)
// 在 delete 之前插入 export，新元素索引 = 2
// 移除 2 筆停用的權限後： read(on) write(on) export(on) audit(on)
// 重點：全程沒有保存任何跨越 insert/erase 的 iterator——需要位置就用索引，需要接續就用回傳值。
