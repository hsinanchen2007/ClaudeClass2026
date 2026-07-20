// =============================================================================
//  lesson27_shallow_problem.cpp  —  淺拷貝的兩大災難：資料別名與 double free
// =============================================================================
//
// 【主題資訊 Information】
//   議題：類別持有裸指標成員時，編譯器隱式產生的拷貝建構函數只做
//         「成員逐一複製（memberwise copy）」——複製的是**指標值**，不是被指的資料。
//   隱式拷貝建構函數的等效形式：
//         SimpleString(const SimpleString& o) : m_data(o.m_data), m_len(o.m_len) {}
//   標準：隱式定義的拷貝建構函數自 C++98 起即為 memberwise copy（[class.copy.ctor]）。
//         C++11 起，若類別有使用者定義的解構函數，隱式拷貝的產生被標記為
//         **deprecated**（但仍會產生，所以不會有錯誤，甚至預設連警告都沒有）。
//   標頭檔：<cstring>（strlen / strcpy）、<map>（本檔的示範用 ledger）
//   複雜度：淺拷貝 O(1)（只複製 8 bytes 指標）；深拷貝 O(n)。
//           「淺拷貝比較快」是真的——它只是順便讓程式壞掉。
//
// ⚠️ 這是「故意寫錯」的教學範例，不是可以模仿的程式碼。
//    本類別只有解構函數卻沒有拷貝建構函數／拷貝賦值運算子（違反 Rule of Three）。
//
// ⚠️⚠️ 本檔在教學上做了一項**重要調整**（請務必讀懂，這正是本課的核心）：
//    原始版本會讓兩個物件真的對同一塊記憶體 delete[] 兩次。那是未定義行為，
//    實測結果為：程式被 glibc 以 "free(): double free detected in tcache 2"
//    中止（rc=134），而且第二個解構函數印出的「內容」是已釋放記憶體裡的殘值，
//    **每次執行都不一樣**（本機三次執行分別得到不同的亂碼位元組）。
//    未定義行為既不能寫進「預期輸出」，也不該拿來當示範。
//    因此本檔改用**帳本（ledger）攔截法**：
//      * 類別依然是錯的（依然沒有拷貝建構函數，依然共享記憶體）；
//      * 但解構函數只「記錄」而不真的 delete[]，
//        並在偵測到「同一位址第 2 次被要求釋放」時明白告訴你：
//        真實程式在這一刻發生 double free。
//    代價是刻意洩漏一小塊記憶體（程式結束時由 OS 回收）——
//    **洩漏是定義明確的行為，double free 不是**。這是刻意的取捨。
//    要看真正的 double free，請依檔尾說明用 -fsanitize=address 自行重現。
//
// 【詳細解釋 Explanation】
//
// 【1. 「拷貝」到底複製了什麼】
//   SimpleString 有兩個成員：char* m_data 與 size_t m_len。
//   編譯器產生的拷貝建構函數對每個成員各做一次複製建構：
//       m_data ← other.m_data     // 複製「指標值」= 8 bytes 的位址
//       m_len  ← other.m_len      // 複製長度
//   對 m_len 來說這完全正確。對 m_data 來說，複製出來的是**同一個位址**，
//   於是兩個物件指向同一塊堆積記憶體。編譯器沒有做錯任何事——
//   它只是不知道「這個指標代表所有權」。指標本身沒有攜帶這個資訊。
//
// 【2. 災難一：資料別名（aliasing）】
//   copy.setFirstChar('X') 會改到 original 看到的內容，因為根本是同一塊。
//   注意這**不是**未定義行為，而是定義明確、只是不符直覺的結果：
//   兩個指標指向同一物件，透過任一個修改，另一個當然看得到。
//   本檔會完整、真實地示範這一段，輸出決定性。
//
// 【3. 災難二：double free】
//   兩個物件離開 scope 時各自解構，各自 delete[] 同一個位址。
//   第二次釋放是**未定義行為**：標準不保證任何結果。
//   實務上常見的表現有三種，而且會隨編譯器、最佳化等級、
//   甚至同一支程式的不同執行而改變：
//     (a) 配置器偵測到並中止（本機 glibc 的行為，rc=134）；
//     (b) 靜靜跑完，堆積結構已被破壞，在很久以後的某次 new 才爆炸；
//     (c) 被攻擊者利用（double free 是經典的記憶體破壞攻擊面）。
//   最危險的是 (b)：症狀出現的地方與肇因完全無關，除錯成本極高。
//
// 【4. 三個層次的正確解法】
//   (a) 深拷貝：自訂拷貝建構函數 + 拷貝賦值運算子，各自持有獨立記憶體
//       → 見本課第 2 個檔案（deep_copy）與第 28、29 課。
//   (b) 禁止拷貝：= delete 掉拷貝建構函數與拷貝賦值運算子
//       → 適合真正獨佔資源的型別（socket、file handle、mutex）。
//   (c) 根本不要自己管：改用 std::string / std::vector / std::unique_ptr
//       → 現代 C++ 的標準答案。你會發現整個問題自動消失，
//         連 Rule of Three 都不必寫（Rule of Zero）。
//
// 【概念補充 Concept Deep Dive】
//   * 為什麼編譯器不警告？因為「兩個指標指向同一塊記憶體」在 C++ 裡完全合法，
//     那是 observer、cache、非擁有型參考的日常用法。編譯器無從得知
//     m_data 代表「所有權」還是「借用」。這正是 unique_ptr 存在的理由：
//     把「所有權」寫進型別，讓編譯器有能力幫你檢查。
//   * -Wdeprecated-copy（含在 -Wextra 中）只在「已有使用者定義的**拷貝賦值**或
//     **拷貝建構**」時才對隱式產生的另一個發出警告；
//     只有解構函數的情況（本檔）預設不會警告。這也是本缺陷極易漏網的原因。
//   * 隱式拷貝建構函數是逐成員複製，不是 memcpy。差別在於：
//     若成員本身是 std::string，它會呼叫 string 的拷貝建構函數（深拷貝）。
//     所以「用 std::string 取代 char*」不只是方便，是把正確性外包給已寫好的型別。
//   * ASan（AddressSanitizer）能穩定捕捉 double free 並印出兩次釋放的呼叫堆疊，
//     這是排查此類缺陷最有效的工具，成本約 2 倍執行時間、3 倍記憶體。
//
// 【注意事項 Pay Attention】
//   1. double free 是未定義行為，**不可**寫成「一定崩潰」。它常常不崩潰，
//      這才是它可怕的地方。本機這次是 glibc 中止，但那是實作行為，不是保證。
//   2. 「兩個物件的指標位址相同」是可以安全觀察的事實（比較指標值），
//      但**不要把位址本身印出來當預期輸出**——ASLR 使它每次執行都不同。
//      要驗證共享，請印「位址是否相同」的布林值（本檔作法）。
//   3. 讀取已釋放的記憶體同樣是未定義行為。原始版本第二個解構函數印出的
//      「內容」正是這種讀取，其結果每次都不同，絕不能寫進預期輸出。
//   4. 記憶體洩漏（本檔刻意為之）與 double free 是完全不同層級的問題：
//      前者是「行為明確、資源沒還」，後者是「行為未定義」。
//      教學示範寧可洩漏，也不要製造 UB。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】淺拷貝與 double free
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 一個類別只寫了解構函數、沒寫拷貝建構函數，會發生什麼事？
//     答：編譯器仍會產生隱式拷貝建構函數，做成員逐一複製。
//         若成員含裸指標，複製的是位址，兩個物件共享同一塊記憶體，
//         結果是 (1) 透過任一物件修改，另一個也會變；
//         (2) 兩者解構時對同一位址 delete 兩次 → double free（未定義行為）。
//         這正是「三法則（Rule of Three）」要解決的問題。
//     追問：C++11 之後這件事有變嗎？
//         → 有。當類別有使用者定義的解構函數時，隱式拷貝的產生被標記為
//           deprecated，但**仍然會產生**，所以程式照樣編譯、照樣壞掉。
//           另外它還會抑制隱式移動建構函數的產生，讓所有「移動」悄悄退化成複製。
//
// 🔥 Q2. double free 一定會 crash 嗎？
//     答：不一定。標準把它定義為未定義行為，沒有任何保證。
//         本機 glibc 會偵測到並以 "free(): double free detected in tcache 2"
//         中止（rc=134），但那是 glibc 的實作行為。
//         另一種常見情況是靜靜跑完、堆積結構被破壞，
//         在毫不相干的地方才出錯——那才是最難除錯的。
//     追問：怎麼可靠地抓到它？
//         → 用 -fsanitize=address 編譯執行，ASan 會印出「哪裡配置、
//           哪裡第一次釋放、哪裡第二次釋放」三段呼叫堆疊；或用 valgrind。
//
// ⚠️ 陷阱1. 「我在解構函數裡寫 delete[] m_data; m_data = nullptr; 就安全了。」
//     答：沒用。兩個物件各有**自己的** m_data 成員；把自己的設成 nullptr
//         完全不影響另一個物件的那份副本，第二次 delete 照樣發生。
//         （delete nullptr 本身是安全的，但這裡的 nullptr 設在錯的物件上。）
//     為什麼會錯：把兩個物件的成員誤以為是同一個變數。
//         被共享的是「指標指向的那塊記憶體」，不是「指標成員本身」。
//
// ⚠️ 陷阱2. 「編譯時開 -Wall -Wextra，這種錯編譯器會提醒我。」
//     答：不會。本檔以 g++ -Wall -Wextra 編譯是**零警告**的。
//         -Wdeprecated-copy 只在「已經自訂了拷貝建構或拷貝賦值其中之一」時
//         才會對另一個發出警告；只有解構函數的情況預設完全沉默。
//     為什麼會錯：以為警告等級開好開滿就能擋住所有記憶體問題。
//         編譯器無法區分「這個指標代表所有權」還是「只是借用」，
//         必須由型別（unique_ptr）來表達，它才有辦法檢查。
//
// ⚠️ 陷阱3. 「淺拷貝比較快，所以效能敏感的程式應該用淺拷貝。」
//     答：淺拷貝確實只複製 8 bytes，但它壓根沒有完成「複製一個字串」這件事。
//         要在效能與正確性之間取捨，正確的工具是**移動語意**（第 31、32 課）
//         或 shared_ptr 的共享所有權，而不是「假裝複製」。
//     為什麼會錯：把「做比較少的工作」當成最佳化。
//         最佳化的前提是結果必須正確；否則那不是快，是壞掉。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <cstring>   // std::strlen, std::strcpy
#include <map>

// ─────────────────────────────────────────────────────────────────────────────
// 教學用的「堆積帳本」：記錄每個位址被要求釋放幾次。
// 目的是在**不製造未定義行為**的前提下，精準呈現 double free 的發生時機。
// 真實程式不會有這種東西——真實程式會直接壞掉。
// ─────────────────────────────────────────────────────────────────────────────
namespace ledger {
    std::map<const void*, int> freeCount;

    // 回傳這是對該位址的第幾次釋放請求（1 = 第一次，正常；>=2 = double free）
    int requestFree(const void* p) {
        return ++freeCount[p];
    }
}

class SimpleString {
private:
    char* m_data;       // 指向堆積上的 C 風格字串
    std::size_t m_len;  // 字串長度（不含 '\0'）

public:
    // 建構函數：配置記憶體並複製字串
    SimpleString(const char* str = "") {
        m_len = std::strlen(str);
        m_data = new char[m_len + 1];  // +1 for '\0'
        std::strcpy(m_data, str);
        std::cout << "  [建構] 配置記憶體，內容=\"" << m_data << "\"\n";
    }

    // 解構函數
    //
    // 原始（真正錯誤）的版本是：
    //     ~SimpleString() { delete[] m_data; }
    // 兩個共享同一位址的物件依序解構時，第二次 delete[] 即為 double free（UB）。
    //
    // 本示範改為「只記帳、不真的釋放」，以便安全且決定性地展示問題發生的時刻。
    ~SimpleString() {
        const int nth = ledger::requestFree(m_data);
        if (nth == 1) {
            std::cout << "  [解構] 對這塊記憶體的第 1 次 delete[]（正常）\n";
        } else {
            std::cout << "  [解構] 對**同一塊**記憶體的第 " << nth
                      << " 次 delete[] ← 真實程式在此發生 double free（未定義行為）\n";
        }
        // 刻意不執行 delete[] m_data;
        //   → 換得「本檔沒有任何未定義行為」，代價是洩漏這一小塊記憶體。
        //   → 洩漏是行為明確的；double free 不是。教學示範選前者。
    }

    // ⚠️ 沒有自定義拷貝建構函數 → 編譯器使用淺拷貝
    // ⚠️ 沒有自定義拷貝賦值運算子 → 編譯器使用淺拷貝

    const char* c_str() const { return m_data; }

    // 供呼叫端在**不列印位址**的前提下驗證「是否共享同一塊記憶體」
    bool sharesBufferWith(const SimpleString& other) const {
        return m_data == other.m_data;
    }

    void setFirstChar(char ch) {
        if (m_len > 0) {
            m_data[0] = ch;
        }
    }
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】—— 從缺，並說明原因
//
// 本檔主題是「隱式拷貝造成的所有權缺陷」，屬於 C++ 資源管理與物件模型議題。
// LeetCode 判題只比對回傳值，既不檢查記憶體洩漏，也不檢查 double free；
// 更關鍵的是，指定清單中的 design 類題（146 LRU Cache、707 Design Linked List
// 等）雖然會 new 節點，但判題器從不複製你的物件，因此永遠不會觸發本檔的缺陷。
// 把它們搬進來只會讓讀者以為「這是刷題技巧」，模糊了真正的重點：
// 這是**函式庫與系統程式設計**的正確性問題。依規格「寧缺勿濫」，此處明確從缺。
// 正確的延伸練習是把本檔的 SimpleString 改寫成深拷貝版本（見本課第 2 個檔案）。
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 【日常實務範例】設定快照（ConfigSnapshot）—— 同一個缺陷在真實系統中的樣貌
//
// 情境：服務會定期把目前設定「快照」下來，交給背景執行緒做比對與稽核。
//       程式碼常寫成 auto snap = currentConfig;（一次隱式拷貝）。
//       若 ConfigSnapshot 內部持有裸指標的緩衝區，這行看似無害的賦值
//       就會讓兩個快照共享同一塊記憶體：
//         (1) 稽核執行緒改到了正式設定；
//         (2) 兩個快照生命週期結束時 double free。
//
// 這裡示範**正確**寫法：用 std::string / std::vector 持有資料。
// 它們自己的拷貝建構函數就是深拷貝，於是整個問題根本不會出現——
// 這就是 Rule of Zero：不寫解構函數、不寫拷貝、不寫移動，正確性自動成立。
// -----------------------------------------------------------------------------
#include <string>
#include <vector>

class ConfigSnapshot {
private:
    std::string             name_;       // 自帶深拷貝
    std::vector<std::string> keys_;      // 自帶深拷貝
    int                     version_;

public:
    ConfigSnapshot(std::string name, std::vector<std::string> keys, int version)
        : name_(std::move(name)), keys_(std::move(keys)), version_(version) {}

    // 完全不需要自訂解構函數／拷貝建構函數／拷貝賦值運算子（Rule of Zero）

    void rename(const std::string& n) { name_ = n; }
    void addKey(const std::string& k) { keys_.push_back(k); }

    void dump(const char* tag) const {
        std::cout << "  " << tag << " name=" << name_
                  << " version=" << version_ << " keys=[";
        for (std::size_t i = 0; i < keys_.size(); ++i) {
            if (i) std::cout << ",";
            std::cout << keys_[i];
        }
        std::cout << "]\n";
    }
};

int main() {
    std::cout << "=== 淺拷貝問題示範 ===\n\n";

    std::cout << "步驟 1：建立 original\n";
    SimpleString original("Hello");

    std::cout << "\n步驟 2：用 original 拷貝建構 copy\n";
    SimpleString copy = original;   // 呼叫編譯器自動生成的拷貝建構函數
    std::cout << "  （沒有任何輸出，因為隱式拷貝建構函數不會印東西——\n";
    std::cout << "    它只是把 8 bytes 的指標值抄過去）\n";

    std::cout << "\n步驟 3：檢查兩者是否共享同一塊記憶體\n";
    std::cout << "  original 與 copy 指向同一位址？ " << std::boolalpha
              << original.sharesBufferWith(copy) << "\n";
    std::cout << "  （刻意不印位址：位址受 ASLR 影響每次執行都不同，\n";
    std::cout << "    印布林值才是決定性的驗證方式）\n";

    std::cout << "\n步驟 4：透過 copy 修改第一個字元\n";
    copy.setFirstChar('X');
    std::cout << "  original = \"" << original.c_str() << "\"\n";
    std::cout << "  copy     = \"" << copy.c_str() << "\"\n";
    std::cout << "  （original 也被改了！因為共享記憶體——這一段是定義明確的行為）\n";

    std::cout << "\n步驟 5：離開作用域，開始解構\n";
    std::cout << "  copy 先解構、original 後解構，兩者持有同一個位址：\n";

    {
        // 讓兩個共享同一緩衝區的物件在此結束生命，觀察「第 2 次 delete[]」
        SimpleString a("Shared");
        SimpleString b = a;          // 淺拷貝：b.m_data == a.m_data
        std::cout << "  a 與 b 共享緩衝區？ " << std::boolalpha
                  << a.sharesBufferWith(b) << "\n";
    }   // b 解構（第 1 次）、a 解構（第 2 次 ← 真實程式在此 double free）

    std::cout << "\n=== 日常實務：用 Rule of Zero 讓問題根本不存在 ===\n";
    ConfigSnapshot live("payment-svc", {"timeout", "retries"}, 7);
    ConfigSnapshot snap = live;      // 隱式拷貝，但成員自帶深拷貝 → 安全

    snap.rename("payment-svc(audit)");
    snap.addKey("audit-only");

    live.dump("正式設定:");
    snap.dump("稽核快照:");
    std::cout << "  兩者完全獨立：改快照不會影響正式設定，也不會 double free\n";

    return 0;
}   // original 與 copy 在此解構：對同一位址的第 1、2 次 delete[]

// 編譯: g++ -std=c++17 -Wall -Wextra -g "第 27 課：淺拷貝與深拷貝（Shallow Copy vs Deep Copy）1.cpp" -o shallow
//
// 想親眼看到真正的 double free（本檔已刻意攔截，不會發生）：
//   把 ~SimpleString() 改回 { delete[] m_data; }，然後
//     g++ -std=c++17 -g -fsanitize=address <本檔> -o shallow_asan && ./shallow_asan
//   ASan 會印出「哪裡配置、哪裡第一次釋放、哪裡第二次釋放」三段呼叫堆疊。
//   不加 ASan 直接跑，本機 glibc 的實測結果是
//     free(): double free detected in tcache 2 → 程式中止（rc=134）
//   但這只是 glibc 的實作行為，**不是標準保證**。

// 注意事項（輸出相關）：
//   * 本檔**不列印任何指標位址**。位址受 ASLR 影響每次執行都不同
//     （本機三次執行分別得到三個不同位址），不可能寫成穩定的預期輸出。
//     驗證共享請看「是否指向同一位址」的布林值。
//   * 本檔**不執行**真正的 double free，也**不讀取已釋放的記憶體**。
//     原始版本第二個解構函數印出的「內容」是已釋放記憶體的殘值，
//     本機三次執行得到三種不同的亂碼位元組——那是未定義行為，
//     不可寫入預期輸出，也不該當作示範。
//   * 代價是刻意洩漏兩小塊緩衝區（"Hello" 與 "Shared" 各一），
//     程式結束時由作業系統回收。洩漏是**行為明確**的，double free 不是。
//   * 「第 2 次 delete[]」出現兩次，分別對應內層 scope 的 a/b 與
//     main 結束時的 original/copy，這兩組各自共享一塊緩衝區。
//   * 解構順序為宣告順序的反序（後建構者先解構），故 b 先於 a、copy 先於 original。

// === 預期輸出 ===
// === 淺拷貝問題示範 ===
//
// 步驟 1：建立 original
//   [建構] 配置記憶體，內容="Hello"
//
// 步驟 2：用 original 拷貝建構 copy
//   （沒有任何輸出，因為隱式拷貝建構函數不會印東西——
//     它只是把 8 bytes 的指標值抄過去）
//
// 步驟 3：檢查兩者是否共享同一塊記憶體
//   original 與 copy 指向同一位址？ true
//   （刻意不印位址：位址受 ASLR 影響每次執行都不同，
//     印布林值才是決定性的驗證方式）
//
// 步驟 4：透過 copy 修改第一個字元
//   original = "Xello"
//   copy     = "Xello"
//   （original 也被改了！因為共享記憶體——這一段是定義明確的行為）
//
// 步驟 5：離開作用域，開始解構
//   copy 先解構、original 後解構，兩者持有同一個位址：
//   [建構] 配置記憶體，內容="Shared"
//   a 與 b 共享緩衝區？ true
//   [解構] 對這塊記憶體的第 1 次 delete[]（正常）
//   [解構] 對**同一塊**記憶體的第 2 次 delete[] ← 真實程式在此發生 double free（未定義行為）
//
// === 日常實務：用 Rule of Zero 讓問題根本不存在 ===
//   正式設定: name=payment-svc version=7 keys=[timeout,retries]
//   稽核快照: name=payment-svc(audit) version=7 keys=[timeout,retries,audit-only]
//   兩者完全獨立：改快照不會影響正式設定，也不會 double free
//   [解構] 對這塊記憶體的第 1 次 delete[]（正常）
//   [解構] 對**同一塊**記憶體的第 2 次 delete[] ← 真實程式在此發生 double free（未定義行為）
