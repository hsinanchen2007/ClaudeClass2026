// ============================================================================
// 課題 3：I/O manipulators 與 persistent formatting state
// ============================================================================
//
// setw 只影響下一個欄位；hex/fixed/setprecision/boolalpha/setfill 等會持續留在 stream。
// 共用 cout/log stream 的 helper 若改狀態不還原，後面的 caller 會收到意外格式。可保存
// flags/precision/fill，scope exit 還原；或先寫到 local ostringstream。
//
// fixed 下 setprecision 表示小數位數；defaultfloat 下是有效數字數。浮點 decimal 只供
// 顯示，不可用格式化後字串做精確數值比較。
// ============================================================================

#include <algorithm>
#include <cassert>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

class FormatGuard {
public:
    explicit FormatGuard(std::ios& stream)
        : stream_(stream), flags_(stream.flags()), precision_(stream.precision()), fill_(stream.fill()) {}
    ~FormatGuard()
    {
        stream_.flags(flags_);
        stream_.precision(precision_);
        stream_.fill(fill_);
    }
private:
    std::ios& stream_;
    std::ios::fmtflags flags_;
    std::streamsize precision_;
    char fill_;
};

std::string format_price(double value)
{
    std::ostringstream output;
    output << '$' << std::fixed << std::setprecision(2) << value;
    return output.str();
}

void basic_example()
{
    assert(format_price(12.5) == "$12.50");
    std::ostringstream output;
    {
        FormatGuard guard(output);
        output << std::hex << std::showbase << 255;
    }
    output << ' ' << 255;
    assert(output.str() == "0xff 255");
    std::cout << "[基礎] formatting state restored after guarded scope\n";
}

// LeetCode 168：Excel Sheet Column Title；輸出 formatting 與演算法分開。
std::string convert_to_title(int number)
{
    std::string result;
    while (number > 0) {
        --number;
        result.push_back(static_cast<char>('A' + number % 26));
        number /= 26;
    }
    std::reverse(result.begin(), result.end());
    return result;
}

void leetcode_168_example()
{
    assert(convert_to_title(1) == "A");
    assert(convert_to_title(28) == "AB");
    assert(convert_to_title(701) == "ZY");
    std::cout << "[LeetCode 168] titles A, AB, ZY\n";
}

// 實務：stable tabular report 用 setw/right，但欄寬不是 Unicode display width。
void practical_example()
{
    std::ostringstream output;
    output << std::left << std::setw(8) << "GPU" << std::right << std::setw(4) << 75 << '%';
    assert(output.str() == "GPU       75%");
    std::cout << "[實務] fixed ASCII columns formatted predictably\n";
}

int main()
{
    basic_example();
    leetcode_168_example();
    practical_example();
}

// 練習：比較 defaultfloat/fixed/scientific 下 setprecision(3) 的輸出。
// 複雜度：設定 flag 近似 O(1)，格式化每個 value 的成本依位數/locale；大量輸出應整體量測。
// 生命週期：多數 manipulator state 會留在 stream 直到改回；RAII saver 應與被借用 stream 同 scope。

/*
【本課面試問答】
Q1：哪些 formatter 狀態會持續，`setw` 為何特別？
A：base、floatfield、precision、fill、locale 等通常留在 stream，會影響後續輸出；`setw` 的 width
通常在下一次 formatted insertion 後重設。library helper 若借用 caller stream，應保存並恢復它改過的狀態。

Q2：`std::endl` 與 `'\n'` 差在哪？
A：endl 寫換行後強制 flush；`'\n'` 只寫字元。每行 endl 可能讓大量輸出因頻繁 system call/同步而
變慢。只有互動提示、協定邊界或確實需要立即可見時才主動 flush。

Q3：`setw` 能否正確對齊中文或 emoji？
A：不能保證。iostream width 以字元序列/locale 規則處理，不等於終端顯示欄寬；UTF-8 一個 glyph
可能多 bytes，combining/wide characters 又不同。正式表格要用 Unicode display-width aware library。
*/
