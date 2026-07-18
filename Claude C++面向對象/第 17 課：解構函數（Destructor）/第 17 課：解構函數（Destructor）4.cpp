#include <iostream>
#include <fstream>
#include <string>
using namespace std;

class FileWriter {
private:
    ofstream file;
    string filename;

public:
    FileWriter(const string& fname) : filename(fname) {
        file.open(filename);
        if (file.is_open()) {
            cout << "  [建構] 打開檔案: " << filename << endl;
        } else {
            cout << "  [建構] 無法打開檔案: " << filename << endl;
        }
    }
    
    ~FileWriter() {
        if (file.is_open()) {
            file.close();    // 自動關閉檔案
            cout << "  [解構] 關閉檔案: " << filename << endl;
        }
    }
    
    void writeLine(const string& text) {
        if (file.is_open()) {
            file << text << endl;
            cout << "  寫入: " << text << endl;
        }
    }
};

int main() {
    cout << "=== 檔案寫入範例 ===" << endl;
    
    {
        FileWriter writer("test_output.txt");
        writer.writeLine("第一行");
        writer.writeLine("第二行");
        writer.writeLine("第三行");
        
        cout << "  --- 即將離開區塊 ---" << endl;
    }
    // writer 離開作用域，解構函數自動關閉檔案
    
    cout << "  --- 檔案已自動關閉 ---" << endl;
    return 0;
}
// 在這裡，FileWriter 的解構函數會自動被調用，關閉檔案