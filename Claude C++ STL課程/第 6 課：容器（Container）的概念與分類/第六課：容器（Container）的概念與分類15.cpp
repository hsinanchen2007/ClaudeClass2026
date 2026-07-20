// =============================================================================
//  第六課 15 — 容器的共同介面：STL 真正的設計貢獻
// =============================================================================
//
// 【主題資訊 Information】
//   幾乎所有標準容器都提供的「共同詞彙」(common vocabulary):
//
//     size_type   size()      const noexcept;   // 元素個數
//     bool        empty()     const noexcept;   // 是否為空,**永遠 O(1)**
//     size_type   max_size()  const noexcept;   // 理論上限(見下方警告)
//     void        swap(C&);                     // 交換內容
//     void        clear()     noexcept;         // 清空(std::array 沒有)
//     iterator    begin() / end();              // 走訪的起點與終點
//
//   複雜度：
//     size()      C++11 起**所有容器都保證 O(1)**(C++11 前 std::list 可為 O(n))
//     empty()     所有容器、所有版本一律 **O(1)**
//     swap()      節點/指標式容器 **O(1)**;**std::array 是 O(n)**(元素內嵌)
//     clear()     O(n)(要逐一解構元素)
//     max_size()  O(1),但這個數字幾乎沒有實用價值(見【注意事項】)
//
//   標準版本：這組介面自 C++98 就存在;
//             cbegin()/cend() 是 C++11;
//             非成員的 std::size()/std::empty()/std::data() 是 C++17;
//             contains() 是 C++20(只有關聯式容器有)。
//   標頭檔：各容器自己的標頭(<vector>、<list>、<set> …)
//
// 【詳細解釋 Explanation】
//
// 【1. 這一課真正的主題不是某個容器,而是「它們長得一樣」這件事】
// 前面十四個檔案各講一種容器。這一課要講的是把它們串起來的那條線。
//
// std::vector 是連續記憶體,std::list 是雙向鏈結串列,std::set 是紅黑樹 ——
// 三者的內部結構毫無共通之處,實作程式碼完全不同。但它們都答應了同一份契約:
// 問大小叫 size(),問空不空叫 empty(),清空叫 clear(),走訪叫 begin()/end()。
//
// 這件事看起來理所當然,其實是 STL 最大的設計貢獻。在 STL 之前,每套容器
// 函式庫都自己取名字:有的叫 length(),有的叫 count(),有的叫 getSize();
// 換一套函式庫,所有程式碼都得重寫。
//
// 統一命名帶來的不只是「好記」,而是**泛型程式設計成為可能**。
//
// 【2. 本檔的 show_container_info() 為什麼寫得出來?】
// 看這個函式:
//
//     template <typename Container>
//     void show_container_info(const std::string& name, const Container& c) {
//         std::cout << c.size() << c.empty() << c.max_size();
//     }
//
// 它沒有繼承任何基底類別、沒有虛擬函式、也不知道 Container 到底是什麼。
// 它能編譯成功的唯一理由是:**傳進來的型別剛好都有 size()/empty()/max_size()
// 這幾個名字**。編譯器在實例化時把名字對上了,就生出一份專屬的程式碼。
//
// 這叫 **duck typing at compile time**(編譯期鴨子型別):
// 「如果它走起來像鴨子、叫起來像鴨子,那它就是鴨子」——
// 差別在於 STL 是在**編譯期**驗證的,對不上就編譯錯誤,不會拖到執行期。
//
// 對比 Java/C# 的做法:那些語言靠 interface + 執行期虛擬呼叫達成同樣的事,
// 每次呼叫都要查 vtable。C++ 的模板則是為每個型別各生一份程式碼,
// 呼叫直接 inline —— 泛用性拿到了,執行期成本是零。這就是為什麼
// STL 能同時做到「非常泛用」和「非常快」,兩者通常是互斥的。
//
// 【3. empty() vs size() == 0:為什麼老手一律寫 empty()】
// 兩者在今天的結果完全一樣,但慣例是永遠用 empty(),理由有三層:
//
//   (a) **歷史效能問題(這是原始理由)**:C++11 之前,標準**沒有**要求
//       std::list::size() 是 O(1)。當時的實作可以選擇不存一個 size 欄位,
//       而在你呼叫 size() 時從頭走一遍鏈結串列去數 —— 那是 O(n)。
//       於是 if (lst.size() == 0) 在大串列上會變成一次完整走訪,只為了
//       回答「有沒有東西」。而 empty() 只要檢查 head 指標是不是 null,
//       **任何版本、任何容器都保證 O(1)**。
//       C++11 起規定 size() 一律 O(1),這個陷阱在標準容器上已經消失,
//       但**自訂容器或第三方函式庫仍可能是 O(n)**,習慣不該改。
//
//   (b) **語意更準確**:你想問的是「是不是空的」,不是「數量是多少,
//       然後拿去和 0 比」。程式碼應該直接說出意圖。
//
//   (c) **一致性**:empty() 在每個容器上都有、都 O(1)、都同樣語意,
//       寫泛型程式碼時不必為某個容器破例。
//
// 【4. swap() 為什麼是 O(1)?—— 交換的是指標,不是資料】
// 這是共同介面裡最違反直覺、也最值得懂的一個。
//
//     std::vector<int> a(1000000), b(2);
//     a.swap(b);        // 一百萬個元素…卻是 O(1)
//
// 因為 vector 物件本身只有三個指標(begin / end / capacity_end),
// 元素全在 heap 上。swap 做的只是把這三個指標對調 ——
// **heap 上那一百萬個元素一個都沒有動**,只是換了個主人。
//
// 本機實測(見輸出):swap 之後 a.data() 正好等於原本 b.data(),
// 反之亦然。指標對調,資料原地不動,這就是 O(1) 的證據。
//
// list、map、set 同理:它們的物件本體只有幾個指向節點的指標,
// swap 也只是換指標,節點全部留在原地。
//
// **唯一的例外是 std::array**:它的元素**內嵌在物件本身裡**,沒有指標可換,
// 只能真的一個一個複製/搬移過去,所以 std::array::swap 是 **O(n)**。
// 本機實測也證實了:array swap 之後 data() 指標**完全沒變**,變的是內容。
// 這個例外很值得記,因為它正好反映了 array「零間接、零配置」的本質。
//
// 【5. clear() 清掉的是元素,不是記憶體 —— vector 的 capacity 陷阱】
// clear() 會把 size() 變成 0,並逐一呼叫元素的解構子。但對 vector 來說,
// 它**不會歸還已配置的記憶體**:
//
//     std::vector<int> v(1000);
//     v.clear();
//     // v.size() == 0,但 v.capacity() 仍然是 1000(本機實測)
//
// 這是刻意的設計:clear() 常出現在「清空後馬上要重新填」的迴圈裡
// (例如每幀重建一份清單),留著容量可以省下反覆的配置與釋放。
//
// 真的想把記憶體還回去有兩招:
//     v.shrink_to_fit();            // C++11,但標準只說是「非強制的請求」
//     std::vector<int>().swap(v);   // 經典手法:和一個空 vector 交換
//
// 第二招的原理正是【4】講的:空 vector 沒有配置任何記憶體,交換後
// v 拿到「什麼都沒有」,而原本那塊記憶體歸給了臨時物件 ——
// 臨時物件在這行結束就解構,記憶體隨之釋放。這在 C++11 之前是唯一的辦法,
// 至今仍然可靠(shrink_to_fit 標準允許實作忽略,swap 手法則保證有效)。
// 本機實測兩種做法都讓 capacity 歸零。
//
// 【6. max_size():一個幾乎沒有用的數字】
// 這是本檔最需要澄清的誤解。很多人以為 max_size() 告訴你「還能放多少」,
// 或至少是「這台機器放得下多少」。**都不是。**
//
// 本機實測(GCC 15.2.0 / x86-64,**實作定義**):
//     std::vector<int>::max_size() = 2305843009213693951
//     std::list<int>::max_size()   = 384307168202282325
//     std::set<int>::max_size()    = 230584300921369395
//
// 第一個數字是 2^61 - 1,也就是 (2^64 / 8) - 1 左右。它是這樣算出來的:
// 把 size_type(64 位元)能表示的範圍,除以單一節點的大小 ——
// vector 存 int 但受 ptrdiff_t 限制,list 每個節點是 24 bytes(值 + 前後指標),
// set 的紅黑樹節點是 40 bytes(值 + 三指標 + 顏色,對齊後)。
// 所以三個數字不同,純粹反映**節點大小不同**。
//
// 換句話說,max_size() 是「型別系統與配置器在數學上的天花板」,
// 它**完全不知道你的機器有多少記憶體**。這台機器有 123 GB RAM,
// 但 vector 說可以放 2.3 × 10^18 個 int —— 那是 9 EB(exabyte),
// 比全世界的儲存裝置加起來還多。
//
// 實務結論:**max_size() 不能用來做容量規劃,也不能用來判斷還能不能配置**。
// 想知道會不會爆記憶體,唯一可靠的方式是實際去配置,並接住
// std::bad_alloc(或用作業系統層的資訊自行判斷)。
// 這個介面存在,主要是給泛型程式碼在極端情況下做健全性檢查用的,
// 日常程式碼幾乎不會用到它。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼 STL 用模板而不是共同基底類別?
//     直覺的物件導向做法是定義 class Container { virtual size_t size() = 0; },
//     讓所有容器繼承它。STL 刻意**沒有**這麼做,原因是:
//       * 虛擬呼叫無法 inline,而 size() 這種一行函式的呼叫開銷可能比
//         函式本體還大;
//       * 每個物件要多存一個 vptr(8 bytes),std::array 的「零開銷」就毀了;
//       * 元素型別 T 不同時,基底類別的介面型別無法統一。
//     改用模板之後,共同介面變成一份**不成文的約定(concept)**,
//     由編譯器在實例化時檢查。代價是錯誤訊息又長又難讀 ——
//     這正是 C++20 引入 concepts 語法要解決的問題。
//
// (B) C++17 的非成員版本:std::size() / std::empty() / std::data()
//     這三個自由函式除了能吃容器,還能吃 **C 原生陣列**:
//         int arr[5];
//         std::size(arr);     // 5 —— 成員版本不存在,因為 C 陣列沒有成員
//     所以寫泛型程式碼時,自由函式版本涵蓋的型別更廣。
//     這也是「共同詞彙」的延伸:連不是類別的型別都被納進同一套語彙。
//
// (C) 為什麼 std::array 沒有 clear()?
//     因為 array 的元素個數 N 是型別的一部分,永遠不可能變成 0。
//     clear() 的語意是「把 size 變成 0」,對 array 而言在型別層次上就矛盾。
//     同理它也沒有 push_back()、insert()、erase() —— 凡是會改變元素個數的
//     操作,array 一律沒有。它有的是 fill()(把每個元素設成同一個值)。
//
// (D) 共同介面的邊界:不是每個容器都有每一樣
//     這份「共同詞彙」是慣例,不是強制契約,實際上有不少例外:
//       * std::array          沒有 clear()、沒有任何改變大小的操作
//       * std::forward_list   **沒有 size()**(單向串列,存 size 會增加成本,
//                             標準選擇不提供),但有 empty()
//       * 配接器 stack/queue/priority_queue
//                             有 size()/empty()/swap(),但**沒有 begin()/end()、
//                             沒有 clear()**(見前三個檔案)
//     所以泛型程式碼真正能安全假設的,大概只有 size() 與 empty() 兩個,
//     再多就得靠 SFINAE 或 C++20 concepts 去偵測。
//
// 【注意事項 Pay Attention】
//  1. **max_size() 不是可用記憶體**。它是「size_type 除以節點大小」算出來的
//     理論上限,和機器實際有多少 RAM **毫無關係**。本機 vector<int> 報
//     2305843009213693951(約 9 EB),遠超過任何真實硬體。
//     絕不可拿它做容量規劃或「還能不能再放」的判斷 ——
//     要知道會不會失敗,只能實際配置並接住 std::bad_alloc。
//     這些數值全部屬**實作定義**,換編譯器/平台就會不同。
//  2. **一律用 empty(),不要寫 size() == 0**。C++11 前 std::list::size()
//     允許是 O(n);雖然現在標準容器都已保證 O(1),但自訂容器與第三方
//     函式庫不受此保證,而 empty() 永遠是 O(1)。
//  3. **clear() 不釋放 vector 的 capacity**。size() 歸零但記憶體仍佔著
//     (本機實測:clear 後 capacity 仍為 1000)。要真正釋放請用
//     shrink_to_fit()(標準允許實作忽略)或 std::vector<T>().swap(v)(保證有效)。
//  4. **swap() 對 std::array 是 O(n),不是 O(1)**。其他容器交換的是內部指標,
//     array 的元素內嵌在物件裡,只能逐一搬移。本機實測:vector swap 後
//     data() 指標互換,array swap 後 data() 指標不變、變的是內容。
//  5. swap() 之後,**指向元素的 iterator/reference 通常仍然有效**,
//     但它們現在屬於**另一個容器**了(因為資料整批換了主人)。
//     std::array 的 swap 則相反:iterator 仍指向原容器,但指向的值變了。
//     這個差異在寫泛型程式碼時很容易出錯。
//  6. 不是每個容器都有每一樣介面:std::forward_list **沒有 size()**,
//     std::array 沒有 clear(),配接器沒有 begin()/end()。
//     寫泛型函式時別假設過多。
//  7. 本檔的 show_container_info() 用到 std::string,**必須明確
//     #include <string>**。原本只靠 <iostream> 間接帶進來能編譯過,
//     但那是實作細節、標準沒有保證 —— 換一套標準庫就可能編譯失敗。
//     用到什麼就 include 什麼(IWYU:include what you use)。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】容器的共同介面
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼老手一律寫 c.empty() 而不寫 c.size() == 0?
//     答：主要是歷史效能理由。**C++11 之前標準沒有要求 std::list::size() 是
//         O(1)** —— 實作可以不存 size 欄位,而在呼叫時從頭走一遍串列去數。
//         於是 size()==0 在大串列上是一次完整 O(n) 走訪,只為回答「有沒有東西」;
//         而 empty() 只需檢查 head 指標,**任何版本、任何容器都保證 O(1)**。
//         C++11 起 size() 已一律 O(1),但自訂容器與第三方函式庫不受此保證,
//         而且 empty() 語意更直接、在泛型程式碼裡更一致,所以習慣沿用至今。
//     追問：C++11 已經修好了,那現在還有實際差別嗎?
//         → 標準容器上沒有效能差別了。但 std::forward_list **根本沒有 size()**
//           (單向串列存 size 要付額外成本,標準選擇不提供),只有 empty()。
//           所以泛型程式碼寫 size()==0 會直接編譯失敗,寫 empty() 才通用。
//
// 🔥 Q2. std::vector 有一百萬個元素,swap 給另一個 vector 要多久?為什麼?
//     答：**O(1)**,和元素個數完全無關。因為 vector 物件本體只有三個指標
//         (資料起點、結束、容量結束),元素全在 heap 上;swap 只是把這幾個
//         指標對調,heap 上那一百萬個元素**一個都沒有動**。
//         本機實測可驗證:swap 後 a.data() 恰好等於原本 b.data()。
//         list/map/set 同理,交換的都是內部指標。
//     追問：有沒有例外?
//         → 有,**std::array**。它的元素直接內嵌在物件裡,沒有指標可換,
//           swap 只能逐一複製/搬移每個元素,所以是 **O(n)**。
//           本機實測:array swap 後 data() 指標完全不變,變的是內容。
//
// ⚠️ 陷阱. max_size() 回傳 2305843009213693951,是不是代表這台機器
//         可以放進 23 億億個 int?
//     答：完全不是。max_size() 是「size_type 能表示的範圍 ÷ 單一節點大小」
//         算出來的**純理論上限**,它**根本不知道你的機器有多少記憶體**。
//         2305843009213693951 個 int 是約 9 EB(exabyte),比全世界的儲存
//         裝置加起來還多。真實可用量取決於實體 RAM、swap、位址空間與配置器,
//         max_size() 對這些一無所知。
//     為什麼會錯：把「max」直覺理解成「這個環境的容量上限」。
//         實際上它衡量的是**型別系統的天花板**,不是硬體的。
//         各容器數字不同(list 384307168202282325、set 230584300921369395)
//         純粹反映節點大小不同(list 節點含前後指標、set 的紅黑樹節點含
//         三指標與顏色),更說明它與可用記憶體無關。
//         想知道會不會爆,唯一可靠的做法是實際配置並接住 std::bad_alloc。
//
// ⚠️ 陷阱. 對一個裝了一百萬筆資料的 vector 呼叫 clear(),記憶體就還給系統了吧?
//     答：沒有。clear() 只把 size() 歸零並解構每個元素,**capacity 原封不動** ——
//         本機實測 clear() 之後 capacity() 仍然是 1000。那塊記憶體還被
//         這個 vector 佔著。這是刻意的:clear() 常用在「清空後馬上重填」的
//         迴圈裡,留著容量能省下反覆配置/釋放的成本。
//     為什麼會錯：把 clear() 想成「重置成剛建構的樣子」。實際上它只管
//         **元素**,不管**容量**。要真的還回去得用 shrink_to_fit()
//         (標準允許實作忽略這個請求)或經典的 std::vector<T>().swap(v)
//         —— 後者利用「swap 只換指標」的特性,把記憶體轉嫁給臨時物件,
//         臨時物件當場解構就釋放了,保證有效。
// ═══════════════════════════════════════════════════════════════════════════
//
// 註：本檔沒有【LeetCode 實戰範例】。「容器的共同介面」是一個 API 設計主題,
//     不對應任何特定的演算法題型 —— 硬湊一題只會誤導。寧缺勿濫。

#include <iostream>
#include <vector>
#include <list>
#include <set>
#include <array>
#include <string>       // ← show_container_info() 用到 std::string,必須明確 include
                        //    原本只靠 <iostream> 間接帶進來,那是實作細節、標準沒保證

template <typename Container>
void show_container_info(const std::string& name, const Container& c) {
    std::cout << name << ":" << std::endl;
    std::cout << "  size(): " << c.size() << std::endl;
    std::cout << "  empty(): " << (c.empty() ? "true" : "false") << std::endl;
    // max_size(): 容器理論上能容納的最大元素數
    std::cout << "  max_size(): " << c.max_size() << std::endl;
    std::cout << std::endl;
}

// -----------------------------------------------------------------------------
// 【共同介面的威力示範】一個函式,吃遍所有容器
//   這個函式只用到 begin()/end() 這組共同詞彙,所以 vector、list、set
//   —— 三種內部結構完全不同的容器 —— 全都能傳進來,而且不需要任何
//   繼承、虛擬函式或執行期分派。編譯器為每個型別各生一份程式碼,
//   呼叫全部 inline,泛用性與效能兼得。
// -----------------------------------------------------------------------------
template <typename Container>
long long sum_all(const Container& c) {
    long long total = 0;
    for (auto it = c.begin(); it != c.end(); ++it) {
        total += *it;
    }
    return total;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】服務的多層快取健康檢查(cache health report)
//   情境：一個後端服務通常同時維護好幾種快取,而且刻意選用不同容器:
//     - 最近請求記錄   → std::vector(連續、掃描快、只追加)
//     - 待寫回佇列     → std::list(中間刪除多、不想搬移元素)
//     - 已封鎖 IP 名單 → std::set(要去重、要快速查詢)
//   監控端要定期回報每一層的狀態,並在超過警戒線時示警。
//   為什麼用到本主題：監控函式**不應該**為三種容器各寫一份。
//     它只需要 size() 和 empty() —— 這正是共同介面存在的意義:
//     一份泛型程式碼涵蓋所有容器,日後新增一層快取(換成 deque、unordered_set)
//     也完全不必修改監控邏輯。
//   注意：警戒線用的是自己設定的閾值,**不是 max_size()** ——
//     max_size() 是理論上限,和這台機器的實際容量毫無關係(見【注意事項】1)。
// -----------------------------------------------------------------------------
template <typename Container>
void report_cache(const std::string& layer, const Container& c, size_t warnAt) {
    std::cout << "  [" << layer << "] ";
    if (c.empty()) {                       // 用 empty(),不用 size()==0
        std::cout << "空置 (0 筆)" << std::endl;
        return;
    }
    std::cout << c.size() << " 筆";
    if (c.size() >= warnAt) {
        std::cout << "  ⚠️ 已達警戒線 " << warnAt << ",建議清理或擴充";
    } else {
        std::cout << "  ✅ 正常 (警戒線 " << warnAt << ")";
    }
    std::cout << std::endl;
}

int main() {
    // ── 原始課堂示範:共同介面查詢 ────────────────────────────────────────
    std::vector<int> vec = {1, 2, 3, 4, 5};
    std::list<int> lst = {1, 2, 3};
    std::set<int> s = {1, 2};
    std::vector<int> empty_vec;

    show_container_info("vector", vec);
    show_container_info("list", lst);
    show_container_info("set", s);
    show_container_info("empty_vector", empty_vec);

    // 其他共同操作
    std::cout << "=== 其他共同操作 ===" << std::endl;

    // swap
    std::vector<int> v1 = {1, 2, 3};
    std::vector<int> v2 = {10, 20};
    v1.swap(v2);

    std::cout << "swap 後 v1: ";
    for (int n : v1) std::cout << n << " ";
    std::cout << std::endl;

    // clear
    v1.clear();
    std::cout << "clear 後 v1.size(): " << v1.size() << std::endl;

    // ── 同一份泛型程式碼吃遍三種容器 ──────────────────────────────────────
    std::cout << "\n=== 共同介面的威力: 一個函式吃遍所有容器 ===" << std::endl;
    std::cout << "sum_all(vector{1,2,3,4,5}) = " << sum_all(vec) << std::endl;
    std::cout << "sum_all(list{1,2,3})       = " << sum_all(lst) << std::endl;
    std::cout << "sum_all(set{1,2})          = " << sum_all(s) << std::endl;
    std::cout << "三種內部結構完全不同(連續記憶體/鏈結串列/紅黑樹)," << std::endl;
    std::cout << "但都答應同一份 begin()/end() 契約 → 一份程式碼即可通吃"
              << std::endl;
    std::cout << "沒有繼承、沒有虛擬函式、沒有執行期分派 —— 全部在編譯期解決"
              << std::endl;

    // ── empty() vs size() == 0 ────────────────────────────────────────────
    std::cout << "\n=== empty() vs size() == 0 ===" << std::endl;
    std::cout << "empty_vec.empty()      = " << std::boolalpha
              << empty_vec.empty() << std::endl;
    std::cout << "empty_vec.size() == 0  = " << (empty_vec.size() == 0) << std::endl;
    std::cout << "結果一樣,但 empty() 永遠 O(1);C++11 前 std::list::size()"
              << std::endl;
    std::cout << "允許是 O(n),且 std::forward_list 至今根本沒有 size()"
              << std::endl;

    // ── swap 為什麼是 O(1):交換的是指標 ──────────────────────────────────
    std::cout << "\n=== swap 的真相: 交換指標,不是搬資料 ===" << std::endl;
    std::vector<int> big{1, 2, 3};
    std::vector<int> small{9, 8};
    const int* pBig   = big.data();      // 記下交換前各自的資料位址
    const int* pSmall = small.data();
    big.swap(small);
    std::cout << "vector: big.data() 現在等於原本 small.data()? "
              << (big.data() == pSmall) << std::endl;
    std::cout << "vector: small.data() 現在等於原本 big.data()? "
              << (small.data() == pBig) << std::endl;
    std::cout << "→ 指標互換,heap 上的元素一個都沒動 ⇒ O(1)" << std::endl;

    // std::array 是唯一的例外:元素內嵌,只能真的搬
    std::array<int, 3> a1{1, 2, 3};
    std::array<int, 3> a2{7, 7, 7};
    const int* pa1 = a1.data();
    const int* pa2 = a2.data();
    a1.swap(a2);
    std::cout << "array : a1.data() 位址不變? " << (a1.data() == pa1)
              << " ,a2.data() 位址不變? " << (a2.data() == pa2) << std::endl;
    std::cout << "array : 但內容換了 → a1[0] = " << a1[0] << std::endl;
    std::cout << "→ 元素內嵌在物件裡,沒有指標可換,只能逐一搬移 ⇒ O(n)"
              << std::endl;

    // ── clear() 不釋放 capacity ───────────────────────────────────────────
    std::cout << "\n=== clear() 清元素,不還記憶體 ===" << std::endl;
    std::vector<int> cap(1000, 1);
    std::cout << "填入 1000 筆後   : size=" << cap.size()
              << " capacity=" << cap.capacity() << std::endl;
    cap.clear();
    std::cout << "clear() 之後     : size=" << cap.size()
              << " capacity=" << cap.capacity()
              << "  ← 記憶體還佔著!" << std::endl;
    cap.shrink_to_fit();                       // C++11,但標準允許實作忽略
    std::cout << "shrink_to_fit()  : size=" << cap.size()
              << " capacity=" << cap.capacity() << std::endl;

    std::vector<int> cap2(1000, 1);
    std::vector<int>().swap(cap2);             // 經典手法,保證有效
    std::cout << "swap-with-empty  : size=" << cap2.size()
              << " capacity=" << cap2.capacity()
              << "  ← 利用『swap 只換指標』把記憶體轉給臨時物件" << std::endl;

    // ── max_size() 的真面目 ───────────────────────────────────────────────
    std::cout << "\n=== max_size() 是理論上限, 不是可用記憶體 ===" << std::endl;
    std::cout << "vector<int>::max_size() = " << vec.max_size() << std::endl;
    std::cout << "list<int>::max_size()   = " << lst.max_size() << std::endl;
    std::cout << "set<int>::max_size()    = " << s.max_size() << std::endl;
    std::cout << "三者不同,純粹反映節點大小不同(以下皆為實作定義):" << std::endl;
    std::cout << "  sizeof(vector<int>) = " << sizeof(std::vector<int>)
              << " ,sizeof(list<int>) = " << sizeof(std::list<int>)
              << " ,sizeof(set<int>) = " << sizeof(std::set<int>) << std::endl;
    std::cout << "vector 那個數字約等於 9 EB(exabyte)的 int," << std::endl;
    std::cout << "比全世界的儲存裝置加起來還多 → 和本機實際記憶體毫無關係"
              << std::endl;
    std::cout << "要知道會不會配置失敗,只能實際配置並接住 std::bad_alloc"
              << std::endl;

    // ── 日常實務:多層快取健康檢查 ────────────────────────────────────────
    std::cout << "\n=== 日常實務: 服務的多層快取健康檢查 ===" << std::endl;
    std::vector<int> recentRequests = {1001, 1002, 1003, 1004, 1005,
                                       1006, 1007, 1008};   // 連續、只追加
    std::list<int>   writeBackQueue = {77, 88};              // 中間刪除多
    std::set<int>    blockedIPs     = {10, 20, 30, 40, 50};  // 要去重 + 快查
    std::list<int>   deadLetters;                            // 目前為空

    std::cout << "一份泛型監控函式,涵蓋 vector / list / set 三種容器:" << std::endl;
    report_cache("最近請求記錄 (vector)", recentRequests, 8);
    report_cache("待寫回佇列   (list)  ", writeBackQueue, 10);
    report_cache("封鎖 IP 名單 (set)   ", blockedIPs,     16);
    report_cache("死信佇列     (list)  ", deadLetters,    5);
    std::cout << "→ 日後新增一層快取(換 deque / unordered_set),"
              << "監控邏輯完全不必改" << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第六課：容器（Container）的概念與分類15.cpp" -o common_api_demo

// === 預期輸出 ===
// vector:
//   size(): 5
//   empty(): false
//   max_size(): 2305843009213693951
//
// list:
//   size(): 3
//   empty(): false
//   max_size(): 384307168202282325
//
// set:
//   size(): 2
//   empty(): false
//   max_size(): 230584300921369395
//
// empty_vector:
//   size(): 0
//   empty(): true
//   max_size(): 2305843009213693951
//
// === 其他共同操作 ===
// swap 後 v1: 10 20
// clear 後 v1.size(): 0
//
// === 共同介面的威力: 一個函式吃遍所有容器 ===
// sum_all(vector{1,2,3,4,5}) = 15
// sum_all(list{1,2,3})       = 6
// sum_all(set{1,2})          = 3
// 三種內部結構完全不同(連續記憶體/鏈結串列/紅黑樹),
// 但都答應同一份 begin()/end() 契約 → 一份程式碼即可通吃
// 沒有繼承、沒有虛擬函式、沒有執行期分派 —— 全部在編譯期解決
//
// === empty() vs size() == 0 ===
// empty_vec.empty()      = true
// empty_vec.size() == 0  = true
// 結果一樣,但 empty() 永遠 O(1);C++11 前 std::list::size()
// 允許是 O(n),且 std::forward_list 至今根本沒有 size()
//
// === swap 的真相: 交換指標,不是搬資料 ===
// vector: big.data() 現在等於原本 small.data()? true
// vector: small.data() 現在等於原本 big.data()? true
// → 指標互換,heap 上的元素一個都沒動 ⇒ O(1)
// array : a1.data() 位址不變? true ,a2.data() 位址不變? true
// array : 但內容換了 → a1[0] = 7
// → 元素內嵌在物件裡,沒有指標可換,只能逐一搬移 ⇒ O(n)
//
// === clear() 清元素,不還記憶體 ===
// 填入 1000 筆後   : size=1000 capacity=1000
// clear() 之後     : size=0 capacity=1000  ← 記憶體還佔著!
// shrink_to_fit()  : size=0 capacity=0
// swap-with-empty  : size=0 capacity=0  ← 利用『swap 只換指標』把記憶體轉給臨時物件
//
// === max_size() 是理論上限, 不是可用記憶體 ===
// vector<int>::max_size() = 2305843009213693951
// list<int>::max_size()   = 384307168202282325
// set<int>::max_size()    = 230584300921369395
// 三者不同,純粹反映節點大小不同(以下皆為實作定義):
//   sizeof(vector<int>) = 24 ,sizeof(list<int>) = 24 ,sizeof(set<int>) = 48
// vector 那個數字約等於 9 EB(exabyte)的 int,
// 比全世界的儲存裝置加起來還多 → 和本機實際記憶體毫無關係
// 要知道會不會配置失敗,只能實際配置並接住 std::bad_alloc
//
// === 日常實務: 服務的多層快取健康檢查 ===
// 一份泛型監控函式,涵蓋 vector / list / set 三種容器:
//   [最近請求記錄 (vector)] 8 筆  ⚠️ 已達警戒線 8,建議清理或擴充
//   [待寫回佇列   (list)  ] 2 筆  ✅ 正常 (警戒線 10)
//   [封鎖 IP 名單 (set)   ] 5 筆  ✅ 正常 (警戒線 16)
//   [死信佇列     (list)  ] 空置 (0 筆)
// → 日後新增一層快取(換 deque / unordered_set),監控邏輯完全不必改
