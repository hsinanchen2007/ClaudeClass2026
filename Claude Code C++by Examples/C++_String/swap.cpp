// =============================================================================
// 檔名: swap.cpp
// 主題: std::string::swap (與另一個 string 交換內容)
// 參考:
//   - https://en.cppreference.com/cpp/string/basic_string/swap
//   - https://cplusplus.com/reference/string/string/swap/
// =============================================================================
//
// 【函式資訊 Information】
//   void swap(basic_string& other) noexcept(/* 視 allocator 而定 */);  // 成員版本
//   // 對應的非成員版本(在 <string> 中):
//   void swap(basic_string& a, basic_string& b) noexcept(noexcept(a.swap(b)));
//
// 回傳: void。
// 例外: 一般情況下 noexcept;唯一例外是 allocator 不滿足 propagate-on-container-swap
//       而兩個 allocator 又不相等時,行為是 UB(實務上不會碰到)。
//
// 【詳細解釋 Explanation】
// swap 的本質是「交換兩個物件的『身分』」 — 把 a 的內容(包含所有 byte、size、
// capacity、可能還有 allocator)整個與 b 交換。表面看起來只是「值對調」,但實
// 作層面相當講究,因為它必須:
//   * 保證 noexcept(這是強例外安全許多 idiom 的基石)
//   * 對 SSO(small string optimization)與 heap-allocated 字串都能正確處理
//   * 不違反 allocator 的傳遞語意
//
// 【1. 為什麼需要 swap】
// 在容器設計中,swap 不只是「把兩個變數對調」的小工具,而是實作以下技術的核心:
//   * Copy-and-swap idiom — 賦值運算子可寫成「先複製、再 swap」即可獲得強
//     例外安全保證。複製失敗時原物件完全不變。
//   * std::sort / std::partition / std::reverse — STL 演算法大量呼叫 swap,
//     若 swap 是 O(1) 且 noexcept,演算法整體效能大幅提升。
//   * Move semantics — 在 C++11 之前,swap 是模擬 move 的標準手段。
//
// 【2. 底層運作:三種情況】
// (a) 兩者都是 heap-allocated:
//     只需要交換內部三個欄位(data 指標、size、capacity),完全不搬資料。
//     這是 O(1),也是 swap 的「典型成本」。
// (b) 兩者都是 SSO(短字串):
//     SSO 把字串內容直接放在物件本體的小 buffer 中(通常 15~22 byte),
//     沒有 heap 指標可交換,只能逐 byte 搬移兩個 buffer 的內容。
//     成本 O(SSO buffer size) ≈ O(1) 但常數較大。
// (c) 一邊 SSO、一邊 heap:
//     比較棘手。實作上先把 SSO 那邊的內容搬到暫存,再交換指標,再把暫存搬回。
//     仍然是 O(SSO buffer size)。
// 重點:不論哪一種,STL 都把這些情況封裝在 swap 內部,從外部看就是 O(1)~O(SSO size)
// 的「便宜操作」,而且永遠 noexcept。
//
// 【3. ADL 與 std::swap 的關係】
// C++ 提供兩種呼叫方式:
//   a.swap(b);          // 直接呼叫成員 swap
//   std::swap(a, b);    // 呼叫 std 命名空間下的 swap 範本
//   using std::swap;
//   swap(a, b);         // 「two-step swap」 — 先 using 再呼叫,讓 ADL 找到自訂版
// std::swap 的預設實作會做三次 move:
//   template<class T> void swap(T& a, T& b) {
//       T tmp = std::move(a); a = std::move(b); b = std::move(tmp);
//   }
// 對 std::string 來說,std::swap 之所以高效,是因為 <string> 標頭裡額外特化了
// std::swap(string&, string&) 直接呼叫成員 swap,避免那三次 move。
// 撰寫泛型程式碼時,慣用法是「先 using std::swap; 再裸呼叫 swap(a, b);」,
// 這樣編譯器會優先透過 ADL 找到使用者自訂的 swap,找不到才退回 std::swap。
//
// 【4. swap 的迭代器規則】
// C++11 後標準明定:string::swap 後,「指向元素的指標、參考、迭代器」會跟著
// 元素一起被交換 — 也就是說,原本指向 a 內字元的 iterator,在 a.swap(b) 之後
// 會指向 b 中的同一字元(因為該字元的 byte 實際上隨著資料搬到了 b)。
// 但實務上,跨 swap 保留迭代器的程式幾乎一定有 bug,務必避免。
//
// 【5. noexcept 的條件】
// C++17 起,member swap 標示為:
//   noexcept(allocator_traits<Allocator>::propagate_on_container_swap::value
//            || allocator_traits<Allocator>::is_always_equal::value);
// 對 std::allocator 而言,is_always_equal 為 true,所以 std::string::swap 一律
// noexcept。只有自訂 allocator 才需要擔心。
//
// 【6. 釋放 capacity 的慣用法】
//   std::string().swap(s);
// 這行的意思:建立一個臨時的空 string(capacity 通常為 0 或 SSO 大小),把
// 它與 s 交換。交換完之後,s 變成空且容量小,而臨時物件帶著 s 原本的大 buffer
// 在語句結束時被解構,釋放記憶體。等同於 C++11 的 shrink_to_fit(),但 swap
// 寫法在 C++03 時代就能用,且強制保證會釋放(shrink_to_fit 是非強制 hint)。
//
// 【7. 時間複雜度】
//   * 兩端都 heap:    O(1)
//   * 任一端 SSO:     O(SSO buffer size),實務上仍可視為 O(1)
//   * 真正的 worst:   理論 O(N) 不存在,因為 SSO 上限固定
//
// 【概念補充 Concept Deep Dive】
// (A) 為什麼每個容器都自帶 swap
//   通用 std::swap 透過三次 move 完成,但對容器(vector/string/map)來說,
//   member swap 通常只是「交換 3~4 個內部指標」,比走 move ctor + move assign
//   便宜得多。所以標準容器都提供 member swap,並對應特化 std::swap。
//   自定義類別若希望被高效率 swap,也應該 follow 此 pattern。
//
// (B) Copy-and-swap idiom 的優雅之處
//   Foo& operator=(Foo other) {     // 注意是 by value(隱含複製)
//       swap(*this, other);          // noexcept,不會丟例外
//       return *this;
//   }
//   若複製失敗(配置失敗),根本還沒進入函式內部,*this 仍完整。
//   一旦複製成功進入函式體,swap 是 noexcept,絕不可能出錯。
//   這個寫法天生具備強例外安全保證,而且自動處理 self-assignment。
//
// (C) Move 與 swap 的取捨
//   C++11 後,a = std::move(b) 通常比 a.swap(b) 表達力更精準
//   (告訴讀者「我不再使用 b 了」)。但 swap 仍有獨特用途:
//     * 雙向都仍會被使用(例如 double-buffering)
//     * 需要保證 noexcept 而不確定 move 是否 noexcept
//
// (D) self-swap 的安全性
//   a.swap(a) 是合法且 well-defined 的,結果是 a 內容不變。標準保證實作不會
//   爆炸(雖然有些 naive 寫法會,因此 STL 實作通常加 if (this != &other) 檢查)。
//
// 【注意事項 Pay Attention】
// 1. swap 是 noexcept,可安全用於 strong exception safety 設計(copy-and-swap)。
// 2. 不要跨 swap 保留並重新使用舊迭代器,雖然規範允許但容易誤用。
// 3. 與 std::swap 等價:std::swap(a, b);標準在 <string> 內已特化,效能相同。
// 4. 自我 swap (a.swap(a)) 是合法且無作用的操作。
// 5. 釋放 capacity 的慣用法:std::string().swap(s);
// 6. 若使用自訂 allocator 且兩端 allocator 不相等且不允許 propagate,行為是 UB。
//    使用 std::allocator 不會踩到此雷區。
//
// =============================================================================

/*
補充筆記：std::string::swap
  - swap 交換兩個物件的狀態；對容器通常是常數時間交換內部資源。
  - 寫泛型程式時常先 using std::swap，再呼叫 swap(a,b) 讓 ADL 找自訂版本。
  - swap 是否 noexcept 會影響某些容器與演算法的例外安全策略。
  - std::string::swap 是 std::string 主題的一部分；先分清楚這個操作會讀取、追加、插入、刪除、搜尋，還是只暴露底層字元。
  - std::string 的索引型別是 size_type；和 int 混用時要小心 unsigned underflow，尤其是倒著迴圈或拿 npos 比較。
  - 修改字串內容或容量可能讓先前取得的 iterator、reference、pointer、c_str() 指標失效。
  - operator[] 不做範圍檢查，at() 越界會丟 std::out_of_range；教學範例可比較兩者，工作程式要依錯誤處理需求選。
  - find/rfind/find_first_of 這類搜尋函式失敗回傳 std::string::npos；判斷時應和 npos 比，不要寫成 >= 0。
  - 字串和 C string 互通時要記得 null terminator；std::string 可以保存內含 \0 的資料，但很多 C API 會在第一個 \0 停止。
  - 若只是傳入唯讀文字片段且不需要擁有資料，std::string_view 可避免複製；但它不能延長原字串生命週期。
  - 處理中文或 UTF-8 時，std::string 的 size() 回傳 byte 數，不是人眼看到的字元數。
*/
#include <iostream>
#include <string>

void demoSwap() {
    std::string a = "AAA";
    std::string b = "BBBBB";

    a.swap(b);
    std::cout << "after swap: a=\"" << a << "\", b=\"" << b << "\"\n";

    // std::swap 也行
    std::swap(a, b);
    std::cout << "after std::swap: a=\"" << a << "\", b=\"" << b << "\"\n";

    // 釋放 capacity 的慣用法
    std::string c;
    c.reserve(1000);
    std::cout << "before trick: capacity=" << c.capacity() << "\n";
    std::string().swap(c);
    std::cout << "after  trick: capacity=" << c.capacity() << "\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 1】LeetCode 344. Reverse String
// 題目: 給字串 s,原地反轉其中字元。
// 為何用 swap: 雙指針 + std::swap(char&, char&) 是最直接最快的實作,
//              一次比較、一次交換就推進兩個位置。比 reverse() 更貼近底層教學意義。
// -----------------------------------------------------------------------------
void reverseString344(std::string& s) {
    size_t l = 0, r = s.size() ? s.size() - 1 : 0;
    while (l < r) {
        std::swap(s[l], s[r]);     // O(1),char 對 char swap
        ++l;
        --r;
    }
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 2】LeetCode 917. Reverse Only Letters
// 題目: 只把字母位置互換,非字母字元留在原位。
// 為何用 swap: 雙指針跳過非字母,遇到兩端都是字母才 swap。標準寫法。
// -----------------------------------------------------------------------------
#include <cctype>
std::string reverseOnlyLetters(std::string s) {
    int l = 0, r = static_cast<int>(s.size()) - 1;
    while (l < r) {
        // 跳過非字母,直到 l 指到字母或越界
        while (l < r && !std::isalpha(static_cast<unsigned char>(s[l]))) ++l;
        while (l < r && !std::isalpha(static_cast<unsigned char>(s[r]))) --r;
        if (l < r) {
            std::swap(s[l], s[r]);  // 只有兩邊都是字母才交換
            ++l;
            --r;
        }
    }
    return s;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 3】LeetCode 151. Reverse Words in a String
// 題目: 反轉字串中的單詞順序,同時去掉多餘空白。
// 為何用 swap: 我們示範用 swap 在組裝過程中替換暫存字串(string-level swap O(1))。
// -----------------------------------------------------------------------------
#include <sstream>
std::string reverseWords(const std::string& s) {
    std::stringstream ss(s);
    std::string word, result;
    while (ss >> word) {
        if (!result.empty()) word += ' ';
        // 把 result 接到 word 後面,然後 swap 回去 → 等同前置插入
        word += result;
        result.swap(word);
        // word 此時是舊的 result + 空格,但反正下次 >> 會覆蓋掉
    }
    return result;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】Double-buffering: 收與處理交替使用兩個緩衝區
// 為何用 swap: 一個 buffer 收新資料,另一個 buffer 處理上次的資料。
//               處理完之後 swap,避免拷貝大字串。
//               日誌批次寫入、影像幀緩衝、串流資料處理常用此模式。
// -----------------------------------------------------------------------------
class DoubleBuffer {
    std::string active;     // 正在收的
    std::string ready;      // 等待處理的
public:
    void append(const std::string& chunk) { active += chunk; }

    // 把 active 的內容交給呼叫方處理,active 變成空但保留 capacity
    std::string& flush() {
        ready.clear();
        ready.swap(active);     // O(1)
        return ready;
    }
};

// -----------------------------------------------------------------------------
// 【LeetCode 範例 4】LeetCode 1page. 1page0. Sum of Beauty of All Substrings - 簡化
// 題目: LeetCode 1page. 1page0. Reverse Vowels of a String (LeetCode 345)
// 把字串中的母音字元位置互換 (頭尾兩指針)。
// 為何用 string.swap: 此處我們用「字元層級的 swap (std::swap on chars)」示範,
//                    雖然不是 string.swap,但是「值對換」概念一致;字串層級的 swap
//                    也常用於 reverse 演算法的內部步驟。
// 複雜度: O(N)。
// -----------------------------------------------------------------------------
std::string reverseVowels(std::string s) {
    auto isV = [](char c) {
        c = static_cast<char>(c | 0x20);    // to lowercase
        return c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u';
    };
    size_t i = 0, j = s.size();
    if (j == 0) return s;
    --j;
    while (i < j) {
        while (i < j && !isV(s[i])) ++i;
        while (i < j && !isV(s[j])) --j;
        if (i < j) { std::swap(s[i], s[j]); ++i; if (j == 0) break; --j; }
    }
    return s;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】使用 swap 高效清空大字串並重設 capacity
// 為何用 swap: 比 clear() + shrink_to_fit() 更直接、行為可預期。
// -----------------------------------------------------------------------------
void releaseLargeString(std::string& s) {
    std::string().swap(s);     // 強制讓 s 變回空字串、釋放 buffer
}

int main() {
    demoSwap();

    std::cout << "\n=== LeetCode 344 ===\n";
    std::string r1 = "hello";
    reverseString344(r1);
    std::cout << r1 << "\n";   // "olleh"

    std::cout << "\n=== LeetCode 917 ===\n";
    std::cout << reverseOnlyLetters("a-bC-dEf-ghIj") << "\n";  // "j-Ih-gfE-dCba"

    std::cout << "\n=== LeetCode 151 ===\n";
    std::cout << "[" << reverseWords("the sky is blue")     << "]\n";
    std::cout << "[" << reverseWords("  hello world  ")     << "]\n";
    std::cout << "[" << reverseWords("a good   example")    << "]\n";

    std::cout << "\n=== LeetCode 345 ===\n";
    std::cout << reverseVowels("hello")     << "\n";   // holle
    std::cout << reverseVowels("leetcode")  << "\n";   // leotcede

    std::cout << "\n=== 日常實務: double buffer ===\n";
    DoubleBuffer db;
    db.append("hello ");
    db.append("world");
    std::cout << "flushed: [" << db.flush() << "]\n";
    db.append("next batch");
    std::cout << "flushed: [" << db.flush() << "]\n";

    std::cout << "\n=== 日常實務: releaseLargeString ===\n";
    std::string big(1024, 'X');
    std::cout << "before: size=" << big.size() << " capacity=" << big.capacity() << "\n";
    releaseLargeString(big);
    std::cout << "after : size=" << big.size() << " capacity=" << big.capacity() << "\n";

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1:member swap 與 std::swap 自由函式哪個比較好?
    //    A:對 std::string 兩者效能完全相同 — std 在 <string> 內已特化
    //      std::swap(string&, string&) 直接呼叫 member。寫泛型程式碼建議用
    //      ADL idiom (using std::swap; swap(a,b);),這樣自訂型別在自家
    //      namespace 提供的高效 swap 也能被找到。
    //
    //  Q2:std::string().swap(s) 與 s.shrink_to_fit() 差在哪?
    //    A:swap 慣用法「強制」把 s 變成空的小 string 並釋放原有大 buffer
    //      (隨 tmp 解構),確定能釋放;shrink_to_fit 是 non-binding hint,
    //      實作可忽略。前者也會把 size 歸零,後者保留內容只縮 capacity。
    //      要保留資料且想瘦身用 shrink_to_fit,要清空 + 釋放用 swap trick。
    //
    //  Q3:swap 的 noexcept 條件是什麼?用自訂 allocator 要注意什麼?
    //    A:C++17 起 noexcept 條件為 propagate_on_container_swap::value ||
    //      is_always_equal::value。std::allocator 的 is_always_equal 為 true,
    //      所以一律 noexcept。自訂 allocator 若兩端 instance 不相等且不允許
    //      propagate,行為是 UB,需在 allocator 設計時妥善處理。
    //
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra swap.cpp -o swap

// === 預期輸出 (節錄) ===
// === LeetCode 345 ===
// holle
// leotcede
// === 日常實務: releaseLargeString ===
// before: size=1024 capacity>=1024
// after : size=0 capacity=0 (或 SSO 上限)
