/*
================================================================================
主題:std::any —— 可裝任何型別的容器(動態型別)
標準:C++17 起
標頭:<any>
參考:https://en.cppreference.com/w/cpp/utility/any
================================================================================

【一、課題介紹】
  std::any 是一個能夠裝「任何可拷貝型別」的型別擦除(type erasure)容器。
  你可以把它想成 C++ 版的「dynamic / object」(類似 C# 的 object 或
  JavaScript 的 any),但拿值時必須明確指定型別。

  為什麼需要它?
    有些情況我們在編譯期不知道值會是什麼型別,例如:
      - 通用設定容器:每個欄位的值型別不同,但介面一致。
      - 外掛系統 / 訊息匯流排:外部模組可塞任意型別進來。
      - 取代 C 的 void* —— any 自帶解構,不會洩漏資源。

【二、觀念解釋】
  1. 標頭:<any>。
  2. 建立:
       std::any a;                       // 空(沒有值)
       std::any b = 42;                  // 裝 int
       std::any c = std::string("hi");   // 裝 string

  3. 是否有值與型別資訊:
       a.has_value();                    // bool
       b.type();                         // 回傳 std::type_info(可比較)
       b.type() == typeid(int);          // 是不是 int

  4. 取值:
       std::any_cast<int>(b);            // 拿錯型別會擲 std::bad_any_cast
       std::any_cast<int>(&b);           // 接指標版:拿不到回 nullptr(推薦)

  5. 重設:
       a.reset();                        // 清空

  6. 注意:存進去的型別必須「可拷貝建構(CopyConstructible)」。
     不可拷貝的型別(例如 std::unique_ptr)不能直接放入 any。

【三、常見陷阱】
  - any_cast 必須與「實際存入的型別完全一致」(包含 const / 引用),
    例如存了 const char*,就不能 any_cast<std::string>;這是常犯錯誤。
  - 性能注意:any 內部多半透過 heap 配置(小物件可能在內部 buffer),
    且取值有 type_info 比對成本。在熱點程式碼中盡量避免,改用 variant。
  - 不要把 any 當成 inheritance 的替代品。

【四、與其他 utility 的比較】
  - vs std::variant:variant 在編譯期就確定可能的型別清單,效能與安全性
                    都好於 any;any 適合「型別清單在編譯期不可知」的場合。
  - vs void*:any 自動管理生命週期、能查型別;void* 完全沒有保護。
  - vs std::optional:optional 是「可能沒有 T」;any 是「可能是任何 T」。

【五、Leetcode 對應題目】
  題號:1672. Richest Customer Wealth(最富有的客戶資產)
  難度:Easy
  連結:https://leetcode.com/problems/richest-customer-wealth/
  題目大意:給一個二維陣列 accounts,accounts[i] 是第 i 位客戶各銀行的存款,
            回傳「資產總和最大的客戶」的總資產。
  選用理由:把每位客戶的「彙整資料」用 std::any 包裝(總資產 / 名稱 / 標籤
            等異質欄位),示範 any 在「異質資料表」場景的常見用法。

【六、日常工作實用範例】
  情境:設定中心的「通用屬性表」—— 每個 key 對應一個值,但值的型別未必相同。
================================================================================
*/

/*
補充筆記：std::any
  - any 是型別擦除容器，可保存任意可拷貝型別，但取回時需要知道真實型別。
  - any_cast 型別錯誤會丟 bad_any_cast 或回傳 null pointer 版本。
  - 能用 variant 表達的封閉型別集合，通常比 any 更安全、更容易維護。
  - std::any 屬於 utility 類工具；這些型別與函式常用來表達小型資料組合、可選值、型別安全聯集或值類別轉換。
  - pair/tuple 適合簡短聚合結果，但欄位語意複雜時應定義具名 struct，避免 first/second 或 get<0> 難讀。
  - optional 表示可能沒有值，使用前要檢查 has_value 或使用 value_or；value() 在無值時會丟例外。
  - variant 表示多選一型別，應用 visit 或 holds_alternative/get_if 安全存取目前替代項。
  - any 提供執行期任意型別保存，但取回需要知道正確型別；過度使用會失去靜態型別檢查優勢。
  - std::move/std::forward/std::exchange/as_const 都是表達意圖的工具；它們本身不一定搬移或複製資料。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::any（C++17）與型別擦除
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::any 和 std::variant 怎麼選？
//     答：候選型別在編譯期已知且有限 → variant（就地儲存、無堆配置、visit 強制窮盡、
//     效能佳）；型別在編譯期未知、是開放集合（外掛、腳本橋接、跨模組傳遞任意資料）
//     → any。實務上大量使用 any 通常代表設計還可以再收斂。
//     追問：any 會堆配置嗎？（大物件會。小物件的實作通常有 small object optimization，
//     但標準只「鼓勵」對小型且 nothrow-move 的型別免配置，並未強制，屬實作品質）
//
// 🔥 Q2. any 怎麼取值？有哪些失敗模式？
//     答：std::any_cast<T>(a) 的值／參考版在型別不完全相符時拋 std::bad_any_cast；
//     指標版 std::any_cast<T>(&a) 不符時回 nullptr、不拋例外。查詢用 a.has_value() 與
//     a.type()（回傳 const std::type_info&）。
//     追問：a.type() 有什麼環境依賴？（需要 RTTI；在 -fno-rtti 的環境下通常不可用，
//     嵌入式領域要注意）
//
// ⚠️ 陷阱. std::any a = 42; 之後 std::any_cast<long>(a) 會得到 42L 嗎？
//     答：不會，會拋 std::bad_any_cast。any_cast 要求型別「精確相符」，完全沒有隱式
//     轉換——存 int 就只能取 int。
//     為什麼會錯：把 any_cast 想成 static_cast 那種「會幫你轉」的轉型，實際上它只是
//     「用正確的型別把東西拿回來」的檢查，不符就是錯誤。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <any>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>

// ---------------------------------------------------------------------------
// 範例 1:基本建立、any_cast、has_value
// ---------------------------------------------------------------------------
void demo_basic() {
    std::cout << "[demo_basic]\n";
    std::any a;
    std::cout << "  a.has_value()=" << std::boolalpha << a.has_value() << "\n";

    a = 123;                                      // 裝 int
    std::cout << "  int = " << std::any_cast<int>(a) << "\n";

    a = std::string("hello");                     // 換裝 string
    std::cout << "  str = " << std::any_cast<std::string>(a) << "\n";

    // 推薦:指標版 any_cast,拿不到回 nullptr,不擲例外
    if (auto p = std::any_cast<int>(&a)) {
        std::cout << "  it is int = " << *p << "\n";
    } else {
        std::cout << "  not int\n";
    }
}

// ---------------------------------------------------------------------------
// 範例 2:any_cast 拿錯型別會擲 std::bad_any_cast
// ---------------------------------------------------------------------------
void demo_bad_cast() {
    std::cout << "[demo_bad_cast]\n";
    std::any a = 42;
    try {
        auto s = std::any_cast<std::string>(a);   // 實際是 int,會擲例外
        (void)s;
    } catch (const std::bad_any_cast& e) {
        std::cout << "  caught: " << e.what() << "\n";
    }
}

// ---------------------------------------------------------------------------
// 範例 2.5:其他常用工具 —— type / reset / emplace / swap / make_any / in_place_type
//
//   - a.type():回傳 std::type_info,可與 typeid(T) 比較得知目前裝什麼型別
//   - a.reset():清空(等同 a = {})
//   - a.emplace<T>(args...):就地把 any 換成 T 型別(避免一次臨時物件 + move)
//   - a.swap(b)、std::swap(a, b):交換內容
//   - std::make_any<T>(args...):類似 make_shared,直接就地建構 T
//   - std::in_place_type<T>:建構時指定「就地建構某個型別」(避免拷貝/移動)
// ---------------------------------------------------------------------------
void demo_any_helpers() {
    std::cout << "[demo_any_helpers]\n";

    // (a) type():取得目前所裝型別的 type_info,可比較
    std::any a = 42;
    std::cout << "  is int? "    << (a.type() == typeid(int)) << "\n";
    std::cout << "  is string? " << (a.type() == typeid(std::string)) << "\n";

    // (b) reset():清空
    a.reset();
    std::cout << "  after reset, has_value=" << a.has_value() << "\n";

    // (c) emplace<T>():就地建構,內部不必先建一個臨時物件
    a.emplace<std::string>(5, '#');                 // std::string(5, '#')
    std::cout << "  after emplace<string>: \"" << std::any_cast<std::string>(a) << "\"\n";

    // (d) swap:成員與非成員形式
    std::any x = 1, y = std::string("two");
    x.swap(y);
    std::cout << "  member swap: x is string? " << (x.type() == typeid(std::string)) << "\n";
    std::swap(x, y);
    std::cout << "  std::swap : x is int? " << (x.type() == typeid(int)) << "\n";

    // (e) std::make_any<T>(args...):類似 make_shared,自動就地建構
    auto m = std::make_any<std::vector<int>>(3, 7); // vector<int>(3, 7) → {7,7,7}
    auto& vref = std::any_cast<std::vector<int>&>(m);
    std::cout << "  make_any<vector>: size=" << vref.size()
              << ", front=" << vref.front() << "\n";

    // (f) std::in_place_type<T>:在建構時就地建立 T
    std::any p(std::in_place_type<std::pair<int, std::string>>, 1, "one");
    auto [k, v] = std::any_cast<std::pair<int, std::string>>(p);
    std::cout << "  in_place_type<pair>: (" << k << "," << v << ")\n";
}

// ---------------------------------------------------------------------------
// 範例 3:Leetcode #1672 Richest Customer Wealth + any 異質資料展示
//
// 解題思路(原版):
//   走訪每位客戶,把該列加總,取最大值。時間 O(n*m),空間 O(1)。
//
// 此處的擴充:
//   把每位客戶的「綜合資料」放在 std::unordered_map<std::string, std::any>
//   裡(例如 wealth、name、is_vip 等異質欄位),最後仍回傳最大資產。
//   這呈現的是「any 用於異質欄位表」的真實用法。
// ---------------------------------------------------------------------------
int maximumWealth(const std::vector<std::vector<int>>& accounts) {
    int best = 0;
    for (const auto& row : accounts) {
        std::unordered_map<std::string, std::any> profile;
        int sum = 0;
        for (int x : row) sum += x;
        profile["wealth"] = sum;
        profile["banks"]  = static_cast<int>(row.size());
        profile["tag"]    = std::string(sum >= 10 ? "rich" : "normal");

        // 從 any 取出 wealth 拿來比較
        int w = std::any_cast<int>(profile.at("wealth"));
        best = std::max(best, w);
    }
    return best;
}

void demo_leetcode_wealth() {
    std::cout << "[demo_leetcode_wealth]\n";
    std::vector<std::vector<int>> accounts = {
        {1, 2, 3},
        {3, 2, 1},
        {2, 8, 7},   // 總和 17 → 最大
    };
    std::cout << "  max wealth = " << maximumWealth(accounts) << "\n";
}

// ---------------------------------------------------------------------------
// 範例 4:日常工作實用範例 —— 通用屬性表 PropertyBag
//
// 情境:很多服務的「設定欄位」型別不同(int, double, string, bool),
//       常見做法是建一個 PropertyBag,介面統一,內部以 any 儲存。
//
// 這展示了 any 真正的用武之地:介面通用、型別卻可不同。
// ---------------------------------------------------------------------------
class PropertyBag {
public:
    template <class T>
    void set(const std::string& key, T value) { data_[key] = std::move(value); }

    template <class T>
    T get(const std::string& key, T fallback) const {
        auto it = data_.find(key);
        if (it == data_.end()) return fallback;
        if (auto p = std::any_cast<T>(&it->second)) return *p;
        return fallback;                          // 型別對不上就回退
    }
private:
    std::unordered_map<std::string, std::any> data_;
};

void demo_practical_property_bag() {
    std::cout << "[demo_practical_property_bag]\n";
    PropertyBag bag;
    bag.set<int>("port", 8080);
    bag.set<std::string>("host", "127.0.0.1");
    bag.set<bool>("debug", true);

    std::cout << "  host = " << bag.get<std::string>("host", "0.0.0.0") << "\n";
    std::cout << "  port = " << bag.get<int>("port", 80) << "\n";
    std::cout << "  debug= " << std::boolalpha
              << bag.get<bool>("debug", false) << "\n";
    // 沒設定的 key 就用 fallback
    std::cout << "  ttl  = " << bag.get<int>("ttl", 30) << "\n";
}

// ---------------------------------------------------------------------------
// 實用範例 (額外):event payload —— 通用事件系統
//
// 工作中常見:Event bus 中,每個 event 帶不同型別的 payload (int / string /
// 自訂 struct)。用 any 把 payload 通用化,handler 用 any_cast 取對應型別。
// ---------------------------------------------------------------------------
struct Event {
    std::string name;
    std::any payload;
};

void handle_event(const Event& e) {
    std::cout << "  event[" << e.name << "] ";
    if (auto p = std::any_cast<int>(&e.payload)) {
        std::cout << "(int) " << *p << "\n";
    } else if (auto p = std::any_cast<std::string>(&e.payload)) {
        std::cout << "(str) " << *p << "\n";
    } else {
        std::cout << "(unknown type)\n";
    }
}

void demo_practical_event_bus() {
    std::cout << "[demo_practical_event_bus]\n";
    std::vector<Event> queue;
    queue.push_back({"click",   42});
    queue.push_back({"input",   std::string("hello")});
    queue.push_back({"unknown", 3.14});                       // double 不在處理範圍
    for (const auto& e : queue) handle_event(e);
}

int main() {
    demo_basic();
    demo_bad_cast();
    demo_any_helpers();
    demo_leetcode_wealth();
    demo_practical_property_bag();
    demo_practical_event_bus();
    return 0;
}

/*
================================================================================
編譯與執行:
    g++ -std=c++17 -Wall -Wextra 05_any.cpp -o 05_any && ./05_any
================================================================================
*/
