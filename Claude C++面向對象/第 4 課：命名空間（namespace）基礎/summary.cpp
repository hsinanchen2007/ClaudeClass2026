/*
 * ================================================================
 * 【第4課：命名空間（namespace）基礎】總複習 summary.cpp
 * ================================================================
 * 編譯方式：g++ -std=c++17 -Wall -Wextra -o summary summary.cpp
 *
 * 本課重點：
 * 1. 命名空間的目的：解決名稱衝突問題
 * 2. 定義命名空間的語法：namespace name { ... }
 * 3. 三種存取方式：完全限定名稱、using 宣告、using 指令
 * 4. 命名空間別名：namespace 別名 = 原命名空間;
 * 5. 匿名命名空間：限制符號只在當前檔案內可見
 * 6. 巢狀命名空間：傳統寫法 vs C++17 簡化寫法
 * 7. 擴展命名空間：同一命名空間可在多處定義
 * 8. std 命名空間：C++ 標準函式庫的命名空間
 * 9. 最佳實踐：何時用、何時不用 using namespace
 * ================================================================
 */

#include <iostream>
#include <string>
#include <cmath>

// ================================================================
// ===== 重點一：為何需要命名空間？=====
// 說明：當多個函式庫都定義了相同名稱的函式或變數時，就會
//       發生「名稱衝突（name collision）」，編譯器無法區分。
//       C 語言的做法是在名稱前加前綴（如 math_add、string_add），
//       C++ 提供了更優雅的解決方案：命名空間。
// 重要性：是大型專案和多函式庫協作的基礎機制。
//
// 示意（若沒有命名空間）：
//   void draw();  // 圖形庫的 draw
//   void draw();  // 日誌庫的 draw  ← 衝突！編譯錯誤
//
// 有了命名空間：
//   namespace graphics { void draw(); }
//   namespace logging  { void draw(); }
//   graphics::draw();  // 明確！
//   logging::draw();   // 明確！
// ================================================================


// ===== 重點二：定義命名空間 =====
// 語法：namespace 名稱 { 變數、函式、類別... }
// 重要性：將相關的識別符號（變數、函式、型別）組織在同一個
//         邏輯命名空間下，避免與外部名稱衝突。

namespace math_tools {
    // 命名空間內可以放常數
    const double PI = 3.14159265358979;
    const double E  = 2.71828182845905;

    // 命名空間內可以放函式
    double circleArea(double radius) {
        return PI * radius * radius;
    }

    double circleCircumference(double radius) {
        return 2.0 * PI * radius;
    }

    // 使用 int 參數的簡單冪次計算（避免引入 cmath 的 pow）
    int power(int base, int exp) {
        int result = 1;
        for (int i = 0; i < exp; i++) {
            result *= base;
        }
        return result;
    }
}

// ===== 重點三：字串工具命名空間（展示不同命名空間隔離） =====
namespace string_tools {
    // 即使函式名稱與其他地方相同也不衝突
    std::string toUpperCase(const std::string& str) {
        std::string result = str;
        for (char& c : result) {
            if (c >= 'a' && c <= 'z') c = c - 'a' + 'A';
        }
        return result;
    }

    std::string toLowerCase(const std::string& str) {
        std::string result = str;
        for (char& c : result) {
            if (c >= 'A' && c <= 'Z') c = c - 'A' + 'a';
        }
        return result;
    }

    std::string repeat(const std::string& str, int times) {
        std::string result;
        for (int i = 0; i < times; i++) result += str;
        return result;
    }
}

// ===== 重點四：巢狀命名空間（Nested Namespace） =====
// 說明：命名空間可以嵌套，模擬公司/專案/模組的層次結構。
// 傳統寫法（C++11/14）：
//   namespace company { namespace graphics { void draw(); } }
// C++17 簡化寫法：
//   namespace company::graphics { void draw(); }
// 重要性：讓大型專案的命名空間層次結構更清晰。

namespace company {
    namespace graphics {
        void draw() {
            std::cout << "  [Graphics] 繪製圖形\n";
        }
        void clear() {
            std::cout << "  [Graphics] 清除畫面\n";
        }
    }
    namespace audio {
        void play() {
            std::cout << "  [Audio] 播放音效\n";
        }
        void stop() {
            std::cout << "  [Audio] 停止播放\n";
        }
    }
}

// C++17 巢狀命名空間簡化寫法（等同於上面的巢狀寫法）
namespace company::network {
    void connect() {
        std::cout << "  [Network] 建立連線\n";
    }
}

// ===== 重點五：擴展命名空間 =====
// 說明：同一個命名空間名稱可以在多個地方出現，編譯器會自動合併。
//       這在多檔案專案中非常常見：
//       - math.h    定義 namespace mylib { 數學函式 }
//       - string.h  定義 namespace mylib { 字串函式 }
//       它們共同組成完整的 mylib 命名空間。
// 重要性：讓大型函式庫可以分散在多個標頭檔中，但共用同一個命名空間。

namespace mylib {
    void funcA() { std::cout << "  mylib::funcA() 已呼叫\n"; }
}

namespace mylib {  // 擴展：繼續在 mylib 中加入新成員
    void funcB() { std::cout << "  mylib::funcB() 已呼叫\n"; }
}

// ===== 重點六：匿名命名空間 =====
// 說明：沒有名稱的命名空間，其成員只在「當前翻譯單元（.cpp 檔）」內可見。
//       等效於 C 語言的 static，但 C++ 更推薦使用匿名命名空間。
// 重要性：隱藏檔案內部實作細節，防止連結時與其他檔案的同名符號衝突。
// 使用時機：輔助函式、檔案內部計數器、只在本檔案用到的工具函式。

namespace {
    int callCount = 0;  // 只在本檔案內可見

    void logCall(const std::string& funcName) {
        callCount++;
        std::cout << "  [Log #" << callCount << "] 呼叫: " << funcName << "\n";
    }
}

// ================================================================
// main() 展示所有重點
// ================================================================
int main() {
    std::cout << "================================================================\n";
    std::cout << "  第4課：命名空間（namespace）基礎 總複習\n";
    std::cout << "================================================================\n\n";

    // ------------------------------------------------------------------
    // 重點二 示範：使用完全限定名稱（:: 運算子）
    // ------------------------------------------------------------------
    // 說明：「命名空間::成員」的完整形式稱為「完全限定名稱」。
    //       這是最安全、最推薦的使用方式，意圖明確，不會引起歧義。
    // 缺點：程式碼較長，但在大型專案中這是值得的代價。
    std::cout << "===== [存取方式一] 完全限定名稱（Fully Qualified Name） =====\n";
    std::cout << "  math_tools::PI            = " << math_tools::PI << "\n";
    std::cout << "  math_tools::E             = " << math_tools::E << "\n";
    std::cout << "  math_tools::circleArea(5) = " << math_tools::circleArea(5) << "\n";
    std::cout << "  math_tools::power(2, 10)  = " << math_tools::power(2, 10) << "\n";
    std::cout << "\n";

    // ------------------------------------------------------------------
    // 重點三 示範：using 宣告（引入特定成員）
    // ------------------------------------------------------------------
    // 說明：using 命名空間::成員;
    //       只將指定的一個成員引入目前作用域，其他成員仍需完整名稱。
    // 優點：比全域 using namespace 更安全，只引入真正需要的名稱。
    // 建議：在函式內部使用，避免污染全域命名空間。
    std::cout << "===== [存取方式二] using 宣告（引入特定成員） =====\n";
    {
        using string_tools::toUpperCase;
        using string_tools::toLowerCase;

        std::string text = "Hello World";
        std::cout << "  原始:    " << text << "\n";
        std::cout << "  toUpperCase: " << toUpperCase(text) << "\n";
        std::cout << "  toLowerCase: " << toLowerCase(text) << "\n";

        // repeat 沒有用 using，仍需完整名稱
        std::cout << "  repeat(\"Hi\", 3): " << string_tools::repeat("Hi", 3) << "\n";
    }
    // 離開大括號後，toUpperCase / toLowerCase 的 using 宣告失效
    std::cout << "\n";

    // ------------------------------------------------------------------
    // 重點三 示範：using 指令（引入整個命名空間）
    // ------------------------------------------------------------------
    // 說明：using namespace 名稱;
    //       將整個命名空間的所有成員引入目前作用域。
    // 缺點：可能造成名稱衝突，在標頭檔（.h）中絕對禁止使用！
    // 安全用法：限制在函式內部或區塊內部，縮小影響範圍。
    std::cout << "===== [存取方式三] using 指令（僅在區塊內使用） =====\n";
    {
        // 只在這個 {} 區塊內有效，出了區塊就失效
        using namespace math_tools;

        std::cout << "  在區塊內直接使用 PI: " << PI << "\n";
        std::cout << "  在區塊內直接使用 circleArea(3): " << circleArea(3) << "\n";
    }
    // 離開區塊後，又必須使用完整名稱
    std::cout << "  離開區塊後需完整名稱: math_tools::PI = " << math_tools::PI << "\n";
    std::cout << "\n";

    // ------------------------------------------------------------------
    // 重點四 示範：命名空間別名
    // ------------------------------------------------------------------
    // 說明：namespace 別名 = 原命名空間;
    //       為名稱很長的命名空間建立一個簡短別名。
    // 重要性：常用於簡化 std::filesystem、boost::asio 等長命名空間。
    //         原始名稱仍然完全有效。
    std::cout << "===== [重點四] 命名空間別名 =====\n";
    {
        namespace mt = math_tools;       // 為 math_tools 建立別名 mt
        namespace st = string_tools;     // 為 string_tools 建立別名 st
        namespace gfx = company::graphics;  // 為巢狀命名空間建立別名

        std::cout << "  mt::PI = " << mt::PI << "\n";
        std::cout << "  st::toUpperCase(\"alias\") = " << st::toUpperCase("alias") << "\n";
        gfx::draw();

        // 原名稱仍然有效
        std::cout << "  math_tools::E = " << math_tools::E << "（原名稱仍有效）\n";
    }
    std::cout << "\n";

    // ------------------------------------------------------------------
    // 重點四 示範：巢狀命名空間存取
    // ------------------------------------------------------------------
    std::cout << "===== [重點四] 巢狀命名空間 =====\n";
    company::graphics::draw();
    company::graphics::clear();
    company::audio::play();
    company::audio::stop();
    company::network::connect();  // C++17 簡化寫法定義的

    {
        // 使用別名簡化巢狀命名空間的存取
        namespace sfx = company::audio;
        std::cout << "  使用別名 sfx：\n";
        sfx::play();
    }
    std::cout << "\n";

    // ------------------------------------------------------------------
    // 重點五 示範：擴展命名空間
    // ------------------------------------------------------------------
    std::cout << "===== [重點五] 擴展命名空間 =====\n";
    // funcA 和 funcB 都屬於 mylib，即使定義在不同地方
    mylib::funcA();
    mylib::funcB();
    std::cout << "\n";

    // ------------------------------------------------------------------
    // 重點六 示範：匿名命名空間
    // ------------------------------------------------------------------
    std::cout << "===== [重點六] 匿名命名空間 =====\n";
    // callCount 和 logCall 定義在匿名命名空間，直接使用（無需前綴）
    logCall("math_tools::circleArea");
    logCall("string_tools::toUpperCase");
    logCall("mylib::funcA");
    std::cout << "  總呼叫次數: " << callCount << "\n";
    std::cout << "\n";

    // ------------------------------------------------------------------
    // 重點七：最佳實踐總結
    // ------------------------------------------------------------------
    std::cout << "===== [重點七] 最佳實踐速查 =====\n";
    std::cout << "  標頭檔（.h）中：絕對不能使用 using namespace！\n";
    std::cout << "  大型專案：    優先使用完全限定名稱（std::cout）\n";
    std::cout << "  局部簡化：    在函式/區塊內用 using 宣告（引入特定成員）\n";
    std::cout << "  長命名空間：  用 namespace 別名 = 原命名空間\n";
    std::cout << "  檔案內部：    使用匿名命名空間取代 static\n";
    std::cout << "  C++17 巢狀：  namespace a::b::c { } 簡化寫法\n";

    std::cout << "\n================================================================\n";
    std::cout << "  總複習完成！\n";
    std::cout << "================================================================\n";

    return 0;
}
