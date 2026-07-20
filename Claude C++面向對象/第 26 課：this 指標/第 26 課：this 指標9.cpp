// =============================================================================
//  第 26 課：this 指標9.cpp  —  誤區二：回傳指向區域物件的指標（懸空指標）
// =============================================================================
//
// 【主題資訊 Information】
//   議題：函式回傳 &local 會產生懸空指標（dangling pointer）；
//         正確做法是回傳值（value）、smart pointer，或由呼叫端提供儲存空間。
//   標準：自動儲存期物件在其 scope 結束時解構（[basic.stc.auto]）；
//         **C++17 起** 以 prvalue 回傳為「保證省略複製（guaranteed copy elision）」，
//         這不是最佳化，而是語言規則：根本不存在需要被省略的暫存物件。
//         NRVO（具名回傳值最佳化）則自 C++98 起即被**允許但不強制**。
//   標頭檔：<memory>（unique_ptr）
//   複雜度：回傳值在 C++17 下為 O(1) 且零次複製／移動（見本檔實測輸出）
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼 return &local 是災難】
//       Trap* createBad() {
//           Trap local(99);
//           return &local;      // local 在 } 的瞬間解構，位址立刻失效
//       }
//   函式返回後，區域物件所在的堆疊框（stack frame）會被回收。
//   呼叫端拿到的指標指向一塊「隨時會被下一個函式呼叫覆寫」的記憶體。
//   之後解參考它是**未定義行為**——不保證任何固定結果。
//
//   最惡毒的地方在於它**通常不會立刻壞掉**：剛返回時那塊記憶體多半還沒被覆寫，
//   所以測試時「看起來是對的」。等到中間多插一個函式呼叫、換個最佳化等級、
//   或換一台機器，值才開始變成垃圾。這種缺陷會存活到正式環境。
//
//   本檔**刻意不執行**這條路徑（createBad 保持註解狀態）：
//   未定義行為不能拿來當示範，也不能把它的輸出寫進「預期輸出」。
//   g++ 對這種寫法會發出 -Wreturn-local-addr 警告，見檔尾說明。
//
// 【2. 三種正確做法的取捨】
//   (a) 回傳動態配置的裸指標   static Trap* createGood() { return new Trap(99); }
//       可行，但**把釋放責任丟給呼叫端**。呼叫端一旦忘記 delete 就洩漏；
//       中途丟出例外也會洩漏。這是 C++98 的作法，現代 C++ 不建議當預設。
//   (b) 回傳值                 static Trap createBest() { return Trap(99); }
//       最推薦。生命週期由語言管理，沒有任何洩漏可能。
//       在 C++17 下**零次複製、零次移動**（見下一點）。
//   (c) 回傳 std::unique_ptr   static std::unique_ptr<Trap> createOwned();
//       需要多型（回傳基底指標）或物件不可複製／過大時用它。
//       所有權在型別上就寫清楚了，呼叫端無從忘記釋放。
//
// 【3. 「回傳值很慢」是 C++98 時代的過時直覺】
//   常見誤解：回傳值會多一次複製，所以回傳指標比較快。在 C++17 之後這完全錯誤。
//     * **回傳 prvalue**（return Trap(99);）：C++17 起這是**語言保證**的行為——
//       Trap(99) 直接就在呼叫端的目標記憶體上建構，從頭到尾只有一個物件，
//       沒有暫存物件、沒有複製也沒有移動。連 -fno-elide-constructors 都改變不了它，
//       因為根本沒有東西可以「不省略」。
//     * **回傳具名區域變數**（NRVO，return local;）：這是**允許但不強制**的最佳化。
//       本機 g++ 15.2 即使在 -O0 也套用了 NRVO（0 次移動）；
//       但加上 -fno-elide-constructors 後就會看到 1 次移動建構。
//       兩者都合法，這正是「允許但不強制」的意思。
//   本檔會用計數器把上述差異實際印出來，數字完全決定性。
//
// 【4. 什麼情況回傳指標／參考才是對的】
//     * 回傳指向**成員**的指標／參考（生命週期由物件擁有，不是區域變數）：
//         const std::string& name() const { return name_; }   // 合法
//       但呼叫端不得讓它活得比物件久——這是 std::string_view 最常見的陷阱來源。
//     * 回傳指向 static 儲存期物件的指標（生命週期跨整個程式）。
//     * 回傳指向呼叫端傳進來的緩衝區的指標（所有權在呼叫端）。
//   共同判準只有一句：**被指物的生命週期必須長於指標的使用期間。**
//
// 【概念補充 Concept Deep Dive】
//   * 「回傳值」在 ABI 層面怎麼做到零複製？x86-64 System V ABI 規定：
//     若回傳型別不是簡單的暫存器型別，呼叫端會先配置好目標空間，
//     把它的位址當成**隱藏的第一個參數**（RVO slot）傳給被呼叫函式；
//     函式直接在那塊空間上建構結果。所以「複製」從一開始就不存在。
//     這與 this 的傳遞方式是同一套機制——都是隱藏參數。
//   * C++17 之前，return Trap(99); 在概念上是「建構暫存物件 → 複製／移動到回傳槽」，
//     編譯器**被允許**省略；因此當年若把複製建構函數設為 private/deleted，
//     即使實際被省略了，程式仍不能編譯（因為語意上需要可存取的複製建構函數）。
//     C++17 改成「語意上就沒有那次複製」，所以連不可複製、不可移動的型別
//     都能以 prvalue 回傳——這是 std::mutex、std::atomic 之類型別能被工廠函式
//     回傳的關鍵改變。
//   * 回傳 std::move(local) 是**反最佳化**：它把 prvalue／NRVO 候選變成 xvalue，
//     直接關閉 NRVO，強迫多一次移動。g++ 會以 -Wpessimizing-move 警告。
//
// 【注意事項 Pay Attention】
//   1. 解參考懸空指標是未定義行為，**不可**描述成「一定印出垃圾」或「一定崩潰」。
//      實際上它最常見的表現反而是「看起來正常」。
//   2. 回傳 prvalue 的零複製是 **C++17 的語言保證**；
//      NRVO 是**允許但不強制**的最佳化——兩者不可混為一談。
//   3. new 出來的物件必須有人 delete。若要回傳所有權，請用 std::unique_ptr，
//      不要回傳裸指標。
//   4. 回傳成員的參考本身合法，但呼叫端把它存起來活得比物件久，
//      就退化成同一種懸空問題（string_view / span 尤其容易踩到）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】回傳區域物件與回傳值最佳化
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 回傳 &local 會怎樣？為什麼測試時常常「看起來是對的」？
//     答：區域物件在函式返回時就已解構、堆疊框被回收，呼叫端拿到懸空指標，
//         解參考它是未定義行為。之所以常常看起來正常，是因為那塊堆疊記憶體
//         在下一次函式呼叫覆寫它之前，內容還沒被動過。
//         換個最佳化等級、多插一層呼叫，值就開始變垃圾。
//     追問：那回傳成員的參考（const string& name() const）為什麼可以？
//         → 因為被指物的生命週期屬於物件，不是區域變數。判準永遠是
//           「被指物活得比指標的使用期間久」。
//
// 🔥 Q2. return Trap(99); 會不會多一次複製？
//     答：C++17 起保證零次複製、零次移動。這不是最佳化而是語言規則：
//         prvalue 直接在呼叫端的回傳槽上建構，語意上就不存在暫存物件。
//         本檔用複製／移動計數器實測，結果是 copies=0 moves=0，
//         而且加上 -fno-elide-constructors 也不變。
//     追問：那 return local;（具名變數）呢？
//         → 那是 NRVO，**允許但不強制**。本機 g++ 15.2 在 -O0 也省略了；
//           但加 -fno-elide-constructors 後會看到 1 次移動建構。
//
// ⚠️ 陷阱1. 「為了效率，回傳大物件時應該回傳指標而不是回傳值。」
//     答：這是 C++98 時代的過時直覺。C++17 的保證省略讓回傳 prvalue
//         連一次移動都沒有，而回傳 new 出來的指標反而多了一次堆積配置、
//         一次間接存取，還把釋放責任丟給呼叫端。回傳值才是預設答案。
//     為什麼會錯：腦中還停留在「回傳值 = 複製整個物件」的模型，
//         沒有意識到 ABI 層面早就用隱藏的回傳槽參數避開了複製。
//
// ⚠️ 陷阱2. 「回傳時寫 return std::move(local); 可以幫編譯器省一次複製。」
//     答：正好相反，這會**關閉** NRVO。std::move 把具名變數變成 xvalue，
//         編譯器就不能再把 local 直接建構在回傳槽上，反而多出一次移動。
//         g++ 會以 -Wpessimizing-move 警告。正確寫法就是 return local;。
//     為什麼會錯：把 std::move 當成「加速咒語」，
//         但它只是型別轉換，不會產生任何最佳化，只會限制編譯器的選擇。
//
// ⚠️ 陷阱3. 「createGood() 回傳 new 出來的指標，有 delete 就沒問題了。」
//     答：能動，但介面把所有權寫在文件裡而不是型別裡。呼叫端忘記 delete 會洩漏；
//         取得指標到 delete 之間丟出例外也會洩漏。改成回傳 std::unique_ptr
//         後，所有權由型別表達，且例外安全免費取得。
//     為什麼會錯：把「記得寫 delete」當成紀律問題。
//         現代 C++ 的作法是讓型別系統替你記住，人不需要記。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <memory>
#include <string>
#include <vector>
using namespace std;

class Trap {
private:
    int value_;

public:
    // 複製／移動計數器：用來實測 C++17 的保證省略與 NRVO
    static int copies;
    static int moves;

    Trap(int v) : value_(v) {}
    Trap(const Trap& o) : value_(o.value_) { ++copies; }
    Trap(Trap&& o) noexcept : value_(o.value_) { ++moves; }

    // ❌ 危險！返回指向局部對象的指標
    //    本檔刻意保持註解狀態：解參考它是未定義行為，不能拿來當示範，
    //    也不能把它的輸出寫進「預期輸出」。
    //    若把它解開，g++ -Wall 會發出 -Wreturn-local-addr 警告。
    // static Trap* createBad() {
    //     Trap local(99);
    //     return &local;   // local 離開作用域就死了！
    // }

    // ✅ 安全：返回動態分配的對象（但把 delete 的責任丟給呼叫端）
    static Trap* createGood() {
        return new Trap(99);   // 堆上的對象不會自動銷毀
    }

    // ✅ 安全：返回值（C++17 起保證零次複製、零次移動）
    static Trap createBest() {
        return Trap(99);       // prvalue：直接建構在呼叫端的回傳槽上
    }

    // ✅ 安全：返回具名區域變數 → NRVO（允許但不強制的最佳化）
    static Trap createNamed() {
        Trap local(88);
        return local;          // 不要寫 return std::move(local); 那會關掉 NRVO
    }

    // ✅ 最推薦：所有權寫在型別上，呼叫端無從忘記釋放
    static std::unique_ptr<Trap> createOwned() {
        return std::make_unique<Trap>(77);
    }

    int getValue() const { return value_; }

    static void resetCounters() { copies = 0; moves = 0; }
};

int Trap::copies = 0;
int Trap::moves  = 0;

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】—— 從缺，並說明原因
//
// 本檔主題是「物件生命週期、懸空指標與回傳值最佳化」。LeetCode 的判題模式是
// 「呼叫你的函式，比對回傳值」，而且題目簽章早就規定好要回傳 int / vector /
// string 這類值型別，本來就不會誘導你回傳區域物件的位址。
// 指定清單中的 design 類題（146 LRU Cache、707 Design Linked List…）雖然會
// new 節點，但它們考的是資料結構操作與複雜度，不會檢查你有沒有洩漏，
// 也不會因為你回傳裸指標而扣分。把它們搬進來只會模糊本檔的焦點。
// 依規格「寧缺勿濫」，此處明確從缺。
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 【日常實務範例】設定檔載入器 —— 三種回傳所有權的介面設計比較
//
// 情境：服務啟動時要從設定檔建立 Codec 物件。Codec 是多型的（JSON / YAML），
//       所以不能單純回傳值，必須回傳基底型別的指標。
//       這正是實務上唯一真正需要「回傳指標」的情境。
//
// 三種介面的差別（同樣的功能，責任歸屬完全不同）：
//   makeCodecRaw()   → 回傳 Codec*        ：能用，但呼叫端必須記得 delete
//   makeCodecOwned() → 回傳 unique_ptr<Codec>：所有權寫在型別上，例外安全免費
//   parseVersion()   → 回傳 std::string   ：非多型的情況，直接回傳值就好
// -----------------------------------------------------------------------------
class Codec {
public:
    virtual ~Codec() = default;
    virtual string decode(const string& raw) const = 0;
    virtual string name() const = 0;
};

class JsonCodec : public Codec {
public:
    string decode(const string& raw) const override { return "{parsed:" + raw + "}"; }
    string name() const override { return "json"; }
};

class YamlCodec : public Codec {
public:
    string decode(const string& raw) const override { return "---\n" + raw; }
    string name() const override { return "yaml"; }
};

// 舊式介面：所有權只寫在文件裡，型別看不出來
Codec* makeCodecRaw(const string& fmt) {
    if (fmt == "yaml") return new YamlCodec();
    return new JsonCodec();
}

// 現代介面：所有權寫在型別上，呼叫端無從忘記釋放，中途丟例外也不洩漏
std::unique_ptr<Codec> makeCodecOwned(const string& fmt) {
    if (fmt == "yaml") return std::make_unique<YamlCodec>();
    return std::make_unique<JsonCodec>();
}

// 非多型的情況：直接回傳值，C++17 下沒有任何複製成本
string parseVersion(const string& configLine) {
    auto pos = configLine.find('=');
    if (pos == string::npos) return "unknown";
    return configLine.substr(pos + 1);      // 具名回傳槽由編譯器處理
}

int main() {
    cout << "=== 誤區二：安全的創建方式 ===" << endl;

    Trap* p = Trap::createGood();
    cout << "  動態創建：" << p->getValue() << endl;
    delete p;

    Trap t = Trap::createBest();
    cout << "  值返回：" << t.getValue() << endl;

    cout << "\n=== 實測：回傳 prvalue 到底複製幾次 ===" << endl;
    Trap::resetCounters();
    Trap a = Trap::createBest();          // return Trap(99);  → prvalue
    cout << "  createBest()（prvalue）→ 複製 " << Trap::copies
         << " 次、移動 " << Trap::moves << " 次，值=" << a.getValue() << endl;
    cout << "  這是 C++17 的語言保證，不是最佳化（加 -fno-elide-constructors 也一樣）" << endl;

    Trap::resetCounters();
    Trap b = Trap::createNamed();         // return local;     → NRVO
    cout << "  createNamed()（NRVO）→ 複製 " << Trap::copies
         << " 次、移動 " << Trap::moves << " 次，值=" << b.getValue() << endl;
    cout << "  NRVO 是允許但不強制的最佳化；本機 g++ 15.2 在 -O0 也套用了" << endl;

    cout << "\n=== 所有權寫在型別上 ===" << endl;
    Trap::resetCounters();
    auto owned = Trap::createOwned();
    cout << "  createOwned()（unique_ptr）值=" << owned->getValue()
         << "，離開 scope 自動釋放，無需 delete" << endl;

    cout << "\n=== 日常實務：設定檔載入器的三種介面 ===" << endl;
    Codec* rawCodec = makeCodecRaw("json");          // 舊式：必須自己 delete
    cout << "  [裸指標]  " << rawCodec->name() << " → "
         << rawCodec->decode("a:1") << endl;
    delete rawCodec;                                  // 忘了這行就是洩漏

    auto ownedCodec = makeCodecOwned("yaml");         // 現代：不需要 delete
    cout << "  [unique_ptr] " << ownedCodec->name() << " → "
         << ownedCodec->decode("a: 1") << endl;

    cout << "  [回傳值] version = " << parseVersion("version=2.4.1") << endl;
    cout << "  [回傳值] version = " << parseVersion("no-equal-sign") << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 26 課：this 指標9.cpp" -o this9
// 進階驗證（觀察 NRVO 被關閉時的差異）：
//   g++ -std=c++17 -Wall -Wextra -fno-elide-constructors "第 26 課：this 指標9.cpp" -o this9_noelide

// 注意事項（輸出相關）：
//   * createBad() 保持註解狀態，本檔**完全不執行**懸空指標路徑。
//     解參考懸空指標是未定義行為，不保證任何固定結果，
//     既不能示範也不能寫進預期輸出。
//   * 「複製 0 次、移動 0 次」對 createBest()（prvalue）是 **C++17 的語言保證**，
//     在本機以 -fno-elide-constructors 重新編譯後實測仍為 0/0。
//   * createNamed() 的「移動 0 次」是 **NRVO 生效**的結果，而 NRVO 是
//     **允許但不強制**的最佳化。本機 g++ 15.2 在 -O0 即套用；
//     以 -fno-elide-constructors 重新編譯後，該行會變成「移動 1 次」。
//     這一行是本檔唯一會隨編譯選項改變的輸出，其餘皆為決定性。
//   * 本檔不列印任何指標位址（ASLR 使其每次執行都不同）。
//   * 各 Codec 的輸出含一個換行（yaml 的 "---\n"），因此該行會折成兩行顯示。

// === 預期輸出 ===
// === 誤區二：安全的創建方式 ===
//   動態創建：99
//   值返回：99
//
// === 實測：回傳 prvalue 到底複製幾次 ===
//   createBest()（prvalue）→ 複製 0 次、移動 0 次，值=99
//   這是 C++17 的語言保證，不是最佳化（加 -fno-elide-constructors 也一樣）
//   createNamed()（NRVO）→ 複製 0 次、移動 0 次，值=88
//   NRVO 是允許但不強制的最佳化；本機 g++ 15.2 在 -O0 也套用了
//
// === 所有權寫在型別上 ===
//   createOwned()（unique_ptr）值=77，離開 scope 自動釋放，無需 delete
//
// === 日常實務：設定檔載入器的三種介面 ===
//   [裸指標]  json → {parsed:a:1}
//   [unique_ptr] yaml → ---
// a: 1
//   [回傳值] version = 2.4.1
//   [回傳值] version = unknown
