// =============================================================================
//  第 18 課：vectorbool 的特殊性與陷阱3.cpp  —  operator[] 回傳的到底是什麼？
// =============================================================================
//
// 【主題資訊 Information】
//   // 主模板 vector<T>
//   reference       vector<T>::operator[](size_type n);        // T&
//   const_reference vector<T>::operator[](size_type n) const;  // const T&
//
//   // 特化 vector<bool>
//   reference       vector<bool>::operator[](size_type n);        // proxy class（實作定義名稱）
//   const_reference vector<bool>::operator[](size_type n) const;  // bool（標準規定是純量！）
//
//   標頭檔  : <vector>、<typeinfo>（typeid）
//   標準版本: C++98 起
//   複雜度  : O(1)
//   libstdc++ 的 proxy 名稱: std::_Bit_reference（本機實測 sizeof = 16，實作定義）
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼不能回傳 bool&】
//   這是整堂課的根，只有一句話：C++ 沒有「指向單一 bit 的引用或指標」。
//   語言規定最小可定址單位是 byte（sizeof(char) == 1），指標算術以 byte 為單位。
//   當 8 個 bool 擠在同一個 byte 裡，你無法造出一個「只指到第 3 個 bit」的 bool&。
//
//   標準委員會的選擇是：既然回不了真引用，就回傳一個「行為像引用的物件」——
//   proxy（代理物件）。這是 C++ 裡 proxy pattern 最著名的一個案例。
//
// 【2. proxy 內部長什麼樣】
//   概念上就是「word 指標 + bit 遮罩」這兩個欄位：
//       struct _Bit_reference {
//           _Bit_type* _M_p;      // 指向所屬的那個 word（本機 unsigned long*，8 bytes）
//           _Bit_type  _M_mask;   // 標出是 word 內第幾個 bit，例如 1ul << 3（8 bytes）
//           operator bool() const { return !!(*_M_p & _M_mask); }   // 讀
//           _Bit_reference& operator=(bool x);                      // 寫
//           void flip();                                            // 翻轉
//       };
//   兩個欄位各 8 bytes，所以本機實測 sizeof 是 16。這個 16 是【實作定義】：
//   換一個 STL（例如用 unsigned int 當 word、或用 offset 而非 mask）就會不同。
//
//   關鍵在於：proxy 是一個「有值語意的小物件」，但它的值是『座標』而不是『內容』。
//   複製 proxy 複製的是座標，兩份 proxy 仍指向同一個 bit——這正是第 4 檔陷阱的根源。
//
// 【3. 讀寫都被轉譯成位元運算】
//       bool b = vb[3];      // → operator bool()：(*_M_p & _M_mask) != 0
//       vb[3]  = true;       // → operator=(bool)：*_M_p |=  _M_mask
//       vb[3]  = false;      //                    *_M_p &= ~_M_mask
//   一個看起來像「讀陣列元素」的動作，其實是取 word、位移、遮罩三步。
//   編譯器最佳化後通常只有 2~3 道指令，不算貴，但確實比 vector<char> 多。
//
// 【4. const 版本回傳的是純量 bool，不是 proxy】
//   這點常被忽略但很重要，而且是標準明文規定：
//       vector<bool>::const_reference 就是 bool。
//   本機實測 std::is_same_v<std::vector<bool>::const_reference, bool> == true。
//   道理很直接：唯讀就不需要「能寫回去」的能力，回傳一份 bool 副本即可，沒必要
//   帶著 proxy 的包袱。所以：
//       const std::vector<bool> cv = {true};
//       auto x = cv[0];      // x 的型別是 bool，沒有陷阱！
//       std::vector<bool> mv = {true};
//       auto y = mv[0];      // y 是 proxy，有陷阱！
//   同一份程式碼加不加 const，auto 推出來的型別完全不同——這也是為什麼「不要用
//   auto 接 vector<bool> 的元素」要當成無條件的守則，而不是看情況判斷。
//
// 【5. typeid(...).name() 為什麼是亂碼】
//   本機輸出 St14_Bit_reference：這是 Itanium C++ ABI 的 mangled name。
//   拆解：St = std::，14 = 接下來的識別字有 14 個字元，_Bit_reference 剛好 14 個。
//   而 int 的 mangled name 就是單一字元 i。
//   要看到人類可讀的名字得呼叫 abi::__cxa_demangle()（GCC/Clang 專屬，非標準），
//   本檔示範了這個做法。注意 typeid(...).name() 的回傳內容是【實作定義】，
//   MSVC 直接回傳可讀字串，不要寫程式去比對它的內容。
//
// 【概念補充 Concept Deep Dive】
//   為什麼 sizeof(proxy) 比 bool 大這麼多，效能還可以接受？
//   因為 proxy 幾乎總是短命的暫存物件：它在運算式中生成、用完就丟，
//   編譯器最佳化後根本不會真的把這 16 bytes 寫進記憶體，只是暫存在暫存器裡。
//   換句話說，這 16 bytes 是「編譯期的抽象成本」，不是「執行期的儲存成本」。
//   真正被存起來的資料仍然是 1 bit/元素。
//
//   但只要你寫了 auto val = vb[0]; 把它「存起來」，這個抽象就從暫存變成長命的
//   物件——16 bytes 真的被實體化，而且繼續指著容器內部。陷阱就是這樣誕生的。
//
// 【注意事項 Pay Attention】
//   1. proxy 的類別名稱（std::_Bit_reference）與 sizeof（本機 16）都是【實作定義】，
//      標準只保證有一個叫 vector<bool>::reference 的型別存在。
//   2. const_reference 是 bool 則是【標準規定】，可以放心依賴。
//   3. typeid(...).name() 的內容是【實作定義】，只適合列印給人看，不要拿來比對。
//   4. 千萬不要以為 sizeof(vb[0]) 反映了「每個元素佔的空間」——那是 proxy 的大小，
//      實際儲存仍是 1 bit/元素。
//   5. abi::__cxa_demangle 是 GCC/Clang 的擴充（<cxxabi.h>），MSVC 沒有。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】proxy reference 的型別
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. vector<bool>::operator[] 回傳什麼型別？為什麼不能回傳 bool&？
//     答：回傳 vector<bool>::reference，一個 proxy class（libstdc++ 叫
//         std::_Bit_reference）。不能回傳 bool& 是因為 C++ 最小可定址單位是 byte，
//         語言層面就造不出「指向單一 bit」的引用。proxy 用「word 指標 + bit 遮罩」
//         模擬引用行為，並靠 operator bool() 與 operator=(bool) 撐起讀寫語法。
//     追問：那 sizeof 是多少？→ 本機 16 bytes（實作定義），因為兩個 8-byte 欄位。
//         但這是編譯期抽象成本，最佳化後通常不落地，實際儲存仍是 1 bit/元素。
//
// 🔥 Q2. 為什麼 vector<bool>::const_reference 沒有這個問題？
//     答：因為標準明文規定 const_reference 就是 bool（純量），不是 proxy。
//         唯讀不需要「寫回去」的能力，直接回傳副本即可。所以對 const 容器
//         寫 auto x = cv[0]; 得到的是實實在在的 bool，完全沒有陷阱。
//     追問：這代表什麼實務含意？→ 同一段程式碼，容器是不是 const 會讓 auto
//         推出完全不同的型別。所以「不要用 auto 接 vector<bool> 元素」必須是
//         無條件守則，不能靠個案判斷。
//
// ⚠️ 陷阱. 「sizeof(vb[0]) 是 16，所以 vector<bool> 每個元素佔 16 bytes，超浪費。」
//     答：完全搞反了。16 是 proxy「這個暫存物件」的大小，不是儲存成本。
//         實際儲存是 1 bit/元素，1000 個 bool 的 payload 約 128 bytes。
//     為什麼會錯：習慣了 sizeof(v[0]) == sizeof(T) 這個對所有正常容器都成立的
//         等式，忘了 vector<bool> 的 operator[] 根本不回傳 T，
//         sizeof 量到的是代理人，不是本人。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <typeinfo>
#include <type_traits>
#include <string>

#if defined(__GNUG__)
#include <cxxabi.h>
#include <cstdlib>
#include <memory>
// GCC/Clang 專屬：把 mangled name 還原成人類可讀的型別名稱（非標準擴充）
static std::string demangle(const char* mangled) {
    int status = 0;
    std::unique_ptr<char, void (*)(void*)> p(
        abi::__cxa_demangle(mangled, nullptr, nullptr, &status), std::free);
    return (status == 0 && p) ? std::string(p.get()) : std::string(mangled);
}
#else
static std::string demangle(const char* mangled) { return std::string(mangled); }
#endif

int main() {
    std::vector<int> vi = {1, 2, 3};
    std::vector<bool> vb = {true, false, true};

    std::cout << "=== 原始範例：typeid 觀察 ===" << std::endl;

    // vector<int>::operator[] 回傳 int&
    auto ri = vi[0];   // auto 推導為 int
    std::cout << "vi[0] 的型別：" << typeid(ri).name() << std::endl;

    // vector<bool>::operator[] 回傳 reference（代理物件）
    auto rb = vb[0];   // auto 推導為 vector<bool>::reference，不是 bool！
    std::cout << "vb[0] 的型別：" << typeid(rb).name() << std::endl;

    // 大小也不同
    std::cout << "sizeof(ri) = " << sizeof(ri) << std::endl;
    std::cout << "sizeof(rb) = " << sizeof(rb) << std::endl;

    // -------------------------------------------------------------------------
    // 把 mangled name 還原成可讀名稱
    // -------------------------------------------------------------------------
    std::cout << "\n=== demangle 之後（可讀名稱）===" << std::endl;
    std::cout << "vi[0] -> " << demangle(typeid(ri).name()) << std::endl;
    std::cout << "vb[0] -> " << demangle(typeid(rb).name())
              << "   <- 這就是 proxy，不是 bool" << std::endl;
    std::cout << "說明：St14_Bit_reference 是 Itanium ABI 的 mangled name，"
              << std::endl;
    std::cout << "      St = std::，14 = 後面識別字長度，_Bit_reference 剛好 14 字元"
              << std::endl;

    // -------------------------------------------------------------------------
    // 用 type traits 做編譯期的鐵證（比 typeid 更可靠）
    // -------------------------------------------------------------------------
    std::cout << "\n=== 編譯期型別檢查（type traits）===" << std::endl;
    std::cout << std::boolalpha;
    std::cout << "is_same<decltype(vi[0]), int&>            = "
              << std::is_same<decltype(vi[0]), int&>::value << std::endl;
    std::cout << "is_same<decltype(vb[0]), bool&>           = "
              << std::is_same<decltype(vb[0]), bool&>::value
              << "   <- 不是 bool&！" << std::endl;
    std::cout << "is_same<decltype(vb[0]),"
              << " vector<bool>::reference> = "
              << std::is_same<decltype(vb[0]), std::vector<bool>::reference>::value
              << std::endl;
    std::cout << "is_same<vector<bool>::value_type, bool>   = "
              << std::is_same<std::vector<bool>::value_type, bool>::value
              << "   <- value_type 倒是老實的 bool" << std::endl;

    // -------------------------------------------------------------------------
    // const 版本：const_reference 是純量 bool（標準規定）
    // -------------------------------------------------------------------------
    std::cout << "\n=== const 容器沒有 proxy 問題（標準規定）===" << std::endl;
    const std::vector<bool> cv = {true, false, true};
    auto cx = cv[0];                       // 型別是 bool，不是 proxy！
    std::cout << "is_same<vector<bool>::const_reference, bool> = "
              << std::is_same<std::vector<bool>::const_reference, bool>::value
              << std::endl;
    std::cout << "const 容器 auto cx = cv[0] 的型別 = "
              << demangle(typeid(cx).name())
              << "，sizeof = " << sizeof(cx) << std::endl;
    std::cout << "非 const auto rb = vb[0] 的型別   = "
              << demangle(typeid(rb).name())
              << "，sizeof = " << sizeof(rb) << std::endl;
    std::cout << "同樣寫 auto，加不加 const 推出完全不同的型別！" << std::endl;

    // -------------------------------------------------------------------------
    // proxy 內部就是「word 指標 + bit 遮罩」
    // -------------------------------------------------------------------------
    std::cout << "\n=== 為什麼 sizeof(proxy) = 16（實作定義）===" << std::endl;
    std::cout << "proxy 概念上有兩個欄位：" << std::endl;
    std::cout << "  word 指標 sizeof = " << sizeof(unsigned long*) << " bytes" << std::endl;
    std::cout << "  bit 遮罩  sizeof = " << sizeof(unsigned long)  << " bytes" << std::endl;
    std::cout << "  合計 " << sizeof(unsigned long*) + sizeof(unsigned long)
              << " bytes = 實測的 sizeof(vb[0])" << std::endl;
    std::cout << "但這是編譯期抽象成本；實際儲存仍是 1 bit/元素" << std::endl;

    // -------------------------------------------------------------------------
    // proxy 確實「連著」容器：透過它寫入會改到原容器
    // -------------------------------------------------------------------------
    std::cout << "\n=== proxy 是活的：寫 proxy 就是寫容器 ===" << std::endl;
    std::cout << "寫入前 vb[0] = " << vb[0] << std::endl;
    rb = false;                            // rb 是 proxy，這行直接改到 vb 內部的 bit
    std::cout << "對 proxy 寫入 false 後 vb[0] = " << vb[0]
              << "   <- 原容器被改了" << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 18 課：vectorbool 的特殊性與陷阱3.cpp" -o vb3

// === 預期輸出 ===
// === 原始範例：typeid 觀察 ===
// vi[0] 的型別：i
// vb[0] 的型別：St14_Bit_reference
// sizeof(ri) = 4
// sizeof(rb) = 16
//
// === demangle 之後（可讀名稱）===
// vi[0] -> int
// vb[0] -> std::_Bit_reference   <- 這就是 proxy，不是 bool
// 說明：St14_Bit_reference 是 Itanium ABI 的 mangled name，
//       St = std::，14 = 後面識別字長度，_Bit_reference 剛好 14 字元
//
// === 編譯期型別檢查（type traits）===
// is_same<decltype(vi[0]), int&>            = true
// is_same<decltype(vb[0]), bool&>           = false   <- 不是 bool&！
// is_same<decltype(vb[0]), vector<bool>::reference> = true
// is_same<vector<bool>::value_type, bool>   = true   <- value_type 倒是老實的 bool
//
// === const 容器沒有 proxy 問題（標準規定）===
// is_same<vector<bool>::const_reference, bool> = true
// const 容器 auto cx = cv[0] 的型別 = bool，sizeof = 1
// 非 const auto rb = vb[0] 的型別   = std::_Bit_reference，sizeof = 16
// 同樣寫 auto，加不加 const 推出完全不同的型別！
//
// === 為什麼 sizeof(proxy) = 16（實作定義）===
// proxy 概念上有兩個欄位：
//   word 指標 sizeof = 8 bytes
//   bit 遮罩  sizeof = 8 bytes
//   合計 16 bytes = 實測的 sizeof(vb[0])
// 但這是編譯期抽象成本；實際儲存仍是 1 bit/元素
//
// === proxy 是活的：寫 proxy 就是寫容器 ===
// 寫入前 vb[0] = true
// 對 proxy 寫入 false 後 vb[0] = false   <- 原容器被改了
