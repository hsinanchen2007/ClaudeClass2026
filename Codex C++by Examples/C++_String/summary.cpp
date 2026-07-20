/*
 * =============================================================================
 * std::string 面試前高頻總複習：擁有權、搜尋、修改、轉換、生命週期與實務解析
 * =============================================================================
 *
 * 這一檔提供可獨立執行的高頻主線與跨 API 契約，不宣稱列完 basic_string 的每個
 * overload、allocator propagation 情境或 Unicode 規則。先記住一句話：
 *
 *   std::string 是擁有連續 char code units 的可變容器；它不是 Unicode 字形容器，
 *   也不是永遠不失效的 C 字串指標。
 *
 * -----------------------------------------------------------------------------
 * 【一、核心資料模型】
 * -----------------------------------------------------------------------------
 * 1. `size()`/`length()` 是 char 元素數，O(1)。UTF-8 的一個中文字通常占 3 bytes，
 *    所以 size 不等於人眼字數、code point 數或 grapheme cluster 數。
 * 2. 元素連續儲存；C++11 起可依賴 `&s[0]+n == &s[n]`（合法索引範圍內）。
 * 3. string 可以保存內嵌 '\0'；`string("A\0B",3)` size 為 3，但 strlen(c_str()) 為 1。
 * 4. `data()[size()]` 有終止 NUL，可供唯讀 C API；內嵌 NUL 仍讓 C API 提早停止。
 * 5. Small String Optimization 很常見但不是標準保證；不要寫死 SSO 容量。
 *
 * -----------------------------------------------------------------------------
 * 【二、API 選擇速查】
 * -----------------------------------------------------------------------------
 * 需求                         首選 API                         關鍵提醒
 * 建立擁有副本                 constructor / operator=          O(n)，可含 NUL 要給長度
 * 整批改成新內容               assign / operator=               舊 iterator 可能失效
 * 唯讀參數或零拷貝切片         string_view                       不擁有，來源要活著
 * 安全索引                     at                                越界丟 out_of_range
 * 已證明合法的快速索引         operator[]                        僅 pos < size()
 * 第一/最後元素                front/back                        先確認 !empty()
 * 加一個字元                   push_back / += char               攤銷 O(1)
 * 刪最後字元                   pop_back                           非空前提、無回傳值
 * 加一段內容                   append / +=                       先 reserve 可減少重配
 * 中間插入/刪除/替換           insert/erase/replace              通常 O(n)，後段要搬移
 * 刪所有特定字元               C++20 std::erase/erase_if         線性、回刪除數量
 * 只預留配置，不建立元素       reserve                            size 不變
 * 真正建立/刪減元素            resize                             size 改變
 * 希望交還多餘容量             shrink_to_fit                      非強制，不可依賴
 * 複製到 caller char buffer     copy                               回實際量，不附 NUL
 * 找完整 substring             find/rfind                         找不到是 npos
 * 找集合中任一 char            find_first_of/find_last_of         不是找整個 pattern
 * 找集合外第一/最後 char       find_first_not_of/last_not_of      常用 trim
 * 擁有型子字串                 substr                             O(k)，可能配置
 * 比 substring 不想配置        compare(pos,count,...)             看結果符號
 * 前綴/後綴                    C++20 starts_with/ends_with         空 pattern 為 true
 * 是否包含                     C++23 contains / C++20 find        contains 不是 C++20
 * C API 唯讀 NUL 字串          c_str                              pointer 可能失效
 * C API 寫入既有元素           data + resize                       不可寫超過 size
 * 整數高效嚴格轉換             from_chars/to_chars                 查 ec 與 ptr
 * 方便但例外式數值轉換         stoi/stod/to_string                 查 idx、catch 例外
 * 一整行輸入                   getline                             while(getline(...))
 * 空白分隔 token               operator>>                          檢查 stream state
 * 複雜 pattern                 regex                              match 與 search 不同
 * arena 配置字串               pmr::string                         resource 必須活得更久
 * producer 直接填 buffer       C++23 resize_and_overwrite          C++20 要 guard/fallback
 * unordered key                std::hash<string>                  非持久/非密碼學 hash
 *
 * -----------------------------------------------------------------------------
 * 【三、複雜度速查】
 * -----------------------------------------------------------------------------
 * - size/empty/capacity/front/back/data/c_str：O(1)。
 * - at/operator[]：O(1)，差在邊界檢查。
 * - append k bytes：O(k) 加可能重配；單字元 push_back 攤銷 O(1)。
 * - insert/erase/replace 中段：O(n)，因後段要搬移。
 * - substr k bytes：O(k) 且通常配置；string_view::substr 是 O(1)。
 * - compare：最壞 O(min(n,m))。
 * - copy count 個 char：O(實際複製量)，且不配置、不補 NUL。
 * - find 系列：依實作與 pattern；不能一概承諾永遠 O(n)。
 * - regex 的時間/空間依 grammar、pattern、input 與實作，不能套單一 O(n) 保證。
 * - hash：計算 O(n)；unordered 查找平均 O(1)、最壞 O(n)。
 *
 * -----------------------------------------------------------------------------
 * 【四、iterator / pointer / reference 失效規則】
 * -----------------------------------------------------------------------------
 * 1. 任何可能重新配置的操作（append/insert/reserve/resize 等）都可能讓全部失效。
 * 2. 不要自行套用 vector 的精細失效表到 basic_string；非 const 修改後，除非該 API
 *    明確保證，portable 作法是重新取得 iterator/reference/pointer/view。
 * 3. 保存 `const char* p=s.c_str()` 後修改 s，再使用 p 是典型 use-after-invalidation。
 * 4. string_view 指向同一 buffer，也會隨 owner 銷毀或重配而懸空。
 * 5. 最穩定的規則：修改後重新取得 iterator/data/c_str/view，不猜實作是否重配。
 *
 * -----------------------------------------------------------------------------
 * 【五、npos 與無號整數安全】
 * -----------------------------------------------------------------------------
 * - npos 是 size_type 的特殊最大值，不是 int -1。
 * - 成功檢查模板：`if (pos != string::npos) { ... }`。
 * - 禁止先做 `pos+1`；npos+1 會無號 wrap 成 0。
 * - 安全倒序：`for (size_t i=s.size(); i-- > 0;)`，不要 `i>=0`。
 * - 長度相加前防 overflow：`incoming <= max_size()-size()`。
 *
 * -----------------------------------------------------------------------------
 * 【六、C++ 版本邊界】
 * -----------------------------------------------------------------------------
 * - C++17：string_view、from_chars/to_chars（整數）、可寫 non-const data()。
 * - C++20：starts_with、ends_with、std::erase/erase_if、std::format（標準庫支援需確認）。
 * - C++23：contains、resize_and_overwrite、append_range/insert_range 等 range modifiers。
 * - 教材若以 C++20 建置，C++23 API 必須 feature-test guard 或明確改用 fallback。
 *
 * -----------------------------------------------------------------------------
 * 【七、重要 sibling 契約補遺】
 * -----------------------------------------------------------------------------
 * - constructor/assign：pointer overload 要求有效來源；需保留內嵌 NUL 時傳 pointer+count
 *   或使用 `"A\0B"s`。assign/replace 會改 owner，舊 view 不可繼續借用。
 * - element access：at 越界丟例外；front/back/pop_back 都要求非空，違反前置條件是 UB。
 *   `s[s.size()]` 是終止 sentinel，不是可改成任意值的一般元素。
 * - copy：實際量為 min(count,size-pos)，pos>size 丟 out_of_range；絕不附加 NUL。
 * - compare/strcmp：只保證負、零、正，不保證恰為 -1/0/1；operator+ 連鎖可能多次配置。
 * - C interop：strlen/strstr 在第一個 NUL 停止；memcpy 不可重疊，重疊用 memmove；
 *   C API 要寫入時先 resize 建立元素，回來再縮到實際長度，不能只 reserve 就寫。
 * - numeric：from_chars 不跳 whitespace、整數不接受前導 `+`、以 ec+ptr 判完整；stoi
 *   可跳前導 whitespace 並接受 partial parse，嚴格格式要檢查 idx 且處理兩種例外。
 * - pmr：polymorphic_allocator 只保存 memory_resource pointer；resource 必須比容器與其
 *   配置活得久。不得假設 copy/move/swap 自動轉移 resource；allocator 不相等時查 propagation。
 * - resize_and_overwrite：C++23 callback 只在取得的 [buffer,buffer+n) 寫，回傳不大於 n
 *   的最終 size，且 pointer 不可逃出 callback；C++20 用 resize/write/resize fallback。
 * - regex：預設 ECMAScript grammar；match 要整串、search 找子區段。regex 建構可丟
 *   regex_error，match_results 內 sub_match 借用原字串 iterators，先複製 capture 再改 owner。
 * - swap/hash/I/O：PMR allocator 不相等時不可任意 swap；hash 不做持久 ID；`>>` 與 getline
 *   混用要處理殘留 newline，逐行讀取以 `while (getline(...))` 驅動。
 *
 * -----------------------------------------------------------------------------
 * 【八、常見面試問答】
 * -----------------------------------------------------------------------------
 * Q1：string 與 string_view 怎麼選？
 * A：需要擁有/保存/修改用 string；只讀且呼叫期間來源有效可用 string_view。
 *
 * Q2：reserve 與 resize 差在哪？
 * A：reserve 只改 capacity；resize 改 size 並建立/刪除元素。
 *
 * Q3：clear 會釋放記憶體或安全抹除密碼嗎？
 * A：都不保證。clear 只讓 size 變 0，capacity 和舊 bytes 可能仍在配置中。
 *
 * Q4：為何 `const char* a,*b; a==b` 不是內容比較？
 * A：它比較位址；轉 string/string_view 或使用 strcmp 才比較內容。
 *
 * Q5：`find_first_of("abc")` 是找 substring "abc" 嗎？
 * A：不是；它找第一個為 a、b 或 c 的字元。完整 pattern 用 find。
 *
 * Q6：std::hash 能否存進資料庫當永久 ID？
 * A：不能，演算法/seed/結果穩定性與密碼學安全都沒有承諾。
 *
 * Q7：`std::tolower(ch)` 有何隱藏 UB？
 * A：若 signed char 為負且不是 EOF 就違反前置條件；先轉 unsigned char。
 *
 * Q8：`std::string("A\0B")` 為何不是 3？
 * A：C 字串建構在第一個 NUL 停止；用 pointer+length 或 "A\0B"s。
 *
 * Q9：`shrink_to_fit` 保證 capacity==size 嗎？
 * A：不保證，它只是 non-binding request。
 *
 * Q10：如何嚴格判斷 from_chars/stoi 已吃完整 token？
 * A：from_chars 檢查 ec=={} 且 ptr==end；stoi 傳 idx 並檢查 idx==size。
 *
 * Q11：`string::copy` 為何不能直接把 buffer 當 C 字串？
 * A：copy 不附終止 NUL；需多留一格並以回傳的 copied 數量手動補上。
 *
 * Q12：pmr::string 自己活著，resource 先析構可以嗎？
 * A：不行；allocator 內只是 resource pointer，後續操作或 string 解構都可能經它 deallocate。
 *
 * Q13：regex_match 與 regex_search 差在哪？
 * A：match 要整個範圍符合，search 只需找到任一符合子區段。
 *
 * -----------------------------------------------------------------------------
 * 【九、下面的整合程式】
 * -----------------------------------------------------------------------------
 * - leetcode_longest_unique_substring：滑動視窗、unsigned char 索引、O(n)。
 * - leetcode_valid_palindrome：string_view、cctype 前置條件、雙指標。
 * - practical_parse_setting：trim、split、from_chars、領域驗證、擁有權邊界。
 * - practical_build_endpoint：reserve/append/to_chars，避免不必要中間字串。
 * - test_copy_pmr_and_overwrite：NUL、allocator/resource lifetime 與 C++23 guarded producer。
 * - test_regex_contracts：整串/局部匹配、capture ownership 與 replace。
 */

#include <algorithm>
#include <array>
#include <cassert>
#include <charconv>
#include <cctype>
#include <cstddef>
#include <iostream>
#include <memory>
#include <memory_resource>
#include <optional>
#include <regex>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>

namespace {

constexpr std::string_view ascii_whitespace = " \t\r\n";

std::string_view trim_view(const std::string_view input) {
    const std::size_t begin = input.find_first_not_of(ascii_whitespace);
    if (begin == std::string_view::npos) return {};
    const std::size_t end = input.find_last_not_of(ascii_whitespace);
    return input.substr(begin, end - begin + 1U);
}

// LeetCode 3（Longest Substring Without Repeating Characters）。
// last_seen[byte] 保存「上次索引 + 1」；0 代表沒出現。把 char 轉 unsigned char，
// 避免 UTF-8 high-bit byte 在 signed-char 平台成負索引。這仍按 byte，不按 Unicode 字形。
int leetcode_longest_unique_substring(const std::string_view text) {
    std::array<std::size_t, 256U> last_seen{};
    std::size_t window_begin = 0U;
    std::size_t best = 0U;

    for (std::size_t index = 0U; index < text.size(); ++index) {
        const auto byte = static_cast<unsigned char>(text[index]);
        const std::size_t next_after_previous = last_seen[byte];
        if (next_after_previous > window_begin) {
            window_begin = next_after_previous;
        }
        const std::size_t length = index - window_begin + 1U;
        if (length > best) best = length;
        last_seen[byte] = index + 1U;
    }
    return static_cast<int>(best);
}

// LeetCode 125（Valid Palindrome），依原題只處理 ASCII 英數語意。
bool leetcode_valid_palindrome(const std::string_view text) {
    std::size_t left = 0U;
    std::size_t right = text.size();

    while (left < right) {
        while (left < right &&
               std::isalnum(static_cast<unsigned char>(text[left])) == 0) {
            ++left;
        }
        while (left < right &&
               std::isalnum(static_cast<unsigned char>(text[right - 1U])) == 0) {
            --right;
        }
        if (left < right) {
            const int a = std::tolower(static_cast<unsigned char>(text[left]));
            const int b = std::tolower(static_cast<unsigned char>(text[right - 1U]));
            if (a != b) return false;
            ++left;
            --right;
        }
    }
    return true;
}

struct Setting {
    std::string key;  // owning：結果可脫離原始 line 生命週期。
    int value{};
};

// 實務：解析 ` key = integer `。parser 內以 view 零拷貝切片，API 邊界回 owning key。
std::optional<Setting> practical_parse_setting(const std::string_view line,
                                               const int minimum,
                                               const int maximum) {
    const std::string_view cleaned = trim_view(line);
    const std::size_t equal = cleaned.find('=');
    if (equal == std::string_view::npos) return std::nullopt;

    const std::string_view key = trim_view(cleaned.substr(0U, equal));
    const std::string_view value_text = trim_view(cleaned.substr(equal + 1U));
    if (key.empty() || value_text.empty()) return std::nullopt;

    int value = 0;
    const auto [end, error] = std::from_chars(value_text.data(),
                                              value_text.data() + value_text.size(), value);
    if (error != std::errc{} || end != value_text.data() + value_text.size()) {
        return std::nullopt;
    }
    if (value < minimum || value > maximum) return std::nullopt;
    return Setting{std::string(key), value};
}

// 實務：建立 `https://host:port/path`。to_chars 避免數字中間 string；reserve 降重配。
std::string practical_build_endpoint(const std::string_view host,
                                     const unsigned short port,
                                     const std::string_view path) {
    std::array<char, 5U> port_buffer{};  // 65535 最多五位。
    const auto [port_end, error] =
        std::to_chars(port_buffer.data(), port_buffer.data() + port_buffer.size(), port);
    assert(error == std::errc{});

    std::string result;
    result.reserve(8U + host.size() + 1U +
                   static_cast<std::size_t>(port_end - port_buffer.data()) + 1U + path.size());
    result.append("https://");
    result.append(host);
    result.push_back(':');
    result.append(port_buffer.data(), port_end);
    if (path.empty() || path.front() != '/') result.push_back('/');
    result.append(path);
    return result;
}

std::string build_status_record()
{
    // resize_and_overwrite 是 C++23；C++20 建置走明確的 resize/write/resize fallback。
    // callback 的 buffer 只在 callback 期間有效，且每個計入回傳長度的 char 都要初始化。
    constexpr std::size_t maximum_size = 16U;
    constexpr std::string_view payload = "job=42";
    std::string result;
#if defined(__cpp_lib_string_resize_and_overwrite) && \
    __cpp_lib_string_resize_and_overwrite >= 202110L
    result.resize_and_overwrite(maximum_size, [payload](char* buffer, const std::size_t writable) {
        if (writable < payload.size()) return std::size_t{0U};
        std::copy(payload.begin(), payload.end(), buffer);
        return payload.size();
    });
#else
    result.resize(maximum_size);
    std::copy(payload.begin(), payload.end(), result.begin());
    result.resize(payload.size());
#endif
    return result;
}

using PmrCharAllocator = std::pmr::polymorphic_allocator<char>;
static_assert(!std::allocator_traits<PmrCharAllocator>::propagate_on_container_copy_assignment::value);
static_assert(!std::allocator_traits<PmrCharAllocator>::propagate_on_container_move_assignment::value);
static_assert(!std::allocator_traits<PmrCharAllocator>::propagate_on_container_swap::value);

void test_copy_pmr_and_overwrite()
{
    const std::string source = "abcdef";
    std::array<char, 5U> destination{};
    destination.fill('#');
    const std::size_t copied = source.copy(destination.data(), 3U, 1U);
    assert(copied == 3U);
    assert(destination[0] == 'b' && destination[1] == 'c' && destination[2] == 'd');
    assert(destination[copied] == '#'); // copy 沒有偷偷附加 NUL。
    destination[copied] = '\0';         // 要當 C 字串時由 caller 使用回傳量補上。
    assert(std::string(destination.data()) == "bcd");

    // 宣告順序刻意是 storage -> resource -> string；解構反序，resource 會比 string 晚死。
    std::array<std::byte, 512U> arena_storage{};
    std::pmr::monotonic_buffer_resource resource(
        arena_storage.data(), arena_storage.size(), std::pmr::null_memory_resource());
    PmrCharAllocator allocator(&resource);
    std::pmr::string arena_text(allocator);
    arena_text.assign(128U, 'x');
    assert(arena_text.size() == 128U);
    assert(arena_text.get_allocator().resource() == &resource);

    assert(build_status_record() == "job=42");
}

void test_regex_contracts()
{
    static const std::regex identifier(R"([A-Za-z_][A-Za-z0-9_]*)");
    assert(std::regex_match("valid_name2", identifier));
    assert(!std::regex_match("prefix valid_name2 suffix", identifier));

    static const std::regex job_id(R"(id=([0-9]+))");
    const std::string line = "status=ok id=42 elapsed=7";
    std::smatch match; // sub_match 內部借用 line 的 iterators；line 不可先銷毀或修改。
    const bool found = std::regex_search(line, match, job_id);
    assert(found);
    assert(match.size() == 2U && match[0].str() == "id=42");
    const std::string owned_capture = match[1].str();
    assert(owned_capture == "42");

    static const std::regex digits(R"([0-9]+)");
    assert(std::regex_replace(std::string{"a1b22"}, digits, "#") == "a#b#");
}

void test_basic_model() {
    const char raw[] = {'A', '\0', 'B'};
    const std::string binary(raw, sizeof(raw));
    assert(binary.size() == 3U);
    assert(binary.length() == binary.size());
    assert(binary[1U] == '\0');
    assert(binary.data()[binary.size()] == '\0');

    std::string capacity_demo = "abc";
    capacity_demo.reserve(64U);
    assert(capacity_demo.size() == 3U);       // reserve 不改 size。
    assert(capacity_demo.capacity() >= 64U);  // 只承諾至少這麼大。
    capacity_demo.resize(5U, '.');
    assert(capacity_demo == "abc..");        // resize 才建立元素。
}

void test_search_and_modifiers() {
    std::string text = "  alpha,beta  ";
    assert(trim_view(text) == "alpha,beta");
    assert(text.find(',') == 7U);
    assert(text.rfind('a') == 11U);
    assert(text.find_first_of(",;") == 7U);
    assert(text.find_first_not_of(' ') == 2U);
    assert(text.find_last_not_of(' ') == 11U);
    assert(text.starts_with("  alpha"));
    assert(text.ends_with("  "));

    text.erase(0U, 2U);
    text.erase(text.size() - 2U);
    text.replace(text.find(','), 1U, " | ");
    assert(text == "alpha | beta");
}

}  // namespace

int main() {
    test_basic_model();
    test_search_and_modifiers();
    test_copy_pmr_and_overwrite();
    test_regex_contracts();

    assert(leetcode_longest_unique_substring("abcabcbb") == 3);
    assert(leetcode_longest_unique_substring("bbbbb") == 1);
    assert(leetcode_longest_unique_substring("") == 0);
    assert(leetcode_valid_palindrome("A man, a plan, a canal: Panama"));
    assert(!leetcode_valid_palindrome("race a car"));

    const auto retries = practical_parse_setting("  retries = 3  ", 0, 10);
    assert(retries.has_value());
    assert(retries->key == "retries" && retries->value == 3);
    assert(!practical_parse_setting("retries=3x", 0, 10).has_value());
    assert(!practical_parse_setting("retries=99", 0, 10).has_value());
    assert(!practical_parse_setting("=3", 0, 10).has_value());

    assert(practical_build_endpoint("api.example.test", 443U, "v1/jobs") ==
           "https://api.example.test:443/v1/jobs");
    assert(practical_build_endpoint("localhost", 8080U, "/health") ==
           "https://localhost:8080/health");

    std::cout << "std::string summary: all tests passed\n";
}

/*
 * =============================================================================
 * 【最後 60 秒檢查清單】
 * =============================================================================
 * 1. 索引是否合法？空字串是否會呼叫 front/back/pop_back？
 * 2. find 結果是否先判 npos，再做 +1、substr 或 erase？
 * 3. 是否把 capacity 誤當 size，寫入尚未建立的元素？
 * 4. 修改後是否仍使用舊 iterator、c_str pointer 或 string_view？
 * 5. cctype 是否先轉 unsigned char？
 * 6. C API 是否要求 NUL？輸入是否可能含內嵌 NUL？目的 buffer 是否足夠？
 * 7. from_chars 是否同時檢查 ec 與 ptr？stoi 是否檢查 idx 並 catch 例外？
 * 8. string_view 是否可能指向 temporary/local/已重配 string？
 * 9. 文字需求到底按 byte、ASCII、Unicode code point 還是 grapheme？
 * 10. C++23 API 是否在 C++20 build 被誤用？
 * 11. copy 的目的 buffer 是否另留 NUL，且以回傳量決定終止位置？
 * 12. pmr resource/upstream 是否比所有使用它的 string 活得久？allocator 是否相等？
 * 13. regex 要整串還是局部？match_results 是否活過被借用的來源字串？
 *
 * 【延伸練習】
 * - 將 practical_parse_setting 擴充為 bool、整數、字串三種 variant。
 * - 寫 split_view(input,delimiter)，回 vector<string_view> 並聲明 owner 契約。
 * - 在 C++23 模式確認 resize_and_overwrite 真分支，並測 callback 回傳 0 與上限長度。
 * - 用實際 benchmark 比較 substr、string_view、find 在大型 log parser 的配置與耗時。
 * - 對 fuzz input 驗證 parser 不越界、不無限迴圈、無例外洩漏。
 */
