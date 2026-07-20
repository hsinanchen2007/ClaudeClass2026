// =============================================================================
//  第 2.5 章 3  —  std::move 什麼時候「沒有」真的移動
// =============================================================================
//
// 【主題資訊 Information】
//   template<class T>
//   constexpr std::remove_reference_t<T>&& move(T&& t) noexcept;   [C++11, <utility>]
//   （constexpr 是 C++14 起）
//   標頭檔  : <utility>
//   複雜度  : O(1) —— 它不做任何事，只是一次編譯期的型別轉換
//
// 【詳細解釋 Explanation】
//
// 【1. std::move 不會移動任何東西】
//   這是整章最重要的一句話。std::move 只是一個 static_cast：
//       static_cast<std::remove_reference_t<T>&&>(t)
//   它把一個左值「標記」成右值（更精確說是 xvalue），如此而已。
//   編譯後它不產生任何指令。真正搬東西的是「被選中的那個建構子」。
//   所以正確的心智模型是：
//       std::move  = 「我不再需要這個物件的內容了，你可以拿走」
//       移動建構子 = 「好，我把資源搬走」
//   如果對方沒有移動建構子、或多載決議選不到它，就什麼也不會被搬走。
//
// 【2. 情境 2：const 物件為什麼移動不了】
//   std::move(c) 的 c 是 const std::string，結果型別是 const std::string&&。
//   多載決議時：
//       string(string&&)        ← 不能綁 const 右值（會丟掉 const）
//       string(const string&)   ← 可以綁（const 左值參考能接受 const 右值）
//   於是選到複製建構子。程式仍然編譯成功、仍然正確，只是「悄悄地變慢了」，
//   沒有任何警告。這是 std::move 最常見的失效情形。
//   推論：想讓移動有效，成員與區域變數就不要無謂地加 const。
//
// 【3. 情境 3：基本型別的移動就是複製】
//   int 沒有任何「外部資源」可以搬，移動與複製都是 4 bytes 的位元組拷貝。
//   對 trivially copyable 的型別，std::move 完全沒有意義，
//   而且來源的值有明確保證：仍然是原值（x 依舊是 42）。
//
// 【4. 移動之後，來源物件到底是什麼狀態】
//   標準的用語是「valid but unspecified」（有效但未指定）：
//     * 可以安全解構 —— 保證。
//     * 可以重新賦值後正常使用 —— 保證。
//     * 內容是什麼 —— 不保證，也不可依賴。
//   libstdc++ 實務上會把 moved-from 的 std::string 留成空字串，
//   但那是實作行為、不是標準保證，換一個標準庫（或短字串走 SSO 時）
//   就可能不同。所以本檔刻意「不印出」moved-from 的內容，
//   改為示範標準真正保證的事：重新賦值之後一切正常。
//
// 【概念補充 Concept Deep Dive】
//   std::move 的回傳型別是 remove_reference_t<T>&& 而不是 T&&，
//   原因是參考摺疊：若 T 被推導成 std::string&（傳左值時），
//   T&& 會摺疊回 std::string&，那就變成回傳左值參考、完全失效。
//   先用 remove_reference_t 剝掉參考，再加 &&，才能保證一定得到右值參考。
//   另外要注意 std::move 與 std::forward 的分工：
//       std::move    ：無條件轉成右值。用於「我確定不再需要它」。
//       std::forward ：有條件轉換，保持原本的值類別。只用於轉發參考 T&&。
//   在轉發參考上誤用 std::move，會把呼叫端的左值也掏空，是常見的嚴重 bug。
//
// 【注意事項 Pay Attention】
// 1. 絕對不要讀取 moved-from 物件的內容並當成固定值——那是未指定的，
//    不是 UB（讀取本身合法），但結果不可依賴、也不可寫進測試預期。
// 2. const 物件上的 std::move 會靜默退化成複製，沒有任何警告。
// 3. 對回傳的區域變數寫 return std::move(local); 是反效果——
//    它會阻止 NRVO（具名回傳值最佳化），反而多一次移動。直接 return local;。
// 4. std::move 本身是 noexcept 且不產生程式碼；效能來自被選中的建構子。
// 5. 對 trivially copyable 型別（int、double、指標）用 std::move 沒有任何效果。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::move 的真實語意
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::move 到底做了什麼？
//     答：什麼也沒做。它是一個 static_cast，把左值轉成右值參考型別，
//         編譯後不產生任何指令。真正搬移資源的是「因此被選中的移動建構子
//         或移動賦值運算子」。所以它更像是一個「授權標記」而非動作。
//     追問：那它為什麼要回傳 remove_reference_t<T>&& 而不是 T&&？
//         → 因為參考摺疊。傳左值時 T 會被推導成 U&，T&& 摺疊後變回 U&，
//         就得不到右值參考了；先剝掉參考才安全。
//
// 🔥 Q2. 對一個 const 物件呼叫 std::move，會發生什麼？
//     答：得到 const T&&。移動建構子 T(T&&) 綁不了它（會丟掉 const），
//         於是多載決議選中 T(const T&)——複製建構子。
//         程式正確但變慢，而且完全沒有警告。
//     追問：這給我們什麼實務啟示？→ 打算被移動的區域變數與成員不要加
//         多餘的 const；「const 正確性」在這裡會與效能直接衝突。
//
// ⚠️ 陷阱. 「移動後原字串一定變成空字串」——為什麼這句話是錯的？
//     答：標準只保證 moved-from 物件處於「有效但未指定」狀態：
//         可以安全解構、可以重新賦值，但內容未指定。
//         libstdc++ 目前確實會留下空字串，但那是實作細節，
//         不是標準保證，不可寫進程式邏輯或測試斷言。
//     為什麼會錯：因為在主流實作上「試出來就是空的」，於是被當成規則記住。
//         這是拿「觀察到的實作行為」冒充「標準保證」的典型錯誤——
//         換標準庫、換最佳化等級、或字串短到走 SSO 時都可能不同。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】從缺。
//   理由：本檔主題是 std::move 的語意邊界（const 阻擋移動、基本型別無效、
//   moved-from 狀態），屬於語言規則層面。LeetCode 判定的是演算法正確性，
//   不會因為少了一次移動而失敗，清單中也沒有任何一題以「值類別轉換」為核心。
//   硬套一題只會模糊焦點，故從缺，改以實務範例呈現 std::move 真正的用途。

#include <iostream>
#include <string>
#include <utility>
#include <vector>

// -----------------------------------------------------------------------------
// 【日常實務範例】把解析好的資料「搬」進容器，而不是複製
//   場景：讀取設定檔／log，每一行解析成一個記錄後放進 vector。
//         每筆記錄都持有字串（堆積資源）。
//   為什麼用 std::move：解析用的區域變數在 push_back 之後就不再使用了，
//     複製它等於白白做一次堆積配置與字元拷貝。用 std::move 明確表示
//     「這個區域變數的內容可以被拿走」，於是只搬指標。
//   對照組刻意寫成 const 區域變數——這正是情境 2 的陷阱在實務上的樣子：
//     加了 const 之後 std::move 靜默失效，效能悄悄掉回複製。
// -----------------------------------------------------------------------------
struct LogRecord {
    std::string level;
    std::string message;

    // 用計數器觀察到底走了複製還是移動
    inline static int copies = 0;
    inline static int moves  = 0;

    LogRecord(std::string lv, std::string msg)
        : level(std::move(lv)), message(std::move(msg)) {}
    LogRecord(const LogRecord& o)
        : level(o.level), message(o.message) { ++copies; }
    LogRecord(LogRecord&& o) noexcept
        : level(std::move(o.level)), message(std::move(o.message)) { ++moves; }
    LogRecord& operator=(const LogRecord&) = default;
    LogRecord& operator=(LogRecord&&) = default;
    ~LogRecord() = default;
};

int main() {
    // ===== 情境 1：移動確實發生 =====
    std::cout << "--- 情境 1：有移動建構子 ---\n";
    std::string a = "Hello";
    std::string b = std::move(a);  // string 有移動建構子 → 移動發生
    std::cout << "b = \"" << b << "\"（資源已搬到 b，這是有保證的）\n";
    // 刻意「不印」a 的內容：moved-from 物件是「有效但未指定」，
    // 印出來的值不可依賴，也不該寫進預期輸出。
    // 標準真正保證的是：可以安全解構、也可以重新賦值後正常使用。
    a = "重新賦值後就完全正常";
    std::cout << "a 重新賦值後 = \"" << a << "\"（這才是標準保證的部分）\n";

    // ===== 情境 2：const 物件，移動不會發生 =====
    std::cout << "\n--- 情境 2：const 物件 ---\n";
    const std::string c = "World";
    std::string d = std::move(c);  // std::move(c) 型別是 const string&&
                                    // 匹配到複製建構子（不是移動建構子）
    std::cout << "c = \"" << c << "\"（仍然是 World：根本沒被移動）\n";
    std::cout << "d = \"" << d << "\"（複製而來，不是移動）\n";

    // ===== 情境 3：基本型別，移動 = 複製 =====
    std::cout << "\n--- 情境 3：基本型別 ---\n";
    int x = 42;
    int y = std::move(x);  // int 沒有「資源」可搬，移動就是複製
    std::cout << "x = " << x << "（仍然是 42，這對 int 是有保證的）\n";
    std::cout << "y = " << y << "\n";

    // ===== 日常實務：搬進容器 vs 複製進容器 =====
    std::cout << "\n--- 日常實務：把記錄放進 vector ---\n";
    std::vector<LogRecord> sink;
    sink.reserve(4);            // 先 reserve，排除擴容造成的搬移干擾計數

    LogRecord::copies = 0;
    LogRecord::moves  = 0;

    // (a) 一般區域變數 + std::move → 走移動
    LogRecord r1("ERROR", "disk full on /dev/nvme0n1p2");
    sink.push_back(std::move(r1));

    // (b) 臨時物件本身就是右值 → 走移動，連 std::move 都不必寫
    sink.push_back(LogRecord("WARN", "memory usage above 85%"));

    // (c) const 區域變數 + std::move → 靜默退化成「複製」
    const LogRecord r3("INFO", "service started");
    sink.push_back(std::move(r3));   // const 擋住了移動！

    std::cout << "放入 " << sink.size() << " 筆記錄\n";
    std::cout << "複製建構次數: " << LogRecord::copies
              << "（來自那個 const 區域變數）\n";
    std::cout << "移動建構次數: " << LogRecord::moves << "\n";
    std::cout << "結論：const 讓 std::move 靜默失效，沒有任何編譯警告。\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第 2.5 章：stdmove — 強制轉換為右值3.cpp -o move_semantics_edge

// 【但書】本檔刻意「不輸出」被移動走的 a 的內容。moved-from 物件是
//   「有效但未指定（valid but unspecified）」——讀它並非 UB，但值不可依賴，
//   因此不適合寫進預期輸出。libstdc++ 目前會留下空字串，那是實作行為、
//   不是標準保證。輸出中呈現的是標準真正保證的部分：重新賦值後一切正常。

// === 預期輸出 ===
// --- 情境 1：有移動建構子 ---
// b = "Hello"（資源已搬到 b，這是有保證的）
// a 重新賦值後 = "重新賦值後就完全正常"（這才是標準保證的部分）
//
// --- 情境 2：const 物件 ---
// c = "World"（仍然是 World：根本沒被移動）
// d = "World"（複製而來，不是移動）
//
// --- 情境 3：基本型別 ---
// x = 42（仍然是 42，這對 int 是有保證的）
// y = 42
//
// --- 日常實務：把記錄放進 vector ---
// 放入 3 筆記錄
// 複製建構次數: 1（來自那個 const 區域變數）
// 移動建構次數: 2
// 結論：const 讓 std::move 靜默失效，沒有任何編譯警告。
