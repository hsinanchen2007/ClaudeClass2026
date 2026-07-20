// =============================================================================
//  課程 2.2：執行緒函式的多種形式3.cpp  —  成員函式 + 物件指標作為執行緒入口
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：#include <thread>（用 std::ref 需 #include <functional>）
//   標準版本：C++11
//   語法：std::thread t(&類別名::成員函式名, 物件指標, 參數...);
//         第 1 參數：成員函式指標（&C::f，& 與類別限定都不可省）
//         第 2 參數：物件——可以是「指標 &obj」「std::ref(obj)」「物件本身 obj」，
//                    三者語意完全不同（見【3.】，本檔以位址實測證明）
//         第 3 參數起：真正要傳給成員函式的參數
//   等價寫法：std::thread t([&obj]{ obj.f(args...); });   // lambda 版，常更好讀
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼要寫 &Worker::doWork，不能只寫 doWork】
//   成員函式不是自由函式：它需要一個「呼叫它的物件」。C++ 規定取成員函式位址
//   一定要寫成 &類別::成員名，兩個部分都不能省：
//     * 少了 &：成員函式名不會像自由函式那樣自動 decay 成指標（這是語言刻意
//       的不對稱設計，避免與「呼叫它」的語法混淆）。
//     * 少了類別限定：成員函式名在類別作用域外根本不可見。
//   所以正確寫法只有 &Worker::doWork 一種。
//
// 【2. INVOKE 語意——標準怎麼決定「第二個參數是物件」】
//   std::thread 呼叫可呼叫物件時用的是 INVOKE(f, args...) 規則。
//   當 f 是「指向類別 C 的成員函式指標」時，INVOKE 會看第一個參數 a1：
//     * a1 是 C（或其衍生類別）的物件或參考   → 呼叫 (a1.*f)(其餘參數)
//     * a1 是 reference_wrapper（std::ref）    → 呼叫 (a1.get().*f)(其餘參數)
//     * 其他（例如指標）                        → 呼叫 ((*a1).*f)(其餘參數)
//   這就是為什麼 std::thread(&Worker::doWork, &worker) 能運作：
//   &worker 是指標，走第三條規則，等價於 (*(&worker)).doWork()。
//   同一套 INVOKE 規則也用在 std::bind、std::function、std::invoke 上，
//   學會一次到處適用。
//
// 【3. 最重要的陷阱：第二參數傳「物件本身」會複製一份】
//   別忘了檔案 1【3.】講的 decay-copy——std::thread 對「每一個參數」都做
//   decay-copy，第二參數（物件）也不例外：
//     std::thread t(&C::f, &obj);              // 複製「指標」→ 操作原物件 ✅
//     std::thread t(&C::f, std::ref(obj));     // 複製 reference_wrapper → 原物件 ✅
//     std::thread t(&C::f, obj);               // 複製「整個物件」→ 操作複本 ⚠️
//   第三種可以編譯、可以執行、不會有任何警告，但成員函式跑在一個「臨時複本」
//   上，所有對成員的修改在執行緒結束後隨複本一起消失——這是最難查的那種
//   邏輯 bug（沒有 crash、沒有警告，只是結果默默不對）。
//   本檔 main 直接印出 this 的位址來證明：前兩者與原物件位址相同，
//   第三者不同。
//
// 【4. 生命週期：物件必須活得比執行緒久】
//   傳 &obj 只是傳位址，std::thread 不會延長 obj 的壽命。若 obj 是區域變數，
//   而執行緒被 detach 之後 obj 所在的作用域結束 → 執行緒透過懸空指標存取
//   已銷毀的物件，屬未定義行為（不保證任何特定結果）。
//   安全模式：
//     * 物件生命週期明顯涵蓋執行緒 → 用 &obj + join()（本檔作法）。
//     * 生命週期不確定 → 用 std::shared_ptr 捕獲，讓執行緒共同持有所有權。
//     * 最糟的組合是「區域物件 + detach」，務必避免。
//
// 【5. 為什麼實務上常改用 lambda】
//   std::thread t(&Worker::doWorkWithParam, &worker, 123);
//   std::thread t([&worker]{ worker.doWorkWithParam(123); });
//   兩者等價，但 lambda 版：（a）不必記 INVOKE 規則；（b）多載函式時不會有
//   「取哪個多載的位址」的歧義；（c）呼叫長相就是平常的 obj.f(x)，好讀。
//   成員函式指標版的優勢是不需要額外的 closure 型別，且能直接和 std::bind、
//   std::function 等既有設施組合。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 成員函式指標不是普通函式指標——它通常比較大
//   本機實測（x86-64，Itanium C++ ABI，GCC 15.2）：
//     sizeof(void(*)())      = 8    普通函式指標
//     sizeof(&Plain::f)      = 16   成員函式指標（非虛擬類別也是 16）
//     sizeof(&WithVirt::g)   = 16   有虛擬函式的類別
//   為什麼要 16？因為成員函式指標必須同時編碼兩件事：
//     * 一個欄位存「函式位址」，或在虛擬情境下存「vtable 內的偏移量」；
//     * 另一個欄位存 this 的調整量（adjustment），用來處理多重繼承／
//       虛擬繼承時 base 與 derived 之間 this 需要平移的情況。
//   結論：成員函式指標是一個「小結構」，不是單純位址，不能和 void* 互轉。
//
// (B) 虛擬函式照樣正確分派
//   std::thread t(&Base::virtualMethod, &derivedObj) 會在執行期透過 vtable
//   分派到 Derived 的覆寫版本——成員函式指標保存的是「要呼叫哪個虛擬槽」，
//   實際函式在呼叫當下才由物件的 vptr 決定。多型不會因為跨執行緒而失效。
//
// (C) 和 std::bind 的關係
//   std::bind(&C::f, &obj, 123) 產生的可呼叫物件用的是同一套 INVOKE 規則，
//   所以 std::thread(std::bind(&C::f, &obj, 123)) 也能跑，只是多包一層。
//   由於 std::thread 建構子本身就支援「可呼叫物件 + 參數」，
//   這裡用 bind 純屬多餘——直接傳 (&C::f, &obj, 123) 即可（見檔案 6）。
//
// 【注意事項 Pay Attention】
//   1. 第二參數傳物件本身（非指標／非 std::ref）→ 靜默複製，成員修改遺失。
//      能編譯、能執行、無警告，是最難察覺的一種錯。
//   2. 傳 &obj 不會延長物件壽命；「區域物件 + detach」是懸空指標的典型來源，
//      屬未定義行為，不保證是 crash 還是看似正常。
//   3. 成員函式若有多載，&C::f 會有歧義，需要用 static_cast 指定簽名：
//      static_cast<void(C::*)(int)>(&C::f)
//   4. 呼叫 private 成員函式一樣受存取控制限制——取位址的那行程式碼必須
//      有存取權（例如在類別自己的成員函式內開執行緒）。
//   5. 多條執行緒同時呼叫「同一物件」的成員函式時，若該函式會修改成員，
//      就必須自行同步。std::thread 只負責把呼叫送過去，完全不提供互斥。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】成員函式作為執行緒入口
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::thread t(&Worker::doWork, &worker); 為什麼第二個參數要傳 &worker？
//     答：成員函式需要一個呼叫它的物件。標準用 INVOKE 規則決定：第一個參數
//         若是指標就等價於 ((*p).*f)()，若是物件／參考就是 (o.*f)()，
//         若是 reference_wrapper 就是 (o.get().*f)()。傳 &worker 走指標那條，
//         等價於 worker.doWork()。
//     追問：可以傳 worker（不加 &）嗎？→ 語法上可以且能編譯，但那會 decay-copy
//         整個物件，成員函式跑在複本上，修改全部遺失。要傳物件又不複製
//         就得用 std::ref(worker)。
//
// 🔥 Q2. 成員函式指標和普通函式指標有什麼不同？
//     答：普通函式指標就是一個位址（本機 8 byte）；成員函式指標是一個小結構
//         （本機 16 byte），要同時編碼「函式位址或 vtable 偏移」以及
//         「this 的調整量」，後者用於多重／虛擬繼承時平移 this。
//         因此它不能和 void* 互轉，也不能直接餵給 C API。
//     追問：虛擬函式跨執行緒還能正確分派嗎？→ 可以，vtable 分派發生在呼叫當下，
//         由物件的 vptr 決定，和在哪條執行緒呼叫無關。
//
// ⚠️ 陷阱. 「std::thread t(&Counter::increment, counter); 沒有編譯錯誤也沒有警告，
//           所以它跟傳 &counter 是一樣的。」
//     答：錯。這行把整個 counter 物件 decay-copy 了一份，成員函式跑在那個
//         臨時複本上。執行緒結束後複本銷毀，所有累加結果一起消失，
//         原物件的計數仍是 0。程式不會 crash，只是答案默默錯掉。
//     為什麼會錯：把「傳物件」想成 Java／Python 那樣「傳的是參照」。
//         C++ 的預設是值語意，而且 std::thread 對每個參數都做 decay-copy，
//         就算你寫的成員函式簽名是 non-const，也擋不住這次複製。
//         要引用語意必須明確寫出來：&obj 或 std::ref(obj)。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <thread>
#include <functional>   // std::ref
#include <vector>
#include <mutex>
#include <string>

class Worker {
public:
    void doWork() {
        std::cout << "Worker doing work" << std::endl;
    }

    void doWorkWithParam(int id) {
        std::cout << "Worker " << id << " working" << std::endl;
    }

    // 印出 this，用來證明「操作的到底是不是原物件」
    void reportIdentity(const char* how) const {
        std::cout << "  " << how << " -> this = " << static_cast<const void*>(this) << std::endl;
    }
};

// -----------------------------------------------------------------------------
// 用來實測「傳物件 vs 傳指標」差異的計數器
// -----------------------------------------------------------------------------
class Counter {
    int count_ = 0;
public:
    void incrementTo(int n) {
        for (int i = 0; i < n; ++i) ++count_;
    }
    int value() const { return count_; }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】背景監控取樣器（metrics sampler）
//   情境：服務要每隔一段時間取樣一次 QPS／延遲，寫進內部環狀緩衝，
//         供 /metrics 端點讀取。取樣邏輯天生屬於某個 Sampler 物件
//         （它要維護 buffer、序號、狀態），所以用成員函式最自然。
//   為什麼用成員函式形式：狀態就在物件裡，不需要額外捕獲；
//         而且 sampler 是成員變數／長生命週期物件，不會有懸空問題。
//   重點：samples_ 由取樣執行緒寫、主執行緒在 join() 之後才讀 →
//         join() 建立了 happens-before 關係，因此不需要額外的鎖。
// -----------------------------------------------------------------------------
class MetricsSampler {
    std::vector<std::string> samples_;
    std::string name_;
public:
    explicit MetricsSampler(std::string name) : name_(std::move(name)) {}

    void sample(int rounds) {
        for (int i = 0; i < rounds; ++i) {
            // 實務上這裡讀真實計數器；此處用固定式子讓輸出可重現
            samples_.push_back(name_ + "#" + std::to_string(i) +
                               " qps=" + std::to_string(100 + i * 10));
        }
    }
    const std::vector<std::string>& samples() const { return samples_; }
};

int main() {
    Worker worker;

    std::cout << "=== 成員函式（無參數 / 有參數）===" << std::endl;
    std::thread t1(&Worker::doWork, &worker);
    t1.join();
    std::thread t2(&Worker::doWorkWithParam, &worker, 123);
    t2.join();

    std::cout << "\n=== 三種第二參數的差別（看 this 位址）===" << std::endl;
    std::cout << "  原物件 worker 位址        = " << static_cast<const void*>(&worker) << std::endl;
    std::thread p1(&Worker::reportIdentity, &worker, "傳指標 &worker      ");
    p1.join();
    std::thread p2(&Worker::reportIdentity, std::ref(worker), "傳 std::ref(worker) ");
    p2.join();
    std::thread p3(&Worker::reportIdentity, worker, "傳物件 worker       ");
    p3.join();
    std::cout << "  ↑ 前兩者 this 等於原物件位址；第三者不同 = 操作的是複本" << std::endl;

    std::cout << "\n=== 複本陷阱的實際後果 ===" << std::endl;
    Counter byValue;
    std::thread c1(&Counter::incrementTo, byValue, 1000);   // ⚠️ 複製整個物件
    c1.join();
    std::cout << "  傳物件      -> 原 Counter 的值 = " << byValue.value()
              << "（累加跑在複本上，結果遺失）" << std::endl;

    Counter byPointer;
    std::thread c2(&Counter::incrementTo, &byPointer, 1000);  // ✅ 傳指標
    c2.join();
    std::cout << "  傳指標 &obj -> 原 Counter 的值 = " << byPointer.value() << std::endl;

    Counter byRef;
    std::thread c3(&Counter::incrementTo, std::ref(byRef), 1000);  // ✅ 傳 std::ref
    c3.join();
    std::cout << "  傳 std::ref -> 原 Counter 的值 = " << byRef.value() << std::endl;

    std::cout << "\n=== 成員函式指標的大小 ===" << std::endl;
    std::cout << "  sizeof(void(*)())            = " << sizeof(void (*)())
              << "  <- 普通函式指標，就是一個位址" << std::endl;
    std::cout << "  sizeof(&Worker::doWork)      = " << sizeof(&Worker::doWork)
              << " <- 成員函式指標：位址/vtable 偏移 + this 調整量（實作定義）" << std::endl;

    std::cout << "\n=== 日常實務：背景監控取樣器 ===" << std::endl;
    MetricsSampler sampler("api.latency");
    std::thread st(&MetricsSampler::sample, &sampler, 3);   // 物件活得比執行緒久
    st.join();   // join() 建立 happens-before，之後主執行緒讀取無需加鎖
    std::cout << "  取樣完成，共 " << sampler.samples().size() << " 筆：" << std::endl;
    for (const auto& s : sampler.samples()) {
        std::cout << "    " << s << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread "課程 2.2：執行緒函式的多種形式3.cpp" -o callable3

// （所有 this／物件位址「每次執行都不同」——ASLR 會讓堆疊位址每次變動；

// === 預期輸出 ===
//   重點在「前兩者相等、第三者不等」這個關係，不是具體數值。
//   sizeof 為實作定義值；本機 x86-64 / GCC 15.2 / Itanium C++ ABI）
//
// === 成員函式（無參數 / 有參數）===
// Worker doing work
// Worker 123 working
//
// === 三種第二參數的差別（看 this 位址）===
//   原物件 worker 位址        = 0x7ffd3f8a4e3f
//   傳指標 &worker       -> this = 0x7ffd3f8a4e3f
//   傳 std::ref(worker)  -> this = 0x7ffd3f8a4e3f
//   傳物件 worker        -> this = 0x5d9c4a2f52b8
//   ↑ 前兩者 this 等於原物件位址；第三者不同 = 操作的是複本
//
// === 複本陷阱的實際後果 ===
//   傳物件      -> 原 Counter 的值 = 0（累加跑在複本上，結果遺失）
//   傳指標 &obj -> 原 Counter 的值 = 1000
//   傳 std::ref -> 原 Counter 的值 = 1000
//
// === 成員函式指標的大小 ===
//   sizeof(void(*)())            = 8  <- 普通函式指標，就是一個位址
//   sizeof(&Worker::doWork)      = 16 <- 成員函式指標：位址/vtable 偏移 + this 調整量（實作定義）
//
// === 日常實務：背景監控取樣器 ===
//   取樣完成，共 3 筆：
//     api.latency#0 qps=100
//     api.latency#1 qps=110
//     api.latency#2 qps=120
