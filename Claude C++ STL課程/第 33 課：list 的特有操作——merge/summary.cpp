// ============================================================
// 第 33 課 總結：list 的特有操作——merge
// 編譯：g++ -std=c++17 -o summary summary.cpp
// ============================================================
// 【merge = 合併兩個已排序的 list】
//   A.merge(B)             預設升序合併
//   A.merge(B, comp)       自訂比較函式
//   ★ 前提：A 和 B 必須已按相同順序排序！否則結果不正確
//   ★ 合併後 B 變空，元素全移到 A
//   ★ 穩定：相同值的元素，A 的排在 B 前面
//   ★ 迭代器全部有效（只改指標，不 new/delete）
// ============================================================

#include <iostream>
#include <list>
#include <string>
#include <functional>
using namespace std;

template <typename T>
void print(const string& label, const list<T>& lst) {
    cout << "  " << label << " [" << lst.size() << "]: ";
    for (const auto& v : lst) cout << v << " ";
    cout << (lst.empty() ? "(空)" : "") << endl;
}

int main() {
    // 1. 升序 merge
    cout << "===== 升序 merge =====\n";
    {
        list<int> A = {2,5,8,10}, B = {1,3,6,7,9};
        print("A", A); print("B", B);
        A.merge(B);
        print("merge 後 A", A);
        print("merge 後 B", B);
    }

    // 2. 降序 merge
    cout << "\n===== 降序 merge =====\n";
    {
        list<int> A = {10,8,5,2}, B = {9,7,6,3,1};
        A.merge(B, greater<int>());
        print("降序 A", A);
    }

    // 3. 穩定性驗證
    cout << "\n===== 穩定性 =====\n";
    {
        struct Item { string src; int val; };
        list<Item> A = {{"A",1},{"A",3},{"A",5}};
        list<Item> B = {{"B",1},{"B",3},{"B",4}};
        A.merge(B, [](const Item& a, const Item& b) { return a.val < b.val; });
        cout << "  合併：";
        for (auto& i : A) cout << i.src << i.val << " ";
        cout << "\n  （相同值時 A 排在 B 前面 → 穩定）\n";
    }

    // 4. 未排序 → 必須先 sort
    cout << "\n===== 先 sort 再 merge =====\n";
    {
        list<int> A = {5,2,8}, B = {3,9,1};
        A.sort(); B.sort();
        A.merge(B);
        print("正確結果", A);
    }

    // 5. 多路合併
    cout << "\n===== 多路合併 =====\n";
    {
        list<int> lists[] = {{1,5,9},{2,6,10},{3,7,11},{4,8,12}};
        for (int i = 1; i < 4; i++) lists[0].merge(lists[i]);
        print("4 路合併", lists[0]);
    }

    // 6. 迭代器穩定性
    cout << "\n===== 迭代器穩定性 =====\n";
    {
        list<int> A = {2,5,8}, B = {1,3,6};
        auto it5 = next(A.begin()); auto it3 = next(B.begin());
        A.merge(B);
        cout << "  *it5=" << *it5 << " *it3=" << *it3 << " ✅ 仍有效\n";
    }

    // 7. 自訂物件
    cout << "\n===== 自訂物件 merge =====\n";
    {
        struct Student { string name; double gpa; };
        list<Student> cA = {{"Alice",3.9},{"Charlie",3.5},{"Eve",3.2}};
        list<Student> cB = {{"Bob",3.8},{"David",3.6},{"Frank",3.0}};
        cA.merge(cB, [](const Student& a, const Student& b) { return a.gpa > b.gpa; });
        cout << "  GPA 降序合併：\n";
        for (auto& s : cA) cout << "    " << s.name << " " << s.gpa << "\n";
    }

    return 0;
}
