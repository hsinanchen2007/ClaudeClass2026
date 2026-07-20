// =============================================================================
// 檔名: shrink_to_fit.cpp
// 主題: std::string::shrink_to_fit (請求釋放多餘容量)
// 參考:
//   - https://en.cppreference.com/cpp/string/basic_string/shrink_to_fit
//   - https://cplusplus.com/reference/string/string/shrink_to_fit/
// =============================================================================
//
// 【函式資訊 Information】
//   void shrink_to_fit();      // C++11 起
//
// 參數: 無
// 回傳: 無
// 例外: 標準未強制 noexcept;若觸發 reallocation 失敗會丟 std::bad_alloc。
//       多數實作在「無變化」時為 noexcept-like;設計上採 strong exception
//       guarantee — 若失敗,字串保持原狀。
//
// =============================================================================
//
// 【詳細解釋 Explanation - 設計理念與底層運作】
//
// 1. 它是「請求」不是「命令」(Non-binding Request)
//    -----------------------------------------------------------------
//    標準明文寫道:"This is a non-binding request to reduce capacity()
//    to size()."  關鍵詞 non-binding 的意思是:
//      - 實作 *允許* 完全忽略這個呼叫;
//      - 實作 *允許* 把 capacity 縮到 size,也允許縮一半,也允許不縮;
//      - 你不能寫程式「依賴 shrink_to_fit 真的縮了」。
//    為什麼?標準想保留實作彈性 — 例如某些實作可能有「最小 heap chunk」
//    機制 (例如 glibc malloc 一定配置 16 bytes 的倍數),縮到剛好 size
//    反而沒意義;或實作可能延後執行 (lazy)。
//
//    但實務上:libstdc++ / libc++ / MSVC STL 三大實作都會「真的縮」,
//    只要 size() 不在 SSO 範圍內。
//
// 2. 為什麼需要 shrink_to_fit?
//    -----------------------------------------------------------------
//    當字串經歷「先變大、再變小」的生命週期 (例如先讀進整個檔案、再
//    erase 大部分內容),capacity 會卡在歷史最大值不會自動縮小。
//    long-lived 物件 (cache、config、長時間存活的 ServiceState) 累積
//    幾百萬個這樣的字串,記憶體浪費可能達 MB ~ GB 等級。
//    shrink_to_fit 是把這些「沉沒記憶體」歸還給 heap 的標準方法。
//
// 3. 底層運作
//    -----------------------------------------------------------------
//    通常實作為:
//        if (capacity() > size() + threshold) {
//            string tmp(*this);     // 拷貝建構,新 buffer 剛好 size
//            swap(tmp);             // 換進去,tmp 解構釋放舊 buffer
//        }
//    成本 O(N) — 因為要拷貝整個字串。
//
// 4. 與 SSO 的互動
//    -----------------------------------------------------------------
//    若 size() 已在 SSO 範圍內,但目前用的是 heap buffer (因為歷史曾
//    超過 SSO),shrink_to_fit 會把資料搬回 SSO 內嵌 buffer,釋放 heap。
//    若一開始就在 SSO 範圍內,shrink_to_fit 通常什麼都不做。
//
// 5. C++11 ~ C++23 的演進
//    -----------------------------------------------------------------
//    - C++11: 引入,定義為 non-binding request。
//    - C++17: reserve() 無參數版被 deprecate (它的縮減語意被移到此函式)。
//    - C++20: reserve() 無參數版正式移除/變 no-op。shrink_to_fit 是
//             縮減容量的「唯一標準介面」。
//    - C++23: 行為未變,但 constexpr 友善度持續強化。
//
// 6. 例外安全 (Exception Safety)
//    -----------------------------------------------------------------
//    Strong exception guarantee:若新配置失敗 (bad_alloc),字串內容、
//    size、capacity、迭代器都不變。雖然標準允許實作只提供 basic
//    guarantee,但主流實作都遵守 strong。
//
// 7. 迭代器/指標/參考失效規則
//    -----------------------------------------------------------------
//    若實作真的縮小 (觸發 reallocation):
//      - 所有迭代器、指標、參考全部失效;
//      - c_str() / data() 之前回傳的指標也失效。
//    若實作沒動作:
//      - 全部仍有效。
//    結論:不論如何,呼叫 shrink_to_fit 後都應該重新取得 c_str() / 迭代器。
//
// 8. 何時不該用 shrink_to_fit?
//    -----------------------------------------------------------------
//    - 短命物件 (區域變數、立刻要 destroy):浪費 CPU,反正馬上釋放;
//    - 還會繼續成長的字串:你只是逼它待會再次 reallocate;
//    - 在 hot loop 中對「會被覆寫」的物件呼叫:純粹拖累效能。
//
// =============================================================================
//
// 【概念補充 Concept Deep Dive】
//
// (A) "Non-binding" 是什麼意思?
//     C++ 標準裡有一些 "should" / "may" 的設計空間給實作。non-binding
//     request 屬於最弱的一種 — 標準說「呼叫者請求縮減」,但「實作可以
//     完全不理」。類似的還有 std::vector::shrink_to_fit、std::deque::
//     shrink_to_fit。 反例是 reserve(n) — 那是 binding,實作 *必須*
//     讓 capacity >= n。 寫測試或 assertion 時請記得不要 assert
//     capacity() == size() after shrink_to_fit。
//
// (B) shrink_to_fit vs swap trick
//     C++03 時代沒有 shrink_to_fit,大家用 "swap idiom":
//         std::string(s).swap(s);   // 拷貝出剛好的副本,swap 進去
//     效果與 shrink_to_fit 相同。但這個 trick 同樣是 non-binding
//     (因為依賴實作真的「拷貝會縮容量」)。C++11 後請優先用
//     shrink_to_fit,語意更清楚。
//
// (C) 為什麼設計成「請求」而非「強制」?
//     考量:
//       1. 某些 allocator 是 page-based,縮一半其實沒釋放任何記憶體;
//       2. 某些實作有「下次擴張會用到」的快取邏輯;
//       3. 強制縮代表強制 reallocation,會破壞 strong exception guarantee
//          的預期成本。
//     讓實作有彈性,語言不會把效能假設綁死。
//
// (D) 與 RAII 的關係
//     若你的目標是「現在就釋放記憶體」,而字串本身可丟棄,最快的方法是
//     讓它 destroy:讓物件離開 scope,或 s = std::string(); 也行。
//     shrink_to_fit 適用於「物件本身要保留、但內部 buffer 想瘦身」的場景。
//
// =============================================================================
//
// 【注意事項 Pay Attention】
// 1. 是「請求」非「命令」— 實作可忽略,不要寫 assert 依賴它。
// 2. 觸發 reallocation 時迭代器/指標/參考全部失效。
// 3. 在 SSO 範圍內可能無變化(因為已經沒有 heap 配置)。
// 4. 適合「字串長度大幅縮減後 + 物件還會長時間存活」的場景。
// 5. 例外安全: strong guarantee — 失敗則內容不變。
// 6. 對短命字串 / hot loop 中的字串使用是反模式。
// =============================================================================

/*
補充筆記：std::string::shrink_to_fit
  - std::string::shrink_to_fit 需要同時考慮內容、容量與舊指標失效；字串 API 常會配置或搬移緩衝區。
  - 回傳位置的函式要先處理 npos，回傳 pointer/view 的函式要追蹤來源 string 生命週期。
  - UTF-8 文字下，size 代表位元組數，不代表使用者看到的字元數。
  - std::string::shrink_to_fit 是 std::string 主題的一部分；先分清楚這個操作會讀取、追加、插入、刪除、搜尋，還是只暴露底層字元。
  - std::string 的索引型別是 size_type；和 int 混用時要小心 unsigned underflow，尤其是倒著迴圈或拿 npos 比較。
  - 修改字串內容或容量可能讓先前取得的 iterator、reference、pointer、c_str() 指標失效。
  - operator[] 不做範圍檢查，at() 越界會丟 std::out_of_range；教學範例可比較兩者，工作程式要依錯誤處理需求選。
  - find/rfind/find_first_of 這類搜尋函式失敗回傳 std::string::npos；判斷時應和 npos 比，不要寫成 >= 0。
  - 字串和 C string 互通時要記得 null terminator；std::string 可以保存內含 \0 的資料，但很多 C API 會在第一個 \0 停止。
  - 若只是傳入唯讀文字片段且不需要擁有資料，std::string_view 可避免複製；但它不能延長原字串生命週期。
  - 處理中文或 UTF-8 時，std::string 的 size() 回傳 byte 數，不是人眼看到的字元數。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】shrink_to_fit
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. shrink_to_fit() 保證釋放記憶體嗎?
//     答:不保證。標準把它定義為「非約束性請求(non-binding request)」,實作
//     可以完全忽略而什麼都不做,呼叫後 capacity() 仍可能大於 size()。它也不會
//     改變 size() 或內容,只可能改變 capacity()。
//     追問:C++11 之前怎麼做?→ swap trick:std::string(s).swap(s),用一個剛好
//     大小的臨時物件跟自己交換。但在 SSO 生效的短字串上這招也可能毫無效果。
//
// Q2. 呼叫 shrink_to_fit() 之後,原本的 iterator 與 c_str() 指標還有效嗎?
//     答:只要實作真的重新配置了,全部失效——iterator、reference、data()/c_str()
//     回傳的指標都不能再用。因為「有沒有真的縮」是實作決定的,你無法預測是否
//     失效,所以正確寫法是一律當作已失效、重新取得。
//
// ⚠️ 陷阱. 需要立刻回收大量記憶體時,shrink_to_fit() 夠可靠嗎?
//     答:不夠。它可以被忽略,而且就算縮了,釋放的也只是那一塊 buffer,是否還給
//     OS 取決於 allocator。真正需要確定性回收時,應該改用生命週期控制
//     (讓 string 離開作用域)而不是依賴這個請求。
//     為什麼會錯:名字看起來像命令句,很多人把它當成保證會執行的操作。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <vector>

void demoShrink() {
    std::string s;
    s.reserve(1000);                            // 故意製造大 capacity
    s = "Hi";                                   // size 變回 2,但 capacity 仍大
    std::cout << "before shrink: size=" << s.size()
              << ", capacity=" << s.capacity() << "\n";

    s.shrink_to_fit();                          // 請求縮減
    std::cout << "after  shrink: size=" << s.size()
              << ", capacity=" << s.capacity() << "\n";
}

// -----------------------------------------------------------------------------
// 【實務範例】處理完大量字串後釋放記憶體
// 為何用 shrink_to_fit: 在處理完大檔/大字串後,釋放掉不再需要的容量,
//                       讓記憶體歸還給 heap。回傳前的最後一步動作。
// -----------------------------------------------------------------------------
std::string processLargeText(std::string&& text) {
    std::string result;
    result.reserve(text.size());                // 最壞情況 (沒任何空白要刪) 預估

    for (char c : text) {
        if (c != ' ') result += c;
    }
    // 處理完後,result.capacity() 可能遠大於實際 size()
    result.shrink_to_fit();                     // 回傳前瘦身,呼叫端拿到剛好的 buffer
    return result;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1047. Remove All Adjacent Duplicates In String (Easy)
//
// 題目敘述:
//   給字串 s,持續移除「兩個相鄰且相同」的字元,直到無法再移除為止,
//   回傳結果。
//   範例:
//     "abbaca" → "ca"
//        a(bb)aca  → aaca
//        (aa)ca    → ca
//     "azxxzy" → "ay"
//
// 為何用 shrink_to_fit:
//   經典 stack 解法 — 用一個 std::string 當 stack:遇到相同就 pop_back,
//   不同就 push_back。整個過程中,結果字串會從「最壞情況 = s.size()」逐
//   步縮短。處理完後,result 的 capacity 仍是輸入字串的最大長度,卻只剩
//   幾個字元。若這個結果要長期保留(放進快取、傳給上層服務),先 shrink_to_fit
//   把多餘的 buffer 還回 heap,長期記憶體使用會明顯改善。
//
//   注意:競賽寫法只看時間複雜度通常省略 shrink_to_fit;但「教學版本」
//   要強調「最終結果遠小於輸入」這種模式正是 shrink_to_fit 的標準場景。
//
// 解題思路:
//   1. 用一個 std::string stack 模擬堆疊,reserve 最大長度避免 realloc。
//   2. 走訪 s,若 stack.back() == c 則 pop_back;否則 push_back。
//   3. 處理完後 stack 的 size 通常 << capacity,呼叫 shrink_to_fit。
//
// 複雜度:時間 O(n),空間 O(n)。
// -----------------------------------------------------------------------------
std::string removeAdjacentDuplicates(const std::string& s) {
    std::string stack;
    stack.reserve(s.size());     // 預先配足,避免迴圈中 realloc
    for (char c : s) {
        if (!stack.empty() && stack.back() == c) {
            stack.pop_back();
        } else {
            stack.push_back(c);
        }
    }
    // 結果可能遠短於輸入,把多餘 buffer 歸還
    stack.shrink_to_fit();
    return stack;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 Daily Work】長期常駐物件的記憶體優化
// 為何用 shrink_to_fit: 後端服務有「大量短字串長期持有」的場景(快取的 key、
//                       config map 的 value)。如果這些字串曾被 += 過大內容
//                       後又縮短,capacity 會卡在大值,占用幾 MB ~ 幾百 MB。
//                       存入容器前 shrink_to_fit 一次,長期省記憶體。
// -----------------------------------------------------------------------------
#include <unordered_map>
void compactCacheKeys(std::unordered_map<std::string, std::string>& cache) {
    for (auto& [k, v] : cache) {
        // key 是 const,但既然 cache 整個在我們手上、目前不會被讀取,合法地 cast
        // (注意這在多執行緒環境是危險的,只示範單執行緒 compact 階段)
        const_cast<std::string&>(k).shrink_to_fit();
        v.shrink_to_fit();
    }
}

// -----------------------------------------------------------------------------
// 【LeetCode 範例 2】LeetCode 38. Count and Say
// 題目: countAndSay 數列產生器。
// 為何用 shrink_to_fit: 中間構建的字串可能 capacity 很大,結束前 shrink_to_fit
//                       可降低長期記憶體占用 (適合保留作 cache key)。
// 複雜度: O(N * L)。
// 難度: medium
// -----------------------------------------------------------------------------
std::string countAndSayShrink(int n) {
    std::string s = "1";
    for (int i = 1; i < n; ++i) {
        std::string next;
        next.reserve(s.size() * 2);
        size_t j = 0;
        while (j < s.size()) {
            size_t k = j;
            while (k < s.size() && s[k] == s[j]) ++k;
            next += std::to_string(k - j);
            next += s[j];
            j = k;
        }
        s = std::move(next);
    }
    s.shrink_to_fit();
    return s;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】拼接大量文字後一次性縮減 (例如組 HTML 結束時)
// 為何用 shrink_to_fit: HTML page builder 用 += 大量拼字串,完成後送網路前縮容量。
// -----------------------------------------------------------------------------
std::string buildAndCompactPage(const std::vector<std::string>& sections) {
    std::string html;
    html.reserve(4096);
    html += "<html><body>";
    for (auto& s : sections) {
        html += "<p>";
        html += s;
        html += "</p>";
    }
    html += "</body></html>";
    html.shrink_to_fit();      // 送出前縮容,長期持有更省記憶體
    return html;
}

int main() {
    demoShrink();

    std::cout << "\n=== 大字串去空白後縮減容量 ===\n";
    std::string big(10000, ' ');
    big += "useful";
    auto out = processLargeText(std::move(big));
    std::cout << "result size=" << out.size()
              << ", capacity=" << out.capacity() << "\n";

    std::cout << "\n=== 日常實務: 縮減 cache 條目 (示範呼叫) ===\n";
    std::unordered_map<std::string, std::string> cache{{"k1", "v1"}, {"k2", "v2"}};
    compactCacheKeys(cache);
    std::cout << "compact done, entries=" << cache.size() << "\n";

    std::cout << "\n=== LeetCode 1047 (處理後 shrink_to_fit) ===\n";
    std::cout << "\"abbaca\" → \"" << removeAdjacentDuplicates("abbaca") << "\"\n"; // "ca"
    std::cout << "\"azxxzy\" → \"" << removeAdjacentDuplicates("azxxzy") << "\"\n"; // "ay"

    std::cout << "\n=== LeetCode 38 ===\n";
    for (int i = 1; i <= 5; ++i) std::cout << countAndSayShrink(i) << "\n";

    std::cout << "\n=== 日常實務: buildAndCompactPage ===\n";
    std::cout << buildAndCompactPage({"hello", "world"}) << "\n";

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1:為什麼標準把 shrink_to_fit 設計成 non-binding 而不是強制縮?
    //    A:給實作彈性。某些 allocator 是 page-based,縮一半其實不會釋放
    //      實體記憶體;某些實作可能延後執行 (lazy)。寫 assert 不要依賴
    //      capacity()==size()。實務上 libstdc++/libc++/MSVC 都會真的縮,
    //      除非 size 已落在 SSO 範圍內。
    //
    //  Q2:shrink_to_fit 與 C++03 swap idiom 「string(s).swap(s)」差別?
    //    A:語意相同,效果相同 (都靠拷貝建構出一個 capacity 剛好的副本再
    //      swap 進來,O(N))。C++11 起優先用 shrink_to_fit,意圖明確、
    //      省去暫時物件名稱、可被優化器辨識。同樣是 non-binding。
    //
    //  Q3:對 SSO 範圍內 (例如 size()==5) 的字串呼叫 shrink_to_fit 有用嗎?
    //    A:若該字串本來就是 SSO (用 inline buffer),沒有 heap 可釋放,
    //      呼叫無作用。若它曾被撐大成 heap buffer 後又縮短,shrink_to_fit
    //      會把資料搬回 SSO inline buffer 並釋放 heap — 此時最有價值。
    //
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra shrink_to_fit.cpp -o shrink_to_fit

// === 預期輸出 (節錄) ===
// === LeetCode 38 ===
// 1
// 11
// 21
// 1211
// 111221
// === 日常實務: buildAndCompactPage ===
// <html><body><p>hello</p><p>world</p></body></html>
