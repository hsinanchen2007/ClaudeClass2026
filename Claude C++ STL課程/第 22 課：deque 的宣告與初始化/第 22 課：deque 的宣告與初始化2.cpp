// =============================================================================
//  第 22 課：deque 的宣告與初始化 2  —  容器複製是「深」還是「淺」？裸指標元素的陷阱
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：#include <deque> / <vector> / <memory>
//   主題：  容器的複製建構 container(const container& other) 到底複製了什麼
//   複雜度：O(n) —— n 次元素的複製建構
//
//   本檔用 vector<int*> 示範，但**結論對 deque、list、set、map 完全一樣**：
//   所有 STL 容器的複製語意都是「逐元素呼叫該元素型別的複製建構子」。
//
// 【詳細解釋 Explanation】
//
// 【1. 「深複製 vs 淺複製」這個問法本身就有陷阱】
// 很多人問「vector 的複製是深還是淺？」，這問題其實**問錯了層次**。
// 正確的說法是：
//     容器的複製 = 對每一個元素呼叫「該元素型別的複製建構子」。
// 所以深或淺，**完全取決於元素型別自己怎麼複製**：
//     vector<int>      → int 複製 int 值        → 看起來是「深」的
//     vector<string>   → string 複製字元內容    → 深（string 自己管理緩衝區）
//     vector<int*>     → 複製「指標這個數值」   → **淺**（兩邊指向同一塊 heap）
//     vector<shared_ptr<T>> → 複製 shared_ptr   → 共用同一個物件，但引用計數 +1（安全）
//     vector<unique_ptr<T>> → **根本無法複製**（unique_ptr 不可複製，編譯錯誤）
// 容器只是忠實地照元素型別的規則走，它沒有「決定要深還是淺」的權力。
//
// 【2. 為什麼 vector<int*> 特別危險】
// 複製之後，v1[0] 和 v2[0] 是兩個各自獨立的指標變數，但**存著同一個位址**：
//
//     v1: [ 0x55f...a0 ][ 0x55f...c0 ]
//                │              │
//                ▼              ▼
//            [ 42 ]         [ 99 ]        ← heap 上只有這一份
//                ▲              ▲
//                │              │
//     v2: [ 0x55f...a0 ][ 0x55f...c0 ]    ← 複製的是位址，不是指向的資料
//
// 由此產生三個經典 bug：
//   (a) 意外的別名（aliasing）：透過 v2 改資料，v1 也跟著變 —— 常常不是本意。
//   (b) double free：v1 和 v2 各自 delete 一輪 → 同一塊被釋放兩次 → **未定義行為**。
//   (c) 懸空指標（dangling）：v1 先 delete 完，v2 裡的指標就全成了懸空指標，
//       再解參考同樣是未定義行為。
//
// 【3. 三種正確做法】
//   (A) 直接存值（最推薦）：vector<int> —— 沒有指標就沒有這些問題。
//       只有在「元素是多型基底類別」或「元素超大且需共享」時才需要指標。
//   (B) 存 shared_ptr<T>：需要共享所有權時用。複製容器 → 引用計數 +1，
//       最後一個析構時才釋放，不會 double free。
//   (C) 存 unique_ptr<T>：獨佔所有權。容器變成 **不可複製、只能移動**，
//       編譯器會直接擋下錯誤的複製 —— 「讓錯誤在編譯期就爆掉」是最好的設計。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼 unique_ptr 版本「不能編譯」反而是好事
//   vector<unique_ptr<int>> v2 = v1;  // 編譯錯誤
//   這行編不過，是因為 unique_ptr 刪除了複製建構子。編譯器攔下你，
//   等於在**編譯期**就消滅了 double free 的可能性。
//   相對地 vector<int*> 編得過、跑得動、只是偶爾在正式環境炸掉 ——
//   這才是最糟的情況。C++ 的核心哲學之一就是「把 runtime 錯誤推到 compile time」。
//   要移動整個容器仍然可以：vector<unique_ptr<int>> v2 = std::move(v1);
//
// (B) double free 是「未定義行為」，不是「一定會 crash」
//   標準對 double free 的規定是 undefined behavior。實際可能發生的包括：
//     - glibc 偵測到並印出 "double free or corruption" 然後 abort（本機常見）
//     - 靜默地破壞 heap 的中繼資料，程式在**很久之後**才在別的地方崩潰
//     - 在某些配置下看起來「正常跑完」，但已經污染了記憶體
//   最後一種最可怕：它會讓你以為程式沒問題。所以本檔**不會**實際執行
//   double free，只用註解示範 —— 示範 UB 的正確方式是說明，不是觸發。
//
// (C) 這跟 deque 有什麼關係
//   本課主題是 deque 的初始化，而 deque 的複製建構子語意與 vector 完全相同。
//   本檔特意用 vector 示範是為了對照第 1 支檔案，但下方 main() 也附上
//   deque<int*> 的版本，證明兩者行為一致 —— 這是容器的**共通語意**，
//   不是某個容器的特殊行為。
//
// 【注意事項 Pay Attention】
// 1. double free 是**未定義行為**。它「可能」被 glibc 偵測並 abort，
//    也「可能」靜默破壞 heap 讓程式在其他地方崩潰。不可描述成固定結果。
// 2. 移動（move）不會有這個問題：移動後來源被清空，只有一個容器持有指標。
// 3. 這個陷阱在**函式傳參**時最常中招：void f(vector<int*> v) 按值傳遞，
//    函式內若 delete 了元素，呼叫端的容器就全是懸空指標了。
// 4. 現代 C++ 的建議很明確：**容器裡不要放需要手動 delete 的裸指標**。
//    需要多型或共享時用 smart pointer，這正是它們存在的理由。
// 5. 裸指標本身不是罪 —— 「只觀察、不擁有」的裸指標（non-owning）是合理的，
//    危險的是「擁有資源卻沒有明確的所有權規則」。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】容器複製與元素所有權
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. vector<T> 的複製建構是深複製還是淺複製？
//     答：這問題問錯層次了。正確答案是：容器複製 = 對每個元素呼叫**元素型別的**
//         複製建構子。所以 vector<int>/vector<string> 表現為深複製，
//         vector<int*> 表現為淺複製（只複製位址），vector<unique_ptr<T>>
//         則根本不能複製。深淺由元素型別決定，容器本身不做這個決定。
//     追問：那 vector<shared_ptr<T>> 呢？
//         → 複製 shared_ptr → 兩個容器共用同一批物件，但引用計數 +1，
//           所以是「共享但安全」，不會 double free。
//
// 🔥 Q2. vector<int*> v2 = v1; 之後，兩個容器該由誰負責 delete？
//     答：語言層面沒有答案 —— 這正是問題所在。裸指標不表達所有權，
//         編譯器無從得知誰該負責。只能靠人為約定，而約定會被違反。
//         正解是改用 unique_ptr（獨佔，複製直接編譯失敗）或
//         shared_ptr（共享，引用計數自動處理）讓所有權寫進型別裡。
//     追問：如果團隊規定「只有 v1 能 delete」，這樣可行嗎？
//         → 能跑，但這個規則不在型別系統裡，只在文件或某人的記憶裡。
//           重構、加新人、例外提前 return —— 任何一個都會讓它失效。
//
// ⚠️ 陷阱. 「用 vector<unique_ptr<int>> v2 = v1; 會 double free」——錯在哪？
//     答：錯在它**根本編譯不過**，所以不可能有任何 runtime 行為。
//         unique_ptr 的複製建構子是 deleted，編譯器直接拒絕。
//         這正是 unique_ptr 的價值：把「可能 double free」變成「編譯失敗」。
//         要移動整個容器請寫 vector<unique_ptr<int>> v2 = std::move(v1);
//     為什麼會錯：一般人把「智慧指標」腦補成「加了自動釋放的裸指標」，
//         以為行為一樣、只是多了自動 delete。但 unique_ptr 的重點**不是**
//         自動釋放，而是**用型別系統強制表達「獨佔所有權」**——
//         不可複製正是這個語意的具體實現，不是附帶限制。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <deque>
#include <memory>
#include <string>
using namespace std;

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 138. Copy List with Random Pointer
//   題目：給一個每個節點除了 next 還帶一根 random 指標（指向串列中任意節點或
//         nullptr）的串列，回傳它的**深複製**。
//   為什麼用到本主題：這題就是「淺複製 vs 深複製」的教科書題目。
//         如果只是把每個節點的 random 指標「照抄」，新串列的 random 會指回
//         **舊串列**的節點 —— 這正是 vector<int*> 複製時發生的事。
//         正解要先建立「舊節點 → 新節點」的對照表，再用它翻譯每一根指標。
//   複雜度：時間 O(n)，空間 O(n)（對照表）。
// -----------------------------------------------------------------------------
struct RandomNode {
    int          val;
    RandomNode*  next   = nullptr;
    RandomNode*  random = nullptr;
    explicit RandomNode(int v) : val(v) {}
};

// 用 vector 當對照表（節點事先編號），避免 unordered_map 讓重點失焦
RandomNode* copyRandomList(RandomNode* head,
                           vector<unique_ptr<RandomNode>>& pool) {
    if (!head) return nullptr;

    // 第 1 步：走一遍，替每個舊節點造一個新節點，並記下對應關係
    vector<RandomNode*> oldNodes, newNodes;
    for (RandomNode* p = head; p; p = p->next) {
        oldNodes.push_back(p);
        pool.push_back(make_unique<RandomNode>(p->val));  // 所有權交給 pool
        newNodes.push_back(pool.back().get());
    }

    // 小工具：把「舊節點指標」翻譯成「對應的新節點指標」
    auto translate = [&](RandomNode* oldPtr) -> RandomNode* {
        if (!oldPtr) return nullptr;
        for (size_t i = 0; i < oldNodes.size(); ++i)
            if (oldNodes[i] == oldPtr) return newNodes[i];
        return nullptr;
    };

    // 第 2 步：翻譯每一根指標 —— 這一步就是「深複製」與「淺複製」的分水嶺
    for (size_t i = 0; i < oldNodes.size(); ++i) {
        newNodes[i]->next   = translate(oldNodes[i]->next);
        newNodes[i]->random = translate(oldNodes[i]->random);
    }
    return newNodes[0];
}

// -----------------------------------------------------------------------------
// 【日常實務範例】設定檔的「預設值 → 使用者覆寫」兩層設定物件
//   情境：伺服器啟動時先載入一份預設設定，再依使用者的設定檔複製一份出來修改。
//         若設定項目用裸指標存放，複製出來的那份會**與預設值共用同一塊記憶體**，
//         使用者改自己的設定時會連預設值一起改掉 —— 下一個租戶就拿到被污染的預設值。
//   為什麼用到本主題：這是淺複製 bug 在真實系統裡最常見的長相：
//         它不會崩潰，只是「設定值莫名其妙被別人改掉」，極難追查。
//   解法：改用 shared_ptr（要共享）或直接存值（要獨立）。此處示範存值的版本。
// -----------------------------------------------------------------------------
struct ServerConfig {
    string host;
    int    port;
    int    timeoutSec;
};

// 用值語意：複製出來就是完全獨立的一份，改它不會影響來源
ServerConfig deriveConfig(const ServerConfig& base, int newPort) {
    ServerConfig derived = base;   // 逐成員複製；string 自己做深複製
    derived.port = newPort;
    return derived;
}

int main() {
    cout << "===== 1. vector<int*>：複製的是位址，不是資料 =====" << endl;
    {
        vector<int*> v1;
        v1.push_back(new int(42));
        v1.push_back(new int(99));

        vector<int*> v2(v1);   // 複製建構：逐元素複製「指標值」

        cout << "  *v1[0] = " << *v1[0] << endl;
        cout << "  *v2[0] = " << *v2[0] << endl;
        cout << "  v1[0] 與 v2[0] 是同一個位址嗎? "
             << boolalpha << (v1[0] == v2[0]) << endl;

        // 透過 v2 修改，v1 也會看到變化 —— 意外的別名
        *v2[0] = 777;
        cout << "  透過 v2 改成 777 後，*v1[0] = " << *v1[0]
             << "  ← v1 也被改了！" << endl;

        // ── double free 示範（只說明，不執行）──────────────────────
        //   delete v1[0];   // 第一次釋放：合法
        //   delete v2[0];   // 第二次釋放同一塊 → **未定義行為**
        //   可能被 glibc 偵測到並 abort，也可能靜默破壞 heap 讓程式
        //   稍後在完全無關的地方崩潰。不會有「固定」的結果。
        cout << "  （double free 是未定義行為，本檔只說明、不實際觸發）" << endl;

        // 正確清理：整個程式中每塊記憶體只能 delete 一次
        for (int* p : v1) delete p;
        // 此後 v1 與 v2 裡的指標**全部**成為懸空指標，不可再解參考
        cout << "  已釋放；v1/v2 內的指標現在都是懸空指標，不可再使用" << endl;
    }

    cout << "\n===== 2. deque<int*> 行為完全相同（不是 vector 特有）=====" << endl;
    {
        deque<int*> d1;
        d1.push_back(new int(1000));
        deque<int*> d2(d1);
        cout << "  d1[0] == d2[0] ? " << boolalpha << (d1[0] == d2[0])
             << "  ← deque 也是複製位址" << endl;
        *d2[0] = 2000;
        cout << "  透過 d2 改成 2000 後，*d1[0] = " << *d1[0] << endl;
        for (int* p : d1) delete p;
    }

    cout << "\n===== 3. 正解 A：直接存值 =====" << endl;
    {
        vector<int> v1 = {42, 99};
        vector<int> v2 = v1;      // 元素是 int，複製值本身
        v2[0] = 777;
        cout << "  v1[0] = " << v1[0] << "（不受影響）" << endl;
        cout << "  v2[0] = " << v2[0] << endl;
        cout << "  沒有指標，就沒有 double free / 懸空指標可言" << endl;
    }

    cout << "\n===== 4. 正解 B：shared_ptr（要共享時）=====" << endl;
    {
        vector<shared_ptr<int>> v1;
        v1.push_back(make_shared<int>(42));
        cout << "  複製前 use_count = " << v1[0].use_count() << endl;

        vector<shared_ptr<int>> v2 = v1;   // 共用物件，但引用計數 +1
        cout << "  複製後 use_count = " << v1[0].use_count() << endl;
        *v2[0] = 777;
        cout << "  透過 v2 改 → *v1[0] = " << *v1[0]
             << "（仍共享，但生命週期安全）" << endl;
        v2.clear();
        cout << "  v2 清空後 use_count = " << v1[0].use_count()
             << "  ← 最後一個持有者析構時才真正釋放" << endl;
    }

    cout << "\n===== 5. 正解 C：unique_ptr（獨佔，複製直接編譯失敗）=====" << endl;
    {
        vector<unique_ptr<int>> v1;
        v1.push_back(make_unique<int>(42));

        // vector<unique_ptr<int>> v2 = v1;      // ← 取消註解會**編譯失敗**
        //   error: use of deleted function unique_ptr(const unique_ptr&)
        //   這正是我們要的：double free 的可能性在編譯期就被消滅了

        vector<unique_ptr<int>> v2 = std::move(v1);   // 移動則完全合法
        cout << "  移動後 v1.size() = " << v1.size()
             << ", v2.size() = " << v2.size() << endl;
        cout << "  *v2[0] = " << *v2[0] << "（所有權唯一，離開作用域自動釋放）" << endl;
    }

    cout << "\n===== LeetCode 138. Copy List with Random Pointer =====" << endl;
    {
        // 建原始串列 7 → 13 → 11，random: 7→null, 13→7, 11→13
        vector<unique_ptr<RandomNode>> origPool;
        origPool.push_back(make_unique<RandomNode>(7));
        origPool.push_back(make_unique<RandomNode>(13));
        origPool.push_back(make_unique<RandomNode>(11));
        RandomNode* a = origPool[0].get();
        RandomNode* b = origPool[1].get();
        RandomNode* c = origPool[2].get();
        a->next = b; b->next = c;
        a->random = nullptr; b->random = a; c->random = b;

        vector<unique_ptr<RandomNode>> copyPool;
        RandomNode* copied = copyRandomList(a, copyPool);

        cout << "  原始串列:";
        for (RandomNode* p = a; p; p = p->next)
            cout << " " << p->val << "(random=" << (p->random ? to_string(p->random->val) : "null") << ")";
        cout << endl;
        cout << "  深複製後:";
        for (RandomNode* p = copied; p; p = p->next)
            cout << " " << p->val << "(random=" << (p->random ? to_string(p->random->val) : "null") << ")";
        cout << endl;

        // 關鍵驗證：新串列的指標必須全部指向新串列，不能洩漏到舊串列
        bool leaked = false;
        for (RandomNode* p = copied; p; p = p->next)
            if (p->random == a || p->random == b || p->random == c) leaked = true;
        cout << "  新串列的 random 有指回舊串列嗎? " << boolalpha << leaked
             << "  ← false 才是真正的深複製" << endl;
    }

    cout << "\n===== 日常實務：伺服器設定的預設值/覆寫兩層 =====" << endl;
    {
        ServerConfig defaults{"0.0.0.0", 8080, 30};
        ServerConfig tenantA = deriveConfig(defaults, 9001);
        ServerConfig tenantB = deriveConfig(defaults, 9002);

        cout << "  預設   : " << defaults.host << ":" << defaults.port
             << " timeout=" << defaults.timeoutSec << "s" << endl;
        cout << "  租戶 A : " << tenantA.host << ":" << tenantA.port << endl;
        cout << "  租戶 B : " << tenantB.host << ":" << tenantB.port << endl;
        cout << "  改了租戶 A 之後，預設值的 port 仍是 " << defaults.port
             << "  ← 值語意保證彼此獨立" << endl;
        cout << "  （若 ServerConfig 內是裸指標，這裡就會互相污染且極難追查）" << endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第\ 22\ 課：deque\ 的宣告與初始化2.cpp -o deque_init2

// === 預期輸出 ===
// ===== 1. vector<int*>：複製的是位址，不是資料 =====
//   *v1[0] = 42
//   *v2[0] = 42
//   v1[0] 與 v2[0] 是同一個位址嗎? true
//   透過 v2 改成 777 後，*v1[0] = 777  ← v1 也被改了！
//   （double free 是未定義行為，本檔只說明、不實際觸發）
//   已釋放；v1/v2 內的指標現在都是懸空指標，不可再使用
//
// ===== 2. deque<int*> 行為完全相同（不是 vector 特有）=====
//   d1[0] == d2[0] ? true  ← deque 也是複製位址
//   透過 d2 改成 2000 後，*d1[0] = 2000
//
// ===== 3. 正解 A：直接存值 =====
//   v1[0] = 42（不受影響）
//   v2[0] = 777
//   沒有指標，就沒有 double free / 懸空指標可言
//
// ===== 4. 正解 B：shared_ptr（要共享時）=====
//   複製前 use_count = 1
//   複製後 use_count = 2
//   透過 v2 改 → *v1[0] = 777（仍共享，但生命週期安全）
//   v2 清空後 use_count = 1  ← 最後一個持有者析構時才真正釋放
//
// ===== 5. 正解 C：unique_ptr（獨佔，複製直接編譯失敗）=====
//   移動後 v1.size() = 0, v2.size() = 1
//   *v2[0] = 42（所有權唯一，離開作用域自動釋放）
//
// ===== LeetCode 138. Copy List with Random Pointer =====
//   原始串列: 7(random=null) 13(random=7) 11(random=13)
//   深複製後: 7(random=null) 13(random=7) 11(random=13)
//   新串列的 random 有指回舊串列嗎? false  ← false 才是真正的深複製
//
// ===== 日常實務：伺服器設定的預設值/覆寫兩層 =====
//   預設   : 0.0.0.0:8080 timeout=30s
//   租戶 A : 0.0.0.0:9001
//   租戶 B : 0.0.0.0:9002
//   改了租戶 A 之後，預設值的 port 仍是 8080  ← 值語意保證彼此獨立
//   （若 ServerConfig 內是裸指標，這裡就會互相污染且極難追查）
