// =============================================================================
//  第 11 課：vector 的容量管理：size、capacity、reserve6.cpp
//  —  shrink_to_fit()：一個「請求」，不是「命令」
// =============================================================================
//
// 【主題資訊 Information】
//   void shrink_to_fit();
//
//   標頭檔：<vector>
//   標準版本：**C++11 起**（C++98 沒有這個函式，當年只能用 swap trick）；
//             C++20 起為 constexpr。
//   複雜度：最壞 O(size())（需要配置新 buffer 並搬移全部元素）。
//   例外：搬移過程可能丟 std::bad_alloc（配置失敗）。若元素的 move
//         constructor 不是 noexcept，會改用 copy。
//
//   標準的用字（[vector.capacity]）：
//     "Preconditions: T is MoveInsertable into vector.
//      Effects: shrink_to_fit is a **non-binding request** to reduce
//      capacity() to size()."
//   關鍵在 **non-binding request** —— 標準明文允許實作**完全忽略它**。
//   所以絕不能寫成「shrink_to_fit 一定會把 capacity 降到 size」。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼是「請求」而不是保證】
// 標準把它定成 non-binding 有幾個實際理由：
//   * **可能失敗**：縮容需要配置一塊新的小 buffer 再搬過去。如果此時記憶體
//     不足，硬性要求就得丟 bad_alloc —— 但「我想省記憶體」的操作反而因為
//     記憶體不足而拋例外，非常荒謬。定成請求就可以在失敗時安靜地什麼都不做。
//   * **配置器可能做不到**：某些 allocator（記憶體池、固定區塊配置器）根本
//     沒有「縮小」這個概念，或縮小反而更浪費。
//   * **實作有優化空間**：例如某些實作對小 buffer 直接跳過（縮了也省不到）。
// 因此標準選擇：表達意圖由使用者負責，是否照辦由實作決定。
//
// 【2. 實作實際上怎麼做】
// libstdc++ 的做法是「shrink-to-fit via swap」：建立一個容量剛好 = size 的
// 新 vector、把元素搬過去、再和自己交換。本機實測（GCC 15.2）**確實會縮**，
// capacity 從 1000 降到 10（正好等於 size）。但這是實作行為，不是標準保證 ——
// 換到別的實作、別的 allocator、或記憶體吃緊時，capacity 可能原封不動。
//
// 【3. 什麼時候該用】
//   * 資料載入完成、之後長期只讀：例如把設定檔 / 索引表載入後就固定不動，
//     且當初用了保守的上界 reserve。
//   * 大量資料處理完畢後、程式還要繼續跑很久：把峰值記憶體還給系統。
//   * 篩選結果遠小於輸入：reserve(input.size()) 收集完只剩 1%。
//   反過來，**不該用**的情況：
//   * 容器之後還會再長大 → 縮了又要重新配置，白做兩次工。
//   * 迴圈裡每輪都 clear() 再重填 → 保留 capacity 才是對的（見 8.cpp）。
//   * 小容器 → 省下的記憶體不值得一次配置 + 搬移。
//
// 【4. 縮容的代價：它不是免費的】
// shrink_to_fit 最壞要「配置新 buffer + 搬移全部元素 + 釋放舊 buffer」，
// 是完整的 O(size()) 操作，而且**會使所有 iterator/pointer/reference 失效**。
// 很多人以為它只是「把 capacity 這個數字改小」，其實它比 reserve 還貴 ——
// reserve 至少只在真的要擴大時才搬，shrink_to_fit 幾乎必然要搬一次。
//
// 【概念補充 Concept Deep Dive】
// (A) C++11 之前怎麼縮容 —— swap trick
//   C++98 沒有 shrink_to_fit，經典慣用法是：
//       std::vector<T>(v).swap(v);
//   拆解：① 用 v 複製建構一個暫時 vector（複製建構通常只配置「剛好 size」）
//         ② 和 v 交換內部三根指標（O(1)）
//         ③ 暫時物件在分號處解構，帶走原本那塊過大的 buffer
//   結果 v 得到緊湊的 buffer。C++11 之後仍可用，但 shrink_to_fit 意圖更清楚。
//   注意兩者的差別：swap trick **一定會**重新配置（它不是請求），
//   而 shrink_to_fit 允許實作忽略。想要「保證縮」時 swap trick 反而更可靠。
//   （swap trick 的完整示範見 8.cpp。）
//
// (B) 為什麼「複製一份再 swap」就能得到剛好的容量
//   因為 vector 的複製建構子只需要容納來源的 size() 個元素，實作沒有理由
//   多配 —— libstdc++ 實測複製建構後 capacity == size。這個性質正是
//   swap trick 成立的基礎。但嚴格說這同樣是實作行為，標準只保證複製後
//   內容相同，未規定新容器的 capacity。
//
// (C) 空 vector 上呼叫 shrink_to_fit
//   size() 為 0 時「縮到剛好」意味著釋放整塊 buffer。libstdc++ 實測
//   capacity 會變成 0（本檔第三段驗證）。這正是 clear() + shrink_to_fit()
//   這組慣用法能真正釋放記憶體的原因（見 8.cpp）。
//
// 【注意事項 Pay Attention】
// 1. **shrink_to_fit 是 non-binding request，標準不保證 capacity 會下降。**
//    絕不可寫 assert(v.capacity() == v.size())，那不是可攜的斷言。
// 2. 若真的縮容成功，**所有 iterator/pointer/reference 全部失效**
//    （因為換了一塊新記憶體）。
// 3. 它不是免費的：最壞 O(size()) 的配置 + 搬移。之後還會再長大就別縮。
// 4. 本檔輸出的「1000 → 10」是 libstdc++（GCC 15.2）實測值；
//    其他實作可能維持 1000 不變，那同樣是合規行為。
// 5. C++11 才有此函式。維護 C++98 程式碼時只能用 swap trick。
// 6. 若 T 的 move constructor 不是 noexcept，縮容時會退回 copy —— 對持有
//    堆積資源的型別，成本會比預期高很多。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】shrink_to_fit
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. shrink_to_fit() 保證把 capacity 降到 size 嗎？
//     答：**不保證**。標準明文寫它是 non-binding request，實作可以完全忽略。
//         libstdc++（GCC 15.2）實測會照做（1000 → 10），但那是實作行為，
//         不是可以依賴的保證，更不該寫成單元測試的斷言。
//     追問：為什麼標準要留這個彈性？
//         → 縮容需要配置新 buffer；若此時記憶體不足，硬性要求就得丟 bad_alloc
//           ——「想省記憶體」卻因記憶體不足而失敗太荒謬。定成請求就能安靜放棄。
//           另外某些 allocator（記憶體池）根本沒有縮小的概念。
//
// 🔥 Q2. C++11 之前要怎麼縮小 vector 的容量？
//     答：swap trick：`std::vector<T>(v).swap(v);`
//         先用 v 複製建構一個容量剛好的暫時 vector，交換內部指標，
//         暫時物件解構時帶走原本過大的 buffer。
//     追問：既然有 shrink_to_fit 了，swap trick 還有價值嗎？
//         → 有。swap trick **一定會**重新配置，不是請求；
//           需要「保證縮容」時它反而比 shrink_to_fit 可靠。
//
// ⚠️ 陷阱 1. shrink_to_fit() 之後，先前的 iterator 還能用嗎？
//     答：不能。只要真的縮容成功就換了一塊新記憶體，所有
//         iterator/pointer/reference 全部失效 —— 和擴容一樣危險。
//     為什麼會錯：以為「變小」比較無害，或以為它只是改個數字。
//         實際上它是完整的重新配置 + 全量搬移。
//
// ⚠️ 陷阱 2. 「每次處理完就 shrink_to_fit()，可以省記憶體」——對嗎？
//     答：若容器接下來還會再長回去，這是反效果：縮容付一次 O(n) 搬移，
//         再長大又付一次。典型如「每輪 clear() 後重填」的緩衝區，
//         正確做法是**保留 capacity**，讓下一輪免配置。
//     為什麼會錯：把「佔用記憶體」一律當成壞事，忽略了 capacity 被保留
//         正是為了讓重複使用免於重新配置。
// ═══════════════════════════════════════════════════════════════════════════

#include <vector>
#include <iostream>

int main() {
    std::vector<int> v;
    v.reserve(1000);

    for (int i = 0; i < 10; ++i) {
        v.push_back(i);
    }

    std::cout << "=== 大量預留、少量使用 ===" << std::endl;
    std::cout << "shrink 前: size=" << v.size()
              << ", capacity=" << v.capacity() << std::endl;
    std::cout << "浪費了 " << (v.capacity() - v.size()) * sizeof(int)
              << " bytes" << std::endl;

    const int* before = v.data();
    v.shrink_to_fit();

    std::cout << "\n=== shrink_to_fit() 之後 ===" << std::endl;
    std::cout << "shrink 後: size=" << v.size()
              << ", capacity=" << v.capacity() << std::endl;
    // 位址每次執行都不同 → 只印「有沒有搬移」這個穩定事實
    std::cout << "資料是否被搬到新記憶體: "
              << (v.data() != before ? "是" : "否")
              << "（故舊 iterator 全部失效）" << std::endl;
    std::cout << "註: capacity 真的下降是 libstdc++ 實測結果；" << std::endl;
    std::cout << "    標準說 shrink_to_fit 是 non-binding request，"
              << "允許實作忽略。" << std::endl;

    std::cout << "\n=== 內容不受影響 ===" << std::endl;
    std::cout << "v 內容: [";
    for (std::size_t i = 0; i < v.size(); ++i) {
        if (i != 0) std::cout << ' ';
        std::cout << v[i];
    }
    std::cout << "]" << std::endl;

    std::cout << "\n=== 空 vector 上縮容：釋放整塊 buffer ===" << std::endl;
    std::vector<int> e;
    e.reserve(1000);
    std::cout << "reserve(1000) 後 capacity=" << e.capacity() << std::endl;
    e.shrink_to_fit();
    std::cout << "shrink_to_fit() 後 capacity=" << e.capacity() << std::endl;

    std::cout << "\n=== 對照：resize 縮小不會降 capacity ===" << std::endl;
    std::vector<int> r(100);
    std::cout << "resize(100) 後: size=" << r.size()
              << ", capacity=" << r.capacity() << std::endl;
    r.resize(5);
    std::cout << "resize(5)   後: size=" << r.size()
              << ", capacity=" << r.capacity()
              << " ← capacity 沒動" << std::endl;
    r.shrink_to_fit();
    std::cout << "再 shrink   後: size=" << r.size()
              << ", capacity=" << r.capacity()
              << " ← 這才真的還記憶體" << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 11 課：vector 的容量管理：size、capacity、reserve6.cpp" -o demo6

// 註：以下 capacity 下降的結果為 libstdc++ / GCC 15.2 實測。
//     標準規定 shrink_to_fit 是 **non-binding request**，允許實作完全忽略；
//     在別的實作或記憶體吃緊時，capacity 可能維持原值，那同樣合規。

// === 預期輸出 ===
// === 大量預留、少量使用 ===
// shrink 前: size=10, capacity=1000
// 浪費了 3960 bytes
//
// === shrink_to_fit() 之後 ===
// shrink 後: size=10, capacity=10
// 資料是否被搬到新記憶體: 是（故舊 iterator 全部失效）
// 註: capacity 真的下降是 libstdc++ 實測結果；
//     標準說 shrink_to_fit 是 non-binding request，允許實作忽略。
//
// === 內容不受影響 ===
// v 內容: [0 1 2 3 4 5 6 7 8 9]
//
// === 空 vector 上縮容：釋放整塊 buffer ===
// reserve(1000) 後 capacity=1000
// shrink_to_fit() 後 capacity=0
//
// === 對照：resize 縮小不會降 capacity ===
// resize(100) 後: size=100, capacity=100
// resize(5)   後: size=5, capacity=100 ← capacity 沒動
// 再 shrink   後: size=5, capacity=5 ← 這才真的還記憶體
