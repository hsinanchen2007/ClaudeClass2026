#include <iostream>
#include <vector>
#include <chrono>
#include <string>
using namespace std;
using namespace chrono;

int main() {
    const int N = 1000000;

    // 測試 push_back（臨時物件）
    {
        vector<string> vec;
        vec.reserve(N);
        auto start = high_resolution_clock::now();

        for (int i = 0; i < N; i++) {
            vec.push_back(string("Hello, World! This is a test string."));
        }

        auto end = high_resolution_clock::now();
        auto ms = duration_cast<milliseconds>(end - start).count();
        cout << "push_back(臨時物件): " << ms << " ms" << endl;
    }

    // 測試 emplace_back
    {
        vector<string> vec;
        vec.reserve(N);
        auto start = high_resolution_clock::now();

        for (int i = 0; i < N; i++) {
            vec.emplace_back("Hello, World! This is a test string.");
        }

        auto end = high_resolution_clock::now();
        auto ms = duration_cast<milliseconds>(end - start).count();
        cout << "emplace_back:       " << ms << " ms" << endl;
    }

    return 0;
}
