// =============================================================================
//  07_nested_namespace.cpp  —  巢狀 namespace 簡化語法 (C++17)
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/language/namespace
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、語法                                                   │
//  └────────────────────────────────────────────────────────────┘
//
//  C++17 之前要寫巢狀 namespace：
//
//      namespace company {
//          namespace product {
//              namespace util {
//                  void f();
//              }
//          }
//      }
//
//  C++17 起：
//
//      namespace company::product::util {
//          void f();
//      }
//
//  少寫好幾層大括號、視覺上跟使用點 company::product::util 對應。
//
//  C++20 又加了「inline + 巢狀」混合：
//
//      namespace x::inline v1::detail { ... }
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、本檔示範                                               │
//  └────────────────────────────────────────────────────────────┘
// =============================================================================

/*
補充筆記：nested_namespace
  - nested_namespace 是現代 C++ 語法或標準庫特性；學習時要把「少寫字」和「語意更精確」分開看。
  - auto 讓型別由初始化式推導，但會丟掉 top-level const/reference；需要保留引用語意時要寫 auto&、const auto& 或 decltype(auto)。
  - brace initialization 能減少未初始化與 narrowing，但遇到 initializer_list overload 可能選到不同建構子。
  - constexpr、static_assert、if constexpr 把部分錯誤和計算提前到編譯期，能讓 template 和常數邏輯更清楚。
  - 屬性如 [[nodiscard]]、[[maybe_unused]]、[[fallthrough]] 是對編譯器和讀者的意圖標記，不應拿來掩蓋設計問題。
  - string_view、optional、variant、structured binding 等特性改善介面表達力，但也帶來生命週期或狀態檢查責任。
  - nested namespace 語法 namespace a::b::c 減少巢狀大括號。
  - 它只是語法糖，不改變 namespace 的 linkage 或查找規則。
*/
#include <iostream>
#include <string>

// C++17 簡化版
namespace company::product::util {
    std::string version() { return "1.0.0"; }

    namespace detail {                       // 仍可繼續嵌入
        int magic() { return 42; }
    }
}

// 對比：傳統寫法
namespace old::nest {
    namespace x {
        int v = 99;
    }
}

// 實用範例用：多層命名空間封裝
namespace app::net::http::v1 {
    std::string userAgent() { return "my-app/1.0 (cpp17)"; }
    std::string formatUrl(const std::string& host, int port, const std::string& path) {
        return "http://" + host + ":" + std::to_string(port) + path;
    }
}

int main() {
    std::cout << "[Demo] util::version()  = "
              << company::product::util::version() << '\n';
    std::cout << "[Demo] detail::magic()  = "
              << company::product::util::detail::magic() << '\n';
    std::cout << "[Demo] old::nest::x::v = "
              << old::nest::x::v << '\n';

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：能跟 inline namespace 結合嗎？
    //    A：C++20 才行：namespace x::inline v1::detail { ... }
    //       C++17 巢狀簡化只支援普通 namespace（不能在中間 inline）。
    //
    //  Q2：用 namespace 別名取代深度命名空間？
    //    A：可以，且常見：
    //         namespace cpu = company::product::util;
    //         cpu::version();
    //
    //  Q3：anonymous namespace 也能嵌套？
    //    A：可以但少用：namespace x::y { namespace { ... } } 等同
    //       「在 x::y 內部一個 anonymous namespace」 — 內容對其他 TU 不
    //       可見。
    //
    // ─────────────────────────────────────────────────────────
    // 實用範例：模組分層 — net::http::v1 內封裝 HTTP 工具
    //   工作上常見：把 networking 函式分層命名，避免污染全域
    //   注意：namespace alias 用 namespace X = ...; 不是 using
    // ─────────────────────────────────────────────────────────
    namespace app_net = ::app::net::http::v1;
    std::cout << "[Demo4] " << app_net::userAgent() << '\n';
    std::cout << "[Demo4] " << app_net::formatUrl("example.com", 80, "/api") << '\n';

    return 0;
}
