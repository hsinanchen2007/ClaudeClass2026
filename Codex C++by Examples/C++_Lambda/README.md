# Lambda：把局部行為安全地封裝

Lambda 是匿名 function object。編譯器建立一個 closure type，capture 會成為其資料成員，參數與 body 則形成 call operator。理解這一點，比背 capture syntax 更重要。

## Capture 選擇

- <code>[value]</code>：建立時複製，closure 自己持有副本。
- <code>[&amp;value]</code>：借用外部物件，外部必須活得更久。
- <code>[value = std::move(resource)]</code>：把所有權搬入 closure。
- <code>[this]</code>：只借用 object pointer；非同步執行時特別容易懸空。
- <code>[self = shared_from_this()]</code>：必要時延長 object 生命，但也可能形成 cycle。

預設 capture <code>[=]</code>/<code>[&amp;]</code> 在短小同步演算法中方便，在長生命 callback 中則容易藏住所有權。後者應明列 capture。

## mutable

Value capture 在預設 const call operator 中不可修改；<code>mutable</code> 允許改 closure 內副本，不會修改原變數。

## Comparator 與 state

Comparator 應是穩定的 strict weak ordering。Stateful lambda 很適合計數或 cache，但若演算法會複製 callable，就不能假設最後檢查原 closure 一定看得到所有狀態。

## 範例索引

本章共 11 個可獨立編譯執行的 `.cpp`；`summary.cpp` 是面試前速查。

<details>
<summary>展開全部檔案</summary>

- [`01_basic_lambda.cpp`](01_basic_lambda.cpp)
- [`02_capture_modes.cpp`](02_capture_modes.cpp)
- [`03_mutable_lambda.cpp`](03_mutable_lambda.cpp)
- [`04_init_capture.cpp`](04_init_capture.cpp)
- [`05_generic_lambda.cpp`](05_generic_lambda.cpp)
- [`06_std_function.cpp`](06_std_function.cpp)
- [`07_function_pointer.cpp`](07_function_pointer.cpp)
- [`08_std_bind.cpp`](08_std_bind.cpp)
- [`09_lambda_in_algorithm.cpp`](09_lambda_in_algorithm.cpp)
- [`10_lambda_pitfalls.cpp`](10_lambda_pitfalls.cpp)
- [`summary.cpp`](summary.cpp)

</details>

## 練習

1. 寫一個 callback，安全擁有一份設定字串。
2. 用 lambda 依字串長度排序，長度相同時再按字典序。
3. 解釋為何把 local variable reference capture 到返回的 lambda 會懸空。
