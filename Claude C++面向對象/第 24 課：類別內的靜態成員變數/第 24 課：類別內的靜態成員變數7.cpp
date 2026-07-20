// =============================================================================
//  第 24 課：類別內的靜態成員變數 7  —  constexpr static：編譯期常數
// =============================================================================
//
// 【主題資訊 Information】
//   語法:  class C { static constexpr T NAME = <常數運算式>; };
//   標準版本:
//     * static constexpr 資料成員：C++11
//     * C++17 起 constexpr static 資料成員隱含 inline，
//       因此不再需要在類別外另寫定義
//   複雜度: 編譯期求值，執行期成本為 0
//   標頭檔: 本檔僅需 <iostream>
//
// 【詳細解釋 Explanation】
//
// 【1. const 與 constexpr 的差別】
//   const 只保證「不可修改」，值不一定在編譯期就知道：
//       const int n = readFromFile();   // 合法，執行期才有值
//   constexpr 保證「編譯期就能求出值」，因此可以用在需要常數運算式的地方
//   （陣列大小、template 引數、case 標籤、static_assert）。
//   本檔的 MAX_DIMENSION 拿來當陣列大小，正是 constexpr 才辦得到。
//
// 【2. 為什麼 TWO_PI = PI * 2.0 不花執行期成本】
//   PI 是編譯期常數，PI * 2.0 因此也是常數運算式，
//   編譯器在編譯階段就把結果算好、直接嵌進機器碼。
//   執行期不會有任何乘法指令 —— 這與「執行期算一次再快取」是完全不同的事。
//   同理 circleArea(5) 這種呼叫，最佳化後也常被直接摺疊成一個常數。
//
// 【3. C++17 的關鍵改變：隱含 inline】
//   C++11/14 時代，如果對 constexpr static 成員做了 odr-use
//   （例如綁到 const 參考、取位址），就必須在某個 .cpp 補上類別外定義：
//       constexpr int MathConstants::MAX_DIMENSION;   // C++11 必要
//   否則連結期會 undefined reference。
//   C++17 起這類成員隱含就是 inline 變數，定義已經完備，不必再寫。
//
//   ★ 本檔原註解說「寫了會導致 multiple definition 連結錯誤」，這是不正確的。
//     本機以 g++ -std=c++17 -pedantic-errors 實測：
//       * 寫出類別外定義 → 只有 -Wdeprecated 警告
//         「redundant redeclaration of 'constexpr' static data member」，
//         編譯與連結都成功，不是錯誤。
//       * 以 -std=c++11 編譯、且對成員做 odr-use（綁 const 參考）
//         而沒有類別外定義 → 連結期 undefined reference。
//     結論：C++17 起那行是「多餘且已棄用」，不是「會壞掉」。
//
// 【4. 為什麼常數要放進類別而不是全域】
//   放進類別可以得到命名空間效果：MathConstants::PI 一看就知道出處，
//   也不會和其他函式庫的 PI 撞名（歷史上 <cmath> 的 M_PI 就不是標準的，
//   在 -std=c++17 這種嚴格模式下未必存在）。
//   C++20 起標準提供 std::numbers::pi（<numbers>），新程式碼可優先使用。
//
// 【概念補充 Concept Deep Dive】
//   * constexpr 變數隱含 const，所以不必再寫 const。
//   * constexpr static 成員通常「不佔執行期記憶體」——
//     編譯器把值直接嵌進使用處。只有在被 odr-use（取位址、綁參考）時
//     才需要真正配置一份儲存空間。
//   * 浮點數的編譯期求值：PI * PI 由編譯器以目標平台的浮點語意計算，
//     結果與執行期算相同，不會因為提前算而失準。
//   * 3.14159265358979 只有 14 位有效數字，double 約可表示 15~17 位。
//     這是常數本身寫得不夠精確，不是型別的限制。
//   * constexpr 函式（C++11 起，C++14 大幅放寬限制）可以讓
//     circleArea 這類函式也能用在編譯期，寫成 static constexpr double
//     circleArea(double r) 即可。本檔維持一般 static 函式以聚焦主題。
//
// 【注意事項 Pay Attention】
//   1. C++17 起不需要類別外定義；寫了不會出錯，但是多餘且已棄用（-Wdeprecated）。
//   2. C++11/14 若對 constexpr static 成員做 odr-use，缺定義會在連結期報錯。
//   3. const 不等於 constexpr —— const 值可以是執行期才決定的。
//   4. 浮點常數的位數要寫足，否則損失的是精度而非效能。
//   5. 需要標準的數學常數時，C++20 起優先用 std::numbers::pi。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】constexpr static 成員
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. const 和 constexpr 差在哪裡?
//     答：const 只保證「不可被修改」，初值可以是執行期才算出來的。
//         constexpr 額外保證「編譯期就能求值」，
//         因此才能用在陣列大小、template 引數、static_assert 這些
//         需要常數運算式的位置。constexpr 變數本身隱含 const。
//     追問：那 const int n = 5; 能當陣列大小嗎?
//         → 可以。整數型別、以常數運算式初始化的 const 變數
//         本來就是常數運算式（這是 C++98 就有的規則）。
//         但換成 const double 或執行期初值就不行了，
//         寫 constexpr 才能讓「必須編譯期可知」這件事變成編譯器強制檢查。
//
// 🔥 Q2. C++17 對 constexpr static 資料成員做了什麼改變?
//     答：C++17 起它隱含就是 inline 變數，類別內的宣告同時就是定義，
//         不需要再到 .cpp 補一行類別外定義。
//         C++11/14 時代只要發生 odr-use（取位址、綁 const 參考）
//         就必須補，否則連結期 undefined reference。
//     追問：C++17 之後還寫那行會怎樣?
//         → 不會錯，只是多餘且已被棄用；
//         本機 g++ 會給 -Wdeprecated「redundant redeclaration」警告。
//
// ⚠️ 陷阱. 「constexpr static 成員反正是編譯期常數，
//            所以絕對不會需要記憶體、也絕對不會有連結問題。」
//     答：不成立。一旦對它做 odr-use ——
//         最常見的是綁到 const 參考、取位址，
//         或傳給 const T& 參數（例如 std::max(a, MathConstants::MAX_DIMENSION)）
//         —— 就需要一個真實的物件存在。
//         在 C++11/14 這正是缺了類別外定義就連結失敗的情境；
//         C++17 靠隱含 inline 才讓它自動成立。
//     為什麼會錯：把「值在編譯期已知」誤推成「永遠不需要實體」。
//         值可以被嵌進指令，但參考與指標必須指向真實位址，
//         那就得有一份實體儲存。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】—— 從缺，理由如下
//   constexpr 是編譯期求值機制，LeetCode 判題只驗輸入輸出，
//   不會考「這個值是編譯期算的還是執行期算的」。
//   （少數題目可用查表法預先算好，但那是演算法技巧、不是本主題。）
//   故改以下方協定常數的實務範例呈現 constexpr 的真正價值：
//   讓錯誤在編譯期就被擋下來。
//
// =============================================================================

#include <iostream>
using namespace std;

class MathConstants {
public:
    // constexpr static：編譯期常量，最高效
    // C++17 以後，constexpr static 成員變數不需要在類外定義
    // 常量定義
    static constexpr double PI = 3.14159265358979;
    static constexpr double E  = 2.71828182845905;
    static constexpr int    MAX_DIMENSION = 3;

    // 編譯期計算
    // 其他常量可以基於 PI 計算得出
    // 這些也是編譯期常量，使用時不會有運算開銷
    static constexpr double TWO_PI = PI * 2.0;
    static constexpr double PI_SQUARED = PI * PI;

    static double circleArea(double r) {
        return PI * r * r;
    }

    static double sphereVolume(double r) {
        return (4.0 / 3.0) * PI * r * r * r;
    }
};

// constexpr static 不需要類別外定義（C++17 起隱含 inline）
// constexpr double MathConstants::PI;  // 多餘：C++17 之後不需要寫
//
// 更正：上面這行若真的寫出來，並不會造成連結錯誤。
// 本機 g++ -std=c++17 -pedantic-errors 實測結果是
//   warning: redundant redeclaration of 'constexpr' static data member [-Wdeprecated]
// 也就是「多餘且已棄用」，編譯與連結都會成功。
// 真正會壞掉的是相反的情況：用 -std=c++11 編譯、對成員做 odr-use
// （例如 const int& r = MathConstants::MAX_DIMENSION;）卻沒寫類別外定義，
// 那才會在連結期得到 undefined reference。

// -----------------------------------------------------------------------------
// 【日常實務範例】網路封包協定的欄位配置
//   情境：自訂二進位協定，標頭固定 16 bytes，欄位偏移量必須與規格書一致。
//         把這些數字寫成 constexpr static，好處不只是命名清楚 ——
//         而是可以用 static_assert 在「編譯期」驗證欄位排列有沒有算錯。
//   價值：規格改了而偏移量忘了同步，編譯就直接失敗，
//         不會等到上線後才在解析封包時讀到錯位的資料。
// -----------------------------------------------------------------------------
struct PacketHeader {
    // 各欄位的位元組偏移量與長度（與規格書一一對應）
    static constexpr size_t kMagicOffset   = 0;
    static constexpr size_t kMagicSize     = 4;   // "PKT\0"
    static constexpr size_t kVersionOffset = 4;
    static constexpr size_t kVersionSize   = 2;
    static constexpr size_t kFlagsOffset   = 6;
    static constexpr size_t kFlagsSize     = 2;
    static constexpr size_t kLengthOffset  = 8;
    static constexpr size_t kLengthSize    = 8;

    static constexpr size_t kHeaderSize    = 16;  // 規格書寫死的標頭大小

    // 編譯期自我檢查：欄位必須首尾相接，且總和等於標頭大小。
    // 任何一個偏移量寫錯，這裡就編譯失敗，錯誤留不到執行期。
    static_assert(kMagicOffset + kMagicSize == kVersionOffset,
                  "magic 與 version 欄位沒有相接");
    static_assert(kVersionOffset + kVersionSize == kFlagsOffset,
                  "version 與 flags 欄位沒有相接");
    static_assert(kFlagsOffset + kFlagsSize == kLengthOffset,
                  "flags 與 length 欄位沒有相接");
    static_assert(kLengthOffset + kLengthSize == kHeaderSize,
                  "欄位總和與 kHeaderSize 不符");
};

int main() {
    cout << "=== constexpr static ===" << endl;
    cout << "  PI = " << MathConstants::PI << endl;
    cout << "  E  = " << MathConstants::E << endl;
    cout << "  2*PI = " << MathConstants::TWO_PI << endl;
    cout << "  PI^2 = " << MathConstants::PI_SQUARED << endl;

    cout << "\n  圓面積(r=5)：" << MathConstants::circleArea(5) << endl;
    cout << "  球體積(r=3)：" << MathConstants::sphereVolume(3) << endl;

    // 可以用在編譯期需要常量的地方
    int arr[MathConstants::MAX_DIMENSION] = {1, 2, 3};  // ✅
    cout << "\n  陣列大小：" << sizeof(arr) / sizeof(arr[0]) << endl;

    // constexpr 可以用在 static_assert：錯了就編譯不過，不會留到執行期
    static_assert(MathConstants::MAX_DIMENSION == 3, "維度必須是 3");
    static_assert(MathConstants::TWO_PI > 6.28 && MathConstants::TWO_PI < 6.29,
                  "TWO_PI 應該約等於 6.283");

    cout << "\n=== 日常實務：封包標頭的編譯期驗證 ===" << endl;
    cout << "  標頭總長度   = " << PacketHeader::kHeaderSize  << " bytes" << endl;
    cout << "  magic   偏移 = " << PacketHeader::kMagicOffset
         << "，長度 " << PacketHeader::kMagicSize   << endl;
    cout << "  version 偏移 = " << PacketHeader::kVersionOffset
         << "，長度 " << PacketHeader::kVersionSize << endl;
    cout << "  flags   偏移 = " << PacketHeader::kFlagsOffset
         << "，長度 " << PacketHeader::kFlagsSize   << endl;
    cout << "  length  偏移 = " << PacketHeader::kLengthOffset
         << "，長度 " << PacketHeader::kLengthSize  << endl;
    cout << "  ↑ 四個 static_assert 已在編譯期驗證欄位首尾相接、" << endl;
    cout << "    總和等於 kHeaderSize。改錯任何一個數字，編譯就會失敗。" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第 24 課：類別內的靜態成員變數7.cpp -o static_member7

// === 預期輸出 ===
// === constexpr static ===
//   PI = 3.14159
//   E  = 2.71828
//   2*PI = 6.28319
//   PI^2 = 9.8696
//
//   圓面積(r=5)：78.5398
//   球體積(r=3)：113.097
//
//   陣列大小：3
//
// === 日常實務：封包標頭的編譯期驗證 ===
//   標頭總長度   = 16 bytes
//   magic   偏移 = 0，長度 4
//   version 偏移 = 4，長度 2
//   flags   偏移 = 6，長度 2
//   length  偏移 = 8，長度 8
//   ↑ 四個 static_assert 已在編譯期驗證欄位首尾相接、
//     總和等於 kHeaderSize。改錯任何一個數字，編譯就會失敗。
