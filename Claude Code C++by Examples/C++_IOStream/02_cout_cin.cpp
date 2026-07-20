// =============================================================================
//  02_cout_cin.cpp  —  std::cout / std::cin 基本用法
// =============================================================================
//  參考：
//    https://en.cppreference.com/w/cpp/io/cin
//    https://en.cppreference.com/w/cpp/io/cout
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、cout / cin 的基本動作                                  │
//  └────────────────────────────────────────────────────────────┘
//
//  cout：把資料「格式化後」寫到 stdout
//  cin：從 stdin「跳過空白、讀一個 token、解析成目標型別」
//
//  例：
//      int x;
//      cin >> x;              // 讀 "  42  abc" 會吃掉前導空白後讀到 42
//                             // 之後 stream 內仍剩 "  abc"
//
//  關鍵特性：
//   * cin >> T 只讀「一個 token」，不吃整行（要整行用 getline）
//   * 預設「跳過空白字元」(operator>> 對 char/string 也是)；想要連空白都讀
//     要 cin.unsetf(std::ios::skipws) 或用 cin.get() / getline()
//   * 解析失敗會把 cin 設為 fail 狀態（後續 cin >> 都會立刻 return 不讀）
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、operator>> 的回傳是 stream 自己 → 可串、可當條件      │
//  └────────────────────────────────────────────────────────────┘
//
//      while (cin >> x) { ... }
//
//  cin 在條件位置會被「contextual converted to bool」 — true 表示「上一次
//  讀取成功」，false 表示讀失敗或 EOF。這是 STL 風格輸入循環的標準寫法。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、本檔示範（不互動，用 stringstream 模擬輸入）          │
//  └────────────────────────────────────────────────────────────┘
//
//  為了讓檔案能直接 ./02_cout_cin 跑而不卡住，本檔不真的呼叫 cin；改用
//  istringstream 模擬「stdin」的內容。所有 cin >> 的姿勢同樣適用。
//
//   * Demo 1：基本 << 鏈式
//   * Demo 2：用 istringstream 模擬連續輸入
//   * Demo 3：while (in >> x) 標準讀法
//   * Demo 4：cin >> 解析失敗的處理
// =============================================================================

/*
補充筆記：std::cout_cin
  - std::cout_cin 屬於 iostream；stream 同時保存資料流位置、格式設定和錯誤狀態。
  - operator>> 會跳過空白並遇空白停止，getline 會讀到換行；混用時常需要先處理殘留 newline。
  - failbit、badbit、eofbit 代表不同狀態；讀取失敗後要 clear() 才能繼續使用 stream。
  - fstream 開檔模式要明確：in/out/app/binary/trunc 組合不同會影響是否覆蓋、追加或以二進位處理。
  - sync_with_stdio(false) 和 cin.tie(nullptr) 可提升競賽輸入速度，但混用 C stdio 和 iostream 時要小心順序。
  - stringstream 適合字串解析與格式化，但大量資料轉換時可考慮 charconv 降低 locale 和配置成本。
  - std::cin >> x 會跳過前導空白，讀到下一個不符合型別的字元時停止。
  - std::cout 通常有緩衝；需要立即刷新可用 std::flush，但過度刷新會降低效能。
  - 互動式輸入常依賴 cout 和 cin 的 tie；解除 tie 可加速，但提示文字可能不立即顯示。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::cout / std::cin 基本用法
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::endl 和 '\n' 差在哪？該用哪個？
//     答：'\n' 只輸出換行字元；endl 是輸出 '\n' 再呼叫 flush()。對檔案或管線而言
//     flush 意味著一次 write syscall，在迴圈裡用 endl 可能造成數量級的效能損失。
//     預設用 '\n'，只在真的需要立即可見時（互動提示、崩潰前的 log）才用 endl 或
//     std::flush。
//     追問：程式正常結束時緩衝區會被 flush 嗎？（會，static 解構或 exit() 時會；但
//     abort()、_exit() 或被 signal 殺掉時不會——這正是「log 少了最後幾行」的成因）
//
// 🔥 Q2. cin >> x 讀取失敗時發生什麼？怎麼復原？
//     答：① 設定 failbit ② C++11 起會把目標變數設為 0（C++11 之前是保持原值）
//     ③ 導致失敗的那個字元仍留在緩衝區、不會被消耗。所以只 clear() 不 ignore() 會造成
//     無窮迴圈。標準復原流程：
//     std::cin.clear();
//     std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
//     追問：clear() 的預設參數是什麼？（clear(goodbit)，把狀態位元全清；反過來寫
//     clear(failbit) 是「設成 failbit」，是常見誤用）
//
// ⚠️ 陷阱. cin >> n; 之後接 std::getline(cin, s); 為什麼讀到空字串？
//     答：cin >> n 讀走數字後把換行留在緩衝區（>> 的語意是跳過前導空白、讀到下一個
//     空白為止，不消耗那個分隔符）。getline 一啟動就遇到殘留的 '\n'，判定這一行結束
//     → 回傳空字串。修法是在中間加
//     cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
//     為什麼會錯：以為 >> 會「順便把整行讀完」；它只讀到分隔符為止，尾端分隔符是留給
//     下一次操作判斷的。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <sstream>
#include <string>

int main() {
    // ─────────────────────────────────────────────────────────
    // Demo 1：基本 cout 鏈
    // ─────────────────────────────────────────────────────────
    int a = 1, b = 2;
    std::cout << "[Demo1] a=" << a << " b=" << b << " sum=" << (a + b) << '\n';

    // ─────────────────────────────────────────────────────────
    // Demo 2：用 istringstream 模擬 cin（教學用）
    //   實際工作中 cin 行為一致 — 改成 std::cin 即可
    // ─────────────────────────────────────────────────────────
    std::istringstream in{"  42  3.14  hello"};
    int n;       in >> n;
    double d;    in >> d;
    std::string s; in >> s;
    std::cout << "[Demo2] read: n=" << n
              << " d=" << d
              << " s=" << s << '\n';

    // ─────────────────────────────────────────────────────────
    // Demo 3：while (in >> x) — 標準讀完所有 token
    // ─────────────────────────────────────────────────────────
    std::istringstream nums{"10 20 30 40 50"};
    int x;
    long long total = 0;
    while (nums >> x) total += x;
    std::cout << "[Demo3] sum = " << total << '\n';   // 150

    // ─────────────────────────────────────────────────────────
    // Demo 4：解析失敗的處理 — clear() + 跳過壞 token
    //   istringstream 跟 cin 一樣，遇到「不能解析的字元」會 set fail bit
    //   之後再讀都會立刻失敗、不前進。要 clear() 重置狀態，並 ignore() 跳掉。
    // ─────────────────────────────────────────────────────────
    std::istringstream mixed{"100 abc 200"};
    int v;
    mixed >> v;                               // 讀到 100
    std::cout << "[Demo4] first = " << v << '\n';

    mixed >> v;                               // ❌ 失敗（"abc"）
    if (mixed.fail()) {
        std::cout << "[Demo4] parse failed, recovering...\n";
        mixed.clear();                        // 清除 fail bit
        std::string bad;
        mixed >> bad;                         // 把壞 token 吃掉
    }
    mixed >> v;                               // 現在讀到 200
    std::cout << "[Demo4] recovered = " << v << '\n';

    // ─────────────────────────────────────────────────────────
    // 實用範例 A：解析 "1+2+3" 之類的簡單運算式
    //   工作上常見：解析使用者輸入的「數字 + 運算符 + 數字...」字串。
    //   方法：先讀一個數字，然後反覆讀「char + 數字」直到 stream 結束。
    // ─────────────────────────────────────────────────────────
    {
        std::istringstream in{"1+2+3+10"};
        int sum;
        in >> sum;                            // 讀第一個 1
        char op;
        int next;
        while (in >> op >> next) {            // op='+', next=2; op='+', next=3 ...
            if (op == '+') sum += next;
            else if (op == '-') sum -= next;
        }
        std::cout << "[parse-expr] 1+2+3+10 = " << sum << '\n';   // 16
    }

    // ─────────────────────────────────────────────────────────
    // 實用範例 B：用 cout 印出「key=value」表 — operator<< 鏈式組合
    //   實務上 dump struct 內容、debug print 很常用。
    // ─────────────────────────────────────────────────────────
    {
        struct User { std::string name; int age; double height; };
        User u{"alice", 30, 1.65};

        auto dump = [](std::ostream& os, const User& x) -> std::ostream& {
            return os << "name=" << x.name
                      << " age=" << x.age
                      << " height=" << x.height;
        };
        dump(std::cout, u) << '\n';
    }

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：cin >> string 跟 getline 差在哪？
    //    A：cin >> s 跳空白讀一個 token（遇到空白停）；getline(cin, s)
    //       讀整行直到 '\n'。兩者混用很容易踩 「上一個 >> 留下 \n 在
    //       buffer」的坑（見 08_getline.cpp）。
    //
    //  Q2：cin.fail() 跟 cin.eof() 不同？
    //    A：fail：解析失敗（型別不匹配等）
    //       eof：讀到檔案尾
    //       bad：底層 IO 錯誤（極少見）
    //       三者狀態獨立，要分別處理；good() 是「三者都沒發生」。
    //
    //  Q3：cin 和 cout 的綁定是什麼意思？
    //    A：預設 cin 跟 cout 綁定 — 每次 cin >> 之前會自動 flush cout，確
    //       保 prompt 字串先出現。可用 cin.tie(nullptr) 解除這個綁定（提
    //       速用，見 04_sync_with_stdio.cpp）。
    //
    return 0;
}
