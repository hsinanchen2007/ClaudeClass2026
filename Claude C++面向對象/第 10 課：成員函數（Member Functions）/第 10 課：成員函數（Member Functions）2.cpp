#include <iostream>
#include <string>
using namespace std;

class StringHelper {
public:
    string text = "";

    // 無參數、無返回值
    void clear() {
        text = "";
    }

    // 有參數、無返回值
    void append(const string& suffix) {
        text += suffix;
    }

    // 無參數、有返回值
    int length() {
        return text.length();
    }

    // 有參數、有返回值
    char charAt(int index) {
        if (index < 0 || index >= (int)text.length()) {
            cout << "錯誤：索引超出範圍" << endl;
            return '\0';
        }
        return text[index];
    }

    // 多個參數
    string substring(int start, int len) {
        if (start < 0 || start >= (int)text.length()) {
            return "";
        }
        return text.substr(start, len);
    }

    // 返回 bool
    bool contains(const string& target) {
        return text.find(target) != string::npos;
    }

    // 返回自身的引用（鏈式調用的基礎）, 這裡的返回類型是 StringHelper&，表示返回對象本身的引用
    // 這樣做的好處是可以實現鏈式調用，例如 sh.appendChain("Hello").appendChain(" World");
    // 在 appendChain 函數中，我們首先將傳入的 suffix 附加到 text 成員變量上，然後返回 *this，
    // 這裡的 *this 是一個指向當前對象的指標，通過解引用（*）我們獲得了對象本身，從而實現了鏈式調用。
    // 這種設計模式在許多 C++ 庫中都很常見，特別是在需要連續調用多個方法的情況下，可以使代碼更加簡潔和易讀。
    StringHelper& appendChain(const string& suffix) {
        text += suffix;
        return *this;  // 返回自己（this 指標會在第 26 課詳講）
    }
};

int main() {
    StringHelper sh;

    sh.append("Hello");
    sh.append(", ");
    sh.append("World!");

    cout << "文字: " << sh.text << endl;
    cout << "長度: " << sh.length() << endl;
    cout << "第 0 個字元: " << sh.charAt(0) << endl;
    cout << "子字串(7, 5): " << sh.substring(7, 5) << endl;
    cout << "包含 \"World\": " << (sh.contains("World") ? "是" : "否") << endl;

    cout << "\n--- 鏈式調用 ---" << endl;
    StringHelper sh2;
    sh2.appendChain("C++").appendChain(" is").appendChain(" awesome!");
    cout << sh2.text << endl;

    return 0;
}
