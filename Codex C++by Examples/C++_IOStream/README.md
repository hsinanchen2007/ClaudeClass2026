# IOStream：資料、格式與狀態

iostream 把 source/sink、buffer、format state 與 locale 組合在一起。方便之處也是陷阱：一次設定 <code>std::hex</code>、precision 或 boolalpha，狀態會留在 stream，影響後續輸出。

## 格式化

C++20 <code>std::format</code> 很適合建立 string，但本機標準庫支援度可能依版本不同；本章以 iostream 展示共通模型。使用 manipulator 時要清楚它是只影響下一欄（setw）或持續狀態（hex、fixed、precision）。

## 解析

Formatted extraction 會跳過空白並在失敗時設 failbit。不要寫 <code>while (!stream.eof())</code>；正確模式是把 extraction 本身當條件：<code>while (stream &gt;&gt; value)</code>。

## File I/O

開檔失敗預設不丟例外，只會設 failbit；呼叫端要測試 stream 或啟用 exceptions。文字/二進位模式、編碼、newline translation 是不同層次，不能假設 iostream 自動理解 UTF-8。

## String streams

適合小型格式化、測試與教學 parser；高吞吐解析通常應用 from_chars 或專用 parser，避免 locale 與配置成本。

## 範例索引

本章共 11 個可獨立編譯執行的 `.cpp`；`summary.cpp` 是面試前速查。

<details>
<summary>展開全部檔案</summary>

- [`01_overview.cpp`](01_overview.cpp)
- [`02_cout_cin.cpp`](02_cout_cin.cpp)
- [`03_manipulators.cpp`](03_manipulators.cpp)
- [`04_sync_with_stdio.cpp`](04_sync_with_stdio.cpp)
- [`05_fstream_text.cpp`](05_fstream_text.cpp)
- [`06_fstream_binary.cpp`](06_fstream_binary.cpp)
- [`07_stringstream.cpp`](07_stringstream.cpp)
- [`08_getline.cpp`](08_getline.cpp)
- [`09_error_state.cpp`](09_error_state.cpp)
- [`10_practical_log.cpp`](10_practical_log.cpp)
- [`summary.cpp`](summary.cpp)

</details>

## 練習

1. 寫一個 RAII format-state guard。
2. 解析每行兩欄資料，錯行要報 line number。
3. 寫 binary uint32_t 前先指定 byte order。
