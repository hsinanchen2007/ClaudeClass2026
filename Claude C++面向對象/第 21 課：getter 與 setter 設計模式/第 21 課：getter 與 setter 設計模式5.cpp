// =============================================================================
//  第 21 課：getter 與 setter 設計模式 5  —  命名風格與同名重載
// =============================================================================
//
// 【主題資訊 Information】
//   三種主流風格:
//     風格 1(Java 風格)  int getHp() const;      void setHp(int);
//     風格 2(C++ 風格)   int hp() const;         void hp(int);      // 同名重載
//     風格 3(STL 風格)   size_t size() const;    // 名詞,通常無 setter
//   標準版本:重載解析(overload resolution)為 C++98 起的核心語言特性。
//   複雜度:命名風格不影響執行期成本,全部可被 inline 消除。
//   標頭檔:<string>
//
// 【詳細解釋 Explanation】
//
// 【1. 風格 2 為什麼「能」同名:重載靠參數列,不靠回傳型別】
//   int  hp() const;      // (a) 無參數
//   void hp(int newHp);   // (b) 有一個 int 參數
//   這兩者參數列不同,因此是合法的重載。編譯器在呼叫端看到 obj.hp() 就選
//   (a),看到 obj.hp(200) 就選 (b)。
//   反過來,「只有回傳型別不同」是不能重載的:
//       int  value();
//       long value();     // 編譯錯誤:重複宣告
//   原因是呼叫端可以忽略回傳值(單純寫 value(); 也合法),編譯器將無從選擇。
//   這是 C++ 重載規則裡最常被誤解的一條。
//
// 【2. const 也參與重載,而且非常重要】
//   風格 2 的 getter 有 const、setter 沒有,這不只是慣例:
//   const 成員函式的隱含 this 型別是 const T*,非 const 版本是 T*,
//   兩者的隱含參數不同,所以可以共存。實務上最常見的一組是:
//       const T& at(size_t i) const;   // const 物件呼叫這個
//       T&       at(size_t i);         // 非 const 物件呼叫這個
//   編譯器依「呼叫者本身是不是 const」挑選。這也是為什麼 getter 忘了加
//   const,會讓 const 物件連讀都讀不到。
//
// 【3. 三種風格的取捨(沒有絕對正確,只有一致性)】
//   風格 1  get/set 前綴
//     優點:一眼看出讀寫方向;與 Java/C# 生態一致;IDE 自動產生友善。
//     缺點:名稱冗長;對「本來就沒有 setter」的成員顯得囉嗦。
//   風格 2  同名重載
//     優點:呼叫端讀起來像屬性 obj.hp() / obj.hp(10);與 STL 風格接近。
//     缺點:寫 obj.hp(); 卻忘了接收回傳值時,不會有任何錯誤提示;
//           想用搜尋 "setHp" 這種明確字串來找出所有寫入點,也失效了。
//   風格 3  STL 名詞風格
//     優點:最簡潔;強調「這是物件的屬性」而非「一次操作」。
//     缺點:通常只適合唯讀查詢;STL 自己也是這樣用的(size/empty/front)。
//   真正重要的是:在同一個專案裡只挑一種,並貫徹到底。
//
// 【4. 為什麼 STL 沒有 setSize()】
//   風格 3 值得單獨一提:std::vector 有 size(),卻沒有 setSize()。
//   因為 size 是 push_back/erase/resize 的「結果」,不是「輸入」。
//   若允許直接設定 size,vector 內部的 begin/end/capacity 立刻對不上。
//   這正好呼應第 4 號檔案的主題:能用行為表達的,就不要用 setter。
//
// 【概念補充 Concept Deep Dive】
//   * 本檔所有 getter/setter 都定義在類別內部,因此隱含 inline。
//     開最佳化後(-O2)這些呼叫幾乎必然被完全展開,直接變成一次
//     記憶體讀寫,與直接存取 public 成員的機器碼相同。
//     也就是說,「封裝有沒有執行期成本」的答案通常是:沒有。
//   * 重載解析發生在編譯期,是純粹的名稱查找 + 參數比對,不產生任何
//     執行期分派(那是 virtual 才有的事)。
//   * 風格 2 有一個真實陷阱:若某天想再加上 hp(double),
//     obj.hp(1) 會因整數提升與轉換規則而變得曖昧或選到非預期的重載。
//     風格 1 另取名 setHpRatio() 反而不會有這個問題。
//
// 【注意事項 Pay Attention】
//   1. 「只有回傳型別不同」不能構成重載;必須參數列或 const 資格不同。
//   2. getter 一定要加 const,否則 const 物件無法呼叫。
//   3. 風格 2 之下,setter 沒有 const、getter 有 const,兩者才可共存;
//      若不小心把兩者寫成完全相同的簽章,會變成重複定義而編譯失敗。
//   4. 本檔 Style1/Style2 的成員未在建構時初始化,示範中一律「先呼叫
//      setter 寫入、再讀取」。讀取未初始化的物件成員是未定義行為(UB),
//      不保證任何特定結果——正式程式碼務必寫建構子或用預設成員初始化器。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】命名風格與重載規則
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. int hp() const; 與 void hp(int); 為什麼可以同名共存?
//     答：因為 C++ 的重載是依「參數列」區分的,兩者參數個數不同
//         (一個無參數、一個吃 int),屬於合法重載。編譯器在呼叫端
//         依實際引數做重載解析,整個過程發生在編譯期,沒有執行期成本。
//     追問：那 int f(); 與 long f(); 為什麼不行?→ 因為只有回傳型別不同。
//         呼叫端可以忽略回傳值(單寫 f();),編譯器將無從判斷該選哪個,
//         所以標準直接禁止。
//
// 🔥 Q2. const 成員函式為什麼可以和同名非 const 版本構成重載?
//     答：因為成員函式有一個隱含的 this 參數,const 版本的型別是
//         const T*,非 const 版本是 T*。隱含參數型別不同,自然構成重載。
//         編譯器依「呼叫者物件本身是否為 const」挑選對應版本。
//     追問：實務上這組重載最常見於哪?→ 容器的存取介面,例如
//         vector::operator[] 與 at():const 版本回傳 const T&(唯讀),
//         非 const 版本回傳 T&(可寫)。
//
// ⚠️ 陷阱. 「getter 反正不改東西,加不加 const 應該差不多吧?」
//     答：差很多。沒加 const 的 getter,對 const 物件是完全無法呼叫的——
//         包含以 const T& 傳進函式的參數。也就是說,一個忘了加 const 的
//         getter,會讓整條「以 const 參考傳遞」的呼叫鏈全部編譯失敗,
//         最後往往被錯誤地用 const_cast 硬解,把問題變得更糟。
//     為什麼會錯：把 const 當成「給人看的註解」,以為它只是宣告意圖。
//         實際上 const 是型別系統的一部分,直接參與重載解析與可呼叫性判定;
//         它不是文件,是會讓程式編不過的硬性規則。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】—— 從缺,理由如下
//   本檔主題是命名慣例與重載解析,屬於 API 風格議題。LeetCode 判題只看
//   輸入輸出與複雜度,對成員函式叫 getHp() 還是 hp() 毫無差別,
//   硬掛一題只會模糊焦點。設計類題目與封裝不變式的關聯,
//   已在同課 3(155. Min Stack)、4(1603. Design Parking System)示範。
//
// =============================================================================

#include <iostream>
#include <string>
#include <vector>     // 日常實務範例的 HttpHeaders 需要；明確引入，不倚賴間接引入
using namespace std;

// ===== 風格 1：get/set 前綴（Java 風格）=====
class Style1 {
private:
    int hp_;
    string name_;
public:
    int getHp() const { return hp_; }
    void setHp(int hp) { hp_ = hp; }
    const string& getName() const { return name_; }
    void setName(const string& name) { name_ = name; }
};

// ===== 風格 2：無前綴，同名函數重載（C++ 風格）=====
class Style2 {
private:
    int hp_;
    string name_;
public:
    // getter：無參數
    int hp() const { return hp_; }
    const string& name() const { return name_; }

    // setter：有參數
    void hp(int newHp) { hp_ = newHp; }
    void name(const string& newName) { name_ = newName; }
};

// ===== 風格 3：STL 風格（getter 用名詞，沒有 setter）=====
// STL 容器的做法：vector::size(), string::length()
// 通常不提供 setter，只提供行為函數

// -----------------------------------------------------------------------------
// 【日常實務範例】HTTP 標頭容器:const / 非 const 成對重載
//   情境:HTTP client 需要一個 headers 容器。讀取時常常拿到的是
//         const HttpHeaders&(例如 log 記錄、簽章計算),寫入時則是可變物件。
//   這裡示範最常見的一組 const 重載:
//       const string& operator[](k) const;  → const 物件用，唯讀
//       string&       operator[](k);        → 非 const 物件用，可寫
//   兩者簽章只差在 const 資格,卻是完全不同的兩個函式。
//   注意 dumpHeaders() 的參數是 const&,因此它「只可能」選到 const 版本——
//   這正是「getter 忘了加 const 會讓整條呼叫鏈壞掉」的實際場景。
// -----------------------------------------------------------------------------
class HttpHeaders {
private:
    // 用平行陣列而非 map，是為了讓示範不依賴額外標頭，且保持插入順序
    vector<string> keys_;
    vector<string> values_;
    string         empty_;   // 查無此鍵時回傳的空字串

    int indexOf(const string& k) const {
        for (size_t i = 0; i < keys_.size(); ++i) {
            if (keys_[i] == k) return static_cast<int>(i);
        }
        return -1;
    }

public:
    void set(const string& k, const string& v) {
        int i = indexOf(k);
        if (i >= 0) values_[static_cast<size_t>(i)] = v;
        else { keys_.push_back(k); values_.push_back(v); }
    }

    bool has(const string& k) const { return indexOf(k) >= 0; }
    size_t size() const { return keys_.size(); }          // 風格 3：STL 名詞風格

    // const 版本：唯讀，const 物件唯一能用的版本
    const string& operator[](const string& k) const {
        int i = indexOf(k);
        return i >= 0 ? values_[static_cast<size_t>(i)] : empty_;
    }

    // 非 const 版本：回傳可寫參考（不存在就先建立，語意同 std::map）
    string& operator[](const string& k) {
        int i = indexOf(k);
        if (i < 0) { keys_.push_back(k); values_.push_back(""); i = static_cast<int>(keys_.size()) - 1; }
        return values_[static_cast<size_t>(i)];
    }
};

// 參數是 const& → 內部所有存取都只會選到 const 版本的重載
static void dumpHeaders(const HttpHeaders& h) {
    cout << "    Content-Type: " << h["Content-Type"] << endl;
    cout << "    X-Request-Id: " << h["X-Request-Id"] << endl;
    cout << "    （查一個不存在的鍵）X-None: [" << h["X-None"] << "]" << endl;
    cout << "    標頭數量：" << h.size() << "（const 版本不會新增鍵）" << endl;
}

int main() {
    cout << "=== 命名風格比較 ===" << endl;

    // 風格 1 的使用
    cout << "\n--- 風格 1：get/set 前綴 ---" << endl;
    Style1 s1;
    s1.setHp(100);
    s1.setName("風格一");
    cout << "  " << s1.getName() << " HP:" << s1.getHp() << endl;

    // 風格 2 的使用
    cout << "\n--- 風格 2：同名重載 ---" << endl;
    Style2 s2;
    s2.hp(200);             // setter
    s2.name("風格二");       // setter
    cout << "  " << s2.name() << " HP:" << s2.hp() << endl;  // getter

    cout << "\n兩種風格都可以，關鍵是在項目中保持一致。" << endl;

    // ─────────────────────────────────────────────────────────
    cout << "\n=== 日常實務：HTTP 標頭（const / 非 const 成對重載）===" << endl;
    HttpHeaders h;
    // 非 const 物件 → 選到回傳 string& 的版本，可以直接寫入
    h["Content-Type"] = "application/json";
    h.set("X-Request-Id", "req-20260719-0001");
    cout << "  寫入後標頭數量：" << h.size() << endl;

    cout << "  以 const& 傳入函式（只可能選到 const 版本）：" << endl;
    dumpHeaders(h);

    // 非 const 版本查不到會「建立」新鍵，const 版本則不會 —— 兩者語意刻意不同
    h["X-None"];
    cout << "  用非 const 版本查同一個不存在的鍵之後，標頭數量：" << h.size()
         << "（非 const 版本會建立新鍵）" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 21 課：getter 與 setter 設計模式5.cpp" -o l21_5
// 執行: ./l21_5        (rc=0)

// === 預期輸出 ===
// === 命名風格比較 ===
//
// --- 風格 1：get/set 前綴 ---
//   風格一 HP:100
//
// --- 風格 2：同名重載 ---
//   風格二 HP:200
//
// 兩種風格都可以，關鍵是在項目中保持一致。
//
// === 日常實務：HTTP 標頭（const / 非 const 成對重載）===
//   寫入後標頭數量：2
//   以 const& 傳入函式（只可能選到 const 版本）：
//     Content-Type: application/json
//     X-Request-Id: req-20260719-0001
//     （查一個不存在的鍵）X-None: []
//     標頭數量：2（const 版本不會新增鍵）
//   用非 const 版本查同一個不存在的鍵之後，標頭數量：3（非 const 版本會建立新鍵）
