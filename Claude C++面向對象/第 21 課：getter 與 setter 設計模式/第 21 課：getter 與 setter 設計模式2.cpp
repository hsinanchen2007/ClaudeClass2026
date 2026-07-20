// =============================================================================
//  第 21 課：getter 與 setter 設計模式 2  —  getter 的回傳方式與封裝強度
// =============================================================================
//
// 【主題資訊 Information】
//   語法：
//     T            getX() const;        // (1) 回傳值(拷貝)
//     const T&     getX() const;        // (2) 回傳 const 參考(唯讀視圖)
//     T&           getX();              // (3) 回傳非 const 參考(破壞封裝)
//   標準版本：本檔語法 C++98 即有;ref-qualifier(getX() &&)為 C++11。
//   複雜度：(1) 對基本型別 O(1);對 string/vector 是 O(n) 深拷貝。
//            (2)(3) 恆為 O(1),不複製任何資料。
//   標頭檔：<string>、<vector>
//
// 【詳細解釋 Explanation】
//
// 【1. getter 不是「把成員原封不動丟出去」,而是「決定外界能對它做什麼」】
//   很多人把 getter 當成樣板碼(boilerplate)——有幾個成員就寫幾個 getX()。
//   這是誤解。getter 真正的設計決策只有一個問題:
//       「我要給出去的是『資料的值』,還是『資料本身的控制權』?」
//   回傳值        = 給出快照,對方怎麼改都跟我無關。
//   回傳 const&   = 給出唯讀視窗,零複製,但視窗會隨我變動、也會隨我失效。
//   回傳非 const& = 把成員的鑰匙交出去,封裝正式宣告失敗。
//
// 【2. 為什麼基本型別回傳值,大型物件回傳 const&】
//   int/double 這種型別,拷貝等同一次暫存器搬移,比「回傳參考再解參考」還便宜
//   (參考在 ABI 上是指標,讀值要多一次記憶體間接)。
//   反過來,string/vector 的拷貝要走 heap 配置 + 逐元素複製,對 1000 筆的
//   vector 就是 1000 次元素複製。所以判準不是習慣,是
//   「拷貝成本 vs 一次指標間接的成本」。
//   經驗法則:sizeof(T) 不超過兩個指標(本機 x86-64 為 16 bytes)且可平凡複製
//   → 回傳值;否則回傳 const&。
//
// 【3. 回傳 const& 的隱藏契約:呼叫端不得比物件活得久】
//       const string& name = inv.getOwnerName();
//   這行沒有複製字串,name 只是 inv.ownerName_ 的別名。一旦 inv 被解構,
//   或 items_ 因 push_back 重新配置,先前取得的參考/迭代器就懸空了。
//   這是 const& getter 的代價:省下拷貝,換來生命週期責任。
//
// 【4. 為什麼「回傳非 const 參考」是設計層級的錯誤,不只是風格問題】
//   一旦 vector<string>& getItems() 存在,類別對 items_ 的所有不變式
//   (invariant)都不再成立——外界可以 clear()、可以塞入不合法的值。
//   此時 private 只是「語法上的 private」,語意上它已經是 public。
//   本檔方式 4 刻意以註解保留,示範它為何危險(下一支檔案會實際示範後果)。
//
// 【概念補充 Concept Deep Dive】
//   * 回傳 const& 在組譯層面就是回傳一個指標。呼叫端寫
//     `const string& n = get();` 不會產生任何建構子呼叫;而寫
//     `string n = get();` 會呼叫一次 copy constructor。
//   * 本機 x86-64 / libstdc++ / GCC 15.2 實測(皆為實作定義值,標準未規定):
//       sizeof(std::string)              = 32 bytes
//       sizeof(std::vector<std::string>) = 24 bytes
//       sizeof(void*)                    =  8 bytes
//     注意 vector 的 24 bytes 只是 header(三個指標);copy constructor 會
//     連帶深拷貝 heap 上的全部元素,這才是回傳值真正昂貴的地方。
//   * const 成員函式回傳 const&,兩個 const 意義不同:
//       const vector<string>& getItems() const;
//       ~~~~~ 回傳型別的 const                ~~~~~ 這個函式不修改 *this
//     前者保護「呼叫端不能改」,後者保護「我自己不會改」。兩者要一起用。
//
// 【注意事項 Pay Attention】
//   1. 把 const& 綁到「暫存物件的成員」會懸空:
//        const string& bad = makeInventory().getOwnerName();
//      暫存物件在該行結束即解構,之後讀 bad 是未定義行為(UB),
//      不保證任何特定結果——尤其不保證會崩潰。
//   2. 回傳值的 getter 不能只看「安全」就無腦用,對大容器是效能地雷。
//   3. getter 一律加 const,否則 const Inventory 物件連讀都讀不到。
//   4. 回傳非 const& 偶爾是刻意的(如 std::vector::operator[]),但那是
//      「容器本來就以元素存取為目的」;有不變式的業務類別不該這樣做。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】getter 的回傳方式
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. getter 什麼時候回傳值、什麼時候回傳 const 參考?判準是什麼?
//     答：判準是「拷貝成本 vs 一次指標間接的成本」,不是型別是否為 class。
//         基本型別(int/double/指標)拷貝就是一次暫存器搬移,回傳值最快,
//         而且沒有生命週期問題;string/vector 這類擁有 heap 資源的型別,
//         拷貝要配置記憶體並複製全部元素,應回傳 const&。
//     追問：那回傳 const& 有什麼代價?→ 呼叫端拿到的是別名,物件被解構或
//         容器重新配置後就懸空,等於把生命週期責任轉嫁給呼叫端。
//
// 🔥 Q2. 為什麼 vector<string>& getItems() 會「破壞封裝」?private 不是還在嗎?
//     答：private 限制的是「名字能不能被存取」,不是「資料能不能被改」。
//         一旦把非 const 參考回傳出去,外界就取得對該成員的完整寫入權,
//         可以繞過所有驗證與記錄邏輯。類別再也無法保證自己的不變式,
//         private 只剩語法意義。
//     追問：那 std::vector::operator[] 回傳非 const& 不也一樣?→ 不一樣。
//         vector 的目的就是暴露元素存取,元素值本來就沒有不變式;
//         業務類別的成員通常有(餘額不得為負、log 不得被清空)。
//
// ⚠️ 陷阱. 「回傳 const& 比較安全,所以全部都用 const& 就對了」——錯在哪?
//     答：const& 保護的是「不能改」,完全不保護「還活著」。
//         const string& n = inv.getOwnerName(); 之後 inv 若解構,
//         n 就是懸空參考,再讀取是 UB,不保證任何特定結果(不保證會崩潰,
//         很可能讀到看似正常的舊資料,反而更難除錯)。
//         而且對 int 這種小型別用 const& 通常還更慢。
//     為什麼會錯：腦中把 const& 當成「加了保險的值」,誤以為 const 涵蓋了
//         安全性的全部面向。實際上 const 管的是可變性(mutability),
//         生命週期(lifetime)是另一個完全獨立、且編譯器不會替你檢查的維度。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】—— 從缺,理由如下
//   「getter 回傳值 vs const&」是 API 設計與拷貝成本的取捨,LeetCode 題庫
//   不會針對回傳型別計分(判題只看行為與複雜度)。硬掛一題設計類題目
//   (如 155. Min Stack)並不會用到本檔的核心觀念,反而模糊焦點,故從缺。
//   封裝不變式與設計類題目的關聯,留待同課 3、4 號檔案示範。
//
// =============================================================================

#include <iostream>
#include <string>
#include <vector>
using namespace std;

class Inventory {
private:
    string ownerName_;
    vector<string> items_;
    int gold_;

public:
    Inventory(const string& owner, int gold)
        : ownerName_(owner), gold_(gold) {}

    void addItem(const string& item) {
        items_.push_back(item);
    }

    // ====== 返回方式比較 ======

    // 方式 1：返回值（拷貝）— 適合基本型別
    // 注意：對於基本型別，返回值會自動拷貝，不會影響原物件
    int getGold() const { return gold_; }

    // 方式 2：返回 const 引用 — 適合大型物件，避免拷貝
    // 注意：返回 const 引用可以讓外部「看」但不能「改」，保護封裝
    const string& getOwnerName() const { return ownerName_; }

    // 方式 3：返回 const 引用 — 讓外部可以「看」但不能「改」
    // 注意：對於容器等大型物件，返回 const 引用可以避免不必要的拷貝，同時保護封裝
    const vector<string>& getItems() const { return items_; }

    // 方式 4（錯誤示範）：返回非 const 引用 — 破壞封裝！
    // 注意：這樣做會讓外部直接修改內部狀態，繞過所有驗證邏輯，極易導致錯誤！
    // vector<string>& getItemsDangerous() { return items_; }
    // ↑ 外部可以直接修改 items_，繞過所有驗證！
};

// -----------------------------------------------------------------------------
// 【日常實務範例】應用程式設定檔:唯讀視圖 vs 快照
//   情境:服務啟動時載入 config,之後各模組都要讀「允許連線的主機白名單」。
//   * hosts() 回傳 const& —— 零複製,適合每次請求都要掃一遍的熱路徑。
//   * snapshotHosts() 回傳值 —— 給「要在背景執行緒慢慢跑、不希望被
//     reload() 中途改掉」的使用者;拿到的是快照,與後續 reload 無關。
//   這正是本檔的核心取捨在真實系統中的樣子:視圖便宜但綁生命週期,
//   快照貴但自給自足。
// -----------------------------------------------------------------------------
class AppConfig {
private:
    string          serviceName_;
    vector<string>  allowedHosts_;
    int             timeoutMs_;

public:
    AppConfig(const string& name, int timeoutMs)
        : serviceName_(name), timeoutMs_(timeoutMs) {}

    void allowHost(const string& h) { allowedHosts_.push_back(h); }

    // 小型別 → 回傳值
    int timeoutMs() const { return timeoutMs_; }
    // 大型別 → 回傳 const&(唯讀視圖,零複製)
    const string&         serviceName()  const { return serviceName_; }
    const vector<string>& hosts()        const { return allowedHosts_; }
    // 明確要快照時才付拷貝成本
    vector<string>        snapshotHosts() const { return allowedHosts_; }

    // 模擬熱重載:會讓先前由 hosts() 取得的參考失效
    void reload(const vector<string>& newHosts) { allowedHosts_ = newHosts; }
};

static bool isHostAllowed(const AppConfig& cfg, const string& host) {
    // 走 const& 視圖,整個函式沒有任何一次 vector 拷貝
    for (const auto& h : cfg.hosts()) {
        if (h == host) return true;
    }
    return false;
}

int main() {
    cout << "=== Getter 返回方式 ===" << endl;

    Inventory inv("勇者", 500);
    inv.addItem("鐵劍");
    inv.addItem("治療藥水");
    inv.addItem("火炬");

    // 方式 1：返回值（拷貝）
    int gold = inv.getGold();
    gold = 0;  // 修改的是拷貝，不影響原物件
    // 同時印出兩者，才看得出「拷貝被改、本體不動」這件事
    cout << "  本地拷貝 gold 被改成：" << gold << endl;
    cout << "  修改拷貝後，實際金幣：" << inv.getGold() << endl;

    // 方式 2：返回 const 引用
    const string& name = inv.getOwnerName();
    cout << "  擁有者：" << name << endl;
    // name = "壞人";  // 編譯錯誤！const 引用不能修改

    // 方式 3：返回 const 引用（容器）
    const vector<string>& items = inv.getItems();
    cout << "  物品數量：" << items.size() << endl;
    for (const auto& item : items) {
        cout << "    - " << item << endl;
    }
    // items.push_back("偷加的");  // 編譯錯誤！const 引用

    // ─────────────────────────────────────────────────────────
    cout << "\n=== 日常實務：設定檔的唯讀視圖 vs 快照 ===" << endl;
    AppConfig cfg("payment-api", 3000);
    cfg.allowHost("10.0.0.1");
    cfg.allowHost("10.0.0.2");

    cout << "  服務：" << cfg.serviceName()
         << "  timeout=" << cfg.timeoutMs() << "ms" << endl;
    cout << "  10.0.0.2 允許連線？" << (isHostAllowed(cfg, "10.0.0.2") ? "是" : "否") << endl;
    cout << "  10.9.9.9 允許連線？" << (isHostAllowed(cfg, "10.9.9.9") ? "是" : "否") << endl;

    // 先取快照，再熱重載：快照不受影響，這正是「回傳值」的價值
    vector<string> snap = cfg.snapshotHosts();
    cfg.reload({"192.168.1.1"});
    cout << "  reload 後目前白名單筆數：" << cfg.hosts().size() << endl;
    cout << "  先前取得的快照筆數：" << snap.size()
         << "（不受 reload 影響）" << endl;
    cout << "  快照第一筆：" << snap.front() << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 21 課：getter 與 setter 設計模式2.cpp" -o l21_2
// 執行: ./l21_2        (rc=0)

// === 預期輸出 ===
// === Getter 返回方式 ===
//   本地拷貝 gold 被改成：0
//   修改拷貝後，實際金幣：500
//   擁有者：勇者
//   物品數量：3
//     - 鐵劍
//     - 治療藥水
//     - 火炬
//
// === 日常實務：設定檔的唯讀視圖 vs 快照 ===
//   服務：payment-api  timeout=3000ms
//   10.0.0.2 允許連線？是
//   10.9.9.9 允許連線？否
//   reload 後目前白名單筆數：1
//   先前取得的快照筆數：2（不受 reload 影響）
//   快照第一筆：10.0.0.1
