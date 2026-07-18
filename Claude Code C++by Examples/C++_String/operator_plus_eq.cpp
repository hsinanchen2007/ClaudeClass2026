// =============================================================================
// 檔名: operator_plus_eq.cpp
// 主題: std::string::operator+= (附加運算子)
// 參考:
//   - https://en.cppreference.com/cpp/string/basic_string/operator%2B%3D
//   - https://cplusplus.com/reference/string/string/operator+=/
// =============================================================================
//
// 【函式資訊 Information】
//   string& operator+=(const string& str);
//   string& operator+=(char ch);
//   string& operator+=(const char* s);
//   string& operator+=(std::initializer_list<char> ilist);
//   string& operator+=(std::string_view sv);          // C++17
//
// 【詳細解釋 Explanation】
//
// 一、本質就是 append 的「語法糖」
//   operator+= 是寫起來最像「自然語言」的附加方式,內部實作就是直接呼叫
//   append。它存在的原因是:
//     - 視覺上接近數學表達式 (s = s + x 的優化版)。
//     - 與內建型別 (int 的 +=) 操作習慣一致。
//   但提供的重載只是 append 的「子集」:沒有 substring、沒有 iterator range、
//   沒有 (count, ch) 重複字元版本。
//
// 二、operator+= 與 operator+ 的關鍵差異 (面試常問)
//   operator+ 是「非成員函式」,語法是 s1 + s2;
//   它必須產生「全新字串」── 配置記憶體、拷貝兩次、回傳暫時物件。
//   operator+= 是「成員函式」,直接在 *this 後面塞,不會建立暫時物件。
//
//   迴圈中差異極大:
//     // 壞:O(N^2) — 每次 + 都建新字串
//     std::string r;
//     for (auto& w : words) r = r + w;
//
//     // 好:O(N) 攤銷 — 直接在尾端 append
//     std::string r;
//     for (auto& w : words) r += w;
//
//   現代編譯器配合 RVO/move,operator+ 的成本已大幅下降,但「迴圈累積」
//   還是必須用 +=,因為左側不是 rvalue,優化吃不到。
//
// 三、與 push_back 的對應
//   s += 'x' 等價於 s.push_back('x');都是「在尾端加單一字元」。
//   兩者效能相同,選用以可讀性為準。
//
// 四、版本演進
//   - C++11 : 加入 initializer_list、noexcept、move semantics 配合。
//   - C++17 : 加入 std::string_view 重載 (不再需要 const char* 一定要 null-terminated)。
//   - C++20 : 加入 constexpr。
//
// 時間複雜度: O(M),M 是新增內容長度。攤銷下接近 O(M)。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為何回傳 string& 而非 void?
//   為了支援「鏈式 (chained)」運算: (s += "a") += "b";
//   但比起鏈式,更實際的用途是當作「if (cond) s += x; return s;」
//   或在 return s += "trailer"; 中省一行。
//
// (B) 與 operator<< (字串流) 的對比
//   字串流 std::ostringstream 也能做「累積拼接」,內部會緩衝後再轉 string。
//   它的優點:支援自動格式化 (數字、浮點數、setw、setprecision...)。
//   它的缺點:額外的 streambuf 物件、虛函式呼叫、locale 處理 → 比 += 慢數倍。
//   只接純字串時優先 +=;需要格式化時才用 ostringstream 或 std::format (C++20)。
//
// (C) 為什麼沒有 operator-= 或 operator*=?
//   字串「減去」、「乘上」沒有自然語意定義。Python 雖然有 s * 3,
//   但 C++ 標準不提供,要用 string(3, 'a') 或 append(3, 'a') 達成。
//
// 【注意事項 Pay Attention】
// 1. operator+= 回傳 *this,可鏈式: (s += "a") += "b";
//    但通常不這樣寫(可讀性差),改 s += "ab"。
// 2. 與 operator+ 不同:operator+ 是非成員函式,會建立新字串(可能很慢);
//    operator+= 是成員函式,直接修改原字串(較快)。
//    迴圈中拼接優先用 operator+=。
// 3. 大量拼接前 reserve(預估大小)避免 realloc。
// 4. operator+= 不接受 (count, ch) 等多參形式,要用就改呼叫 append。
// 5. 與 push_back 一樣,單字元附加可能比一次塞短字串還省 (避免 strlen)。
//
// =============================================================================

/*
補充筆記：std::string::operator_plus_eq
  - std::string::operator_plus_eq 需要同時考慮內容、容量與舊指標失效；字串 API 常會配置或搬移緩衝區。
  - 回傳位置的函式要先處理 npos，回傳 pointer/view 的函式要追蹤來源 string 生命週期。
  - UTF-8 文字下，size 代表位元組數，不代表使用者看到的字元數。
  - std::string::operator_plus_eq 是 std::string 主題的一部分；先分清楚這個操作會讀取、追加、插入、刪除、搜尋，還是只暴露底層字元。
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
#include <vector>

void demoPlusEq() {
    std::string s = "Hello";
    s += ", ";              // const char*
    s += std::string("World");
    s += '!';               // single char
    std::cout << s << "\n"; // "Hello, World!"
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1614. Maximum Nesting Depth of the Parentheses
// 題目: 求括號字串的最大巢狀深度。
// 為何用 +=: 雖然這題本身不需要 +=,但我們示範把 trace 過程記錄到字串裡。
// -----------------------------------------------------------------------------
int maxDepth(const std::string& s, std::string* trace = nullptr) {
    int cur = 0, best = 0;
    for (char c : s) {
        if (c == '(') {
            ++cur;
            best = std::max(best, cur);
            if (trace) *trace += '+';
        } else if (c == ')') {
            --cur;
            if (trace) *trace += '-';
        }
    }
    return best;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 2】LeetCode 657. Robot Return to Origin
// 題目: 給移動指令字串 (UDLR),判斷機器人是否回到原點。
// 為何用 +=: 示範用 string += 紀錄走過路線,並順便數方向。
// -----------------------------------------------------------------------------
bool judgeCircle(const std::string& moves) {
    int x = 0, y = 0;
    for (char c : moves) {
        if      (c == 'U') ++y;
        else if (c == 'D') --y;
        else if (c == 'L') --x;
        else if (c == 'R') ++x;
    }
    return x == 0 && y == 0;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】用 += 累積錯誤訊息(逐一收集多個失敗原因)
// 為何用 +=: 表單驗證 / 配置檢查時,常需要列出「所有」錯誤而非首次失敗就退出。
//             += 在迴圈中累積最直觀,且不會反覆建立暫時物件。
// -----------------------------------------------------------------------------
std::string accumulateErrors(const std::vector<std::string>& errs) {
    std::string out;
    for (size_t i = 0; i < errs.size(); ++i) {
        if (i > 0) out += "; ";
        out += '#';
        out += std::to_string(i + 1);
        out += ' ';
        out += errs[i];
    }
    return out;
}

#include <algorithm>

// -----------------------------------------------------------------------------
// 【LeetCode 範例 3】LeetCode 1page. 1page. Replace All Digits with Characters
// 題目: LeetCode 1844. Replace All Digits with Characters
// 給字串 s,偶數位是字母、奇數位是數字 d;把 s[2k+1] 替換為 s[2k] + d 對應的字母。
// 為何用 +=: 用 += 在迴圈中累積結果,展示「逐字構建字串」最常見場景。
// 複雜度: O(N)。
// -----------------------------------------------------------------------------
std::string replaceDigits(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i) {
        if (i % 2 == 0) out += s[i];
        else {
            int d = s[i] - '0';
            out += static_cast<char>(s[i - 1] + d);
        }
    }
    return out;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】把 vector<int> 序列化為 "[1, 2, 3]" 字串
// 為何用 +=: 一段一段累積,可讀性最高,且事先 reserve 後完全沒有 realloc。
// -----------------------------------------------------------------------------
std::string toJsonArray(const std::vector<int>& v) {
    std::string out;
    out.reserve(v.size() * 4 + 2);
    out += '[';
    for (size_t i = 0; i < v.size(); ++i) {
        if (i > 0) out += ", ";
        out += std::to_string(v[i]);
    }
    out += ']';
    return out;
}

int main() {
    demoPlusEq();

    std::cout << "\n=== LeetCode 1614 ===\n";
    std::string trace;
    std::cout << maxDepth("(1+(2*3)+((8)/4))+1", &trace) << "\n";  // 3
    std::cout << "trace: " << trace << "\n";

    std::cout << "\n=== LeetCode 657 ===\n";
    std::cout << std::boolalpha << judgeCircle("UDLR") << "\n";    // true
    std::cout << std::boolalpha << judgeCircle("LL")   << "\n";    // false

    std::cout << "\n=== LeetCode 1844 ===\n";
    std::cout << replaceDigits("a1c1e1") << "\n";   // abcdef
    std::cout << replaceDigits("a1b2c3d4e") << "\n";// abbdcfdhe

    std::cout << "\n=== 日常實務: 累積錯誤 ===\n";
    std::cout << accumulateErrors({"name missing", "email invalid", "age out of range"}) << "\n";

    std::cout << "\n=== 日常實務: toJsonArray ===\n";
    std::cout << toJsonArray({1, 2, 30, 400, 5000}) << "\n";   // [1, 2, 30, 400, 5000]

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1:operator+= 跟 append 究竟差在哪?該怎麼選?
    //    A:operator+= 只是 append 的「常用子集語法糖」。+= 的重載少
    //       (string、char、const char*、initializer_list、string_view),
    //       而 append 還支援 (count, ch)、iterator range、(s, pos, count) 等。
    //       單純尾端串接用 += 可讀;要部分 substring / 迭代器範圍才需 append。
    //
    //  Q2:s += 'x' 跟 s.push_back('x') 哪個比較快?
    //    A:標準語意完全等價,效能也相同 (都是 O(1) 攤銷,不需 strlen)。
    //       選用以可讀性為準。一般「附加單字元並可讀為流」用 += '\n',
    //       「容器尾端追加元素」語意則用 push_back。
    //
    //  Q3:operator+= 觸發 reallocation 後,先前 c_str() / data() / 迭代器
    //       還能用嗎?
    //    A:不能,全部失效。+= 的失效規則跟 push_back / append / insert 一樣 —
    //       只要 reallocation 就全部失效,沒 reallocation 就 end() 失效。
    //       已知總長度建議先 reserve(N) 預配置,避免迴圈中反覆 reallocation
    //       讓效能從 O(N) amortized 降為 O(N²) (拷貝累加)。
    //
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra operator_plus_eq.cpp -o operator_plus_eq

// === 預期輸出 (節錄) ===
// === LeetCode 1844 ===
// abcdef
// abbdcfdhe
// === 日常實務: toJsonArray ===
// [1, 2, 30, 400, 5000]
