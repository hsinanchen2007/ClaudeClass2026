// =============================================================================
//  第 4 課：命名空間（namespace）基礎（5） — 命名空間別名:縮短名稱而不放棄限定
// =============================================================================
//
// 【主題資訊 Information】
//   語法：namespace 別名 = 原命名空間名稱;      // namespace alias
//         例：namespace fs = std::filesystem;
//   標準版本：C++98 起。
//   複雜度  ：零執行期成本 —— 別名只是編譯期的另一個名字,不產生任何實體。
//
// 【詳細解釋 Explanation】
//
// 【1. 別名要解決的問題:巢狀命名空間會很長】
//   命名空間鼓勵你把名稱分層,於是真實專案裡常見這種東西:
//       mycompany::graphics::rendering::backend::vulkan::createDevice()
//   每次都寫全名不切實際,但改用 using namespace 又會把整層倒進來、失去限定。
//   別名剛好落在中間:
//       namespace vk = mycompany::graphics::rendering::backend::vulkan;
//       vk::createDevice();
//   名稱變短了,但「vk::」這個限定前綴還在 —— 讀者仍看得出這個函式來自何處,
//   而且沒有任何其他名稱被引入目前作用域。這是安全性與便利性的最佳折衷。
//
// 【2. 別名不是複製,只是另一個入口】
//   本檔 main 同時用 vln::greet() 與 very_long_namespace_name::greet(),
//   兩者呼叫的是同一個函式,輸出完全相同。別名不會產生新的命名空間,
//   也不會複製任何成員 —— 它只是讓同一個作用域多一個稱呼。
//   推論:透過別名新增的東西,原名稱也看得到,反之亦然。
//
// 【3. 別名有作用域,而且可以是區域的】
//   本檔的 namespace vln = ...; 寫在 main 內部,因此只在 main 中有效。
//   這一點常被忽略,卻很有用:你可以在不同函式裡,把同一個短別名
//   指向不同的實作命名空間 —— 這正是下方【4】版本切換手法的基礎。
//
// 【4. 最實用的場景:切換實作版本】
//   真實專案常見這種模式:
//       namespace v1 { void encode(); }
//       namespace v2 { void encode(); }
//       namespace api = v2;      // 只改這一行,整個檔案就切到新版
//       api::encode();
//   要回退只需把別名指回 v1,所有呼叫端一行都不用動。
//   標準函式庫自己也用同樣的手法 —— std::filesystem 幾乎總是被寫成
//       namespace fs = std::filesystem;
//   已經是業界慣例。
//
// 【概念補充 Concept Deep Dive】
//   C++ 有三種「取別名」的機制,語法相似但作用對象完全不同,別搞混:
//     namespace fs = std::filesystem;    // 命名空間別名(C++98)
//     using Vec = std::vector<int>;      // 型別別名(C++11,取代 typedef)
//     using std::cout;                   // using 宣告 —— 這不是別名!
//   第三者是「把名稱引入目前作用域」,前兩者是「為既有實體另取一個名字」。
//   一個判別方式:前兩者用了 = 號,第三者沒有。
//   另外,命名空間別名不能指向「不存在的命名空間」,也不能像
//   命名空間本身那樣被重新開啟擴充 —— 它純粹是個指標性的名字。
//
// 【注意事項 Pay Attention】
//   1. 別名的右側必須是「已經宣告過」的命名空間,不能用來提前宣告。
//   2. 別名可以指向別名,形成鏈:namespace a = b; namespace c = a;
//      合法,但層層轉指會讓讀者難以追蹤,不建議。
//   3. 同一作用域內把同一個別名重新指向「不同」的命名空間會編譯錯誤;
//      重複指向「相同」的命名空間則是允許的(等同重複宣告)。
//   4. 標頭檔的全域範圍雖然可以定義別名(它不像 using 指示那樣汙染),
//      但仍會佔用引入者的名稱。短別名如 fs、vk 撞名機率不低,
//      在標頭中請取較具識別度的名字,或改放在函式內。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】命名空間別名
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. namespace fs = std::filesystem; 和 using namespace std::filesystem;
//        差在哪?為什麼幾乎所有專案都選前者?
//     答：別名只是為該命名空間另取一個短名字,呼叫時仍須寫 fs:: 限定前綴,
//         沒有任何名稱被引入目前作用域;using 指示則把該命名空間的
//         所有名稱都變成非限定可見(path、copy、remove、exists……,
//         其中 copy 和 remove 與 <algorithm> 的同名函式極易衝突)。
//         前者兼顧了「短」與「看得出出處」,所以成為業界慣例。
//     追問：別名有執行期成本嗎?
//         → 完全沒有。它是純編譯期的名稱對應,不產生任何符號或間接層。
//
// 🔥 Q2. 命名空間別名在實務上除了「縮短名稱」還有什麼用途?
//     答：切換實作版本。把 namespace api = v2; 集中在一處,
//         所有呼叫端都寫 api::foo();要回退到 v1 只需改那一行。
//         這比在數十處呼叫點做全域搜尋取代安全得多,
//         也比用巨集或條件編譯乾淨。
//     追問：那和 inline namespace 有什麼不同?
//         → inline namespace(C++11)是讓內層命名空間的名稱「自動出現在外層」,
//           常用於函式庫的 ABI 版本控管;呼叫端連前綴都不用寫。
//           別名則是呼叫端自己決定要怎麼稱呼,兩者的控制權在不同人手上。
//
// ⚠️ 陷阱. namespace vln = very_long_namespace_name; 是不是等於
//         把那個命名空間「複製」了一份?之後往 vln 裡加東西,
//         原命名空間會不會沒有?
//     答：不是複製,兩個名字指的是同一個命名空間。
//         而且你根本無法「往 vln 裡加東西」—— 別名不能被重新開啟擴充,
//         想擴充必須寫原名稱 namespace very_long_namespace_name { ... }。
//         擴充之後,透過 vln:: 一樣看得到新成員。
//     為什麼會錯：把 namespace X = Y; 的等號誤讀成「賦值/複製」。
//         它其實比較接近「取綽號」——同一個人,兩個叫法。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【本檔不加 LeetCode 範例的理由】
//   命名空間別名純粹是名稱管理機制,與演算法及複雜度無關。
//   下方改以「版本切換」這個別名最有價值的真實用途來示範。

#include <iostream>
#include <string>    // 下方實務範例用到 std::string / std::to_string
                     // （<iostream> 通常會間接帶進 <string>，但不可依賴這種傳遞性，
                     //   用到什麼就明確 include 什麼）

namespace very_long_namespace_name {
    void greet() {
        std::cout << "Hello from long namespace!" << std::endl;
    }
}

// -----------------------------------------------------------------------------
// 【日常實務範例】用命名空間別名切換 API 版本
// 情境：序列化模組改版了。v1 用舊格式，v2 改成帶長度前綴的新格式。
//       兩個版本必須並存一段時間（舊資料還要能讀），但「新程式碼一律用新版」。
//       做法：把選擇集中成一行別名，呼叫端全部寫 codec::encode(...)。
//       要回退只需把 codec 指回 v1，數十處呼叫點一行都不必改。
//       這是真實專案裡命名空間別名最有價值的用途，遠比「名字太長」重要。
// -----------------------------------------------------------------------------
namespace codec_v1 {
    std::string encode(const std::string& payload) {
        return "[v1]" + payload;                       // 舊格式：單純加標記
    }
}

namespace codec_v2 {
    std::string encode(const std::string& payload) {
        // 新格式：帶長度前綴，接收端可據此切割串流
        return "[v2:" + std::to_string(payload.size()) + "]" + payload;
    }
}

// ★ 全專案的版本選擇，就這一行。改成 codec_v1 即完成回退。
namespace codec = codec_v2;

int main() {
    // 建立別名
    namespace vln = very_long_namespace_name;

    // 使用別名
    vln::greet();

    // 原名稱仍然有效——兩者是同一個命名空間，不是複製品
    very_long_namespace_name::greet();

    // ── 實務：透過別名呼叫，呼叫端不知道也不需要知道目前是哪一版 ──
    std::cout << "\n=== 用別名切換 API 版本 ===\n";
    std::cout << "  codec::encode  -> " << codec::encode("user=alice") << "\n";
    // 需要明確指定版本時，仍可直接寫原名稱（例如讀取舊資料）
    std::cout << "  codec_v1::encode -> " << codec_v1::encode("user=alice") << "\n";
    std::cout << "  codec_v2::encode -> " << codec_v2::encode("user=alice") << "\n";
    std::cout << "  → 目前 codec 指向 v2；把別名改成 codec_v1 即可整批回退\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 4 課：命名空間（namespace）基礎5.cpp" -o ns5

// ─────────────────────────────────────────────────────────────────────────────
// 【輸出但書】
//  1. 前兩行輸出完全相同,這正是重點:vln:: 與 very_long_namespace_name::
//     指的是同一個命名空間,別名不是複製品。
//  2. codec::encode 的輸出帶 [v2:10] 前綴,是因為檔案中的別名目前指向 codec_v2。
//     把 namespace codec = codec_v2; 改成 codec_v1,該行就會變成 [v1]user=alice,
//     而所有呼叫端一個字都不用改 —— 這就是別名用於版本切換的價值。
//     ("user=alice" 長度為 10,故前綴是 [v2:10]。)
//  3. 以下為本機 g++ 15.2.0 (Ubuntu 26.04) 連續執行 3 次、逐位元組相同的結果。
// ─────────────────────────────────────────────────────────────────────────────

// === 預期輸出 ===
// Hello from long namespace!
// Hello from long namespace!
//
// === 用別名切換 API 版本 ===
//   codec::encode  -> [v2:10]user=alice
//   codec_v1::encode -> [v1]user=alice
//   codec_v2::encode -> [v2:10]user=alice
//   → 目前 codec 指向 v2；把別名改成 codec_v1 即可整批回退
