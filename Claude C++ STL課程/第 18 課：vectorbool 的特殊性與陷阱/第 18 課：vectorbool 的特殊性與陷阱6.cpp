// =============================================================================
//  第 18 課-6：陷阱三 —— 為什麼 bool& ref = vb[0]; 綁不起來
// =============================================================================
//
// 【主題資訊 Information】
//   vector<bool>::reference operator[](size_type n);   // 代理物件（右值）
//   對照：vector<T>::operator[] 回傳 T&，可直接綁到 T& 或傳給吃 T& 的函式
//   關鍵語言規則：非 const 左值引用（T&）不能綁定到右值，
//                 也不能綁定到「型別不同、需要轉換」的物件
//   標準版本：C++98 起（[vector.bool]）
//   標頭檔：<vector>
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼 bool& 綁不到代理物件 —— 兩層原因疊加】
//   (a) 型別根本不同。vb[0] 的型別是 vector<bool>::reference，
//       不是 bool。bool& 只能綁到 bool 型別的左值。
//   (b) 就算透過 operator bool() 轉換，那也只是產生一個
//       「暫時的 bool 右值」。而非 const 的 bool& 不能綁到右值——
//       這是 C++ 從一開始就有的規則，用意正是防止你
//       「修改一個馬上就要消失的暫時物件」。
//   所以 bool& ref = vb[0]; 在兩個層次上都不成立。
//
// 【2. 但 const bool& 可以綁 —— 而且這件事更危險】
//   const bool& cref = vb[0];   // 這行編譯得過！
//   因為 const 左值引用可以綁定到右值，而且會把該暫時物件的
//   生命週期延長到與引用一樣長。
//   問題是：它綁的是「轉換後產生的那個 bool 副本」，
//   而不是容器裡的那個 bit。之後修改 vb[0]，cref 完全不會跟著變。
//   看起來像引用、行為卻像副本——這比直接編譯失敗更難察覺。
//
// 【3. 傳參數時同樣會爆】
//   void set_true(bool& b);
//   set_true(vb[0]);            // 編譯錯誤，理由同上
//   這在真實程式碼中最常出現：一個接受 bool& 的既有函式，
//   對 vector<int>、vector<char> 都能用，唯獨對 vector<bool> 不行。
//   改成 void set_true(bool b) 又失去了「回寫」的能力。
//
// 【4. 正確的做法有三種】
//   (a) 直接指派：vb[0] = true;
//       代理物件本身有 operator=(bool)，會把值寫回那個 bit。
//   (b) 用 auto&& 接住代理：auto&& r = vb[0]; r = true;
//       auto&& 會推導成 vector<bool>::reference（代理物件本身），
//       透過它賦值確實會寫回容器。
//   (c) 改用 vector<uint8_t> 或 deque<bool>——
//       兩者的 operator[] 都回傳真正的引用，行為完全正常。
//
// 【5. 這才是「破壞泛型」的真正代價】
//   一段對所有 vector<T> 都成立的模板程式碼，
//   只因為 T 剛好是 bool 就編不過——而且錯誤訊息通常出現在
//   模板內部，離真正的呼叫點很遠，非常難讀。
//   這是 vector<bool> 被列為設計失誤的核心理由：
//   它違反了「相同模板、相同介面語意」這個最基本的期待。
//
// 【概念補充 Concept Deep Dive】
//   ▸ 為什麼 auto&& 可以、bool& 不行
//     auto&& 是轉發引用，會依初始化運算式推導出恰當的型別——
//     這裡推導成 vector<bool>::reference（而非 bool），
//     所以綁的是代理物件本身，透過它賦值會真正寫回那個 bit。
//     bool& 則是寫死了型別，強迫發生轉換，也就強迫產生了暫時物件。
//   ▸ 代理物件的 operator= 才是真正的寫入點
//     reference& operator=(bool b) 內部大致是
//       if (b) *word_ |=  mask_;
//       else   *word_ &= ~mask_;
//     也就是對那個字組做位元運算。這正是「單一 bit 沒有位址、
//     但仍然可以被讀寫」的實現方式。
//   ▸ 多執行緒下的額外陷阱
//     因為相鄰的 bool 共用同一個字組，兩個執行緒
//     分別寫 vb[0] 與 vb[1] 會對「同一個 word」做讀改寫，
//     形成 data race。對 vector<int> 寫不同元素則完全安全。
//     這是 vector<bool> 少被提及、卻很致命的一點。
//
// 【注意事項 Pay Attention】
//   1. bool& 無法綁定 vb[i]；接受 bool& 的函式也不能傳 vb[i]。
//   2. const bool& 綁得起來，但綁的是轉換後的副本，不會跟著容器變動。
//   3. 想修改請直接寫 vb[i] = value;，或用 auto&& 接住代理物件。
//   4. 不要用 auto val = vb[i];——那會得到代理，之後容器一變 val 也跟著變。
//   5. 相鄰元素共用同一個字組，多執行緒分別寫不同元素也會有 data race。
//   6. 需要真正的引用語意，請改用 vector<uint8_t> 或 deque<bool>。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】vector<bool> 與引用綁定
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 bool& ref = vb[0]; 編譯不過？
//     答：兩個原因疊加。第一，vb[0] 的型別是 vector<bool>::reference
//         而不是 bool，型別就對不上；第二，就算透過 operator bool()
//         轉換，得到的也只是一個暫時的 bool 右值，
//         而非 const 的左值引用不能綁定到右值。
//     追問：那 const bool& 為什麼可以？
//         → const 左值引用允許綁定右值並延長其生命週期。
//           但它綁的是「轉換後的那份 bool 副本」，不是容器裡的 bit，
//           之後修改 vb[0] 它完全不會變——這比編譯失敗更難察覺。
//
// 🔥 Q2. 那要怎麼樣才能透過一個變數去修改 vector<bool> 的元素？
//     答：用 auto&& 接住代理物件本身：
//           auto&& r = vb[0];  r = true;    // 確實寫回容器
//         auto&& 會推導成 vector<bool>::reference，
//         而代理物件的 operator=(bool) 會對底層字組做位元運算寫回那個 bit。
//         最直接的做法當然是 vb[0] = true;。
//     追問：那 auto r = vb[0]; 呢？
//         → 同樣會得到代理物件（不是 bool 副本），
//           所以 r 會「跟著」vb[0] 變動，這通常不是你要的。
//           要取獨立副本必須明確寫 bool r = vb[0];。
//
// ⚠️ 陷阱. 一個現成的函式 void toggle(bool& b) { b = !b; }
//          對 vector<char>、vector<int> 都能配合使用，
//          為什麼一換成 vector<bool> 整個模板就編不過？
//     答：因為 vector<bool> 是標準規定的特化，operator[] 回傳代理物件
//         而非 bool&，無法傳給吃 bool& 的參數。
//         這正是 vector<bool> 破壞泛型契約的具體後果——
//         而且錯誤訊息通常出現在模板內部，離呼叫點很遠。
//     為什麼會錯：預期 vector<T> 對所有 T 的介面語意一致。
//         這個預期在 C++ 中幾乎總是成立，唯獨 bool 是被標準
//         挖出來的例外。所以撰寫可能被 bool 實例化的模板時，
//         必須額外考慮這個特化——或乾脆用 vector<uint8_t> 避開。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <vector>

void set_true(bool& b) {
    b = true;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】排班表：把「某人某時段是否有空」交給既有的工具函式修改
//   情境：既有的排程模組提供 void reserveSlot(bool& slot)，
//         用來把某個時段標記為已佔用。現在要接上一份大型的可用性表格。
//   為什麼用本主題：這是最真實的踩雷方式——
//         既有函式的介面是 bool&，你為了省記憶體選了 vector<bool>，
//         結果整個接不起來。範例呈現兩種可行的修法，
//         並說明為什麼 vector<uint8_t> 通常是比較省事的選擇。
// -----------------------------------------------------------------------------

// 既有模組的介面（不能改），只吃 bool&
void reserveSlot(bool& slot) {
    slot = true;
}

// 做法 A：改用 vector<uint8_t>，但既有函式吃 bool&，仍需一個轉接
//         這裡改用 deque<bool>（見 10.cpp）或直接用 bool 陣列都可以。
//         最務實的是讓儲存型別本身就提供真引用：
void reserveWithByteVector(std::vector<unsigned char>& slots, std::size_t idx) {
    bool tmp = (slots[idx] != 0);
    reserveSlot(tmp);                 // 既有函式照樣可用
    slots[idx] = tmp ? 1u : 0u;       // 明確寫回
}

// 做法 B：維持 vector<bool>，但放棄「傳引用給既有函式」，改用代理直接賦值
void reserveWithBitVector(std::vector<bool>& slots, std::size_t idx) {
    // reserveSlot(slots[idx]);       // 編譯錯誤！代理物件無法綁到 bool&
    auto&& proxy = slots[idx];        // auto&& 接住代理物件本身
    proxy = true;                     // 代理的 operator=(bool) 會寫回那個 bit
}

int main() {
    std::cout << std::boolalpha;

    std::cout << "=== 一、vector<int> 可以取得真正的引用 ===" << std::endl;
    std::vector<int> vi = {0, 0, 0};
    int& ref = vi[0];  // OK
    ref = 42;
    std::cout << "透過 int& 修改後 vi[0] = " << vi[0] << std::endl;

    std::cout << "\n=== 二、vector<bool> 不行（以下皆為編譯錯誤，故註解）===" << std::endl;
    std::vector<bool> vb = {false, false, false};
    // bool& ref_b = vb[0];   // 編譯錯誤！代理物件不是 bool，也不是左值
    // set_true(vb[0]);       // 編譯錯誤！函式參數是 bool&，不接受代理物件
    std::cout << "以上兩種寫法都被編譯器擋下（這是保護，不是刁難）" << std::endl;

    std::cout << "\n=== 三、const bool& 綁得起來，但綁到的是副本 ===" << std::endl;
    vb[0] = true;
    const bool& cref = vb[0];      // 合法：const 左值引用可綁右值並延長其壽命
    std::cout << "綁定當下 cref = " << cref << std::endl;
    vb[0] = false;                 // 修改容器
    std::cout << "把 vb[0] 改成 false 之後：" << std::endl;
    std::cout << "  vb[0] = " << static_cast<bool>(vb[0]) << std::endl;
    std::cout << "  cref  = " << cref << "  ← 沒有跟著變！它是副本不是引用" << std::endl;

    std::cout << "\n=== 四、正確的修改方式 ===" << std::endl;
    std::vector<bool> w = {false, false, false};
    w[0] = true;                          // 方式一：直接指派（最簡單）
    auto&& proxy = w[1];                  // 方式二：auto&& 接住代理物件
    proxy = true;
    std::cout << "w = ";
    for (std::size_t i = 0; i < w.size(); ++i) std::cout << static_cast<bool>(w[i]) << " ";
    std::cout << std::endl;

    std::cout << "\n=== 五、auto 與 bool 的差別（陷阱一的預習）===" << std::endl;
    std::vector<bool> t = {true, false};
    auto  a = t[0];    // a 是代理物件，會跟著容器變
    bool  b = t[0];    // b 是獨立的 bool 副本
    t[0] = false;
    std::cout << "把 t[0] 改成 false 之後： auto a = " << static_cast<bool>(a)
              << "（跟著變了）， bool b = " << b << "（沒變）" << std::endl;

    std::cout << "\n=== 六、日常實務：把可用性表格接上既有的 bool& 介面 ===" << std::endl;
    std::vector<unsigned char> byteSlots(8, 0);
    reserveWithByteVector(byteSlots, 3);
    std::cout << "做法A vector<uint8_t>：";
    for (unsigned char s : byteSlots) std::cout << static_cast<int>(s);
    std::cout << std::endl;

    std::vector<bool> bitSlots(8, false);
    reserveWithBitVector(bitSlots, 3);
    std::cout << "做法B vector<bool>   ：";
    for (std::size_t i = 0; i < bitSlots.size(); ++i)
        std::cout << (bitSlots[i] ? 1 : 0);
    std::cout << std::endl;
    std::cout << "兩者結果相同，但 A 能直接沿用既有的 bool& 介面，B 必須改寫呼叫端"
              << std::endl;

    // set_true 在本檔僅用於示範「不能傳 vb[0]」，這裡對一般 bool 呼叫一次
    bool plain = false;
    set_true(plain);
    std::cout << "（對照）對一般 bool 呼叫 set_true → " << plain << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 18 課：vectorbool 的特殊性與陷阱6.cpp" -o vb_no_ref
//
// 【本檔未附 LeetCode 範例的理由】
//   本檔談的是「引用綁定規則」與「模板特化破壞泛型契約」，
//   屬於 C++ 型別系統與 API 設計層面的議題。
//   LeetCode 的題目不會出現「把容器元素傳給吃 T& 的既有函式」這種介面約束，
//   硬套一題無法呈現本主題的價值，因此改以排班表接既有介面的實務範例呈現。

// === 預期輸出 ===
// === 一、vector<int> 可以取得真正的引用 ===
// 透過 int& 修改後 vi[0] = 42
//
// === 二、vector<bool> 不行（以下皆為編譯錯誤，故註解）===
// 以上兩種寫法都被編譯器擋下（這是保護，不是刁難）
//
// === 三、const bool& 綁得起來，但綁到的是副本 ===
// 綁定當下 cref = true
// 把 vb[0] 改成 false 之後：
//   vb[0] = false
//   cref  = true  ← 沒有跟著變！它是副本不是引用
//
// === 四、正確的修改方式 ===
// w = true true false
//
// === 五、auto 與 bool 的差別（陷阱一的預習）===
// 把 t[0] 改成 false 之後： auto a = false（跟著變了）， bool b = true（沒變）
//
// === 六、日常實務：把可用性表格接上既有的 bool& 介面 ===
// 做法A vector<uint8_t>：00010000
// 做法B vector<bool>   ：00010000
// 兩者結果相同，但 A 能直接沿用既有的 bool& 介面，B 必須改寫呼叫端
// （對照）對一般 bool 呼叫 set_true → true
