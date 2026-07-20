// =============================================================================
//  第 11 課 -2  —  public：對外開放的介面，以及「開放欄位」與「開放操作」的差別
// =============================================================================
//
// 【主題資訊 Information】
//   語法：  class X { public: /* 成員與函式 */ };
//   標準：  C++98 起
//   標頭檔：本例僅需 <iostream>
//   關鍵詞：public interface、member function、encapsulation、const correctness
//
//   public 區段裡的東西，任何人（任何函式、任何檔案）都能碰。
//   本檔的重點不是「public 怎麼寫」，而是：
//     同樣掛在 public 底下，「開放一個欄位」和「開放一個操作」的後果完全不同。
//
// 【詳細解釋 Explanation】
//
// 【1. public 成員函式 = 這個型別的「動詞表」】
// 一個類別的 public 成員函式集合，就是它對外承諾「我會做這些事」。
// 以電燈為例，toggle() 表達的是一個有意義的操作：「切換狀態」。
// 呼叫端不需要知道燈的狀態是用 bool、int 還是 enum 存的，只需要知道
// 「呼叫 toggle 之後，燈的開關會反過來」。這就是抽象。
//
// 【2. public 資料成員 = 你把「內部表示法」也一起公開了】
// 本檔的 Light 把 isOn 也開成 public，於是外界可以：
//     lamp.isOn = false;      // 繞過 toggle()，直接改內部狀態
// 這行程式碼「能跑」，而且看起來人畜無害。但它造成兩個具體損失：
//
//   (a) 你失去了掛檢查／副作用的位置。
//       若哪天需求變成「開燈要記錄用電量」「切換要通知 App」，
//       toggle() 裡加得上，但 lamp.isOn = false; 這條路徑加不上。
//       結果是「有些狀態變更被記錄到，有些沒有」——這種 bug 極難查。
//
//   (b) 你失去了換掉內部表示法的自由。
//       若哪天要支援調光，狀態要從 bool isOn 變成 int brightness（0~100），
//       toggle() 可以改寫成 brightness = (brightness > 0 ? 0 : 100);
//       呼叫端零修改。但所有寫過 lamp.isOn 的地方全部編不過。
//
// 【3. 判準：這個欄位有沒有「不變量」或「未來會變的表示法」】
// 不是所有 public 資料成員都是壞的。真正的判準是：
//   * 有不變量要維護（餘額不可為負、size 必須等於實際元素數）→ 一定要 private。
//   * 表示法可能改變（今天 bool、明天 int）→ 應該 private，只開放操作。
//   * 純粹的資料聚合，沒有任何規則（如 struct Point { int x, y; }）→ public 完全合理。
// Light 屬於第二類：它今天沒有不變量，但 isOn 是「表示法」，遲早會變。
//
// 【4. const 正確性：show() 為什麼該是 const】
// show() 只讀不寫，理應宣告為 `void show() const`。這有兩個實際好處：
//   * 它讓編譯器幫你檢查「這個函式真的沒改到狀態」。
//   * 更重要：只有 const 成員函式能被 const 物件呼叫。
//     若 show() 不是 const，那麼 `const Light& ref = lamp; ref.show();` 會編譯失敗。
//   本檔保留了原始的非 const 版本（示範常見寫法），並額外提供 ImprovedLight
//   示範 const 正確的寫法，兩者可直接對照。
//
// 【概念補充 Concept Deep Dive】
// (A) 成員函式並不佔物件空間
//     非虛擬成員函式只有一份程式碼，所有物件共用；它靠隱含的 this 指標
//     知道自己在操作哪個物件。所以 Light 加再多成員函式，sizeof(Light) 都不變
//     ——它只由「非靜態資料成員 + 對齊 padding（+ 有虛擬函式時的 vptr）」決定。
//     本檔會實際印出 sizeof 佐證；該數值是實作定義的，不同編譯器／平台可能不同。
//
// (B) public 區段可以出現很多次，順序不影響存取權
//     class X { public: int a; private: int b; public: int c; };  完全合法。
//     不過同一 access 區段內的資料成員位址遞增順序與宣告順序一致，
//     跨區段之間的相對順序則由實作決定（實作定義）。
//     實務上仍建議每種 access 只寫一段，可讀性好很多。
//
// (C) 慣例：public 放前面
//     多數風格指南（Google C++ Style Guide 等）建議把 public 區段寫在最前面。
//     理由是讀者打開一個 header，最想先知道的是「我能拿它做什麼」，
//     而不是「它裡面藏了什麼」。這純粹是可讀性慣例，與語意無關。
//
// 【注意事項 Pay Attention】
// 1. public 成員函式是契約，改簽名／改語意都是 breaking change。
// 2. public 資料成員把「內部表示法」也變成契約，代價比多數人想的大。
// 3. 只讀的成員函式請加 const —— 否則 const 物件／const reference 無法呼叫它。
// 4. sizeof 只跟資料成員與對齊有關，跟成員函式數量無關（數值本身實作定義）。
// 5. 「能編譯」不代表「該這樣寫」：lamp.isOn = false; 合法但繞過了抽象。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】public 介面設計
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. public 成員函式和 public 資料成員，都對外開放，差別在哪？
//     答：public 成員函式開放的是「操作」，內部表示法仍然藏著，你隨時能換掉；
//         public 資料成員開放的是「表示法本身」，等於把實作細節寫進公開契約。
//         實務差別：前者能掛檢查／log／通知，後者的賦值路徑上你插不進任何程式碼。
//     追問：那 struct Point { int x, y; } 這種全 public 算不算壞設計？
//         → 不算。判準是「有沒有不變量」。Point 沒有任何規則要維護，
//           它就是純資料聚合，public 是正確選擇，硬包 getter 反而是過度設計。
//
// 🔥 Q2. 為什麼只讀的成員函式要加 const？不加會怎樣？
//     答：const 成員函式承諾不修改物件狀態，編譯器會檢查。
//         最實際的後果是：const 物件、const reference、const 指標
//         只能呼叫 const 成員函式。show() 不加 const，
//         `void print(const Light& l) { l.show(); }` 就編譯不過。
//     追問：const 成員函式裡真的完全不能改任何東西嗎？
//         → 可以改宣告為 mutable 的成員（常用於 cache、mutex 這類
//           「邏輯上不算狀態」的欄位）。
//
// ⚠️ 陷阱. 「我把成員設成 public，需要檢查時再改成 private 加 setter 就好」——問題在哪？
//     答：問題在於「再改」這件事的成本不是線性的。一旦 lamp.isOn 這種寫法散佈到
//         幾十處（可能包含別的團隊、已發布的函式庫使用者），把它改成 private
//         就是 breaking change，你會被迫維持相容而永遠改不掉。
//     為什麼會錯：多數人用「單檔案、我自己改」的心智模型估算重構成本，
//         以為「反正之後再改」。但介面一旦公開，成本就從「改一個 class」
//         變成「改所有呼叫端 + 協調所有使用者」。正確策略是預設 private，
//         確定不需要保護時才放寬 —— 放寬永遠比收緊容易。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
using namespace std;

// -----------------------------------------------------------------------------
// 原始版本：操作與欄位「都」是 public
//   toggle()/show() 是好的 public（開放操作）
//   isOn 是有疑慮的 public（開放內部表示法）
// -----------------------------------------------------------------------------
class Light {
public:
    bool isOn = false;

    void toggle() {
        isOn = !isOn;
    }

    void show() {
        cout << "  燈: " << (isOn ? "開" : "關") << endl;
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】改良版：智慧燈具的狀態機
//   情境：韌體要求
//     (a) 每次狀態變更都要累計切換次數（用於估算繼電器壽命）
//     (b) 之後可能要支援調光 → 內部表示法必須能換
//     (c) 唯讀查詢要能被 const reference 呼叫（給監控模組用）
//   做法：狀態 private，只開放 toggle()/turnOn()/turnOff() 三個操作。
//        注意 brightness 是 int 而非 bool —— 表示法換了，但介面沒變。
// -----------------------------------------------------------------------------
class ImprovedLight {
private:
    int m_brightness = 0;      // 0 = 關，1..100 = 亮度百分比
    int m_switchCount = 0;     // 繼電器切換次數（壽命估算用）

    void setBrightness(int b) {
        int old = m_brightness;
        m_brightness = b;
        if ((old == 0) != (b == 0)) {   // 只有「開↔關」才算一次切換
            ++m_switchCount;
        }
    }

public:
    void toggle()  { setBrightness(m_brightness > 0 ? 0 : 100); }
    void turnOn()  { setBrightness(100); }
    void turnOff() { setBrightness(0); }
    void dim(int percent) {                       // 新功能，介面自然擴充
        if (percent < 0)   percent = 0;
        if (percent > 100) percent = 100;
        setBrightness(percent);
    }

    // const 成員函式：可被 const 物件／const reference 呼叫
    bool isOn()         const { return m_brightness > 0; }
    int  brightness()   const { return m_brightness; }
    int  switchCount()  const { return m_switchCount; }

    void show() const {
        cout << "  智慧燈: " << (isOn() ? "開" : "關")
             << "（亮度 " << m_brightness << "%，累計切換 "
             << m_switchCount << " 次）" << endl;
    }
};

// 只接受 const reference 的監控函式：
// 這證明 show()/isOn() 必須是 const，否則此函式編譯不過。
void monitor(const ImprovedLight& lamp) {
    cout << "  [監控] 目前狀態 = " << (lamp.isOn() ? "ON" : "OFF")
         << "，切換次數 = " << lamp.switchCount() << endl;
}

int main() {
    cout << "=== 原始版 Light：public 操作 ===" << endl;
    Light lamp;
    lamp.show();        // 可以呼叫 public 函數
    lamp.toggle();      // 可以呼叫 public 函數 —— 走的是抽象介面
    lamp.show();

    cout << "\n=== 原始版 Light：public 欄位（繞過抽象） ===" << endl;
    lamp.isOn = false;  // 合法，但沒有經過 toggle()，任何副作用都不會發生
    lamp.show();
    cout << "  → 這次狀態變更沒有經過任何一行 Light 的程式碼。" << endl;

    cout << "\n=== 改良版 ImprovedLight：只開放操作 ===" << endl;
    ImprovedLight smart;
    smart.show();
    smart.toggle();     // 開
    smart.show();
    smart.toggle();     // 關
    smart.show();
    smart.turnOn();     // 再開
    smart.show();

    cout << "\n=== 表示法換成 int 之後，介面自然長出新功能 ===" << endl;
    smart.dim(30);      // 調光：bool 表示法做不到，但呼叫端介面沒被破壞
    smart.show();
    smart.dim(150);     // 超出範圍被夾住，這正是 private 的價值
    smart.show();

    cout << "\n=== const 正確性：const reference 只能呼叫 const 成員函式 ===" << endl;
    monitor(smart);

    cout << "\n=== sizeof：成員函式不佔物件空間（數值為實作定義） ===" << endl;
    cout << "  sizeof(Light)         = " << sizeof(Light) << " bytes（1 個 bool）" << endl;
    cout << "  sizeof(ImprovedLight) = " << sizeof(ImprovedLight)
         << " bytes（2 個 int，函式再多也不影響）" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 11 課：存取修飾符——public、private、protected2.cpp" -o access2

// === 預期輸出 ===
// === 原始版 Light：public 操作 ===
//   燈: 關
//   燈: 開
//
// === 原始版 Light：public 欄位（繞過抽象） ===
//   燈: 關
//   → 這次狀態變更沒有經過任何一行 Light 的程式碼。
//
// === 改良版 ImprovedLight：只開放操作 ===
//   智慧燈: 關（亮度 0%，累計切換 0 次）
//   智慧燈: 開（亮度 100%，累計切換 1 次）
//   智慧燈: 關（亮度 0%，累計切換 2 次）
//   智慧燈: 開（亮度 100%，累計切換 3 次）
//
// === 表示法換成 int 之後，介面自然長出新功能 ===
//   智慧燈: 開（亮度 30%，累計切換 3 次）
//   智慧燈: 開（亮度 100%，累計切換 3 次）
//
// === const 正確性：const reference 只能呼叫 const 成員函式 ===
//   [監控] 目前狀態 = ON，切換次數 = 3
//
// === sizeof：成員函式不佔物件空間（數值為實作定義） ===
//   sizeof(Light)         = 1 bytes（1 個 bool）
//   sizeof(ImprovedLight) = 8 bytes（2 個 int，函式再多也不影響）
