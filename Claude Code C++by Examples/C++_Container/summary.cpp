/*
================================================================================
【C++_Container/summary.cpp】

本目錄主題：STL 容器（以「可當課件複習」為目標的總整理）

本目錄檔案涵蓋的容器族群：
  - Sequence containers：array / vector / deque / list / forward_list
  - Container adapters ：stack / queue / priority_queue
  - Associative        ：set / multiset / map / multimap（通常紅黑樹）
  - Unordered          ：unordered_set / unordered_multiset / unordered_map / unordered_multimap（雜湊表）
  - span（C++20 view，本 summary 只在附錄提示）

你這份 summary 的使用方式（課件式）：
  - 主線只展示「工作中最常用」的 member functions，且每個都用一個很小很直觀的例子示範
  - 附錄提供「cppreference 分類的 member functions 速查表」
    - 但對於極少用的 API（或 C++20+）只列出名稱 + 用途提示，不硬塞進主線範例

編譯：
  g++ -std=c++17 -Wall -Wextra summary.cpp -o summary && ./summary
================================================================================
*/

/*
補充筆記：C++_Container/C++_Container summary
  - 如果兩個範例看起來都能完成同一件事，優先比較它們是否擁有資料、是否配置記憶體、是否改變輸入。
  - C++_Container/C++_Container summary 的核心是資料如何儲存、查找與保持順序；選容器前先想插入刪除位置、查找方式和 iterator 失效規則。
  - vector 連續記憶體適合索引和快取區域性，list/deque/set/map 則針對不同插入刪除或查找需求。
  - 關聯容器 set/map 依比較器排序，unordered 容器依 hash 分桶；兩者不是誰永遠比較快，而是前提與需求不同。
  - operator[] 在 map/unordered_map 查不到 key 會插入預設值；純查詢應使用 find、contains 或 at。
  - 容器元素型別若昂貴，優先理解 emplace、move 和 reference/iterator 有效性，不要盲目複製。
  - 所有容器都要考慮空容器邊界；front/back/top 在空容器上呼叫通常是未定義行為或前置條件違反。
  - 這個 summary.cpp 只做章節整理，不新增題庫題解；需要實作練習時回到各主題檔。
  - C++_Container/C++_Container summary 的複習方式是把 API 依用途分組，再比較輸入條件、輸出語意、失敗狀態和複雜度。
  - 初學複習 summary 時，不要只背函式名稱；要能說出何時該用、何時不該用、和相近工具差在哪裡。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】STL 容器總覽
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 實務上你怎麼選容器？請講一個決策流程。
//     答：先問三件事：(1) 需不需要依 key 查找？(2) 需不需要有序？(3) 插入刪除發生在哪裡？
//         不需 key 查找 → sequence container：預設 vector；頭尾都要 O(1) → deque；
//         需要 iterator / reference 穩定性或 O(1) splice → list。
//         需 key 查找：要有序、要 range query → map / set（O(log n)）；
//         不在乎順序、要均攤 O(1) → unordered_map / unordered_set。允許重複就加 multi。
//     追問：為什麼預設是 vector？（連續記憶體的 cache locality 在實測上常壓過理論複雜度優勢）
//
// 🔥 Q2. container 和 container adaptor 的差別？
//     答：container（vector / list / map / unordered_map …）自己管理儲存並提供 iterator；
//         container adaptor（stack / queue / priority_queue）只是包裝另一個容器、只暴露特定介面，
//         因此沒有 iterator、不能用 range-for、也沒有 clear。
//         預設底層：stack / queue 是 std::deque，priority_queue 是 std::vector。
//
// Q3. 各容器的 iterator 分別是哪一類？
//     答：vector / array → contiguous（C++20）；deque → random access 但 **不是** contiguous（分段儲存）；
//         list 與 set / map / multiset / multimap → bidirectional；
//         forward_list 與所有 unordered_* → forward（底層是 singly linked list，沒有 operator--）；
//         stack / queue / priority_queue → 沒有 iterator。
// ═══════════════════════════════════════════════════════════════════════════

#include <algorithm>
#include <deque>
#include <forward_list>
#include <iostream>
#include <list>
#include <map>
#include <stack>
#include <queue>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// -----------------------------------------------------------------------------
// 【重點 1】容器選型速查（超實用）
// -----------------------------------------------------------------------------
// - vector:
//   - 連續記憶體、隨機存取 O(1)、尾端 push/pop 攤銷 O(1)
//   - 中間插入刪除 O(n)（搬移元素）
//   - 需要 cache 友善/與 C API 互動 → 首選
//
// - deque:
//   - 兩端 push/pop O(1)，但不是連續記憶體（分段）
//   - 隨機存取仍是 O(1)，常數略大
//   - 需要「雙端佇列」語意 → deque
//
// - list / forward_list:
//   - 節點容器：插入/刪除本身 O(1)（已知位置）
//   - 但找位置要 O(n)，且不連續、cache 差、記憶體 overhead 大
//   - list：雙向；forward_list：單向（更省）
//
// - set/map（紅黑樹）：
//   - 有序、可做 lower_bound/upper_bound/range query
//   - 插入/查找/刪除 O(log n)
//
// - unordered_set/unordered_map（雜湊表）：
//   - 平均 O(1) 插入/查找/刪除，最壞 O(n)
//   - 不保序；需要自訂 hash 時要小心品質
//
// - stack/queue/priority_queue：
//   - 容器適配器：限制介面，強化語意
//   - stack/queue 預設以 deque 為底層；priority_queue 預設以 vector + heap
// -----------------------------------------------------------------------------

template <class C>
static void print_range(const C& c, const std::string& label) {
    std::cout << label << ":";
    for (const auto& x : c) std::cout << ' ' << x;
    std::cout << "\n";
}

static void header(const char* name) { std::cout << "\n[" << name << "]\n"; }

// -----------------------------------------------------------------------------
// 【重點 2】Sequence containers（vector / deque / list / forward_list）
// -----------------------------------------------------------------------------
static void demo_vector_common_members() {
    header("demo_vector_common_members");

    // constructors / initializer_list
    std::vector<int> v{3, 1, 4};
    std::cout << "  size=" << v.size() << " capacity=" << v.capacity() << "\n";

    // capacity / reserve
    v.reserve(16);
    std::cout << "  after reserve(16): capacity=" << v.capacity() << "\n";

    // push_back / emplace_back
    v.push_back(1);
    v.emplace_back(5);
    print_range(v, "  after push_back/emplace_back");

    // element access：operator[] / at / front / back / data
    std::cout << "  v[0]=" << v[0] << ", at(1)=" << v.at(1)
              << ", front=" << v.front() << ", back=" << v.back() << "\n";
    std::cout << "  data()[0]=" << v.data()[0] << " (連續記憶體)\n";

    // insert / erase（中間操作 O(n)）
    v.insert(v.begin() + 1, 99);
    print_range(v, "  after insert(pos,99)");
    v.erase(v.begin() + 1);
    print_range(v, "  after erase(pos)");

    // clear / empty
    std::vector<int> tmp = v;
    tmp.clear();
    std::cout << "  clear(): size=" << tmp.size() << " empty=" << tmp.empty() << "\n";

    // sort（演算法，不是 member function，但最常與 vector 一起用）
    std::sort(v.begin(), v.end());
    print_range(v, "  sort(v.begin(),v.end())");
}

static void demo_deque_common_members() {
    header("demo_deque_common_members");

    std::deque<int> dq{2, 3};
    dq.push_front(1);
    dq.push_back(4);
    print_range(dq, "  after push_front/push_back");

    // front/back
    std::cout << "  front=" << dq.front() << ", back=" << dq.back() << "\n";

    // pop_front/pop_back
    dq.pop_front();
    dq.pop_back();
    print_range(dq, "  after pop_front/pop_back");

    // insert/erase（中間仍是 O(n)，但兩端是 O(1)）
    dq.insert(dq.begin(), 9);
    dq.erase(dq.begin());
}

static void demo_list_common_members() {
    header("demo_list_common_members");

    std::list<int> ls{1, 2, 4};

    // push_front/push_back（兩端 O(1)）
    ls.push_front(0);
    ls.push_back(5);
    print_range(ls, "  after push_front/push_back");

    // insert（已知 iterator 位置時 O(1)）
    auto it = std::find(ls.begin(), ls.end(), 4);
    ls.insert(it, 3);
    print_range(ls, "  insert before 4");

    // erase（已知 iterator 位置時 O(1)）
    it = std::find(ls.begin(), ls.end(), 2);
    if (it != ls.end()) ls.erase(it);
    print_range(ls, "  erase value 2");

    // splice：把另一個 list 的節點「接過來」（幾乎 O(1)，不拷貝元素）
    std::list<int> other{7, 8};
    ls.splice(ls.end(), other); // other 會變空
    print_range(ls, "  splice from other");
}

static void demo_forward_list_common_members() {
    header("demo_forward_list_common_members");

    std::forward_list<int> fl{1, 2, 4};
    fl.push_front(0); // 只有 push_front

    // insert_after / erase_after：forward_list 的核心 API
    auto prev = fl.before_begin();
    for (auto cur = fl.begin(); cur != fl.end(); ++cur) {
        if (*cur == 4) break;
        ++prev;
    }
    fl.insert_after(prev, 3); // 在 4 前面插入 3
    print_range(fl, "  insert_after before 4");

    // erase_after：刪掉某個節點之後的節點
    auto p = fl.before_begin();
    fl.erase_after(p); // 刪掉原本的第一個元素（0）
    print_range(fl, "  erase_after(before_begin)");
}

static void demo_associative() {
    header("demo_associative (set/map)");

    // set：有序且去重
    std::set<int> s{3, 1, 4, 1};
    print_range(s, "  set(sorted unique)");

    // multiset：允許重複
    std::multiset<int> ms{3, 1, 4, 1};
    std::cout << "  multiset count(1)=" << ms.count(1) << "\n";

    // map：有序 key->value（operator[] 會必要時插入預設值）
    std::map<std::string, int> m;
    m["apple"] = 3;
    m["banana"] = 2;
    for (const auto& kv : m) {
        std::cout << "  map: " << kv.first << " => " << kv.second << "\n";
    }

    // lower_bound：range query 的核心（unordered_* 做不到）
    auto itb = m.lower_bound("banana");
    if (itb != m.end()) std::cout << "  lower_bound(\"banana\")=" << itb->first << "\n";

    // find / erase
    auto it = m.find("apple");
    if (it != m.end()) {
        m.erase(it);
        std::cout << "  erase(\"apple\") ok, size=" << m.size() << "\n";
    }
}

static void demo_unordered() {
    header("demo_unordered (unordered_set/unordered_map)");

    std::unordered_set<int> us{3, 1, 4, 1};
    std::cout << "  unordered_set has 4? " << (us.find(4) != us.end()) << "\n";

    std::unordered_map<std::string, int> um;
    um.emplace("alice", 10);
    um.emplace("bob", 20);
    std::cout << "  unordered_map[alice]=" << um["alice"] << "\n";

    // 注意：operator[] 會「必要時插入預設值」，查詢只讀請用 find/at
    // std::cout << um["not_exist"]; // 會插入 key="not_exist", value=0

    // at：若 key 不存在會丟 out_of_range（適合「必須存在」的情境）
    try {
        std::cout << "  at(\"bob\")=" << um.at("bob") << "\n";
        // um.at("not_exist"); // 會丟例外
    } catch (const std::out_of_range& e) {
        std::cout << "  out_of_range: " << e.what() << "\n";
    }
}

static void demo_adapters() {
    header("demo_adapters (stack/queue/priority_queue)");

    std::stack<int> st;
    st.push(1);
    st.push(2);
    std::cout << "  stack top=" << st.top() << "\n";
    st.pop();

    std::queue<int> q;
    q.push(1);
    q.push(2);
    std::cout << "  queue front=" << q.front() << ", back=" << q.back() << "\n";
    q.pop();

    // priority_queue：預設最大堆（top 是最大值）
    std::priority_queue<int> pq;
    for (int x : {3, 1, 4, 2}) pq.push(x);
    std::cout << "  priority_queue top=" << pq.top() << "\n";
}

int main() {
    demo_vector_common_members();
    demo_deque_common_members();
    demo_list_common_members();
    demo_forward_list_common_members();
    demo_associative();
    demo_unordered();
    demo_adapters();

    std::cout << "\n[done]\n";
    return 0;
}

/*
================================================================================
【附錄：容器 member functions 速查（以「常用」為主）】

（完整列表請以 cppreference 對應各容器頁面為準；本附錄用來複習，不追求逐字列全。）

vector / deque（sequence）
  - constructors, operator=, assign
  - begin/end/rbegin/rend + cbegin/cend...
  - size/empty/capacity/reserve/shrink_to_fit(vector)
  - operator[]/at/front/back/data(vector)
  - insert/emplace/erase
  - push_back/emplace_back/pop_back
  - clear/resize/swap

list / forward_list（node-based）
  - push_front / push_back(list) / pop_front / pop_back(list)
  - insert / erase
  - splice(list), merge(list), sort(list), unique(list), remove/remove_if(list)
  - insert_after / erase_after(forward_list)

map/set（ordered associative, tree）
  - find/count/contains(C++20)/lower_bound/upper_bound/equal_range
  - insert/emplace/try_emplace(map)/insert_or_assign(map)
  - erase/clear/swap
  - operator[](map) / at(map)

unordered_map/unordered_set（hash）
  - bucket interface（bucket_count, load_factor, rehash, reserve）
  - find/count/contains(C++20)
  - insert/emplace/erase
  - operator[](unordered_map) / at(unordered_map)

container adapters
  - stack: push/emplace/pop/top/size/empty
  - queue: push/emplace/pop/front/back/size/empty
  - priority_queue: push/emplace/pop/top/size/empty

================================================================================
*/

// 編譯: g++ -std=c++20 -Wall -Wextra summary.cpp -o summary

// === 預期輸出 (節錄) ===
//
// [demo_vector_common_members]
//   size=3 capacity=3
//   after reserve(16): capacity=16
//   after push_back/emplace_back: 3 1 4 1 5
//   v[0]=3, at(1)=1, front=3, back=5
//   data()[0]=3 (連續記憶體)
//   after insert(pos,99): 3 99 1 4 1 5
//   after erase(pos): 3 1 4 1 5
//   clear(): size=0 empty=1
//   sort(v.begin(),v.end()): 1 1 3 4 5
//
// [demo_deque_common_members]
//   after push_front/push_back: 1 2 3 4
//   front=1, back=4
//   after pop_front/pop_back: 2 3
//
// [demo_list_common_members]
//   after push_front/push_back: 0 1 2 4 5
//   insert before 4: 0 1 2 3 4 5
// …（後略，完整輸出共 46 行）
