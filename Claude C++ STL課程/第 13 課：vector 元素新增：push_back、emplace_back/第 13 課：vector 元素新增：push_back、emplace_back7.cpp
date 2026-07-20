// =============================================================================
//  第 13 課：vector 元素新增：push_back、emplace_back7.cpp
//    —  陷阱：emplace_back 用「直接初始化」，會挑到你沒預期的建構子
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<vector>
//   template<class... Args>
//   reference emplace_back(Args&&... args);   // 直接初始化 direct-initialization
//   void      push_back(const T& / T&&);      // 複製初始化 copy-initialization
//   複雜度：皆為攤銷 O(1)
//
//   關鍵差異（本檔主題）：
//       emplace_back → ::new (ptr) T(args...)     ← 直接初始化，括號
//       push_back    → 參數型別是 T，需要隱式轉換 ← 複製初始化
//   直接初始化「可以」呼叫 explicit 建構子，複製初始化「不行」。
//
// 【詳細解釋 Explanation】
//
// 【1. vv.emplace_back(5) 到底建立了什麼】
// 對 vector<vector<int>> 呼叫 emplace_back(5)，參數 5 會被完美轉發給
// 內層 vector<int> 的建構子。vector<int> 有這個建構子：
//     explicit vector(size_type count);        // 建立 count 個「值初始化」元素
// 於是 emplace_back(5) 建立的是「**含有 5 個 0 的 vector**」，
// 而不是很多人直覺以為的「含有一個元素 5 的 vector」。
// 本檔實測：vv[0].size() == 5，內容是 0 0 0 0 0。
//
// 【2. 為什麼 push_back(5) 反而編不過】
// 這是本檔最值得記住的一點。試著寫：
//     vv.push_back(5);      // ✗ 編譯失敗
// 本機 g++ 15.2 實測錯誤訊息：
//     error: no matching function for call to
//            ‘std::vector<std::vector<int> >::push_back(int)’
// 原因是 push_back 的參數型別是 vector<int>，編譯器必須把 5 隱式轉換成
// vector<int> 才能傳進去——但 vector(size_type) 被標成 **explicit**，
// 明文禁止隱式轉換，所以轉換失敗、找不到可用的重載。
//
// 於是形成一個乍看矛盾的局面：
//     vv.emplace_back(5);   // ✓ 編過，而且結果可能不是你要的
//     vv.push_back(5);      // ✗ 編不過，編譯器擋住了你
// **編不過的那個才是在保護你。** emplace_back 的直接初始化繞過了
// 型別作者刻意加上的 explicit，把「這個轉換太危險、不該自動發生」
// 的警示直接跳過。
//
// 【3. 想要「一個元素是 5 的 vector」該怎麼寫】
//     vv.push_back({5});          // ✓ 用 initializer_list 建構，size()==1
//     vv.emplace_back(std::initializer_list<int>{5});   // ✓ 明確指定型別
// 大括號會選中 vector(initializer_list<int>)，這個建構子不是 explicit，
// 語意也正好是「用這些值當內容」。
//
// 注意 emplace_back({5}) **不能編譯**：裸的 braced-init-list 沒有型別，
// 無法被 template 參數推導（non-deduced context），必須像上面那樣明確寫出
// std::initializer_list<int>。這也是 emplace 系列常見的絆腳石。
//
// 【4. 同一個陷阱的另一面：不做窄化檢查】
//     std::vector<int> n;
//     n.emplace_back(3.9);    // ✓ 編過，實測存進去的是 3（靜默截斷）
//     n.push_back({3.9});     // ✗ 編譯失敗
// 本機 g++ 15.2 實測後者的錯誤訊息：
//     error: narrowing conversion of ‘3.8999999999999999e+0’ from ‘double’
//            to ‘std::vector<int>::value_type’ {aka ‘int’} [-Wnarrowing]
// list-initialization（大括號）會做窄化檢查，direct-initialization（括號）
// 不會。所以同樣是「型別不精確」，一個被擋下、一個悄悄通過。
//
// 【概念補充 Concept Deep Dive】
// 「直接初始化」與「複製初始化」是 C++ 的兩套初始化規則，差別不只在語法：
//
//     T a(x);      // direct-init：考慮**所有**建構子，含 explicit
//     T a = x;     // copy-init  ：只考慮非 explicit 的轉換路徑
//
// explicit 這個關鍵字的用意，就是讓型別作者宣告
// 「這個轉換語意不夠明顯，請使用者明確寫出來，不要讓它自動發生」。
// vector(size_type) 正是典型案例——`vector<int> v = 5;` 讀起來像「v 等於 5」，
// 實際卻是「v 有 5 個 0」，語意落差極大，所以標準把它標成 explicit。
//
// emplace 系列（emplace_back / emplace / emplace_front）內部一律用
// placement new + 括號，屬於 direct-init，因此 explicit 的保護在這裡完全失效。
// 這不是實作的疏漏，而是 emplace 的設計本意——它就是要能呼叫任意建構子，
// 包括 explicit 的。代價就是你必須自己確認參數會選中哪個建構子。
//
// 實務上的判斷準則：
//   * 元素型別的建構子有多個重載、或含 explicit、或有 initializer_list 版本時，
//     emplace_back 的參數要格外小心。
//   * 拿不準時，寫出完整的臨時物件（push_back(T{...})）讓意圖無從誤解，
//     多一次移動換來正確性通常非常划算。
//
// 【注意事項 Pay Attention】
// 1. vv.emplace_back(5) 建立的是「5 個 0」，不是「一個 5」。
// 2. vv.push_back(5) 編譯失敗是**正確行為**，因為 vector(size_type) 是 explicit。
// 3. emplace_back({5}) 無法編譯：braced-init-list 無法被 template 推導。
//    要用 initializer_list 請寫 push_back({5}) 或明確指定型別。
// 4. emplace_back 不做窄化檢查，push_back({...}) 會做。
// 5. 上述所有錯誤訊息皆為本機 g++ 15.2.0 實測；不同編譯器措辭會不同，
//    但「explicit 禁止隱式轉換」「大括號做窄化檢查」是標準規定的行為。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】emplace_back 的直接初始化陷阱
// ───────────────────────────────────────────────────────────────────────────
// ⚠️ 陷阱 Q1. vector<vector<int>> vv; vv.emplace_back(5); 之後 vv[0] 是什麼？
//     答：是一個**含有 5 個 0** 的 vector<int>，size() == 5。
//         參數 5 被完美轉發給內層 vector 的 explicit vector(size_type)
//         建構子，語意是「建立 5 個值初始化的元素」。
//         想要「一個元素是 5」必須寫 vv.push_back({5})。
//     為什麼會錯：把 emplace_back 的參數想成「要放進去的值」。
//         實際上它是「元素型別的建構子參數」，
//         所以真正的問題永遠是「這組參數會選中哪個建構子」。
//
// 🔥 Q2. 為什麼 vv.emplace_back(5) 能編譯，vv.push_back(5) 卻不行？
//     答：push_back 的參數型別是 vector<int>，需要把 5 **隱式轉換**成
//         vector<int>；但 vector(size_type) 是 explicit，禁止隱式轉換，
//         因此找不到可用的重載（本機實測：no matching function for call to
//         push_back(int)）。emplace_back 走直接初始化，可以呼叫 explicit
//         建構子，所以編得過。
//     追問：那這兩個哪一個的行為比較「安全」？
//         → 編不過的 push_back 比較安全。它讓型別作者加的 explicit
//           發揮了預期作用，把語意不明的轉換擋在編譯期。
//
// ⚠️ 陷阱 Q3. 有人說「emplace_back 功能比較強，所以比較好用」，強在哪？代價是什麼？
//     答：強在它用 direct-init，能呼叫**任何**建構子（含 explicit）、
//         能接受多個參數。代價是同時失去兩層編譯期保護：
//         (a) explicit 的防線被繞過（本檔的 emplace_back(5)）；
//         (b) 不做 narrowing 檢查——emplace_back(3.9) 進 vector<int>
//             會靜默變成 3，而 push_back({3.9}) 會編譯失敗
//             （實測：narrowing conversion ... [-Wnarrowing]）。
//     為什麼會錯：把「能編譯的情況比較多」等同於「比較好」。
//         在型別安全這件事上，「編不過」往往才是你要的結果。
//
// 🔥 Q4. emplace_back({1, 2, 3}) 為什麼無法編譯？
//     答：裸的 braced-init-list 本身沒有型別，屬於 non-deduced context，
//         無法被 template<class... Args> 推導出 Args。
//         要傳 initializer_list 必須明確寫出型別，
//         例如 emplace_back(std::initializer_list<int>{1,2,3})，
//         或直接改用 push_back({1, 2, 3})。
//     追問：那 emplace_back(3, 0) 對 vector<vector<int>> 是什麼意思？
//         → 選中 vector(size_type count, const T& value)，
//           建立「3 個 0」的 vector，是常見的建 grid 手法。
// ═══════════════════════════════════════════════════════════════════════════

#include <vector>
#include <iostream>

// -----------------------------------------------------------------------------
// 【日常實務範例】配置固定大小的資料格（訂票系統座位表）
//   情境：訂票系統要建立「rows 排 × cols 位」的座位表，0 代表空位。
//   為什麼用 emplace_back：這裡刻意利用 vector(size_type, const T&) 這個
//                          「多參數建構子」——emplace_back(cols, 0) 直接就地
//                          建構一整列，不必先造臨時 vector 再移動進去。
//                          注意這正是上面陷阱的「正面用法」：同一個建構子，
//                          理解它就是工具，誤解它就是陷阱。
// -----------------------------------------------------------------------------
std::vector<std::vector<int>> makeSeatMap(int rows, int cols) {
    std::vector<std::vector<int>> seats;
    seats.reserve(static_cast<size_t>(rows));
    for (int r = 0; r < rows; ++r) {
        // 就地建構一列 cols 個 0；等價於 push_back(vector<int>(cols, 0))
        // 但省掉臨時 vector 的建構與移動
        seats.emplace_back(static_cast<size_t>(cols), 0);
    }
    return seats;
}

int main() {
    std::vector<std::vector<int>> vv;

    std::cout << "=== 陷阱：emplace_back(5) 選中了 explicit vector(size_type) ==="
              << std::endl;
    // 直覺以為是「加入一個元素 5」，實際是「加入含 5 個 0 的 vector」
    vv.emplace_back(5);

    std::cout << "  vv[0].size() = " << vv[0].size() << std::endl;
    std::cout << "  vv[0] 內容: ";
    for (int x : vv[0]) std::cout << x << " ";
    std::cout << std::endl;
    std::cout << "  → 得到的是 5 個 0，不是一個 5" << std::endl;

    std::cout << "\n=== 正解：要「一個元素是 5」請用大括號 ===" << std::endl;
    vv.push_back({5});   // 選中 vector(initializer_list<int>)，非 explicit

    std::cout << "  vv[1].size() = " << vv[1].size() << std::endl;
    std::cout << "  vv[1] 內容: ";
    for (int x : vv[1]) std::cout << x << " ";
    std::cout << std::endl;

    std::cout << "\n=== 對照：push_back(5) 編譯失敗，而那是正確的保護 ===" << std::endl;
    // vv.push_back(5);
    //   ✗ 本機 g++ 15.2 實測：
    //     error: no matching function for call to
    //            ‘std::vector<std::vector<int> >::push_back(int)’
    //   因為 vector(size_type) 是 explicit，不允許 int → vector<int> 的隱式轉換。
    std::cout << "  push_back(5) 無法編譯（vector(size_type) 是 explicit）"
              << std::endl;
    std::cout << "  emplace_back(5) 卻編得過（direct-init 可呼叫 explicit）"
              << std::endl;

    std::cout << "\n=== 同一陷阱的另一面：不做窄化檢查 ===" << std::endl;
    std::vector<int> n;
    n.emplace_back(3.9);     // ✓ direct-init 不檢查窄化，靜默截斷
    // n.push_back({3.9});   // ✗ list-init 會檢查：narrowing conversion [-Wnarrowing]
    std::cout << "  emplace_back(3.9) 存入 vector<int> 的結果 = " << n[0]
              << "（靜默截斷，本機實測）" << std::endl;
    std::cout << "  push_back({3.9}) 則會編譯失敗，被 -Wnarrowing 擋下"
              << std::endl;

    std::cout << "\n=== 日常實務：建立 3x4 座位表（正面利用多參數建構子）==="
              << std::endl;
    std::vector<std::vector<int>> seats = makeSeatMap(3, 4);
    std::cout << "  共 " << seats.size() << " 排，每排 " << seats[0].size()
              << " 個座位" << std::endl;
    // 賣掉幾個位子，示範這個結構真的可用
    seats[0][1] = 1;
    seats[2][3] = 1;
    for (size_t r = 0; r < seats.size(); ++r) {
        std::cout << "    第 " << (r + 1) << " 排: ";
        for (int s : seats[r]) std::cout << (s == 0 ? "." : "X") << " ";
        std::cout << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 13 課：vector 元素新增：push_back、emplace_back7.cpp" -o emplace_trap

// === 預期輸出 ===
// === 陷阱：emplace_back(5) 選中了 explicit vector(size_type) ===
//   vv[0].size() = 5
//   vv[0] 內容: 0 0 0 0 0 
//   → 得到的是 5 個 0，不是一個 5
// 
// === 正解：要「一個元素是 5」請用大括號 ===
//   vv[1].size() = 1
//   vv[1] 內容: 5 
// 
// === 對照：push_back(5) 編譯失敗，而那是正確的保護 ===
//   push_back(5) 無法編譯（vector(size_type) 是 explicit）
//   emplace_back(5) 卻編得過（direct-init 可呼叫 explicit）
// 
// === 同一陷阱的另一面：不做窄化檢查 ===
//   emplace_back(3.9) 存入 vector<int> 的結果 = 3（靜默截斷，本機實測）
//   push_back({3.9}) 則會編譯失敗，被 -Wnarrowing 擋下
// 
// === 日常實務：建立 3x4 座位表（正面利用多參數建構子）===
//   共 3 排，每排 4 個座位
//     第 1 排: . X . . 
//     第 2 排: . . . . 
//     第 3 排: . . . X 
