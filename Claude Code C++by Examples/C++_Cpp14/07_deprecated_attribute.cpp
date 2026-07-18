// =============================================================================
//  07_deprecated_attribute.cpp  —  [[deprecated]] 屬性 (C++14)
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/language/attributes/deprecated
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、什麼是 [[deprecated]]？                                │
//  └────────────────────────────────────────────────────────────┘
//
//  C++14 標準化的「屬性」(attribute)，告訴編譯器：「這個 entity 已棄用，
//  使用時要警告」。
//
//      [[deprecated]] void oldFunc();
//      [[deprecated("use newFunc instead")]] void oldFunc2();
//
//      void caller() {
//          oldFunc();         // 編譯時警告：oldFunc is deprecated
//          oldFunc2();        // 警告 + 訊息 "use newFunc instead"
//      }
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、可以加在哪裡？                                         │
//  └────────────────────────────────────────────────────────────┘
//
//   * 函式 / 成員函式
//   * class / struct / enum
//   * typedef / using 別名
//   * 變數
//   * namespace（C++14 之後）
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、跟編譯器的 deprecation diagnostic 對接                 │
//  └────────────────────────────────────────────────────────────┘
//
//  GCC / Clang / MSVC 都把 [[deprecated]] 對應到自家「deprecation
//  warning」 — 通常是 -Wdeprecated-declarations 警告群。專案 CI 設
//  -Werror=deprecated-declarations 後就能逐步淘汰舊 API。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 四、本檔示範                                               │
//  └────────────────────────────────────────────────────────────┘
//
//   * 注意：被 deprecated 的東西「編譯時會警告」 — 我們把警告壓掉以免
//     Makefile 的 -Wall -Wextra 變成 error。
// =============================================================================

/*
補充筆記：deprecated_attribute
  - deprecated_attribute 是現代 C++ 語法或標準庫特性；學習時要把「少寫字」和「語意更精確」分開看。
  - auto 讓型別由初始化式推導，但會丟掉 top-level const/reference；需要保留引用語意時要寫 auto&、const auto& 或 decltype(auto)。
  - brace initialization 能減少未初始化與 narrowing，但遇到 initializer_list overload 可能選到不同建構子。
  - constexpr、static_assert、if constexpr 把部分錯誤和計算提前到編譯期，能讓 template 和常數邏輯更清楚。
  - 屬性如 [[nodiscard]]、[[maybe_unused]]、[[fallthrough]] 是對編譯器和讀者的意圖標記，不應拿來掩蓋設計問題。
  - string_view、optional、variant、structured binding 等特性改善介面表達力，但也帶來生命週期或狀態檢查責任。
  - [[deprecated]] 讓編譯器在使用舊 API 時警告，適合平滑遷移。
  - deprecated 訊息應指出替代 API，否則呼叫者只知道不能用，不知道該怎麼改。
*/
#include <iostream>
#include <string>

[[deprecated]]
static void oldApi() { std::cout << "oldApi running\n"; }

[[deprecated("use modernHash() instead, MD5 is broken")]]
static int legacyHash(const std::string& s) {
    int h = 0;
    for (char c : s) h = h * 31 + c;
    return h;
}

class [[deprecated("use NewClient")]] OldClient {
public:
    void connect() { std::cout << "OldClient::connect\n"; }
};

int main() {
    // 用 GCC pragma 局部關掉 deprecated warning（教學示範用）
    // 實務工作上，這些呼叫會被編譯警告抓出，正是 [[deprecated]] 的目的
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

    oldApi();
    int h = legacyHash("hello");
    std::cout << "[Demo] legacyHash(\"hello\") = " << h << '\n';

    OldClient c;
    c.connect();

#pragma GCC diagnostic pop

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：[[deprecated]] 跟 #pragma deprecated 差在哪？
    //    A：[[deprecated]] 是標準語法，跨編譯器一致；#pragma 是 MSVC 私有
    //       的 / GCC __attribute__((deprecated)) 私有的。新程式碼一律用
    //       標準屬性。
    //
    //  Q2：使用 deprecated 函式會編譯失敗嗎？
    //    A：預設只「警告」。-Werror=deprecated-declarations 才會把警告升
    //       級為錯誤。許多大專案 CI 開這個 flag 強制清掉舊 API。
    //
    //  Q3：能加在 lambda 上嗎？
    //    A：C++14 還不行；C++17 起 [[attr]] 也能放 lambda 上：
    //         auto f = [[deprecated]](int x) { return x; };
    //       但 lambda deprecated 用處不多（lambda 通常一次性）。
    //
    // ─────────────────────────────────────────────────────────
    // 實用範例 1：deprecated 型別別名 — 引導使用者改用新 API
    //   工作上常見：把 typedef OldName 標 deprecated，要他們改用 NewName
    // ─────────────────────────────────────────────────────────
    using NewBufferSize = std::size_t;
    using OldBufferSize [[deprecated("use NewBufferSize instead")]] = int;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    OldBufferSize legacy_size = 1024;          // 編譯器警告
#pragma GCC diagnostic pop
    NewBufferSize modern_size = 2048;
    std::cout << "[Demo4] legacy_size=" << legacy_size
              << " modern_size=" << modern_size << '\n';

    // ─────────────────────────────────────────────────────────
    // 實用範例 2：deprecated enumerator — enum 內個別值棄用
    //   工作上常見：協定升版時某些 code 被廢棄但未刪
    // ─────────────────────────────────────────────────────────
    enum class Algo {
        AES,
        ChaCha20,
        DES [[deprecated("DES is broken, use AES or ChaCha20")]]
    };
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    Algo a = Algo::DES;                        // 警告：DES deprecated
#pragma GCC diagnostic pop
    std::cout << "[Demo5] algo code = " << static_cast<int>(a) << '\n';

    return 0;
}
