// =============================================================================
//  第 20 課：vector 效能分析與最佳實踐 2  —  reserve 還是 resize？三種場景
// =============================================================================
//
// 【主題資訊 Information】
//   void reserve(size_type n);              // 只擴容量：capacity >= n，size 不變
//   void resize (size_type n);              // 改變元素個數：size == n，新元素 value-init
//   void resize (size_type n, const T& v);  // 同上，但新元素用 v 拷貝建構
//   void shrink_to_fit();                   // 「請求」把 capacity 降到 size —— 非強制
//
//   標準版本：reserve / resize 為 C++98；shrink_to_fit 為 C++11
//   複雜度  ：需要重新配置時 O(n)，否則 O(1)（resize 縮小時是 O(被銷毀的個數)）
//   標頭檔  ：<vector>
//
// 【詳細解釋 Explanation】
//
// 【1. 一句話分辨：capacity 是「地」，size 是「房子」】
//   * reserve(n) = 買地。地有了（capacity），但上面一棟房子都還沒蓋（size 仍 0）。
//                  你不能搬進去住 —— v[0] 此時是未定義行為。
//   * resize(n)  = 直接蓋好 n 棟空房子。size 變成 n，每個元素都被 value-initialize
//                  （int/double 是 0，std::string 是空字串，class 呼叫預設建構子）。
//                  你可以立刻 v[i] = ...。
//
//   兩者共通點：都可能觸發重新配置，因此都會讓既有 iterator/pointer/reference 失效。
//
// 【2. 三種場景的正確選擇】
//
//   場景一：知道總量，而且要逐一 push_back
//       -> reserve。零重新配置，而且不會白白付出「先 value-init 再覆寫」的成本。
//
//   場景二：需要用索引直接寫入，或要把 buffer 交給 C API 去填
//       -> resize。因為 C API（read()、recv()、fread()、memcpy 目的地…）
//          會直接往 data() 指向的記憶體寫，那些位置必須已經有「真正的物件」。
//          只 reserve 就把 data() 交出去 = 對未建構的儲存空間寫入 = 未定義行為。
//
//   場景三：不知道確切總量，但抓得到上界或合理估計
//       -> reserve(估計值)。即使實際只用了 480/500，也已經把
//          9 次左右的重新配置壓成 0 次。估多了只是暫時多佔記憶體，
//          真的在意可以最後 shrink_to_fit()。
//
// 【3. resize 的隱藏成本：值初始化不是免費的】
//   `std::vector<int> v(N);` 會把 N 個 int 全部寫成 0（實務上是一次 memset）。
//   如果你接下來要把每一格都覆寫掉，這次歸零就是純浪費。
//   所以：
//     * 要覆寫全部 -> 其實 reserve + push_back 反而少一次寫入
//     * 要當隨機存取的畫布（只寫一部分）-> resize 才對，因為沒寫到的格子
//       必須有確定的值
//   實務上兩者差距通常不大（memset 很快），但概念要清楚：
//   resize 付的是「初始化成本」，reserve 付的是「逐次 push_back 的檢查成本」。
//
// 【4. shrink_to_fit 是「請求」不是「命令」】
//   標準用字是 non-binding request：實作可以完全忽略它。
//   libstdc++ 的實作是「建立一個剛好大小的暫時 vector 再 swap」，
//   所以本機實測會真的把 capacity 降下來——但那是實作行為，不是保證。
//   而且它可能重新配置 -> 一樣讓 iterator 全部失效。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼 clear() 不還記憶體？
//     clear() 的規格是「銷毀所有元素，size 變 0」，並且明確規定
//     **不改變 capacity**。理由是 vector 常被當作可重複使用的緩衝區：
//         while (有下一批資料) { buf.clear(); 填 buf; 處理 buf; }
//     如果 clear() 順手把記憶體還掉，這個迴圈每一輪都要重新配置，
//     等於把「緩衝區重用」這個最重要的最佳化毀掉。
//     本機實測：reserve(1000) 後 clear()，capacity 仍是 1000。
//
// (B) 真的要立刻釋放記憶體：swap trick
//     C++11 之前沒有 shrink_to_fit，慣用法是
//         std::vector<int>(v).swap(v);    // 建一個剛好大小的副本，再交換
//     或要完全清空並釋放：
//         std::vector<int>().swap(v);     // 和一個空的暫時物件交換
//     暫時物件在該行結束時解構，把舊的大塊記憶體帶走。
//     這招在今天仍然有用，因為它「一定」會換掉緩衝區，
//     而 shrink_to_fit 在規格上可以不做事。
//
// (C) reserve 的容量是精確的
//     本機實測 reserve(1000) 之後 capacity() 就是 1000，不會被進位到 1024。
//     這是 libstdc++ 的行為（reserve 直接配置所要求的量）；
//     倍增只發生在「push_back 撞到上限」的時候。
//
// 【注意事項 Pay Attention】
//
// 1. reserve 之後 size() 還是 0：此時 v[0]、v.front()、v.back() 都是未定義行為。
//    要判斷「有沒有東西」永遠看 size()/empty()，不要看 capacity()。
// 2. resize 縮小時會「銷毀」多出來的元素，但通常**不會**降低 capacity。
// 3. reserve 不能縮小容量；reserve(比目前小的值) 是 no-op。
// 4. shrink_to_fit 是 non-binding request，標準允許實作什麼都不做。
//    不要寫出「呼叫了就一定省下記憶體」的程式邏輯。
// 5. reserve / resize / shrink_to_fit 只要真的重新配置，
//    先前的 iterator、reference、pointer（含 data()）全部失效。
// 6. resize(n) 對自訂型別要求 DefaultInsertable；沒有預設建構子的型別
//    只能用 resize(n, value) 這個版本。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】reserve / resize / capacity
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. reserve(1000) 之後，v.size() 和 v.capacity() 各是多少？
//     答：size() = 0，capacity() >= 1000（本機 libstdc++ 實測剛好 1000）。
//         reserve 只保證容量，完全不建構元素。
//     追問：那 resize(1000) 呢？
//         → size() = 1000、capacity() >= 1000，而且 1000 個元素都被
//           value-initialize 成 0。
//
// 🔥 Q2. clear() 之後 capacity 會變 0 嗎？
//     答：不會。標準規定 clear() 只把 size 變 0、銷毀元素，capacity 不變。
//         本機實測 reserve(1000) 後 clear()，capacity 仍是 1000。
//     追問：那要怎麼真的把記憶體還回去？
//         → shrink_to_fit()（但它是 non-binding request），
//           或用保證有效的 swap trick：std::vector<int>().swap(v);
//
// ⚠️ 陷阱. 這段程式碼有什麼問題？
//         std::vector<char> buf;
//         buf.reserve(4096);
//         ssize_t n = read(fd, buf.data(), 4096);   // 把 buffer 交給 C API
//         buf.resize(n);
//     答：未定義行為。reserve 只配置了原始儲存空間，那 4096 個 char 物件
//         **還沒有被建構**，把 data() 交給 read() 去寫是對未建構儲存空間寫入。
//         正確寫法是先 buf.resize(4096) 再 read，最後 resize(n) 縮回實際長度。
//     為什麼會錯：大家把 reserve 想成「malloc 一塊 buffer」，
//         但 vector 的抽象是「物件的序列」，不是「一塊記憶體」——
//         size() 之外的區域在型別系統的意義上並不存在。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 1】LeetCode 88. Merge Sorted Array
//   題目：nums1 長度為 m+n，前 m 個是有效元素、後 n 個是預留的 0；
//         把有序的 nums2（n 個）就地合併進 nums1，結果仍有序。
//   為什麼用到本主題：這題的介面本身就是「resize 語意」的最佳教材——
//     nums1 不是「容量 m+n、長度 m」，而是**長度就是 m+n**、後段已經有
//     真正存在的元素（值為 0）。所以我們能直接用 nums1[k] 從後往前寫。
//     如果它只是 reserve 出來的容量，這種寫法就會是未定義行為。
// -----------------------------------------------------------------------------
void mergeSortedArray(std::vector<int>& nums1, int m, const std::vector<int>& nums2, int n) {
    int i = m - 1;          // nums1 有效段的最後一個
    int j = n - 1;          // nums2 的最後一個
    int k = m + n - 1;      // 寫入位置（從最尾巴往前填）

    while (j >= 0) {
        if (i >= 0 && nums1[i] > nums2[j]) {
            nums1[k--] = nums1[i--];
        } else {
            nums1[k--] = nums2[j--];
        }
    }
}

// -----------------------------------------------------------------------------
// 【日常實務範例 1】音訊取樣緩衝區：固定長度、逐格覆寫、可重複使用
//   情境：DSP 迴圈每次要處理固定 frameSize 個樣本。
//     * 緩衝區用 resize 建立 —— 因為我們是用索引直接寫入，不是 push_back
//     * 每一輪直接覆寫，**不要**重新建立 vector：
//       capacity 留著，整個處理迴圈零配置
//   這是即時音訊／影像處理最基本的紀律：熱迴圈裡不配置記憶體。
// -----------------------------------------------------------------------------
class SampleBuffer {
    std::vector<float> buf_;

public:
    explicit SampleBuffer(std::size_t frameSize) {
        buf_.resize(frameSize);      // resize 而非 reserve：接下來要用 buf_[i] 寫
    }

    // 模擬「填入一個 frame」：直接用索引覆寫，不新增元素 -> 完全不配置記憶體
    void fillWithRamp(float start, float step) {
        for (std::size_t i = 0; i < buf_.size(); ++i) {
            buf_[i] = start + step * static_cast<float>(i);
        }
    }

    float peak() const {
        float m = 0.0f;
        for (float s : buf_) m = std::max(m, s < 0 ? -s : s);
        return m;
    }

    std::size_t size()     const { return buf_.size(); }
    std::size_t capacity() const { return buf_.capacity(); }
};

int main() {
    std::cout << "=== 1. 場景一：預知總量 + 逐一 push_back -> reserve ===" << std::endl;
    {
        std::vector<int> v;
        v.reserve(1000);                       // size 仍為 0
        std::cout << "  reserve(1000) 直後: size=" << v.size()
                  << ", capacity=" << v.capacity() << std::endl;
        for (int i = 0; i < 1000; ++i) v.push_back(i);   // 全程不重新配置
        std::cout << "  填完 1000 個:      size=" << v.size()
                  << ", capacity=" << v.capacity() << std::endl;
    }

    std::cout << "\n=== 2. 場景二：要用索引寫入／交給 C API -> resize ===" << std::endl;
    {
        std::vector<int> v;
        v.resize(1000);                        // size 變 1000，元素全被歸零
        std::cout << "  resize(1000) 直後: size=" << v.size()
                  << ", capacity=" << v.capacity()
                  << ", v[0]=" << v[0] << " (value-initialized)" << std::endl;
        for (int i = 0; i < 1000; ++i) v[i] = i;         // 合法：元素已經存在
        std::cout << "  索引賦值後:        v[999]=" << v[999] << std::endl;
    }

    std::cout << "\n=== 3. 場景三：只有估計值 -> reserve(上界) ===" << std::endl;
    {
        std::vector<int> v;
        v.reserve(500);                        // 估計約 500 個
        for (int i = 0; i < 480; ++i) v.push_back(i);    // 實際只用了 480
        std::cout << "  size=" << v.size() << ", capacity=" << v.capacity()
                  << "  (估多了 20 格，但省下所有重新配置)" << std::endl;
        v.shrink_to_fit();                     // 非強制請求；本機實作會照做
        std::cout << "  shrink_to_fit() 後: size=" << v.size()
                  << ", capacity=" << v.capacity()
                  << "  (本機 libstdc++ 行為，標準未保證)" << std::endl;
    }

    std::cout << "\n=== 4. clear() 不還記憶體（緩衝區重用的基礎）===" << std::endl;
    {
        std::vector<int> v;
        v.reserve(1000);
        for (int i = 0; i < 100; ++i) v.push_back(i);
        std::cout << "  clear() 前: size=" << v.size() << ", capacity=" << v.capacity() << std::endl;
        v.clear();
        std::cout << "  clear() 後: size=" << v.size() << ", capacity=" << v.capacity()
                  << "  (容量留著，下一輪不必再配置)" << std::endl;

        std::vector<int>().swap(v);            // swap trick：保證換掉緩衝區
        std::cout << "  swap trick 後: size=" << v.size() << ", capacity=" << v.capacity()
                  << "  (這招不依賴 shrink_to_fit 的善意)" << std::endl;
    }

    std::cout << "\n=== 5. LeetCode 88. Merge Sorted Array ===" << std::endl;
    {
        std::vector<int> nums1 = {1, 2, 3, 0, 0, 0};
        std::vector<int> nums2 = {2, 5, 6};
        mergeSortedArray(nums1, 3, nums2, 3);
        std::cout << "  ";
        for (int x : nums1) std::cout << x << " ";
        std::cout << std::endl;

        std::vector<int> a = {0};
        std::vector<int> b = {1};
        mergeSortedArray(a, 0, b, 1);
        std::cout << "  ";
        for (int x : a) std::cout << x << " ";
        std::cout << std::endl;
    }

    std::cout << "\n=== 6. 日常實務：音訊取樣緩衝區 ===" << std::endl;
    {
        SampleBuffer sb(512);
        std::cout << "  建立後 size=" << sb.size() << " capacity=" << sb.capacity() << std::endl;
        sb.fillWithRamp(0.0f, 0.001f);
        std::cout << "  第 1 個 frame peak = " << sb.peak() << std::endl;
        sb.fillWithRamp(0.0f, 0.002f);          // 第二輪：純覆寫，零配置
        std::cout << "  第 2 個 frame peak = " << sb.peak() << std::endl;
        std::cout << "  處理兩個 frame 後 capacity 仍是 " << sb.capacity()
                  << "（熱迴圈零配置）" << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第 20 課：vector 效能分析與最佳實踐2.cpp -o lesson20_2

// === 預期輸出 ===
// ※ 本檔沒有計時，輸出是決定性的（同一實作下每次都相同）。
// ※ 但 capacity 的實際數值屬「實作定義」：以下為本機 libstdc++ (g++ 15.2.0) 實測。
//
// === 1. 場景一：預知總量 + 逐一 push_back -> reserve ===
//   reserve(1000) 直後: size=0, capacity=1000
//   填完 1000 個:      size=1000, capacity=1000
//
// === 2. 場景二：要用索引寫入／交給 C API -> resize ===
//   resize(1000) 直後: size=1000, capacity=1000, v[0]=0 (value-initialized)
//   索引賦值後:        v[999]=999
//
// === 3. 場景三：只有估計值 -> reserve(上界) ===
//   size=480, capacity=500  (估多了 20 格，但省下所有重新配置)
//   shrink_to_fit() 後: size=480, capacity=480  (本機 libstdc++ 行為，標準未保證)
//
// === 4. clear() 不還記憶體（緩衝區重用的基礎）===
//   clear() 前: size=100, capacity=1000
//   clear() 後: size=0, capacity=1000  (容量留著，下一輪不必再配置)
//   swap trick 後: size=0, capacity=0  (這招不依賴 shrink_to_fit 的善意)
//
// === 5. LeetCode 88. Merge Sorted Array ===
//   1 2 2 3 5 6
//   1
//
// === 6. 日常實務：音訊取樣緩衝區 ===
//   建立後 size=512 capacity=512
//   第 1 個 frame peak = 0.511
//   第 2 個 frame peak = 1.022
//   處理兩個 frame 後 capacity 仍是 512（熱迴圈零配置）
