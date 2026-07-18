// =============================================================================
// 檔名: erase.cpp
// 主題: std::string::erase (移除指定字元)
// 參考:
//   - https://en.cppreference.com/cpp/string/basic_string/erase
//   - https://cplusplus.com/reference/string/string/erase/
// =============================================================================
//
// 【函式資訊 Information】
//   // index 版 (回傳 *this)
//   string&  erase(size_type index = 0, size_type count = npos);
//
//   // iterator 版 (回傳 iterator,指向被刪除位置之後的字元)
//   iterator erase(const_iterator position);
//   iterator erase(const_iterator first, const_iterator last);
//
// 非成員函式 (C++20 起):
//   template<class CharT, class Traits, class Alloc, class U>
//   constexpr typename basic_string<CharT,Traits,Alloc>::size_type
//       erase(basic_string<CharT,Traits,Alloc>& c, const U& value);
//   // erase_if(c, predicate) — 移除符合條件的字元,回傳被移除個數
//
// =============================================================================
//
// 【詳細解釋 Explanation - 設計理念與底層運作】
//
// 1. 三個重載各自的角色
//    -----------------------------------------------------------------
//    - erase(index, count):
//        從 index 開始移除 count 個字元 (count 預設 npos = 直到結尾)。
//        最 idiomatic、最常用、回傳 *this 方便鏈式呼叫。
//    - erase(it):
//        移除 it 指向的單一字元,回傳指向下一個字元的迭代器。
//        適合與 std::find 等演算法鏈接。
//    - erase(first, last):
//        移除 [first, last) 範圍,回傳指向 last 位置的迭代器。
//        適合迭代器為主的演算法 (std::remove_if + erase 慣用法)。
//
// 2. 底層運作:就是把後面字元搬到前面
//    -----------------------------------------------------------------
//    erase 不會釋放記憶體,只是把「被刪除位置之後」的字元用 memmove
//    往前覆蓋,然後把 size 減小、把 buffer[new_size] 設為 '\0'。
//    capacity 完全不變 — 這與 vector::erase 行為一致。
//
// 3. 時間複雜度
//    -----------------------------------------------------------------
//    - O(N) — 後續字元需向前搬移;
//    - 即使刪除單一字元,最壞情況仍是 O(N) (字串開頭刪除最痛);
//    - 從尾端刪 (erase(size()-1) 或 erase(end()-1)) 接近 O(1)。
//
// 4. 邊界與例外
//    -----------------------------------------------------------------
//    - index > size() → std::out_of_range;
//    - count > size() - index 不會丟例外,自動截斷到結尾;
//    - first / last 必須是合法迭代器、且 first <= last,否則 UB;
//    - erase 對 string 是 noexcept (前提是 allocator 不丟例外);
//    - 例外安全: noexcept,沒有「中途失敗」這回事 — memmove 不會丟。
//
// 5. 迭代器/指標/參考失效規則
//    -----------------------------------------------------------------
//    - 不會觸發 reallocation (因為 erase 只縮不長);
//    - 「被刪除位置之前」的迭代器/指標仍有效;
//    - 「被刪除位置與其後」的迭代器/指標全部失效 (位置改變了);
//    - end() 一律失效 (位置變了)。
//
// 6. 經典陷阱:邊走邊 erase 的迴圈
//    -----------------------------------------------------------------
//    ❌ 錯誤寫法:
//        for (auto it = s.begin(); it != s.end(); ++it) {
//            if (*it == 'x') s.erase(it);    // erase 後 it 失效,++it = UB
//        }
//    ❌ 另一個錯誤:
//        for (auto it = s.begin(); it != s.end(); ++it) {
//            if (*it == 'x') it = s.erase(it);   // 跳過 erase 後的下一個字元!
//        }
//    ✅ 正確寫法 1:條件性遞增
//        for (auto it = s.begin(); it != s.end(); /* no ++ */) {
//            if (*it == 'x') it = s.erase(it);   // erase 已回傳「下一個」
//            else            ++it;
//        }
//    ✅ 正確寫法 2 (推薦):erase-remove idiom
//        s.erase(std::remove(s.begin(), s.end(), 'x'), s.end());
//    ✅ 正確寫法 3 (最推薦,C++20+):std::erase / std::erase_if
//        std::erase(s, 'x');
//        std::erase_if(s, [](char c){ return std::isdigit((unsigned char)c); });
//
// 7. erase-remove idiom 為什麼存在?
//    -----------------------------------------------------------------
//    std::remove 不會「真的刪除」,它只是把 *不要保留* 的元素往後挪、
//    回傳「新邏輯結尾」。然後容器的 erase(new_end, end()) 才把尾巴清掉。
//    這樣設計是因為 std::remove 是「演算法」,不知道容器有 erase。
//    從 C++20 起,std::erase / std::erase_if 把這兩步驟封裝成一個函式。
//
// 8. C++11 / 17 / 20 / 23 的演進
//    -----------------------------------------------------------------
//    - C++11: 加入 const_iterator 重載 (原本只有 iterator)。
//    - C++20: 引入非成員 std::erase / std::erase_if,簡化 erase-remove。
//    - C++23: 加 constexpr 加強。
//
// =============================================================================
//
// 【概念補充 Concept Deep Dive】
//
// (A) 迭代器失效規則 (修改類操作)
//     | 操作                  | 規則                                    |
//     |----------------------|-----------------------------------------|
//     | reserve / shrink     | 觸發 realloc → 全部失效;否則全有效     |
//     | push_back / += 字元   | 觸發 realloc → 全部失效;否則 end() 失效|
//     | insert                | realloc → 全部;否則插入點及後失效       |
//     | erase                 | 不 realloc;刪除點及後失效              |
//     | clear                 | 全部失效 (邏輯上)                       |
//     | resize 變大           | realloc → 全部失效;否則 end() 失效      |
//     | resize 變小           | 不 realloc;新 end 之後失效              |
//     記住一個原則:「能改變記憶體位置或邏輯結尾的操作 → 對應位置之後失效」。
//
// (B) erase 不釋放 capacity 的設計理由
//     與 clear 同理:容器設計哲學是「reuse, not release」。erase 只縮 size,
//     buffer 留著供後續寫入。如果你「erase 後想釋放」,組合呼叫:
//         s.erase(...);
//         s.shrink_to_fit();
//
// (C) erase-remove idiom 的詳細運作 (C++20 前的標準寫法)
//     std::string s = "abcXdefXghi";
//     auto new_end = std::remove(s.begin(), s.end(), 'X');
//     // 此時 s = "abcdefghiXi" (前面是要保留的、後面是垃圾,但 size 沒變!)
//     s.erase(new_end, s.end());      // 真正裁掉垃圾
//     // s = "abcdefghi"
//     std::remove 並不知道 string 的細節,它只能在 [first, last) 範圍內
//     重排,容器要自己負責 erase。這就是為什麼 C++20 加了 std::erase。
//
// (D) Strong vs Basic Exception Guarantee
//     erase 通常 noexcept 或 basic guarantee — 因為「縮短」不可能失敗
//     (沒有 allocate)。即使 noexcept 不在簽名裡,實作上 erase 不會丟例外。
//     這代表你可以放心在 catch / destructor 內呼叫 erase 做清理動作。
//
// (E) 與其他容器 erase 的差異
//     - vector::erase:單元素 O(N)、範圍 O(N) — 與 string 完全一致;
//     - list::erase:O(1)、不失效其他迭代器;
//     - map::erase:O(log N)、不失效其他迭代器 (除了被刪的);
//     - 字串本身是連續 buffer,erase 必然 O(N)。如果你的應用大量做
//       中間刪除,改用 std::list<char> 或 std::deque<char> 可能更快。
//
// =============================================================================
//
// 【注意事項 Pay Attention】
// 1. index > size() 會丟 std::out_of_range。
// 2. erase 後所有指向「被刪除位置之後」的迭代器 / 指標 / 參考都會失效。
// 3. 邊走邊 erase 的常見錯誤:在 for 迴圈用 ++it 與 erase(it) 混用會跳過字元。
//    正確寫法:it = erase(it),或使用 std::remove_if + erase 慣用法
//    (C++20 起可用 std::erase_if)。
// 4. erase 不會改變 capacity()。要釋放需配合 shrink_to_fit。
// 5. 例外安全: noexcept (對 string)。
// 6. 整段刪光等同 clear,但 clear 語意更清楚、優先使用。
// =============================================================================

/*
補充筆記：std::string::erase
  - std::string::erase 需要同時考慮內容、容量與舊指標失效；字串 API 常會配置或搬移緩衝區。
  - 回傳位置的函式要先處理 npos，回傳 pointer/view 的函式要追蹤來源 string 生命週期。
  - UTF-8 文字下，size 代表位元組數，不代表使用者看到的字元數。
  - std::string::erase 是 std::string 主題的一部分；先分清楚這個操作會讀取、追加、插入、刪除、搜尋，還是只暴露底層字元。
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
#include <cctype>
#include <algorithm>

void demoErase() {
    // (1) 用 index/count
    std::string a = "Hello, World!";
    a.erase(5, 7);                              // 移除 ", World"
    std::cout << "(1) " << a << "\n";           // "Hello!"

    // (2) 用 iterator 移除單一字元
    std::string b = "abcdef";
    b.erase(b.begin() + 2);                     // 移除 'c'
    std::cout << "(2) " << b << "\n";           // "abdef"

    // (3) 用 range
    std::string c = "abcdef";
    c.erase(c.begin() + 1, c.begin() + 4);      // 移除 "bcd"
    std::cout << "(3) " << c << "\n";           // "aef"

    // (4) C++20 std::erase 從 string 移除特定字元 (一行解決,無需 erase-remove)
    std::string d = "ababab";
    std::erase(d, 'a');                         // 移除所有 'a'
    std::cout << "(4) " << d << "\n";           // "bbb"

    // (5) C++20 std::erase_if (predicate) — 條件式移除
    std::string e = "Hello123World456";
    std::erase_if(e, [](char ch){ return std::isdigit(static_cast<unsigned char>(ch)); });
    std::cout << "(5) " << e << "\n";           // "HelloWorld"
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1047. Remove All Adjacent Duplicates In String (Easy)
// 題目: 反覆移除字串中相鄰重複的兩個字元,直到不能再移除。
//       例如 "abbaca" → "aaca" → "ca"。
// 為何用 erase: 用 string 當 stack — 遇到與 back() 相同的字元就 pop_back()
//               (等同 erase end-1)。這裡示範 erase 的 iterator 版本。
// 思路: 線性掃過,維護一個 stack 字串,新字元若與 stack top 相同就消去配對。
// 複雜度: O(N) — 每個字元最多 push/pop 一次。
// -----------------------------------------------------------------------------
std::string removeDuplicates(const std::string& s) {
    std::string st;
    st.reserve(s.size());                       // 上限預估
    for (char c : s) {
        if (!st.empty() && st.back() == c) st.erase(st.end() - 1); // 配對成功消去
        else st += c;                                              // 否則 push
    }
    return st;
}

// -----------------------------------------------------------------------------
// 【實務範例】移除字串首尾空白 (trim)
// 為何用 erase: 兩端搜尋第一個/最後一個非空白字元,erase 多餘部分。
//               經典寫法,std::find_if_not + erase。
// -----------------------------------------------------------------------------
std::string trim(std::string s) {
    auto isSpace = [](unsigned char c){ return std::isspace(c); };
    auto first = std::find_if_not(s.begin(), s.end(), isSpace);
    s.erase(s.begin(), first);                                      // 砍頭
    auto last = std::find_if_not(s.rbegin(), s.rend(), isSpace).base();
    s.erase(last, s.end());                                         // 砍尾
    return s;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 Daily Work】移除單行註解 (// 之後)
// 為何用 erase: 簡易程式碼預處理 / 設定檔解析 (#、//、--) 都常需要去掉註解。
//               find + erase 是經典寫法。但要小心字串字面值內的 "//"
//               (這個簡化版不處理,實務應用常會配合狀態機)。
// -----------------------------------------------------------------------------
std::string stripLineComment(std::string line, const std::string& marker = "//") {
    size_t p = line.find(marker);
    if (p != std::string::npos) line.erase(p);  // 從 marker 起全砍掉
    return line;
}

// -----------------------------------------------------------------------------
// 【LeetCode 範例 2】LeetCode 1910. Remove All Occurrences of a Substring
// 題目: 反覆把 part 從 s 中刪除,直到不再出現。
// 為何用 erase: 找到位置後直接 erase(pos, part.size()),經典 find + erase 模式。
// 複雜度: O(N^2/M) (最壞)。
// -----------------------------------------------------------------------------
std::string removeOccurrencesAll(std::string s, const std::string& part) {
    size_t p;
    while ((p = s.find(part)) != std::string::npos) {
        s.erase(p, part.size());
    }
    return s;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】去除 BOM (UTF-8 byte order mark) 前綴
// 為何用 erase: 從外部讀進文字檔常見開頭 \xEF\xBB\xBF (UTF-8 BOM);若不去掉
//                會被當成第一個字串的一部分,導致比對失敗。erase(0, 3) 一發解決。
// -----------------------------------------------------------------------------
std::string stripUtf8Bom(std::string s) {
    if (s.size() >= 3 &&
        static_cast<unsigned char>(s[0]) == 0xEF &&
        static_cast<unsigned char>(s[1]) == 0xBB &&
        static_cast<unsigned char>(s[2]) == 0xBF) {
        s.erase(0, 3);
    }
    return s;
}

int main() {
    demoErase();

    std::cout << "\n=== LeetCode 1047 ===\n";
    std::cout << removeDuplicates("abbaca") << "\n";    // "ca"

    std::cout << "\n=== LeetCode 1910 ===\n";
    std::cout << removeOccurrencesAll("daabcbaabcbc", "abc") << "\n";   // dab
    std::cout << removeOccurrencesAll("axxxxyyyyb", "xy")    << "\n";   // axxxyyyb (注意 erase 是非重疊)

    std::cout << "\n=== trim ===\n";
    std::cout << "[" << trim("   hello world   ") << "]\n";

    std::cout << "\n=== 日常實務: 去除單行註解 ===\n";
    std::cout << "[" << stripLineComment("int x = 1; // initial value") << "]\n";
    std::cout << "[" << stripLineComment("port=8080  # default", "#") << "]\n";

    std::cout << "\n=== 日常實務: 去 UTF-8 BOM ===\n";
    std::string withBom = "\xEF\xBB\xBFhello";
    std::cout << "before: size=" << withBom.size() << "\n";
    auto noBom = stripUtf8Bom(withBom);
    std::cout << "after : size=" << noBom.size() << " content=[" << noBom << "]\n";

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1:erase 的 index 版回傳什麼?iterator 版又回傳什麼?
    //    A:index 版 (erase(pos, count)) 回傳 string& (即 *this) 方便鏈式呼叫;
    //       iterator 版 erase(it) 與 erase(first, last) 回傳 iterator,指向
    //       「被刪除位置之後」的字元 — 這是與「邊走邊 erase」迴圈正確配合的關鍵。
    //
    //  Q2:erase 後 capacity 會縮小嗎?如果想要把 buffer 真的釋放掉怎麼辦?
    //    A:不會。erase 永遠只縮 size、不縮 capacity (與 vector::erase 一致,
    //       reuse-not-release 設計)。要釋放需呼叫 shrink_to_fit (non-binding),
    //       或 swap 一個全新的小字串進來強制換掉 buffer。
    //
    //  Q3:erase-remove idiom 與 C++20 std::erase / std::erase_if 差在哪?
    //    A:erase-remove 要 s.erase(std::remove(b,e,'x'), s.end()) 兩段式寫法,
    //       因為 std::remove 只重排不縮容器。C++20 把這 pattern 包成單行
    //       std::erase(s, 'x') / std::erase_if(s, pred),語意更直接、不易寫錯,
    //       且對 vector / deque / list 等容器同名同義 (uniform container erasure)。
    //
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra erase.cpp -o erase

// === 預期輸出 (節錄) ===
// === LeetCode 1910 ===
// dab
// === 日常實務: 去 UTF-8 BOM ===
// before: size=8
// after : size=5 content=[hello]
