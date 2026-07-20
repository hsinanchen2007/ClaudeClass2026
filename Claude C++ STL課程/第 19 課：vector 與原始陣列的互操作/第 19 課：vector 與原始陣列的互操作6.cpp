// =============================================================================
//  第 19 課：vector 與原始陣列的互操作 6  —  reserve vs resize：緩衝區的致命分野
// =============================================================================
//
// 【主題資訊 Information】
//   void reserve(size_type n);   // 只確保 capacity() >= n；size() 完全不變
//   void resize (size_type n);   // 讓 size() 變成 n；新增的元素被「值初始化」
//   void resize (size_type n, const T& value);   // 新增的元素複製自 value
//
//   標頭檔 : <vector>
//   複雜度 : reserve 為 O(size())（需要搬移既有元素）；
//            resize 放大為 O(新增量 + 可能的搬移)，縮小為 O(縮減量)。
//   關鍵   : 要把緩衝區交給別人「直接寫入」，唯一正確的是 resize（或建構子指定大小）。
//
// 【詳細解釋 Explanation】
//
// 【1. vector 的兩個長度：size 與 capacity】
//   vector 內部維護三根指標，對應兩個「長度」概念：
//       size()     = 目前有幾個「已建構的元素」→ 決定 begin()/end()、range-for 的範圍
//       capacity() = 目前配置的記憶體能放下幾個元素 → 決定何時需要重新配置
//   永遠 size() <= capacity()。
//   reserve 只動 capacity，resize 會動 size（必要時連帶動 capacity）。
//
// 【2. 為什麼 reserve + data() 是錯的】
//   reserve(5) 之後，記憶體確實配好了 20 bytes（本機 sizeof(int)==4），
//   data() 也會回傳一個非空指標。看起來一切正常，但：
//
//   (a) 那塊區域上「沒有物件」。C++ 物件模型認為那只是原始儲存空間（raw storage）。
//       把值寫進去，是在尚未開始生命週期的儲存空間上寫入，屬於未定義行為。
//       實務上對 int 這種 trivial 型別，在 libstdc++ 上通常「看起來會動」，
//       但那不是保證——標準沒有承諾任何結果，換編譯器/最佳化等級就可能不同。
//
//   (b) 就算寫進去了，vector 也不知道。size() 仍是 0，
//       所以 begin() == end()，range-for 一次都不跑，你「看不到」自己寫的資料。
//
//   (c) 更糟的是之後 push_back：它從索引 0 開始放，
//       直接覆蓋掉你辛苦填的內容，而且完全沒有任何警告。
//
// 【3. resize 為什麼是對的】
//   resize(5) 做兩件事：確保容量足夠、並「真的建構」出 5 個元素（值初始化為 0）。
//   之後 size() == 5，[data(), data()+5) 上的物件確實存在，
//   寫入完全合法、range-for 看得到、push_back 會接在第 6 個位置。
//
// 【4. 那 reserve 什麼時候用】
//   reserve 是為了「避免多次重新配置」而生，適用於「接下來我要 push_back N 次」：
//       v.reserve(n);
//       for (...) v.push_back(x);     // 全程不會重新配置，指標/迭代器也不失效
//   它的價值在效能，不在「準備一塊給別人寫的緩衝區」。兩者要解決的問題不同。
//
//   如果想要「既不付值初始化成本、又能安全填資料」，正解不是 reserve + data()，
//   而是 reserve + back_inserter / push_back——透過 vector 自己的介面寫入，
//   size 才會跟著長。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼 vector 不能「假裝」那些元素存在
//   有人會問：既然記憶體配好了，vector 為什麼不乾脆把 size 也設成 capacity？
//   因為對非 trivial 型別（例如 vector<std::string>），
//   「元素存在」意味著建構子已經跑過、解構子將來要跑。
//   如果 vector 把未建構的儲存空間當成元素，解構時就會對垃圾資料呼叫 ~string()，
//   那是立刻的災難。size 與 capacity 分離，正是為了精確追蹤「哪些物件真的活著」。
//
// (B) 縮小的 resize 不會釋放記憶體
//   v.resize(3) 從 5 縮到 3，會解構掉後面 2 個元素，但 capacity 不變、
//   記憶體不歸還。這是刻意的設計（避免縮放來回抖動時反覆配置）。
//   真的要歸還記憶體，C++11 起有 shrink_to_fit()，但它是「非強制的請求」，
//   實作可以忽略。
//
// (C) capacity 的成長倍率是實作定義的
//   標準只要求 push_back 的攤銷複雜度為 O(1)，沒有規定成長倍率。
//   libstdc++ 用 2 倍，MSVC 用 1.5 倍。本檔印出的 capacity 變化屬於
//   libstdc++ 15.2 的本機實測值，不可當成跨平台保證。
//
// 【注意事項 Pay Attention】
//   1. 要給 C 函式寫入的緩衝區，一律用 resize 或 vector<T> v(n)，絕不用 reserve。
//   2. reserve 之後 data() 非空，但那塊區域上沒有元素，寫入是未定義行為。
//   3. reserve 之後 size() 仍是 0，range-for 看不到任何東西。
//   4. reserve 之後 push_back 從索引 0 開始，會覆蓋你直接寫進去的內容。
//   5. capacity 的成長策略是實作定義的，不要寫依賴特定倍率的程式碼。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】reserve vs resize
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. reserve(n) 和 resize(n) 差在哪裡？
//     答：reserve 只保證 capacity() >= n，size() 完全不變，不建構任何元素；
//         resize 讓 size() 變成 n，會真的建構（或解構）元素，
//         新增的部分被值初始化。
//         一句話記法：reserve 準備「空間」，resize 準備「元素」。
//     追問：兩者都可能使既有的迭代器/指標失效嗎？
//         → 是。只要造成重新配置就會失效；resize 縮小則不會（capacity 不變）。
//
// 🔥 Q2. 為什麼「reserve(n) 之後把 data() 交給 C 函式寫入」是錯的？
//     答：兩個層次的錯。第一，那塊儲存空間上還沒有物件，
//         寫入是未定義行為（標準不保證任何結果，雖然對 int 在多數實作上看似可行）。
//         第二，就算寫進去了 size() 仍是 0，vector 完全不知道有資料，
//         range-for 看不到，之後 push_back 還會從索引 0 覆蓋掉。
//         正解是 resize(n) 或 vector<T> v(n)。
//     追問：那我 reserve 之後手動把 size 改掉不行嗎？
//         → 標準容器沒有提供修改 size 的介面（這是刻意的）。
//           C++23 的 resize_and_overwrite 只給 std::string，vector 目前沒有對應設施。
//
// ⚠️ 陷阱. 「我 reserve(5) 又用 data() 寫了 5 筆，印出來卻什麼都沒有，是不是編譯器的 bug？」
//     答：不是 bug，是 size() 仍然是 0。range-for 走的是 begin() 到 end()，
//         而 end() 由 size() 決定，所以迴圈一次都不跑。
//         資料可能真的躺在那塊記憶體裡，但 vector 完全不認得它們。
//         而且那次寫入本身就是未定義行為，不能依賴「資料還在」這件事。
//     為什麼會錯：把 capacity 誤當成 size，以為「配了 5 格就等於有 5 個元素」。
//         正確的心智模型是：capacity 是「停車場有幾個車位」，
//         size 是「現在停了幾台車」。你把車開進沒有登記的車位，
//         管理員的帳上依然是 0 台——而且下一台車會直接停到你頭上。
// ═══════════════════════════════════════════════════════════════════════════

#include <cstddef>
#include <iostream>
#include <vector>

void fill_data(int* buffer, int count) {
    for (int i = 0; i < count; ++i) {
        buffer[i] = i * 10;
    }
}

static void dump(const char* label, const std::vector<int>& v) {
    std::cout << label << "size=" << v.size() << " capacity=" << v.capacity()
              << " 內容=[";
    for (std::size_t i = 0; i < v.size(); ++i) std::cout << (i ? " " : "") << v[i];
    std::cout << "]\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 88. Merge Sorted Array
//   題目：nums1 的長度是 m + n，但只有前 m 個是有效資料，後面 n 個是預留空位（0）；
//         把長度為 n 的 nums2 併入 nums1，結果就地排序。
//   為什麼用到本主題：這題把「物理容量」與「邏輯筆數」的差別直接寫進題目——
//         nums1.size() 是 m+n（空間），但有效資料只有 m 筆。
//         這正是 reserve/resize 之爭的核心：緩衝區有多大，和裡面有幾筆真資料，
//         是兩件必須分開追蹤的事。解法從後往前填，也正是因為尾端是預留空間。
//   複雜度：時間 O(m + n)，空間 O(1)。
// -----------------------------------------------------------------------------
void merge(std::vector<int>& nums1, int m, const std::vector<int>& nums2, int n) {
    int i = m - 1;          // nums1 有效資料的最後一個
    int j = n - 1;          // nums2 的最後一個
    int k = m + n - 1;      // 寫入位置（從最尾端開始）

    while (j >= 0) {
        if (i >= 0 && nums1[static_cast<std::size_t>(i)] > nums2[static_cast<std::size_t>(j)]) {
            nums1[static_cast<std::size_t>(k--)] = nums1[static_cast<std::size_t>(i--)];
        } else {
            nums1[static_cast<std::size_t>(k--)] = nums2[static_cast<std::size_t>(j--)];
        }
    }
}

// -----------------------------------------------------------------------------
// 【日常實務範例】從 socket 讀取一個網路封包
//   情境：TCP 連線上要讀取最多 MAX_PACKET 位元組。recv() 是 C 函式，
//         需要一塊「已存在」的緩衝區，並回傳「實際讀到幾個位元組」
//         （可能遠少於緩衝區大小，因為 TCP 是串流、不保證一次收滿）。
//   為何是本主題：初學者最常見的錯誤就是 buf.reserve(MAX_PACKET) 然後
//         recv(buf.data(), MAX_PACKET)——編譯過、跑起來也「好像有資料」，
//         但 buf.size() 是 0，後續所有處理都拿到空的內容。
//         正解是 resize 到最大長度、收完再 resize 到實際長度。
// -----------------------------------------------------------------------------
long fake_recv(char* buf, long capacity, const char* payload, long payloadLen) {
    long n = (payloadLen < capacity) ? payloadLen : capacity;   // 假裝這是 recv()
    for (long i = 0; i < n; ++i) buf[i] = payload[i];
    return n;                                                    // 實際讀到的位元組數
}

std::vector<char> readOnePacket(long maxPacket, const char* payload, long payloadLen) {
    std::vector<char> buf;
    buf.resize(static_cast<std::size_t>(maxPacket));   // 元素真的存在，才能給 recv 寫
    long got = fake_recv(buf.data(), static_cast<long>(buf.size()), payload, payloadLen);
    if (got < 0) return {};
    buf.resize(static_cast<std::size_t>(got));         // 收斂到實際收到的長度
    return buf;
}

int main() {
    // -------------------------------------------------------------------------
    std::cout << "=== 錯誤做法：reserve + data() 寫入 ===\n";
    // -------------------------------------------------------------------------
    {
        std::vector<int> v;
        v.reserve(5);                 // 只配置空間，一個元素都沒有
        dump("reserve(5) 後：", v);

        // ⚠️ 下面這一行是未定義行為：在尚未建構物件的儲存空間上寫入。
        //    標準不保證任何結果。本機 libstdc++ + int（trivial 型別）看似可行，
        //    但這是實作巧合，不是語言保證，換平台/最佳化等級都可能不同。
        fill_data(v.data(), 5);

        dump("寫入 5 筆後：", v);
        std::cout << "  ↑ size 仍是 0，range-for 一次都不跑，看不到任何資料\n";

        // 更糟的是：接下來 push_back 會從索引 0 開始覆蓋
        v.push_back(999);
        dump("push_back(999) 後：", v);
        std::cout << "  ↑ 999 蓋在索引 0，剛才寫的 0 被覆蓋掉了\n";
    }

    // -------------------------------------------------------------------------
    std::cout << "\n=== 正確做法：resize + data() 寫入 ===\n";
    // -------------------------------------------------------------------------
    {
        std::vector<int> v;
        v.resize(5);                  // 真的建構出 5 個元素（值初始化為 0）
        dump("resize(5) 後：", v);

        fill_data(v.data(), static_cast<int>(v.size()));   // 完全合法
        dump("寫入 5 筆後：", v);

        v.push_back(999);
        dump("push_back(999) 後：", v);
        std::cout << "  ↑ 999 正確接在第 6 個位置，資料完好\n";
    }

    // -------------------------------------------------------------------------
    std::cout << "\n=== reserve 的正確用途：避免多次重新配置 ===\n";
    // -------------------------------------------------------------------------
    {
        std::vector<int> noReserve;
        std::size_t reallocCount = 0, lastCap = noReserve.capacity();
        for (int i = 0; i < 20; ++i) {
            noReserve.push_back(i);
            if (noReserve.capacity() != lastCap) { ++reallocCount; lastCap = noReserve.capacity(); }
        }
        std::cout << "不 reserve：push_back 20 次，capacity 變動 "
                  << reallocCount << " 次，最終 capacity = "
                  << noReserve.capacity() << "\n";

        std::vector<int> withReserve;
        withReserve.reserve(20);
        reallocCount = 0; lastCap = withReserve.capacity();
        for (int i = 0; i < 20; ++i) {
            withReserve.push_back(i);
            if (withReserve.capacity() != lastCap) { ++reallocCount; lastCap = withReserve.capacity(); }
        }
        std::cout << "先 reserve(20)：push_back 20 次，capacity 變動 "
                  << reallocCount << " 次，最終 capacity = "
                  << withReserve.capacity() << "\n";
        std::cout << "  （成長倍率為實作定義；以上為 libstdc++ 15.2 本機實測）\n";
    }

    // -------------------------------------------------------------------------
    std::cout << "\n=== LeetCode 88. Merge Sorted Array ===\n";
    // -------------------------------------------------------------------------
    {
        std::vector<int> nums1 = {1, 2, 3, 0, 0, 0};   // size 6，有效資料只有前 3 筆
        std::vector<int> nums2 = {2, 5, 6};
        std::cout << "合併前 nums1 = [1 2 3 0 0 0]，m = 3（後 3 格是預留空間）\n";
        merge(nums1, 3, nums2, 3);
        std::cout << "合併後 nums1 = [";
        for (std::size_t i = 0; i < nums1.size(); ++i) std::cout << (i ? " " : "") << nums1[i];
        std::cout << "]\n";

        std::vector<int> a = {1};
        std::vector<int> b;
        merge(a, 1, b, 0);
        std::cout << "邊界 m=1,n=0 → [" << a[0] << "]\n";
    }

    // -------------------------------------------------------------------------
    std::cout << "\n=== 日常實務：從 socket 讀一個封包 ===\n";
    // -------------------------------------------------------------------------
    {
        const char* payload = "GET /index.html HTTP/1.1";
        std::vector<char> pkt = readOnePacket(1500, payload, 24);
        std::cout << "緩衝區宣告 1500 bytes，實際收到 " << pkt.size() << " bytes\n";
        std::cout << "內容：";
        for (char c : pkt) std::cout << c;
        std::cout << "\n";
        std::cout << "  ↑ size() 是真實資料長度，可以安心交給後續解析器\n";
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第\ 19\ 課：vector\ 與原始陣列的互操作6.cpp -o interop6

// === 預期輸出 ===
// === 錯誤做法：reserve + data() 寫入 ===
// reserve(5) 後：size=0 capacity=5 內容=[]
// 寫入 5 筆後：size=0 capacity=5 內容=[]
//   ↑ size 仍是 0，range-for 一次都不跑，看不到任何資料
// push_back(999) 後：size=1 capacity=5 內容=[999]
//   ↑ 999 蓋在索引 0，剛才寫的 0 被覆蓋掉了
//
// === 正確做法：resize + data() 寫入 ===
// resize(5) 後：size=5 capacity=5 內容=[0 0 0 0 0]
// 寫入 5 筆後：size=5 capacity=5 內容=[0 10 20 30 40]
// push_back(999) 後：size=6 capacity=10 內容=[0 10 20 30 40 999]
//   ↑ 999 正確接在第 6 個位置，資料完好
//
// === reserve 的正確用途：避免多次重新配置 ===
// 不 reserve：push_back 20 次，capacity 變動 6 次，最終 capacity = 32
// 先 reserve(20)：push_back 20 次，capacity 變動 0 次，最終 capacity = 20
//   （成長倍率為實作定義；以上為 libstdc++ 15.2 本機實測）
//
// === LeetCode 88. Merge Sorted Array ===
// 合併前 nums1 = [1 2 3 0 0 0]，m = 3（後 3 格是預留空間）
// 合併後 nums1 = [1 2 2 3 5 6]
// 邊界 m=1,n=0 → [1]
//
// === 日常實務：從 socket 讀一個封包 ===
// 緩衝區宣告 1500 bytes，實際收到 24 bytes
// 內容：GET /index.html HTTP/1.1
//   ↑ size() 是真實資料長度，可以安心交給後續解析器
