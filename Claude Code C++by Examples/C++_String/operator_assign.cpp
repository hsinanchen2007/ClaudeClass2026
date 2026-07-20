// =============================================================================
// 檔名: operator_assign.cpp
// 主題: std::string::operator= (賦值運算子)
// 參考連結 References:
//   cppreference: https://en.cppreference.com/cpp/string/basic_string/operator%3D
//   cplusplus.com: https://cplusplus.com/reference/string/string/operator=/
// =============================================================================
//
// 【函式資訊 Information】
//   string& operator=(const string& other);            // (1) 複製賦值
//   string& operator=(string&& other) noexcept;        // (2) 移動賦值 C++11
//   string& operator=(const char* s);                  // (3) 從 C-string 賦值
//   string& operator=(char ch);                        // (4) 從單一字元賦值
//   string& operator=(std::initializer_list<char>);    // (5) 初始化列表 C++11
//   string& operator=(std::string_view sv);            // (6) C++17
//
// 與 assign() 函式的差異:
//   - operator= 較簡潔,但只支援上述幾種輸入。
//   - assign() 提供更多 overload (例如 assign(count, ch)、assign(it, it)、
//     assign(s, pos, count)),適合「需要部分賦值」的情境。
//
// =============================================================================
//
// 【詳細解釋 Explanation】
//
// (一) 賦值運算子做了什麼?
// ----------------------------------------------------------------------------
// 當你執行 a = b; 時,std::string 的賦值運算子會:
//   1. 先處理 self-assignment:檢查 &a == &b,若是則直接 return *this。
//      (有些實作用 copy-and-swap idiom 也能自然處理 self-assignment)
//   2. 比較 b.size() 與 a.capacity():
//      - 若 a 的緩衝區夠大 → 直接覆蓋,不重新配置 (重要的最佳化!)
//      - 若 a 的緩衝區不夠 → 釋放舊 buffer,配置新 buffer 後複製
//   3. 將 size 更新為 b.size(),並在末端寫入 '\0'。
//   4. 回傳 *this,允許鏈式賦值 a = b = c。
//
// 移動賦值 (2) 的差異:
//   - 直接「偷走」other 的內部指標、size、capacity,把自己原本的釋放掉。
//   - 是 noexcept 操作 (除非 allocator 不允許 propagate_on_container_move
//     且 allocator 不相等,但這是極罕見的情境)。
//   - 被 move 的物件處於 valid-but-unspecified 狀態。
//
// (二) 鏈式賦值 (Chained Assignment)
// ----------------------------------------------------------------------------
// 因為回傳 *this (string&),所以可以寫:
//   a = b = c = "hello";
// 這個式子的求值順序為從右到左:
//   c = "hello" → 回傳 c → b = c → 回傳 b → a = b
// 內建型別也是這樣設計,std::string 沿用這個慣例,讓使用者不需要記特例。
//
// (三) 為何 operator=(char) 把 string 的 size 變成 1?
// ----------------------------------------------------------------------------
//   std::string s = "Hello";
//   s = 'X';                  // s 變成 "X",size() == 1
// 這常讓 C 程式設計師驚訝。因為這個 overload 的語意是「重新指派為單字元字串」,
// 而非「把第一個字元改成 X」(後者要用 s[0] = 'X' 或 s.front() = 'X')。
// 設計上此 overload 主要是為了讓 std::string s; s = 'A'; 這種快速建立
// 單字元字串的寫法可行。
//
// (四) 與 assign 的選擇原則
// ----------------------------------------------------------------------------
//   - 簡單覆蓋 (整個換掉)        → operator=
//   - 需要 substring 範圍賦值     → assign(s, pos, count)
//   - 需要從迭代器範圍賦值        → assign(it1, it2)
//   - 需要 fill 多個字元賦值      → assign(count, ch)
//
// (五) Capacity 不會自動縮小
// ----------------------------------------------------------------------------
// s = "hi"; (size 2)  之後  s = "this is a longer string"; (size 23)
// 第二次賦值時 capacity 可能會擴張到 23+ ;但若再來一次 s = "x"; ,
// 雖然 size 變回 1,capacity 仍維持 23+。要真的釋放空間需呼叫
// shrink_to_fit() 或先 swap 一個新的小 string。
//
// (六) Iterator / Pointer / Reference 全部失效
// ----------------------------------------------------------------------------
// 不論是複製賦值還是移動賦值,先前透過 begin()、data()、operator[]、
// at()、front()、back() 取得的指標 / 參考 / 迭代器,在賦值後都失效。
// 在多執行緒環境或回呼設計中,務必特別小心這個失效規則。
//
// (七) C++ 標準演進
// ----------------------------------------------------------------------------
//   C++03    : 提供 (1)(3)(4)
//   C++11    : 新增 (2) 移動賦值、(5) 初始化列表
//   C++17    : 新增從 std::string_view (透過隱式轉換) 的賦值
//   C++23    : operator=(std::nullptr_t) 明確 = delete
//
// =============================================================================
//
// 【概念補充 Concept Deep Dive】
//
// 1) Copy-and-Swap Idiom
//    經典的賦值運算子實作技巧:
//      string& operator=(string other) {  // by value (利用 copy ctor)
//          swap(*this, other);            // 與本身交換
//          return *this;                  // other 在 scope 結束時釋放舊資料
//      }
//    優點:自然處理 self-assignment、提供 strong exception safety。
//    缺點:即便 buffer 夠大也一定會重新配置,效能略差。
//    現代 STL 多採取「複用 buffer」的最佳化,而非純粹的 copy-and-swap。
//
// 2) Strong Exception Safety
//    一個強例外安全的賦值保證:若操作中途丟例外,被賦值的物件保持原狀
//    (好像什麼都沒發生)。std::string 的複製賦值在 C++ 中通常達到
//    basic guarantee (不洩漏資源、不破壞不變量),但 strong guarantee
//    並非標準強制要求。
//
// 3) 三/五法則 (Rule of Three / Five)
//    若一個 class 自訂了 dtor、copy ctor、copy assignment,通常三者都要
//    自訂 (Rule of Three);C++11 加入 move 後,變成 Rule of Five
//    (再加 move ctor、move assignment)。std::string 是這個規則的標準
//    教科書範例 — 五個都有提供。
//
// 4) noexcept 移動賦值的重要性
//    如果 move assignment 不是 noexcept,std::vector<std::string> 在
//    reallocation 時會選擇 copy 而非 move,效能大幅下降。所以標準明文
//    要求 string 的移動操作都必須 noexcept。
//
// 5) Self-assignment 為何要小心?
//    在「先 release 後 copy」的天真實作中:
//        delete[] data; data = new char[other.size]; copy(...);
//    若 other == *this,delete 後 other.data 已是 dangling pointer,
//    再讀取 other.size 與內容就是 UB。STL 已處理好,但自己寫類似
//    container 時要記得這個雷。
//
// =============================================================================
//
// 【注意事項 Pay Attention】
// 1. operator=(nullptr) 是 Undefined Behavior (C++23 起標準明確 = delete)。
// 2. 移動賦值後,被移動的物件處於 valid-but-unspecified 狀態。
// 3. 賦值會使原本所有 iterator/pointer/reference 全部失效。
// 4. 若新內容比舊內容短,通常不會自動縮小 capacity (需 shrink_to_fit)。
// 5. 例外安全:複製賦值若配置失敗會丟 bad_alloc;移動賦值是 noexcept。
// 6. operator=(char) 會讓 size() 變成 1,不是「修改第 0 個字元」。
//
// =============================================================================

/*
補充筆記：std::string::operator_assign
  - std::string::operator_assign 需要同時考慮內容、容量與舊指標失效；字串 API 常會配置或搬移緩衝區。
  - 回傳位置的函式要先處理 npos，回傳 pointer/view 的函式要追蹤來源 string 生命週期。
  - UTF-8 文字下，size 代表位元組數，不代表使用者看到的字元數。
  - std::string::operator_assign 是 std::string 主題的一部分；先分清楚這個操作會讀取、追加、插入、刪除、搜尋，還是只暴露底層字元。
  - std::string 的索引型別是 size_type；和 int 混用時要小心 unsigned underflow，尤其是倒著迴圈或拿 npos 比較。
  - 修改字串內容或容量可能讓先前取得的 iterator、reference、pointer、c_str() 指標失效。
  - operator[] 不做範圍檢查，at() 越界會丟 std::out_of_range；教學範例可比較兩者，工作程式要依錯誤處理需求選。
  - find/rfind/find_first_of 這類搜尋函式失敗回傳 std::string::npos；判斷時應和 npos 比，不要寫成 >= 0。
  - 字串和 C string 互通時要記得 null terminator；std::string 可以保存內含 \0 的資料，但很多 C API 會在第一個 \0 停止。
  - 若只是傳入唯讀文字片段且不需要擁有資料，std::string_view 可避免複製；但它不能延長原字串生命週期。
  - 處理中文或 UTF-8 時，std::string 的 size() 回傳 byte 數，不是人眼看到的字元數。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::string 的 operator=(含 move assignment)
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. s2 = std::move(s1); 之後,s1 是什麼狀態?
//     答:「valid but unspecified」——s1 仍是合法物件,可以安全解構、重新
//         賦值、呼叫 clear() 等沒有前置條件的操作;但標準「不保證」它變成
//         空字串。多數實作確實會清空,但那是實作行為,寫程式不可依賴。
//     追問:那 assert(s1.empty()) 會過嗎?→ 實務上常常會過,但這正是最
//           危險的地方:它是未指定行為,換編譯器或版本就可能不成立。
//
// 🔥 Q2. move assignment 一定比 copy assignment 快嗎?一定是 O(1) 嗎?
//     答:不一定是 O(1)。長字串(heap 模式)只要接管 pointer/size/capacity
//         → O(1);但短字串走 SSO 時資料就在物件本體內,必須真的 memcpy
//         內嵌 buffer,是有常數上界的複製。短字串的 move 幾乎沒有優勢。
//     追問:SSO 門檻多少?→ libstdc++ 為 15 個字元,但這是
//           implementation-defined,標準沒規定任何數字。
//
// 🔥 Q3. 為什麼 std::string 的 move 是不是 noexcept 對 vector 很重要?
//     答:vector 擴容時會用 move_if_noexcept:元素的 move constructor 是
//         noexcept 才敢用 move 搬遷,否則退回 copy,才能維持 strong
//         exception guarantee(搬到一半失敗時舊 buffer 還完好)。
//         ⚠️ 精確講法:「退回 copy」只在【該型別可 copy】時成立;move-only 型別
//         沒有 copy 可退,即使 move 未標 noexcept 仍會 move,此時就沒有強保證。
//         std::string 的 move ctor 是 noexcept,所以走的是最好的那條路。
//         std::string 的 move constructor 標準要求為 noexcept,所以
//         vector<string> 擴容能走 move 而不是逐一複製。
//     追問:move assignment 也是無條件 noexcept 嗎?→ 不是,它是條件式
//           noexcept(取決於 allocator 的 propagate / is_always_equal),
//           對預設的 std::allocator 而言成立。
//
// ⚠️ 陷阱. std::string s = "hello"; s = 'x'; 之後 s 是什麼?
//     答:s 變成長度 1 的字串 "x",不是「把第 0 個字元改成 'x'」。
//         operator=(char) 是整個內容的覆蓋賦值,原本的 "hello" 全沒了。
//         要改單一字元請用 s[0] = 'x'(需先確保 size() > 0)。
//     為什麼會錯:看到 = 'x' 直覺聯想成「指派一個字元進去」,忽略了
//         operator= 的語意一律是「用右邊的東西取代整個字串內容」。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <utility>

void demoAssign() {
    std::cout << "=== operator= 示範 ===\n";

    std::string a = "Hello";
    std::string b;

    // (1) 複製賦值
    b = a;
    std::cout << "(1) copy   : a=\"" << a << "\", b=\"" << b << "\"\n";

    // (2) 移動賦值
    std::string c;
    c = std::move(a);
    std::cout << "(2) move   : c=\"" << c << "\" (a 已被移動,內容未指定)\n";

    // (3) 從 C-string
    b = "World";
    std::cout << "(3) cstr   : b=\"" << b << "\"\n";

    // (4) 從單一字元 → 字串長度變為 1
    b = 'X';
    std::cout << "(4) char   : b=\"" << b << "\", size=" << b.size() << "\n";

    // (5) 初始化列表
    b = {'F', 'o', 'o'};
    std::cout << "(5) ilist  : b=\"" << b << "\"\n";

    // 鏈式賦值
    std::string x, y, z;
    x = y = z = "chain";
    std::cout << "chain      : " << x << ", " << y << ", " << z << "\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 範例】LeetCode 1768. Merge Strings Alternately (Easy)
//
// 題目敘述:
//   給兩個字串 word1, word2,把它們字元交錯合併成一個新字串。
//   若一邊比較長,把剩下的部分整段接到結尾。
//   範例: word1="abc", word2="pqr"  → "apbqcr"
//        word1="ab",  word2="pqrs" → "apbqrs"
//
// 為何用 operator=:
//   題目最後要把組合好的字串「賦值回」result 回傳;每一輪迴圈我們也用
//   operator+= 累積結果,展示 = 與 += 的搭配。對 std::string 而言,
//   operator= 才是初始化長字串值最自然的途徑。
//
// 解題思路:
//   雙指標 i, j 各自走 word1 與 word2,輪流取字元串接。
//   迴圈結束後若一邊還有剩,直接 += 剩餘部分。
//
// 複雜度: 時間 O(n + m),空間 O(n + m)
// -----------------------------------------------------------------------------
std::string mergeAlternately(const std::string& word1, const std::string& word2) {
    std::string result;
    result.reserve(word1.size() + word2.size());

    size_t i = 0, j = 0;
    while (i < word1.size() && j < word2.size()) {
        result += word1[i++];
        result += word2[j++];
    }
    // 把剩餘的接上
    if (i < word1.size()) result += word1.substr(i);
    if (j < word2.size()) result += word2.substr(j);

    return result;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 Daily Work】重置 session token / 機敏資料
//
// 為何用 operator=:
//   後端在 logout / token 過期時,要把記憶體中的 token 替換掉以降低
//   leak 風險。直接 secret = "" 雖然會把 size 歸零,但 capacity 不變,
//   底層 buffer 還留著舊資料。實務上常見的「擦除手法」:
//     1. 先 operator=(string(size, '\0')) 把 buffer 內容覆蓋掉。
//     2. 再 operator=("") 清空長度。
//     3. 最後 shrink_to_fit() 釋放底層 buffer。
//
// 注意:此寫法雖比較安全,但編譯器最佳化仍可能把覆寫 dead-code 消除。
// 真正的安全擦除請用平台 API (例如 SecureZeroMemory)。
// -----------------------------------------------------------------------------
void resetSecret(std::string& secret) {
    secret = std::string(secret.size(), '\0');  // 先以 '\0' 覆蓋舊內容
    secret = "";                                // 再清空 size
    secret.shrink_to_fit();                     // 釋放底層緩衝
}

// -----------------------------------------------------------------------------
// 【LeetCode 範例 2】LeetCode 1location0. 1location0. Maximum Repeating Substring
// 題目: LeetCode 1668. Maximum Repeating Substring
// 給 sequence 與 word,找最大 k 使得 word 重複 k 次仍為 sequence 的子字串。
// 為何用 operator=: 我們在迴圈中反覆把 candidate 用 += 累積、用 = 替換,展示
//                   string 賦值在動態建構過程的角色。
// 複雜度: O(N * k) (k 通常很小)。
// -----------------------------------------------------------------------------
int maxRepeating(const std::string& sequence, const std::string& word) {
    int k = 0;
    std::string candidate = word;
    while (sequence.find(candidate) != std::string::npos) {
        ++k;
        candidate = word;            // 重置為 word
        for (int i = 0; i < k; ++i)  // 再連續加上 k 次
            candidate += word;
    }
    return k;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】用 move-assignment 接收 builder 的最終結果
// 為何用 operator=(string&&): 把 builder 製造的 string 一次搬給目標,免拷貝。
// -----------------------------------------------------------------------------
std::string buildAndMove() {
    std::string sink;
    {
        std::string tmp;
        tmp.reserve(64);
        for (int i = 0; i < 5; ++i) tmp += "abc";
        sink = std::move(tmp);     // move-assignment,tmp 之後不再使用
    }
    return sink;
}

int main() {
    demoAssign();

    std::cout << "\n=== LeetCode 1768 ===\n";
    std::cout << mergeAlternately("abc", "pqr")   << "\n";   // apbqcr
    std::cout << mergeAlternately("ab",  "pqrs")  << "\n";   // apbqrs

    std::cout << "\n=== LeetCode 1668 ===\n";
    std::cout << maxRepeating("ababc",   "ab")  << "\n";    // 2
    std::cout << maxRepeating("ababc",   "ba")  << "\n";    // 1
    std::cout << maxRepeating("ababc",   "ac")  << "\n";    // 0

    std::cout << "\n=== 日常實務: 重置敏感資料 ===\n";
    std::string token = "abcd1234SECRET";
    resetSecret(token);
    std::cout << "after reset: size=" << token.size()
              << ", capacity=" << token.capacity() << "\n";

    std::cout << "\n=== 日常實務: move-assignment ===\n";
    std::cout << buildAndMove() << "\n";   // abcabcabcabcabc

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1:s = 'X' 為什麼會讓 size 變成 1 而不是「修改第 0 個字元」?
    //    A:operator=(char) 的語意是「重新指派為單字元字串」,等同
    //       s.assign(1, 'X')。這是設計者讓 std::string s; s = 'A'; 能成立
    //       的便利重載。要修改第 0 字元需 s[0] = 'X' 或 s.front() = 'X'。
    //
    //  Q2:move assignment 之後被搬走的 string 處於什麼狀態?能繼續用嗎?
    //    A:處於 valid-but-unspecified 狀態。可呼叫不需要前置條件的操作
    //       (size、empty、clear、再次賦值),但不可假設它的內容、size 為何。
    //       move assignment 必須是 noexcept,否則 vector<string> reallocation
    //       會選擇 copy 而非 move,效能大降。
    //
    //  Q3:s = "shorter" 之後 capacity 會自動縮小嗎?
    //    A:不會。賦值會盡量重用既有 buffer (若夠大就不重新配置),size 變小
    //       但 capacity 維持。若先前 buffer 因為大字串膨脹過,要釋放需呼叫
    //       shrink_to_fit() (non-binding),或 swap 一個全新小字串進來。
    //
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra operator_assign.cpp -o operator_assign

// === 預期輸出 (節錄) ===
// === LeetCode 1668 ===
// 2
// 1
// 0
// === 日常實務: move-assignment ===
// abcabcabcabcabc
