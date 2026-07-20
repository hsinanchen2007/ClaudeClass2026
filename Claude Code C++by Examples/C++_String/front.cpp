// =============================================================================
// 檔名: front.cpp
// 主題: std::string::front (取得第一個字元)
// 參考連結 References:
//   cppreference: https://en.cppreference.com/cpp/string/basic_string/front
//   cplusplus.com: https://cplusplus.com/reference/string/string/front/
// =============================================================================
//
// 【函式資訊 Information】
//   reference       front();
//   const_reference front() const;
//
// 等同於 operator[](0) 或 *begin()。
//
// =============================================================================
//
// 【詳細解釋 Explanation】
//
// (一) 為何要有 front()?
// ----------------------------------------------------------------------------
// 你可能會問:既然 s[0] 就行,為什麼還要 front()?原因有三:
//   1. 語意更清楚:s.front() 一看就知道「取第一個元素」,比 s[0] 更直白。
//      在閱讀代碼時,front() 顯示作者「明確地」要取頭部,不是用 0 當索引。
//   2. 與其他容器一致:std::vector::front、std::deque::front、std::list::front
//      都是 STL 規範的「序列容器通用介面」。學會 front 等於學會所有序列容器。
//   3. 寫泛型程式碼 (template) 時必備:你不知道某 container 是否支援 [],
//      但「序列容器」一定有 front。寫 template<class C> auto first(const C& c)
//      { return c.front(); } 才能跨容器工作。
//
// (二) 與 at(0)、operator[](0)、*begin() 的差異
// ----------------------------------------------------------------------------
//   操作              | 空字串時的行為            | 是否有邊界檢查
//   ------------------|---------------------------|----------------
//   front()           | Undefined Behavior        | 無
//   operator[](0)     | Undefined Behavior        | 無 (s[0] 在 const 版本實際合法,回傳 '\0')
//   at(0)             | throw out_of_range        | 有
//   *begin()          | Undefined Behavior        | 無
//
// 注意:對 const 空字串呼叫 operator[](0),C++11 起標準保證會回傳 '\0' (合法),
// 但 front() 沒有這個保證,空字串呼叫 front() 一律 UB。差別來自於 front()
// 的語意是「第一個有效字元」,空字串本來就沒有有效字元。
//
// (三) 時間複雜度
// ----------------------------------------------------------------------------
// O(1) — 直接讀取緩衝區第一個 byte。
//
// (四) 回傳 reference 的意義
// ----------------------------------------------------------------------------
// front() 回傳 char& (非 const) 或 const char& (const string),意味著:
//   s.front() = 'X';   // 直接修改第一個字元
// 這與 STL 其他容器 front() 的設計完全一致。
//
// (五) C++ 標準演進
// ----------------------------------------------------------------------------
//   C++11 起新增 (與 back()、cbegin/cend、emplace 系列一同進來)。
//   C++11 之前要寫 s[0],雖然行為相同,但語意較不明確。
//
// =============================================================================
//
// 【概念補充 Concept Deep Dive】
//
// 1) 「序列容器 (Sequence Container)」的通用介面
//    front()、back()、size()、empty()、begin()、end() 是所有序列容器
//    (vector、deque、list、forward_list、string) 共同的成員。設計這套
//    一致介面的好處:
//      - 學一次用到處
//      - 寫泛型 template 函式時可任意替換容器
//      - C++ STL 演算法 (std::find、std::sort 等) 全靠這層抽象運作
//
// 2) Range-based for loop 與 front 的對應
//    for (char c : s)  其實就是 for (auto it = s.begin(); it != s.end(); ++it)
//    第一輪迴圈拿到的 *it 等於 s.front()。當你只需要看一眼第一個元素時,
//    用 front() 就不用啟動整個迴圈,語意更精準。
//
// 3) UB (Undefined Behavior) 的真實風險
//    對空字串呼叫 front() 不保證會 crash。它可能:
//      - 讀到 SSO buffer 中殘留的舊資料 (回傳奇怪字元)
//      - 讀到 heap 中 dangling 區域 (隨機資料)
//      - 在某些編譯器最佳化下被當作「不可能發生」而引發更詭異的錯誤
//    所以「永遠先檢查 !empty()」是基本紀律。
//
// 4) front() 與「peek 第一個 token」的對應
//    在解析器 (parser) 寫法中,front() 等於 lexer 的「peek 下一個 token」。
//    用 front() 做分支判斷,然後 erase(0,1) 或 substr(1) 消費,是字串
//    解析的常見手法。
//
// 5) 與 std::string_view::front 的關係
//    std::string_view 也提供 front(),語意完全一致,但 string_view 不擁有
//    資料,front() 回傳的 reference 指向別人的記憶體。如果原始字串生命
//    週期結束,view::front 就成了 dangling — 這是 string_view 常見的雷。
//
// =============================================================================
//
// 【注意事項 Pay Attention】
// 1. 呼叫前務必檢查 !empty(),否則為 UB。
// 2. 與 at(0) 不同 — at(0) 會在空字串時丟例外,front() 不會。
// 3. 回傳值是 reference,可直接修改: s.front() = 'X';
// 4. front() 不是 noexcept (標準未明示),保守起見視同會 throw 處理。
// 5. reallocation / clear 後,先前取得的 reference 失效。
//
// =============================================================================

/*
補充筆記：std::string::front
  - std::string::front 需要同時考慮內容、容量與舊指標失效；字串 API 常會配置或搬移緩衝區。
  - 回傳位置的函式要先處理 npos，回傳 pointer/view 的函式要追蹤來源 string 生命週期。
  - UTF-8 文字下，size 代表位元組數，不代表使用者看到的字元數。
  - std::string::front 是 std::string 主題的一部分；先分清楚這個操作會讀取、追加、插入、刪除、搜尋，還是只暴露底層字元。
  - std::string 的索引型別是 size_type；和 int 混用時要小心 unsigned underflow，尤其是倒著迴圈或拿 npos 比較。
  - 修改字串內容或容量可能讓先前取得的 iterator、reference、pointer、c_str() 指標失效。
  - operator[] 不做範圍檢查，at() 越界會丟 std::out_of_range；教學範例可比較兩者，工作程式要依錯誤處理需求選。
  - find/rfind/find_first_of 這類搜尋函式失敗回傳 std::string::npos；判斷時應和 npos 比，不要寫成 >= 0。
  - 字串和 C string 互通時要記得 null terminator；std::string 可以保存內含 \0 的資料，但很多 C API 會在第一個 \0 停止。
  - 若只是傳入唯讀文字片段且不需要擁有資料，std::string_view 可避免複製；但它不能延長原字串生命週期。
  - 處理中文或 UTF-8 時，std::string 的 size() 回傳 byte 數，不是人眼看到的字元數。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::string::front
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. front() 等同於什麼?既然有 s[0] 為什麼還要它?
//     答:front() 等同 operator[](0),也等同 *begin();回傳 reference,
//         所以 s.front() = 'H' 是合法的原地修改。存在的理由是介面統一:
//         vector、deque、list 等序列容器都有 front(),寫 template 泛型程式時
//         不能假設容器有 operator[](list 就沒有),但序列容器一定有 front()。
//         可讀性上 front() 也比 s[0] 更直白地表達「取頭部」。
//     追問:const 物件呼叫會回傳什麼?→ const_reference,不能拿來寫入。
//
// 🔥 Q2. front() 有邊界檢查嗎?空字串上呼叫會怎樣?
//     答:沒有檢查。標準規定 empty() 為 true 時呼叫 front() 是 UB ——
//         它不像 at() 會丟例外,也不保證回傳結尾的 '\0'。
//         安全寫法是先問 if (!s.empty()),或改用 s.at(0)(空字串時丟
//         std::out_of_range)。
//     追問:那 s[0] 在空字串上呢?→ 那是 s[s.size()],C++11 起「讀取」合法
//         (得到 '\0' 的引用),寫入非 '\0' 才是 UB —— 語意跟 front() 不同,
//         這正是兩者「等同」只在 size() > 0 時才成立的原因。
//
// ⚠️ 陷阱. 讀一行輸入後直接 if (line.front() == '#') 跳過註解,錯在哪?
//     答:輸入是空行時 line.empty(),front() 就是 UB。而且這個 bug 極難發現:
//         多數實作會安分地讀到那個 '\0' 回傳 0,測試時「看起來正常」,
//         直到換了編譯器、開了最佳化或改了 SSO 路徑才爆。
//     為什麼會錯:誤以為「反正 buffer 尾端有 '\0',讀到的頂多是 0」。
//         但 UB 的重點不是「當下讀到什麼值」,而是編譯器有權假設它不發生 ——
//         它可以據此刪掉你後面的檢查。正確寫法:
//         if (!line.empty() && line.front() == '#'),或 C++20 的
//         line.starts_with('#')(空字串時安全地回傳 false)。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>

void demoFront() {
    std::string s = "Hello";
    std::cout << "s.front() = " << s.front() << "\n";   // 'H'
    s.front() = 'h';
    std::cout << "after write: " << s << "\n";          // "hello"
}

// -----------------------------------------------------------------------------
// 【LeetCode 範例】LeetCode 1021. Remove Outermost Parentheses (Easy)
//
// 題目敘述:
//   給一個合法的括號字串 s,它由若干「primitive」(最外層配對的括號) 串接而成。
//   把每個 primitive 的最外層括號移除後串接回傳。
//   範例: "(()())(())" → "()()()"
//        "(()())(())(()(()))" → "()()()()(())"
//
// 為何用 front:
//   解析括號時常需要「先看一眼當前的第一個字元」決定動作 — 這正是 front()
//   的典型用途。本題我們用 depth 計數已經夠,但程式末段示範用 front()
//   做防禦性檢查,展示「peek 第一個字元」的思維。
//
// 解題思路:
//   用 depth 計數當前巢狀深度。
//   遇 '(' 時:depth>0 才加入結果 (因為 depth==0 的 '(' 是最外層)。
//   遇 ')' 時:先 --depth,depth>0 才加入結果。
//
// 複雜度: 時間 O(n),空間 O(n)
// -----------------------------------------------------------------------------
std::string removeOuterParentheses(const std::string& s) {
    std::string result;
    int depth = 0;
    for (char c : s) {
        if (c == '(') {
            if (depth > 0) result += c;
            ++depth;
        } else { // ')'
            --depth;
            if (depth > 0) result += c;
        }
    }
    // 額外示範: front() 拿邊界字元做防禦性檢查
    if (!s.empty() && s.front() != '(') {
        // 真正的題目保證輸入合法,這裡只是示範 front() 的用法
    }
    return result;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 Daily Work】偵測並去除 UTF-8 BOM
//
// 為何用 front:
//   讀取 Windows 文字檔常會看到開頭三個 bytes EF BB BF (UTF-8 Byte Order Mark)。
//   若忽略,JSON 解析、INI 設定檔讀取都會踩雷 — 第一個 key 解析成
//   "\xEF\xBB\xBFkey" 會匹配失敗。實務上每個讀檔工具都會做這個檢查。
//   front() 在這裡用來示範「peek 並驗證第一個 byte」的常見模式。
// -----------------------------------------------------------------------------
std::string stripUtf8BOM(std::string s) {
    if (s.size() >= 3 &&
        static_cast<unsigned char>(s[0]) == 0xEF &&
        static_cast<unsigned char>(s[1]) == 0xBB &&
        static_cast<unsigned char>(s[2]) == 0xBF) {
        s.erase(0, 3);
    }
    return s;
}

// -----------------------------------------------------------------------------
// 【LeetCode 範例 2】LeetCode 1location7. 1location0. Maximum Number of Words You Can Type
// 題目: LeetCode 1935. Maximum Number of Words You Can Type
// 給 text (空白分隔的單字) 與 brokenLetters,計算「可以打出」的單字數量。
// 為何用 front: 在掃描單字時可用 front() 快速 peek 第一個字元(本題實際是逐字檢查)。
// 我們示範另一個更貼近 front() 的變奏 ── 計算「首字母 in {brokenLetters} 的單字數」。
// 解法: 在分詞時用 front() 快速判斷首字。
// 複雜度: O(N)。
// -----------------------------------------------------------------------------
int countWordsStartingWithBroken(const std::string& text, const std::string& brokenLetters) {
    bool isBroken[128] = {false};
    for (unsigned char c : brokenLetters) isBroken[c] = true;
    int cnt = 0;
    size_t i = 0;
    while (i < text.size()) {
        // 跳過空白
        while (i < text.size() && text[i] == ' ') ++i;
        if (i >= text.size()) break;
        size_t j = i;
        while (j < text.size() && text[j] != ' ') ++j;
        std::string word = text.substr(i, j - i);
        if (!word.empty() && isBroken[static_cast<unsigned char>(word.front())]) ++cnt;
        i = j;
    }
    return cnt;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】判斷字串是否以「絕對路徑」開頭 (front == '/')
// 為何用 front: 一次呼叫即可,語意清晰且和泛型容器介面一致。
// -----------------------------------------------------------------------------
bool isAbsolutePath(const std::string& path) {
    return !path.empty() && path.front() == '/';
}

int main() {
    demoFront();
    std::cout << "\n=== LeetCode 1021 ===\n";
    std::cout << removeOuterParentheses("(()())(())") << "\n";        // "()()()"
    std::cout << removeOuterParentheses("(()())(())(()(()))") << "\n";// "()()()()(())"

    std::cout << "\n=== LeetCode 1935 變奏 ===\n";
    std::cout << countWordsStartingWithBroken("hello world", "ad") << "\n";   // 0
    std::cout << countWordsStartingWithBroken("apple banana cherry", "ab") << "\n";  // 2

    std::cout << "\n=== 日常實務: 去除 UTF-8 BOM ===\n";
    std::string withBOM = "\xEF\xBB\xBF{\"key\":\"value\"}";
    std::string clean = stripUtf8BOM(withBOM);
    std::cout << "first byte (hex): "
              << std::hex << (clean.empty() ? 0 : (unsigned)(unsigned char)clean.front())
              << std::dec << "\n";

    std::cout << "\n=== 日常實務: isAbsolutePath ===\n";
    std::cout << std::boolalpha
              << isAbsolutePath("/etc/passwd") << "\n"   // true
              << isAbsolutePath("etc/passwd")  << "\n";  // false

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1:對空字串呼叫 front() 會發生什麼事?跟 at(0) 有什麼差?
    //    A:front() 對空字串是 Undefined Behavior (可能讀到 SSO buffer 殘值
    //       或隨機資料,不保證 crash)。at(0) 對空字串會丟 std::out_of_range
    //       例外。寫安全程式碼一律先檢查 !empty()。
    //
    //  Q2:s.front() 跟 s[0] 行為一樣嗎?哪個比較推薦?
    //    A:對 non-empty 字串行為相同。但對 const 空字串,標準規定 s[0] 合法
    //       且回傳 '\0',front() 仍是 UB。語意上 front() 更明確 (「取頭」),
    //       且和其他序列容器 (vector / deque) 介面一致,寫泛型 template 才能用。
    //
    //  Q3:s.front() = 'X' 是合法的嗎?reallocation 後拿到的 reference 還能用嗎?
    //    A:non-const string 的 front() 回傳 char&,可直接寫值。但任何會觸發
    //       reallocation 的操作 (reserve、+=、insert 撐爆 capacity 等) 後,
    //       先前 front()/back() 取得的 reference 一律失效,再用就是 UB。
    //
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra front.cpp -o front

// === 預期輸出 (節錄) ===
// === LeetCode 1935 變奏 ===
// 0
// 2
// === 日常實務: isAbsolutePath ===
// true
// false
