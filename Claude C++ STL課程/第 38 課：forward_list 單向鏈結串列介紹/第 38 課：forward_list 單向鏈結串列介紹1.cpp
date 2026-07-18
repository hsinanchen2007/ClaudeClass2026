// lesson38_forward_list_intro.cpp
// 編譯：g++ -std=c++17 -Wall -Wextra -o lesson38 lesson38_forward_list_intro.cpp

#include <iostream>
#include <forward_list>
#include <list>
#include <vector>
#include <string>
#include <iterator>    // std::distance
using namespace std;

template <typename T>
void print_flist(const string& label, const forward_list<T>& flst) {
    cout << label << ": ";
    for (const auto& val : flst) cout << val << " ";
    // 注意：不能用 flst.size()
    cout << "(元素數: " << distance(flst.begin(), flst.end()) << ")" << endl;
}

int main() {
    // ===== 1. 初始化方式 =====
    cout << "===== 初始化方式 =====" << endl;
    {
        forward_list<int> fl1;                        // 空
        forward_list<int> fl2(5, 42);                 // 5 個 42
        forward_list<int> fl3 = {10, 20, 30, 40, 50}; // 初始化列表
        forward_list<int> fl4(fl3);                   // 複製建構
        forward_list<int> fl5(fl3.begin(), fl3.end()); // 範圍建構

        print_flist("fl1（空）    ", fl1);
        print_flist("fl2（填充）  ", fl2);
        print_flist("fl3（列表）  ", fl3);
        print_flist("fl4（複製）  ", fl4);
        print_flist("fl5（範圍）  ", fl5);
    }

    // ===== 2. 元素存取 =====
    cout << "\n===== 元素存取 =====" << endl;
    {
        forward_list<int> flst = {10, 20, 30};

        cout << "front(): " << flst.front() << endl;
        // cout << flst.back();     // ✗ 編譯錯誤！
        // cout << flst[0];         // ✗ 編譯錯誤！
        // cout << flst.size();     // ✗ 編譯錯誤！
        cout << "empty(): " << (flst.empty() ? "是" : "否") << endl;
    }

    // ===== 3. push_front / pop_front =====
    cout << "\n===== push_front / pop_front =====" << endl;
    {
        forward_list<int> flst;

        flst.push_front(30);
        flst.push_front(20);
        flst.push_front(10);
        print_flist("push_front x3", flst);   // 10 20 30

        flst.pop_front();
        print_flist("pop_front    ", flst);   // 20 30

        // push_back 不存在！
        // flst.push_back(40);    // ✗ 編譯錯誤
    }

    // ===== 4. before_begin 的用法 =====
    cout << "\n===== before_begin =====" << endl;
    {
        forward_list<int> flst = {20, 30, 40};
        print_flist("初始      ", flst);

        // 在最前面插入（用 before_begin）
        flst.insert_after(flst.before_begin(), 10);
        print_flist("頭端插入  ", flst);   // 10 20 30 40

        // 用 emplace_after 在頭端建構
        flst.emplace_after(flst.before_begin(), 5);
        print_flist("頭端emplace", flst);   // 5 10 20 30 40
    }

    // ===== 5. insert_after 各種重載 =====
    cout << "\n===== insert_after =====" << endl;
    {
        forward_list<int> flst = {10, 30, 50};
        print_flist("初始            ", flst);

        // 5a. 在第一個元素（10）之後插入 20
        auto it = flst.begin();    // 指向 10
        flst.insert_after(it, 20);
        print_flist("insert_after(20)", flst);  // 10 20 30 50

        // 5b. 插入多個相同值
        it = flst.begin();
        advance(it, 3);           // 指向 50
        flst.insert_after(it, 3, 60);
        print_flist("insert_after x3 ", flst);  // 10 20 30 50 60 60 60

        // 5c. 插入初始化列表
        flst.insert_after(flst.before_begin(), {1, 2, 3});
        print_flist("insert_after{}  ", flst);  // 1 2 3 10 20 30 50 60 60 60
    }

    // ===== 6. erase_after =====
    cout << "\n===== erase_after =====" << endl;
    {
        forward_list<int> flst = {10, 20, 30, 40, 50, 60};
        print_flist("初始             ", flst);

        // 6a. 刪除第一個元素之後的元素（= 刪除 20）
        flst.erase_after(flst.begin());
        print_flist("erase_after(begin)", flst);  // 10 30 40 50 60

        // 6b. 刪除範圍 (pos, last)
        // 刪除 30 和 40（即 begin 之後到 50 之前）
        auto it = flst.begin();      // 指向 10
        auto last = it;
        advance(last, 3);            // 指向 50
        flst.erase_after(it, last);
        print_flist("erase_after範圍  ", flst);  // 10 50 60

        // 6c. 刪除第一個元素（用 before_begin）
        flst.erase_after(flst.before_begin());
        print_flist("刪除第一個       ", flst);  // 50 60
    }

    // ===== 7. 特有成員函數 =====
    cout << "\n===== 特有成員函數 =====" << endl;
    {
        // sort
        forward_list<int> flst = {5, 3, 8, 1, 9, 2};
        print_flist("排序前  ", flst);
        flst.sort();
        print_flist("sort 後 ", flst);

        // reverse
        flst.reverse();
        print_flist("reverse ", flst);

        // unique（需要先排序）
        forward_list<int> flst2 = {1, 1, 2, 3, 3, 3, 4};
        flst2.unique();
        print_flist("unique  ", flst2);

        // remove
        forward_list<int> flst3 = {1, 2, 3, 2, 4, 2, 5};
        flst3.remove(2);
        print_flist("remove(2)", flst3);

        // remove_if
        forward_list<int> flst4 = {1, 2, 3, 4, 5, 6, 7, 8};
        flst4.remove_if([](int x) { return x % 2 == 0; });
        print_flist("remove偶數", flst4);

        // merge
        forward_list<int> a = {1, 3, 5};
        forward_list<int> b = {2, 4, 6};
        a.merge(b);
        print_flist("merge   ", a);
        print_flist("b(空了) ", b);
    }

    // ===== 8. 遍歷方式 =====
    cout << "\n===== 遍歷方式 =====" << endl;
    {
        forward_list<int> flst = {10, 20, 30, 40, 50};

        // 方式 1：範圍 for
        cout << "範圍 for：";
        for (int val : flst) cout << val << " ";
        cout << endl;

        // 方式 2：迭代器
        cout << "迭代器：  ";
        for (auto it = flst.begin(); it != flst.end(); ++it) {
            cout << *it << " ";
        }
        cout << endl;

        // ✗ 不能反向遍歷！
        // for (auto rit = flst.rbegin(); rit != flst.rend(); ++rit)
        //     → 編譯錯誤！forward_list 沒有 rbegin/rend

        // ✗ 不能用 --it
        // auto it = flst.end();
        // --it;    → 編譯錯誤！Forward Iterator 不支援 --
    }

    // ===== 9. 記憶體對比 =====
    cout << "\n===== 記憶體對比 =====" << endl;
    {
        cout << "sizeof(forward_list<int>): " << sizeof(forward_list<int>) << " bytes" << endl;
        cout << "sizeof(list<int>):         " << sizeof(list<int>) << " bytes" << endl;
        cout << "sizeof(vector<int>):       " << sizeof(vector<int>) << " bytes" << endl;

        // 估算 10 萬個 int 的記憶體
        const int N = 100000;

        // forward_list: 每個節點 = next(8) + int(4) + padding(4) = 16 bytes
        // list:         每個節點 = prev(8) + next(8) + int(4) + padding(4) = 24 bytes
        // vector:       N × 4 bytes（連續）

        cout << "\n" << N << " 個 int 的估算記憶體：" << endl;
        cout << "  forward_list: ~" << (N * 16) / 1024 << " KB" << endl;
        cout << "  list:         ~" << (N * 24) / 1024 << " KB" << endl;
        cout << "  vector:       ~" << (N * 4) / 1024 << " KB" << endl;
    }

    // ===== 10. 迭代器失效規則 =====
    cout << "\n===== 迭代器失效規則 =====" << endl;
    {
        forward_list<int> flst = {10, 20, 30, 40, 50};

        auto it20 = next(flst.begin());    // 指向 20
        auto it40 = next(it20, 2);         // 指向 40

        cout << "操作前: *it20=" << *it20 << " *it40=" << *it40 << endl;

        // insert_after 不影響任何迭代器
        flst.insert_after(flst.begin(), 15);

        cout << "insert後: *it20=" << *it20 << " *it40=" << *it40 << endl;

        // push_front 不影響任何迭代器
        flst.push_front(5);

        cout << "push後: *it20=" << *it20 << " *it40=" << *it40 << endl;

        // erase_after 只影響被刪的
        auto it_before_30 = next(flst.begin(), 3);  // 指向 20
        flst.erase_after(it_before_30);              // 刪除 30

        cout << "erase後: *it20=" << *it20 << " *it40=" << *it40 << endl;

        print_flist("最終結果", flst);
        cout << "→ 規則和 list 完全一致：除了被刪的，其餘全有效" << endl;
    }

    return 0;
}
