// =============================================================================
//  第 2.7 章 範例 11  —  陷阱：位元欄位（bit-field）無法被完美轉發
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<iostream>、<utility>
//   問題形式：
//       struct Flags { unsigned int readable : 1; };
//       template<class T> void wrapper(T&&);
//       wrapper(f.readable);      // ✗ 編譯錯誤
//   本機 g++ 15.2 實測的錯誤訊息：
//       error: cannot bind bit-field 'f.Flags::readable' to 'unsigned int&'
//   解法：先複製到具名變數，或用一元 + 產生 prvalue。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼位元欄位「沒有位址」】
//   C++ 的定址單位是 byte，最小的可取址物件就是一個 byte。
//   位元欄位刻意讓多個成員共用同一個 byte——本檔的 Flags 三個成員
//   總共只佔 3 個 bit，全部擠在同一個 byte 裡。
//   於是 &f.readable 這件事在語言層面根本無從定義：它沒有屬於自己的位址。
//   標準因此明文禁止對位元欄位取址。
//
// 【2. 為什麼「不能取址」會導致「不能完美轉發」】
//   轉發參考 T&& 在傳入左值時會推導成 unsigned int&，
//   而參考在實作上就是「被綁定物件的位址」。
//   要建立一個 unsigned int& 就必須有位址可綁 → 位元欄位提供不了 → 編譯錯誤。
//   注意錯誤發生在「建立參考」這一步，跟 std::forward 完全無關；
//   就算函式本體是空的（如本檔原始版本），照樣編譯不過。
//
// 【3. 關鍵區別：const 參考「可以」綁定位元欄位】
//   void takeConstRef(const unsigned int& v);
//   takeConstRef(f.readable);        // ✓ 可以編譯，本機實測通過
//   原因是：const 左值參考允許綁定到「臨時物件」。
//   編譯器會先把位元欄位的值複製到一個臨時的 unsigned int，
//   再讓 const& 綁到那個臨時物件上。你拿到的是副本的位址，不是欄位的位址。
//   所以規則要精確記成：
//       位元欄位 → 非 const 參考：✗（沒有位址可綁）
//       位元欄位 → const 參考：  ✓（綁到隱含產生的臨時副本）
//       位元欄位 → 傳值：        ✓（本來就只需要值）
//   而完美轉發傳左值時要的正是「非 const 參考」，所以撞牆。
//
// 【4. 兩種解法】
//   解法一：先複製到具名變數
//       unsigned int r = f.readable;  wrapper(r);
//       r 是有位址的普通變數，一切正常。意圖也最清楚。
//   解法二：一元 + 產生 prvalue
//       wrapper(+f.readable);
//       一元 + 會做整數提升，產生一個「純右值」臨時物件。
//       T 於是推導成 unsigned int（不是參考），走 T&& = unsigned int&&，
//       綁到臨時物件成立。寫法簡潔但較隱晦，團隊裡建議加註解說明。
//
// 【概念補充 Concept Deep Dive】
//   (A) 位元欄位的實際佈局是實作定義的：位元的排列順序（由低位還是高位開始）、
//       跨越 byte 邊界時的行為、以及 sizeof(Flags) 的結果，都由 ABI 決定。
//       本機（x86-64 System V ABI，g++ 15.2）上 sizeof(Flags) 為 4——
//       這是實作定義的值，不是標準保證，程式不可依賴它。
//   (B) 位元欄位也不能有非靜態成員參考指向它，不能用於 std::atomic，
//       也不能取 sizeof。它在語言中是相當受限的存在。
//   (C) 為什麼還要用位元欄位？硬體暫存器映射、網路封包標頭、
//       極度在意記憶體的巨大陣列。這些場景下省下來的空間是真的值得。
//
// 【注意事項 Pay Attention】
//   1. 不能對位元欄位取址，也不能綁定非 const 參考。
//   2. const 參考可以綁，但綁的是臨時副本——改它不會影響原欄位。
//   3. 一元 + 會做整數提升，型別可能從 unsigned int 變成 int，
//      在意型別精確性時應改用解法一。
//   4. sizeof(Flags) 與位元排列順序都是實作定義，跨平台傳輸請勿直接
//      memcpy 整個結構，要自己做序列化。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】位元欄位與完美轉發
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 wrapper(f.readable) 無法通過編譯？
//     答：轉發參考對左值會推導成 unsigned int&，而建立參考需要位址；
//         位元欄位與其他成員共用同一個 byte，語言層面沒有屬於自己的位址，
//         標準禁止對它取址。本機錯誤訊息是
//         "cannot bind bit-field 'f.Flags::readable' to 'unsigned int&'"。
//     追問：這跟 std::forward 有關嗎？
//         → 完全無關。錯誤發生在「綁定參考」這一步，函式本體空的照樣失敗。
//
// 🔥 Q2. 有哪兩種解法？各自的取捨是什麼？
//     答：(1) 先複製到具名變數 unsigned int r = f.readable; wrapper(r);
//             ——意圖最清楚，型別完全可控，推薦。
//         (2) 用一元 + 產生 prvalue：wrapper(+f.readable);
//             ——簡潔，但會觸發整數提升可能改變型別，且寫法隱晦需加註解。
//
// ⚠️ 陷阱. 「位元欄位不能綁定參考」——為什麼這句話是錯的？
//     答：不能綁的只有「非 const 參考」。const 左值參考可以綁，
//         因為它允許綁定臨時物件：編譯器會先把欄位值複製到一個臨時的
//         unsigned int，再讓 const& 綁上去（本機已實測通過）。
//         但要注意你拿到的是副本的位址，透過它看不到原欄位後續的變化。
//     為什麼會錯：大家把「取址失敗」直接推廣成「所有參考都不行」，
//         忽略了 const 左值參考有「延長臨時物件壽命」這個特殊能力——
//         它綁的根本不是原物件，所以位元欄位沒有位址這件事不構成障礙。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <utility>

struct Flags {
    unsigned int readable : 1;
    unsigned int writable : 1;
    unsigned int executable : 1;
};

// 真的把收到的值印出來，讓兩種解法的效果看得見
template<typename T>
void wrapper(T&& arg) {
    std::cout << "    wrapper 收到值 = " << arg << "\n";
}

// const 參考可以綁定位元欄位（綁的是隱含產生的臨時副本）
void takeConstRef(const unsigned int& v) {
    std::cout << "    const& 收到值 = " << v << "（綁定的是臨時副本）\n";
}

// -----------------------------------------------------------------------------
// 【日常實務範例】解析封包標頭的旗標位元
//   情境：網路協定與檔案格式的標頭常用位元欄位來省空間
//         （例如 TCP 標頭的各種控制旗標）。
//         把這些旗標交給任何泛型介面（日誌、序列化、轉發包裝器）時，
//         就會踩到本檔的陷阱——實務上的固定做法是「先讀進具名變數」。
//   這裡用一個簡化的封包標頭示範。
// -----------------------------------------------------------------------------
struct PacketHeader {
    unsigned int version   : 4;   // 協定版本（0-15）
    unsigned int encrypted : 1;   // 是否加密
    unsigned int compressed: 1;   // 是否壓縮
    unsigned int priority  : 2;   // 優先權（0-3）
};

template<typename T>
void logField(const char* name, T&& value) {
    std::cout << "    " << name << " = " << value << "\n";
}

void dumpHeader(const PacketHeader& h) {
    // ✗ logField("version", h.version);  // 位元欄位無法綁定非 const 參考
    // ✓ 先複製到具名變數再交給泛型介面
    unsigned int version    = h.version;
    unsigned int encrypted  = h.encrypted;
    unsigned int compressed = h.compressed;
    unsigned int priority   = h.priority;

    logField("version",    version);
    logField("encrypted",  encrypted);
    logField("compressed", compressed);
    logField("priority",   priority);
}

int main() {
    Flags f = {1, 1, 0};

    // wrapper(f.readable);
    // 錯誤！位元欄位不能取址，不能綁定到非 const 參考
    // T&& 推導為 unsigned int&，需要位址 → 失敗
    // 本機實測訊息：cannot bind bit-field 'f.Flags::readable' to 'unsigned int&'

    std::cout << "=== 解法 1：先複製到具名變數 ===\n";
    unsigned int r = f.readable;
    wrapper(r);              // OK

    std::cout << "\n=== 解法 2：一元 + 產生 prvalue ===\n";
    wrapper(+f.readable);    // OK：一元 + 運算子產生 prvalue

    std::cout << "\n=== 對照：const 參考可以綁定位元欄位 ===\n";
    takeConstRef(f.readable);   // OK：綁到隱含產生的臨時副本

    std::cout << "\n=== 日常實務：解析封包標頭旗標 ===\n";
    PacketHeader h = {2, 1, 0, 3};
    dumpHeader(h);
    std::cout << "  （sizeof(PacketHeader) 為實作定義，本機 x86-64 為 "
              << sizeof(PacketHeader) << " bytes）\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 2.7 章：完美轉發 (Perfect Forwarding) — 泛型程式設計的關鍵技術11.cpp" -o bitfield_fwd

// 註：本檔未附 LeetCode 範例。位元欄位屬於系統程式與硬體介面的領域，
//     LeetCode 題目不會用到；硬套一題只會失真。
//     位元運算主題的題目（如 191、338）用的是整數位元操作，不是位元欄位。
//
// 註：sizeof(PacketHeader)、位元排列順序皆為「實作定義」，
//     下方輸出中的數值為本機 x86-64 System V ABI + g++ 15.2 的結果，
//     不是標準保證的值。

// === 預期輸出 ===
// === 解法 1：先複製到具名變數 ===
//     wrapper 收到值 = 1
//
// === 解法 2：一元 + 產生 prvalue ===
//     wrapper 收到值 = 1
//
// === 對照：const 參考可以綁定位元欄位 ===
//     const& 收到值 = 1（綁定的是臨時副本）
//
// === 日常實務：解析封包標頭旗標 ===
//     version = 2
//     encrypted = 1
//     compressed = 0
//     priority = 3
//   （sizeof(PacketHeader) 為實作定義，本機 x86-64 為 4 bytes）
