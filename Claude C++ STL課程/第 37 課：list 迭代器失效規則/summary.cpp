// ============================================================
// 第 37 課 總結：list 迭代器失效規則
// 編譯：g++ -std=c++17 -o summary summary.cpp
// ============================================================
// 【list 迭代器失效規則（極簡版）】
//   insert / push  → 所有迭代器有效 ✅
//   erase          → 只有被刪元素的迭代器失效 ❌ 其餘有效 ✅
//   splice         → 所有迭代器有效 ✅（被移植的也有效）
//   sort           → 所有迭代器有效 ✅
//   remove/unique  → 被刪的失效 ❌ 其餘有效 ✅
//   reverse        → 所有迭代器有效 ✅
//   merge          → 所有迭代器有效 ✅
//   clear          → 全部失效 ❌
//
// 【vs vector 的巨大差異】
//   vector insert/erase → 插入/刪除點之後的所有迭代器失效
//   list insert/erase   → 只有被刪的失效，其餘全部安全
//   → 這就是 list 最大的優勢之一：迭代器穩定性
//
// 【安全刪除模式】
//   ❌ for (...; ++it) { lst.erase(it); }      → it 已失效，++it UB
//   ✅ for (...; ) { it = lst.erase(it); }      → erase 回傳下一個
//   ✅ lst.remove(val) / lst.remove_if(pred)    → 更簡潔
// ============================================================

#include <iostream>
#include <list>
#include <vector>
#include <string>
using namespace std;

template <typename T>
void print(const string& label, const list<T>& lst) {
    cout << "  " << label << " [" << lst.size() << "]: ";
    for (const auto& v : lst) cout << v << " ";
    cout << endl;
}

int main() {
    // 1. insert 後全部有效
    cout << "===== insert 後迭代器有效 =====\n";
    {
        list<int> lst = {10,20,30,40,50};
        auto it10 = lst.begin();
        auto it30 = next(it10, 2);
        auto it50 = next(it10, 4);
        lst.insert(it30, 25);
        lst.push_back(60);
        lst.push_front(5);
        cout << "  *it10=" << *it10 << " *it30=" << *it30 << " *it50=" << *it50 << " ✅\n";
        print("結果", lst);
    }

    // 2. erase 只影響被刪的
    cout << "\n===== erase 只影響被刪的 =====\n";
    {
        list<int> lst = {10,20,30,40,50};
        auto it10=lst.begin(), it20=next(it10), it30=next(it20);
        auto it40=next(it30), it50=next(it40);
        lst.erase(it30);  // it30 失效
        cout << "  刪30後：*it10=" << *it10 << " *it20=" << *it20
             << " *it40=" << *it40 << " *it50=" << *it50 << " ✅\n";

        // 對比 vector
        cout << "  （vector erase 後，刪除點之後的迭代器全部失效！）\n";
    }

    // 3. splice 後全部有效
    cout << "\n===== splice 後迭代器有效 =====\n";
    {
        list<int> A = {1,2,3}, B = {10,20,30};
        auto it1 = A.begin();
        auto it20 = next(B.begin());
        A.splice(A.end(), B, it20);
        cout << "  *it1=" << *it1 << " *it20=" << *it20 << " ✅\n";
        cout << "  （it20 仍有效，但現在屬於 A）\n";
    }

    // 4. sort 後全部有效
    cout << "\n===== sort 後迭代器有效 =====\n";
    {
        list<int> lst = {50,30,10,40,20};
        auto it30 = next(lst.begin());
        cout << "  sort 前 *it=" << *it30 << " addr=" << &(*it30) << "\n";
        lst.sort();
        cout << "  sort 後 *it=" << *it30 << " addr=" << &(*it30) << " ✅\n";
        cout << "  （值和地址不變，在 list 中位置改變）\n";
    }

    // 5. remove 只使被刪的失效
    cout << "\n===== remove 迭代器影響 =====\n";
    {
        list<int> lst = {1,2,3,2,4,2,5};
        auto it1=lst.begin(), it3=next(it1,2), it4=next(it3,2), it5=next(it4,2);
        lst.remove(2);
        cout << "  remove(2)後：" << *it1 << " " << *it3 << " " << *it4 << " " << *it5 << " ✅\n";
    }

    // 6. reverse 後全部有效
    cout << "\n===== reverse 後迭代器有效 =====\n";
    {
        list<int> lst = {10,20,30,40,50};
        auto it_begin = lst.begin();
        lst.reverse();
        cout << "  *it_begin=" << *it_begin << "（仍是10，但現在是最後一個）\n";
        cout << "  it_begin == prev(end)? " << (it_begin==prev(lst.end())?"是":"否") << " ✅\n";
    }

    // 7. 正確 vs 錯誤的刪除模式
    cout << "\n===== 安全刪除模式 =====\n";
    {
        list<int> lst = {1,2,3,4,5,6,7,8};
        // ✅ 正確
        for (auto it = lst.begin(); it != lst.end(); ) {
            if (*it % 2 == 0) it = lst.erase(it);
            else ++it;
        }
        print("刪偶數（正確）", lst);
    }
    cout << "  ❌ 錯誤：erase(it); ++it; → it 已失效\n";
    cout << "  ✅ 正確：it = erase(it);  → erase 回傳下一個\n";

    // 8. 書籤系統
    cout << "\n===== 實戰：迭代器書籤 =====\n";
    {
        list<string> doc = {"第一章","第二章","第三章","第四章","第五章"};
        auto bm1 = next(doc.begin());    // 第二章
        auto bm3 = next(doc.begin(), 2); // 第三章
        doc.insert(bm3, "新增章節");
        doc.erase(next(doc.begin(), 4)); // 刪第四章
        doc.sort();
        cout << "  經過 insert+erase+sort 後：\n";
        cout << "  書籤1=" << *bm1 << " 書籤3=" << *bm3 << " ✅ 仍有效\n";
    }

    return 0;
}
