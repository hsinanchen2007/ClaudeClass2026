/*
================================================================================
主題:std::move —— 把物件「轉成右值」以啟用移動語意
標準:C++11 起
標頭:<utility>
參考:https://en.cppreference.com/w/cpp/utility/move
================================================================================

【一、課題介紹】
  std::move 是新手最容易誤解的工具之一。先說結論:
    **std::move 不會「移動」任何東西,它只是個 cast。**

  它把一個變數「轉成右值參考(rvalue reference)」,告訴編譯器:
  「這個物件的內容你可以拿走」。真正執行搬運動作的是 move constructor /
  move assignment;沒有這些函式時,就會退回拷貝。

  為什麼需要它?
    在 C++11 之前,從 a 把資源「搬」給 b 需要拷貝整份(O(n)),
    然後再清空 a。例如把一個 1GB vector 傳給另一個變數要花很久。
    C++11 引入「移動語意」後,這種搬運只要把內部指標換手即可(O(1)),
    舊物件保留在「合法但已清空」的狀態。

【二、觀念解釋】
  1. 標頭:<utility>。
  2. 形式:std::move(x) 等同 static_cast<T&&>(x)。
  3. 觸發移動的兩個條件(任一即可):
       (a) 參數本身就是右值(臨時物件)。
       (b) 透過 std::move 把左值轉成右值。
  4. 移動後的物件「處於有效但未指定狀態」(valid but unspecified):
       - 還能呼叫解構子、能再次賦值。
       - 但不能假設它的值是什麼(別讀!)。
  5. const 物件不能被移動:std::move(const T) 會退回拷貝。
  6. 千萬不要對「準備繼續使用」的變數 std::move,例如:
       std::vector<int> v = {1,2,3};
       takesVector(std::move(v));
       v.push_back(4);                // 危險:v 已被搬走,內容未知
  7. 函式回傳區域變數時,不需要寫 std::move(編譯器自動有 NRVO / 隱式 move)。

【三、常見陷阱】
  - std::move 不真的搬:沒有 move ctor 時還是拷貝。
  - 寫 std::move(v) 之後又用 v:邏輯錯誤。
  - 對函式回傳值寫 return std::move(x):多此一舉、且常常會關掉 NRVO,效能反而變差。
  - const T 用 std::move 等於白做(轉成 const T&&,只能呼叫拷貝建構子)。

【四、與其他 utility 的比較】
  - vs std::forward:forward 是「條件性移動」,只在原本就是右值時才轉成右值,
    用於完美轉發(perfect forwarding);move 則是「無條件轉右值」。
  - vs std::swap:swap 兩物件互換,內部用 move 實作。
  - vs std::exchange:exchange 是「取舊值、放新值」,內部也以 move 實作。

【五、Leetcode 對應題目】
  題號:面試常見:把字串陣列 grouping 成 vector<vector<string>>(類似 #49 Group
        Anagrams)。為了維持簡單,我們用一個自製的小例子展示「把暫存 buckets
        的內容用 std::move 倒進結果」,避免不必要的拷貝。
  難度:Easy 級別示範(不是 LC 原題)。
  選用理由:Group Anagrams 結尾「把 map 中的 vector 倒到結果 vector」是
            std::move 的經典使用場景。

【六、日常工作實用範例】
  情境:函式回傳一個大型容器,呼叫端「接收後立刻塞進另一個容器」,
        中間用 std::move 把搬運成本變成 O(1)。
================================================================================
*/

/*
補充筆記：std::move
  - std::move 演算法會把來源元素轉成右值逐一搬到目的地；來源元素仍有效但值未指定。
  - 目的地仍需有足夠空間，move 不會自動 push_back。
  - 移動後的容器通常只適合清空、重新賦值或銷毀，不應再讀取原內容當作有效資料。
  - std::move 屬於 utility 類工具；這些型別與函式常用來表達小型資料組合、可選值、型別安全聯集或值類別轉換。
  - pair/tuple 適合簡短聚合結果，但欄位語意複雜時應定義具名 struct，避免 first/second 或 get<0> 難讀。
  - optional 表示可能沒有值，使用前要檢查 has_value 或使用 value_or；value() 在無值時會丟例外。
  - variant 表示多選一型別，應用 visit 或 holds_alternative/get_if 安全存取目前替代項。
  - any 提供執行期任意型別保存，但取回需要知道正確型別；過度使用會失去靜態型別檢查優勢。
  - std::move/std::forward/std::exchange/as_const 都是表達意圖的工具；它們本身不一定搬移或複製資料。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::move 與移動語意
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::move 到底做了什麼？
//     答：什麼都沒搬。它就是一個 static_cast<T&&>，把引數無條件轉成 xvalue（右值），
//     讓後續的重載決議選中移動建構／移動賦值。真正的搬移發生在那些移動函式裡；
//     std::move 本身編譯後不產生任何指令。
//     追問：被 move 之後的物件還能用嗎？（處於「有效但未指定」狀態：可安全解構、可賦
//     新值，但不可假設其內容。標準容器被 move 後通常變空，那不是所有型別的通則）
//
// 🔥 Q2. 為什麼移動建構函式應該標 noexcept？
//     答：vector 擴容時要把舊元素搬到新記憶體。若元素的移動建構函式不是 noexcept，
//     vector 為了維持強例外保證（搬到一半拋例外時舊資料仍完整）會改用複製，效能大幅
//     下降。標準以 std::move_if_noexcept 實作這個決策。忘了標 noexcept 不會有任何警告，
//     只會默默變慢。
//     ⚠️ 但別把話講死：「沒 noexcept 就一定 copy」只對【可 copy 的型別】成立。
//        move-only 型別（copy ctor = delete）根本沒有 copy 可退，即使 move ctor 沒標
//        noexcept 仍然會 move——只是這時 vector 給不出強保證。
//        本機實測：可 copy + move 未標 noexcept → COPY；move-only + move 未標 → MOVE。
//     追問：怎麼驗證？（static_assert(std::is_nothrow_move_constructible_v<T>);）
//
// ⚠️ 陷阱 1. 對一個 const 物件呼叫 std::move 會發生什麼？
//     答：發生的是「複製」。const std::string s; std::string t = std::move(s); 中
//     std::move(s) 的型別是 const std::string&&，移動建構函式的參數是 std::string&&
//     綁不上，但複製建構函式的 const std::string& 綁得上 → 重載決議選中複製，而且完全
//     沒有警告。推論：把成員宣告成 const 會讓整個 class 失去移動能力。
//     為什麼會錯：以為 std::move 是「動作」，寫了就會搬；它只是轉型，搬不搬由重載決議決定。
//
// ⚠️ 陷阱 2. return std::move(local); 是好習慣嗎？
//     答：不是，反而有害——它會阻止 NRVO，讓編譯器從「完全省略建構」退化成「執行一次
//     移動」。正確寫法是 return local;，編譯器會先嘗試 NRVO，不成功時也會自動把它當
//     右值處理。例外：回傳的是函式參數，或回傳型別與區域變數型別不同（例如區域是
//     unique_ptr<Derived>、回傳型別是 unique_ptr<Base>）時，才需要顯式 std::move。
//     為什麼會錯：直覺是「加了 move 一定不會更慢」，但 copy elision 比 move 更省。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <utility>
#include <string>
#include <vector>
#include <unordered_map>

// ---------------------------------------------------------------------------
// 範例 1:std::move 把 vector 從 a 轉到 b
//
// move 之後 a 的內容「未指定」,千萬不要再用 a 的資料。
// 這裡 a.empty() 在大部分實作會是 true,但別當成保證。
// ---------------------------------------------------------------------------
void demo_basic() {
    std::cout << "[demo_basic]\n";
    std::vector<int> a = {1, 2, 3, 4, 5};
    std::vector<int> b = std::move(a);            // 把 a 的內部指標搬給 b
    std::cout << "  b.size=" << b.size()
              << ", b[0]=" << b[0] << "\n";
    std::cout << "  a.size after move = " << a.size()
              << " (valid but unspecified)\n";
}

// ---------------------------------------------------------------------------
// 範例 2:配合 push_back —— 拷貝 vs 移動
//
// 對「已不再用」的字串,push_back(std::move(s)) 可省下整份字串拷貝。
// ---------------------------------------------------------------------------
void demo_push_back() {
    std::cout << "[demo_push_back]\n";
    std::vector<std::string> v;
    std::string s = "this is a long string that costs to copy";

    v.push_back(s);                               // 拷貝(s 之後還能用)
    v.push_back(std::move(s));                    // 移動(s 之後別再讀)
    std::cout << "  v[0]=" << v[0] << "\n";
    std::cout << "  v[1]=" << v[1] << "\n";
    std::cout << "  s after move = [" << s << "]  (unspecified)\n";
}

// ---------------------------------------------------------------------------
// 範例 2.5:std::move_if_noexcept 與「const 不能移動」的觀察
//
//   - std::move_if_noexcept(x):條件式移動。如果 x 的型別有 noexcept 的
//     move ctor(或無拷貝建構子可選),就回傳右值參考(可移動);否則回傳
//     const 左值參考(只能拷貝)。常用於 std::vector 在重新配置時:
//     若 move ctor 沒標 noexcept,vector 寧可拷貝以維持強例外保證。
//
//   - const 物件對 std::move 沒效果:會推導出 const T&&,move ctor 接不到,
//     最終仍走拷貝建構子。下面用 NoexceptMove / ThrowingMove 兩個小型別
//     呈現差異。
// ---------------------------------------------------------------------------
struct NoexceptMove {
    NoexceptMove() = default;
    NoexceptMove(const NoexceptMove&)            { std::cout << "  [NoexceptMove copy]\n"; }
    NoexceptMove(NoexceptMove&&) noexcept        { std::cout << "  [NoexceptMove move]\n"; }
};
struct ThrowingMove {
    ThrowingMove() = default;
    ThrowingMove(const ThrowingMove&)            { std::cout << "  [ThrowingMove copy]\n"; }
    ThrowingMove(ThrowingMove&&) /*無 noexcept*/ { std::cout << "  [ThrowingMove move]\n"; }
};

void demo_move_if_noexcept_and_const() {
    std::cout << "[demo_move_if_noexcept_and_const]\n";

    // (a) 有 noexcept move:走移動
    NoexceptMove a;
    NoexceptMove b = std::move_if_noexcept(a);
    (void)b;

    // (b) 沒 noexcept 的 move:move_if_noexcept 退回拷貝(保護強例外保證)
    ThrowingMove c;
    ThrowingMove d = std::move_if_noexcept(c);
    (void)d;

    // (c) const 物件 std::move:看似要 move,實際呼叫到拷貝建構子
    const NoexceptMove e;
    NoexceptMove f = std::move(e);                  // 這行其實是拷貝!
    (void)f;
}

// ---------------------------------------------------------------------------
// 範例 3:Leetcode 風格 —— Group by length(分組)
//
// 把字串依「長度」分組,結尾再把每個 bucket 的 vector 用 std::move 倒進結果,
// 避免拷貝整個 vector<string>。這是 LC #49 Group Anagrams 解法的標準收尾。
// ---------------------------------------------------------------------------
std::vector<std::vector<std::string>>
groupByLength(std::vector<std::string> words) {
    std::unordered_map<std::size_t, std::vector<std::string>> buckets;
    for (auto& w : words) {
        // 把 w 移進 bucket(別之後再讀 w)
        buckets[w.size()].push_back(std::move(w));
    }

    std::vector<std::vector<std::string>> result;
    result.reserve(buckets.size());
    for (auto& kv : buckets) {
        // 把每個 bucket 整個搬進結果,而不是拷貝
        result.push_back(std::move(kv.second));
    }
    return result;
}

void demo_group_by_length() {
    std::cout << "[demo_group_by_length]\n";
    auto groups = groupByLength({"cat","dog","apple","bee","fish"});
    for (const auto& g : groups) {
        std::cout << "  group(size=" << g[0].size() << "):";
        for (const auto& s : g) std::cout << " " << s;
        std::cout << "\n";
    }
}

// ---------------------------------------------------------------------------
// 範例 4:日常工作實用範例 —— 把工廠函式的回傳值「搬」進成員
//
// 情境:有一個 Repository 類別,它呼叫 buildPayload() 得到一個大字串,
//       要存進自己的成員。直接用 std::move 把字串搬進去,而非拷貝。
// ---------------------------------------------------------------------------
class Repository {
public:
    void load() {
        std::string payload = buildPayload();     // 暫存
        // 把 payload 搬進成員,省下整段字串的拷貝
        cache_ = std::move(payload);
    }
    const std::string& cache() const { return cache_; }

private:
    static std::string buildPayload() {
        return std::string(50, '*');              // 模擬一段大字串
    }
    std::string cache_;
};

void demo_practical_repo() {
    std::cout << "[demo_practical_repo]\n";
    Repository r;
    r.load();
    std::cout << "  cache size = " << r.cache().size() << "\n";
}

// ---------------------------------------------------------------------------
// 實用範例 (額外):consume_and_clear —— 工作隊列的「拿走全部」模式
//
// 工作中常見:work_queue 處理者要「一次拿走目前所有 task,清空 queue,
// 然後慢慢處理」。如果是內部 std::vector<Task>,用 std::move(queue) 是 O(1)。
// (queue 必須允許「被搬走後再使用」 — 把它變成空 vector 即可。)
// ---------------------------------------------------------------------------
class WorkQueue {
public:
    void enqueue(std::string task) { tasks_.push_back(std::move(task)); }
    std::vector<std::string> drain() {
        std::vector<std::string> out = std::move(tasks_);    // O(1) 搬移
        tasks_.clear();                                       // 確保是空狀態
        return out;                                           // RVO + move
    }
    size_t size() const { return tasks_.size(); }
private:
    std::vector<std::string> tasks_;
};

void demo_practical_work_queue() {
    std::cout << "[demo_practical_work_queue]\n";
    WorkQueue q;
    q.enqueue("send email");
    q.enqueue("update db");
    q.enqueue("flush cache");
    std::cout << "  before drain, size=" << q.size() << "\n";
    auto batch = q.drain();
    std::cout << "  after drain, size=" << q.size() << ", batch=" << batch.size() << "\n";
    for (auto& t : batch) std::cout << "    -> " << t << "\n";
}

int main() {
    demo_basic();
    demo_push_back();
    demo_move_if_noexcept_and_const();
    demo_group_by_length();
    demo_practical_repo();
    demo_practical_work_queue();
    return 0;
}

/*
================================================================================
編譯與執行:
    g++ -std=c++17 -Wall -Wextra 07_move.cpp -o 07_move && ./07_move
================================================================================
*/

// 編譯: g++ -std=c++20 -Wall -Wextra 07_move.cpp -o 07_move

// === 預期輸出 ===
// [demo_basic]
//   b.size=5, b[0]=1
//   a.size after move = 0 (valid but unspecified)
// [demo_push_back]
//   v[0]=this is a long string that costs to copy
//   v[1]=this is a long string that costs to copy
//   s after move = []  (unspecified)
// [demo_move_if_noexcept_and_const]
//   [NoexceptMove move]
//   [ThrowingMove copy]
//   [NoexceptMove copy]
// [demo_group_by_length]
//   group(size=4): fish
//   group(size=5): apple
//   group(size=3): cat dog bee
// [demo_practical_repo]
//   cache size = 50
// [demo_practical_work_queue]
//   before drain, size=3
//   after drain, size=0, batch=3
//     -> send email
//     -> update db
//     -> flush cache
