// =============================================================================
//  第 5 課：輸入輸出流（iostream）入門18.cpp  —  cout / cerr / clog 的分工
// =============================================================================
//
// 【主題資訊 Information】
//   extern std::ostream cout;   // 標準輸出 stdout，fd 1，全緩衝（導向檔案時）
//   extern std::ostream cerr;   // 標準錯誤 stderr，fd 2，unitbuf 且 tie 到 cout
//   extern std::ostream clog;   // 標準錯誤 stderr，fd 2，【有緩衝】、未 tie
//   標準版本：C++98 起（宣告於 <iostream>）
//   複雜度  ：O(輸出長度)；cerr 每次額外付一次 flush 的系統呼叫
//   標頭檔  ：<iostream>
//
// 【詳細解釋 Explanation】
//
// 【1. 三者的真正差別（本機實測，非背誦）】
//   本機用 flags() 與 tie() 直接查詢，得到：
//       cout : unitbuf=0   （不會每次自動 flush）
//       cerr : unitbuf=1   tie() 指向 cout
//       clog : unitbuf=0   tie() 為 nullptr
//   換句話說：
//     • cerr 和 clog 【寫到同一個檔案描述子 fd 2】，
//       兩者的差別【不在去哪裡】，而在【緩衝策略】。
//     • cerr 設了 unitbuf → 每次輸出後立刻 flush，保證即時可見。
//     • clog 沒設 unitbuf → 會累積在緩衝區，適合大量日誌（效能好得多）。
//
// 【2. 「cerr 是無緩衝的」這句話並不精確】
//   常見教材（包括本檔原始註解）寫 cerr「無緩衝」、clog「有緩衝」。
//   更準確的說法是：cerr 有緩衝區，但設定了 unitbuf 旗標，
//   使得【每一次】格式化輸出結束後都會自動 flush。
//   效果上接近無緩衝，但機制不同 —— 這個區別在自己實作 ostream 時很重要。
//
// 【3. cerr 被 tie 到 cout 的實際後果】
//   cerr.tie() 指向 cout，意思是：
//   【每次寫入 cerr 之前，會先自動 flush cout】。
//   這解決了一個很實際的問題：
//       std::cout << "正在處理檔案 A";   // 沒有換行，留在緩衝區
//       std::cerr << "錯誤：檔案損毀\n";
//   若沒有 tie，錯誤訊息會先出現，而「正在處理檔案 A」還卡在緩衝區裡，
//   使用者會看到錯誤訊息卻不知道是處理哪個檔案時發生的。
//   本機實測（把兩者都導向同一個檔案）確實得到：
//       A-coutB-cerr
//       C-cout
//   —— A 在 B 之前，證明 cout 確實被 tie 機制強制 flush 了。
//   注意 clog 【沒有】這個 tie，所以用 clog 記錄時不保證與 cout 的相對順序。
//
// 【4. 為什麼要把錯誤和正常輸出分開】
//   這是 Unix 的核心設計哲學，實務價值極高：
//       ./prog > result.txt          # 只有正常輸出進檔案，錯誤仍顯示在終端
//       ./prog 2> error.log          # 只有錯誤進 log
//       ./prog | grep xxx            # 管線只接 stdout，錯誤訊息不會污染資料流
//   如果把錯誤訊息也印到 cout，上面每一種用法都會壞掉 ——
//   錯誤訊息會混進資料裡被下游程式當成資料解析。
//   這就是為什麼所有診斷訊息都應該走 cerr/clog，而不是 cout。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼導向檔案時輸出順序可能「看起來錯亂」
//   cout 導向檔案時通常是【全緩衝】（4096 bytes 或更大），
//   而 cerr 每次都 flush。若沒有 tie 機制，
//   大量 cout 輸出會集中在程式結束時才寫出，
//   而 cerr 的訊息早已寫進去 → 交錯順序與程式碼順序不一致。
//   cerr 的 tie 只保證「寫 cerr 前先 flush cout」，
//   clog 則完全沒有這個保護。
//
// (B) std::endl 與 flush 的成本
//   每次 flush 都是一次 write 系統呼叫。
//   在迴圈裡對 cerr 大量輸出會非常慢（每行一次系統呼叫），
//   這正是 clog 存在的理由 —— 同樣送到 stderr，但可以攢一批再寫。
//   高頻日誌請用 clog，真正的錯誤才用 cerr。
//
// (C) 這三個物件的初始化順序問題
//   它們是具有靜態儲存期的全域物件。
//   <iostream> 內含一個 std::ios_base::Init 物件，
//   確保只要包含了這個標頭，這三個串流就會在 main 之前完成初始化。
//   但【其他】全域物件的建構子若使用 cout，
//   仍可能踩到靜態初始化順序問題（static initialization order fiasco）。
//   在全域物件的建構子裡輸出訊息是不安全的做法。
//
// 【注意事項 Pay Attention】
//   1. cerr 與 clog 【都】寫到 stderr（fd 2），差別只在緩衝策略。
//      「clog 寫到別的地方」是常見誤解。
//   2. cerr 是 unitbuf（每次輸出後 flush），不是嚴格意義的「無緩衝」。
//   3. cerr 被 tie 到 cout，寫 cerr 前會先 flush cout；clog 沒有這個保證。
//   4. 診斷訊息一律不要用 cout —— 會污染管線與重導向的資料流。
//   5. 高頻日誌用 clog（省下大量 flush 系統呼叫），真正的錯誤才用 cerr。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】cout / cerr / clog 的差別
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. cerr 和 clog 有什麼不同？它們是不是輸出到不同地方？
//     答：【輸出到同一個地方】—— 兩者都寫到 stderr（fd 2）。
//         差別在緩衝策略：cerr 設定了 unitbuf 旗標，每次輸出後自動 flush，
//         保證即時可見；clog 沒有設，會累積在緩衝區裡，
//         適合高頻日誌（省下大量 write 系統呼叫）。
//         本機實測 flags() & unitbuf 的結果是 cerr=1、clog=0。
//     追問：那為什麼要分成兩個物件而不是一個？
//           → 因為「必須立刻看到」和「可以慢慢寫」是不同需求。
//             程式崩潰前的錯誤訊息必須已經寫出去（cerr）；
//             但每秒上千筆的日誌若每筆都 flush，效能會慘不忍睹（clog）。
//
// 🔥 Q2. 為什麼診斷訊息不能印到 std::cout？
//     答：因為 stdout 是【資料通道】。一旦把錯誤訊息混進去：
//             ./prog | grep xxx        錯誤訊息會被當成資料解析
//             ./prog > result.txt      錯誤訊息被寫進結果檔、且終端看不到
//         把診斷走 stderr 才能讓「資料」與「訊息」各走各的路，
//         這是 Unix 工具能自由組合的基礎。
//     追問：那 stderr 導向檔案的寫法是什麼？
//           → 2> error.log；要合併兩者則是 > all.log 2>&1（順序有意義）。
//
// ⚠️ 陷阱. cout 沒有立刻 flush，那先寫 cout 再寫 cerr，
//          輸出順序會不會顛倒？
//     答：對 cerr 而言【不會】—— 因為 cerr.tie() 指向 cout，
//         標準規定每次寫入 cerr 之前會先 flush 被 tie 的串流。
//         本機實測（cout 不加換行、兩者都導向同一檔案）得到
//         "A-coutB-cerr" 然後 "C-cout"，順序正確。
//         但對 clog 就【沒有這個保證】——clog.tie() 是 nullptr。
//     為什麼會錯：兩種相反的誤解都很常見 ——
//         一種人以為「緩衝一定會造成順序錯亂」，忽略了 tie 機制；
//         另一種人（更危險）把 cerr 的保證直接套到 clog 身上，
//         結果在日誌裡看到錯亂的時間順序而百思不解。
//         精確的說法是：cerr 有 tie 保護、clog 沒有。
//         需要嚴格順序時，要嘛統一用同一個串流，
//         要嘛在關鍵點手動 std::cout.flush()。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>

int main() {
    // 正常輸出
    std::cout << "這是正常訊息" << std::endl;
    
    // 錯誤輸出（無緩衝，立即顯示）
    std::cerr << "這是錯誤訊息" << std::endl;
    
    // 日誌輸出（有緩衝）
    std::clog << "這是日誌訊息" << std::endl;
    
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 5 課：輸入輸出流（iostream）入門18.cpp" -o io18
// 執行: ./io18            # 終端上三行都看得到（stdout 與 stderr 都到終端）
//       ./io18 2>/dev/null   # 只剩「這是正常訊息」
//       ./io18 1>/dev/null   # 只剩「這是錯誤訊息」與「這是日誌訊息」

// 註 1:【本檔的重點只有在重導向時才看得出來】。
//      直接執行時三行都印在終端上，看不出任何差別；
//      必須把 stdout 與 stderr 分開，才會發現
//      cerr 與 clog 【都】走 stderr（fd 2），只有 cout 走 stdout（fd 1）。
//      本機實測三種重導向的結果如下方預期輸出所列。
//
// 註 2:原始註解寫 cerr「無緩衝」、clog「有緩衝」，
//      更精確的說法是：cerr 設定了 unitbuf 旗標（每次輸出後自動 flush），
//      clog 沒有設。本機用 flags() 直接查詢的實測結果是
//        cout unitbuf=0 / cerr unitbuf=1 / clog unitbuf=0
//      兩者都有緩衝區，差別在「要不要每次都 flush」。
//
// 註 3:另一個實測到、但原始註解沒提的重要事實：
//      cerr.tie() 指向 cout（clog.tie() 是 nullptr）。
//      這表示寫 cerr 之前會先自動 flush cout，
//      確保錯誤訊息不會搶在尚未輸出的正常訊息前面。
//      clog 沒有這個保護。
//
// 註 4:本檔三行都用了 std::endl，所以即使沒有 unitbuf 也會被 flush。
//      要觀察緩衝差異，得把 endl 換成 '\n' 才看得出來。

// === 預期輸出 ===
// --- 直接執行 ./io18（stdout 與 stderr 都在終端）---
// 這是正常訊息
// 這是錯誤訊息
// 這是日誌訊息
//
// --- 只看 stdout：./io18 2>/dev/null ---
// 這是正常訊息
//
// --- 只看 stderr：./io18 1>/dev/null ---
// 這是錯誤訊息
// 這是日誌訊息
