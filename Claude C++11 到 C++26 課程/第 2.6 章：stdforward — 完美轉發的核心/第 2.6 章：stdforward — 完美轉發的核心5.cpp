// =============================================================================
//  第 2.6 章 範例 5  —  emplace_back vs push_back：完美轉發最常見的落地形式
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<vector>
//   push_back    : void push_back(const T& value);  /  void push_back(T&& value);   (C++11)
//   emplace_back : template<class... Args> reference emplace_back(Args&&... args);  (C++11)
//                  （C++17 起回傳 reference，C++11/14 回傳 void）
//   兩者攤銷複雜度都是 O(1)。差別不在複雜度，而在「建構了幾次物件」。
//
// 【詳細解釋 Explanation】
//
// 【1. 兩者的本質差異：傳「物件」vs 傳「建構子的引數」】
//   push_back 的參數型別是 T——你必須先有一個 T 物件，它再把那個物件
//   複製或移動進容器。
//   emplace_back 的參數是 Args&&...——你交出的是「建構 T 需要的原料」，
//   容器直接在自己的記憶體上用 placement new 就地建構：
//       ::new (ptr) T(std::forward<Args>(args)...);
//   所以 emplace_back 少的那一次，是「臨時物件的建構 + 搬移 + 解構」。
//
// 【2. 逐行對照本檔的四個呼叫】
//   vec.push_back(Entry(key, 1));
//       ① 先建 Entry 臨時物件（key 是左值 → Entry(const string&, int)）
//       ② 臨時物件是右值 → 移動進 vector（Entry 沒寫移動建構子，
//          但有隱式生成的版本，故不印任何訊息）
//       ③ 臨時物件解構
//   vec.emplace_back(key, 3);
//       key 與 3 被轉發到 Entry 建構子；key 是左值 → Entry(const string&, int)
//       全程只有「容器內那一個」Entry，沒有臨時物件
//   vec.emplace_back(std::string("delta"), 4);
//       右值被保持成右值 → Entry(string&&, int) → 內部再 move 到成員，零複製
//
// 【3. 為什麼輸出看起來「行數一樣」，emplace 卻比較快】
//   Entry 只在「自訂的兩個建構子」裡印訊息，隱式生成的移動建構子不印。
//   所以 push_back 那次額外的移動與解構在輸出上是看不見的——這正是效能問題
//   難以靠肉眼發現的原因。要看見它，得像第 2.9 章那樣自己插入計數器。
//
// 【概念補充 Concept Deep Dive】
//   (A) reserve 的重要性：本檔先 reserve(4)，所以四次插入都不會觸發擴容。
//       若不 reserve，vector 成長時要把既有元素搬到新緩衝區——這一步是否用
//       移動，取決於元素的移動建構子是否 noexcept（見第 2.9 章）。
//       libstdc++ 的成長倍率是 2 倍，這是實作定義，不是標準規定
//       （MSVC 用 1.5 倍）。
//   (B) emplace_back 用的是「直接初始化」T(args...)，而非「複製初始化」T = args。
//       因此 explicit 建構子也能被呼叫——這既是彈性，也是風險：
//       v.emplace_back(10) 對 vector<std::vector<int>> 會建出一個長度 10 的向量，
//       而 v.push_back(10) 根本編譯不過。emplace 讓錯誤從編譯期跑到執行期。
//   (C) 什麼時候 push_back 反而比較好？當你手上「已經有一個 T 物件」時，
//       push_back(std::move(obj)) 語意更清楚，而且兩者成本相同。
//       emplace_back 的優勢只在「還沒有物件、要現場建」的情境。
//
// 【注意事項 Pay Attention】
//   1. emplace_back 可能使既有的 iterator/reference/pointer 失效（擴容時）。
//   2. emplace_back 會繞過 explicit 檢查，容易寫出「能編譯但語意錯」的程式碼。
//   3. C++17 起 emplace_back 回傳新元素的 reference；C++11/14 回傳 void，
//      舊標準下寫 auto& e = v.emplace_back(...) 會編譯失敗。
//   4. 自我插入要小心：v.emplace_back(v[0]) 在擴容時，v[0] 的參考可能先失效。
//      libstdc++ 對此有特別處理，但不要依賴這種邊角行為。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】emplace_back vs push_back
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. emplace_back 和 push_back 差在哪裡？為什麼 emplace 可能比較快？
//     答：push_back 收的是「已建好的物件」，要多一次複製或移動再加一次解構；
//         emplace_back 收的是「建構子的引數」，用完美轉發在容器記憶體上
//         直接 placement new，省掉臨時物件那一整輪。
//     追問：已經有物件時，push_back(std::move(x)) 和 emplace_back(std::move(x))
//         誰快？→ 一樣快，兩者都只做一次移動；此時該選語意較清楚的 push_back。
//
// 🔥 Q2. emplace_back 一定比 push_back 快嗎？
//     答：不一定。當引數本來就是同型別的物件時兩者等價。
//         而且 emplace_back 用直接初始化，會讓 explicit 建構子也參與，
//         可能把「本來該編譯錯」的程式碼變成能跑但語意錯的程式碼。
//
// ⚠️ 陷阱. std::vector<std::vector<int>> v; v.emplace_back(10); 會發生什麼？
//     答：建出一個「含 10 個 0 的 vector<int>」當作新元素，
//         而不是塞入數值 10。因為 emplace 走直接初始化，
//         explicit vector(size_type) 這個建構子被選中了。
//     為什麼會錯：大家把 emplace_back 讀成「push_back 的加速版」，
//         以為參數語意相同；實際上它的參數是「建構子引數」，
//         同樣的 10 在兩個函式裡代表完全不同的意思——
//         push_back(10) 直接編譯失敗，emplace_back(10) 卻默默照做。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <vector>

class Entry {
    std::string key_;
    int value_;
public:
    Entry(const std::string& k, int v) : key_(k), value_(v) {
        std::cout << "  Entry(const string&, int)\n";
    }
    Entry(std::string&& k, int v) : key_(std::move(k)), value_(v) {
        std::cout << "  Entry(string&&, int)\n";
    }
    void print() const { std::cout << "    " << key_ << " = " << value_ << "\n"; }
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 118. Pascal's Triangle
//   題目：給定列數 numRows，回傳巴斯卡三角形的前 numRows 列。
//   為什麼用到本主題：每一列都是一個「現場算出來的 vector<int>」。
//     用 res.push_back(row) 會複製整列；用 res.push_back(std::move(row)) 要自己記得
//     搬移；而 res.emplace_back(i + 1, 1) 直接把「長度、初值」轉發給
//     vector<int> 的建構子，在結果容器裡就地生出這一列，完全沒有臨時 vector。
//     這是 emplace_back 完美轉發在演算法題中最自然的用法。
//   複雜度：時間 O(numRows^2)，空間 O(numRows^2)（即輸出本身）。
// -----------------------------------------------------------------------------
std::vector<std::vector<int>> generatePascalTriangle(int numRows) {
    std::vector<std::vector<int>> res;
    res.reserve(static_cast<std::size_t>(numRows));   // 先備好空間，避免擴容搬移

    for (int i = 0; i < numRows; ++i) {
        // ★ 把「大小 i+1、初值 1」轉發給 std::vector<int>(count, value)，就地建構
        res.emplace_back(i + 1, 1);
        for (int j = 1; j < i; ++j) {
            res[i][j] = res[i - 1][j - 1] + res[i - 1][j];
        }
    }
    return res;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】把 key=value 設定檔逐行解析進設定表
//   情境：讀 app.conf 這類設定檔，每行 "key=value"。
//     解析出來的 key/value 是「當場切出來的臨時字串」，正好是右值；
//     用 emplace_back 可以讓它們被移動進容器，而不是複製一次再丟掉。
// -----------------------------------------------------------------------------
std::vector<Entry> parseConfigLines(const std::vector<std::string>& lines) {
    std::vector<Entry> cfg;
    cfg.reserve(lines.size());
    for (const auto& line : lines) {
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;          // 略過格式不符的行
        // substr 產生的是臨時字串（右值）→ 轉發後選到 Entry(string&&, int)
        cfg.emplace_back(line.substr(0, eq), std::stoi(line.substr(eq + 1)));
    }
    return cfg;
}

int main() {
    std::vector<Entry> vec;
    vec.reserve(4);

    std::string key = "alpha";

    std::cout << "--- push_back（必須先建構，再複製或移動進容器）---\n";
    vec.push_back(Entry(key, 1));           // 先建構臨時 Entry，再移動
    vec.push_back(Entry("beta", 2));        // 同上

    std::cout << "\n--- emplace_back（直接在容器內建構）---\n";
    vec.emplace_back(key, 3);               // 完美轉發 → Entry(const string&, int)
    vec.emplace_back(std::string("delta"), 4); // 完美轉發 → Entry(string&&, int)

    std::cout << "\n=== LeetCode 118. Pascal's Triangle ===\n";
    for (const auto& row : generatePascalTriangle(5)) {
        std::cout << "  [";
        for (std::size_t i = 0; i < row.size(); ++i) {
            std::cout << row[i] << (i + 1 < row.size() ? "," : "");
        }
        std::cout << "]\n";
    }

    std::cout << "\n=== 日常實務：解析設定檔 ===\n";
    auto cfg = parseConfigLines({"max_conn=128", "timeout_ms=3000",
                                 "# 這行沒有等號會被略過", "retry=3"});
    std::cout << "  解析出 " << cfg.size() << " 筆設定\n";
    for (const auto& e : cfg) e.print();

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 2.6 章：stdforward — 完美轉發的核心5.cpp" -o emplace_demo

// 註：Entry 的隱式移動建構子不印訊息，因此 push_back 多出來的那一次移動與解構
//     在下方輸出中「看不見」。效能差異要靠計數器或計時才觀察得到（見第 2.9 章）。

// === 預期輸出 ===
// --- push_back（必須先建構，再複製或移動進容器）---
//   Entry(const string&, int)
//   Entry(string&&, int)
//
// --- emplace_back（直接在容器內建構）---
//   Entry(const string&, int)
//   Entry(string&&, int)
//
// === LeetCode 118. Pascal's Triangle ===
//   [1]
//   [1,1]
//   [1,2,1]
//   [1,3,3,1]
//   [1,4,6,4,1]
//
// === 日常實務：解析設定檔 ===
//   Entry(string&&, int)
//   Entry(string&&, int)
//   Entry(string&&, int)
//   解析出 3 筆設定
//     max_conn = 128
//     timeout_ms = 3000
//     retry = 3
