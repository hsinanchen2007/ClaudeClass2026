# Codex C++ 教科書式教材規格與章節模板

> **用途：這是寫給未來 Codex session 的執行規格。**
>
> 當使用者要求「照上次格式新增／更新 C++ by Examples」「做成教科書」「補 LeetCode、
> 實務與面試題」時，直接依本檔工作，不要再要求使用者重述需求，也不要把 405 個既有
> 檔案全部重做。先讀同目錄 `AGENTS.md` 的最近驗證基線，再只處理基線後新增或變動的檔案。
>
> 本規格只適用 `Codex C++by Examples/`。Claude 軌只提供相對路徑與題目映射，不得複製、
> 改寫或「修正」其教材內容。

---

## 1. 最終目標

每個 `.cpp` 必須同時是：

1. **離線教科書章節**：讀者假設沒有其他書籍、老師或網路，仍能從零理解本主題。
2. **可執行實驗**：所有主要說法都有短小程式、檢查與可觀察結果，不只停在文字。
3. **題目應用示範**：至少有一個真正實作並測試的短 LeetCode 案例。
4. **工程應用示範**：至少有一個真正實作並測試的工作情境，不用空泛的 `foo/bar`。
5. **面試複習材料**：有正確答案、常見誤答、追問及工程取捨。
6. **可重現產物**：檔尾有可直接複製的編譯／執行指令，以及由實際執行取得的輸出。

「程式能編譯」只是最低門檻。沒有完整概念模型、案例為何選這個 API、失敗模式及取捨，
就不算教科書式教材。

---

## 2. 動手前的增量流程

### 2.1 不要每次全庫重做

1. 先讀 `AGENTS.md` 的「最近完整驗證基線」。
2. 使用其中的 baseline commit 執行 path-scoped `git diff`。
3. 只列出 baseline 之後新增或實質變動的 Codex 檔案。
4. 若 Claude 軌新增／刪除題目，才用 `tools/audit_textbook.sh` 檢查 1:1 路徑差異。
5. `AGENTS.md`、本模板或 README 的純記憶更新，不代表 405 份教材都要重驗。

### 2.2 先回答六個設計問題

在寫程式前，先在草稿中確認：

| 問題 | 必須得到的答案 |
|---|---|
| 這個主題解決什麼問題？ | 不能只回答「這是某個 API」 |
| 最低 C++ 標準？ | C++11／14／17 章依章節；其餘預設 C++20 |
| 核心契約？ | 前置條件、後置條件、ownership、lifetime、iterator/reference 規則 |
| 效能模型？ | 時間、空間、allocation、cache、同步或 I/O 成本 |
| 最常見錯誤？ | 編譯錯誤、例外、未指定值、實作定義或 UB 必須分清 |
| 如何證明範例正確？ | release 仍存在的檢查、固定輸入與實際輸出 |

---

## 3. 一般 `.cpp` 的固定閱讀順序

每個檔案依下列順序組織。說明必須放在相關函式或案例的**正前方**，不能先在檔尾只寫
「LeetCode 2558」一句，再讓讀者自行猜前面的程式與它有何關係。

### 3.1 檔頭：本課地圖

檔頭區塊至少包含：

- 主題名稱與一句話目的。
- 先備知識與最低 C++ 標準。
- 本檔會學到什麼、最後能完成什麼工作。
- 核心 API／語法速查：標頭、函式簽名、回傳值及複雜度。
- 何時應使用、何時不應使用、可替代方案。
- ownership、lifetime、iterator/reference invalidation、例外安全與 thread safety。
- 標準保證、實作定義、未指定行為與 UB 的界線。

建議格式：

```cpp
/*
 * 主題：<完整名稱>
 *
 * 一、為什麼需要它
 * 二、基本語法與心智模型
 * 三、API／複雜度／生命週期契約
 * 四、選擇準則與替代方案
 * 五、常見錯誤與如何驗證
 */
```

### 3.2 基礎範例

先用最小、可執行的例子說明語法，再進入題目。每個重要步驟要說明「為何這樣寫」，
而不是逐字翻譯程式碼。

```cpp
// -----------------------------------------------------------------------------
// 【基礎用法】<此例要證明的規則>
// 情境：...
// 步驟：1. ... 2. ... 3. ...
// 觀察：輸出或檢查如何證明規則成立。
// 易錯點：...
// -----------------------------------------------------------------------------
```

### 3.3 LeetCode 案例

每個一般 `.cpp` 至少實作並測試一個短案例。語言機制主題若不是 LeetCode 的直接考點，
也要展示該機制如何支撐一個解法，例如 RAII 管理解題暫存資源、lambda 作 comparator、
move semantics 安全傳回結果；不能只寫「此主題沒有 LeetCode」。

每個案例必須在實作前完整標示：

```cpp
// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode <題號>. <官方英文題名>（繁中短名）
// 題目：以自己的話簡述輸入、輸出與一個例子，不長篇複製原題。
// 為何使用本章主題：指出 API／觀念與解法的直接關係。
// 思路：按 1、2、3 步驟說明資料如何流動，以及 invariant 是什麼。
// 複雜度：時間 O(...)、額外空間 O(...)，並定義 N／M／K。
// 易錯點：空輸入、重複值、溢位、end iterator、失效或 ownership 等。
// 工程補充：本教學解法若不是大資料最佳方案，必須指出更合適的替代方案。
// -----------------------------------------------------------------------------
```

硬性要求：

- 題號、英文題名與問題必須真的存在且吻合。
- 必須有真正的解法函式，不得只放題號或 pseudo-code。
- `main()` 必須以正常與邊界案例呼叫並驗證。
- 不把測試只藏在 `assert(function())`；`-DNDEBUG` 後必要呼叫與檢查仍須存在。
- 複雜度必須描述目前實作，不可貼理想演算法的複雜度。

### 3.4 實際工作案例

案例要對應真實需求，例如：

- 依 ID 查服務、訂單或設定項。
- 解析 log／HTTP header／設定檔。
- 批次資料去重、排序、合併與索引。
- RAII 包裝 file descriptor、lock、CUDA／資料庫資源。
- 佇列、cache、snapshot、timeout、重試或 graceful shutdown。

每個案例在程式前標示：

```cpp
// -----------------------------------------------------------------------------
// 【日常實務範例】<真實情境名稱>
// 問題：production 中要解決什麼，輸入可能有什麼髒資料或失敗。
// 為何使用本章主題：與替代方案比較，而不是只說「可以用」。
// 設計：資料結構、ownership、錯誤模型與步驟。
// 成本：時間／空間／allocation／鎖競爭或 I/O。
// 上線注意：邊界、thread safety、可觀測性、安全性與測試策略。
// -----------------------------------------------------------------------------
```

實務函式也必須在 `main()` 執行並驗證；「假裝是工作情境但只印 Hello」不合格。

### 3.5 面試問題與解答

一般檔至少涵蓋核心定義、陷阱與設計取捨；題數不是目標，答案完整才是。

```cpp
/*
==============================================================================
【本課面試問答】

Q1｜<30 秒核心題>
答：先直接下正確結論，再補必要契約。
追問：<面試官常接著問什麼？>
答：<區分標準保證與常見實作，並給反例或替代方案。>

Q2｜<生命週期／複雜度／例外安全題>
答：...
常見誤答：...

Q3｜<production 設計題>
答：說明何時不用本 API、如何測試、如何觀測失敗。
==============================================================================
*/
```

好答案至少包含適用時的其中數項：前置條件、所有權、生命週期、失效規則、複雜度、
例外安全、thread safety、替代方案與驗證。不要只背一行定義。

### 3.6 `main()` 是驗證入口

`main()` 必須：

1. 分段執行基礎、LeetCode 與實務案例。
2. 有正常、邊界及失敗路徑的確定性檢查。
3. release build 移除 `assert` 後仍會執行必要操作並判斷結果。
4. 輸出簡短、穩定且與檔尾預期輸出一致。
5. 不依賴網路、互動輸入或目前工作目錄中既有檔案。

可用簡單 helper 取代把重要驗證塞入 `assert`：

```cpp
void require(bool condition, std::string_view message) {
    if (!condition) {
        throw std::runtime_error(std::string(message));
    }
}
```

---

## 4. `summary.cpp` 的額外要求

`summary.cpp` 是面試前能單獨閱讀的完整章，不是檔名索引。它必須：

1. 涵蓋同目錄所有課檔的核心觀念，不漏主題。
2. 先給選擇表：需求、建議 API／型別、前置條件、複雜度與禁忌。
3. 統整共同契約：ownership、lifetime、失效、例外、並行及 UB。
4. 提供可推導的規則或 invariant，而非只列結論。
5. 收錄高頻深挖問答、常見誤答與追問。
6. 至少有一個整合式 LeetCode 案例和一個整合式工作案例，兩者都可執行與驗證。
7. 有考前速查、測試清單、何時選替代方案，以及延伸練習。
8. 檔尾同樣提供實際編譯命令與實跑輸出。

若讀者必須打開其他 10 個檔案才能理解 summary，summary 就尚未完成。

---

## 5. 正確性與安全規範

### 5.1 C++ 語意

- C++11、C++14、C++17 章使用對應標準；其他章預設 C++20。
- 不把 GCC／libstdc++／x86-64 的現象寫成 C++ 標準保證。
- UB 只能在註解或明確 opt-in 路徑說明，預設執行不可觸發。
- moved-from object 只能依「valid but unspecified」等標準契約使用。
- comparator 必須符合 strict weak ordering；range algorithm 的前置條件要明講。
- 任何 pointer、reference、iterator 或 view 都要交代 owner 與有效期限。
- `assert` 只適合 internal invariant；不可讓 release build 因副作用消失而變成另一支程式。
- 外部輸入、索引、空範圍、轉型、算術溢位與配置失敗要有正常檢查。

### 5.2 可重現性

- random 範例用固定 seed，並註明 production randomness 的需求不同。
- thread 排程、位址、耗時等非決定輸出不要硬寫固定值；改用 invariant／計數，或清楚標示節錄。
- benchmark 必須使用 `steady_clock`，含暖機、多次量測與清楚計時邊界。
- 範例若建立檔案，測試要在獨立暫存目錄執行並清理。

### 5.3 禁止污染 source tree

- executable 一律寫 `/tmp` 或工具建立的 `mktemp -d`。
- 不在 `.cpp` 旁產生無副檔名 binary。
- 不提交 ELF、core dump、log、測試資料或 sanitizer 產物。
- commit 必須使用 Codex pathspec，不可 `git add -A`。

---

## 6. 每檔最後的編譯與輸出區塊

這個區塊必須位於檔案最末端。命令要與該檔實際標準、library 及 `-pthread` 需求一致，
輸出名稱要在 `/tmp`，避免覆寫來源或污染 repo。

```cpp
/*
================================================================================
編譯與執行（請先 cd 到本檔所在目錄）：
    g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror \
        -pthread '<檔名>.cpp' -o '/tmp/codex_cpp_<唯一名稱>' && \
        '/tmp/codex_cpp_<唯一名稱>'

預期輸出（由上列 executable 實際執行取得）：
    === 基礎用法 ===
    ...
    === LeetCode <題號> ===
    ...
    === 日常實務 ===
    ...
================================================================================
*/
```

輸出規則：

- 不可憑閱讀程式手寫，必須實際執行後貼入。
- 至少再執行一次確認穩定。
- 非決定值用 `<位址>` 等描述會掩蓋真值時，優先改程式輸出可驗 invariant。
- 若只能提供節錄，明寫「節錄」與哪些欄位可能改變。
- command 中的引號要能處理路徑空白。

---

## 7. 完整格式示範：`std::find`／`std::find_if`

下面是一份**可直接編譯**的縮小示範，用來展示段落位置與講解深度。真正教材要再依主題
補足檔頭原理、陷阱及面試內容，不可機械複製文字。

```cpp
#include <algorithm>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

void require(bool condition, std::string_view message) {
    if (!condition) {
        throw std::runtime_error(std::string(message));
    }
}

// -----------------------------------------------------------------------------
// 【基礎用法】以 find 找值
// find 回傳第一個相等元素的 iterator；找不到時回傳 last，而不是 null。
// 對 vector 是線性搜尋，時間 O(N)、額外空間 O(1)。解參考前必須先與 end() 比較。
// -----------------------------------------------------------------------------
std::size_t find_index(const std::vector<int>& values, int wanted) {
    const auto it = std::find(values.begin(), values.end(), wanted);
    if (it == values.end()) {
        return values.size();
    }
    return static_cast<std::size_t>(std::distance(values.begin(), it));
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 349. Intersection of Two Arrays（兩陣列交集）
// 題目：回傳兩個整數陣列共有的值，每個值只能出現一次。
// 為何使用本章主題：一次 find 判斷右側是否含該值，另一次 find 防止重複加入答案。
// 思路：依序掃左側；同時存在右側且尚未出現在答案時才 push_back。
// 複雜度：目前教學版時間 O(N*M + N*K)、額外空間 O(K)。大量資料應改 unordered_set，
// 平均可降至 O(N+M)，所以不能把本例誤稱為最佳解。
// 易錯點：交集要求唯一值；不能直接把每次命中都加入。
// -----------------------------------------------------------------------------
std::vector<int> intersection(const std::vector<int>& left,
                              const std::vector<int>& right) {
    std::vector<int> result;
    for (const int value : left) {
        const bool in_right =
            std::find(right.begin(), right.end(), value) != right.end();
        const bool already_added =
            std::find(result.begin(), result.end(), value) != result.end();
        if (in_right && !already_added) {
            result.push_back(value);
        }
    }
    return result;
}

struct Service {
    std::string id;
    bool healthy;
};

// -----------------------------------------------------------------------------
// 【日常實務範例】依 service ID 查健康狀態
// 問題：小型設定快照要依字串 ID 找服務，找不到與 unhealthy 必須是不同狀態。
// 為何使用本章主題：資料量小且保持 insertion order，find_if 比另建 hash index 簡單。
// 設計：回傳 const_iterator；呼叫端先比 end()，且不得在使用 iterator 時修改 vector。
// 成本：每次 O(N)。若查詢頻繁或資料很大，應改 unordered_map 並定義更新策略。
// 上線注意：vector reallocation 會使 iterator 失效；跨 thread 發布應使用 immutable snapshot。
// -----------------------------------------------------------------------------
std::vector<Service>::const_iterator find_service(
    const std::vector<Service>& services, std::string_view id) {
    return std::find_if(services.begin(), services.end(),
                        [id](const Service& service) { return service.id == id; });
}

int main() {
    std::cout << "=== 基礎用法 ===\n";
    const std::vector<int> values{10, 20, 30, 40};
    const std::size_t index = find_index(values, 30);
    require(index == 2U, "find_index 應找到索引 2");
    require(find_index(values, 99) == values.size(), "找不到時應回 size");
    std::cout << "30 的索引：" << index << '\n';

    std::cout << "=== LeetCode 349 ===\n";
    const std::vector<int> common = intersection({4, 9, 5, 4}, {9, 4, 9, 8, 4});
    require(common == std::vector<int>({4, 9}), "交集應唯一且保持左側發現順序");
    std::cout << "交集：";
    for (const int value : common) {
        std::cout << ' ' << value;
    }
    std::cout << '\n';

    std::cout << "=== 日常實務 ===\n";
    const std::vector<Service> services{{"api", false}, {"worker", true}};
    const auto service = find_service(services, "api");
    require(service != services.end(), "api service 必須存在");
    std::cout << service->id << "：" << (service->healthy ? "healthy" : "degraded")
              << '\n';
}

/*
==============================================================================
【本課面試問答】

Q1｜`std::find` 找不到時回什麼？
答：回傳呼叫者提供的 `last` iterator。解參考前必須先比較 `it != last`；它不是 null。
追問：在 `vector` 上取得 iterator 後插入元素有何風險？
答：若發生 reallocation，所有 iterator/reference 都失效；即使沒有 reallocation，插入點
之後的 iterator 也會失效，因此不能把舊 iterator 無條件保留。

Q2｜何時不該用線性 `find`？
答：單次查詢或小型連續資料通常很合適；大量重複 membership query 可考慮先建
`unordered_set/map`，需要排序／範圍查詢則考慮 sorted vector 或 ordered container。
選擇時要把建立索引、更新頻率、記憶體與 cache locality 一起計算。

Q3｜`find_if` 的 predicate 可以修改容器嗎？
答：不可讓 predicate 使目前 range 的 iterator 失效，也不應依賴每次呼叫會變動的狀態。
若需要修改命中元素，先取得 iterator，再在演算法完成後依容器規則修改。
==============================================================================

編譯與執行（請先 cd 到本檔所在目錄）：
    g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror \
        -pthread 'find_example.cpp' -o '/tmp/codex_cpp_find_example' && \
        '/tmp/codex_cpp_find_example'

預期輸出（由上列 executable 實際執行取得）：
    === 基礎用法 ===
    30 的索引：2
    === LeetCode 349 ===
    交集： 4 9
    === 日常實務 ===
    api：degraded
==============================================================================
*/
```

---

## 8. 單檔驗證矩陣

對新增或變更檔至少執行：

```bash
# Debug + 嚴格警告
g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror \
    -pthread source.cpp -o /tmp/codex_cpp_debug
/tmp/codex_cpp_debug

# Release：確認 -DNDEBUG 後必要行為沒有消失
g++ -std=c++20 -O2 -DNDEBUG -Wall -Wextra -Wpedantic -Wconversion -Wshadow \
    -Werror -pthread source.cpp -o /tmp/codex_cpp_release
/tmp/codex_cpp_release

# 第二編譯器
clang++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror \
    -pthread source.cpp -o /tmp/codex_cpp_clang
/tmp/codex_cpp_clang

# Sanitizer；一般終端才可完整驗 LeakSanitizer
g++ -std=c++20 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
    -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread \
    source.cpp -o /tmp/codex_cpp_sanitize
ASAN_OPTIONS=halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 /tmp/codex_cpp_sanitize
```

若檔案屬 C++11／14／17 章，四條命令都換成該章標準，不可為了方便一律 C++20。

---

## 9. Repo 級交付清單

### 9.1 每次修改都要做

- [ ] 只改 `Codex C++by Examples/`，沒有碰 Claude 路徑。
- [ ] 基礎、LeetCode、實務案例都真正執行，且說明緊鄰相關程式碼。
- [ ] 面試答案區分標準保證、常見實作與工程取捨。
- [ ] 檔尾編譯命令可直接複製，輸出來自實跑。
- [ ] 變更檔通過 debug、release；重要語意通過 Clang 與 sanitizer。
- [ ] `tools/audit_textbook.sh` 通過。
- [ ] `tools/update_readme_indexes.sh --check` 通過。
- [ ] repository 沒有新增 executable 或測試垃圾。
- [ ] commit 只 stage 明確 Codex pathspec。

### 9.2 何時必須全跑 405 檔

只有下列情況才完整跑 GCC debug/release、Clang 與 sanitizer：

- 修改 `tools/check_all.sh`、共用 flags 或 C++ 標準映射。
- 大量機械修改可能影響所有檔案。
- compiler／standard library 大版本更新。
- 結構稽核顯示廣泛漂移。
- 要建立新的「完整驗證基線」。

一般只新增／修改少數章節時，依 `AGENTS.md` 的增量規則重驗變更檔與必要的 repo guard，
不要無理由重跑全部教材。

---

## 10. 完成定義

一個檔案只有同時滿足以下條件才算完成：

1. 沒有外部教材也能理解。
2. 基本概念、API 契約、用法與錯誤模型完整。
3. LeetCode 與實務案例位於正確位置、真正實作、真正測試。
4. 面試題能從 30 秒核心答到資深追問，不含背誦式空話。
5. debug、release 皆能編譯執行；需要時已做 Clang/sanitizer。
6. 檔尾命令安全且可複製，預期輸出與 executable 相符。
7. 沒有污染 repo，也沒有動到 Claude 內容。

若其中任一項只能靠「讀者自己猜」，就尚未完成。
