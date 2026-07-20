// =============================================================================
//  第 16 課：vector 的迭代器操作 9  —  範圍 for 的三種接收方式：值、引用、常引用
// =============================================================================
//
// 【主題資訊 Information】
//   for (T x : v)          // 值：每輪拷貝一份，改 x 不影響容器
//   for (T& x : v)         // 引用：直接操作容器內的元素
//   for (const T& x : v)   // 常引用：不拷貝也不可改（唯讀首選）
//   for (auto&& x : v)     // 萬用引用：泛型程式碼與 proxy 容器（如 vector<bool>）
//   標準版本：C++11
//   複雜度：值語意每輪多一次拷貝建構 + 一次解構；引用語意零額外成本
//
// 【詳細解釋 Explanation】
//
// 【1. 三種寫法的差別只有一處：declaration = *__begin 這一行】
//   回顧第 8 個檔案的展開，範圍 for 每輪都執行 declaration = *__begin。
//   你在 declaration 寫什麼，就決定了這一行是拷貝還是綁定：
//       T x        → 從 *it 拷貝建構出一個新物件 x
//       T& x       → x 成為 *it 的別名，讀寫都直接作用在容器上
//       const T& x → 同上但唯讀
//   語言層面就這麼單純，複雜的是「選錯的後果」。
//
// 【2. 選錯的後果一：值語意的靜默失效】
//   for (std::string name : names) { name += "!"; }
//   這段程式碼會編譯成功、執行成功、而且什麼都沒改到 —— 每輪修改的是副本，
//   迴圈結束就被丟棄。沒有警告，沒有錯誤，只有「功能沒生效」。
//   這類 bug 在 code review 時特別難抓，因為程式碼讀起來完全合理。
//
// 【3. 選錯的後果二：值語意的效能成本】
//   對 vector<std::string> 而言，每輪的拷貝可能包含一次堆積配置與一次
//   memcpy（短字串因 SSO 而不配置堆積，但仍要複製整個物件）。
//   十萬筆資料就是十萬次不必要的配置與釋放。這在 profiler 上看得見，
//   但在原始碼上完全看不出來 —— 差別只有一個 & 符號。
//
// 【4. 選擇準則】
//       要修改元素                 → auto&
//       只讀、元素是 int/double 等 → auto（拷貝比取址還便宜）
//       只讀、元素較大（string、struct、vector）→ const auto&
//   實務上把「只讀就寫 const auto&」當成預設習慣最省心：對純量型別它也
//   不會比較慢（編譯器會處理掉），卻能讓大型元素永遠不被誤複製。
//
// 【概念補充 Concept Deep Dive】
//   有一個容器會讓 auto& 直接編譯失敗：std::vector<bool>。
//   它是標準特意做的空間最佳化特化，一個 bool 只佔 1 bit，
//   因此 operator* 無法回傳 bool&（位元沒有位址），只好回傳一個
//   proxy 物件 std::vector<bool>::reference。那是個「暫時值（rvalue）」，
//   而非 const 的左值引用不能綁定 rvalue，於是：
//       for (auto& b : vb) b = !b;    // 本機實測編譯錯誤：
//                                     // cannot bind non-const lvalue reference
//                                     // of type std::_Bit_reference& to an rvalue
//       for (auto&& b : vb) b = !b;   // OK：萬用引用可綁 rvalue，proxy 照樣能寫回
//   這是 vector<bool> 惡名昭彰的原因之一 —— 它不滿足一般容器的行為契約。
//   泛型程式碼（不知道元素型別是什麼）因此常寫 auto&&，以同時涵蓋
//   一般容器與 proxy 容器。
//
// 【注意事項 Pay Attention】
//   1. for (T x : v) 修改的是副本，對容器完全沒有效果，且不會有任何警告。
//   2. 對大型元素用值語意會產生每輪拷貝，是實務上常見的效能問題。
//   3. std::vector<bool> 不能用 auto&，要用 auto&&（見上）。
//   4. 三種寫法都不允許在迴圈中改變容器結構（push_back/erase）——
//      那與接收方式無關，是迭代器失效問題。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】範圍 for 的接收方式
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. for (auto x : v)、for (auto& x : v)、for (const auto& x : v) 各用在什麼時候？
//     答：要修改元素用 auto&；只讀且元素是純量（int、double）用 auto，
//         拷貝成本比取址還低；只讀且元素較大（string、struct）用 const auto&，
//         避免每輪一次完整拷貝。把「只讀就寫 const auto&」當預設習慣最安全。
//     追問：對 vector<int> 寫 const auto& 會比 auto 慢嗎？
//         → 實務上不會，編譯器最佳化後兩者等價；但語意更明確地表達了唯讀。
//
// ⚠️ 陷阱 1. 這段程式碼為什麼「執行成功但沒有效果」？
//         for (std::string name : names) name += "!";
//     答：宣告的是值，每輪從元素拷貝建構出一個新字串，修改的是這個副本，
//         迴圈結束就銷毀。容器完全沒被動到，而且不會有任何編譯警告。
//         要真的改到容器必須寫 for (std::string& name : names)。
//     為什麼會錯：腦中把範圍 for 想成「取出容器裡的那個元素」，
//         但展開後是 declaration = *__begin —— 沒寫 & 就是一次拷貝賦值。
//
// ⚠️ 陷阱 2. for (auto& b : vb) b = !b; 對 std::vector<bool> 為什麼編譯不過？
//     答：vector<bool> 是位元壓縮特化，一個元素只佔 1 bit，沒有獨立位址，
//         operator* 只好回傳 proxy 物件 std::vector<bool>::reference（一個 rvalue）。
//         非 const 的左值引用不能綁 rvalue，本機實測錯誤訊息是
//         "cannot bind non-const lvalue reference ... to an rvalue"。
//         改用 auto&&（萬用引用）即可，proxy 一樣能寫回底層位元。
//     為什麼會錯：大家預期 vector<bool> 只是「元素型別是 bool 的 vector」，
//         但它其實不滿足一般容器契約 —— 這是標準自己承認的設計失誤。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <string>

void demo_three_forms() {
    std::vector<std::string> names = {"Alice", "Bob", "Charlie"};

    // 1. 拷貝（值語意）—— 不會修改原容器
    std::cout << "值拷貝：" << std::endl;
    for (std::string name : names) {
        name += "!";  // 修改的是拷貝
        std::cout << "  " << name << std::endl;
    }

    // 驗證原容器未被修改
    std::cout << "原容器：" << names[0] << std::endl;  // Alice（沒有 !）

    // 2. 引用 —— 可以修改原容器
    for (std::string& name : names) {
        name += "!";  // 修改原容器中的元素
    }
    std::cout << "修改後：" << names[0] << std::endl;   // Alice!

    // 3. const 引用 —— 不拷貝、不修改（最常用的唯讀方式）
    for (const std::string& name : names) {
        std::cout << name << " ";
        // name += "?";  // 編譯錯誤！
    }
    std::cout << std::endl;
}

void demo_vector_bool() {
    std::vector<bool> flags = {true, false, true, false};

    std::cout << "原始：";
    for (bool b : flags) std::cout << b << " ";
    std::cout << std::endl;

    // for (auto& b : flags) b = !b;   // 編譯錯誤：proxy 是 rvalue，auto& 綁不上
    for (auto&& b : flags) b = !b;     // 萬用引用可以，proxy 能寫回底層位元

    std::cout << "全部反轉後：";
    for (bool b : flags) std::cout << b << " ";
    std::cout << std::endl;
    std::cout << "（vector<bool> 是位元壓縮特化，一個元素只佔 1 bit）" << std::endl;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】批次記錄正規化：把使用者匯入的名單統一成標準格式
//   情境：從外部系統匯入的會員名單，Email 大小寫混亂、姓名帶多餘空白，
//         入庫前要統一：Email 全轉小寫、姓名去除前後空白。
//   為什麼用到本主題：正規化必須「就地改寫」原始資料，所以走訪用 auto&。
//         若寫成 auto，整個函式會安靜地什麼都不做 —— 編譯通過、測試若只檢查
//         「函式有沒有拋例外」也會通過，直到有人發現資料庫裡的 Email 還是大寫。
//         而後面統計用的走訪只讀不寫，就用 const auto& 避免複製整個結構。
// -----------------------------------------------------------------------------
struct Member {
    std::string name;
    std::string email;
};

void normalizeMembers(std::vector<Member>& members) {
    for (auto& m : members) {              // auto&：必須改到本體
        // Email 轉小寫
        for (auto& ch : m.email) {
            if (ch >= 'A' && ch <= 'Z') ch = static_cast<char>(ch - 'A' + 'a');
        }
        // 姓名去前後空白
        const std::string ws = " \t";
        std::size_t b = m.name.find_first_not_of(ws);
        if (b == std::string::npos) { m.name.clear(); continue; }
        std::size_t e = m.name.find_last_not_of(ws);
        m.name = m.name.substr(b, e - b + 1);
    }
}

std::size_t countCompanyMails(const std::vector<Member>& members, const std::string& domain) {
    std::size_t n = 0;
    for (const auto& m : members) {        // const auto&：唯讀且零拷貝
        if (m.email.size() >= domain.size() &&
            m.email.compare(m.email.size() - domain.size(), domain.size(), domain) == 0) {
            ++n;
        }
    }
    return n;
}

int main() {
    std::cout << "=== 三種接收方式 ===" << std::endl;
    demo_three_forms();

    std::cout << "\n=== vector<bool> 的 proxy 特例 ===" << std::endl;
    demo_vector_bool();

    std::cout << "\n=== 日常實務：會員名單正規化 ===" << std::endl;
    std::vector<Member> members = {
        {"  Alice ",   "Alice@Example.COM"},
        {"Bob",        "BOB@corp.local"},
        {"\tCharlie ", "charlie@Example.com"}
    };

    std::cout << "正規化前：" << std::endl;
    for (const auto& m : members) std::cout << "  [" << m.name << "] " << m.email << std::endl;

    normalizeMembers(members);

    std::cout << "正規化後：" << std::endl;
    for (const auto& m : members) std::cout << "  [" << m.name << "] " << m.email << std::endl;

    std::cout << "example.com 網域人數 = " << countCompanyMails(members, "example.com") << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 16 課：vector 的迭代器操作9.cpp" -o iter9

// === 預期輸出 ===
// === 三種接收方式 ===
// 值拷貝：
//   Alice!
//   Bob!
//   Charlie!
// 原容器：Alice
// 修改後：Alice!
// Alice! Bob! Charlie! 
//
// === vector<bool> 的 proxy 特例 ===
// 原始：1 0 1 0 
// 全部反轉後：0 1 0 1 
// （vector<bool> 是位元壓縮特化，一個元素只佔 1 bit）
//
// === 日常實務：會員名單正規化 ===
// 正規化前：
//   [  Alice ] Alice@Example.COM
//   [Bob] BOB@corp.local
//   [	Charlie ] charlie@Example.com
// 正規化後：
//   [Alice] alice@example.com
//   [Bob] bob@corp.local
//   [Charlie] charlie@example.com
// example.com 網域人數 = 2
