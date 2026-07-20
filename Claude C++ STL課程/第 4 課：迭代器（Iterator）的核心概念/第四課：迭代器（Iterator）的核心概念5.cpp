// =============================================================================
//  第四課：迭代器（Iterator）的核心概念 5  —  const_iterator 與 const 正確性
// =============================================================================
//
// 【主題資訊 Information】
//   四個取得方式：
//     c.begin()  / c.end()    非 const 容器 → iterator；const 容器 → const_iterator
//     c.cbegin() / c.cend()   永遠回傳 const_iterator（不管容器是不是 const）
//   型別名稱：
//     Container::iterator        可透過它修改元素
//     Container::const_iterator  只能讀，*it 的型別是 const T&
//   標準版本：const_iterator 與 begin() 的 const 重載是 C++98；
//             **cbegin()/cend() 是 C++11 新增**；
//             自由函式 std::cbegin/std::cend 是 C++14。
//   複雜度：全部 O(1)，const_iterator 與 iterator 執行期效能完全相同。
//   標頭檔：容器自身；std::cbegin/std::cend 在 <iterator>。
//
// 【詳細解釋 Explanation】
//
// 【1. const_iterator 不是「const 的 iterator」——兩者差很多】
//   這是最容易搞混的一組概念，關鍵在於 const 修飾的是誰：
//       std::vector<int>::const_iterator it;   // 指向的元素是 const，it 自己可以動
//       const std::vector<int>::iterator it;   // it 自己是 const，指向的元素可以改
//   對應到指標就一清二楚：
//       const int* p;   ←→  const_iterator    （*p 不能改，p++ 可以）
//       int* const p;   ←→  const iterator    （*p 可以改，p++ 不行）
//   所以 `for (const auto it = v.begin(); ...; ++it)` 會編譯失敗——
//   ++it 動到的是 it 自己，而 it 被宣告成 const。
//   要唯讀走訪，正解是 `for (auto it = v.cbegin(); it != v.cend(); ++it)`。
//
// 【2. 為什麼 C++11 要補上 cbegin()/cend()】
//   C++98 就有 const_iterator，但取得方式只有一種：從 const 容器呼叫 begin()。
//   問題是在「容器本身不是 const」的情境下，要寫出唯讀迭代器非常笨拙：
//       std::vector<int> v;
//       std::vector<int>::const_iterator it = v.begin();  // OK，隱式轉換
//       auto it2 = v.begin();                              // C++11 之後推導成 iterator！
//   有了 auto 之後這個問題就變嚴重了：`auto it = v.begin();` 一定推導成
//   可寫的 iterator，你**沒辦法用 auto 表達「我只想讀」**。
//   cbegin() 就是為了補這個洞：`auto it = v.cbegin();` 明確推導成 const_iterator。
//   換句話說，cbegin() 是 auto 的配套設施，兩者都是 C++11 一起進來的。
//
// 【3. 為什麼要在乎？——const 正確性的三個實際好處】
//   (a) 編譯期抓錯。把「我不打算改」寫進型別，手滑寫成 `*it = 0` 會直接編譯失敗，
//       而不是在 code review 或線上才被發現。
//   (b) 介面契約。函式簽名 `void print(const std::vector<int>&)` 對呼叫端是一個
//       承諾：這個函式不會動你的資料。讀者不需要看實作就能相信。
//   (c) 允許最佳化與併發。編譯器知道一段資料在此範圍內不會被改，
//       可以放寬別名分析（aliasing）假設；多執行緒下，「只讀」的共享資料
//       不需要同步——這是 const 正確性在實務上最有價值的地方。
//
// 【4. iterator 可以隱式轉成 const_iterator，反之不行】
//   標準要求 `Container::iterator` 必須可以隱式轉換成 `Container::const_iterator`
//   （單向）。這讓下面這行合法：
//       std::vector<int>::const_iterator cit = v.begin();   // OK
//   反向則不行，要拿掉 const 必須用 const_cast（幾乎總是設計有問題的訊號）。
//   C++11 起還多了一個實用結果：容器的 insert/erase 都改成接受 const_iterator，
//   所以你可以先用 cbegin() 找位置，再拿去 erase——C++98 時代這是不行的。
//
// 【概念補充 Concept Deep Dive】
//   const_iterator 在多數實作裡不是獨立寫一份，而是同一個 class template
//   餵不同型別參數的產物。本機 libstdc++：
//       vector<int>::iterator       = __normal_iterator<int*,       vector<int>>
//       vector<int>::const_iterator = __normal_iterator<const int*, vector<int>>
//   兩者的差別**只在包起來的指標型別**（int* vs const int*）。
//   這也解釋了為什麼 const_iterator 執行期完全沒有額外成本：
//   const 是純編譯期的型別資訊，機器碼一模一樣。
//   同時也解釋了為什麼「iterator → const_iterator」能隱式轉換：
//   libstdc++ 在 __normal_iterator 裡寫了一個 template 建構子，
//   只在 int* 能隱式轉成 const int* 時才啟用（SFINAE），反向自然不成立。
//
// 【注意事項 Pay Attention】
//   1. `const auto it = v.begin();` 不是唯讀走訪，而是「it 自己不能動」，
//      ++it 會編譯失敗。要唯讀請用 cbegin()。
//   2. const 只是「編譯期承諾」，不是記憶體保護。用 const_cast 拿掉 const
//      再寫入原本就宣告為 const 的物件是 UB。
//   3. const 是**淺層的**：`const std::vector<int*> v;` 不能改 v 的元素
//      （那些指標本身），但可以透過那些指標改到它們指向的物件。
//   4. 對 std::map，`iterator` 解參考得到 `pair<const Key, T>&`——
//      key 永遠是 const（改 key 會破壞紅黑樹排序），即使是非 const 迭代器。
//   5. 混用 `v.begin()` 與 `v.cend()` 比較，C++11 起可以（因為 begin() 的
//      結果會隱式轉成 const_iterator），但可讀性差，建議兩端一致。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】const_iterator
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. `const_iterator` 和 `const iterator` 差在哪裡？
//     答：const_iterator 是「指向的元素是 const」——不能 *it = x，但可以 ++it。
//         const iterator 是「迭代器本身是 const」——可以 *it = x，但不能 ++it。
//         對應到指標：const int* vs int* const。
//         唯讀走訪要的是前者，寫成後者會在 ++it 那行編譯失敗。
//     追問：那 `const_iterator` 可以拿來當 erase 的參數嗎？
//         → C++11 起可以。C++98 的 erase/insert 只吃 iterator，C++11 全部
//           改成 const_iterator，因為「指定位置」本來就不需要寫入權限。
//
// 🔥 Q2. C++98 就有 const_iterator 了，為什麼 C++11 還要加 cbegin()？
//     答：因為 auto 的出現。C++98 靠隱式轉換（`const_iterator it = v.begin();`）
//         就能取得唯讀迭代器，但 `auto it = v.begin();` 一定推導成可寫的
//         iterator——auto 沒辦法表達「我只想讀」。cbegin() 讓
//         `auto it = v.cbegin();` 明確得到 const_iterator。
//         cbegin/cend 與 auto 都是 C++11，是配套設計。
//     追問：那為什麼還有 std::cbegin(c) 這個自由函式？
//         → C++14 加的，為了對 C 陣列與沒有成員 cbegin() 的型別也能通用，
//           寫泛型程式碼時比成員函式版本更安全。
//
// ⚠️ 陷阱. 有人想「唯讀走訪」而寫成
//         for (const auto it = v.begin(); it != v.end(); ++it)
//         為什麼編不過？
//     答：const 修飾到的是 it 這個迭代器物件本身，於是 `++it` 想修改一個
//         const 物件 → 編譯錯誤（no match for operator++）。
//         而且就算能編過，它也保護不到元素——`*it = 5` 反而是合法的。
//     為什麼會錯：把 const 的位置當成「這整件事都唯讀」的形容詞，
//         沒有意識到 const 修飾的是它左邊（或緊接右邊）的那個東西。
//         唯讀走訪的正解是換迭代器**型別**，不是給迭代器變數加 const：
//             for (auto it = v.cbegin(); it != v.cend(); ++it)
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <type_traits>   // std::is_same

void print_vector(const std::vector<int>& vec) {
    // const 容器參數只能用 const_iterator
    // 使用 cbegin() 和 cend() 明確取得 const_iterator
    for (auto it = vec.cbegin(); it != vec.cend(); ++it) {
        std::cout << *it << " ";
        // *it = 100;  // 編譯錯誤！不能透過 const_iterator 修改
    }
    std::cout << std::endl;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】監控系統：計算一批取樣值的統計摘要
//   情境：伺服器每分鐘回報 CPU 使用率，監控模組要算出平均值與尖峰值，
//         但**絕對不可以動到原始取樣資料**（其他模組還要用同一份做趨勢圖）。
//   為什麼用 const_iterator：把「唯讀」寫進函式簽名與迭代器型別，
//         等於用編譯器強制執行這個契約。若有人日後在函式裡手滑寫了
//         `*it = 0;`，會在編譯期就被擋下，而不是在正式環境才發現
//         監控資料被汙染。
// -----------------------------------------------------------------------------
struct SampleStats {
    double average;
    int peak;
};

SampleStats summarize(const std::vector<int>& samples) {
    if (samples.empty()) return {0.0, 0};

    long long sum = 0;
    int peak = *samples.cbegin();
    // 全程使用 const_iterator：編譯器保證這個函式不會改到 samples
    for (auto it = samples.cbegin(); it != samples.cend(); ++it) {
        sum += *it;
        if (*it > peak) peak = *it;
    }
    return {static_cast<double>(sum) / static_cast<double>(samples.size()), peak};
}

int main() {
    std::vector<int> vec = {10, 20, 30, 40, 50};

    // 一般迭代器：可讀可寫
    std::cout << "=== 一般迭代器（可寫）===" << std::endl;
    for (auto it = vec.begin(); it != vec.end(); ++it) {
        *it *= 2;  // 可以修改
    }
    print_vector(vec);

    // const_iterator：只能讀
    std::cout << "\n=== const_iterator（唯讀）===" << std::endl;

    // 方法一：從 const 容器取得（begin() 的 const 重載回傳 const_iterator）
    std::cout << "方法一 從 const 參考取得: ";
    const std::vector<int>& const_ref = vec;
    for (auto it = const_ref.begin(); it != const_ref.end(); ++it) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;

    // 方法二：使用 cbegin() / cend()（C++11，容器非 const 也能取得唯讀迭代器）
    std::cout << "方法二 用 cbegin/cend  : ";
    for (auto it = vec.cbegin(); it != vec.cend(); ++it) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;

    // 兩種方法拿到的是同一個型別
    std::cout << "\n=== 兩種方法型別相同 ===" << std::endl;
    std::cout << "const_ref.begin() 與 vec.cbegin() 同型別嗎? "
              << (std::is_same<decltype(const_ref.begin()),
                               decltype(vec.cbegin())>::value ? "是" : "否")
              << std::endl;
    std::cout << "vec.begin() 與 vec.cbegin() 同型別嗎?      "
              << (std::is_same<decltype(vec.begin()),
                               decltype(vec.cbegin())>::value ? "是" : "否")
              << "（前者可寫、後者唯讀）" << std::endl;

    // iterator 可以隱式轉成 const_iterator（單向）
    std::cout << "\n=== iterator → const_iterator 隱式轉換 ===" << std::endl;
    std::vector<int>::const_iterator cit = vec.begin();  // OK：單向隱式轉換
    std::cout << "const_iterator cit = vec.begin(); 合法，*cit = " << *cit << std::endl;
    std::cout << "反過來（const_iterator → iterator）不合法，需要 const_cast" << std::endl;

    // const_iterator 沒有執行期成本：sizeof 相同
    std::cout << "\n=== const 是純編譯期資訊，零執行期成本 ===" << std::endl;
    std::cout << "sizeof(iterator)       = " << sizeof(decltype(vec.begin())) << " bytes" << std::endl;
    std::cout << "sizeof(const_iterator) = " << sizeof(decltype(vec.cbegin())) << " bytes" << std::endl;

    // C++11 起 erase 接受 const_iterator
    std::cout << "\n=== C++11：erase 接受 const_iterator ===" << std::endl;
    std::vector<int> v2 = {1, 2, 3, 4, 5};
    std::vector<int>::const_iterator pos = v2.cbegin() + 2;  // 指向 3
    v2.erase(pos);   // C++98 不允許，C++11 起合法
    std::cout << "erase(cbegin()+2) 後: ";
    for (int n : v2) std::cout << n << " ";
    std::cout << std::endl;

    std::cout << "\n=== 日常實務：監控取樣統計（全程唯讀）===" << std::endl;
    std::vector<int> cpu_samples = {31, 47, 52, 96, 44, 38, 71};
    SampleStats st = summarize(cpu_samples);
    std::cout << "取樣筆數 = " << cpu_samples.size() << std::endl;
    std::cout << "平均使用率 = " << st.average << " %" << std::endl;
    std::cout << "尖峰使用率 = " << st.peak << " %" << std::endl;
    std::cout << "原始資料未被更動: ";
    for (int n : cpu_samples) std::cout << n << " ";
    std::cout << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第四課：迭代器（Iterator）的核心概念5.cpp" -o iter5

// === 預期輸出 ===
// === 一般迭代器（可寫）===
// 20 40 60 80 100
//
// === const_iterator（唯讀）===
// 方法一 從 const 參考取得: 20 40 60 80 100
// 方法二 用 cbegin/cend  : 20 40 60 80 100
//
// === 兩種方法型別相同 ===
// const_ref.begin() 與 vec.cbegin() 同型別嗎? 是
// vec.begin() 與 vec.cbegin() 同型別嗎?      否（前者可寫、後者唯讀）
//
// === iterator → const_iterator 隱式轉換 ===
// const_iterator cit = vec.begin(); 合法，*cit = 20
// 反過來（const_iterator → iterator）不合法，需要 const_cast
//
// === const 是純編譯期資訊，零執行期成本 ===
// sizeof(iterator)       = 8 bytes
// sizeof(const_iterator) = 8 bytes
//
// === C++11：erase 接受 const_iterator ===
// erase(cbegin()+2) 後: 1 2 4 5
//
// === 日常實務：監控取樣統計（全程唯讀）===
// 取樣筆數 = 7
// 平均使用率 = 54.1429 %
// 尖峰使用率 = 96 %
// 原始資料未被更動: 31 47 52 96 44 38 71
