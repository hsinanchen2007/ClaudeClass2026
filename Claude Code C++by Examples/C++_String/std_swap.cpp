// =============================================================================
// 檔名: std_swap.cpp
// 主題: std::swap 對 std::string 的特化 (非成員函式)
// 參考:
//   - https://en.cppreference.com/cpp/string/basic_string/swap2
//   - https://cplusplus.com/reference/string/string/swap-free/
// =============================================================================
//
// 【函式資訊 Information】
//   namespace std {
//     template<class CharT, class Traits, class Alloc>
//     void swap(basic_string<CharT,Traits,Alloc>& a,
//               basic_string<CharT,Traits,Alloc>& b) noexcept;
//   }
//
//   - 所屬:`<string>` 標頭內的非成員函式 (std namespace 中對 basic_string 的特化)
//   - 參數:兩個同型別 string 的 lvalue reference
//   - 回傳:void
//   - noexcept:C++17 起為無條件 noexcept (假設 allocator 不丟例外)
//
// =============================================================================
//
// 【詳細解釋 Explanation】
//
// 【1. std::swap 的兩種樣貌】
//   - 通用版 (在 <utility>) 對任意 T:做三次 move (tmp = move(a); a = move(b); b = move(tmp))
//     ── 總計三次 move,如果 T 是 std::string,實際上每個 move 也只是換指標,但仍多一個臨時物件。
//   - basic_string 特化版 (在 <string>) 直接呼叫成員 a.swap(b),只交換 (data ptr, size, capacity,
//     SBO buffer 內容) 三件套。
//
// 【2. 為什麼 std::string 的 swap 是 O(1) (long string 情況)】
//   - 一個 std::string 物件,在「Long string」(超出 SBO 的字串) 狀態下,內部僅持有
//     (heap 指標 + size + capacity)。swap 就是換這三個 word,完全不需要動字元資料。
//   - SBO (Small Buffer Optimization) 啟用時,小字串會放在物件本體裡;此時 swap 仍是
//     固定大小的 memcpy,複雜度仍視為 O(1)(常數,但常數較大)。
//
// 【3. 為什麼有「成員 swap」與「非成員 swap」兩種?】
//   STL 統一慣例:
//     1. 提供成員 a.swap(b) 給直接呼叫者
//     2. 提供 std::namespace 的非成員 swap 與容器同名,讓泛型程式碼透過 ADL 找到最高效版本
//     3. std::swap 通用模板退而求其次 (走三次 move)
//
// 【4. ADL (Argument-Dependent Lookup) 慣用法】
//   標準 idiom:
//       using std::swap;
//       swap(a, b);   // 不限定 namespace
//   為什麼要這樣寫?
//     - 若 a 是 my_ns::Foo,編譯器會先在 my_ns 中找名為 swap 的函式 (ADL),
//       找不到才退回 using 引入的 std::swap。
//     - 這樣使用者自定型別可以提供更高效的 swap,而泛型演算法不必修改。
//
// 【5. 何時 std::swap 會「複製」而非真正 swap?】
//   - 如果模板參數 T 沒有 noexcept 的 move,std::swap 仍可工作但可能複製 (退化效能)。
//   - basic_string 的 move/swap 都是 noexcept,因此沒有此風險。
//
// 【6. swap 與 strong exception safety】
//   - copy-and-swap idiom 是經典 strong-guarantee assignment 寫法 (見下方範例)。
//   - 因為 swap 是 noexcept,所以「賦值流程」可拆成:
//       (1) 先 copy 到一個臨時物件 (可能拋例外但 *this 不變)
//       (2) swap 將臨時物件與自己對換 (noexcept)
//       (3) 函式結束時臨時物件析構
//     若 (1) 失敗,*this 完全不被破壞。
//
// 【7. C++20 起 ranges::swap】
//   - <concepts> 引入 std::ranges::swap CPO,自動處理 ADL,與 swap idiom 等價,
//     但語意更乾淨。
//
// =============================================================================
//
// 【概念補充 Concept Deep Dive】
//
// 【ADL (Argument-Dependent Lookup) 是什麼?】
//   - 又稱 Koenig lookup,當無命名空間限定的函式呼叫遇到引數時,
//     編譯器除了當前 scope,還會「自動」搜尋引數型別所在的 namespace。
//   - 範例:
//       namespace foo { struct X{}; void bar(X){} }
//       int main() {
//           foo::X x;
//           bar(x);          // OK!ADL 在 namespace foo 找到 bar
//       }
//   - swap 之所以採 `using std::swap; swap(a,b);` 寫法,就是讓 ADL 與 std::swap fallback
//     同時生效。
//
// 【為何不能直接寫 std::swap(a, b) 在泛型函式中?】
//   - 寫死 std::swap 會「跳過」用戶自定的高效率 swap。例如 boost::container::vector
//     有自己的 swap,直接 std::swap 會走通用三次 move 版本。
//   - 對 std::string 沒影響(它的 std::swap 已特化),但寫 idiom 比較通用。
//
// 【custom-swap idiom 三步驟】
//   1. 在 class 內提供 noexcept 的 swap(MyClass&)
//   2. 在 class 所在 namespace 提供非成員 swap(MyClass&, MyClass&) noexcept
//      → 內部呼叫 lhs.swap(rhs)
//   3. 賦值用 copy-and-swap:operator=(MyClass rhs) { swap(*this, rhs); return *this; }
//
// 【SBO 對 swap 的影響】
//   - libstdc++ / libc++ 的 std::string 都使用 SBO (~15 chars on 64-bit)。
//   - SBO 字串 swap 必須 byte-by-byte 拷貝整個物件 (約 32 bytes),因此「常數較大但仍 O(1)」。
//
// 【swap 與容器 iterator 的 invalidation】
//   - 標準保證:swap 後,iterator/pointer/reference 仍指向「原本的資料」(換句話說,
//     對換後 iterator 屬於另一個容器)。但對 std::string 而言,大多數實作會 invalidate
//     all iterators (libstdc++ 與 libc++ 行為不一致,程式不應依賴)。
//
// =============================================================================
//
// 【注意事項 Pay Attention】
// 1. ADL 慣用法:
//      using std::swap;
//      swap(a, b);
//    這會優先找到 a 的命名空間裡的 swap;若沒找到再用 std::swap。
//    對 std::string 兩種寫法等價。
// 2. C++17 起 std::swap 對 string 為無條件 noexcept。
// 3. 別在 string 即將被解構時 swap (邏輯錯誤,雖然技術上合法)。
// 4. swap 自己 (a.swap(a)) 是合法的且為 no-op,但寫出來可讀性差。
//
// =============================================================================

/*
補充筆記：std::string::std_swap
  - std::string::std_swap 需要同時考慮內容、容量與舊指標失效；字串 API 常會配置或搬移緩衝區。
  - 回傳位置的函式要先處理 npos，回傳 pointer/view 的函式要追蹤來源 string 生命週期。
  - UTF-8 文字下，size 代表位元組數，不代表使用者看到的字元數。
  - std::string::std_swap 是 std::string 主題的一部分；先分清楚這個操作會讀取、追加、插入、刪除、搜尋，還是只暴露底層字元。
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
#include <utility>

void demoStdSwap() {
    std::string a = "AAA";
    std::string b = "BBBBB";

    std::swap(a, b);                                            // 直接呼叫 std namespace 版本
    std::cout << "after std::swap: a=\"" << a << "\", b=\"" << b << "\"\n";

    // ADL 慣用法 (推薦寫法,泛型程式碼相容性最好)
    using std::swap;
    swap(a, b);
    std::cout << "after ADL swap : a=\"" << a << "\", b=\"" << b << "\"\n";
}

// -----------------------------------------------------------------------------
// 【實務範例】copy-and-swap idiom
// 為何用 std::swap: 經典的 strong exception safety assignment 寫法。
//                  if 任何複製過程拋例外,*this 完全不被破壞。
// -----------------------------------------------------------------------------
class StringWrapper {
    std::string data;
public:
    StringWrapper(const std::string& s) : data(s) {}
    StringWrapper(const StringWrapper&) = default;
    StringWrapper(StringWrapper&&) noexcept = default;

    // copy-and-swap:by value 接收 → 已經完成複製 → 與自身 swap
    StringWrapper& operator=(StringWrapper rhs) {     // by value (複製或移動發生在這裡)
        std::swap(data, rhs.data);                    // noexcept,strong guarantee
        return *this;
    }
    const std::string& get() const { return data; }
};

// -----------------------------------------------------------------------------
// 【LeetCode 344. Reverse String】(用 std::swap 反轉字元)
// 題目: 原地反轉字串 (字元陣列)。
// 為何用 std::swap: 雙指針逐對交換是最直觀寫法,對 char 也同樣 O(1) 高效。
// -----------------------------------------------------------------------------
void reverseString(std::string& s) {
    size_t l = 0, r = s.size();
    if (r == 0) return;
    --r;
    while (l < r) {
        std::swap(s[l], s[r]);                  // char 的 swap 也是 O(1)
        ++l;
        --r;
    }
}

// -----------------------------------------------------------------------------
// 【LeetCode 917. Reverse Only Letters】
// 題目: 只反轉字串中的字母,非字母位置保持不動。
// 為何用 std::swap: 雙指針跳過非字母,字母對換用 swap 直觀又安全。
// -----------------------------------------------------------------------------
#include <cctype>
std::string reverseOnlyLetters(std::string s) {
    if (s.empty()) return s;
    size_t l = 0, r = s.size() - 1;
    while (l < r) {
        while (l < r && !std::isalpha(static_cast<unsigned char>(s[l]))) ++l;
        while (l < r && !std::isalpha(static_cast<unsigned char>(s[r]))) --r;
        if (l < r) {
            std::swap(s[l], s[r]);
            ++l;
            --r;
        }
    }
    return s;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】Atomic-style 內容替換 ── 讀寫分離
// 為何用 std::swap: 在多執行緒場景中 (這裡示意),先在 worker 中組好結果,
//                   然後快速把它與 service 的「目前資料」swap (持鎖時間極短)。
//                   報表快取、前端 snapshot 推送很常見此模式。
//                   swap 是 O(1) → critical section 短到幾乎可忽略。
// -----------------------------------------------------------------------------
class Snapshot {
    std::string current;
public:
    // 把外部準備好的 next 與內部 current 對換,呼叫端拿到舊版本
    std::string replace(std::string next) {
        std::swap(current, next);                    // O(1),只換指標
        return next;     // 此時 next 是舊的 current
    }
    const std::string& view() const { return current; }
};

// -----------------------------------------------------------------------------
// 【LeetCode 範例 3】LeetCode 1page. 1page0. Goal Parser Interpretation - 簡化
// 題目: LeetCode 1page. 1page0. Maximum Number of Vowels in a Substring - 字串版
// 變奏: 給字串 s,把所有母音「整段」往後移到結尾(維持各自原有相對順序)。
// 為何用 swap: 用兩個 string (vowels, consonants),最後 swap 完成順序整合。
// 複雜度: O(N)。
// -----------------------------------------------------------------------------
std::string moveVowelsToEnd(const std::string& s) {
    std::string vowels, consonants;
    vowels.reserve(s.size());
    consonants.reserve(s.size());
    auto isV = [](char c) { return c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u'; };
    for (char c : s) (isV(c) ? vowels : consonants).push_back(c);
    consonants += vowels;
    std::string out;
    std::swap(out, consonants);     // O(1) move 給 out
    return out;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】double-buffering: 主線程從 frontBuffer 讀,worker 寫 backBuffer
// 為何用 std::swap: 完成 backBuffer 後與 frontBuffer swap (O(1)),
//                   是高效 lockless-ish 雙緩衝模式核心。
// -----------------------------------------------------------------------------
class DoubleBuffer {
    std::string front, back;
public:
    void writeBack(const std::string& data) { back = data; }
    void flip() { std::swap(front, back); back.clear(); }
    const std::string& read() const { return front; }
};

int main() {
    demoStdSwap();

    std::cout << "\n=== copy-and-swap ===\n";
    StringWrapper a("hello"), b("world");
    a = b;
    std::cout << a.get() << "\n";   // "world"

    std::cout << "\n=== LeetCode 344: Reverse String ===\n";
    std::string r1 = "hello";
    reverseString(r1);
    std::cout << r1 << "\n";        // "olleh"

    std::cout << "\n=== LeetCode 917: Reverse Only Letters ===\n";
    std::cout << reverseOnlyLetters("a-bC-dEf-ghIj") << "\n";   // "j-Ih-gfE-dCba"

    std::cout << "\n=== LeetCode 變奏 (moveVowelsToEnd) ===\n";
    std::cout << moveVowelsToEnd("leetcode")  << "\n";   // ltcdeeeo
    std::cout << moveVowelsToEnd("hello")     << "\n";   // hlleo

    std::cout << "\n=== 日常實務: Snapshot ===\n";
    Snapshot snap;
    auto old1 = snap.replace("v1 data");
    auto old2 = snap.replace("v2 data");
    std::cout << "current=" << snap.view() << ", prev=" << old2 << "\n";
    (void)old1;

    std::cout << "\n=== 日常實務: DoubleBuffer ===\n";
    DoubleBuffer db;
    db.writeBack("frame1"); db.flip();
    std::cout << "read: " << db.read() << "\n";   // frame1
    db.writeBack("frame2"); db.flip();
    std::cout << "read: " << db.read() << "\n";   // frame2

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1:為什麼要寫 using std::swap; swap(a,b); 而不直接 std::swap(a,b)?
    //    A:這是 ADL idiom。寫 std::swap 會強制走通用 template (三次 move),
    //      跳過自訂型別在自己 namespace 中可能提供的高效率 swap。using +
    //      unqualified call 讓 ADL 找到型別專屬版本,找不到才退回 std::swap。
    //
    //  Q2:std::string 的 swap 真的 O(1) 嗎?SSO 短字串怎麼處理?
    //    A:長字串 (heap) 是 O(1) — 只交換 (data ptr, size, capacity) 三個
    //      欄位。短字串 (SSO) 仍須 byte-copy 整個物件本體 (約 24~32 bytes),
    //      但仍是常數時間,只是常數較大。複雜度視為 O(1)。
    //
    //  Q3:swap 後原本拿到的 iterator 還能用嗎?
    //    A:標準保證 swap 後 iterator/reference/pointer 仍指向「原本的資料」
    //      (現在屬於對換後的另一個容器);但 std::string 的實作對此一致性
    //      不一 (libstdc++ vs libc++ 行為不同)。安全做法:swap 後不要再用
    //      swap 前保存的 iterator/c_str() 指標,重新取得。
    //
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra std_swap.cpp -o std_swap

// === 預期輸出 (節錄) ===
// === LeetCode 變奏 (moveVowelsToEnd) ===
// ltcdeeeo
// hlleo
// === 日常實務: DoubleBuffer ===
// read: frame1
// read: frame2
