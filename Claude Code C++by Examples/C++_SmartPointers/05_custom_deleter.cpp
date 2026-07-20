// =====================================================================
// 主題 05: Custom Deleter  (自訂釋放器: 管理 new/delete 之外的資源)
// =====================================================================
// 參考來源:
//   https://en.cppreference.com/w/cpp/memory/unique_ptr  (template 第二參數: Deleter)
//   https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr (建構子可帶 deleter)
//   https://en.cppreference.com/w/cpp/memory/default_delete
//   https://cplusplus.com/reference/memory/unique_ptr/
//   https://cplusplus.com/reference/memory/shared_ptr/shared_ptr/
//
// ---------------------------------------------------------------------
// 一、為什麼需要 custom deleter?
// ---------------------------------------------------------------------
// 預設情況下, 智慧指標的釋放方式是:
//   - unique_ptr<T>     -> delete
//   - unique_ptr<T[]>   -> delete[]
//   - shared_ptr<T>     -> delete
// (這些其實是透過 std::default_delete<T> 完成。)
//
// 但實務上有大量「不是 new 出來」的資源:
//   - C 函式庫: FILE* (fopen/fclose), socket fd (open/close),
//     SQLite handle (sqlite3_open/sqlite3_close), libpng, OpenSSL...
//   - OS handle: HANDLE (Windows CloseHandle), HMODULE...
//   - 自訂物件池 / arena: 不能直接 delete, 必須交還給 pool。
//
// 這些資源的釋放方式不同, 但「自動釋放」的需求一樣 - 此時就用
// custom deleter, 把釋放邏輯交給智慧指標, 享受 RAII 的好處。
//
// ---------------------------------------------------------------------
// 二、unique_ptr 與 shared_ptr 的差異
// ---------------------------------------------------------------------
// (A) unique_ptr<T, Deleter>
//     - Deleter 是「型別」的一部分 (template parameter)。
//     - 因此 unique_ptr<FILE, decltype(&fclose)> 與
//       unique_ptr<FILE, MyDeleter> 是「不同型別」, 不能互相轉換。
//     - 沒有額外空間負擔 (若 deleter 是 stateless 的 functor, EBO)。
//
// (B) shared_ptr<T>
//     - Deleter 「不」屬於型別, 它存在 control block 裡 (type-erased)。
//     - 所以兩個 shared_ptr<FILE> 即使 deleter 不同, 仍是同型別,
//       可以互相轉換、放在同一個容器。
//     - 有少量額外空間 (control block 多一個 deleter 物件)。
//
// 結論:
//   - 想要零成本、明確獨占 -> unique_ptr + custom deleter
//   - 想要靈活、共享、deleter 可動態 -> shared_ptr + 建構子帶 deleter
//
// ---------------------------------------------------------------------
// 三、unique_ptr custom deleter 的幾種寫法
// ---------------------------------------------------------------------
// 1. Lambda (最方便, 但要注意「型別」是 lambda 的型別):
//      auto fp = std::unique_ptr<FILE, decltype(&fclose)>(fopen(...), &fclose);
//
// 2. 函式指標:
//      using FileGuard = std::unique_ptr<FILE, int(*)(FILE*)>;
//      FileGuard fp(fopen(...), &fclose);
//
// 3. Stateless functor (推薦, 零空間成本):
//      struct FileCloser { void operator()(FILE* f) const { if (f) fclose(f); } };
//      using FileGuard = std::unique_ptr<FILE, FileCloser>;
//      FileGuard fp(fopen(...));   // deleter 不需傳, 因為 default 建構即可
//
// ---------------------------------------------------------------------
// 四、shared_ptr custom deleter 的寫法
// ---------------------------------------------------------------------
//   std::shared_ptr<FILE> fp(fopen(...), [](FILE* f){ if (f) fclose(f); });
// 注意: shared_ptr 不能用 std::make_shared 搭配 custom deleter
//       (因為 make_shared 會幫你選擇 deleter, 你無從介入)。
//       必須用建構子直接傳。
//
// ---------------------------------------------------------------------
// 五、常見陷阱
// ---------------------------------------------------------------------
// 1. 函式指標 deleter 會佔一個 pointer 空間 (sizeof(unique_ptr) 變大)。
//    若需要極致效能, 用 stateless functor。
// 2. C 函式可能不接受 nullptr (例如 fclose(nullptr) 是 UB on 某些舊系統),
//    所以在 deleter 裡加上 if (ptr) 檢查比較保險。
// 3. shared_ptr 的 deleter 是 type-erased, 但複製 shared_ptr 不會複製
//    deleter, 它只活在 control block, 直到最後一個 ref 釋放才執行。
// 4. 想換 deleter? unique_ptr 不行 (型別不同); shared_ptr 必須 reset。
//
// =====================================================================

/*
補充筆記：std::custom_deleter
  - std::custom_deleter 屬於智慧指標；先判斷所有權是獨占、共享，還是只是觀察。
  - unique_ptr 表達獨占所有權，不可複製但可 move；shared_ptr 表達共享所有權，weak_ptr 打破循環引用。
  - make_unique/make_shared 通常比裸 new 更安全；它們把配置和建構包在一起，降低例外中途洩漏風險。
  - get() 只取得借用指標，不轉移所有權；不要 delete get() 回傳值。
  - shared_ptr 的 control block 很重要；不要用同一裸指標建立多個 shared_ptr。
  - custom deleter 用於 FILE*、C API handle、特殊釋放函式；deleter 型別也會影響 unique_ptr 型別大小。
  - custom deleter 讓智慧指標能管理不是用 delete 釋放的資源，例如 FILE* 要 fclose。
  - unique_ptr 的 deleter 型別是指標型別的一部分，因此不同 deleter 會形成不同 unique_ptr 型別。
  - shared_ptr 把 deleter 放在 control block 中，型別不會出現在 shared_ptr<T> 的 T 裡。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】Custom Deleter（自訂釋放器）
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼自訂 deleter 會改變 sizeof(unique_ptr)，卻不影響 sizeof(shared_ptr)？
//     答：unique_ptr<T, D> 的 deleter 型別 D 是「型別的一部分」，所以會佔物件空間 ——
//         無狀態的 functor 或無捕獲 lambda 因 empty base optimization 不佔空間，但
//         函式指標、有狀態的 functor、std::function 都會讓 unique_ptr 變大。
//         shared_ptr<T> 的 deleter 則是 type erasure 後存在 control block 裡、不屬於
//         型別，所以大小不變（通常是兩個指標大小，這是常見實作、非標準保證）。
//     追問：實務差異？（shared_ptr<FILE> 不論 deleter 為何都是同一型別、可放進同一個
//         容器；unique_ptr 必須把 deleter 型別寫出來 —— 本檔的 FilePtr 與 FilePtr2
//         就是兩個不同型別）
//
// 🔥 Q2. 自訂 deleter 有哪些實際用途？怎麼寫？
//     答：管理非 new 配置的資源：FILE*（fclose）、malloc 記憶體（free）、OS handle
//         （close/CloseHandle）、第三方 C 函式庫的 xxx_destroy()、物件池的「歸還」。
//         unique_ptr 有三種寫法：函式指標、無捕獲 lambda、stateless functor（最後者
//         無空間開銷，最推薦）。shared_ptr 則直接 shared_ptr<FILE>(fp, fclose)。
//
// 🔥 Q3. 為什麼 make_shared 不支援自訂 deleter？
//     答：make_shared 的價值就在「物件與 control block 配置在同一塊記憶體」，釋放路徑
//         因此是固定的；一旦讓使用者插入自訂 deleter，這個優化就不成立。需要自訂
//         deleter 只能用 shared_ptr<T>(raw, deleter) 建構子。make_unique 同理也不支援。
//
// ⚠️ 陷阱. shared_ptr<Base> 管理 Derived、而 Base 沒有 virtual destructor，會漏解構嗎？
//     答：不會 —— 這是 shared_ptr 少見的優勢。deleter 是在「建構時」依當時的靜態型別
//         決定並存進 control block 的，所以 shared_ptr<Base> sp = make_shared<Derived>();
//         會記得「用 Derived 的方式刪除」。unique_ptr<Base> 則不行（它用
//         default_delete<Base>，走 delete (Base*) → UB）。
//     為什麼會錯：這題常被反過來理解成「那就不需要 virtual destructor 了」—— 錯。只要
//         還有人可能用裸指標或 unique_ptr 持有該物件，仍然是 UB，不要拿這個技巧去
//         設計 API。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <memory>
#include <cstdio>
#include <vector>
#include <string>

// =====================================================================
// 範例 (1): unique_ptr 管理 FILE* (最經典的 custom deleter 用例)
// =====================================================================
// 觀念: fopen 取得 FILE*, 必須 fclose; 用 unique_ptr 自動 fclose,
//       即使中途 throw 例外也不會洩漏。
struct FileCloser {
    void operator()(std::FILE* f) const noexcept {
        if (f) {
            std::cout << "  [FileCloser: fclose]\n";
            std::fclose(f);
        }
    }
};
// alias 讓宣告變短
using FilePtr = std::unique_ptr<std::FILE, FileCloser>;

void example_unique_ptr_FILE() {
    std::cout << "--- 範例 1: unique_ptr 管理 FILE* ---\n";
    // 寫一個小檔案, 再讀回來
    {
        FilePtr fp(std::fopen("/tmp/sp_demo.txt", "w"));
        if (!fp) { std::cout << "  open 失敗\n"; return; }
        std::fputs("hello smart pointer\n", fp.get());
    } // 離開 scope -> FileCloser 被呼叫 -> fclose

    {
        FilePtr fp(std::fopen("/tmp/sp_demo.txt", "r"));
        if (!fp) { std::cout << "  open 失敗\n"; return; }
        char buf[64] = {0};
        if (std::fgets(buf, sizeof(buf), fp.get())) {
            std::cout << "  讀回: " << buf;
        }
    } // 同樣自動 fclose
}

// =====================================================================
// 範例 (2): unique_ptr + 函式指標 deleter
// =====================================================================
// 觀念: 不想另寫 functor 時, 直接用函式指標 (例如 C API 的釋放函式)。
//       缺點: deleter 佔 pointer 空間。
void example_unique_ptr_function_pointer() {
    std::cout << "--- 範例 2: 函式指標 deleter ---\n";
    using FilePtr2 = std::unique_ptr<std::FILE, int(*)(std::FILE*)>;
    FilePtr2 fp(std::fopen("/tmp/sp_demo.txt", "r"), &std::fclose);
    if (!fp) { std::cout << "  open 失敗\n"; return; }
    std::cout << "  函式指標 deleter, sizeof(FilePtr2)=" << sizeof(FilePtr2) << "\n";
    std::cout << "  sizeof(FilePtr 用 functor)=" << sizeof(FilePtr) << "  (functor 較小)\n";
}

// =====================================================================
// 範例 (3): unique_ptr<T[]> - 陣列特化
// =====================================================================
// 觀念: 動態配置陣列要用 delete[], unique_ptr<T[]> 自動處理。
//       這個其實是 std::default_delete<T[]> 的特化, 不算 custom,
//       但很多人不知道, 順便提一下。
void example_unique_ptr_array() {
    std::cout << "--- 範例 3: unique_ptr<T[]> 陣列 ---\n";
    auto arr = std::make_unique<int[]>(5);              // 5 個 0
    for (int i = 0; i < 5; ++i) arr[i] = i * i;
    for (int i = 0; i < 5; ++i) std::cout << "  arr[" << i << "]=" << arr[i] << "\n";
    // 離開 scope 自動 delete[]
}

// =====================================================================
// 範例 (4): shared_ptr 帶 lambda deleter
// =====================================================================
// 觀念: shared_ptr 可以共享 + 自訂釋放; deleter 存在 control block,
//       不影響型別, 因此 shared_ptr<FILE> 各種來源可放同一容器。
void example_shared_ptr_FILE() {
    std::cout << "--- 範例 4: shared_ptr 管理 FILE* (lambda deleter) ---\n";
    std::shared_ptr<std::FILE> a(
        std::fopen("/tmp/sp_demo.txt", "r"),
        [](std::FILE* f) {
            if (f) {
                std::cout << "  [lambda deleter: fclose]\n";
                std::fclose(f);
            }
        });
    if (!a) { std::cout << "  open 失敗\n"; return; }

    auto b = a;                                          // 共享; 引用計數 +1
    std::cout << "  use_count = " << a.use_count() << "\n"; // 2
    // a, b 都離開後, deleter 才被呼叫
}

// =====================================================================
// 範例 (5): 物件池 (object pool) 的歸還語意
// =====================================================================
// 觀念: 物件不是 new 出來, 而是從 pool 借的; 釋放就是「歸還」。
//       用 custom deleter 把歸還邏輯藏進智慧指標, 使用者不必手動還。
class ConnectionPool {
public:
    struct Connection {
        int id;
        explicit Connection(int i) : id(i) {}
    };

private:
    std::vector<std::unique_ptr<Connection>> idle_;     // 池內閒置連線
    int next_id_ = 0;

public:
    // 借出一個連線; 用 shared_ptr + 自訂 deleter, 釋放時自動歸還
    std::shared_ptr<Connection> acquire() {
        std::unique_ptr<Connection> c;
        if (!idle_.empty()) {
            c = std::move(idle_.back());
            idle_.pop_back();
        } else {
            c = std::make_unique<Connection>(next_id_++);
        }
        Connection* raw = c.release();                  // 暫時放棄擁有權
        // deleter 把 raw 包回 unique_ptr 還給 pool, 而不是 delete
        return std::shared_ptr<Connection>(raw, [this](Connection* p) {
            std::cout << "  [歸還 Connection #" << p->id << " 回 pool]\n";
            idle_.emplace_back(p);                       // 回收
        });
    }

    size_t idle_size() const { return idle_.size(); }
};

void example_pool() {
    std::cout << "--- 範例 5: 物件池 (custom deleter 做歸還) ---\n";
    ConnectionPool pool;
    {
        auto c1 = pool.acquire();                        // 池空 -> 建立 #0
        auto c2 = pool.acquire();                        // 池空 -> 建立 #1
        std::cout << "  借出 c1=#" << c1->id << ", c2=#" << c2->id << "\n";
        std::cout << "  pool idle = " << pool.idle_size() << "\n";   // 0
    } // c1, c2 離開 scope, 自動歸還
    std::cout << "  歸還後 pool idle = " << pool.idle_size() << "\n"; // 2

    auto c3 = pool.acquire();                            // 從池內取出, 不新建
    std::cout << "  再借, 拿到 #" << c3->id
              << ", pool idle = " << pool.idle_size() << "\n";       // 1
}

// =====================================================================
// 範例 (6): shared_ptr aliasing constructor (實用進階)
// =====================================================================
// 觀念: 一種特殊的 shared_ptr 建構子, 讓「兩個 shared_ptr 共用同一個
//       control block, 但指向不同物件 (通常是子物件)」。
//       常見場景: 想把容器中的某個成員交出去, 又不想拷貝整個容器,
//       只要這個成員還在用, 整個容器就會活著。
struct BigData {
    std::vector<int> values{10, 20, 30};
    ~BigData() { std::cout << "  [BigData 釋放]\n"; }
};

std::shared_ptr<int> share_one_value(std::shared_ptr<BigData> big, size_t idx) {
    // aliasing constructor: 共用 big 的 control block, 但指向 big->values[idx]
    return std::shared_ptr<int>(big, &big->values[idx]);
}

// =====================================================================
// 範例 7 (實用): scope_exit — 用 unique_ptr 模擬「離開 scope 自動執行」
// =====================================================================
// 觀念: C++17 沒有官方 std::scope_exit (C++23 才有 std::experimental::scope_exit
//       或 boost::scope_exit)。實務上常見的 workaround: 用 unique_ptr 配
//       「一個 dummy 指標 + lambda deleter」, 離開 scope 時自動呼叫 lambda。
//
// 用途: 註冊資源後, 不論是否成功 / 是否 throw, 都要在 scope 結束時做清理。
//       對 C 函式庫整合非常實用 (mutex unlock, fd close 但不想包整個 class 等)。
// =====================================================================
struct ScopeExitDummy {};                       // 不需要真實內容
template <class F>
auto make_scope_exit(F&& f) {
    static ScopeExitDummy dummy;
    return std::unique_ptr<ScopeExitDummy, std::decay_t<F>>(&dummy, std::forward<F>(f));
}

void example_scope_exit() {
    std::cout << "--- 範例 7 (實用): scope_exit 用 unique_ptr 模擬 ---\n";
    std::cout << "  enter scope\n";
    auto guard = make_scope_exit([](ScopeExitDummy*) {
        std::cout << "  [scope_exit] 自動清理被執行!\n";
    });
    std::cout << "  doing work...\n";
    // 即使這裡 throw 也會觸發 lambda - 這就是 RAII 的威力
}

void example_aliasing() {
    std::cout << "--- 範例 6: shared_ptr aliasing constructor ---\n";
    std::shared_ptr<int> ref;
    {
        auto big = std::make_shared<BigData>();
        ref = share_one_value(big, 1);                   // 取出 values[1]=20
        std::cout << "  *ref = " << *ref << "\n";
        std::cout << "  big.use_count = " << big.use_count() << "\n"; // 2
    } // big 離開 scope, 但 ref 還活著, BigData 不會被釋放
    std::cout << "  big 離開 scope 後, *ref = " << *ref << " (BigData 仍在)\n";
    // 函式結束, ref 釋放, BigData 才真正釋放
}

// =====================================================================
// (本主題沒有特別合適的 Leetcode 對應題)
// 原因: Leetcode 題目基本上都用 new/delete 或內建容器, 不涉及 C 資源
//       管理。Custom deleter 的價值主要在「真實世界 C/系統 API」整合。
//
// 上面 6 個範例覆蓋的都是日常工程實務的情境:
//   1. C 檔案 handle (fopen/fclose) - 最經典
//   2. 函式指標 deleter 與空間成本權衡
//   3. unique_ptr 陣列特化
//   4. shared_ptr + lambda deleter
//   5. 物件池 (acquire/release pattern)
//   6. shared_ptr aliasing constructor (子物件共享生命週期)
// =====================================================================

// =====================================================================
// main
// =====================================================================
int main() {
    example_unique_ptr_FILE();
    example_unique_ptr_function_pointer();
    example_unique_ptr_array();
    example_shared_ptr_FILE();
    example_pool();
    example_aliasing();
    example_scope_exit();

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：為什麼 std::make_shared 不支援 custom deleter, 但 shared_ptr
    //       的 ctor 卻可以?
    //    A：make_shared 的優勢是「物件 + control block 配置在同一塊」,
    //       它預設用 placement new + ::operator delete 釋放。如果讓你
    //       自訂 deleter, 物件就不能跟 control block 同塊配置, 這個優化
    //       就失效, 失去 make_shared 的意義。需要 custom deleter 就用
    //       shared_ptr<T>(raw, deleter) 直接建。
    //
    //  Q2：unique_ptr 的 deleter 是型別參數, 為什麼 shared_ptr 不是?
    //    A：unique_ptr<T, D> 把 deleter 編進型別 → 零成本 (stateless
    //       functor 走 EBO)。shared_ptr 的 deleter 存在 control block,
    //       靠 type erasure 隱藏, 所以兩個 shared_ptr<FILE> 即使釋放方式
    //       不同, 在型別系統上仍可互換、可放同 vector, 代價是多一個
    //       virtual call 與少量空間。
    //
    //  Q3：deleter 傳進來的指標可能是 nullptr 嗎?
    //    A：當 unique_ptr/shared_ptr 在 reset() 或解構時, 若它持有 null,
    //       「不會」呼叫 deleter (標準保證)。所以你的 deleter 在標準
    //       路徑下不必檢查 null。但如果是「自己拿到 raw 後手動丟給
    //       deleter」 (例如 pool 範例), 仍應加 if (p) 防呆, 因為 fclose
    //       (nullptr) 在某些舊系統是 UB。
    //
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra 05_custom_deleter.cpp -o 05_custom_deleter

// === 預期輸出 (節錄) ===
// --- 範例 1: unique_ptr 管理 FILE* ---
//   [FileCloser: fclose]
//   讀回: hello smart pointer
//   [FileCloser: fclose]
// --- 範例 2: 函式指標 deleter ---
//   函式指標 deleter, sizeof(FilePtr2)=16
//   sizeof(FilePtr 用 functor)=8  (functor 較小)
// --- 範例 3: unique_ptr<T[]> 陣列 ---
//   arr[0]=0
//   arr[1]=1
//   arr[2]=4
//   arr[3]=9
//   arr[4]=16
// --- 範例 4: shared_ptr 管理 FILE* (lambda deleter) ---
//   use_count = 2
//   [lambda deleter: fclose]
// --- 範例 5: 物件池 (custom deleter 做歸還) ---
//   借出 c1=#0, c2=#1
//   pool idle = 0
//   [歸還 Connection #1 回 pool]
// …（後略，完整輸出共 33 行）
