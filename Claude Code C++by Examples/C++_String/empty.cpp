// =============================================================================
// 檔名: empty.cpp
// 主題: std::string::empty (檢查字串是否為空)
// 參考: https://en.cppreference.com/cpp/string/basic_string/empty
//       https://cplusplus.com/reference/string/string/empty/
// =============================================================================
//
// 【函式資訊 Information】
//   bool empty() const noexcept;
//   [[nodiscard]] bool empty() const noexcept;     // C++20 起加上 nodiscard
//
// 【詳細解釋 Explanation】
//
// (1) 設計理念:語意比效能更重要
//   empty() 與 size() == 0 在效能上完全等價 (都是 O(1)),為什麼還要單獨提供?
//   答案是「語意」(intent)。讀者一眼看到 if (s.empty()) 就知道在「檢查是否為空」,
//   不必停下來想「為什麼要把 size() 跟 0 比較」。這也是所有 STL 容器
//   (vector, list, map, set, queue, ...) 都統一提供 empty() 的原因 —
//   讓泛型程式碼能用同一個動詞表達同一件事。
//
// (2) 為什麼不能用 empty() 來「清空」?
//   有些語言 (Ruby、JS) 的方法名 empty 是動詞 (清空),C++ 的 empty 是
//   形容詞 (是否為空)。寫 s.empty(); 是回傳 bool 但結果被丟掉,
//   字串本身不會改變。要清空請用 s.clear()。C++20 的 [[nodiscard]] 屬性
//   就是為了攔截這個常見誤用。
//
// (3) 實作機制
//   多數實作就是 return _size == 0; — 一個比對指令。
//   不需要計算長度、不需要走訪內容、絕對 O(1)。
//   注意:對 SSO (短字串) 與 heap-allocated 字串都同樣 O(1),
//   因為 size 永遠存在 string 物件本身。
//
// (4) 與其他容器一致的 contract
//   STL 全體容器都遵守:
//     c.empty() ⇔ c.begin() == c.end() ⇔ c.size() == 0
//   這三個說法是等價的,差別只在效能與語意。
//   - size():要回傳數字,所有容器 O(1) (C++11 起 list 也是 O(1))。
//   - empty():只回傳 bool,某些情況可能比 size() 更快 (例如 forward_list
//     的 size() 是 O(n),但 empty() 是 O(1))。
//   - begin() == end():最低階的判斷,但寫起來囉嗦。
//   實務上請永遠用 empty()。
//
// (5) 歷史背景
//   - C++98: bool empty() const;
//   - C++11: 加上 noexcept (永遠不丟例外)
//   - C++17: STL 容器全面標 [[nodiscard]] 是主流提案,但實際併入是
//   - C++20: 標準正式為所有容器的 empty() 加上 [[nodiscard]]
//   呼叫卻不使用結果會收到編譯警告 (clang/gcc/MSVC 都會報)。
//
// (6) 複雜度與例外安全
//   時間 O(1)、空間 O(0)、noexcept、不會修改字串。
//   可在 const 物件上呼叫。
//
// 【注意事項 Pay Attention】
// 1. C++20 起標記 [[nodiscard]],呼叫 empty() 卻不使用結果會收到警告。
//    避免常見錯誤:寫 s.empty(); 卻誤以為「清空字串」 — 那是 s.clear()!
// 2. 對於極短字串,empty() 與 size()==0 同樣是 O(1),選擇前者只為可讀性。
// 3. 對「空字串呼叫 front/back/pop_back」是 UB,empty() 是進入這些操作前
//    的標準守門員 (sentinel check)。
// 4. empty() 不檢查內容是否為「空白字元」,只檢查長度。"   " 不是 empty。
//    若要檢查是否「全空白」需自己寫 (std::all_of + std::isspace)。
// 5. empty() 也不檢查是否含 '\0'。"\0\0" (size=2) 不是 empty。
//
// 【概念補充 Concept Deep Dive】
//
// ★ empty() vs size() == 0:在泛型場合的差異
//   對於 std::string、std::vector,兩者完全等價。但對 std::forward_list 來說:
//     - size() 是 O(n) (因為單向鏈結串列要走完才知道長度)
//     - empty() 是 O(1)
//   所以教學書都建議「永遠用 empty()」,在重構時若改換容器型別才不會被坑。
//
// ★ [[nodiscard]] 的真正威力
//   [[nodiscard]] 不只是警告誤用,還引導 API 設計者思考函式語意:
//   「我的回傳值是否關鍵到不該被忽略?」
//   empty()、find()、try_emplace() 等查詢類函式都是。
//   反過來,push_back、erase 等動作類函式不會標 nodiscard,因為使用者
//   經常不在乎回傳的 iterator。
//
// ★ 與 std::optional / 空字串的設計取捨
//   API 設計時,「沒有值」可以用 empty string 或 std::optional<string> 表達。
//   - empty string:省記憶體、語意明確、但無法區分「沒設定」與「設成空字串」
//   - optional<string>:可以區分,但多一層 overhead 與語法
//   常見原則:若空字串本身就是合法值 (使用者名稱可空),用 optional;
//   若空字串就是「不存在」,empty() 已足夠。
//
// ★ noexcept 的意義
//   empty() 是 noexcept 表示:它絕不會丟例外。這對 C++ 編譯器極重要 —
//   noexcept 函式可被放在 std::vector 的 move 操作中、可成為其他 noexcept
//   函式的呼叫對象、可在 destructor 中安全使用。
//
// =============================================================================

/*
補充筆記：std::string::empty
  - std::string::empty 需要同時考慮內容、容量與舊指標失效；字串 API 常會配置或搬移緩衝區。
  - 回傳位置的函式要先處理 npos，回傳 pointer/view 的函式要追蹤來源 string 生命週期。
  - UTF-8 文字下，size 代表位元組數，不代表使用者看到的字元數。
  - std::string::empty 是 std::string 主題的一部分；先分清楚這個操作會讀取、追加、插入、刪除、搜尋，還是只暴露底層字元。
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

void demoEmpty() {
    std::string a;
    std::string b = "Hi";
    std::cout << std::boolalpha;
    std::cout << "a.empty() = " << a.empty() << "\n";
    std::cout << "b.empty() = " << b.empty() << "\n";

    // 常見錯誤:很多人會誤寫 a.empty(); 想要清空,正確是 a.clear();
    // 含空白的字串不算 empty
    std::string spaces = "   ";
    std::cout << "spaces.empty() = " << spaces.empty() << "\n"; // false
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 14. Longest Common Prefix
// 題目: 找一組字串的最長共同前綴。
// 為何用 empty: 邊界檢查 — 輸入空陣列、空字串時直接回傳。
//               迴圈中 prefix 縮短到空時也用 empty() 提早退出,語意清楚。
// 解題思路: 以第一個字串為候選 prefix,逐字串縮短到對方都符合為止。
// 複雜度: 時間 O(S),S 為所有字元數總和;空間 O(1) (回傳除外)。
// -----------------------------------------------------------------------------
std::string longestCommonPrefix(const std::vector<std::string>& strs) {
    if (strs.empty()) return "";

    std::string prefix = strs[0];
    for (size_t i = 1; i < strs.size(); ++i) {
        // 縮短 prefix 直到它是 strs[i] 的前綴
        while (strs[i].compare(0, prefix.size(), prefix) != 0) {
            prefix.pop_back();
            if (prefix.empty()) return "";
        }
    }
    return prefix;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】輸入驗證 - 必填欄位檢查
// 為何用 empty: API endpoint / form validation 第一步就是檢查必填字串。
//               比 size() == 0 語意更清楚,且配合 [[nodiscard]] 編譯器會警告誤用。
//               在 web 後端、CLI 工具、設定檔解析中,這是第一道防線。
// -----------------------------------------------------------------------------
struct ValidationError { std::string field; std::string message; };

std::vector<ValidationError> validateUserInput(
    const std::string& name,
    const std::string& email,
    const std::string& password) {
    std::vector<ValidationError> errors;
    if (name.empty())     errors.push_back({"name",     "name is required"});
    if (email.empty())    errors.push_back({"email",    "email is required"});
    if (password.empty()) errors.push_back({"password", "password is required"});
    else if (password.size() < 8)
                          errors.push_back({"password", "password too short"});
    return errors;
}

// -----------------------------------------------------------------------------
// 【LeetCode 範例 2】LeetCode 2114. Maximum Number of Words Found in Sentences
// 題目: 給一組句子 (每句以單一空白分隔字),回傳所有句子中單字數的最大值。
// 為何用 empty: 計算單字數時,用 split + 排除 empty token,空字串檢查不可少。
// 複雜度: O(總長度)。
// -----------------------------------------------------------------------------
int mostWordsFound(const std::vector<std::string>& sentences) {
    int mx = 0;
    for (const auto& s : sentences) {
        if (s.empty()) continue;            // 空句直接跳過
        int cnt = 1;
        for (char c : s) if (c == ' ') ++cnt;
        if (cnt > mx) mx = cnt;
    }
    return mx;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】HTTP server 的 fallback 機制
// 為何用 empty: 從多個來源讀設定 (env > config file > default),用 empty()
//                從上層往下找第一個非空值,標準的 fallback chain pattern。
// -----------------------------------------------------------------------------
std::string resolveConfigValue(const std::string& fromEnv,
                                const std::string& fromFile,
                                const std::string& defaultValue) {
    if (!fromEnv.empty())  return fromEnv;
    if (!fromFile.empty()) return fromFile;
    return defaultValue;
}

int main() {
    demoEmpty();
    std::cout << "\n=== LeetCode 14 ===\n";
    std::cout << "\"" << longestCommonPrefix({"flower","flow","flight"}) << "\"\n"; // "fl"
    std::cout << "\"" << longestCommonPrefix({"dog","racecar","car"})    << "\"\n"; // ""

    std::cout << "\n=== LeetCode 2114 ===\n";
    std::cout << mostWordsFound({"alice and bob love leetcode",
                                  "i think so too",
                                  "this is great thanks very much"}) << "\n";  // 6

    std::cout << "\n=== 日常實務: 必填欄位驗證 ===\n";
    auto errs = validateUserInput("Alice", "", "abc");
    for (auto& e : errs) std::cout << e.field << ": " << e.message << "\n";

    std::cout << "\n=== 日常實務: config fallback ===\n";
    std::cout << resolveConfigValue("",        "from_file", "default") << "\n";  // from_file
    std::cout << resolveConfigValue("from_env", "from_file","default") << "\n";  // from_env
    std::cout << resolveConfigValue("",        "",          "default") << "\n";  // default

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：empty() 與 size() == 0 有效能差別嗎?為什麼還要提供 empty()?
    //    A：對 std::string 完全沒有 ── 兩者都是 O(1) 一條比較指令。提供
    //       empty() 純粹為了語意 (intent) 與一致性:所有 STL 容器
    //       (vector, list, map, set, queue...) 都有 empty(),泛型程式碼可
    //       用同一個動詞表達同一件事。對 std::forward_list 等 size() 是
    //       O(N) 的容器,empty() 才是 O(1) 的唯一選擇。
    //
    //  Q2：C++20 為什麼把 empty() 標 [[nodiscard]]?
    //    A：因為 empty 在自然語言中是動詞 (清空),很多人 (尤其從 Ruby/JS
    //       來的) 會誤寫 s.empty(); 以為清空了字串,實際只是回傳 bool 並
    //       丟掉。[[nodiscard]] 讓這類誤用觸發編譯警告,提醒應該用
    //       s.clear() 才是真的清空。
    //
    //  Q3：empty() 對 SSO 字串與 heap 字串行為相同嗎?
    //    A：是,行為與效能完全一致。因為 size 永遠存在 string 物件本身
    //       (即使是 SSO 短字串,size 也存在 inline metadata 裡),不需要
    //       走訪 buffer 或解參考 heap pointer。empty() 一律 O(1) noexcept。
    //
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra empty.cpp -o empty

// === 預期輸出 (節錄) ===
// === LeetCode 2114 ===
// 6
// === 日常實務: config fallback ===
// from_file
// from_env
// default
