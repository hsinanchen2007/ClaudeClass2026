// =============================================================================
//  第 9 課：成員變數（Member Variables）2.cpp  —  以容器（std::vector）作為成員
// =============================================================================
//
// 【主題資訊 Information】
//   std::vector<T> 作為成員：動態大小的集合，自動管理堆積記憶體
//   std::string 作為成員   ：同樣自動管理，預設建構為空字串
//   兩者都有【預設建構子】，因此不寫類內初始值也是合法狀態
//   標準版本：C++98 起（range-for 為 C++11）
//   複雜度：push_back 均攤 O(1)；size() O(1)；索引存取 O(1)
//   標頭檔：<string>、<vector>
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼 string 與 vector 成員不需要類內初始值】
//   對照同課 1 號檔的 int / double —— 那些內建型別若不給初始值，
//   預設初始化【什麼都不做】，值不確定。
//   但 std::string 與 std::vector 是【類別型別】，它們有自己的預設建構子，
//   會把物件初始化成明確的空狀態（空字串、空容器）。
//   所以本檔的 Classroom room; 一建立就是完全合法的狀態，
//   students.size() 保證是 0，不是垃圾值。
//   這條規則值得記牢：
//   【內建型別成員一定要給初始值，類別型別成員可以依賴其預設建構子】。
//
// 【2. vector 成員讓物件的大小固定，但內容可變】
//   sizeof(Classroom) 是固定的（本機實測 88），
//   不管裡面裝了 3 個還是 3000 個學生。
//   因為 vector 物件本身只有三個指標（本機 24 bytes：起點、終點、容量上限），
//   實際的字串資料放在【堆積】上。
//   這是「物件在堆疊、資料在堆積」的標準模式 ——
//   使用者只看到一個自動生命週期的物件，
//   動態記憶體完全由 vector 內部封裝並保證在解構時釋放。
//
// 【3. addStudent 為什麼參數是 const string&】
//   傳值會複製整個字串（可能觸發堆積配置），
//   而 const string& 只傳一個位址。
//   push_back 內部才會做真正需要的那一次複製。
//   若想再進一步，C++11 可以提供右值多載或直接用 emplace_back：
//       void addStudent(std::string name) { students.push_back(std::move(name)); }
//   這樣呼叫端傳暫存物件時就是移動而非複製。
//
// 【4. 本檔有一個真實的編譯警告】
//       for (int i = 0; i < students.size(); i++)
//   students.size() 回傳的是 std::size_t（無號），而 i 是 int（有號），
//   兩者比較會觸發 -Wsign-compare 警告（-Wextra 已包含）。
//   這不只是風格問題：
//   若容器元素數量超過 INT_MAX，i 會溢位成負數而形成無窮迴圈。
//   三種正確寫法：
//       for (std::size_t i = 0; i < students.size(); ++i)      // 型別一致
//       for (const auto& s : students)                          // range-for，C++11
//       for (auto it = students.begin(); it != students.end(); ++it)
//   本檔保留原寫法是為了忠實呈現教材，
//   並讓這個警告成為可以討論的教學素材。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 本機實測的大小
//   sizeof(std::string) = 32、sizeof(std::vector<std::string>) = 24，
//   因此 sizeof(Classroom) = 32 + 32 + 24 = 88（皆屬實作定義）。
//   注意這 88 bytes 完全不含學生名字的實際內容 ——
//   那些在堆積上，而且 vector 成長時可能重新配置與搬移。
//
// (B) vector 的成長策略
//   libstdc++ 的 vector 在容量不足時以【2 倍】擴充（此倍率屬實作定義，
//   MSVC 用 1.5 倍）。所以連續 push_back n 次的總成本是【均攤 O(1)】——
//   雖然個別某次會觸發重新配置與搬移。
//   若事先知道數量，呼叫 reserve() 可以完全避免中途的重新配置。
//
// (C) 成員的建構與解構順序
//   Classroom 的三個成員依【宣告順序】建構：
//   teacherName → roomNumber → students；解構則是嚴格反序。
//   這也意味著 students 的解構子（釋放堆積記憶體）是
//   由編譯器產生的 ~Classroom() 自動呼叫的 ——
//   本檔沒有寫任何解構子，卻不會洩漏任何記憶體。
//   這正是 RAII 的核心：資源的釋放綁定在物件的生命週期上。
//
// 【注意事項 Pay Attention】
//   1. string 與 vector 成員【不需要】類內初始值（它們有預設建構子）；
//      但內建型別成員一定要給。
//   2. 本檔的 int i < students.size() 會觸發 -Wsign-compare 警告，
//      這是刻意保留的教材內容 —— 正確寫法是用 std::size_t 或 range-for。
//   3. sizeof(Classroom) 是固定的，與裝了多少學生無關。
//   4. 本檔沒有寫解構子，但 vector 與 string 的資源都會被正確釋放
//      （編譯器產生的解構子會呼叫各成員的解構子）。
//   5. show() 應宣告為 const 成員函式（本檔為簡化而省略）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】容器作為成員變數
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 string 和 vector 成員不寫初始值也是安全的，
//        但 int 成員不寫就會有問題？
//     答：因為 string 與 vector 是【類別型別】，有自己的預設建構子，
//         預設初始化時會被建構成明確的空狀態（空字串、空容器）。
//         而 int 是內建型別，預設初始化【什麼都不做】，
//         值是不確定的，讀取它是未定義行為。
//         實用規則：內建型別成員一律給類內初始值，
//         類別型別成員可以依賴其預設建構子。
//     追問：那 int* ptr 成員呢？
//           → 指標也是內建型別，不給初始值就是【野指標】，
//             值不確定且無法檢查（if (ptr) 是沒有意義的，
//             垃圾值很可能非零）。一定要寫 int* ptr = nullptr;。
//
// 🔥 Q2. Classroom 裝了 3 個學生和 3000 個學生，sizeof 會不同嗎？
//     答：完全相同（本機實測 88 bytes）。
//         因為 vector 物件本身只有三個指標（起點、終點、容量上限，
//         本機共 24 bytes），實際的字串資料在【堆積】上。
//         sizeof 是編譯期常數，不可能反映執行期的內容量。
//     追問：那怎麼知道實際用了多少記憶體？
//           → 要自己計算：vector 的 capacity() × 元素大小，
//             再加上每個 string 若超過 SSO 門檻（libstdc++ 為 15 字元）
//             另外配置的堆積空間。這正是為什麼記憶體分析
//             必須用專門的工具（valgrind massif、heaptrack）而不能靠 sizeof。
//
// ⚠️ 陷阱. for (int i = 0; i < students.size(); i++) 這行有什麼問題？
//          反正學生不會有幾十億個，應該無所謂吧？
//     答：它會觸發 -Wsign-compare 警告（有號 int 與無號 size_t 比較）。
//         「數量不會那麼大」在這個例子確實成立，
//         但這個習慣有一個更常見、也更致命的變形：
//             for (int i = 0; i < v.size() - 1; ++i)
//         當 v 是【空的】時候，v.size() 是無號的 0，
//         0 - 1 不是 -1 而是 SIZE_MAX（本機為 18446744073709551615），
//         迴圈於是跑了天文數字次並立刻越界存取。
//     為什麼會錯：把 size() 想成一個普通的整數。
//         它是【無號】型別，無號數的減法永遠不會產生負數，
//         而是回繞（wrap around）成一個巨大的值。
//         這也是為什麼 Google 與 LLVM 的風格指南
//         都傾向使用有號索引，而 C++ 標準庫因為歷史包袱只能用無號。
//         最安全的做法是完全不要手寫索引 ——
//         用 range-for（for (const auto& s : students)）或標準演算法，
//         連比較都不會發生。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <vector>
using namespace std;

class Classroom {
public:
    string teacherName;
    string roomNumber;
    vector<string> students;  // 動態陣列作為成員

    void addStudent(const string& name) {
        students.push_back(name);
    }

    void show() {
        cout << "教室: " << roomNumber << endl;
        cout << "老師: " << teacherName << endl;
        cout << "學生 (" << students.size() << " 人):" << endl;
        for (int i = 0; i < students.size(); i++) {
            cout << "  " << (i + 1) << ". " << students[i] << endl;
        }
    }
};

int main() {
    Classroom room;
    room.teacherName = "王老師";
    room.roomNumber = "A-301";

    room.addStudent("小明");
    room.addStudent("小華");
    room.addStudent("小美");

    room.show();

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 9 課：成員變數（Member Variables）2.cpp" -o member2
//   注意：編譯時會出現 1 個 -Wsign-compare 警告（第 20 行的
//   int i < students.size()，有號與無號比較）。
//   這是刻意保留的教材內容，也是下方【面試題】討論的對象；
//   正確寫法是改用 std::size_t 或 range-for。

// 註 1:本檔輸出是【完全確定的】—— 沒有輸入、亂數、執行緒或位址。
//
// 註 2:Classroom room; 一建立就是完全合法的狀態 ——
//      teacherName / roomNumber 是空字串、students 是空容器。
//      因為 string 與 vector 都有自己的預設建構子，
//      不像同課 1 號檔的 int / double 會留下不確定的值。
//
// 註 3:本機實測 sizeof(Classroom) = 88（string 32 + string 32 + vector 24，
//      皆屬實作定義）。這個數字【與裝了多少學生無關】——
//      學生名字的實際資料在堆積上，vector 物件本身只有三個指標。
//
// 註 4:本檔沒有寫任何解構子，卻不會洩漏記憶體 ——
//      編譯器產生的 ~Classroom() 會依【宣告的反序】
//      呼叫 students、roomNumber、teacherName 各自的解構子。
//      這就是 RAII：資源釋放綁定在物件生命週期上。

// === 預期輸出 ===
// 教室: A-301
// 老師: 王老師
// 學生 (3 人):
//   1. 小明
//   2. 小華
//   3. 小美
