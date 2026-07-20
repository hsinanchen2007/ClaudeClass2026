/*
================================================================================
主題:std::bitset —— 固定大小的位元集合
標準:C++98 起
標頭:<bitset>
參考:https://en.cppreference.com/w/cpp/utility/bitset
================================================================================

【一、課題介紹】
  std::bitset<N> 是一個「N 個 bit 的容器」,提供整數位元運算的便捷介面。
  你可以把它想成「比 unsigned long 更通用、可表達任意大小、且有完整工具方法的
  位元陣列」。

  為什麼需要它?
    - 旗標管理:同時記錄多個 boolean 狀態時,bitset 比 vector<bool> 直觀。
    - 篩法 / 集合運算:埃式篩法、子集合枚舉、權限位元組合等。
    - 比 vector<bool> 快、無 proxy reference 怪異行為(vector<bool> 因為
      包了 proxy reference,常常踩坑)。

【二、觀念解釋】
  1. 標頭:<bitset>。N 必須是「編譯期常數」。
  2. 建立:
       std::bitset<8> b;                  // 全部為 0
       std::bitset<8> b2(0b10101010);     // 從整數初始化
       std::bitset<8> b3(std::string("11001100"));   // 從字串
  3. 常用成員:
       b[i]              // 第 i 個 bit(可讀寫)
       b.test(i)         // 同 b[i],但越界擲例外
       b.set(i [, val])  // 設 1 或指定值
       b.reset(i)        // 設 0
       b.flip(i)         // 反轉
       b.count()         // 1 的個數
       b.size()          // 總 bit 數
       b.any() / .all() / .none()
       b.to_ulong() / .to_ullong() / .to_string()
  4. 整套位元運算子:&、|、^、~、<<、>>、==。

【三、常見陷阱】
  - 大小 N 是「編譯期常數」,動態大小不行,要動態請用 std::vector<bool>
    (注意 vector<bool> 是 proxy 容器)。
  - to_ulong() 在 N > 32 時可能溢位擲例外;to_ullong() 同理(N > 64)。
  - 位元順序:bitset 的 [0] 是「最低位元(右側)」,印 to_string() 時會「最高位在左」。

【四、與其他 utility 的比較】
  - vs unsigned int 位元運算:bitset 介面友善、能任意大小;單純位元 hack 用整數即可。
  - vs std::vector<bool>:vector<bool> 大小可動態變、但有 proxy reference 怪異;
    bitset 大小固定、行為單純。
  - vs std::set<int>:bitset 對「值域小且密集」的集合操作快很多。

【五、Leetcode 對應題目】
  題號:204. Count Primes(質數計數)
  難度:Medium(本身觀念簡單,屬於入門級)
  連結:https://leetcode.com/problems/count-primes/
  題目大意:回傳小於 n 的質數個數。
  選用理由:埃式篩法是 bitset 的經典應用,記錄「i 是否合數」用一個 bit 即可,
            記憶體大幅節省、快取更友善。

【六、日常工作實用範例】
  情境:用 bitset 表達「使用者權限」(讀、寫、刪除、管理員、稽核 ...),
        比寫一堆 bool 成員或單純 int 位元運算清楚很多。
================================================================================
*/

/*
補充筆記：std::bitset
  - std::bitset 屬於 utility 類工具；這些型別與函式常用來表達小型資料組合、可選值、型別安全聯集或值類別轉換。
  - pair/tuple 適合簡短聚合結果，但欄位語意複雜時應定義具名 struct，避免 first/second 或 get<0> 難讀。
  - optional 表示可能沒有值，使用前要檢查 has_value 或使用 value_or；value() 在無值時會丟例外。
  - variant 表示多選一型別，應用 visit 或 holds_alternative/get_if 安全存取目前替代項。
  - any 提供執行期任意型別保存，但取回需要知道正確型別；過度使用會失去靜態型別檢查優勢。
  - std::move/std::forward/std::exchange/as_const 都是表達意圖的工具；它們本身不一定搬移或複製資料。
  - std::bitset<N> 的大小 N 是編譯期常數，適合固定長度旗標或小型 bit 集合。
  - bitset 支援 count、test、set、reset、flip，比手寫整數遮罩更易讀。
  - to_ulong/to_ullong 若 bitset 數值放不進目標型別會丟 overflow_error。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::bitset
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::bitset 適合什麼場景？和 std::vector<bool> 差在哪？
//     答：bitset<N> 的大小在編譯期固定、就地儲存、不做堆配置，提供 count()、any()、
//     all()、test()、flip()、位元運算與 to_string()。vector<bool> 則是動態大小的特化，
//     也是有名的「不是真正的容器」——operator[] 回傳 proxy 而不是 bool&，不能取位址、
//     不滿足容器需求。要動態大小又想避開 proxy，可用 vector<std::uint64_t> 自行管理或
//     boost::dynamic_bitset。
//     追問：bitset 的 count() 大概怎麼實作？（以字為單位處理，實作通常直接用 popcount 指令）
//
// 🔥 Q2. bitset 越界存取會怎樣？
//     答：operator[] 不做邊界檢查（越界是 UB）；test(pos) 會檢查並在越界時拋
//     std::out_of_range。這與 vector 的 operator[] vs at() 是同一組設計哲學。
//
// ⚠️ 陷阱. 要把 bitset 當旗標傳給 C API，直接 to_ulong() 就好嗎？
//     答：要小心。to_ulong()／to_ullong() 在數值無法用目標型別表示時會拋
//     std::overflow_error，所以 N 大於目標型別位元數且高位有設定時就會失敗。傳給 C API
//     之前必須確認 N 與目標型別相容。
//     為什麼會錯：以為轉換是純粹的位元重新詮釋、不可能失敗；實際上它是有值域檢查的轉換。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <bitset>
#include <string>

// ---------------------------------------------------------------------------
// 範例 1:基本操作
// ---------------------------------------------------------------------------
void demo_basic() {
    std::cout << "[demo_basic]\n";
    std::bitset<8> b(0b10101010);
    std::cout << "  b           = " << b << "\n";
    std::cout << "  b.count()   = " << b.count() << "\n";
    std::cout << "  b.test(1)   = " << b.test(1) << "\n";
    std::cout << "  b.test(0)   = " << b.test(0) << "\n";

    b.set(0);
    b.flip(7);
    std::cout << "  after set/flip: " << b << "\n";

    std::bitset<8> mask("00001111");
    std::cout << "  b & mask    = " << (b & mask) << "\n";
    std::cout << "  b | mask    = " << (b | mask) << "\n";
}

// ---------------------------------------------------------------------------
// 範例 1.5:bitset 完整工具盤點
//
// 上一個範例只展示了一小部分,bitset 還有許多常用方法,這裡把它們一次列齊:
//
//   查詢類:
//     b.all()    —— 是否全部為 1
//     b.any()    —— 是否「有任何 1」
//     b.none()   —— 是否「全為 0」
//
//   修改類(無參數版,會一次作用於全部 bits):
//     b.set()    —— 全部設為 1
//     b.reset()  —— 全部設為 0
//     b.flip()   —— 全部反轉
//     b.set(i, val) —— 把第 i 個 bit 設為 val(0 / 1)
//
//   轉換類:
//     b.to_ulong()  —— 轉 unsigned long(超出寬度會擲 std::overflow_error)
//     b.to_ullong() —— 轉 unsigned long long
//     b.to_string() —— 轉字串(高位在左、低位在右)
//
//   位元運算:
//     ~b、b ^ c、b << k、b >> k
//     b &= c、b |= c、b ^= c、b <<= k、b >>= k
//
//   比較:b == c、b != c
//
//   流入流出:std::cin >> b;  std::cout << b;
//
//   雜湊:std::hash<std::bitset<N>>(可放進 unordered_set)
// ---------------------------------------------------------------------------
#include <sstream>          // 為了示範 operator>>
#include <unordered_set>    // 為了示範 std::hash<bitset>

void demo_bitset_helpers() {
    std::cout << "[demo_bitset_helpers]\n";

    std::bitset<8> b(0b10110001);

    // (a) 查詢類
    std::cout << "  any="  << b.any()
              << ", all=" << b.all()
              << ", none=" << b.none() << "\n";

    // (b) 修改類無參數版本
    std::bitset<8> x;       x.set();      std::cout << "  set()   = " << x << "\n"; // 全 1
    std::bitset<8> y(0xFF); y.reset();    std::cout << "  reset() = " << y << "\n"; // 全 0
    std::bitset<8> z(0b1100); z.flip();   std::cout << "  flip()  = " << z << "\n"; // 反轉

    // set(i, val) —— 帶值版本
    z.set(0, true);
    z.set(7, false);
    std::cout << "  set(i,val)= " << z << "\n";

    // (c) 轉換類
    std::cout << "  to_ulong  = " << b.to_ulong()  << "\n";
    std::cout << "  to_ullong = " << b.to_ullong() << "\n";
    std::cout << "  to_string = " << b.to_string() << "\n";

    // (d) 位元運算:^、~、<<、>>、複合運算
    std::bitset<8> m("00001111");
    std::cout << "  b ^ m   = " << (b ^ m)  << "\n";
    std::cout << "  ~b      = " << (~b)     << "\n";
    std::cout << "  b << 2  = " << (b << 2) << "\n";
    std::cout << "  b >> 2  = " << (b >> 2) << "\n";

    std::bitset<8> w = b;
    w &= m; std::cout << "  b &= m  = " << w << "\n";
    w  = b; w |= m; std::cout << "  b |= m  = " << w << "\n";
    w  = b; w ^= m; std::cout << "  b ^= m  = " << w << "\n";
    w  = b; w <<= 2; std::cout << "  b <<= 2 = " << w << "\n";
    w  = b; w >>= 2; std::cout << "  b >>= 2 = " << w << "\n";

    // (e) 比較
    std::bitset<8> b2 = b;
    std::cout << "  b == b2 ? " << (b == b2)
              << ", b != m ? " << (b != m) << "\n";

    // (f) 串流讀入(>>):從文字讀回 bitset
    std::istringstream iss("11110000");
    std::bitset<8> readIn;
    iss >> readIn;
    std::cout << "  read by >>: " << readIn << "\n";

    // (g) std::hash<bitset> —— 可作為 unordered_set 的 key
    std::unordered_set<std::bitset<8>> seen;
    seen.insert(b);
    seen.insert(m);
    seen.insert(b);                                  // 重複,不會增加
    std::cout << "  unordered_set size = " << seen.size() << "\n";
}

// ---------------------------------------------------------------------------
// 範例 2:Leetcode #204 Count Primes —— 埃式篩法 + bitset
//
// 解題思路:
//   1. 建立 bitset<MAXN> isComposite,初始全 0(代表全部還可能是質數)。
//   2. 從 2 開始,若 i 還沒被標成合數,則 i 是質數;將 i*i, i*i+i, ... 全標為 1。
//   3. 最後計數 [2, n) 區間裡「沒被標」的數量。
//
// 為什麼用 bitset:
//   每個位置只需要 1 bit。對 n=10^7 量級的篩法,比 vector<bool> 更直觀。
//
// 注意:bitset 大小要編譯期常數,這裡先示範到 MAXN=200(夠用了)。
// 時間複雜度:O(n log log n)。
// ---------------------------------------------------------------------------
constexpr int MAXN = 200;

int countPrimes(int n) {
    if (n <= 2) return 0;
    std::bitset<MAXN> isComposite;                // 全 0
    int cnt = 0;
    for (int i = 2; i < n; ++i) {
        if (isComposite[i]) continue;
        ++cnt;
        for (long long j = static_cast<long long>(i) * i; j < n; j += i) {
            isComposite.set(j);
        }
    }
    return cnt;
}

void demo_leetcode_count_primes() {
    std::cout << "[demo_leetcode_count_primes]\n";
    std::cout << "  countPrimes(10)  = " << countPrimes(10)  << "\n"; // 4
    std::cout << "  countPrimes(100) = " << countPrimes(100) << "\n"; // 25
}

// ---------------------------------------------------------------------------
// 範例 3:日常工作實用範例 —— 使用者權限位元
//
// 情境:用 bitset<8> 表達使用者權限,bit 0=讀, 1=寫, 2=刪除, 3=管理員...
//       比起寫五六個 bool 成員或散落整數位元魔法,bitset 一目了然。
// ---------------------------------------------------------------------------
namespace Perm {
    enum : int { Read=0, Write=1, Delete=2, Admin=3, Audit=4 };
}

class UserPermission {
public:
    void grant(int p) { bits_.set(p); }
    void revoke(int p) { bits_.reset(p); }
    bool has(int p) const { return bits_.test(p); }
    std::string describe() const { return bits_.to_string(); }
private:
    std::bitset<8> bits_;
};

void demo_practical_perm() {
    std::cout << "[demo_practical_perm]\n";
    UserPermission u;
    u.grant(Perm::Read);
    u.grant(Perm::Write);
    u.grant(Perm::Audit);
    std::cout << "  bits = " << u.describe() << "\n";
    std::cout << "  has Read?  " << std::boolalpha << u.has(Perm::Read)  << "\n";
    std::cout << "  has Admin? " << u.has(Perm::Admin) << "\n";

    u.revoke(Perm::Write);
    std::cout << "  after revoke Write: " << u.describe() << "\n";
}

// ---------------------------------------------------------------------------
// 實用範例 (額外):feature_flags —— bitset 表達功能開關
//
// 工作中常見:同一個請求可能要走十幾種功能旗標 (A/B 測試, 灰度釋出,
// 暫時開關等)。bitset<64> 可表示 64 個獨立旗標, 比一堆 bool 變數清楚。
// ---------------------------------------------------------------------------
class FeatureFlags {
public:
    enum F : int { NewUi=0, FastApi=1, Beta=2, DebugLog=3 };
    void enable(F f)  { bits_.set(f); }
    void disable(F f) { bits_.reset(f); }
    bool on(F f) const { return bits_.test(f); }
    int active_count() const { return (int)bits_.count(); }
private:
    std::bitset<64> bits_;
};

void demo_practical_flags() {
    std::cout << "[demo_practical_flags]\n";
    FeatureFlags ff;
    ff.enable(FeatureFlags::NewUi);
    ff.enable(FeatureFlags::DebugLog);
    std::cout << "  active=" << ff.active_count() << "\n";
    std::cout << "  on(NewUi)=" << ff.on(FeatureFlags::NewUi) << "\n";
    std::cout << "  on(Beta)= " << ff.on(FeatureFlags::Beta)  << "\n";
}

int main() {
    demo_basic();
    demo_bitset_helpers();
    demo_leetcode_count_primes();
    demo_practical_perm();
    demo_practical_flags();
    return 0;
}

/*
================================================================================
編譯與執行:
    g++ -std=c++17 -Wall -Wextra 11_bitset.cpp -o 11_bitset && ./11_bitset
================================================================================
*/

// 編譯: g++ -std=c++20 -Wall -Wextra 11_bitset.cpp -o 11_bitset

// === 預期輸出 (節錄) ===
// [demo_basic]
//   b           = 10101010
//   b.count()   = 4
//   b.test(1)   = 1
//   b.test(0)   = 0
//   after set/flip: 00101011
//   b & mask    = 00001011
//   b | mask    = 00101111
// [demo_bitset_helpers]
//   any=1, all=0, none=0
//   set()   = 11111111
//   reset() = 00000000
//   flip()  = 11110011
//   set(i,val)= 01110011
//   to_ulong  = 177
//   to_ullong = 177
//   to_string = 10110001
//   b ^ m   = 10111110
//   ~b      = 01001110
//   b << 2  = 11000100
// …（後略，完整輸出共 41 行）
