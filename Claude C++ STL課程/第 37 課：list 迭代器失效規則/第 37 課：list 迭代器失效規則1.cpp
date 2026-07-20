// =============================================================================
//  第 37 課：list 迭代器失效規則 1  —  為什麼 list 的迭代器幾乎不會壞
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<list>
//   標準版本：std::list 為 C++98；本檔用到 C++11 的 std::next / std::prev、
//             initializer_list 版 insert，以及 C++11 原始字串字面值 R"(...)"
//
//   std::list 的失效規則（標準明文規定，[list.modifiers] / [list.ops]）：
//     ┌────────────────────┬──────────────────────────────────────────────┐
//     │ 操作               │ 對 iterator / pointer / reference 的影響     │
//     ├────────────────────┼──────────────────────────────────────────────┤
//     │ insert / emplace   │ 完全不失效                                   │
//     │ push_front/back    │ 完全不失效                                   │
//     │ erase / pop_*      │ 只有「被刪除的那些元素」失效，其餘全有效     │
//     │ splice             │ 完全不失效（節點換了隸屬的 list）            │
//     │ merge              │ 完全不失效                                   │
//     │ sort               │ 完全不失效（只重接指標，元素位址不變）       │
//     │ reverse            │ 完全不失效                                   │
//     │ remove/remove_if   │ 只有被移除的元素失效                         │
//     │ unique             │ 只有被移除的元素失效                         │
//     │ clear              │ 全部失效（end() 除外）                       │
//     │ resize（縮小）     │ 被截掉的元素失效                             │
//     └────────────────────┴──────────────────────────────────────────────┘
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼 list 的迭代器這麼穩：因為它就是「節點指標」】
//   list 的迭代器內部只包一個指向節點的指標。節點是獨立配置在堆積上的，
//   它的位址在被 delete 之前永遠不會改變。
//   所有「重排」類的操作（sort、reverse、splice、merge）做的都是
//   「改寫節點裡的 prev / next 欄位」——節點本身不動、不搬、不重配。
//   所以只要那個節點還活著，指向它的迭代器就永遠有效。
//   唯一會讓迭代器失效的事情只有一件：**那個節點被銷毀了**。
//   這就是為什麼上表看起來這麼寬鬆——不是標準特別優待 list，
//   而是它的資料結構本來就沒有「搬移元素」這回事。
//
// 【2. 對照 vector：為什麼它那麼容易失效】
//   vector 的迭代器本質上是「指向連續緩衝區中某個位置的指標」。
//   它的有效性同時綁在兩件事上：緩衝區的位址，以及元素在緩衝區中的位置。
//     - 擴容 → 整塊緩衝區換位置 → 全部迭代器失效。
//     - 中間 erase → 後面的元素往前搬 → 該位置之後的迭代器全部失效。
//     - 中間 insert（未擴容）→ 後面的元素往後搬 → 同樣失效。
//   所以 vector 的規則是「大多數修改都會讓一部分或全部迭代器失效」，
//   跟 list 剛好相反。**這兩套規則必須分開記，不可互相套用。**
//
// 【3. sort 後迭代器仍指向「原來那個元素」——不是「原來那個位置」】
//   這是最容易搞混的一點。lst.sort() 之後：
//     - 迭代器仍然有效；
//     - 它指向的還是**原本那個元素**（值不變、位址不變）；
//     - 但那個元素在串列中的**位置**變了。
//   對照 vector：v 排序後，同一個 index 的位置換成了別的值——
//   因為 vector 是把「值」搬來搬去。list 是把「節點」重新接線。
//   本檔第 4 段用位址證明這件事：sort 前後 &(*it) 完全相同。
//
// 【4. reverse 的陷阱：迭代器有效，但語意變了】
//   lst.reverse() 之後，原本指向第一個元素的迭代器仍然有效，
//   但那個元素現在跑到最後面去了。所以：
//       it_begin == lst.begin()      → 不再成立
//       it_begin == prev(lst.end())  → 現在成立
//   「迭代器有效」只保證「可以安全解參考」，
//   不保證「它跟容器的相對關係沒變」。這兩件事必須分開理解。
//
// 【5. splice 之後：節點換了主人】
//   A.splice(A.end(), B, it) 把 it 指向的節點從 B 搬到 A。
//   it 仍然有效，*it 仍然是原來的值——但它現在隸屬於 A。
//   因此 ++it 會走到 **A 裡面** 該節點的下一個元素，不是 B 的下一個元素。
//   如果你原本打算用 it 繼續遍歷 B，那個意圖已經斷了。
//   這不是「失效」，而是「有效但語意改變」，比失效更難察覺。
//
// 【概念補充 Concept Deep Dive】
//   ● 為什麼 erase 一定要接住回傳值：
//     erase(it) 會 delete 那個節點，it 立刻變成懸空指標（dangling）。
//     此時 ++it 是讀取已釋放記憶體 → 未定義行為。
//     C++11 起 list::erase 回傳「下一個元素的迭代器」，正是為了讓你寫
//         it = lst.erase(it);
//     這個慣用法。所有序列容器的 erase 都遵循同一個約定。
//   ● 為什麼 end() 通常不失效：
//     libstdc++ 的 list 是環狀串列，end() 指向一個不存資料的哨兵節點。
//     哨兵在 list 物件的生命期內一直存在，所以 end() 在增刪元素後仍有效。
//     （clear() 之後 end() 也還在——被銷毀的是資料節點，不是哨兵。）
//   ● 「有效」與「可解參考」是兩件事：
//     end() 是有效的迭代器（可以比較、可以是範圍的邊界），
//     但解參考 end() 是未定義行為。標準用 valid 與 dereferenceable
//     兩個不同的詞來描述，讀規格時要分清楚。
//   ● 迭代器失效的偵測工具：
//     libstdc++ 提供 -D_GLIBCXX_DEBUG，會把容器換成帶檢查的除錯版本，
//     在執行期抓出「使用已失效迭代器」並直接中止並印出診斷訊息。
//     開發階段值得開；它會顯著拖慢程式，不要用在正式編譯。
//     另有 AddressSanitizer（-fsanitize=address）可抓 use-after-free。
//
// 【注意事項 Pay Attention】
//   1. 使用已失效的迭代器是**未定義行為**。它可能看起來「還能跑」、
//      可能印出舊值、可能崩潰——標準不保證任何特定結果，
//      也不可以用「我試過沒事」當作它安全的證據。
//   2. list 的 insert 不失效、erase 只讓被刪的失效——
//      這是 list 的規則，**不要套用到 vector / deque**。
//   3. sort / reverse 之後迭代器有效，但它與容器的相對位置可能已改變。
//   4. splice 之後迭代器仍有效，但已隸屬於目的地 list。
//   5. 迴圈中刪除元素，一律用 `it = lst.erase(it);` 的形式。
//   6. 本檔第 4 段會印出記憶體位址，每次執行都不同；
//      重點在「sort 前後位址相同」這個關係，不在數值本身。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::list 迭代器失效規則
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 list 的 sort() 之後迭代器還有效，vector 排序後卻「像是」變了？
//     答：list 的迭代器是節點指標，sort 只重接 prev/next，節點位址完全不變，
//         所以迭代器仍指向**原本那個元素**（值與位址都不變），只是它在串列中的
//         位置改變了。vector 的迭代器是緩衝區中的位置，排序是把「值」搬來搬去，
//         所以同一個位置換成了別的值——迭代器沒失效，但指到的內容不同了。
//     追問：那 vector 排序後迭代器算失效嗎？→ 不算，std::sort 不會使 vector
//         迭代器失效（沒有重新配置），只是它指向的值被換掉了。這兩件事要分清。
//
// 🔥 Q2. 在迴圈中刪除 list 元素，正確寫法是什麼？為什麼常見寫法是錯的？
//     答：正確寫法是
//             for (auto it = lst.begin(); it != lst.end(); ) {
//                 if (cond(*it)) it = lst.erase(it);
//                 else ++it;
//             }
//         錯的寫法是在 for 的第三段寫 ++it 又在迴圈裡 erase(it)——
//         erase 之後 it 指向已釋放的節點，接著 ++it 是讀取已釋放記憶體，
//         屬於未定義行為。erase 回傳下一個有效迭代器，就是為了解決這件事。
//     追問：有沒有更簡潔的寫法？→ C++20 起可用 std::erase_if(lst, pred)；
//         C++20 之前 list 也有成員函式 lst.remove_if(pred)。
//
// 🔥 Q3. list 的哪些操作會使迭代器失效？
//     答：只有「銷毀節點」的操作：erase、pop_front、pop_back、clear、
//         縮小的 resize、remove/remove_if、unique——而且只有被移除的那些元素失效。
//         insert、splice、merge、sort、reverse 一律不失效，因為它們不銷毀節點。
//     追問：為什麼 splice 不失效？→ 節點只是從一條串列摘下接到另一條，
//         沒有被 delete 也沒有被重新配置，所以指向它的迭代器繼續有效。
//
// ⚠️ 陷阱. 「splice 之後 it 還有效，所以我可以繼續用 it 遍歷原本的 B。」
//     答：不行。it 確實有效，但那個節點已經隸屬於 A 了。
//         ++it 會走到 **A 裡面**該節點的下一個元素，不會回到 B。
//         這是「有效但語意改變」，比單純失效更難察覺。
//     為什麼會錯：把「迭代器有效」理解成「它跟原容器的關係沒變」。
//         有效只保證能安全解參考，不保證所屬容器與相對位置不變。
//
// ⚠️ 陷阱. 「我用失效的迭代器測過，印出來是對的，所以應該沒問題。」
//     答：那是未定義行為。節點已被釋放，記憶體可能還沒被覆寫，所以「看起來正常」。
//         這種程式可能在換編譯器、換最佳化等級、換配置器、或程式跑久一點之後
//         才爆炸。用 -D_GLIBCXX_DEBUG 或 AddressSanitizer 才能可靠抓出來。
//     為什麼會錯：把「這次觀察到的結果」當成語言保證。
//         UB 最危險的地方就是它經常「暫時看起來是對的」。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <list>
#include <vector>
#include <string>
#include <map>
#include <unordered_map>
using namespace std;

template <typename T>
void print_list(const string& label, const list<T>& lst) {
    cout << label << " [" << lst.size() << "]: ";
    for (const auto& val : lst) cout << val << " ";
    cout << endl;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 146. LRU Cache
//   題目：設計一個 LRU 快取，get 與 put 都要 O(1)。
//   為什麼用到本主題：這題之所以能做到 O(1)，關鍵就是本課的失效規則——
//     hash map 裡存的是 list 的迭代器；因為 list 的 insert/splice/erase(其他元素)
//     都不會使既有迭代器失效，這些存起來的迭代器才能長期安全使用。
//     若改用 vector，任何一次擴容就會讓 map 中所有迭代器變成懸空指標，
//     這個設計立刻崩潰。這是「迭代器穩定性」在面試題中最經典的應用。
// -----------------------------------------------------------------------------
class LRUCache {
public:
    explicit LRUCache(int capacity) : cap_(static_cast<size_t>(capacity)) {}

    int get(int key) {
        auto found = index_.find(key);
        if (found == index_.end()) return -1;
        // splice 把該節點搬到最前面：O(1)，且不使任何迭代器失效
        order_.splice(order_.begin(), order_, found->second);
        return found->second->second;
    }

    void put(int key, int value) {
        auto found = index_.find(key);
        if (found != index_.end()) {
            found->second->second = value;
            order_.splice(order_.begin(), order_, found->second);
            return;
        }
        if (order_.size() >= cap_) {
            index_.erase(order_.back().first);   // 淘汰最久未使用者
            order_.pop_back();
        }
        order_.emplace_front(key, value);
        index_[key] = order_.begin();
    }

private:
    size_t cap_;
    list<pair<int, int>> order_;                                  // 前=最近使用
    unordered_map<int, list<pair<int, int>>::iterator> index_;    // key → 節點迭代器
};

// -----------------------------------------------------------------------------
// 【日常實務範例】編輯器的多游標／書籤系統
//   情境：程式編輯器讓使用者在多行上放置書籤或多重游標。
//         使用者接著會在別的地方插入行、刪除行、甚至排序整個檔案。
//   為什麼用 list：書籤就是「指向某一行的迭代器」。
//         list 的 insert / erase(別行) / sort 都不使既有迭代器失效，
//         所以書籤在這些編輯操作之後仍然指向使用者當初標記的那一行。
//         若用 vector<string>，任何一次插入導致擴容就會讓所有書籤懸空，
//         必須改存索引再逐一修正——那是額外的複雜度與 bug 溫床。
//   注意：唯一要處理的情況是「使用者刪掉了被書籤標記的那一行」，
//         此時該書籤（且僅該書籤）失效，必須主動移除。
// -----------------------------------------------------------------------------
class DocumentBookmarks {
public:
    using LineIt = list<string>::iterator;

    explicit DocumentBookmarks(list<string>& doc) : doc_(doc) {}

    void addBookmark(const string& name, LineIt where) { marks_[name] = where; }

    // 刪除某一行：必須先清掉指向它的書籤（這是唯一會失效的情況）
    void eraseLine(LineIt where) {
        for (auto it = marks_.begin(); it != marks_.end(); ) {
            if (it->second == where) it = marks_.erase(it);   // 書籤失效 → 移除
            else ++it;
        }
        doc_.erase(where);
    }

    void dump() const {
        for (const auto& kv : marks_)
            cout << "    書籤 " << kv.first << " → " << *kv.second << endl;
    }

private:
    list<string>& doc_;
    // 這裡故意用 map 保證輸出順序穩定，方便對照預期輸出
    map<string, LineIt> marks_;
};

int main() {
    // ===== 1. insert 後迭代器全部有效 =====
    cout << "===== 1. insert 後迭代器有效 =====" << endl;
    {
        list<int> lst = {10, 20, 30, 40, 50};

        // 保存所有元素的迭代器
        auto it10 = lst.begin();
        auto it20 = next(it10);
        auto it30 = next(it20);
        auto it40 = next(it30);
        auto it50 = next(it40);

        // 大量插入
        lst.insert(it30, 25);         // 在 30 前面
        lst.push_back(60);            // 尾端
        lst.push_front(5);            // 頭端
        lst.insert(it50, {45, 48});   // 在 50 前面

        // 驗證所有舊迭代器仍然有效（標準保證：insert 完全不失效）
        cout << "*it10=" << *it10 << " *it20=" << *it20
             << " *it30=" << *it30 << " *it40=" << *it40
             << " *it50=" << *it50 << endl;

        print_list("完整 list", lst);
        cout << "→ 所有舊迭代器在插入後仍然有效！" << endl;
    }

    // ===== 2. erase 只影響被刪的迭代器 =====
    cout << "\n===== 2. erase 只影響被刪的 =====" << endl;
    {
        list<int> lst = {10, 20, 30, 40, 50};

        auto it10 = lst.begin();
        auto it20 = next(it10);
        auto it30 = next(it20);
        auto it40 = next(it30);
        auto it50 = next(it40);

        // 刪除 30
        lst.erase(it30);
        // it30 已失效！解參考它是未定義行為，以下刻意不碰它。
        // 但其他迭代器全部有效：
        cout << "*it10=" << *it10 << " *it20=" << *it20
             << " *it40=" << *it40 << " *it50=" << *it50 << endl;

        print_list("刪除 30 後", lst);

        // 對比 vector 的行為
        cout << "\n--- 對比 vector ---" << endl;
        vector<int> vec = {10, 20, 30, 40, 50};
        auto vit40 = vec.begin() + 3;  // 指向 40
        cout << "erase 前 *vit40 = " << *vit40 << endl;
        vec.erase(vec.begin() + 2);    // 刪除 30 → 後面元素往前搬
        // 此刻 vit40 已失效（erase 使刪除點之後的迭代器全部失效），
        // 解參考它是未定義行為。下面改用「索引」讀取，這是完全合法的：
        // erase 之後 vec 剩 4 個元素，vec[3] 在範圍內。
        cout << "erase 後 vec[3] = " << vec[3]
             << "（vit40 已失效，故改用索引讀取，不解參考 vit40）" << endl;
        cout << "→ 同樣是 erase，vector 讓刪除點之後的迭代器全部失效，"
             << "list 只讓被刪的那一個失效" << endl;
    }

    // ===== 3. splice 後所有迭代器有效 =====
    cout << "\n===== 3. splice 後迭代器有效 =====" << endl;
    {
        list<int> A = {1, 2, 3};
        list<int> B = {10, 20, 30};

        auto it_1 = A.begin();
        auto it_20 = next(B.begin());

        cout << "splice 前: *it_1=" << *it_1 << " *it_20=" << *it_20 << endl;

        // 把 B 的 20 移到 A
        A.splice(A.end(), B, it_20);

        cout << "splice 後: *it_1=" << *it_1 << " *it_20=" << *it_20 << endl;
        print_list("A", A);
        print_list("B", B);
        cout << "→ it_20 仍有效，但現在屬於 A（++it_20 會走 A 的元素，不是 B 的）"
             << endl;
    }

    // ===== 4. sort 後迭代器有效 =====
    cout << "\n===== 4. sort 後迭代器有效 =====" << endl;
    {
        list<int> lst = {50, 30, 10, 40, 20};

        auto it_30 = next(lst.begin());    // 指向 30
        auto it_40 = next(it_30, 2);       // 指向 40

        // 注意：以下印出的位址每次執行都不同；
        //       重點在「sort 前後的位址完全相同」這個關係。
        cout << "sort 前: *it_30=" << *it_30
             << " addr=" << &(*it_30) << endl;
        cout << "sort 前: *it_40=" << *it_40
             << " addr=" << &(*it_40) << endl;

        lst.sort();

        cout << "sort 後: *it_30=" << *it_30
             << " addr=" << &(*it_30) << endl;
        cout << "sort 後: *it_40=" << *it_40
             << " addr=" << &(*it_40) << endl;

        print_list("排序結果", lst);
        cout << "→ 值和地址都不變！只是在 list 中的位置改變了" << endl;
    }

    // ===== 5. remove 只使被刪節點的迭代器失效 =====
    cout << "\n===== 5. remove 的迭代器影響 =====" << endl;
    {
        list<int> lst = {1, 2, 3, 2, 4, 2, 5};

        auto it_1 = lst.begin();
        auto it_3 = next(it_1, 2);
        auto it_4 = next(it_3, 2);
        auto it_5 = next(it_4, 2);

        cout << "remove 前: " << *it_1 << " " << *it_3
             << " " << *it_4 << " " << *it_5 << endl;

        lst.remove(2);   // 刪除所有 2 → 只有那些節點的迭代器失效

        // 所有非 2 的迭代器仍然有效
        cout << "remove 後: " << *it_1 << " " << *it_3
             << " " << *it_4 << " " << *it_5 << endl;

        print_list("結果", lst);
    }

    // ===== 6. reverse 後迭代器有效（但相對位置變了） =====
    cout << "\n===== 6. reverse 後迭代器有效 =====" << endl;
    {
        list<int> lst = {10, 20, 30, 40, 50};

        auto it_begin = lst.begin();        // 指向 10
        auto it_second = next(it_begin);    // 指向 20

        cout << "reverse 前: begin→" << *it_begin
             << " second→" << *it_second << endl;

        lst.reverse();

        cout << "reverse 後: *it_begin=" << *it_begin
             << " *it_second=" << *it_second << endl;
        cout << "lst.front()=" << lst.front()
             << " lst.back()=" << lst.back() << endl;

        // 注意：it_begin 仍指向 10，但 10 現在是最後一個元素
        // 所以 it_begin != lst.begin() 了！
        // 這就是「迭代器有效 ≠ 它與容器的相對關係沒變」
        cout << "it_begin == lst.begin()? "
             << (it_begin == lst.begin() ? "是" : "否") << endl;
        cout << "it_begin == prev(lst.end())? "
             << (it_begin == prev(lst.end()) ? "是" : "否") << endl;
    }

    // ===== 7. 典型錯誤模式 vs 正確模式 =====
    cout << "\n===== 7. 常見錯誤 vs 正確寫法 =====" << endl;
    {
        // 錯誤模式 1：erase 後繼續使用失效迭代器
        cout << "--- 錯誤模式 1：erase 後用失效迭代器 ---" << endl;
        cout << R"(
  // ❌ 錯誤！
  auto it = lst.begin();
  lst.erase(it);
  ++it;           // 未定義行為！it 指向已釋放的節點

  // ✔ 正確
  auto it = lst.begin();
  it = lst.erase(it);   // erase 回傳下一個有效迭代器
)" << endl;

        // 錯誤模式 2：迴圈中 erase + ++it
        cout << "--- 錯誤模式 2：迴圈中 erase + ++it ---" << endl;
        cout << R"(
  // ❌ 錯誤！
  for (auto it = lst.begin(); it != lst.end(); ++it) {
      if (*it == target) {
          lst.erase(it);   // it 失效
      }                    // 下一次 ++it → 未定義行為！
  }

  // ✔ 正確
  for (auto it = lst.begin(); it != lst.end(); ) {
      if (*it == target) {
          it = lst.erase(it);   // erase 回傳下一個
      } else {
          ++it;
      }
  }

  // ✔ 更簡潔：C++20 起
  std::erase(lst, target);       // 或 std::erase_if(lst, pred)
  // C++20 之前：lst.remove(target);  /  lst.remove_if(pred);
)" << endl;

        // 錯誤模式 3：splice 後用迭代器在舊容器上遍歷
        cout << "--- 錯誤模式 3：splice 後用舊容器遍歷 ---" << endl;
        cout << R"(
  list<int> A = {1, 2, 3};
  list<int> B = {10, 20};
  auto it = B.begin();    // 指向 B 的 10

  A.splice(A.end(), B, it);  // 10 移到了 A

  // ❌ 概念錯誤（雖然 *it 仍然有效）：
  // 不能期望從 it 遍歷能走到 B 的其他元素
  // 因為 it 現在屬於 A 了

  // ✔ 正確認知：it 指向的節點現在在 A 中
  // ++it 會走到 A 中 10 之後的元素，不是 B 的元素
)" << endl;
    }

    // ===== 8. 實戰：安全地持有多個迭代器 =====
    cout << "===== 8. 實戰：多迭代器書籤系統 =====" << endl;
    {
        list<string> document = {
            "第一章", "第二章", "第三章",
            "第四章", "第五章", "第六章"
        };

        // 用迭代器做「書籤」
        auto bookmark1 = document.begin();
        advance(bookmark1, 1);     // 書籤1 → 第二章
        auto bookmark2 = document.begin();
        advance(bookmark2, 4);     // 書籤2 → 第五章

        cout << "書籤1: " << *bookmark1 << endl;
        cout << "書籤2: " << *bookmark2 << endl;

        // 在第三章前插入新章節 → 書籤不受影響（insert 不失效）
        auto it3 = document.begin();
        advance(it3, 2);
        document.insert(it3, "新增章節A");

        // 刪除第四章 → 書籤不受影響（書籤沒指向第四章）
        auto it4 = document.begin();
        advance(it4, 3);   // 原本的第三章（因為插入了一個）
        advance(it4, 1);   // 第四章
        document.erase(it4);

        // 排序 → 書籤仍有效（sort 只重接指標）
        document.sort();

        // 書籤仍然指向原來的值
        cout << "操作後書籤1: " << *bookmark1 << endl;
        cout << "操作後書籤2: " << *bookmark2 << endl;

        print_list("最終文件", document);
        cout << "→ 經過 insert、erase、sort 後，書籤始終有效！" << endl;
        cout << "（若換成 vector，上面任何一個操作都可能讓書籤懸空）" << endl;
    }

    // ===== 9. LeetCode 146. LRU Cache =====
    cout << "\n===== 9. LeetCode 146. LRU Cache =====" << endl;
    {
        LRUCache cache(2);
        cache.put(1, 1);
        cache.put(2, 2);
        cout << "get(1) = " << cache.get(1) << "   （預期 1）" << endl;
        cache.put(3, 3);                       // 容量滿 → 淘汰 key 2
        cout << "get(2) = " << cache.get(2) << "  （預期 -1，已被淘汰）" << endl;
        cache.put(4, 4);                       // 淘汰 key 1
        cout << "get(1) = " << cache.get(1) << "  （預期 -1，已被淘汰）" << endl;
        cout << "get(3) = " << cache.get(3) << "   （預期 3）" << endl;
        cout << "get(4) = " << cache.get(4) << "   （預期 4）" << endl;
        cout << "→ 全靠 list 迭代器不失效，才能把迭代器存進 hash map" << endl;
    }

    // ===== 10. 日常實務：編輯器書籤（含「被刪行」的處理） =====
    cout << "\n===== 10. 日常實務：編輯器書籤 =====" << endl;
    {
        list<string> doc = {"line-1", "line-2", "line-3", "line-4"};
        DocumentBookmarks bm(doc);

        auto l2 = next(doc.begin());        // line-2
        auto l4 = next(doc.begin(), 3);     // line-4
        bm.addBookmark("A", l2);
        bm.addBookmark("B", l4);
        cout << "  初始書籤：" << endl;
        bm.dump();

        // 在最前面插入一行 → 書籤完全不受影響
        doc.push_front("line-0");
        cout << "  插入 line-0 之後：" << endl;
        bm.dump();

        // 刪除 line-4 → 指向它的書籤 B 必須一併移除（唯一會失效的情況）
        cout << "  刪除 line-4（書籤 B 指向它）：" << endl;
        bm.eraseLine(l4);
        bm.dump();
        print_list("  最終文件", doc);
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 37 課：list 迭代器失效規則1.cpp" -o lesson37
// 想在執行期抓出「使用失效迭代器」，開發時可加 -D_GLIBCXX_DEBUG 或 -fsanitize=address。

// 注意：第 4 段印出的記憶體位址每次執行都不同（配置器行為 + ASLR）。
//       重點在「sort 前後位址完全相同」這個關係，不在數值本身。

// === 預期輸出 ===
// ===== 1. insert 後迭代器有效 =====
// *it10=10 *it20=20 *it30=30 *it40=40 *it50=50
// 完整 list [10]: 5 10 20 25 30 40 45 48 50 60
// → 所有舊迭代器在插入後仍然有效！
//
// ===== 2. erase 只影響被刪的 =====
// *it10=10 *it20=20 *it40=40 *it50=50
// 刪除 30 後 [4]: 10 20 40 50
//
// --- 對比 vector ---
// erase 前 *vit40 = 40
// erase 後 vec[3] = 50（vit40 已失效，故改用索引讀取，不解參考 vit40）
// → 同樣是 erase，vector 讓刪除點之後的迭代器全部失效，list 只讓被刪的那一個失效
//
// ===== 3. splice 後迭代器有效 =====
// splice 前: *it_1=1 *it_20=20
// splice 後: *it_1=1 *it_20=20
// A [4]: 1 2 3 20
// B [2]: 10 30
// → it_20 仍有效，但現在屬於 A（++it_20 會走 A 的元素，不是 B 的）
//
// ===== 4. sort 後迭代器有效 =====
// sort 前: *it_30=30 addr=0x55f5589723b0
// sort 前: *it_40=40 addr=0x55f558972390
// sort 後: *it_30=30 addr=0x55f5589723b0
// sort 後: *it_40=40 addr=0x55f558972390
// 排序結果 [5]: 10 20 30 40 50
// → 值和地址都不變！只是在 list 中的位置改變了
//
// ===== 5. remove 的迭代器影響 =====
// remove 前: 1 3 4 5
// remove 後: 1 3 4 5
// 結果 [4]: 1 3 4 5
//
// ===== 6. reverse 後迭代器有效 =====
// reverse 前: begin→10 second→20
// reverse 後: *it_begin=10 *it_second=20
// lst.front()=50 lst.back()=10
// it_begin == lst.begin()? 否
// it_begin == prev(lst.end())? 是
//
// ===== 7. 常見錯誤 vs 正確寫法 =====
// --- 錯誤模式 1：erase 後用失效迭代器 ---
//
//   // ❌ 錯誤！
//   auto it = lst.begin();
//   lst.erase(it);
//   ++it;           // 未定義行為！it 指向已釋放的節點
//
//   // ✔ 正確
//   auto it = lst.begin();
//   it = lst.erase(it);   // erase 回傳下一個有效迭代器
//
// --- 錯誤模式 2：迴圈中 erase + ++it ---
//
//   // ❌ 錯誤！
//   for (auto it = lst.begin(); it != lst.end(); ++it) {
//       if (*it == target) {
//           lst.erase(it);   // it 失效
//       }                    // 下一次 ++it → 未定義行為！
//   }
//
//   // ✔ 正確
//   for (auto it = lst.begin(); it != lst.end(); ) {
//       if (*it == target) {
//           it = lst.erase(it);   // erase 回傳下一個
//       } else {
//           ++it;
//       }
//   }
//
//   // ✔ 更簡潔：C++20 起
//   std::erase(lst, target);       // 或 std::erase_if(lst, pred)
//   // C++20 之前：lst.remove(target);  /  lst.remove_if(pred);
//
// --- 錯誤模式 3：splice 後用舊容器遍歷 ---
//
//   list<int> A = {1, 2, 3};
//   list<int> B = {10, 20};
//   auto it = B.begin();    // 指向 B 的 10
//
//   A.splice(A.end(), B, it);  // 10 移到了 A
//
//   // ❌ 概念錯誤（雖然 *it 仍然有效）：
//   // 不能期望從 it 遍歷能走到 B 的其他元素
//   // 因為 it 現在屬於 A 了
//
//   // ✔ 正確認知：it 指向的節點現在在 A 中
//   // ++it 會走到 A 中 10 之後的元素，不是 B 的元素
//
// ===== 8. 實戰：多迭代器書籤系統 =====
// 書籤1: 第二章
// 書籤2: 第五章
// 操作後書籤1: 第二章
// 操作後書籤2: 第五章
// 最終文件 [6]: 新增章節A 第一章 第三章 第二章 第五章 第六章
// → 經過 insert、erase、sort 後，書籤始終有效！
// （若換成 vector，上面任何一個操作都可能讓書籤懸空）
//
// ===== 9. LeetCode 146. LRU Cache =====
// get(1) = 1   （預期 1）
// get(2) = -1  （預期 -1，已被淘汰）
// get(1) = -1  （預期 -1，已被淘汰）
// get(3) = 3   （預期 3）
// get(4) = 4   （預期 4）
// → 全靠 list 迭代器不失效，才能把迭代器存進 hash map
//
// ===== 10. 日常實務：編輯器書籤 =====
//   初始書籤：
//     書籤 A → line-2
//     書籤 B → line-4
//   插入 line-0 之後：
//     書籤 A → line-2
//     書籤 B → line-4
//   刪除 line-4（書籤 B 指向它）：
//     書籤 A → line-2
//   最終文件 [4]: line-0 line-1 line-2 line-3
