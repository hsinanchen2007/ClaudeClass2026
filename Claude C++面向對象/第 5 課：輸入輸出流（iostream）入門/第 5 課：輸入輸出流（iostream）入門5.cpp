// =============================================================================
//  第 5 課：輸入輸出流（iostream）入門5.cpp  —  std::cin >> 讀取數值與失敗處理
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<iostream>
//   簽名（成員版，數值型別）：
//     std::istream& std::istream::operator>>(int&    value);
//     std::istream& std::istream::operator>>(double& value);
//   狀態查詢：
//     bool good() / fail() / eof() / bad();   explicit operator bool() const;
//   標準版本：C++98 起有 operator>>；
//             「讀取失敗時把變數設為 0」是 C++11 才加入的規定（見【3.】）。
//   複雜度：O(讀取的字元數)。
//
// 【詳細解釋 Explanation】
//
// 【1. operator>> 讀 int 時到底做了什麼】
//   步驟固定是三段：
//     (1) 跳過前導空白（whitespace：空格、tab、換行…）。
//         這是因為預設開啟 std::ios_base::skipws 這個 format flag。
//     (2) 盡可能多地讀取「看起來像一個整數」的字元（可含正負號與數字）。
//     (3) 把讀到的字元轉成數值存進變數；停在第一個不合法的字元前面，
//         而那個字元「留在輸入緩衝區裡」不會被吃掉。
//   第 (3) 步的「留在緩衝區」是後續所有詭異行為的根源。
//
// 【2. 三種結果，不是兩種】
//     * 成功       ：讀到合法數值，stream 保持 good。
//     * 失敗(fail) ：第一個非空白字元就不合法（例如使用者打 "abc"）。
//                    設定 failbit，且輸入位置不前進——那些字元還在。
//     * 到檔尾(eof)：沒有東西可讀。設定 eofbit，通常同時設 failbit。
//   注意 fail 之後 stream 會「凍結」：後續所有 >> 立刻返回、什麼都不做。
//   這就是為什麼一個沒處理的輸入錯誤，會讓後面的讀取全部失效，
//   而且如果外層還有 while 迴圈，就會變成無窮迴圈（見【面試題】陷阱）。
//
// 【3. 讀取失敗時，變數的值是多少？——C++11 改過規則】
//   * C++98/03：失敗時「不修改」該變數。
//                若變數本來未初始化，讀取失敗後它仍是未定值，
//                之後拿來運算就是 undefined behavior。
//   * C++11 起 ：失敗時對該變數做 value-initialization，也就是設為 0。
//                （eofbit 的情況同樣適用。）
//   所以本檔原始範例的 `int age;` 雖然沒初始化，在 C++11 之後的編譯器下
//   即使讀取失敗也會是 0，不是垃圾值。
//   但好習慣仍然是明確初始化 + 檢查 stream 狀態，別依賴這條規則。
//
// 【4. 失敗之後怎麼復原】
//   兩步，缺一不可：
//     std::cin.clear();                                            // 1) 清掉錯誤旗標
//     std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // 2) 丟掉壞資料
//   只做 clear 不做 ignore，壞字元還在緩衝區裡，下次 >> 又立刻失敗。
//   這正是新手迴圈變無窮迴圈的原因。
//
// 【概念補充 Concept Deep Dive】
//   (A) 為什麼 if (std::cin >> age) 可以直接當條件用？
//       basic_ios 定義了 explicit operator bool()，回傳 !fail()。
//       因為 operator>> 回傳 istream&，所以 (cin >> age) 的結果是 stream 本身，
//       在 if 的條件式中會做 contextual conversion to bool。
//       「explicit」是關鍵：它防止 stream 被意外隱式轉成 int 之類的型別。
//       （C++98/03 時代這裡是 operator void*()，C++11 改成 explicit operator bool。）
//
//   (B) 不要用 while (!cin.eof()) 當迴圈條件
//       eofbit 是「已經嘗試讀取並碰到檔尾」之後才會設定，不是「預測」下次會失敗。
//       所以最後一筆資料處理完時 eof 還沒設，迴圈會多跑一輪，
//       用到一筆沒讀成功的資料。正確寫法永遠是 while (cin >> x)。
//
//   (C) >> 不讀空白，getline 才讀整行
//       因為 skipws 與「遇空白即停」的規則，>> 永遠只能讀到一個「詞」。
//       要讀含空白的整行必須用 std::getline——這是下一個檔案的主題。
//
//   (D) 提示訊息為什麼不用 flush 就會出現
//       std::cin.tie() 預設指向 &std::cout，
//       標準規定從 cin 讀取前會自動 flush 被 tie 的 ostream。
//
// 【注意事項 Pay Attention】
//   1. 一定要檢查讀取是否成功，別假設使用者輸入合法。
//   2. 失敗後要 clear() + ignore()，只做其中一個沒有用。
//   3. C++11 起讀取失敗會把變數設為 0；C++98 則是保持原值（可能是未定值）。
//      不要在教材或程式碼中把「失敗後一定是 0」當成永恆真理，要標明標準版本。
//   4. 別用 while (!cin.eof())；用 while (cin >> x)。
//   5. 輸入 "25abc" 時，>> 會成功讀到 25，"abc" 留在緩衝區——
//      這不是錯誤，但常常是使用者本意以外的結果，需要額外驗證。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::cin >> 的失敗處理
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::cin >> age 讀取失敗時會發生什麼事？age 的值是多少？
//     答：設定 failbit，且輸入位置不前進（壞字元還留在緩衝區）。
//         C++11 起規定失敗時對變數做 value-initialization，也就是 age = 0；
//         C++98/03 則是不修改變數，若原本未初始化就會是未定值。
//         正確做法都是先檢查 if (std::cin >> age) 再使用。
//     追問：那要怎麼從失敗狀態復原？
//         → std::cin.clear() 清旗標，再用 std::cin.ignore(max, '\n') 丟掉壞資料，
//           兩者缺一不可。
//
// 🔥 Q2. 為什麼可以寫 if (std::cin >> x) ？其中發生了什麼轉換？
//     答：operator>> 回傳 std::istream&，而 basic_ios 定義了
//         explicit operator bool()（回傳 !fail()）。
//         if 的條件式屬於 contextual conversion to bool，允許呼叫 explicit 版本。
//         標成 explicit 是為了避免 stream 被意外隱式轉型成整數等型別。
//
// ⚠️ 陷阱. 下面這段在使用者輸入 "abc" 時為什麼會變成無窮迴圈？
//         int n;
//         while (true) {
//             std::cout << "輸入數字: ";
//             if (std::cin >> n) break;
//             std::cin.clear();          // 只清了旗標
//         }
//     答：clear() 只把 failbit 清掉，"abc" 這三個字元仍留在輸入緩衝區。
//         下一輪 >> 又碰到同樣的 'a'，又立刻失敗，如此無限循環，
//         而且畫面會被提示字洗版。
//         必須再加 std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
//         把該行剩餘內容丟掉。
//     為什麼會錯：大家把 clear() 理解成「清空輸入」，但它的意思是
//         「清除錯誤狀態旗標」，跟緩衝區內容完全無關。
//         名字取得容易誤解，這也是這題常年出現在面試題庫的原因。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <limits>
#include <sstream>
#include <string>

// -----------------------------------------------------------------------------
// 【日常實務範例】解析設定檔中的數值欄位，並回報是否合法
// 情境：讀 server.conf 這類設定檔，每行形如 "port=8080"。
//       設定值來自外部，永遠不能假設它是合法數字——
//       "port=abc" 或 "port=" 都必須被明確判定為錯誤並回報，
//       而不是靜默地當成 0（那會讓服務綁到 port 0 這種難以追查的狀況）。
// 為何用本主題：這裡示範的正是 operator>> 的成功/失敗判定，
//       以及「讀完之後要確認沒有殘留字元」這個容易被忽略的第二道檢查。
// -----------------------------------------------------------------------------
bool parseIntSetting(const std::string& text, int& out) {
    std::istringstream iss(text);
    int value = 0;

    if (!(iss >> value)) {
        return false;              // 根本不是數字，例如 "abc" 或空字串
    }

    // 第二道檢查：確認後面沒有殘留的非空白字元。
    // 少了這一步，"8080abc" 會被當成合法的 8080——這是很常見的漏洞。
    char leftover = '\0';
    if (iss >> leftover) {
        return false;              // 還有東西沒讀完，視為格式錯誤
    }

    out = value;
    return true;
}

// 【LeetCode 實戰範例】—— 本檔從缺，並說明理由
//   本檔主題是「輸入解析與 stream 錯誤狀態管理」，屬於 I-O 與防禦性程式設計。
//   LeetCode 題目的輸入由評測系統直接以參數形式給定、且保證格式合法，
//   完全不涉及 stream 狀態處理。硬套一題只會模糊焦點，故留白。

int main() {
    // ── 原始教學範例：從 std::cin 讀一個整數 ───────────────────
    std::cout << "=== 原始範例: 從 cin 讀年齡 ===" << std::endl;

    int age = 0;                    // 明確初始化，不依賴 C++11 的失敗歸零規則

    std::cout << "請輸入你的年齡: ";
    // 提示不需手動 flush：std::cin 預設 tie 到 std::cout
    if (std::cin >> age) {
        std::cout << "\n你的年齡是: " << age << " 歲" << std::endl;
    } else {
        // 沒有輸入（例如直接 EOF）或輸入不是數字時走這裡
        std::cout << "\n讀取失敗：輸入不是合法的整數。\n";
        std::cout << "C++11 起規定此時 age 會被設為 0，實際值: " << age << '\n';
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }

    // ── 用 istringstream 做決定性的失敗示範 ────────────────────
    // 這裡改用 istringstream 而非 cin，因為它的內容由程式自己決定，
    // 不受執行時輸入影響，輸出才能穩定重現。
    std::cout << "\n=== operator>> 的三種結果 ===" << std::endl;

    auto tryRead = [](const std::string& input) {
        std::istringstream iss(input);
        int v = 0;
        const bool ok = static_cast<bool>(iss >> v);
        std::cout << "  輸入 \"" << input << "\" -> "
                  << (ok ? "成功" : "失敗")
                  << "，值 = " << v
                  << "，fail=" << std::boolalpha << iss.fail()
                  << "，eof=" << iss.eof() << std::noboolalpha << '\n';
    };

    tryRead("42");        // 正常
    tryRead("   42");     // 前導空白會被 skipws 跳過
    tryRead("42abc");     // 成功讀到 42，abc 留在後面
    tryRead("abc");       // 第一個字元就不合法 -> fail
    tryRead("");          // 沒東西可讀 -> eof + fail

    // ── clear 與 ignore 的差別 ────────────────────────────────
    std::cout << "\n=== 為什麼 clear() 不夠、還要 ignore() ===" << std::endl;
    {
        std::istringstream iss("abc 99");
        int v = 0;

        iss >> v;   // 失敗
        std::cout << "  第一次讀取: fail=" << std::boolalpha << iss.fail()
                  << std::noboolalpha << '\n';

        iss.clear();                 // 只清旗標，"abc" 還在
        iss >> v;
        std::cout << "  只 clear() 後再讀: fail=" << std::boolalpha << iss.fail()
                  << std::noboolalpha << "  <- 又失敗了，因為壞字元還在\n";

        iss.clear();
        iss.ignore(std::numeric_limits<std::streamsize>::max(), ' ');  // 丟掉 "abc"
        const bool ok = static_cast<bool>(iss >> v);
        std::cout << "  clear() + ignore() 後: 成功=" << std::boolalpha << ok
                  << std::noboolalpha << "，讀到 " << v << '\n';
    }

    // ── 日常實務：設定檔數值解析 ──────────────────────────────
    std::cout << "\n=== 日常實務: 解析設定值 ===" << std::endl;
    const std::string samples[] = {"8080", "  443", "abc", "", "8080abc", "-1"};
    for (const std::string& s : samples) {
        int port = -12345;
        const bool ok = parseIntSetting(s, port);
        std::cout << "  port=\"" << s << "\" -> "
                  << (ok ? "合法，值 = " + std::to_string(port)
                         : std::string("不合法（已拒絕，不會誤當成 0）"))
                  << '\n';
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 5 課：輸入輸出流（iostream）入門5.cpp" -o io5
// 執行: echo 25 | ./io5
//   （本程式第一段會從標準輸入讀一個整數；下方預期輸出是以 `echo 25` 餵入所得。
//     若不給輸入直接執行，第一段會走「讀取失敗」分支並印出 age = 0，
//     其餘各段輸出不變。）

// 說明（標準版本相關，非不確定行為）：
//   * 「讀取失敗時變數被設為 0」是 C++11 起的規定；在 C++98/03 下變數會保持原值。
//     本檔已明確初始化 age = 0，因此兩種標準下行為都一致，不會出現未定值。
//   * tryRead("") 的情況同時設定 eofbit 與 failbit，這是標準規定的行為。
//   * tryRead("42abc") 成功讀到 42 並非錯誤，而是 operator>> 的既定語意：
//     它只讀到第一個不合法字元為止，其餘留在 stream 中。
//   * 本程式輸出完全決定性（僅第一段取決於餵入的輸入），已多次執行確認一致。

// === 預期輸出 ===
// === 原始範例: 從 cin 讀年齡 ===
// 請輸入你的年齡: 
// 你的年齡是: 25 歲
//
// === operator>> 的三種結果 ===
//   輸入 "42" -> 成功，值 = 42，fail=false，eof=true
//   輸入 "   42" -> 成功，值 = 42，fail=false，eof=true
//   輸入 "42abc" -> 成功，值 = 42，fail=false，eof=false
//   輸入 "abc" -> 失敗，值 = 0，fail=true，eof=false
//   輸入 "" -> 失敗，值 = 0，fail=true，eof=true
//
// === 為什麼 clear() 不夠、還要 ignore() ===
//   第一次讀取: fail=true
//   只 clear() 後再讀: fail=true  <- 又失敗了，因為壞字元還在
//   clear() + ignore() 後: 成功=true，讀到 99
//
// === 日常實務: 解析設定值 ===
//   port="8080" -> 合法，值 = 8080
//   port="  443" -> 合法，值 = 443
//   port="abc" -> 不合法（已拒絕，不會誤當成 0）
//   port="" -> 不合法（已拒絕，不會誤當成 0）
//   port="8080abc" -> 不合法（已拒絕，不會誤當成 0）
//   port="-1" -> 合法，值 = -1
