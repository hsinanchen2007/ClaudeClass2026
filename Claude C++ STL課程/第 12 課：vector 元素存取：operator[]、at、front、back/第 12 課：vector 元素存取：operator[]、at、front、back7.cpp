// =============================================================================
//  第 12 課 (7)  —  const 正確性：同一個名字，兩個不同的函式
// =============================================================================
//
// 【主題資訊 Information】
//   vector 的每個存取函式都提供「一對」重載（overload）：
//
//     reference       operator[](size_type);        const_reference operator[](size_type) const;
//     reference       at(size_type);                const_reference at(size_type) const;
//     reference       front();                      const_reference front() const;
//     reference       back();                       const_reference back()  const;
//     T*              data() noexcept;              const T*        data() const noexcept;
//
//   其中 reference = T&、const_reference = const T&
//   標頭檔：<vector>
//   決定呼叫哪一個：由**物件本身是不是 const** 決定（不是由你想不想改決定）
//
// 【詳細解釋 Explanation】
//
// 【1. 這是「重載」，不是「同一個函式」】
// 很多人以為 const vector 只是「不准你改」，其實編譯器做的事更根本：
// 它挑了**另一個完全不同的函式**來呼叫。
//       vector<int>       v;    v[0]   → 呼叫非 const 版本，回傳 int&
//       const vector<int> cv;   cv[0]  → 呼叫 const     版本，回傳 const int&
// 函式簽名尾端的 const 是重載解析的一部分，就像參數型別不同一樣。
// 這稱為「const 重載」，是 C++ 讓同一個介面同時服務可變與不可變物件的機制。
//
// 【2. 為什麼一定要成對提供？】
// 若只有非 const 版本，那 const vector 就完全**無法讀取**了 ——
// 因為在 const 物件上不能呼叫非 const 成員函式。
// 若只有 const 版本，那就沒有人能修改元素。
// 兩個版本並存，才能做到：
//       const 物件 → 只能讀（編譯期強制）
//       非 const 物件 → 可讀可寫
// 而且這個限制是**編譯期**的，零執行期成本。
//
// 【3. const 正確性真正的價值：把契約寫進型別】
// 函式簽名 void print(const vector<int>& v) 傳達了一個承諾：
//       「我只會讀，不會改你的資料」
// 這個承諾由編譯器強制執行，不是靠註解或人的自律。好處是：
//   (a) 呼叫端不必擔心自己的資料被偷改，能安心傳參考（避免複製）
//   (b) 維護者改壞了會編譯失敗，而不是上線後才發現資料被污染
//   (c) const 參考可以綁定暫時物件，介面更泛用
//   (d) 編譯器有更多最佳化空間（知道資料不會從這裡被改動）
//
// 【4. 傳參考而不傳值 —— 但要傳 const 參考】
// void f(vector<int> v)         // 複製整個容器，可能很貴
// void f(vector<int>& v)        // 不複製，但宣告了「我可能會改」
// void f(const vector<int>& v)  // 不複製，且承諾不改 ← 唯讀時的正確選擇
// 對於「只讀不改」的函式，const 參考是唯一正確的選擇：既省了複製成本，
// 又把「不會改」這件事變成編譯器保證。
//
// 【概念補充 Concept Deep Dive】
// const 是「淺層」的（shallow constness）：
// const vector<int*> 的意思是「不能換掉 vector 裡的指標」，
// 但**可以透過那些指標修改它們指向的東西**：
//       const std::vector<int*> cv = {new int(1)};
//       // cv[0] = nullptr;   // ✗ 不行：不能改 vector 裡的指標值
//       *cv[0] = 99;          // ✓ 可以！const 管不到指標指向的目標
// 這對「持有指標的容器」是重要的認知落差 ——
// 容器的 const 只保護容器本身的元素，保護不到元素間接指向的資料。
//
// 另一個常見誤解是 const_iterator 與 const iterator 的差別：
//       vector<int>::const_iterator it;   // 指向的內容不可改，it 本身可移動
//       const vector<int>::iterator it;   // it 本身不可移動，內容可改
// 兩者完全不同。C++11 起建議用 cbegin()/cend() 明確取得 const_iterator。
//
// 【注意事項 Pay Attention】
// 1. const vector 一樣可能發生越界 UB —— const 保護的是「不被修改」，
//    不是「不會越界」。cv[100] 照樣是未定義行為。
// 2. const vector 上呼叫 at() 越界一樣擲出 std::out_of_range（行為不變）。
// 3. const 不是「執行期唯讀記憶體」，它是編譯期的型別檢查。
//    用 const_cast 去掉 const 再修改「原本就宣告為 const 的物件」是 UB。
// 4. 成員函式若不修改狀態就應標成 const —— 否則 const 物件無法呼叫它。
//    這是自訂類別最常見的 const 正確性疏漏。
// 5. const 是淺層的：容器內若是指標，const 保護不到指標指向的資料。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】const 正確性與 const 重載
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. const vector<int>& v 上呼叫 v[0]，回傳的型別是什麼？為什麼？
//     答：回傳 const_reference，也就是 const int&。
//         因為 operator[] 有一對 const 重載，編譯器依「物件是否為 const」
//         做重載解析 —— const 物件會選到尾端帶 const 的那個版本。
//         因此 v[0] = 5 會在編譯期失敗，不是執行期錯誤。
//     追問：那 v.data() 回傳什麼？→ const int*，
//         同樣是 const 重載的結果，所以無法透過它修改元素。
//
// 🔥 Q2. 為什麼「只讀」的函式參數要寫 const vector<int>& 而不是 vector<int>？
//     答：傳值會複製整個容器（heap 配置 + 逐元素複製），對大容器成本很高；
//         傳 const 參考不複製，同時用型別系統承諾「我不會改你的資料」，
//         由編譯器強制執行。另外 const 參考還能綁定暫時物件，介面更泛用。
//     追問：什麼時候反而該傳值？→ 當函式本來就需要一份可修改的副本時
//         （例如要排序後回傳），傳值再搭配移動語意反而更直接。
//
// ⚠️ 陷阱. const std::vector<int*> cv; 之下，*cv[0] = 99; 能編譯嗎？
//     答：**能**。const 是淺層的 —— 它保護的是「vector 裡存的那些指標值」，
//         cv[0] = nullptr 會失敗，但 *cv[0] = 99 完全合法，
//         因為 const 管不到指標所指向的目標物件。
//     為什麼會錯：把 const 想成「這底下的一切都不能動」。
//         實際上 const vector<int*> 對應的元素型別是 int* const（指標本身唯讀），
//         而不是 const int*（指向唯讀資料）。要兩者都唯讀需寫成
//         const std::vector<const int*>。
// ═══════════════════════════════════════════════════════════════════════════

#include <vector>
#include <iostream>
#include <string>
#include <type_traits>

// -----------------------------------------------------------------------------
// 唯讀函式：參數宣告為 const 參考 → 編譯器保證這裡不會改到呼叫端的資料
// -----------------------------------------------------------------------------
void print_first(const std::vector<int>& v) {
    if (!v.empty()) {
        std::cout << "  v[0]      = " << v[0]       << "\n";   // OK：讀取
        std::cout << "  v.at(0)   = " << v.at(0)    << "\n";   // OK：讀取
        std::cout << "  v.front() = " << v.front()  << "\n";   // OK：讀取
        std::cout << "  v.back()  = " << v.back()   << "\n";   // OK：讀取

        // 以下每一行都會編譯失敗，因為 v 是 const，選到的是 const 重載：
        // v[0]       = 100;   // error: assignment of read-only location
        // v.at(0)    = 100;   // error: assignment of read-only location
        // v.front()  = 100;   // error: assignment of read-only location
    }

    // data() 的 const 重載回傳 const int*，只能讀不能寫
    const int* ptr = v.data();
    if (ptr != nullptr) {
        std::cout << "  ptr[0]    = " << ptr[0] << "（透過 const int* 讀取）\n";
    }
    // ptr[0] = 100;   // error: assignment of read-only location
}

// -----------------------------------------------------------------------------
// 用 static_assert 在「編譯期」證明 const 重載真的選到了不同的回傳型別。
// 這比口頭解釋有說服力得多 —— 若推論錯誤，這個檔案根本編不過。
// -----------------------------------------------------------------------------
void prove_const_overload_at_compile_time() {
    using V = std::vector<int>;

    // 非 const 物件 → 選到非 const 重載 → 回傳 int&
    static_assert(std::is_same_v<decltype(std::declval<V&>()[0]),        int&>,
                  "非 const vector 的 operator[] 應回傳 int&");
    static_assert(std::is_same_v<decltype(std::declval<V&>().front()),   int&>,
                  "非 const vector 的 front() 應回傳 int&");
    static_assert(std::is_same_v<decltype(std::declval<V&>().data()),    int*>,
                  "非 const vector 的 data() 應回傳 int*");

    // const 物件 → 選到 const 重載 → 回傳 const int&
    static_assert(std::is_same_v<decltype(std::declval<const V&>()[0]),      const int&>,
                  "const vector 的 operator[] 應回傳 const int&");
    static_assert(std::is_same_v<decltype(std::declval<const V&>().front()), const int&>,
                  "const vector 的 front() 應回傳 const int&");
    static_assert(std::is_same_v<decltype(std::declval<const V&>().data()),  const int*>,
                  "const vector 的 data() 應回傳 const int*");

    std::cout << "  全部 static_assert 通過（在編譯期就已驗證，執行期零成本）\n";
}

// -----------------------------------------------------------------------------
// 【日常實務範例】設定檔驗證器：唯讀掃描 + 可寫入的修正函式，兩者介面分明
//   情境：服務啟動時要 (1) 檢查設定值是否都在合理範圍（純唯讀），
//         (2) 把超出範圍的值夾回合法區間（需要修改）。
//   為什麼是本主題：這兩件事對資料的權限完全不同，
//         用 const 參考 vs 非 const 參考把「我只看」與「我會改」寫進函式簽名，
//         讀程式碼的人光看宣告就知道哪個函式會動到資料 —— 不必翻實作。
// -----------------------------------------------------------------------------
// 唯讀：只回報問題，保證不動到呼叫端的資料
int countOutOfRange(const std::vector<int>& timeouts, int lo, int hi) {
    int bad = 0;
    for (std::size_t i = 0; i < timeouts.size(); ++i) {
        if (timeouts[i] < lo || timeouts[i] > hi) {   // const 版 operator[]
            std::cout << "    [warn] 第 " << i << " 項 = " << timeouts[i]
                      << " 超出合法範圍 [" << lo << ", " << hi << "]\n";
            ++bad;
        }
    }
    return bad;
}

// 可寫：簽名沒有 const，明確宣告「我會修改你的資料」
void clampToRange(std::vector<int>& timeouts, int lo, int hi) {
    for (int& t : timeouts) {                          // 非 const 版，取得 int&
        if (t < lo) t = lo;
        if (t > hi) t = hi;
    }
}

int main() {
    std::cout << "=== 從非 const vector 呼叫（選到非 const 重載）===\n";
    std::vector<int> v = {1, 2, 3};
    v[0] = 11;              // 可以修改
    v.front() = 111;        // 可以修改
    std::cout << "  修改後: ";
    for (int x : v) std::cout << x << " ";
    std::cout << "\n";

    std::cout << "\n=== 傳入 const 參考的函式（選到 const 重載，只能讀）===\n";
    print_first(v);

    std::cout << "\n=== 真正的 const 物件 ===\n";
    const std::vector<int> cv = {7, 8, 9};
    std::cout << "  cv[1]      = " << cv[1] << "\n";
    std::cout << "  cv.at(1)   = " << cv.at(1) << "\n";
    std::cout << "  cv.front() = " << cv.front() << "\n";
    std::cout << "  cv.back()  = " << cv.back() << "\n";
    // cv[1] = 99;   // 編譯錯誤：assignment of read-only location

    std::cout << "\n=== 編譯期證明：const 重載回傳不同型別 ===\n";
    prove_const_overload_at_compile_time();

    std::cout << "\n=== const 是淺層的：容器存指標時的落差 ===\n";
    int a = 1, b = 2;
    const std::vector<int*> cpv = {&a, &b};
    // cpv[0] = nullptr;   // ✗ 編譯錯誤：不能改 vector 裡存的指標值
    *cpv[0] = 99;          // ✓ 合法！const 管不到指標指向的目標
    std::cout << "  *cpv[0] = 99 之後，a = " << a << "\n";
    std::cout << "  （元素型別是 int* const，不是 const int*）\n";

    std::cout << "\n=== 日常實務：設定檔驗證（唯讀）與修正（可寫）===\n";
    std::vector<int> timeouts = {5, 300, 30, -1, 60};
    std::cout << "  原始值: ";
    for (int t : timeouts) std::cout << t << " ";
    std::cout << "\n";

    std::cout << "  唯讀掃描（const vector<int>&）:\n";
    int bad = countOutOfRange(timeouts, 1, 120);
    std::cout << "    共 " << bad << " 項超出範圍\n";

    std::cout << "  執行修正（vector<int>&）:\n";
    clampToRange(timeouts, 1, 120);
    std::cout << "    修正後: ";
    for (int t : timeouts) std::cout << t << " ";
    std::cout << "\n";

    std::cout << "  再次唯讀掃描:\n";
    std::cout << "    共 " << countOutOfRange(timeouts, 1, 120) << " 項超出範圍\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 12 課：vector 元素存取：operator[]、at、front、back7.cpp" -o access7

// === 預期輸出 ===
// === 從非 const vector 呼叫（選到非 const 重載）===
//   修改後: 111 2 3 
//
// === 傳入 const 參考的函式（選到 const 重載，只能讀）===
//   v[0]      = 111
//   v.at(0)   = 111
//   v.front() = 111
//   v.back()  = 3
//   ptr[0]    = 111（透過 const int* 讀取）
//
// === 真正的 const 物件 ===
//   cv[1]      = 8
//   cv.at(1)   = 8
//   cv.front() = 7
//   cv.back()  = 9
//
// === 編譯期證明：const 重載回傳不同型別 ===
//   全部 static_assert 通過（在編譯期就已驗證，執行期零成本）
//
// === const 是淺層的：容器存指標時的落差 ===
//   *cpv[0] = 99 之後，a = 99
//   （元素型別是 int* const，不是 const int*）
//
// === 日常實務：設定檔驗證（唯讀）與修正（可寫）===
//   原始值: 5 300 30 -1 60 
//   唯讀掃描（const vector<int>&）:
//     [warn] 第 1 項 = 300 超出合法範圍 [1, 120]
//     [warn] 第 3 項 = -1 超出合法範圍 [1, 120]
//     共 2 項超出範圍
//   執行修正（vector<int>&）:
//     修正後: 5 120 30 1 60 
//   再次唯讀掃描:
//     共 0 項超出範圍
