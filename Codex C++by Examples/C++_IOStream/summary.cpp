// ============================================================================
// C++ IOStream 總複習：格式化 I/O、檔案、字串解析、狀態機與可攜二進位格式
// ============================================================================
//
// 【本章地圖：對應 01～10】
//   overview / cout+cin / manipulators / sync_with_stdio / text fstream /
//   binary fstream / stringstream / getline / error state / practical log
//
// 【stream 類別關係】
//   istream：輸入（cin, ifstream, istringstream）
//   ostream：輸出（cout/cerr/clog, ofstream, ostringstream）
//   iostream：雙向；fstream/stringstream 可同時讀寫但 position/state 更需管理。
//   stream object 管理 buffer/resource；不可 copy，多數可 move。reference 參數不取得 ownership。
//
// 【formatted 與 unformatted I/O】
//   `>> value`        formatted：跳過前導 whitespace（除非 noskipws）、依 locale 解析。
//   `getline`         讀到 delimiter，不保留 delimiter；可保留行內空白。
//   get/read/gcount   unformatted；binary fixed-size 讀取要核對 gcount/state。
//   put/write         unformatted output；長度用 streamsize，避免不受檢查的 narrowing。
//
// 【state bits：stream 其實是一個狀態機】
//   goodbit  目前無錯；`good()` 僅此時 true。
//   eofbit   到達輸入尾端；最後一次成功讀取也可能同時設 eofbit。
//   failbit  formatted parse 失敗、格式不符或某些 seek/open 失敗。
//   badbit   底層不可恢復 I/O 錯誤。
//   `operator bool` 等同沒有 failbit/badbit；典型 loop 是 `while (input >> value)`。
//   recover：`clear()` 清 state，再 `ignore(...)` 丟掉壞資料；只 clear 不移動會無限失敗。
//
// 【絕對不要寫 while (!stream.eof())】
//   eof 只有「嘗試讀過尾端」後才設定；先檢查 eof 會讓舊 value 多處理一次。
//   正確：`while (stream >> value) { use(value); }` 或 `while (getline(stream,line))`。
//
// 【openmode】
//   in / out / app / ate / trunc / binary。
//   app：每次 write 都定位末尾；ate：開啟時先移末尾，之後仍可 seek。
//   binary：Windows 避免文字換行轉換；它不替你定義 endian、padding、version。
//
// 【常見 manipulators 與持續性】
//   setw 只影響下一個欄位；setfill、fixed/scientific、setprecision、boolalpha、hex 等會持續。
//   library helper 不應污染 caller stream；可保存 flags/precision/fill，或在 local
//   ostringstream 格式化後只輸出字串。`std::endl` = '\n' + flush，熱路徑常只需 '\n'。
//
// 【cin/cout 加速】
//   `ios::sync_with_stdio(false); cin.tie(nullptr);` 適合競賽，但設後不要混用 stdio 與
//   iostream 並期待原順序；應在任何 I/O 前只做一次。互動程式取消 tie 後 prompt 要手動 flush。
//
// 【API 選型速查】
//   讀空白分隔 typed values       -> operator>>，完成後檢查 fail/eof
//   讀完整一行（可含空白）       -> getline
//   寬鬆 typed token stream       -> operator>>；大量/嚴格純數值用 from_chars
//   組一段格式化文字再提交       -> ostringstream，避免污染 caller formatting state
//   文字檔逐行                    -> ifstream + getline
//   固定 wire bytes               -> binary mode + read/write + 明確 endian/schema
//   需要完整 CSV/JSON quoting     -> 成熟 parser/library，不用 delimiter 小技巧硬湊
//
// 【文字與二進位格式】
//   文字：可讀、較大、需 quoting/escaping/locale/version policy。
//   二進位：明訂 magic、version、每欄 width、byte order、length、checksum；不可直接 dump struct，
//   因為 padding、alignment、endianness、ABI 與 object representation 不穩定。
//
// 【複雜度與 lifetime】
//   順序讀寫 n bytes 是 O(n)；stringstream 產生 `.str()` 通常複製 O(n)。
//   flush/close 可能昂貴且可能失敗；ofstream destructor 不能可靠回報 close error，重要寫入要
//   明確 flush/close 並檢查。streambuf/stream/view 的借用不得活過底層 object。
//
// 【面試快問快答】
//   Q: eofbit 是否代表資料損壞？ A: 否；正常讀完也會有 eofbit，要和 fail/bad 及預期長度合看。
//   Q: `fail() && eof()` 能否當成正常結束？ A: 不能；溢位 token 在 EOF 也可能同時設兩個 bit。
//   Q: `>>` 後接 getline 為何空行？ A: formatted extraction 留下 delimiter newline。
//   Q: cerr 與 clog？ A: cerr 預設 unitbuf 常立即 flush；clog 適合 buffered diagnostic。
// ============================================================================

/*
==============================================================================
【面試深挖：IOStream】

IO1｜為何 `while (!in.eof())` 是錯誤讀檔模式？
答：eofbit 只有「讀取嘗試失敗」後才設定，loop 會多處理一次舊值。應把 extraction 本身
放條件：`while (in >> value)` 或 `while (getline(in,line))`。

IO2｜eofbit、failbit、badbit 差在哪？
答：eofbit 表示到輸入尾；failbit 表示格式解析/操作失敗；badbit 表示底層 I/O 嚴重錯誤。
`operator bool` 主要看 fail/bad；到尾時一次 extraction 可同時設 eofbit 與 failbit。

IO3｜為何 `operator>>` 後第一個 getline 常讀到空字串？
答：formatted extraction 通常留下 delimiter newline；getline 立即把它消耗成空行。
可先 ignore 到 newline，但要依 protocol 決定，不可每次盲目丟一整行。

IO4｜`std::endl` 與 '\n' 的差別？
答：endl 寫 newline 並 flush；'\n' 只寫字元。每行 endl 會強制同步造成性能損失，
只有真的需要立即對外可見時才 flush。

IO5｜開 `ios::binary` 是否就能可攜序列化 struct？
答：不能。binary 主要關閉平台文字轉換；struct 仍有 padding、endianness、型別寬度、
ABI、版本與 pointer 問題。可攜格式要逐欄定義 wire representation。

IO6｜RAII 關檔是否代表所有錯誤都能被看到？
答：destructor 會 close，但 close/flush error 無法安全由 destructor 丟出給呼叫端。
重要寫入要明確 flush/close 並檢查 stream state，再做 atomic rename 等發布。

IO7｜`sync_with_stdio(false)` 做什麼？
答：允許 C++ streams 不再與 C stdio 同步，可能更快；此後混用 printf/cout 的相對次序
不可依賴，且應在任何 I/O 前設定。它不是「讓 iostream thread-safe」的開關。

IO8｜何時用 stringstream，何時不用？
答：需要多型 formatted I/O、混合欄位時方便；大量數值 parse/format 可用 charconv，
簡單拼接可用 string/format。重複使用 stringstream 時要同時重設 buffer 與 state flags。

IO9｜stream exceptions mask 的陷阱？
答：預設以 state flags 回報；設定 exceptions(failbit|badbit) 後相關 setstate 會丟
ios_base::failure。API 要統一錯誤模型，避免一半檢查 bool、一半期待例外。

IO10｜多執行緒寫 cout 如何避免每行交錯？
答：每個個別 stream operation 有標準庫同步規則不代表多個 `<<` 是一筆原子訊息。
C++20 `osyncstream` 可先緩衝一整段再原子提交，或由 logger mutex/queue 序列化。

IO11｜locale 為何會影響 formatted I/O？
答：數字小數點、千分位、分類與轉換可依 locale；資料交換格式通常應固定 classic/C locale
或用 locale-independent charconv，避免同一文字在不同機器解析不同。
==============================================================================
*/

#include <array>
#include <cassert>
#include <charconv>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

int parse_integer_token(const std::string& token)
{
    int value = 0;
    const char* const begin = token.data();
    const char* const end = begin + token.size();
    const auto [parsed_end, error] = std::from_chars(begin, end, value, 10);
    if (error != std::errc{} || parsed_end != end) {
        throw std::invalid_argument("malformed or out-of-range integer token: " + token);
    }
    return value;
}

std::vector<int> parse_integer_line(const std::string& line)
{
    // stream 只負責依 whitespace 切 token；from_chars 負責 locale-independent 嚴格整數解析。
    // 不能以 input.eof() 判斷最後 token 是否成功：`999...` 在 EOF 可同時設 failbit|eofbit。
    std::istringstream input(line);
    std::vector<int> values;
    for (std::string token; input >> token;) {
        values.push_back(parse_integer_token(token));
    }
    if (input.bad()) {
        throw std::runtime_error("I/O failure while tokenizing integer line");
    }
    return values;
}

std::string format_table_row(const std::string& name, double value)
{
    // local stream 隔離 formatting state，不改呼叫者的 cout flags。
    std::ostringstream output;
    output << std::left << std::setw(10) << name
           << std::right << std::fixed << std::setprecision(2) << value;
    return output.str();
}

void basic_stream_demo()
{
    assert((parse_integer_line("10 20 -3") == std::vector<int>{10, 20, -3}));
    assert(parse_integer_line(" \t\n").empty());
    // 使用可精確表示的 7.5 測欄寬；不要把 binary floating tie 當格式器單測。
    assert(format_table_row("GPU", 7.5) == "GPU       7.50");

    const std::vector<std::string> malformed{
        "10 bad 30",
        "12x",
        "+",                         // from_chars 整數 grammar 不接受單獨正號。
        "-",                         // 有符號型別可有負號，但負號後仍必須有 digit。
        std::to_string(std::numeric_limits<int>::max()) + "0",
        std::to_string(std::numeric_limits<int>::min()) + "0"
    };
    for (const std::string& text : malformed) {
        bool rejected = false;
        try {
            static_cast<void>(parse_integer_line(text));
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        assert(rejected);
    }
}

// ---------------------------------------------------------------------------
// LeetCode 151：Reverse Words in a String
// operator>> 自然把一個以上 whitespace 視為分隔；再反向輸出單一空白。
// O(n) time / O(n) space。
// ---------------------------------------------------------------------------
std::string reverse_words(const std::string& text)
{
    std::istringstream input(text);
    std::vector<std::string> words;
    for (std::string word; input >> word;) {
        words.push_back(std::move(word));
    }

    std::ostringstream output;
    for (auto iterator = words.rbegin(); iterator != words.rend(); ++iterator) {
        if (iterator != words.rbegin()) output << ' ';
        output << *iterator;
    }
    return output.str();
}

void leetcode_demo()
{
    assert(reverse_words("the sky is blue") == "blue is sky the");
    assert(reverse_words("  hello   world  ") == "world hello");
    assert(reverse_words("a good   example") == "example good a");
}

// ---------------------------------------------------------------------------
// 實務：明確 wire format，而非 `write(reinterpret_cast<char*>(&struct), sizeof struct)`。
// 格式：magic "JOB1" + big-endian uint32 job id + 1-byte priority。
// 固定長度 9 bytes；encode/decode 都檢查完整性。
// ---------------------------------------------------------------------------
struct JobHeader {
    std::uint32_t id;
    std::uint8_t priority;
};

void write_u32_be(std::ostream& output, std::uint32_t value)
{
    const std::array<char, 4> bytes{
        static_cast<char>((value >> 24U) & 0xFFU),
        static_cast<char>((value >> 16U) & 0xFFU),
        static_cast<char>((value >> 8U) & 0xFFU),
        static_cast<char>(value & 0xFFU)};
    output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    if (!output) throw std::runtime_error("binary write failed");
}

std::uint32_t read_u32_be(std::istream& input)
{
    std::array<unsigned char, 4> bytes{};
    input.read(reinterpret_cast<char*>(bytes.data()),
               static_cast<std::streamsize>(bytes.size()));
    if (!input) throw std::runtime_error("truncated uint32");
    return (static_cast<std::uint32_t>(bytes[0]) << 24U)
         | (static_cast<std::uint32_t>(bytes[1]) << 16U)
         | (static_cast<std::uint32_t>(bytes[2]) << 8U)
         | static_cast<std::uint32_t>(bytes[3]);
}

void encode_job(std::ostream& output, const JobHeader& job)
{
    output.write("JOB1", 4);
    write_u32_be(output, job.id);
    output.put(static_cast<char>(job.priority));
    if (!output) throw std::runtime_error("job write failed");
}

JobHeader decode_job(std::istream& input)
{
    std::array<char, 4> magic{};
    input.read(magic.data(), static_cast<std::streamsize>(magic.size()));
    if (!input || magic != std::array<char, 4>{'J', 'O', 'B', '1'}) {
        throw std::runtime_error("bad job magic/version");
    }
    const std::uint32_t id = read_u32_be(input);
    const int priority = input.get();
    if (priority == std::char_traits<char>::eof()) {
        throw std::runtime_error("truncated priority");
    }
    return JobHeader{id, static_cast<std::uint8_t>(priority)};
}

std::string make_log_line(const std::string& level, const std::string& message)
{
    const auto escape_field = [](const std::string& input) {
        constexpr char hex[] = "0123456789ABCDEF";
        std::string escaped;
        escaped.reserve(input.size());
        for (const char raw_byte : input) {
            const auto byte = static_cast<unsigned char>(raw_byte);
            switch (byte) {
                case '\\': escaped += "\\\\"; break;
                case '\n': escaped += "\\n"; break;
                case '\r': escaped += "\\r"; break;
                case '\t': escaped += "\\t"; break;
                default:
                    if (byte < 0x20U || byte == 0x7FU) {
                        escaped += "\\x";
                        escaped += hex[(byte >> 4U) & 0x0FU];
                        escaped += hex[byte & 0x0FU];
                    } else {
                        escaped += static_cast<char>(byte);
                    }
            }
        }
        return escaped;
    };

    std::ostringstream output;
    // 先產生一筆完整、已 escape 的 record。多執行緒 logger 應把整個字串放進 queue，
    // 由單一 writer 一次提交；flush 是 durability/latency policy，不應每筆無條件 std::endl。
    output << '[' << escape_field(level) << "] " << escape_field(message) << '\n';
    return output.str();
}

void practical_demo()
{
    std::ostringstream wire(std::ios::binary);
    encode_job(wire, JobHeader{0x01020304U, 7U});
    const std::string bytes = wire.str();
    assert(bytes.size() == 9U);

    std::istringstream input(bytes, std::ios::binary);
    const JobHeader decoded = decode_job(input);
    assert(decoded.id == 0x01020304U && decoded.priority == 7U);
    assert(make_log_line("INFO", "job accepted") == "[INFO] job accepted\n");
    assert(make_log_line("WARN\nFORGED", "bad\rvalue") ==
           "[WARN\\nFORGED] bad\\rvalue\n");
}

int main()
{
    basic_stream_demo();
    leetcode_demo();
    practical_demo();
    std::cout << "IOStream summary: all assertions passed\n";
}
