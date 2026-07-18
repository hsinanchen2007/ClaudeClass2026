/*
 * ================================================================
 * 【第5課：輸入輸出流（iostream）入門】總複習 summary.cpp
 * ================================================================
 * 編譯方式：g++ -std=c++17 -Wall -Wextra -o summary summary.cpp
 *
 * 本課重點：
 * 1. 什麼是「流（Stream）」：資料流動的抽象概念
 * 2. 四個標準流物件：cout、cin、cerr、clog
 * 3. 插入運算子 << 與鏈式輸出
 * 4. cout 自動型別識別（不需要格式符，對比 C 的 printf）
 * 5. 換行方式比較：std::endl vs '\n' vs std::flush
 * 6. 提取運算子 >>：讀取單一值、連續讀取多值
 * 7. cin 的空白字元處理特性（>> 遇到空白停止）
 * 8. std::getline：讀取含空格的整行輸入
 * 9. cin >> 與 getline 混用的陷阱與解決方案（cin.ignore）
 * 10. 流狀態標誌：good()、eof()、fail()、bad()
 * 11. 輸入錯誤處理：cin.fail()、cin.clear()、cin.ignore()
 * 12. 格式化輸出（iomanip）：setprecision、fixed、scientific、
 *     setw、setfill、left/right、hex/oct/dec、boolalpha
 * 13. cerr（無緩衝錯誤輸出）與 clog（有緩衝日誌輸出）
 * ================================================================
 */

#include <iostream>
#include <iomanip>   // 格式化控制器所需標頭
#include <string>
#include <limits>    // std::numeric_limits

// ================================================================
// ===== 重點一：什麼是「流」？=====
// 說明：流（Stream）是資料流動的抽象模型。
//       輸入流：資料從外部（鍵盤、檔案）流向程式
//       輸出流：資料從程式流向外部（螢幕、檔案）
// 重要性：C++ 的 I/O 以流為核心，比 C 的 printf/scanf 更型別安全、
//         更可擴展（可為自訂類別重載 << 和 >>）。
//
// 四個標準流物件（定義在 <iostream>）：
//   std::cout  → 標準輸出（螢幕），有緩衝，對應 C 的 stdout
//   std::cin   → 標準輸入（鍵盤），對應 C 的 stdin
//   std::cerr  → 標準錯誤，無緩衝（立即輸出），對應 C 的 stderr
//   std::clog  → 標準日誌，有緩衝，對應 C 的 stderr
// ================================================================

// ================================================================
// ===== 重點十一：安全的輸入輔助函式 =====
// 說明：將輸入驗證邏輯封裝成函式，避免程式在錯誤輸入時崩潰。
// 流程：嘗試讀取 → 檢查 cin.fail() → 若失敗則 clear() + ignore() 重試
// ================================================================
int readInt(const std::string& prompt) {
    int value;
    while (true) {
        std::cout << prompt;
        std::cin >> value;

        if (std::cin.fail()) {
            // cin.fail() 為 true 代表輸入與目標型別不符（例如輸入了字母）
            std::cout << "  [錯誤] 請輸入有效的整數！\n";

            // cin.clear()：清除錯誤狀態旗標，讓 cin 恢復可用
            std::cin.clear();

            // cin.ignore(n, delim)：從緩衝區丟棄字元，直到遇到 delim 或丟棄 n 個
            // numeric_limits<streamsize>::max() 表示「不限數量」，直到換行為止
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        } else {
            // 成功讀取後也要清除緩衝區中剩餘的換行符
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return value;
        }
    }
}

// ================================================================
// 主程式：展示所有重點
// ================================================================
int main() {
    std::cout << "================================================================\n";
    std::cout << "  第5課：輸入輸出流（iostream）入門 總複習\n";
    std::cout << "================================================================\n\n";

    // ---------------------------------------------------------------
    // 重點三 & 四：插入運算子 << 與自動型別識別
    // ---------------------------------------------------------------
    // 說明：<< 稱為「插入運算子（insertion operator）」，
    //       將右側的資料「插入」到左側的輸出流。
    //       可以連續串接（鏈式操作），因為每次 << 都傳回 ostream&。
    //       cout 無需格式符，自動根據資料型別決定輸出格式。
    std::cout << "===== [重點3/4] 插入運算子 << 與自動型別識別 =====\n";
    {
        int    i = 42;
        double d = 3.14159;
        char   c = 'A';
        const char* s = "Hello";
        bool   b = true;

        // 自動識別型別，不需要 %d %f %c 等格式符
        std::cout << "  int:    " << i << "\n";
        std::cout << "  double: " << d << "\n";
        std::cout << "  char:   " << c << "\n";
        std::cout << "  string: " << s << "\n";
        std::cout << "  bool:   " << b << "  (預設輸出 1 或 0)\n";

        // 鏈式輸出：多個 << 串在同一行
        int age = 25;
        double height = 175.5;
        std::cout << "  年齡: " << age << " 歲，身高: " << height << " cm\n";
    }
    std::cout << "\n";

    // ---------------------------------------------------------------
    // 重點五：換行方式比較
    // ---------------------------------------------------------------
    // 說明：三種換行（或清空緩衝區）的方式，效能和行為各有差異。
    //   std::endl → 輸出 '\n' 並立即清空（flush）緩衝區（較慢）
    //   '\n'      → 僅輸出換行字元，不清空緩衝區（較快）
    //   std::flush→ 只清空緩衝區，不換行
    // 建議：大量輸出時用 '\n'；需要立即顯示（如進度條）時用 std::endl。
    std::cout << "===== [重點5] 換行方式比較 =====\n";
    std::cout << "  Line 1（std::endl）" << std::endl;
    std::cout << "  Line 2（'\\n'）\n";
    std::cout << "  Line 3（std::flush，不換行）" << std::flush;
    std::cout << " ← 繼續在同一行\n";
    std::cout << "\n";

    // ---------------------------------------------------------------
    // 重點六 & 七：提取運算子 >> 的特性
    // ---------------------------------------------------------------
    // 說明：>> 稱為「提取運算子（extraction operator）」。
    //       特性一：自動跳過前導空白（空格、Tab、換行）
    //       特性二：在遇到空白或型別不符時停止讀取
    //       特性三：可以連續串接讀取多個值（>> a >> b >> c）
    //
    // 注意：本 summary.cpp 不做互動式輸入，以下僅展示程式結構。
    std::cout << "===== [重點6/7] 提取運算子 >> 說明 =====\n";
    std::cout << "  std::cin >> age          → 讀取一個整數\n";
    std::cout << "  std::cin >> a >> b >> c  → 連續讀取三個值（空格/換行分隔）\n";
    std::cout << "  注意：>> 遇到空白就停止，'Hello World' 只會讀到 'Hello'\n";
    std::cout << "\n";

    // ---------------------------------------------------------------
    // 重點八：std::getline
    // ---------------------------------------------------------------
    // 說明：std::getline(cin, str) 讀取整行（包含空格），
    //       直到遇到換行符 '\n' 才停止，換行符被丟棄。
    // 對比：cin >> str 只讀到第一個空白就停止。
    // 使用場景：讀取全名、地址、句子等包含空格的輸入。
    std::cout << "===== [重點8] std::getline =====\n";
    std::cout << "  std::getline(std::cin, fullLine) → 讀取整行含空格\n";
    std::cout << "  適合讀取：全名、地址、完整句子\n";
    std::cout << "\n";

    // ---------------------------------------------------------------
    // 重點九：cin >> 與 getline 混用陷阱
    // ---------------------------------------------------------------
    // 說明：cin >> number 讀取數字後，緩衝區中仍殘留換行符 '\n'。
    //       緊接著呼叫 getline 時，它會讀到這個 '\n' 就立刻結束，
    //       導致讀到空字串。
    //
    // 解決方案：在 cin >> 之後、getline 之前，呼叫 cin.ignore() 清除換行符。
    //
    // 錯誤示範（不要這樣做）：
    //   std::cin >> age;
    //   std::getline(std::cin, name);  // 會讀到空字串！
    //
    // 正確做法：
    //   std::cin >> age;
    //   std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    //   std::getline(std::cin, name);  // 正確讀取整行
    std::cout << "===== [重點9] cin >> 與 getline 混用陷阱 =====\n";
    std::cout << "  問題：cin >> age 後緩衝區殘留 '\\n'，getline 會讀到空字串\n";
    std::cout << "  解法：cin.ignore(numeric_limits<streamsize>::max(), '\\n');\n";
    std::cout << "\n";

    // ---------------------------------------------------------------
    // 重點十：流狀態標誌
    // ---------------------------------------------------------------
    // 說明：cin（和其他 istream）維護四個狀態旗標：
    //   good()  → true 表示流狀態完全正常，可以繼續操作
    //   eof()   → true 表示已到達輸入的結尾（End Of File）
    //   fail()  → true 表示上一次操作失敗（型別不符、格式錯誤），可恢復
    //   bad()   → true 表示嚴重錯誤（底層 I/O 故障），通常不可恢復
    //
    // 恢復流：先呼叫 cin.clear() 清除旗標，再 cin.ignore() 清除緩衝區。
    std::cout << "===== [重點10] 流狀態標誌 =====\n";
    std::cout << "  cin.good()  → 一切正常（可繼續讀取）\n";
    std::cout << "  cin.eof()   → 已到輸入結尾（EOF）\n";
    std::cout << "  cin.fail()  → 操作失敗，可恢復（呼叫 clear() + ignore()）\n";
    std::cout << "  cin.bad()   → 嚴重 I/O 錯誤，不可恢復\n";
    std::cout << "\n";

    // ---------------------------------------------------------------
    // 重點十一：輸入驗證迴圈
    // ---------------------------------------------------------------
    // 說明：實際應用中應驗證輸入，防止程式因錯誤輸入而崩潰或陷入無窮迴圈。
    //       本課提供了一個「正整數驗證」迴圈的模式：
    //       1. 嘗試讀取
    //       2. 若 cin.fail()（型別錯誤）→ clear() + ignore() 再試
    //       3. 若數值範圍不符 → 提示再試
    //       4. 若正確 → break 離開迴圈
    //
    //（以下為靜態示範，不做真正的互動讀取）
    std::cout << "===== [重點11] 輸入驗證迴圈模式 =====\n";
    std::cout << "  while (true) {\n";
    std::cout << "      cin >> number;\n";
    std::cout << "      if (cin.fail()) { clear(); ignore(); }\n";
    std::cout << "      else if (number <= 0) { 提示範圍錯誤 }\n";
    std::cout << "      else { break; }  // 正確輸入\n";
    std::cout << "  }\n";
    std::cout << "\n";

    // ---------------------------------------------------------------
    // 重點十二：格式化輸出（iomanip）
    // ---------------------------------------------------------------
    // 說明：<iomanip> 提供「操縱器（manipulator）」來控制輸出格式。
    //       大部分操縱器會持續生效（sticky），直到被覆蓋。
    //       例外：setw() 只對緊接的下一個輸出有效（non-sticky）。

    std::cout << "===== [重點12] 格式化輸出（iomanip） =====\n";

    double pi = 3.14159265358979;

    // --- 12.1 精度控制 ---
    // setprecision(n)：設定有效位數（預設模式）或小數位數（fixed/scientific 模式）
    std::cout << "  [精度控制]\n";
    std::cout << "  預設:           " << pi << "\n";
    std::cout << "  setprecision(3):" << std::setprecision(3) << pi << "\n";
    std::cout << "  setprecision(8):" << std::setprecision(8) << pi << "\n";

    // fixed：固定小數點格式，此後 setprecision 代表小數位數
    std::cout << "  fixed+prec(2):  " << std::fixed << std::setprecision(2) << pi << "\n";

    // scientific：科學記號格式
    std::cout << "  scientific+p(3):" << std::scientific << std::setprecision(3) << pi << "\n";

    // 恢復預設浮點格式
    std::cout << std::defaultfloat << std::setprecision(6);
    std::cout << "\n";

    // --- 12.2 欄位寬度與對齊 ---
    // setw(n)：設定欄位最小寬度（只對緊接的一個輸出有效！）
    // left/right：對齊方式（right 為預設）
    // setfill(c)：填充字元（預設為空格）
    std::cout << "  [欄位寬度與對齊]\n";
    std::cout << "  右對齊 setw(10): [" << std::right << std::setw(10) << 42    << "]\n";
    std::cout << "  右對齊 setw(10): [" << std::right << std::setw(10) << "Hi"  << "]\n";
    std::cout << "  左對齊 setw(10): [" << std::left  << std::setw(10) << 42    << "]\n";
    std::cout << "  左對齊 setw(10): [" << std::left  << std::setw(10) << "Hi"  << "]\n";
    std::cout << "  補零   setw(6):  ["
              << std::right << std::setfill('0') << std::setw(6) << 42 << "]\n";
    std::cout << std::setfill(' ');  // 恢復填充字元
    std::cout << "\n";

    // --- 12.3 數值進位顯示 ---
    // hex：十六進位，oct：八進位，dec：十進位（預設）
    // showbase：顯示進位前綴（0x 或 0）
    std::cout << "  [數值進位顯示]\n";
    int num = 255;
    std::cout << "  dec: " << std::dec  << num << "\n";
    std::cout << "  hex: " << std::hex  << num << "\n";
    std::cout << "  oct: " << std::oct  << num << "\n";
    std::cout << std::showbase;
    std::cout << "  帶前綴 hex: " << std::hex << num << "\n";
    std::cout << "  帶前綴 oct: " << std::oct << num << "\n";
    std::cout << std::dec << std::noshowbase;  // 恢復預設
    std::cout << "\n";

    // --- 12.4 布林值顯示 ---
    // boolalpha：將 bool 輸出為 "true"/"false" 而非 1/0
    std::cout << "  [布林值顯示]\n";
    bool t = true, f = false;
    std::cout << "  預設:     true=" << t << ", false=" << f << "\n";
    std::cout << std::boolalpha;
    std::cout << "  boolalpha: true=" << t << ", false=" << f << "\n";
    std::cout << std::noboolalpha;
    std::cout << "\n";

    // --- 12.5 表格輸出綜合示範 ---
    // 展示 setw、left、fixed、setprecision 的組合使用
    std::cout << "  [表格輸出示範]\n";
    std::cout << std::left;
    std::cout << std::setw(12) << "商品"
              << std::setw(10) << "單價"
              << std::setw(8)  << "數量"
              << std::setw(12) << "小計" << "\n";
    std::cout << std::string(42, '-') << "\n";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << std::setw(12) << "蘋果"  << std::setw(10) << 35.0  << std::setw(8) << 3 << std::setw(12) << 35.0*3  << "\n";
    std::cout << std::setw(12) << "香蕉"  << std::setw(10) << 25.5  << std::setw(8) << 5 << std::setw(12) << 25.5*5  << "\n";
    std::cout << std::setw(12) << "橘子"  << std::setw(10) << 40.0  << std::setw(8) << 2 << std::setw(12) << 40.0*2  << "\n";
    std::cout << std::string(42, '-') << "\n";
    std::cout << std::setw(30) << "總計: " << std::setw(12) << (35.0*3+25.5*5+40.0*2) << "\n";
    std::cout << std::defaultfloat << std::right;  // 恢復預設
    std::cout << "\n";

    // ---------------------------------------------------------------
    // 重點十三：cerr 與 clog
    // ---------------------------------------------------------------
    // 說明：
    //   std::cerr → 無緩衝，立即輸出。用於錯誤訊息，
    //               確保程式崩潰時訊息也能顯示出來。
    //   std::clog → 有緩衝，效能較好。用於日誌記錄。
    //
    // 在命令列可以分別重導向：
    //   ./program > output.txt 2> errors.txt
    //   ( 1 = stdout, 2 = stderr )
    std::cout << "===== [重點13] cerr 與 clog =====\n";
    std::cout << "  cout（標準輸出，有緩衝）\n";
    std::cerr << "  cerr（標準錯誤，無緩衝，立即顯示）\n";
    std::clog << "  clog（日誌輸出，有緩衝）\n";
    std::cout << "\n";

    // ---------------------------------------------------------------
    // iostream vs printf/scanf 快速對比
    // ---------------------------------------------------------------
    std::cout << "===== [補充] iostream vs printf/scanf 對比 =====\n";
    std::cout << "  類型安全：printf 需 %d %f 等格式符，cout 自動識別\n";
    std::cout << "  可擴展性：自訂類別可重載 << 和 >>，printf 無法擴展\n";
    std::cout << "  效能：    printf 通常稍快，但 '\n' 比 endl 快（不 flush）\n";
    std::cout << "  錯誤偵測：cout 部分在編譯期，printf 格式錯誤只在執行期\n";

    std::cout << "\n================================================================\n";
    std::cout << "  總複習完成！\n";
    std::cout << "================================================================\n";

    return 0;
}
