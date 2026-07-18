// ============================================================
// std::partition_copy   (C++11 起)
// 分類 (Category): Partitioning operations (分割演算法)
// 標頭檔 (Header):  <algorithm>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/partition_copy
//   * https://cplusplus.com/reference/algorithm/partition_copy/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 (Topic Introduction)                          │
// └────────────────────────────────────────────────────────────┘
//
// std::partition_copy 解的問題:
//
//   「給我一段資料和述詞 p。把符合的拷貝到一個輸出端,
//    把不符合的拷貝到另一個輸出端。原資料不動。」
//
// 直觀來看就是「過濾 + 分流」一次完成 — 不必掃兩次。
//
// 想像情境:
//   * 一份訂單 → 已付款組 / 未付款組
//   * 一份學生成績 → 通過 / 不通過
//   * 一份事件 log → 錯誤 / 正常
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、為什麼比 copy_if 兩次更好?                            │
// └────────────────────────────────────────────────────────────┘
//
// 想分流成兩組,自然會想到呼叫兩次 copy_if:
//
//   std::copy_if(src, dest_true,  p);
//   std::copy_if(src, dest_false, [&](auto x){ return !p(x); });
//
// 但這要掃兩次資料,做兩次述詞呼叫。
// partition_copy 一次掃描就完成 — 對大資料量更省。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、partition_copy vs partition                            │
// └────────────────────────────────────────────────────────────┘
//
//   * partition       : 原地重排,改變原資料,得到「分組好的單一 vector」
//   * partition_copy  : 不改原資料,把兩組分別寫到兩個目的端
//
// 想保留原資料、想得到「兩組分開的容器」 → 用 partition_copy。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、函式簽章 (Signatures)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   template <class InputIt, class OutTrueIt, class OutFalseIt, class UnaryPred>
//   std::pair<OutTrueIt, OutFalseIt>
//   partition_copy(InputIt first, InputIt last,
//                  OutTrueIt  d_first_true,
//                  OutFalseIt d_first_false,
//                  UnaryPred p);
//
//   * C++20 起為 constexpr。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、回傳值 (Return value)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   pair { 真分組寫入結束位置, 假分組寫入結束位置 }
//
//   兩個結尾迭代器分別告訴你「真組寫了幾個」「假組寫了幾個」。
//   兩者相加應等於原始範圍長度。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 六、複雜度 (Complexity)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   時間: 恰好 N 次述詞呼叫 + N 次指派 — O(N)
//   空間: 兩個輸出範圍各占一部分,總共 O(N)
//
// ┌────────────────────────────────────────────────────────────┐
// │ 七、注意事項 (Pitfalls)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   1. 兩個目的端不可重疊,且不可與來源範圍重疊。
//   2. 適合配 back_inserter 動態擴充輸出容器。
//   3. 與 copy_if 比較 — copy_if 只拿「滿足者」;
//      partition_copy 兩組都拿。
//
// ============================================================

/*
補充筆記：std::partition_copy
  - std::partition_copy 依 predicate 把資料分成 true 區和 false 區；partition 後同一區內相對順序不一定保留。
  - stable_partition 會保留相對順序，但通常需要更多移動或額外空間；是否穩定要依需求決定。
  - partition_point 只能用在已 partitioned 的範圍；若資料沒有先分區，結果不可靠。
  - predicate 應對同一元素回傳穩定結果；若 predicate 依外部可變狀態改變，分區結果很難推理。
  - partition_copy 需要兩個輸出範圍，分別接 true 和 false 元素；目的地容量仍由呼叫者負責。
  - 分區不是排序；它只保證條件分界，不保證每一區內元素大小順序。
  - std::partition_copy 的 predicate 就是分界線定義；先決定哪些元素應放在 true 區，再看函式是否保留相對順序。
  - std::partition_copy 完成後資料通常只保證被分成兩段，不保證每段內已排序；把 partition 當 sort 使用會得到錯誤假設。
  - std::partition_copy 若回傳 iterator，它通常代表 true 區結尾或第一個 false 位置；使用前要把這個位置當成半開區間邊界理解。
*/
#include <algorithm>
#include <iostream>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

// ============================================================
//                          基本範例
// ============================================================
int main() {
    std::vector<int> v{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    std::vector<int> evens, odds;

    // --- 範例 1: 一次分流偶/奇 ---
    auto [te, to] = std::partition_copy(
        v.begin(), v.end(),
        std::back_inserter(evens),
        std::back_inserter(odds),
        [](int x){ return x % 2 == 0; });
    (void)te; (void)to;

    std::cout << "evens: ";
    for (int x : evens) std::cout << x << ' ';
    std::cout << '\n';
    std::cout << "odds:  ";
    for (int x : odds) std::cout << x << ' ';
    std::cout << '\n';
    std::cout << "src unchanged: ";
    for (int x : v) std::cout << x << ' ';
    std::cout << '\n';

    // --- 範例 2: 分離正負數 ---
    std::vector<int> nums{3, -1, 4, -1, 5, -9, 2, -6};
    std::vector<int> pos, neg;
    std::partition_copy(nums.begin(), nums.end(),
                        std::back_inserter(pos),
                        std::back_inserter(neg),
                        [](int x){ return x >= 0; });
    std::cout << "positive: ";
    for (int x : pos) std::cout << x << ' ';
    std::cout << '\n';
    std::cout << "negative: ";
    for (int x : neg) std::cout << x << ' ';
    std::cout << '\n';

    // === LeetCode / 實務範例 ===
    void practical_split_orders_by_payment();
    void practical_split_pass_fail();
    void leetcode_2overlap_split_by_threshold();
    void practical_send_emails_split_by_domain();
    practical_split_orders_by_payment();
    practical_split_pass_fail();
    leetcode_2overlap_split_by_threshold();
    practical_send_emails_split_by_domain();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================
//
// partition_copy 在 LeetCode 上題目少見,但「過濾分流」是
// 後端開發每天都做的事 — 給兩個直接的實務範例。

// ----------------------------------------------------------------
// 實務範例 1:訂單分成「已付款 / 未付款」
// ----------------------------------------------------------------
// 場景:後台收到訂單清單,要分成兩組:
//      已付款 → 進入出貨流程
//      未付款 → 進入催款流程
//      同時保留原訂單列表以供稽核。
//
// 為什麼用 std::partition_copy:
//   * 一次掃描得到兩組 (比 copy_if 兩次省一半時間)。
//   * 原資料不動,符合稽核需求。
void practical_split_orders_by_payment() {
    struct Order { int id; bool paid; };
    std::vector<Order> orders{
        {101, true}, {102, false}, {103, true}, {104, false}, {105, true}
    };
    std::vector<Order> paid, unpaid;
    std::partition_copy(orders.begin(), orders.end(),
                        std::back_inserter(paid),
                        std::back_inserter(unpaid),
                        [](const Order& o){ return o.paid; });
    std::cout << "Practical paid ids: ";
    for (auto& o : paid) std::cout << o.id << ' ';
    std::cout << "| unpaid ids: ";
    for (auto& o : unpaid) std::cout << o.id << ' ';
    std::cout << '\n';
}

// ----------------------------------------------------------------
// 實務範例 2:學生成績分成「通過 / 未通過」
// ----------------------------------------------------------------
// 場景:課程結算,>= 60 分為通過,需要分別寄送通過/補考通知。
//      原成績清單要保留 (做存檔)。
void practical_split_pass_fail() {
    std::vector<int> scores{55, 90, 60, 45, 73, 88, 30};
    std::vector<int> pass, fail;
    std::partition_copy(scores.begin(), scores.end(),
                        std::back_inserter(pass),
                        std::back_inserter(fail),
                        [](int s){ return s >= 60; });
    std::cout << "Practical pass: ";
    for (int s : pass) std::cout << s << ' ';
    std::cout << "| fail: ";
    for (int s : fail) std::cout << s << ' ';
    std::cout << '\n';
}

// ----------------------------------------------------------------
// LeetCode 概念題:依閾值切分陣列為「合格 / 不合格」
// ----------------------------------------------------------------
// 題目:給陣列 nums 與閾值 T,輸出兩個陣列:>= T 與 < T。
//
// 為什麼用 std::partition_copy:
//   一次掃描完成「分流」,程式碼比兩次 copy_if 簡潔。
//
// 複雜度:時間 O(n);空間 O(n)。
void leetcode_2overlap_split_by_threshold() {
    std::vector<int> nums{10, 5, 20, 8, 15, 3};
    int T = 10;
    std::vector<int> hi, lo;
    std::partition_copy(nums.begin(), nums.end(),
                        std::back_inserter(hi),
                        std::back_inserter(lo),
                        [T](int x){ return x >= T; });
    std::cout << "hi:";
    for (int x : hi) std::cout << ' ' << x;
    std::cout << " | lo:";
    for (int x : lo) std::cout << ' ' << x;
    std::cout << '\n';
}

// ----------------------------------------------------------------
// 實務範例:電子報寄送 — 依網域分流到不同 SMTP 服務
// ----------------------------------------------------------------
// 場景:寄發電子報時,@gmail.com 帳號透過 Gmail 介面送、其他用一般 SMTP。
//      用 partition_copy 把收件人清單分流,原清單保留供稽核。
void practical_send_emails_split_by_domain() {
    std::vector<std::string> emails{
        "a@gmail.com", "b@yahoo.com", "c@gmail.com", "d@hotmail.com", "e@gmail.com"
    };
    std::vector<std::string> gmail, others;
    std::partition_copy(emails.begin(), emails.end(),
                        std::back_inserter(gmail),
                        std::back_inserter(others),
                        [](const std::string& s){
                            return s.find("@gmail.com") != std::string::npos;
                        });
    std::cout << "gmail:";
    for (auto& e : gmail) std::cout << ' ' << e;
    std::cout << " | other:";
    for (auto& e : others) std::cout << ' ' << e;
    std::cout << '\n';
}

// === 預期輸出 (Expected output) ===
// evens: 2 4 6 8 10
// odds:  1 3 5 7 9
// src unchanged: 1 2 3 4 5 6 7 8 9 10
// positive: 3 4 5 2
// negative: -1 -1 -9 -6
// Practical paid ids: 101 103 105 | unpaid ids: 102 104
// Practical pass: 90 60 73 88 | fail: 55 45 30
// hi: 10 20 15 | lo: 5 8 3
// gmail: a@gmail.com c@gmail.com e@gmail.com | other: b@yahoo.com d@hotmail.com
