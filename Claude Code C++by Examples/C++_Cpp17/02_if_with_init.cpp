// =============================================================================
//  02_if_with_init.cpp  —  if 帶 init statement (C++17)
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/language/if
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、語法                                                   │
//  └────────────────────────────────────────────────────────────┘
//
//      if (init_statement; condition) { ... }
//      else { ... }
//
//  init 跟 for-loop 的 init 一樣 — 在 if scope 內宣告變數，只在 if/else
//  區塊看得到。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、為什麼好？                                             │
//  └────────────────────────────────────────────────────────────┘
//
//  傳統寫法：
//
//      auto it = m.find(key);
//      if (it != m.end()) {
//          /* 用 it */
//      }
//      // ⚠️ it 還活到這 — 容易跨 scope 誤用
//
//  C++17：
//
//      if (auto it = m.find(key); it != m.end()) {
//          /* 用 it */
//      }
//      // ✅ it 出 scope 即死，不污染外層
//
//  好處：
//   * scope 收緊 — 變數只在「真的會用」的地方存在
//   * 變數「拿到 + 條件判斷」一行解
//   * 跟 STL find / insert 等回傳 (iterator, bool) 配合極簡潔
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、本檔示範                                               │
//  └────────────────────────────────────────────────────────────┘
//
//   * Demo 1：std::map::find
//   * Demo 2：std::map::insert 跟 structured binding 結合
//   * LeetCode 1. Two Sum — 用 hash 表 + if-with-init
// =============================================================================

/*
補充筆記：if_with_init
  - if_with_init 是現代 C++ 語法或標準庫特性；學習時要把「少寫字」和「語意更精確」分開看。
  - auto 讓型別由初始化式推導，但會丟掉 top-level const/reference；需要保留引用語意時要寫 auto&、const auto& 或 decltype(auto)。
  - brace initialization 能減少未初始化與 narrowing，但遇到 initializer_list overload 可能選到不同建構子。
  - constexpr、static_assert、if constexpr 把部分錯誤和計算提前到編譯期，能讓 template 和常數邏輯更清楚。
  - 屬性如 [[nodiscard]]、[[maybe_unused]]、[[fallthrough]] 是對編譯器和讀者的意圖標記，不應拿來掩蓋設計問題。
  - string_view、optional、variant、structured binding 等特性改善介面表達力，但也帶來生命週期或狀態檢查責任。
  - if init-statement 可把暫時變數限制在 if/else 範圍內，減少外層命名污染。
  - 常見寫法是 if (auto it = m.find(k); it != m.end())，it 只在判斷分支中可見。
*/
#include <iostream>
#include <map>
#include <unordered_map>
#include <vector>

int main() {
    // ─────────────────────────────────────────────────────────
    // Demo 1：std::map::find — scope 收緊
    // ─────────────────────────────────────────────────────────
    std::map<std::string, int> m{{"alice", 30}, {"bob", 25}};

    if (auto it = m.find("alice"); it != m.end()) {
        std::cout << "[Demo1] found alice, age=" << it->second << '\n';
    }
    // it 已不存在

    if (auto it = m.find("zoe"); it != m.end()) {
        std::cout << "[Demo1] found zoe\n";
    } else {
        std::cout << "[Demo1] zoe not in map (else block)\n";
    }

    // ─────────────────────────────────────────────────────────
    // Demo 2：insert + structured binding
    //   insert 回傳 pair<iterator, bool>；用 init + binding 一行解
    // ─────────────────────────────────────────────────────────
    if (auto [it, inserted] = m.insert({"charlie", 40}); inserted) {
        std::cout << "[Demo2] inserted charlie -> " << it->second << '\n';
    } else {
        std::cout << "[Demo2] charlie already exists, age=" << it->second << '\n';
    }

    // 第二次 insert，應該 inserted = false
    if (auto [it, inserted] = m.insert({"charlie", 99}); !inserted) {
        std::cout << "[Demo2] charlie was already there with age="
                  << it->second << '\n';
    }

    // ─────────────────────────────────────────────────────────
    // LeetCode 1. Two Sum
    //   題意：給 nums 與 target，找兩個 index i, j 使 nums[i]+nums[j]=target
    //   用 hash 表記「值 → index」，邊掃邊查 (target - x) 是否已見過
    //   if-with-init 讓「查 + 判斷」乾淨
    // ─────────────────────────────────────────────────────────
    auto twoSum = [](const std::vector<int>& nums, int target) {
        std::unordered_map<int, int> seen;     // value -> index
        for (int i = 0; i < static_cast<int>(nums.size()); ++i) {
            int need = target - nums[i];
            if (auto it = seen.find(need); it != seen.end()) {
                return std::vector<int>{it->second, i};
            }
            seen[nums[i]] = i;
        }
        return std::vector<int>{};
    };
    auto ans = twoSum({2, 7, 11, 15}, 9);
    std::cout << "[LC1] indices:";
    for (int x : ans) std::cout << ' ' << x;
    std::cout << '\n';

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：init 內宣告多個變數可以嗎？
    //    A：可以 — 跟 for 一樣：
    //         if (int a = 1, b = 2; a + b == 3) ...
    //
    //  Q2：跟 if constexpr 結合？
    //    A：可以同時用：
    //         if constexpr (auto x = compute(); std::is_integral_v<decltype(x)>)
    //
    //  Q3：if 帶 init 跟 for 帶 init 規則一樣嗎？
    //    A：精神一樣（init scope 限縮）。語法上 if/switch 都 C++17 加的；
    //       for/while 早就有 init scope。
    //
    // ─────────────────────────────────────────────────────────
    // 實用範例 1：mutex try_lock — 「拿到鎖才執行」的乾淨寫法
    //   工作上常見：避免阻塞、嘗試取鎖失敗就跳過
    // ─────────────────────────────────────────────────────────
    // 簡化的 mock mutex（避免真的引 <mutex>）
    struct FakeMutex {
        bool locked = false;
        bool try_lock() { if (locked) return false; locked = true; return true; }
        void unlock() { locked = false; }
    };
    FakeMutex mtx;
    if (bool got = mtx.try_lock(); got) {
        std::cout << "[Demo3] acquired lock, doing work\n";
        mtx.unlock();
    } else {
        std::cout << "[Demo3] lock busy, skip\n";
    }

    // ─────────────────────────────────────────────────────────
    // 實用範例 2：「字串解析 + 邊界檢查」一行解
    //   工作上常見：解析使用者輸入的整數，邊界錯誤要立刻處理
    // ─────────────────────────────────────────────────────────
    auto parsePort = [](const std::string& s) {
        try {
            if (int p = std::stoi(s); p > 0 && p < 65536) {
                std::cout << "[Demo4] valid port = " << p << '\n';
            } else {
                std::cout << "[Demo4] port " << p << " out of range\n";
            }
        } catch (...) {
            std::cout << "[Demo4] not a number: " << s << '\n';
        }
    };
    parsePort("8080");
    parsePort("99999");
    parsePort("abc");

    return 0;
}
