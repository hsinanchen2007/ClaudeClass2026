// =============================================================================
//  第 2 課：C 與 C++ 的關鍵差異 14  —  引用（Reference）：C++ 獨有的別名機制
// =============================================================================
//
// 【主題資訊 Information】
//   語法：void swap(int& a, int& b);   呼叫端直接寫 swap(x, y);
//   標準版本：C++98 起有左值引用 T&；C++11 新增右值引用 T&&。
//             C 語言完全沒有引用（只有指標）。
//   標頭檔：本檔用 <iostream>、<string>、<vector>。
//   複雜度：O(1)；引用本身不佔額外的抽象成本。
//
// 【詳細解釋 Explanation】
//
// 【1. 引用是什麼？—— 它是「別名」，不是「物件」】
//   int x = 10;
//   int& r = x;      // r 不是新變數，而是 x 的另一個名字
//   從此之後，對 r 做的任何事，就是對 x 做。r 沒有自己的身分：
//     &r == &x        （位址完全相同）
//     sizeof(r) 是 sizeof(int)，不是指標大小
//   這和指標是根本不同的東西：指標是一個「存放位址的變數」，
//   它有自己的位址、自己的大小；引用則是編譯期建立的一個名字對應關係。
//
// 【2. 引用的三條鐵律（全部源自「它是別名」）】
//   (a) 必須初始化。 int& r; 是編譯錯誤——別名不能沒有對象。
//   (b) 不能改綁。 r = y; 的意思是「把 y 的值賦給 x」，
//       而不是「讓 r 改指向 y」。引用一旦綁定，終生不變。
//   (c) 沒有空引用。 引用一定綁到某個物件（除非你已經先做了未定義行為）。
//   這三條合起來，就是引用相對指標的全部安全保證。
//
// 【3. 引用 vs 指標：該用哪一個？】
//   實務判準只有一句話：「這個參數可以不存在嗎？」
//     ● 一定要有值、且不會換對象  → 用引用（更簡潔，且不必檢查 NULL）
//     ● 可能沒有值（optional）    → 用指標，並在函式開頭檢查 NULL
//     ● 需要重新指向 / 指標算術   → 用指標
//   常見的參數傳遞慣例：
//     ● 小型且只讀（int、double）      → 傳值
//     ● 大型且只讀（string、vector）   → 傳 const T&（避免複製）
//     ● 需要修改呼叫端               → 傳 T&
//   本檔的實務範例會實測「傳值 vs 傳 const&」在複製次數上的差距。
//
// 【4. 引用的代價：呼叫端看不出來會被修改】
//   這是引用唯一真正的缺點，而且值得認真看待：
//       swap(x, y);        // 從這行完全看不出 x、y 會被改
//       swap(&x, &y);      // C 版本的 & 至少是個提示
//   Google C++ Style Guide 曾長期規定「輸出參數一律用指標」，
//   理由正是可讀性。後來的共識則是：優先用「回傳值」而不是輸出參數，
//   真的需要修改時用引用，並且靠命名（如 fillBuffer）讓意圖明顯。
//
// 【概念補充 Concept Deep Dive】
//   ● 引用在機器碼層級幾乎一定是用指標實作的，
//     但標準刻意不規定「引用是否佔用儲存空間」，
//     所以編譯器可以把它完全最佳化掉。
//     ★ 因此不要說「引用就是指標的語法糖」——在語意層它們是不同的東西，
//       只是常見實作恰好相同。面試時這個區分很加分。
//   ● const 引用可以綁定右值（臨時物件），並延長其生命週期：
//         const std::string& s = std::string("temp");   // 臨時物件活到 s 結束
//     非 const 引用不行。這是「傳 const& 給函式」能接受字面量的原因。
//   ● 引用的引用不存在。T& & 會透過「引用摺疊（reference collapsing）」
//     規則塌陷成 T&，這是 C++11 完美轉發（perfect forwarding）的基礎。
//   ● std::swap 在 C++11 之後是用 move 實作的：
//         T tmp = std::move(a); a = std::move(b); b = std::move(tmp);
//     對 std::string、std::vector 這種持有堆積資源的型別，
//     交換只是搬指標，不會複製內容。自己手寫 temp 版本則可能觸發深複製。
//
// 【注意事項 Pay Attention】
//   1. 引用必須初始化、不能改綁、沒有 NULL——這三點是它的全部價值來源。
//   2. 絕不要回傳「區域變數的引用」（函式結束後物件已銷毀，成為懸空引用）。
//      這是本課程第 18 課的重點，gcc 會以 -Wreturn-local-addr 警告。
//   3. 傳大型物件時用 const T&；但對 int、char 這種小型別，
//      傳值反而比傳引用快（少一層間接存取）。
//   4. 引用不是「零成本的萬用解」：把成員宣告成引用會讓類別無法被賦值
//      （因為引用不能改綁），通常應該改用指標或 std::reference_wrapper。
//   5. 本檔自訂的全域 swap(int&,int&) 與 std::swap 並不衝突，
//      因為本檔沒有寫 using namespace std。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】引用（Reference）
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 引用和指標的差別是什麼？什麼時候該選哪一個？
//     答：引用是「別名」——必須初始化、不能改綁、不存在空引用，
//         sizeof 與取址都會作用在被綁定的物件上。指標是「存位址的變數」，
//         可為 NULL、可改指向、可做算術，自己也有位址與大小。
//         判準：參數「可以不存在」就用指標並檢查 NULL；
//         「一定存在且不換對象」就用引用。
//     追問：引用底層是不是就是指標？
//         → 常見實作是，但標準不規定引用是否佔儲存空間，
//           編譯器可以完全最佳化掉。語意上它們是不同的東西，
//           說「引用是指標的語法糖」並不精確。
//
// 🔥 Q2. 為什麼 const T& 可以綁定臨時物件，而 T& 不行？
//     答：因為修改一個即將消失的臨時物件毫無意義，多半是筆誤，
//         所以標準禁止非 const 左值引用綁右值。const 引用綁定臨時物件時，
//         該臨時物件的生命週期會被延長到引用的作用域結束。
//         這正是「傳 const std::string& 可以直接吃字面量」的原因。
//     追問：那生命週期延長有例外嗎？
//         → 有。若把臨時物件綁到「函式回傳的引用」或建構函式初始化列表中的
//           成員引用，延長規則不適用，仍然會懸空。
//
// ⚠️ 陷阱. int& r = x; 之後寫 r = y;，r 會改成 y 的別名嗎？
//     答：不會。r 永遠是 x 的別名。r = y; 的意思是「把 y 的值賦給 x」，
//         執行後 x 的值變成 y 的值，而 r 仍然綁在 x 上。
//         引用一旦綁定就終生不變，語言沒有任何語法可以改綁。
//     為什麼會錯：因為指標可以 p = &y 改指向，大家就把同樣的心智模型
//         套到引用上，以為 = 對引用也是「重新綁定」。
//         實際上引用把 = 完整地轉交給被綁定的物件，
//         引用本身在初始化之後就再也不是賦值的對象了。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <vector>

void swap(int& a, int& b) {  // 使用引用
    int temp = a;
    a = b;
    b = temp;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 1】LeetCode 344. Reverse String
//   題目：就地反轉字元陣列，不可額外配置陣列空間。
//   為什麼用到本主題：核心動作是「交換頭尾元素」。用引用寫出來的
//         swapChar(s[l], s[r]) 不需要任何 & 取址，
//         和 13.cpp 的 swapChar(&s[l], &s[r]) 一比，
//         就是本課「引用讓語法更簡潔」最直接的證據。
//   複雜度：時間 O(n)，空間 O(1)。
// -----------------------------------------------------------------------------
void swapChar(char& a, char& b) {
    char temp = a;
    a = b;
    b = temp;
}

void reverseString(std::vector<char>& s) {
    size_t left = 0;
    size_t right = s.size();
    while (left + 1 < right) {          // right 是「尾端後一格」，避免無號數下溢
        swapChar(s[left], s[right - 1]);
        ++left;
        --right;
    }
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 2】LeetCode 283. Move Zeroes
//   題目：把陣列中所有 0 移到末尾，其餘元素維持相對順序，必須就地完成。
//   為什麼用到本主題：參數是 std::vector<int>&，函式直接修改呼叫端的容器
//         而不複製——這正是引用最主流的用途（大型物件 + 需要修改）。
//         若寫成傳值，改的就只是複本，題目要求的「就地」完全做不到。
//   複雜度：時間 O(n)，空間 O(1)。
// -----------------------------------------------------------------------------
void moveZeroes(std::vector<int>& nums) {
    size_t insertPos = 0;
    for (size_t i = 0; i < nums.size(); ++i) {
        if (nums[i] != 0) {
            std::swap(nums[insertPos], nums[i]);   // std::swap 也是吃引用
            ++insertPos;
        }
    }
}

// -----------------------------------------------------------------------------
// 【日常實務範例】傳值 vs 傳 const& 的實際複製次數（可量測，不是空談）
//   情境：把一筆「使用者設定」傳進函式做唯讀檢查。設定物件通常不小，
//         用傳值會整包複製一次，用 const& 則一次都不複製。
//   下面用一個會自報複製次數的類別實測，讓差異變成可觀察的數字，
//   而不是「據說比較快」。
// -----------------------------------------------------------------------------
struct Payload {
    std::string data;
    static int copyCount;                       // 統計被複製了幾次

    explicit Payload(std::string d) : data(std::move(d)) {}
    Payload(const Payload& other) : data(other.data) { ++copyCount; }
    Payload& operator=(const Payload&) = delete;  // 本範例不需要賦值
};
int Payload::copyCount = 0;

bool isValidByValue(Payload p) {                // 傳值：整包複製一份
    return !p.data.empty();
}

bool isValidByConstRef(const Payload& p) {      // 傳 const&：零複製
    return !p.data.empty();
}

int main() {
    int x = 10, y = 20;
    swap(x, y);  // 不需要 &，語法更簡潔
    std::cout << "x = " << x << ", y = " << y << std::endl;

    std::cout << "\n=== 引用是別名：位址完全相同 ===" << std::endl;
    int value = 42;
    int& alias = value;
    alias = 99;                                  // 改 alias 就是改 value
    std::cout << "  改 alias 之後 value = " << value << std::endl;
    std::cout << "  &alias == &value ? "
              << ((&alias == &value) ? "true" : "false") << std::endl;
    std::cout << "  sizeof(alias) == sizeof(int) ? "
              << ((sizeof(alias) == sizeof(int)) ? "true" : "false") << std::endl;

    std::cout << "\n=== LeetCode 344. Reverse String ===" << std::endl;
    {
        std::vector<char> s = {'h', 'e', 'l', 'l', 'o'};
        std::cout << "  before: ";
        for (char c : s) std::cout << c;
        reverseString(s);
        std::cout << "\n  after : ";
        for (char c : s) std::cout << c;
        std::cout << std::endl;
    }

    std::cout << "\n=== LeetCode 283. Move Zeroes ===" << std::endl;
    {
        std::vector<int> nums = {0, 1, 0, 3, 12};
        std::cout << "  before: [";
        for (size_t i = 0; i < nums.size(); ++i)
            std::cout << nums[i] << (i + 1 < nums.size() ? "," : "");
        moveZeroes(nums);
        std::cout << "]\n  after : [";
        for (size_t i = 0; i < nums.size(); ++i)
            std::cout << nums[i] << (i + 1 < nums.size() ? "," : "");
        std::cout << "]" << std::endl;
    }

    std::cout << "\n=== 日常實務：傳值 vs 傳 const& 的複製次數 ===" << std::endl;
    {
        Payload cfg("theme=dark;lang=zh-TW;retries=3");

        Payload::copyCount = 0;
        isValidByValue(cfg);
        std::cout << "  傳值    isValidByValue(cfg)    複製次數 = "
                  << Payload::copyCount << std::endl;

        Payload::copyCount = 0;
        isValidByConstRef(cfg);
        std::cout << "  傳const& isValidByConstRef(cfg) 複製次數 = "
                  << Payload::copyCount << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 2 課：C 與 C++ 的關鍵差異14.cpp" -o diff14

// === 預期輸出 ===
// x = 20, y = 10
//
// === 引用是別名：位址完全相同 ===
//   改 alias 之後 value = 99
//   &alias == &value ? true
//   sizeof(alias) == sizeof(int) ? true
//
// === LeetCode 344. Reverse String ===
//   before: hello
//   after : olleh
//
// === LeetCode 283. Move Zeroes ===
//   before: [0,1,0,3,12]
//   after : [1,3,12,0,0]
//
// === 日常實務：傳值 vs 傳 const& 的複製次數 ===
//   傳值    isValidByValue(cfg)    複製次數 = 1
//   傳const& isValidByConstRef(cfg) 複製次數 = 0
