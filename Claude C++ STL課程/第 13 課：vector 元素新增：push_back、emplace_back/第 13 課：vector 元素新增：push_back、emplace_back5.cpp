// =============================================================================
//  第 13 課：vector 元素新增：push_back、emplace_back5.cpp
//    —  emplace_back 的回傳值：C++17 是分水嶺
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<vector>
//
//   C++11 / C++14：
//       template<class... Args> void      emplace_back(Args&&... args);
//   C++17 起（P0084R2）：
//       template<class... Args> reference emplace_back(Args&&... args);
//
//   push_back 從頭到尾都是 void，從來沒有改過。
//   複雜度：攤銷 O(1)
//
//   ★ 本檔需要 C++17 才能編譯（用到了回傳值）。
//     本機以 g++ 15.2.0 實測驗證：
//       g++ -std=c++17 -pedantic-errors → 通過
//       g++ -std=c++14 -pedantic-errors → 編譯失敗，實測診斷訊息為
//           error: invalid use of ‘void’
//           error: invalid initialization of non-const reference of type
//                  ‘CartLine&’ from an rvalue of type ‘void’
//       （若改寫成 auto& r = v.emplace_back(x)，C++14 下的訊息則是
//         error: forming reference to void；措辭依寫法而異，
//         根本原因都是「C++14 的 emplace_back 回傳 void」。）
//     （注意：只用 -fsyntax-only 驗證會被 GCC 當擴充放行，得到錯誤結論；
//       一定要真的編譯，並加上 -pedantic-errors。）
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼 C++11 當初設計成 void】
// emplace_back 在 C++11 加入時沿用了 push_back 的慣例回傳 void。
// 結果是「插入之後想立刻操作那個新元素」變得很囉嗦：
//     v.emplace_back("Apple", 10);
//     v.back().quantity += 5;        // 得再呼叫一次 back()
// back() 雖然是 O(1)，但這個寫法有兩個問題：
//   * 多一次不必要的呼叫，也多一次「這中間 vector 有沒有被別人動過」的疑慮。
//   * 語意上斷成兩句，讀者要自己確認 back() 指的就是剛剛插入的那個。
// C++17 的 P0084R2 提案因此讓 emplace_back 回傳新元素的 reference。
//
// 【2. 回傳值帶來的兩種寫法】
//   (a) 接住參考，之後繼續用：
//         Item& apple = items.emplace_back("Apple", 10);
//         apple.quantity += 5;
//   (b) 鏈式操作，一行完成：
//         items.emplace_back("Banana", 20).quantity *= 2;
//   push_back 因為回傳 void，這兩種寫法都做不到，只能退回 back()。
//
// 【3. 這個 reference 的有效期限】
// 回傳的 reference 指向 vector 內部的元素，因此適用一般的失效規則：
//     * 下一次 push_back / emplace_back 若觸發 reallocation → **立刻失效**
//     * insert / erase / resize / clear 等也可能使其失效
// 本檔先 reserve(3) 再插入 2 個，保證過程中不會擴容，所以 apple 這個參考
// 全程有效。真實程式碼裡若要長期保存這個參考，必須自己確保不會擴容
// （例如事先 reserve 足夠空間），否則就會變成懸空參考。
//
// 【概念補充 Concept Deep Dive】
// 為什麼回傳 reference 而不是 iterator？
// 因為 emplace_back 插入的位置永遠是尾端，位置資訊本身沒有價值——
// 你要的是「那個物件」而不是「它在哪」。回傳 reference 讓
// `.quantity *= 2` 這種鏈式寫法自然成立；若回傳 iterator，
// 每次都得多寫一個 `->` 或 `*`。
// 對照 insert() 就回傳 iterator，因為插入位置是任意的，位置資訊有意義。
//
// 另外注意 reference 這個型別名稱：它是 vector 的巢狀 typedef，
// 對 vector<T> 而言就是 T&（更精確地說是 allocator_traits 定義的
// value_type&）。寫 auto& 接住即可，不需要拼出完整型別。
//
// 【注意事項 Pay Attention】
// 1. 回傳 reference 是 C++17 起才有的行為；C++11/14 回傳 void，
//    寫 auto& r = v.emplace_back(...) 會編譯失敗（forming reference to void）。
// 2. push_back 永遠回傳 void，不能鏈式呼叫。
// 3. 回傳的 reference 會被後續的 reallocation 作廢；要長期保存必須先確保
//    capacity 足夠。
// 4. 驗證標準版本差異時要真的編譯並加 -pedantic-errors，
//    -fsyntax-only 會被 GCC 當擴充放行。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】emplace_back 的回傳值
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. emplace_back 回傳什麼？push_back 呢？
//     答：emplace_back 在 **C++17 起**回傳 reference（指向剛建構的新元素），
//         在 C++11 / C++14 回傳 void。push_back 從頭到尾都回傳 void。
//         本機以 g++ -std=c++14 -pedantic-errors 實測，
//         寫 auto& r = v.emplace_back(x) 會得到 "forming reference to void"；
//         改成 -std=c++17 即通過。
//     追問：為什麼回傳 reference 而不是 iterator？
//         → 插入位置固定在尾端，位置資訊沒有價值；回傳物件本身才能寫出
//           v.emplace_back(...).member = x 這種鏈式操作。
//
// 🔥 Q2. C++17 之前想在插入後立刻修改新元素，該怎麼寫？
//     答：用 v.back()：
//             v.emplace_back("Apple", 10);
//             v.back().quantity += 5;
//         back() 是 O(1)，功能上等價，只是語意斷成兩句、多一次呼叫。
//     追問：這兩種寫法有什麼實質差別嗎？
//         → 效能上幾乎沒有。差別在可讀性與安全性：回傳值版本讓「這就是
//           剛剛插入的那個元素」在語法上不言自明。
//
// ⚠️ 陷阱 Q3. Item& r = v.emplace_back(...); 之後再 emplace_back，r 還能用嗎？
//     答：不一定。若第二次插入觸發了 reallocation，所有元素會被搬到新記憶體，
//         r 就成了指向已釋放記憶體的懸空參考，再使用是未定義行為。
//         只有在「確定不會擴容」時（例如事先 reserve 足夠空間，
//         如本檔的 reserve(3)）r 才保持有效。
//     為什麼會錯：把回傳的 reference 當成獨立於容器的物件。
//         它本質上只是「vector 內部某個位址」的別名，
//         容器一搬家，這個別名就失效了。
// ═══════════════════════════════════════════════════════════════════════════

#include <vector>
#include <iostream>
#include <string>

struct Item {
    std::string name;
    int quantity;

    Item(const std::string& n, int q) : name(n), quantity(q) {}
};

// -----------------------------------------------------------------------------
// 【日常實務範例】購物車：加入品項後立刻套用促銷規則
//   情境：電商後端把品項加進購物車，加入的同時要依規則調整數量
//         （買一送一、批發加倍等）。
//   為什麼用回傳值：加入與調整是同一個業務動作，用 C++17 的回傳 reference
//                   可以一行寫完，不必先 emplace_back 再 back()，
//                   也就不會有「back() 是不是我剛加的那個」的疑慮。
// -----------------------------------------------------------------------------
struct CartLine {
    std::string sku;
    int qty;
    CartLine(const std::string& s, int q) : sku(s), qty(q) {}
};

std::vector<CartLine> buildCart() {
    std::vector<CartLine> cart;
    cart.reserve(3);                     // 先 reserve，保證回傳的參考不失效

    // 一般品項：加入後不調整
    cart.emplace_back("SKU-1001", 2);

    // 買一送一：加入後立刻加倍（鏈式操作）
    cart.emplace_back("SKU-2002", 3).qty *= 2;

    // 批發品項：接住參考，套用比較複雜的規則
    CartLine& bulk = cart.emplace_back("SKU-3003", 10);
    if (bulk.qty >= 10) {
        bulk.qty += 1;                   // 滿 10 送 1
    }
    return cart;
}

int main() {
    std::vector<Item> items;
    items.reserve(3);   // 保證後續不會擴容，回傳的參考才會一直有效

    std::cout << "=== C++17：接住 emplace_back 的回傳參考 ===" << std::endl;
    Item& apple = items.emplace_back("Apple", 10);
    apple.quantity += 5;                 // 直接透過參考修改
    std::cout << "  apple 修改後 quantity = " << apple.quantity << std::endl;

    std::cout << "\n=== C++17：鏈式操作 ===" << std::endl;
    items.emplace_back("Banana", 20).quantity *= 2;
    std::cout << "  Banana 鏈式運算後 quantity = " << items.back().quantity
              << std::endl;

    std::cout << "\n=== C++11/14 的等價寫法（用 back()）===" << std::endl;
    items.emplace_back("Cherry", 7);
    items.back().quantity += 3;          // 回傳 void 時只能這樣寫
    std::cout << "  Cherry 用 back() 修改後 quantity = "
              << items.back().quantity << std::endl;

    std::cout << "\n=== 最終內容 ===" << std::endl;
    for (const auto& item : items) {
        std::cout << "  " << item.name << ": " << item.quantity << std::endl;
    }

    std::cout << "\n=== 日常實務：購物車促銷規則 ===" << std::endl;
    for (const CartLine& line : buildCart()) {
        std::cout << "  " << line.sku << " x" << line.qty << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 13 課：vector 元素新增：push_back、emplace_back5.cpp" -o emplace_return

// === 預期輸出 ===
// === C++17：接住 emplace_back 的回傳參考 ===
//   apple 修改後 quantity = 15
// 
// === C++17：鏈式操作 ===
//   Banana 鏈式運算後 quantity = 40
// 
// === C++11/14 的等價寫法（用 back()）===
//   Cherry 用 back() 修改後 quantity = 10
// 
// === 最終內容 ===
//   Apple: 15
//   Banana: 40
//   Cherry: 10
// 
// === 日常實務：購物車促銷規則 ===
//   SKU-1001 x2
//   SKU-2002 x6
//   SKU-3003 x11
