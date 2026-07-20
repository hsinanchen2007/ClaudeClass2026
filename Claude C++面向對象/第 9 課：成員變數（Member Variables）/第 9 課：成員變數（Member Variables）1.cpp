// =============================================================================
//  第 9 課：成員變數（Member Variables）1.cpp  —  用內建型別描述物件的狀態
// =============================================================================
//
// 【主題資訊 Information】
//   成員變數（非靜態資料成員）：定義物件「有什麼狀態」
//   四種基本內建型別：int / double / bool / char
//   本機大小（x86-64 / g++ 15.2.0，屬實作定義）：
//       int 4、double 8、bool 1、char 1
//   標準版本：C++98 起
//   複雜度：成員存取 O(1)（編譯期算好偏移量）
//   標頭檔：語言核心特性
//
// 【詳細解釋 Explanation】
//
// 【1. 成員變數是在描述「這個東西的狀態」】
//   設計類別時，成員變數要回答的問題是
//   【要描述一個感測器，最少需要記住哪些事？】
//   本檔的答案是：編號、溫度、是否啟用、等級。
//   選型別時的判準是【這個欄位的值域是什麼】：
//       id          → 整數，不會有小數        → int
//       temperature → 會有小數，需要精度      → double
//       isActive    → 只有是／否兩種狀態      → bool
//       grade       → 單一字元 'A'~'F'        → char
//   型別選對了，很多錯誤在編譯期就被擋掉了
//   （例如不可能把 "很燙" 指派給 temperature）。
//
// 【2. bool 為什麼佔 1 byte 而不是 1 bit】
//   bool 在概念上只需要 1 bit，但本機 sizeof(bool) 是 1 byte。
//   原因是 CPU 定址的最小單位就是 byte ——
//   沒有辦法取得「某個 bit 的位址」，
//   而 C++ 要求每個物件都必須有自己的位址。
//   若真的要壓縮成 bit，可以用 bit-field（int flag : 1;）
//   或 std::bitset；但那會犧牲存取速度（要做位移與遮罩），
//   通常只在資料量極大時才值得。
//
// 【3. char 是「一個 byte」而不是「一個字」】
//   grade 存 'A' 沒問題，因為 ASCII 字元剛好是 1 byte。
//   但 char 【無法】存一個中文字 —— UTF-8 下一個中文字要 3 個 bytes。
//   這是本課容易誤解的地方：char 的語意是「最小的可定址單位」，
//   歷史上剛好等同於「一個英文字元」，
//   在 Unicode 時代兩者已經不再等價。
//   要處理單一 Unicode 字元請用 char32_t（C++11）或整個 std::string。
//   另外 char 的正負號【屬實作定義】——
//   x86-64 Linux 上是 signed，某些 ARM 平台是 unsigned。
//   需要明確語意時請寫 signed char 或 unsigned char。
//
// 【4. 本檔的一個隱藏風險】
//   Sensor 的四個成員都【沒有類內初始值】。
//   本檔在 show() 之前把四個都設定好了，所以安全；
//   但只要漏掉任何一個，show() 印出的就是不確定的值（未定義行為）。
//   正確做法是給每個內建型別成員一個類內初始值（C++11）：
//       int id = 0;
//       double temperature = 0.0;
//       bool isActive = false;
//       char grade = 'F';
//   同課 5、6 號檔專門示範漏掉初始化的後果。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 本機實測的 sizeof(Sensor)
//   sizeof(Sensor) = 24（x86-64 / g++ 15.2.0，屬實作定義）：
//       int    id          偏移 0，佔 4
//       【填充 4 bytes】   ← double 需要 8 bytes 對齊
//       double temperature 偏移 8，佔 8
//       bool   isActive    偏移 16，佔 1
//       char   grade       偏移 17，佔 1
//       【尾端填充 6 bytes】← 整體須為 8 的倍數
//   四個成員的實際資料只有 14 bytes，卻佔了 24 ——
//   10 bytes 是填充。把 double 移到最前面
//   （double, int, bool, char）就變成 16 bytes，本機已實測確認。
//
// (B) 為什麼溫度要用 double 而不是 float
//   float 只有約 7 位十進位有效位數。
//   36.5 本身沒問題，但一旦做累加或比較，誤差會很快浮現。
//   而且 C++ 的浮點字面值預設就是 double，
//   用 float 反而會多一次轉換。
//   除非有明確的記憶體壓力（例如百萬筆的陣列），一般計算一律用 double。
//
// (C) show() 應該是 const 成員函式
//   show() 只讀不寫，應宣告為 void show() const。
//   不加的話，const Sensor& 就無法呼叫它 ——
//   而「傳 const 參考」正是最常見的參數傳遞方式。
//   const 正確性必須從一開始就做對，事後補會沿著呼叫鏈牽連一大片。
//
// 【注意事項 Pay Attention】
//   1. 本檔的成員【沒有類內初始值】。本檔在使用前都先賦值所以安全，
//      但漏掉任何一個就是未定義行為（見同課 5、6 號檔）。
//   2. sizeof(bool) 是 1 byte 而不是 1 bit —— CPU 的最小定址單位是 byte。
//   3. char 存不下一個中文字（UTF-8 下需要 3 bytes）；
//      char 的正負號屬實作定義。
//   4. sizeof(Sensor) 大於成員總和，差額是對齊填充；
//      成員宣告順序會影響物件大小。
//   5. show() 應宣告為 const 成員函式（本檔為簡化而省略）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】成員變數與內建型別
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. sizeof(bool) 是多少？為什麼不是 1 bit？
//     答：本機是 1 byte（標準只規定它至少能表示 true/false，
//         實際大小屬實作定義）。
//         不能是 1 bit 的根本原因是：CPU 定址的最小單位就是 byte，
//         沒有辦法取得「某個 bit 的位址」，
//         而 C++ 要求每個物件都必須有自己的位址（&flag 必須合法）。
//     追問：那 std::vector<bool> 呢？
//           → 它是標準庫中一個著名的【特化】，
//             內部真的用 bit 壓縮儲存。代價是它不滿足一般容器的要求：
//             operator[] 回傳的是代理物件而非 bool&，
//             所以 bool* p = &v[0]; 無法編譯。
//             這被公認為標準庫的設計失誤，需要真正的容器請用
//             std::vector<char> 或 std::deque<bool>。
//
// 🔥 Q2. char 可以用來存一個中文字嗎？
//     答：不行。char 的大小是 1 byte（標準定義 sizeof(char) == 1），
//         而一個中文字在 UTF-8 編碼下需要 3 bytes。
//         char 真正的語意是「最小的可定址單位」，
//         歷史上剛好等同於一個英文字元，在 Unicode 時代兩者已不再等價。
//         要存單一 Unicode 字元請用 char32_t（C++11），
//         處理文字則直接用 std::string（它是 byte 序列，size() 回傳的是
//         byte 數而非字元數）。
//     追問：char 是有號還是無號？
//           → 【實作定義】。x86-64 Linux 上是 signed，
//             某些 ARM 平台是 unsigned。所以 char、signed char、
//             unsigned char 是【三個不同的型別】。
//             需要明確語意時一定要寫全，
//             這也是 <cctype> 的函式（isdigit 等）要求先轉成
//             unsigned char 的原因 —— 傳入負的 char 是未定義行為。
//
// ⚠️ 陷阱. Sensor 有 int(4) + double(8) + bool(1) + char(1) = 14 bytes 的資料，
//          所以 sizeof(Sensor) 是 14 或 16？
//     答：本機實測是 24。因為：
//         id 在偏移 0 佔 4，接著要【填充 4 bytes】
//         讓 double 落在 8 的倍數位置；
//         temperature 偏移 8 佔 8；isActive 偏移 16、grade 偏移 17；
//         最後整體大小必須是最大對齊值（8）的倍數，
//         所以【尾端再填充 6 bytes】→ 總計 24。
//         14 bytes 的資料佔了 24 bytes，其中 10 bytes 是填充。
//     為什麼會錯：把 sizeof 當成成員大小的加總。
//         實際上還要考慮每個成員的【對齊需求】以及整體的尾端補齊。
//         實用的推論是：把成員【由大到小】宣告通常能減少填充 ——
//         本檔改成 double, int, bool, char 的順序後就變成 16 bytes，
//         省下三分之一，本機已實測確認。
//         在單一物件上這無關緊要，但當你有一百萬個物件時，
//         這就是 8 MB 的差距，而且直接影響快取命中率。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
using namespace std;

class Sensor {
public:
    int id;
    double temperature;
    bool isActive;
    char grade;

    void show() {
        cout << "ID: " << id << endl;
        cout << "溫度: " << temperature << " °C" << endl;
        cout << "啟用: " << (isActive ? "是" : "否") << endl;
        cout << "等級: " << grade << endl;
    }
};

int main() {
    Sensor s;
    s.id = 1001;
    s.temperature = 36.5;
    s.isActive = true;
    s.grade = 'A';

    s.show();

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 9 課：成員變數（Member Variables）1.cpp" -o member1

// 註 1:本檔輸出是【完全確定的】—— 沒有輸入、亂數、執行緒或位址。
//      四個成員在 show() 之前都已被賦值，因此沒有未初始化的問題。
//
// 註 2:「溫度: 36.5 °C」而非 36.500000，是 iostream 對 double 的預設格式
//      （6 位有效數字、去掉尾隨零）所致。
//
// 註 3:「等級: A」印出的是字元本身。若把 grade 宣告成 int 再存 65，
//      印出來就會是數字 65 —— 同樣的位元組，型別決定了如何呈現。
//
// 註 4:本機實測 sizeof(Sensor) = 24（x86-64 / g++ 15.2.0，屬實作定義），
//      但四個成員的實際資料只有 4+8+1+1 = 14 bytes，
//      另外 10 bytes 全是對齊填充。
//      把宣告順序改成 double, int, bool, char 之後變成 16 bytes ——
//      本機已實測確認。
//
// 註 5:⚠️ 本檔的成員都沒有類內初始值。本檔在使用前都先賦值所以安全，
//      但漏掉任何一個，show() 印出的就是不確定的值（未定義行為）。
//      同課 5、6 號檔專門示範這個後果，7 號檔則示範正確的類內初始值寫法。

// === 預期輸出 ===
// ID: 1001
// 溫度: 36.5 °C
// 啟用: 是
// 等級: A
