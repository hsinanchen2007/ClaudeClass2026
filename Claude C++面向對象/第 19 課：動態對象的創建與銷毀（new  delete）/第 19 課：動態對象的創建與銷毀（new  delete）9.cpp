// =============================================================================
//  第 19 課 範例 9  —  裸 new/delete vs 智能指標（現代 C++ 的標準答案）
// =============================================================================
//
// 【主題資訊 Information】
//   標準版本：unique_ptr 為 C++11；make_unique 為 C++14；
//             make_unique<T[]>(n) 亦為 C++14
//   標頭檔  ：<memory>
//   複雜度  ：unique_ptr 的解參考與呼叫成本與裸指標相同（零額外開銷）
//   關鍵語法：
//     std::unique_ptr<T>   p = std::make_unique<T>(args...);
//     std::unique_ptr<T[]> a = std::make_unique<T[]>(n);
//
// 【詳細解釋 Explanation】
//
// 【1. unique_ptr 是什麼】
//   它是一個「只擁有一個物件的 RAII 包裝」：
//   建構時接管指標，解構時自動 delete。
//   關鍵在於它把「所有權」寫進了型別 ——
//   unique_ptr 不能複製（複製會直接編譯失敗），只能移動。
//   於是「誰負責釋放」這件事在編譯期就是明確的，
//   不會出現兩個地方都以為自己該 delete 的窘境。
//
// 【2. 為什麼零開銷】
//   unique_ptr 內部就只有一個指標（預設 deleter 是無狀態的空類別，
//   透過 empty base optimization 不佔空間），
//   因此 sizeof(unique_ptr<T>) 通常等於 sizeof(T*)。
//   解參考、成員存取都會被 inline 成與裸指標完全相同的指令。
//   你付出的成本是零，換到的是「不可能忘記 delete」——
//   這是 C++ 少見的「白拿」交易，所以現代 C++ 一律建議優先使用它。
//   （對照組是 shared_ptr：它有引用計數，sizeof 通常是兩個指標，
//    且計數的增減是原子操作，確實有成本。所以預設應該用 unique_ptr，
//    只有真的需要共享所有權時才用 shared_ptr。）
//
// 【3. 為什麼用 make_unique 而不是 unique_ptr<T>(new T(...))】
//   (a) 少寫一次型別名稱，也不會寫錯。
//   (b) 例外安全：在 C++17 之前，
//       `f(unique_ptr<A>(new A), unique_ptr<B>(new B))` 的參數求值順序未指定，
//       可能先 new A、再 new B、其中一個建構失敗就漏掉另一個。
//       make_unique 把「配置」與「接管」綁成一個不可分割的動作，杜絕這個空窗。
//       （C++17 起收緊了求值順序規則，這個特定漏洞已被堵住，
//        但 make_unique 仍是較清楚、較不易出錯的寫法。）
//
// 【4. 陣列版本】
//   `make_unique<int[]>(5)` 回傳 unique_ptr<int[]>，
//   它的特化版本會正確地呼叫 delete[]（而不是 delete），
//   並且支援 arr[i] 語法、不提供 operator->。
//   實務上若需要動態陣列，通常 std::vector 是更好的選擇
//   （它還帶有 size、迭代器、可增長）；
//   unique_ptr<T[]> 適合「大小固定、且不需要 vector 那些功能」的場合。
//
// 【概念補充 Concept Deep Dive】
//   unique_ptr 為什麼「不能複製、只能移動」？
//   因為它的複製建構函數與複製賦值運算子被明確標記為 = delete。
//   移動則是把裡面的指標交出去、並把來源設為 nullptr ——
//   這一點與一般「有效但未指定」的移動不同，
//   標準明確保證被移動後的 unique_ptr 等於 nullptr。
//   這讓「所有權轉移」成為一個可以安全依賴的行為。
//
//   自訂 deleter 也是它的重要能力：
//   `std::unique_ptr<FILE, decltype(&std::fclose)> fp(std::fopen(...), &std::fclose);`
//   可以用同一套 RAII 管理任何資源（檔案、socket、C 函式庫的 handle），
//   不限於 new 出來的記憶體。注意帶狀態的 deleter 會讓 sizeof 變大。
//
// 【注意事項 Pay Attention】
// 1. 不要對同一個裸指標建立兩個 unique_ptr —— 那會 double free。
//    正確做法是一開始就用 make_unique，讓裸指標根本不出現。
// 2. unique_ptr<T> 與 unique_ptr<T[]> 是不同的特化，
//    前者用 delete、後者用 delete[]。用錯是未定義行為，
//    使用 make_unique 就不會有這個問題。
// 3. .get() 取出的裸指標只能借用、不可 delete，也不可再交給另一個智能指標。
// 4. 若類別的成員是 unique_ptr，該類別會自動變成「不可複製、可移動」
//    （Rule of Zero）。這通常正是你要的，但若你需要可複製的語意，
//    就得自己實作深複製。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】智能指標
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. unique_ptr 相比裸指標有什麼執行期成本？
//     答：通常沒有。unique_ptr 內部只有一個指標，
//         預設的 deleter 是無狀態的空類別，經 empty base optimization
//         不佔額外空間，因此 sizeof(unique_ptr<T>) 一般等於 sizeof(T*)。
//         解參考與成員存取都會被 inline 成與裸指標相同的指令。
//         成本是零，換到的是「不可能忘記 delete」與例外安全。
//     追問：那 shared_ptr 呢？
//         → 有實際成本。它額外持有一個控制區塊指標
//           （sizeof 通常是兩個指標），且引用計數的增減是原子操作，
//           在多執行緒下尤其明顯。所以預設用 unique_ptr，
//           只有真正需要共享所有權時才用 shared_ptr。
//
// 🔥 Q2. 為什麼建議用 make_unique 而不是 unique_ptr<T>(new T(...))？
//     答：一是少寫一次型別、不易寫錯；二是例外安全。
//         在 C++17 之前，函式呼叫的參數求值順序未指定，
//         `f(unique_ptr<A>(new A), unique_ptr<B>(new B))` 可能先做完兩個 new
//         才建構智能指標，其中一個建構失敗就會漏掉另一塊記憶體。
//         make_unique 把配置與接管綁成不可分割的一步，消除這個空窗。
//     追問：C++17 收緊求值順序之後，是不是就可以隨便寫了？
//         → 那個特定漏洞確實被堵住了，但 make_unique 仍然較簡潔、
//           較不易出錯，而且能讓程式碼中完全不出現裸 new，
//           仍應作為預設寫法。
//
// ⚠️ 陷阱. 「unique_ptr 會自動管理記憶體，
//           所以我可以把同一個 new 出來的指標交給兩個 unique_ptr，
//           反正它們會自己協調。」
//     答：絕對不行，那是 double free。unique_ptr 之間沒有任何協調機制 ——
//         「unique」的意思正是「我假設只有我一個擁有者」。
//         兩個 unique_ptr 各自持有同一個位址，解構時會各 delete 一次，
//         這是未定義行為。（會做引用計數協調的是 shared_ptr，
//         但即使是 shared_ptr，也必須用同一個 shared_ptr 去複製，
//         而不是拿同一個裸指標各自建構 —— 那樣會產生兩個獨立的控制區塊，
//         同樣會 double free。）
//     為什麼會錯：把智能指標想成「有垃圾回收的指標」。
//         它其實只是一個在解構時呼叫 delete 的薄包裝，
//         所有權必須由你在設計時決定清楚。
//         正確做法是一開始就用 make_unique，讓裸指標根本沒有機會被共享。
// ═══════════════════════════════════════════════════════════════════════════
//
// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】—— 本檔不附
//   理由：本檔主題是「所有權管理與自動釋放」，屬於資源管理機制。
//   LeetCode 的樹／鏈結串列題目雖然會 new 節點，但評測只看回傳結果，
//   單次執行結束即回收，不會因為用不用智能指標而有不同答案
//   （實務上這類題目反而常刻意用裸指標以符合題目給定的節點定義）。
//   依規格「寧缺勿濫」從缺；本檔的「舊方式 vs 新方式」對照
//   本身就是最直接的實務指引。
// -----------------------------------------------------------------------------

#include <iostream>
#include <string>
#include <memory>    // 智能指標
using namespace std;

class Monster {
private:
    string name;
public:
    Monster(const string& n) : name(n) {
        cout << "  [+] " << name << " 出現" << endl;
    }
    ~Monster() {
        cout << "  [-] " << name << " 消失" << endl;
    }
    void roar() const {
        cout << "  " << name << " 吼叫！" << endl;
    }
};

int main() {
    cout << "=== 裸 new/delete vs 智能指標 ===" << endl;
    
    // ====== 舊方式：裸 new/delete ======
    cout << "\n--- 舊方式（裸指標）---" << endl;
    {
        Monster* m = new Monster("哥布林");
        m->roar();
        delete m;       // 必須手動 delete！否則會洩漏記憶體
    }
    
    // ====== 新方式：unique_ptr ======
    cout << "\n--- 新方式（unique_ptr）---" << endl;
    {
        // make_unique 會自動調用 new，unique_ptr 離開作用域時自動 delete
        // 這裡我們創建了一個 unique_ptr，指向一個 Monster 對象
        // unique_ptr 是一種智能指標，確保在離開作用域時自動釋放資源
        unique_ptr<Monster> m = make_unique<Monster>("史萊姆");
        m->roar();
        // 不需要 delete！unique_ptr 自動處理！即使在這裡發生異常，unique_ptr 也會確保記憶體被釋放，避免洩漏。
    }
    
    // ====== 異常安全 ======
    cout << "\n--- 異常安全示範 ---" << endl;
    try {
        unique_ptr<Monster> m = make_unique<Monster>("龍");
        m->roar();
        throw runtime_error("勇者逃跑了！");
        // m 在堆疊展開時自動解構，不會洩漏記憶體, 即使發生異常也能確保資源被釋放。
    } catch (const exception& e) {
        cout << "  " << e.what() << endl;
    }
    
    // ====== 動態陣列 ======
    cout << "\n--- 動態陣列（unique_ptr）---" << endl;
    {
        unique_ptr<int[]> arr = make_unique<int[]>(5);
        for (int i = 0; i < 5; i++) {
            arr[i] = (i + 1) * 10;
        }
        
        cout << "  ";
        for (int i = 0; i < 5; i++) {
            cout << arr[i] << " ";
        }
        cout << endl;
        // 不需要 delete[]！unique_ptr 自動處理！即使在這裡發生異常，unique_ptr 也會確保記憶體被釋放，避免洩漏。
    }
    
    cout << "\n=== 所有記憶體已自動管理 ===" << endl;
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 19 課：動態對象的創建與銷毀（new  delete）9.cpp" -o smartptr

// ── 輸出但書 ────────────────────────────────────────────────────────────
// 1. 本檔輸出逐位元組可重現（實測連跑 3 次 md5 相同）。
// 2. 判讀重點：四段的每個 [+] 都有對應的 [-]，但只有第一段
//    自己寫了 delete；後三段一行 delete 都沒有，仍然全部正確釋放。
// 3. 「異常安全示範」中 [-] 龍 消失 出現在錯誤訊息之前，
//    證明 unique_ptr 是在堆疊展開期間被解構的，而非 catch 之後。
// 4. 文中的 sizeof 主張已在本機實測確認（非推測）：
//      sizeof(std::unique_ptr<int>) = 8
//      sizeof(int*)                 = 8   ← 與裸指標相同，零額外空間
//      sizeof(std::shared_ptr<int>) = 16  ← 多一個控制區塊指標
//    這些數值屬實作定義（GCC 15.2.0 / libstdc++ / x86-64 LP64）。
// 5. 動態陣列那段用 unique_ptr<int[]>，其特化會正確呼叫 delete[]。
//    實務上多數情況 std::vector 是更好的選擇。
// 6. 本機環境：GCC 15.2.0 (Ubuntu 15.2.0-16ubuntu1) / libstdc++ / x86-64。

// === 預期輸出 ===
// === 裸 new/delete vs 智能指標 ===
//
// --- 舊方式（裸指標）---
//   [+] 哥布林 出現
//   哥布林 吼叫！
//   [-] 哥布林 消失
//
// --- 新方式（unique_ptr）---
//   [+] 史萊姆 出現
//   史萊姆 吼叫！
//   [-] 史萊姆 消失
//
// --- 異常安全示範 ---
//   [+] 龍 出現
//   龍 吼叫！
//   [-] 龍 消失
//   勇者逃跑了！
//
// --- 動態陣列（unique_ptr）---
//   10 20 30 40 50 
//
// === 所有記憶體已自動管理 ===
