// lesson37_list_iterator_invalidation.cpp
// 編譯：g++ -std=c++17 -Wall -Wextra -o lesson37 lesson37_list_iterator_invalidation.cpp

#include <iostream>
#include <list>
#include <vector>
#include <string>
using namespace std;

template <typename T>
void print_list(const string& label, const list<T>& lst) {
    cout << label << " [" << lst.size() << "]: ";
    for (const auto& val : lst) cout << val << " ";
    cout << endl;
}

int main() {
    // ===== 1. insert 後迭代器全部有效 =====
    cout << "===== 1. insert 後迭代器有效 =====" << endl;
    {
        list<int> lst = {10, 20, 30, 40, 50};

        // 保存所有元素的迭代器
        auto it10 = lst.begin();
        auto it20 = next(it10);
        auto it30 = next(it20);
        auto it40 = next(it30);
        auto it50 = next(it40);

        // 大量插入
        lst.insert(it30, 25);         // 在 30 前面
        lst.push_back(60);            // 尾端
        lst.push_front(5);            // 頭端
        lst.insert(it50, {45, 48});   // 在 50 前面

        // 驗證所有舊迭代器仍然有效
        cout << "*it10=" << *it10 << " *it20=" << *it20
             << " *it30=" << *it30 << " *it40=" << *it40
             << " *it50=" << *it50 << endl;

        print_list("完整 list", lst);
        cout << "→ 所有舊迭代器在插入後仍然有效！" << endl;
    }

    // ===== 2. erase 只影響被刪的迭代器 =====
    cout << "\n===== 2. erase 只影響被刪的 =====" << endl;
    {
        list<int> lst = {10, 20, 30, 40, 50};

        auto it10 = lst.begin();
        auto it20 = next(it10);
        auto it30 = next(it20);
        auto it40 = next(it30);
        auto it50 = next(it40);

        // 刪除 30
        lst.erase(it30);
        // it30 已失效！不能再使用
        // 但其他迭代器全部有效：
        cout << "*it10=" << *it10 << " *it20=" << *it20
             << " *it40=" << *it40 << " *it50=" << *it50 << endl;

        print_list("刪除 30 後", lst);

        // 對比 vector 的行為
        cout << "\n--- 對比 vector ---" << endl;
        vector<int> vec = {10, 20, 30, 40, 50};
        auto vit40 = vec.begin() + 3;  // 指向 40
        cout << "erase 前 *vit40 = " << *vit40 << endl;
        vec.erase(vec.begin() + 2);    // 刪除 30
        // vit40 現在失效了（30 之後的元素都搬移了）
        // 以下是未定義行為，但為了教學展示：
        cout << "erase 後 vec[3] = " << vec[3]
             << "（vit40 已失效，不應存取）" << endl;
    }

    // ===== 3. splice 後所有迭代器有效 =====
    cout << "\n===== 3. splice 後迭代器有效 =====" << endl;
    {
        list<int> A = {1, 2, 3};
        list<int> B = {10, 20, 30};

        auto it_1 = A.begin();
        auto it_20 = next(B.begin());

        cout << "splice 前: *it_1=" << *it_1 << " *it_20=" << *it_20 << endl;

        // 把 B 的 20 移到 A
        A.splice(A.end(), B, it_20);

        cout << "splice 後: *it_1=" << *it_1 << " *it_20=" << *it_20 << endl;
        print_list("A", A);
        print_list("B", B);
        cout << "→ it_20 仍有效，但現在屬於 A" << endl;
    }

    // ===== 4. sort 後迭代器有效 =====
    cout << "\n===== 4. sort 後迭代器有效 =====" << endl;
    {
        list<int> lst = {50, 30, 10, 40, 20};

        auto it_30 = next(lst.begin());    // 指向 30
        auto it_40 = next(it_30, 2);       // 指向 40

        cout << "sort 前: *it_30=" << *it_30
             << " addr=" << &(*it_30) << endl;
        cout << "sort 前: *it_40=" << *it_40
             << " addr=" << &(*it_40) << endl;

        lst.sort();

        cout << "sort 後: *it_30=" << *it_30
             << " addr=" << &(*it_30) << endl;
        cout << "sort 後: *it_40=" << *it_40
             << " addr=" << &(*it_40) << endl;

        print_list("排序結果", lst);
        cout << "→ 值和地址都不變！只是在 list 中的位置改變了" << endl;
    }

    // ===== 5. remove 只使被刪節點的迭代器失效 =====
    cout << "\n===== 5. remove 的迭代器影響 =====" << endl;
    {
        list<int> lst = {1, 2, 3, 2, 4, 2, 5};

        auto it_1 = lst.begin();
        auto it_3 = next(it_1, 2);
        auto it_4 = next(it_3, 2);
        auto it_5 = next(it_4, 2);

        cout << "remove 前: " << *it_1 << " " << *it_3
             << " " << *it_4 << " " << *it_5 << endl;

        lst.remove(2);   // 刪除所有 2

        // 所有非 2 的迭代器仍然有效
        cout << "remove 後: " << *it_1 << " " << *it_3
             << " " << *it_4 << " " << *it_5 << endl;

        print_list("結果", lst);
    }

    // ===== 6. reverse 後迭代器有效 =====
    cout << "\n===== 6. reverse 後迭代器有效 =====" << endl;
    {
        list<int> lst = {10, 20, 30, 40, 50};

        auto it_begin = lst.begin();        // 指向 10
        auto it_second = next(it_begin);    // 指向 20

        cout << "reverse 前: begin→" << *it_begin
             << " second→" << *it_second << endl;

        lst.reverse();

        cout << "reverse 後: *it_begin=" << *it_begin
             << " *it_second=" << *it_second << endl;
        cout << "lst.front()=" << lst.front()
             << " lst.back()=" << lst.back() << endl;

        // 注意：it_begin 仍指向 10，但 10 現在是最後一個元素
        // 所以 it_begin != lst.begin() 了！
        cout << "it_begin == lst.begin()? "
             << (it_begin == lst.begin() ? "是" : "否") << endl;
        cout << "it_begin == prev(lst.end())? "
             << (it_begin == prev(lst.end()) ? "是" : "否") << endl;
    }

    // ===== 7. 典型錯誤模式 vs 正確模式 =====
    cout << "\n===== 7. 常見錯誤 vs 正確寫法 =====" << endl;
    {
        // 錯誤模式 1：erase 後繼續使用失效迭代器
        cout << "--- 錯誤模式 1：erase 後用失效迭代器 ---" << endl;
        cout << R"(
  // ❌ 錯誤！
  auto it = lst.begin();
  lst.erase(it);
  ++it;           // 未定義行為！it 已失效

  // ✔ 正確
  auto it = lst.begin();
  it = lst.erase(it);   // erase 回傳下一個有效迭代器
)" << endl;

        // 錯誤模式 2：迴圈中 erase + ++it
        cout << "--- 錯誤模式 2：迴圈中 erase + ++it ---" << endl;
        cout << R"(
  // ❌ 錯誤！
  for (auto it = lst.begin(); it != lst.end(); ++it) {
      if (*it == target) {
          lst.erase(it);   // it 失效
      }                    // 下一次 ++it → 未定義行為！
  }

  // ✔ 正確
  for (auto it = lst.begin(); it != lst.end(); ) {
      if (*it == target) {
          it = lst.erase(it);   // erase 回傳下一個
      } else {
          ++it;
      }
  }
)" << endl;

        // 錯誤模式 3：splice 後用迭代器在舊容器上遍歷
        cout << "--- 錯誤模式 3：splice 後用舊容器遍歷 ---" << endl;
        cout << R"(
  list<int> A = {1, 2, 3};
  list<int> B = {10, 20};
  auto it = B.begin();    // 指向 B 的 10

  A.splice(A.end(), B, it);  // 10 移到了 A

  // ❌ 概念錯誤（雖然 *it 仍然有效）：
  // 不能期望從 it 遍歷能走到 B 的其他元素
  // 因為 it 現在屬於 A 了

  // ✔ 正確認知：it 指向的節點現在在 A 中
  // ++it 會走到 A 中 10 之後的元素，不是 B 的元素
)" << endl;
    }

    // ===== 8. 實戰：安全地持有多個迭代器 =====
    cout << "===== 8. 實戰：多迭代器書籤系統 =====" << endl;
    {
        list<string> document = {
            "第一章", "第二章", "第三章",
            "第四章", "第五章", "第六章"
        };

        // 用迭代器做「書籤」
        auto bookmark1 = document.begin();
        advance(bookmark1, 1);     // 書籤1 → 第二章
        auto bookmark2 = document.begin();
        advance(bookmark2, 4);     // 書籤2 → 第五章

        cout << "書籤1: " << *bookmark1 << endl;
        cout << "書籤2: " << *bookmark2 << endl;

        // 在第三章前插入新章節 → 書籤不受影響
        auto it3 = document.begin();
        advance(it3, 2);
        document.insert(it3, "新增章節A");

        // 刪除第四章 → 書籤不受影響（因為書籤不指向第四章）
        auto it4 = document.begin();
        advance(it4, 3);   // 原本的第三章（因為插入了一個）
        advance(it4, 1);   // 第四章
        document.erase(it4);

        // 排序 → 書籤仍有效
        document.sort();

        // 書籤仍然指向原來的值
        cout << "操作後書籤1: " << *bookmark1 << endl;
        cout << "操作後書籤2: " << *bookmark2 << endl;

        print_list("最終文件", document);
        cout << "→ 經過 insert、erase、sort 後，書籤始終有效！" << endl;

        // 如果是 vector，上面任何一個操作都可能讓書籤失效
    }

    return 0;
}
