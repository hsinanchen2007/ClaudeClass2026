# 第二十六課：串列 List（二）— 新增、刪除、修改
# 本課程將介紹串列的新增、刪除和修改操作，讓你能夠更靈活地使用串列來管理資料。
# 在這個課程中，我們將學習如何使用 append() 方法來新增元素，使用 remove() 方法來刪除元素，
# 以及使用索引來修改元素的值。這些操作將幫助你更有效地處理串列中的資料。

# ═══════════════════════════════════════════════════════════════════════════
# 【面試題】串列 List（二）：新增、刪除、修改
# ───────────────────────────────────────────────────────────────────────────
# 🔥 Q1. list 的新增與刪除方法各有什麼差別？
#     答：新增——`append(x)` 在尾端加**一個**元素；
#     `extend(iterable)` 把元素**逐一**加入；`insert(i,x)` 是 O(n)。
#     刪除——`remove(v)` 依**值**刪**第一個**符合的
#     （實測 `[1,2,3,2].remove(2)` → `[1,3,2]`）；
#     `pop(i)` 依**索引**刪並**回傳**該元素；`clear()` 清空。
#     追問：`append([1,2])` 與 `extend([1,2])` 差在哪？
#     （前者塞入一個巢狀 list，後者攤平成兩個元素）
#
# 🔥 Q2. `remove()` 找不到元素會怎樣？
#     答：拋 **ValueError**，不是安靜略過。
#     所以刪除前應先 `if v in L:` 檢查，或用 try/except 包起來。
#     追問：`pop()` 對空 list 呢？（拋 IndexError；`pop()` 不給參數
#     預設刪最後一個）
#
# ⚠️ 陷阱. 一邊走訪 list 一邊刪除元素會怎樣？
#     答：會**跳過元素**。實測 `n=[2,4,6]` 在 for 迴圈中刪掉每個偶數，
#     結果是 **`[4]`** 而不是 `[]`——刪除讓後面元素前移，
#     但迭代器索引仍往前走，於是漏掉一個。
#     為什麼會錯：以為迭代器走的是「元素的快照」，
#     實際上它走的是「原 list + 目前索引」。
#     修法：對副本迭代 `for x in n[:]`，或用推導式重建
#     （實測 `[x for x in n if x % 2]` 正確）。dict 更嚴格——
#     迭代中改變大小會直接拋 RuntimeError。
# ═══════════════════════════════════════════════════════════════════════════

fruits = ["蘋果", "香蕉"]
print(f"原始：{fruits}")       # 輸出：['蘋果', '香蕉']

fruits.append("芒果")
print(f"加入芒果：{fruits}")   # 輸出：['蘋果', '香蕉', '芒果']

fruits.append("葡萄")
print(f"加入葡萄：{fruits}")   # 輸出：['蘋果', '香蕉', '芒果', '葡萄']

# 使用 append() 方法會把整個元素當成「一個元素」加入串列中：
# 例如，如果我們把一個串列當成元素加入另一個串列，會發生什麼情況呢？
# 下面的程式碼示範了這種情況： 
list_a = [1, 2, 3]
list_a.append([4, 5])
print(list_a)   # 輸出：[1, 2, 3, [4, 5]]  ← 整個串列被當成「一個元素」塞進去了！


# 如果想要把 list_b 的元素一個一個加入 list_a，可以使用 extend() 方法：
list_a = [1, 2, 3]
list_b = [4, 5, 6]

list_a.extend(list_b)
print(list_a)   # 輸出：[1, 2, 3, 4, 5, 6]  ← 每個元素獨立加入


# append：把整個東西當一個元素塞入
a = [1, 2, 3]
a.append([4, 5])
print(a)   # [1, 2, 3, [4, 5]]    長度變 4

# extend：把裡面的元素逐一加入
b = [1, 2, 3]
b.extend([4, 5])
print(b)   # [1, 2, 3, 4, 5]      長度變 5

# insert：在指定位置插入元素
colors = ["紅", "黃", "藍"]
print(f"原始：{colors}")         # ['紅', '黃', '藍']

# 在索引 1 的位置插入 "橙"
colors.insert(1, "橙")
print(f"插入橙：{colors}")       # ['紅', '橙', '黃', '藍']

# 在最前面插入（索引 0）
colors.insert(0, "白")
print(f"插入白：{colors}")       # ['白', '紅', '橙', '黃', '藍']


# + 運算子：串列相加會產生一個新的串列，原來的串列不會改變：
a = [1, 2]
b = [3, 4]
c = a + b
print(a)   # [1, 2]      ← 原串列不變
print(c)   # [1, 2, 3, 4] ← 新串列

# remove：刪除串列中第一個出現的指定元素
fruits = ["蘋果", "香蕉", "芒果", "香蕉", "葡萄"]
print(f"原始：{fruits}")

fruits.remove("香蕉")
print(f"刪除香蕉：{fruits}")
# 輸出：['蘋果', '芒果', '香蕉', '葡萄']
# 注意：只刪除了第一個 "香蕉"，第二個還在！


fruits = ["蘋果", "香蕉"]
# fruits.remove("草莓")   # ❌ ValueError: list.remove(x): x not in list
# 如果要刪除的元素不在串列中，會引發 ValueError 錯誤。
# 因此，在使用 remove() 方法之前，最好先檢查元素是否存在於串列中：
if "草莓" in fruits:
    fruits.remove("草莓")   # 這樣就不會引發錯誤了  

fruits = ["蘋果", "香蕉"]
target = "草莓"

if target in fruits:
    fruits.remove(target)
    print(f"已刪除 {target}")
else:
    print(f"{target} 不在串列中")
# 輸出：草莓 不在串列中


# del：根據索引刪除元素
numbers = [10, 20, 30, 40, 50]  
print(f"原始：{numbers}")   # [10, 20, 30, 40, 50]
del numbers[2]
print(f"刪除索引 2 的元素：{numbers}")   # [10, 20, 40, 50]
# del 也可以用來刪除整個串列：
# del numbers   # 這行會刪除整個串列，之後再訪問 numbers 就會引發 NameError 錯誤。

# pop：刪除並回傳指定位置的元素
fruits = ["蘋果", "香蕉", "芒果", "葡萄"]

# 不給參數：刪除並回傳最後一個
last = fruits.pop()
print(f"被刪除的：{last}")      # 輸出：葡萄
print(f"剩餘：{fruits}")        # 輸出：['蘋果', '香蕉', '芒果']

# 給索引：刪除並回傳該位置的元素
second = fruits.pop(1)
print(f"被刪除的：{second}")     # 輸出：香蕉
print(f"剩餘：{fruits}")         # 輸出：['蘋果', '芒果']


# del 和 pop 的差異：
# del 是根據索引刪除元素，但不會回傳被刪除的元素；pop 也是根據索引刪除元素，但會回傳被刪除的元素。
nums = [10, 20, 30, 40, 50, 60]

# 刪除單一元素
del nums[0]
print(nums)          # 輸出：[20, 30, 40, 50, 60]

# 刪除一段範圍（切片刪除）
del nums[1:3]
print(nums)          # 輸出：[20, 50, 60]

# 刪除整個串列（變數本身消失）
# del nums
# print(nums)        # ❌ NameError: name 'nums' is not defined

# clear：清空串列，但串列本身還在
nums = [1, 2, 3, 4, 5]
nums.clear()
print(nums)          # 輸出：[]（串列還在，但裡面是空的）


# 修改串列中的元素
# 串列是可變的（mutable），可以直接修改其中的元素：
# 透過索引來修改元素的值：
# 例如，將 fruits 串列中的 "香蕉" 修改為 "草莓"：
# 注意：修改元素的值不會改變串列的長度，只是改變了該位置的內容。
# 如果要修改的索引超出串列的範圍，會引發 IndexError 錯誤。
fruits = ["蘋果", "香蕉", "芒果"]
print(f"修改前：{fruits}")       # ['蘋果', '香蕉', '芒果']

fruits[1] = "草莓"
print(f"修改後：{fruits}")       # ['蘋果', '草莓', '芒果']


# 使用切片來修改一段範圍的元素：
# 切片修改的語法是：串列[開始索引:結束索引] = 新的元素列表
# 這裡的開始索引是包含的，結束索引是排除的（半開區間）。
# 切片修改可以一次修改多個元素，甚至可以改變串列的長度（如果替換的元素數量不同）。
nums = [1, 2, 3, 4, 5]
print(f"修改前：{nums}")

# 把索引 1~2 的元素替換
nums[1:3] = [20, 30]
print(f"等量替換：{nums}")       # [1, 20, 30, 4, 5]

# 替換的數量可以不同（串列長度會改變！）
nums[1:3] = [200, 300, 400, 500]
print(f"多量替換：{nums}")       # [1, 200, 300, 400, 500, 4, 5]


# 用切片插入元素（不刪除原有元素）
# 當開始和結束索引相同時，表示在該位置插入元素，而不刪除任何元素。
# 例如，在索引 2 的位置插入 3 和 4：
# 注意：這種方式會把新的元素插入到指定位置，原有的元素會往後移動。
# 如果插入的位置超出串列的範圍，會引發 IndexError 錯誤。
nums = [1, 2, 5, 6]
# 在索引 2 的位置插入，起始和結束相同 = 不刪除任何元素
nums[2:2] = [3, 4]
print(nums)          # 輸出：[1, 2, 3, 4, 5, 6]


# 串列是可變的：可以修改內容，記憶體位址不變
# id() 函式可以用來查看物件的記憶體位址，當我們修改串列的內容時，記憶體位址不會改變，
# 表示我們是在原有的串列上進行修改，而不是創建了一個新的串列。
fruits = ["蘋果", "香蕉"]
print(id(fruits))     # 例如：140234567890
fruits.append("芒果")
print(id(fruits))     # 還是：140234567890（同一個物件）

# 字串是不可變的：無法修改內容
name = "Hello"
# name[0] = "h"       # ❌ TypeError: 'str' object does not support item assignment

# 賦值只是建立別名（重要！）
# 當我們把一個串列賦值給另一個變數時，並不會創建一個新的串列，而是讓兩個變數指向同一個串列。
# 因此，對其中一個變數進行修改，另一個變數也會受到影響，因為它們指向的是同一個物件。
a = [1, 2, 3]
b = a           # b 不是副本，b 和 a 指向同一個串列！

b.append(4)
print(a)        # 輸出：[1, 2, 3, 4]  ← a 也被影響了！
print(b)        # 輸出：[1, 2, 3, 4]
print(a is b)   # 輸出：True（確實是同一個物件）


# 如何複製串列？（重要！）
# 如果想要創建一個新的串列，讓它和原來的串列內容相同，但又不互相影響，我們需要複製串列。
# 有幾種常見的方法可以複製串列：
# 1. 切片複製：使用切片語法 [:] 來創建一個新的串列，內容與原串列相同。
# 2. list() 建構：使用 list() 函式來創建一個新的串列，並把原串列作為參數傳入。
# 3. .copy() 方法：使用串列的 copy() 方法來創建一個新的串列，內容與原串列相同。
original = [1, 2, 3]

# 方法一：切片複製（最常用）
copy1 = original[:]

# 方法二：list() 建構
copy2 = list(original)

# 方法三：.copy() 方法
copy3 = original.copy()

# 現在修改副本不會影響原始串列
copy1.append(99)
print(original)   # [1, 2, 3]（不受影響）
print(copy1)      # [1, 2, 3, 99]


# ❌ 危險：一邊走訪一邊刪除
nums = [1, 2, 3, 4, 5, 6]
for n in nums:
    if n % 2 == 0:
        nums.remove(n)
print(nums)   # 輸出：[1, 3, 5, 6]  ← 6 沒被刪掉！


nums = [1, 2, 3]
# ❌ 錯誤寫法
result = nums.append(4)
print(result)    # 輸出：None（append 沒有回傳值！）
print(nums)      # [1, 2, 3, 4]（串列本身已經改了）

# ✅ 正確寫法
nums.append(4)   # 不需要接回傳值
print(nums)      # [1, 2, 3, 4]


# ✅ 安全做法一：建立新串列
nums = [1, 2, 3, 4, 5, 6]
odds = [n for n in nums if n % 2 != 0]
print(odds)   # [1, 3, 5]

# ✅ 安全做法二：走訪副本
nums = [1, 2, 3, 4, 5, 6]
for n in nums[:]:       # nums[:] 是副本
    if n % 2 == 0:
        nums.remove(n)
print(nums)   # [1, 3, 5]


# ❌ 以為是複製，其實是別名
a = [1, 2, 3]
b = a
b[0] = 999
print(a)   # [999, 2, 3]  ← a 被改了！

# ✅ 真正的複製
a = [1, 2, 3]
b = a.copy()
b[0] = 999
print(a)   # [1, 2, 3]    ← a 沒被影響
print(b)   # [999, 2, 3]  ← b 被改了！


# === 簡易待辦事項管理 ===

todos = []

while True:
    print("\n📋 待辦事項管理")
    print("1. 查看所有待辦")
    print("2. 新增待辦")
    print("3. 完成待辦（刪除）")
    print("4. 修改待辦")
    print("5. 離開")

    choice = input("請選擇功能（1-5）：")

    if choice == "1":
        if not todos:
            print("  🎉 沒有待辦事項！")
        else:
            print("  --- 待辦清單 ---")
            for i, task in enumerate(todos):
                print(f"  {i + 1}. {task}")

    elif choice == "2":
        task = input("  請輸入新待辦：")
        todos.append(task)
        print(f"  ✅ 已新增：{task}")

    elif choice == "3":
        if not todos:
            print("  ⚠️ 沒有可刪除的待辦！")
        else:
            for i, task in enumerate(todos):
                print(f"  {i + 1}. {task}")
            num = input("  請輸入要完成的編號：")
            if num.isdigit() and 1 <= int(num) <= len(todos):
                removed = todos.pop(int(num) - 1)
                print(f"  🎉 已完成：{removed}")
            else:
                print("  ❌ 無效的編號")

    elif choice == "4":
        if not todos:
            print("  ⚠️ 沒有可修改的待辦！")
        else:
            for i, task in enumerate(todos):
                print(f"  {i + 1}. {task}")
            num = input("  請輸入要修改的編號：")
            if num.isdigit() and 1 <= int(num) <= len(todos):
                new_task = input("  請輸入新內容：")
                old_task = todos[int(num) - 1]
                todos[int(num) - 1] = new_task
                print(f"  ✏️ 已將「{old_task}」修改為「{new_task}」")
            else:
                print("  ❌ 無效的編號")

    elif choice == "5":
        print("👋 再見！")
        break

    else:
        print("  ❌ 請輸入 1-5 的數字")


# 執行: python3 第 26 課：串列 List（二）— 新增、刪除、修改1.py

# === 預期輸出 (節錄) ===
# 原始：['蘋果', '香蕉']
# 加入芒果：['蘋果', '香蕉', '芒果']
# 加入葡萄：['蘋果', '香蕉', '芒果', '葡萄']
# [1, 2, 3, [4, 5]]
# [1, 2, 3, 4, 5, 6]
# [1, 2, 3, [4, 5]]
# [1, 2, 3, 4, 5]
# 原始：['紅', '黃', '藍']
# 插入橙：['紅', '橙', '黃', '藍']
# 插入白：['白', '紅', '橙', '黃', '藍']
# [1, 2]
# [1, 2, 3, 4]
# 原始：['蘋果', '香蕉', '芒果', '香蕉', '葡萄']
# 刪除香蕉：['蘋果', '芒果', '香蕉', '葡萄']
# 草莓 不在串列中
# 原始：[10, 20, 30, 40, 50]
# 刪除索引 2 的元素：[10, 20, 40, 50]
# 被刪除的：葡萄
# 剩餘：['蘋果', '香蕉', '芒果']
# 被刪除的：香蕉
# …（後略，完整輸出共 54 行）
# ⚠️ 本檔需要互動輸入（input()），以上為未輸入時的輸出；請自行執行並輸入資料。
# ⚠️ 上面的 id() 位址每次執行都不同，數值僅供參考；重點在於「兩個 id 是否相同」而非數字本身。
