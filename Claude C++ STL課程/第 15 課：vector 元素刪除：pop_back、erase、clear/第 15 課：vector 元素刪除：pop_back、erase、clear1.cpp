// =============================================================================
//  第 15 課：vector 元素刪除 1  —  pop_back：唯一 O(1) 的刪除
// =============================================================================
//
// 【主題資訊 Information】
//   void pop_back();
//   標頭檔：<vector>
//   標準版本：C++98。
//   複雜度：O(1)。呼叫最後一個元素的解構子，size 減一。
//   回傳：void ——【不會回傳被刪除的元素】。理由見【詳細解釋 3.】。
//   前置條件：容器【不得為空】。對空 vector 呼叫是未定義行為。
//   失效範圍：指向最後一個元素的 iterator/reference/pointer，以及 end()。
//             其餘元素完全不受影響。
//   關鍵性質：capacity【不變】。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼只有尾端刪除是 O(1)】
//   vector 保證元素連續存放，這是它快的原因，也是它的限制。
//   刪除中間的元素，後面所有元素都得往前補位（O(n)）；
//   但刪除最後一個不會在中間留洞，只要：
//     ① 呼叫該元素的解構子
//     ② size 減一
//   兩步都與元素個數無關，所以是 O(1)。
//   這也是為什麼 vector 天生適合當堆疊（stack），
//   std::stack 的預設底層容器正是 deque，而 vector 也是常見選擇。
//
// 【2. 為什麼 capacity 不變】
//   pop_back 完全不碰記憶體配置。這是刻意的：
//   刪掉一個元素就把記憶體還回去，接著又 push_back，
//   就會變成「配置→釋放→配置」的抖動（thrashing）。
//   STL 一貫的策略是「capacity 只增不減，除非你明確要求」。
//   要真的縮回去得呼叫 shrink_to_fit()，而它是【非約束性請求】，
//   標準允許實作忽略——不能說「呼叫它一定會縮容」。
//
// 【3. 為什麼 pop_back 不回傳被刪除的元素】
//   這是 STL 一個經過深思的設計決定，理由是【例外安全】：
//   若 pop_back 要回傳 T，就必須複製（或移動）一份出來給呼叫端。
//   萬一那個複製建構子拋出例外，元素已經從容器移除、
//   但呼叫端也沒拿到——資料就永久遺失了，而且無法回復。
//   拆成兩步就沒有這個問題：
//       T value = v.back();   // 複製失敗的話，容器完好無損
//       v.pop_back();         // 這一步不會拋例外
//   代價是要寫兩行。這是 STL 用「介面稍微囉唆」換「絕不遺失資料」的例子。
//   （Java 的 Stack.pop() 就回傳值，因為它有 GC，語意不同。）
//
// 【4. 空容器上呼叫是未定義行為，不是「什麼都不做」】
//   標準明訂前置條件是 `!empty()`。違反的話：
//     * 不保證崩潰
//     * 不保證什麼都不做
//     * libstdc++ 的實作會讓 size 從 0 減成 SIZE_MAX（無號環繞），
//       接著整個 vector 的狀態就毀了，後續任何操作都不可預測
//   所以每次呼叫前都要確認非空。這是本課最重要的一條規則。
//
// 【概念補充 Concept Deep Dive】
//
// (A) pop_back 對 trivially destructible 型別幾乎是零成本
//     若 T 是 int、double、或沒有自訂解構子的 POD，
//     編譯器知道「解構子什麼都不做」，於是 pop_back 只剩 `--size`。
//     對 std::string 這種型別則要真的呼叫解構子（釋放字串緩衝區）。
//     這就是為什麼 vector<int> 的 pop_back 幾乎免費，
//     而 vector<string> 的 pop_back 有實質工作。
//
// (B) pop_back 與 erase(end()-1) 的差別
//     兩者結果相同，但語意與成本不同：
//       pop_back()        —— O(1)，明說「拿掉最後一個」
//       erase(end() - 1)  —— 同樣是 O(1)（沒有元素需要搬），
//                            但要多算一次迭代器，語意也不如前者直接
//     另外 erase 對空容器同樣是 UB（end()-1 本身就已經越界）。
//     結論：要刪最後一個就用 pop_back。
//
// (C) 為什麼標準沒有提供 pop_front
//     vector 沒有 pop_front，因為它會是 O(n)——所有元素都要往前搬。
//     STL 的原則是「容器只提供它能高效做到的操作」，
//     不提供 pop_front 就是誠實地告訴你「這件事 vector 做不好」。
//     需要頭尾都 O(1) 的，該用 std::deque（它有 pop_front）。
//     這也解釋了為什麼 list 有 pop_front 而 vector 沒有。
//
// 【注意事項 Pay Attention】
//   1. 對空 vector 呼叫 pop_back 是未定義行為。呼叫前務必 !empty()。
//   2. pop_back 不回傳值。要取值請先 back() 再 pop_back()。
//   3. capacity 不變。要釋放記憶體得另外呼叫 shrink_to_fit()，
//      而它是非約束性請求，不保證真的縮容。
//   4. 只有「指向最後一個元素」的 iterator/reference 與 end() 失效，
//      其餘元素的參考完全不受影響。
//   5. vector 沒有 pop_front（那會是 O(n)）。需要的話用 deque。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】vector::pop_back
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 pop_back 的回傳型別是 void，而不是回傳被刪除的元素？
//     答：為了例外安全。若要回傳 T，就得複製或移動一份給呼叫端；
//         萬一那次複製拋出例外，元素已經從容器移除、呼叫端又沒拿到，
//         資料就永久遺失且無法回復。
//         拆成 `T x = v.back(); v.pop_back();` 兩步後，
//         第一步失敗時容器完好無損，第二步保證不拋例外。
//     追問：那為什麼 Java 的 Stack.pop() 就可以回傳值？
//         → 它回傳的是參考、且有 GC，物件不會因為「沒接住」而消失，
//           語意前提完全不同。
//
// 🔥 Q2. pop_back 之後 capacity 會變嗎？為什麼這樣設計？
//     答：不會變，只有 size 減一。因為刪一個就還記憶體、
//         下次 push_back 又要重新配置，會造成配置/釋放的抖動。
//         STL 的一貫策略是 capacity 只增不減，除非明確要求。
//     追問：那要怎麼真的把記憶體釋放掉？
//         → 呼叫 shrink_to_fit()。但要注意它是【非約束性請求】，
//           標準允許實作直接忽略。想要保證的話用 swap 慣用法：
//           std::vector<T>(v).swap(v);
//
// ⚠️ 陷阱. 「對空 vector 呼叫 pop_back()，頂多就是什麼都不做，
//         或者當場崩潰讓我知道，反正很容易發現」——這個想法錯在哪？
//     答：兩種猜測都不對，因為那是未定義行為。
//         libstdc++ 的實作是 `--this->_M_impl._M_finish`，
//         也就是把「結尾指標」往前挪一格。對空容器來說，
//         這個指標會退到配置區塊之外，於是 size() 變成一個天文數字
//         （無號環繞），vector 的內部狀態徹底毀壞。
//         接下來任何操作——甚至只是印出 size()——都不可預測，
//         而且【不會當場崩潰】，錯誤會在很久之後才浮現。
//     為什麼會錯：把 UB 當成「兩種可預期結果之一」。
//         UB 最危險的形態恰恰是「看起來沒事」——
//         它讓錯誤在遠離現場的地方爆炸，除錯成本極高。
//         本檔【不會】示範這個結果，因為它沒有正確答案。
// ═══════════════════════════════════════════════════════════════════════════

#include <vector>
#include <iostream>
#include <string>

// -----------------------------------------------------------------------------
// 【日常實務範例】編輯器的復原（undo）堆疊
//   情境：文字編輯器每次操作都把「反向操作」推進 undo 堆疊；
//         使用者按 Ctrl+Z 就取出最後一筆執行。
//   為什麼用到本主題：這是 pop_back 最典型的用途——vector 當堆疊用。
//     而且它示範了正確的取值姿勢：
//       ① 先 empty() 檢查（pop_back 不會幫你檢查）
//       ② 先 back() 取值、再 pop_back() 刪除（分兩步，例外安全）
//     同時也用到「capacity 不變」這個性質：反覆 undo/redo 時，
//     堆疊的記憶體被重複利用，不會反覆配置釋放。
// -----------------------------------------------------------------------------
class UndoStack {
public:
    void record(const std::string& action) {
        history_.push_back(action);
    }

    // 回傳 true 表示有東西可以復原；復原的內容寫進 out
    bool undo(std::string& out) {
        if (history_.empty()) return false;   // pop_back 不檢查，我們自己檢查
        out = history_.back();                // 先取值（複製失敗的話容器完好）
        history_.pop_back();                  // 再刪除（這一步不拋例外）
        return true;
    }

    size_t size()     const { return history_.size(); }
    size_t capacity() const { return history_.capacity(); }

private:
    std::vector<std::string> history_;
};

int main() {
    std::vector<int> v = {1, 2, 3, 4, 5};

    std::cout << "=== 原始示範：pop_back ===\n";
    std::cout << "原始: ";
    for (int x : v) std::cout << x << " ";
    std::cout << std::endl;
    std::cout << "初始 size: " << v.size()
              << ", capacity: " << v.capacity() << std::endl;

    v.pop_back();  // 刪除 5
    v.pop_back();  // 刪除 4

    std::cout << "pop_back 兩次後: ";
    for (int x : v) std::cout << x << " ";  // 1 2 3
    std::cout << std::endl;

    std::cout << "size: " << v.size() << std::endl;        // 3
    std::cout << "capacity: " << v.capacity() << std::endl; // 不變
    std::cout << "→ size 減少了 2，capacity 完全沒動。\n";

    std::cout << "\n=== pop_back 不回傳值：正確的取值姿勢 ===\n";
    {
        std::vector<std::string> names = {"alice", "bob", "carol"};
        // 錯誤想像：std::string last = names.pop_back();  // 編譯失敗，回傳 void
        std::string last = names.back();   // 第一步：取值
        names.pop_back();                  // 第二步：刪除
        std::cout << "取出的最後一個: " << last << "\n";
        std::cout << "剩下: ";
        for (const auto& s : names) std::cout << s << " ";
        std::cout << "\n";
        std::cout << "（分兩步是為了例外安全：第一步失敗時容器完好無損）\n";
    }

    std::cout << "\n=== 空容器：一定要先檢查 ===\n";
    {
        std::vector<int> empty;
        std::cout << "empty.size() = " << empty.size() << "\n";

        // 安全做法
        if (!empty.empty()) {
            empty.pop_back();
            std::cout << "刪除了一個元素\n";
        } else {
            std::cout << "容器是空的 → 不呼叫 pop_back（正確做法）\n";
        }

        std::cout << "若直接呼叫 empty.pop_back() 會怎樣？\n";
        std::cout << "  → 未定義行為。libstdc++ 會把結尾指標退到配置區塊之外，\n";
        std::cout << "    size() 變成天文數字、vector 狀態毀壞，\n";
        std::cout << "    而且【不會當場崩潰】——錯誤會在很久之後才浮現。\n";
        std::cout << "    本檔不示範它的「結果」，因為那不存在正確答案。\n";
    }

    std::cout << "\n=== 只有最後一個元素的參考會失效 ===\n";
    {
        std::vector<int> w = {10, 20, 30, 40};
        int& first = w[0];      // 指向第一個
        int& last  = w[3];      // 指向最後一個
        std::cout << "pop_back 前: first=" << first << " last=" << last << "\n";

        w.pop_back();           // 刪掉 40

        std::cout << "pop_back 後: first=" << first
                  << "（仍然有效，pop_back 不搬動其他元素）\n";
        std::cout << "  last 現在是懸空參考，不可使用（本檔不讀它）\n";
        std::cout << "  目前內容: ";
        for (int x : w) std::cout << x << " ";
        std::cout << "\n";
    }

    std::cout << "\n=== capacity 只增不減；shrink_to_fit 是非約束性請求 ===\n";
    {
        std::vector<int> s(1000, 7);
        std::cout << "1000 個元素: size=" << s.size()
                  << " capacity=" << s.capacity() << "\n";
        for (int i = 0; i < 990; ++i) s.pop_back();
        std::cout << "pop 掉 990 個: size=" << s.size()
                  << " capacity=" << s.capacity() << "  ← capacity 沒動\n";
        s.shrink_to_fit();
        std::cout << "shrink_to_fit(): size=" << s.size()
                  << " capacity=" << s.capacity() << "\n";
        std::cout << "（本機 libstdc++ 確實縮了，但標準允許實作忽略此請求，\n";
        std::cout << "  不能寫成「呼叫它一定會縮容」）\n";
    }

    std::cout << "\n=== 日常實務：編輯器的 undo 堆疊 ===\n";
    {
        UndoStack undo;
        undo.record("輸入 'Hello'");
        undo.record("刪除第 3 行");
        undo.record("貼上 20 行");
        std::cout << "記錄了 " << undo.size() << " 個操作"
                  << "（capacity=" << undo.capacity() << "）\n";

        std::string what;
        while (undo.undo(what)) {
            std::cout << "  Ctrl+Z 復原: " << what
                      << "（剩 " << undo.size() << " 筆）\n";
        }

        std::cout << "再按一次 Ctrl+Z: "
                  << (undo.undo(what) ? "有東西可復原" : "沒有東西可復原（安全回傳 false）")
                  << "\n";
        std::cout << "堆疊清空後 capacity 仍是 " << undo.capacity()
                  << " → 之後再記錄操作不必重新配置記憶體\n";
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第 15 課：vector 元素刪除1.cpp -o demo1

// === 預期輸出 ===
// === 原始示範：pop_back ===
// 原始: 1 2 3 4 5
// 初始 size: 5, capacity: 5
// pop_back 兩次後: 1 2 3
// size: 3
// capacity: 5
// → size 減少了 2，capacity 完全沒動。
//
// === pop_back 不回傳值：正確的取值姿勢 ===
// 取出的最後一個: carol
// 剩下: alice bob
// （分兩步是為了例外安全：第一步失敗時容器完好無損）
//
// === 空容器：一定要先檢查 ===
// empty.size() = 0
// 容器是空的 → 不呼叫 pop_back（正確做法）
// 若直接呼叫 empty.pop_back() 會怎樣？
//   → 未定義行為。libstdc++ 會把結尾指標退到配置區塊之外，
//     size() 變成天文數字、vector 狀態毀壞，
//     而且【不會當場崩潰】——錯誤會在很久之後才浮現。
//     本檔不示範它的「結果」，因為那不存在正確答案。
//
// === 只有最後一個元素的參考會失效 ===
// pop_back 前: first=10 last=40
// pop_back 後: first=10（仍然有效，pop_back 不搬動其他元素）
//   last 現在是懸空參考，不可使用（本檔不讀它）
//   目前內容: 10 20 30
//
// === capacity 只增不減；shrink_to_fit 是非約束性請求 ===
// 1000 個元素: size=1000 capacity=1000
// pop 掉 990 個: size=10 capacity=1000  ← capacity 沒動
// shrink_to_fit(): size=10 capacity=10
// （本機 libstdc++ 確實縮了，但標準允許實作忽略此請求，
//   不能寫成「呼叫它一定會縮容」）
//
// === 日常實務：編輯器的 undo 堆疊 ===
// 記錄了 3 個操作（capacity=4）
//   Ctrl+Z 復原: 貼上 20 行（剩 2 筆）
//   Ctrl+Z 復原: 刪除第 3 行（剩 1 筆）
//   Ctrl+Z 復原: 輸入 'Hello'（剩 0 筆）
// 再按一次 Ctrl+Z: 沒有東西可復原（安全回傳 false）
// 堆疊清空後 capacity 仍是 4 → 之後再記錄操作不必重新配置記憶體
