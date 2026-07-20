# ═══════════════════════════════════════════════════════════════════════════
# 【面試題】迴圈 for
# ───────────────────────────────────────────────────────────────────────────
# 🔥 Q1. Python 的 for 與 C 的 for 有什麼本質差別？
#     答：Python 的 `for` 是 **for-each**——走訪一個**可迭代物件**，
#     不是「初始化／條件／遞增」的計數迴圈。它背後呼叫 `iter()` 取得迭代器，
#     再反覆呼叫 `next()` 直到 StopIteration。
#     追問：那需要索引怎麼辦？（用 `enumerate()`，不要用 `range(len(x))`）
#
# 🔥 Q2. `enumerate()` 與 `zip()` 怎麼用？
#     答：`enumerate(it, start=0)` 同時給索引與值
#     （實測 `list(enumerate(['a','b'],1))` → `[(1,'a'),(2,'b')]`）；
#     `zip()` 並行走訪多個可迭代物件，**以最短的為準截斷**
#     （實測 `zip([1,2,3],['a','b'])` 只產生 2 組，不會報錯）。
#     追問：不想被靜默截斷怎麼辦？（3.10+ 的 `zip(..., strict=True)`，
#     實測長度不符會拋 ValueError）
#
# ⚠️ 陷阱. 迴圈變數在迴圈結束後還存在嗎？
#     答：**存在**。實測 `for i in range(3): pass` 之後 `i` 仍是 **2**——
#     普通 for 迴圈**會洩漏**迴圈變數到外層作用域。
#     但推導式**不會**：實測 `x='outer'; [x for x in range(3)]` 之後 x 仍是 `'outer'`。
#     為什麼會錯：以為兩者都有自己的作用域。
#     實際上是 Python 3 給了推導式獨立作用域，普通 for 迴圈維持原樣。
# ═══════════════════════════════════════════════════════════════════════════

print("我不會再遲到了")
print("我不會再遲到了")
print("我不會再遲到了")
# ... 還要再寫 97 行 😱

# 迴圈可以幫助我們重複執行相同的程式碼，讓我們不需要寫很多行重複的程式碼
for i in range(100):
    print("我不會再遲到了")

# 迴圈也可以用來處理列表中的每個元素
fruits = ["蘋果", "香蕉", "櫻桃"]

for fruit in fruits:
    print(fruit)

word = "Python"

# 迴圈也可以用來處理字串
for char in word:
    print(char)

# 迴圈也可以用來處理數字
numbers = [10, 20, 30, 40, 50]

for num in numbers:
    print(f"{num} 的兩倍是 {num * 2}")

# 迴圈也可以用來處理不同類型的資料
mixed = ["信安", 25, True, 3.14]

for item in mixed:
    print(f"值：{item}，型別：{type(item).__name__}")


# # ✅ 好的命名：見名知意
# for student in students:
#     print(student)

# for number in numbers:
#     print(number)

# for letter in "Hello":
#     print(letter)

# # ❌ 不好的命名：看不出在遍歷什麼
# for x in students:
#     print(x)

# for abc in numbers:
#     print(abc)
 
for _ in range(3):
    print("重複三次！")
# 當我們不需要使用迴圈變數時，可以使用下劃線（_）作為變數名稱，表示這個變數是無意義的，不會被使用到。


scores = [85, 62, 93, 47, 78]

for score in scores:
    print(f"分數：{score}")
    if score >= 60:
        print("  → 及格 ✅")
    else:
        print("  → 不及格 ❌")
    print()  # 印一個空行做分隔
# 在這個範例中，我們使用迴圈來遍歷分數列表，並根據分數的值判斷是否及格。每次迴圈執行時，
# 我們都會印出分數和對應的評價，最後再印一個空行來分隔不同分數的輸出。


numbers = [10, 20, 30, 40, 50]
total = 0  # 先準備一個「容器」放結果

for num in numbers:
    total = total + num  # 也可以寫 total += num
    print(f"  加上 {num}，目前累計：{total}")

print(f"總和：{total}")
# 在這個範例中，我們使用迴圈來遍歷數字列表，並將每個數字加到 total 變數中，最後印出總和。



scores = [85, 62, 93, 47, 78, 55, 91]
pass_count = 0

for score in scores:
    if score >= 60:
        pass_count += 1

print(f"全班 {len(scores)} 人，及格 {pass_count} 人")
# 在這個範例中，我們使用迴圈來遍歷分數列表，並計算及格的人數。最後印出全班人數和及格人數。


numbers = [34, 67, 12, 89, 45, 23]
max_value = numbers[0]  # 先假設第一個是最大的

for num in numbers:
    if num > max_value:
        max_value = num  # 發現更大的，就更新

print(f"最大值是：{max_value}")
# 在這個範例中，我們使用迴圈來遍歷數字列表，並找出其中的最大值。初始時，我們假設第一個數字是最大的，
# 然後在迴圈中不斷比較並更新最大值，最後印出結果。


# 模式 1：累加器（Accumulator）
total = 0                    # 初始化容器
for num in numbers:
    total += num             # 每一輪累加
# 用途：求和、串接字串等

# # 模式 2：計數器（Counter）
# count = 0                    # 初始化計數
# for item in items:
#     if 某條件:
#         count += 1           # 符合條件就 +1
# # 用途：計算符合條件的數量

# # 模式 3：追蹤器（Tracker）
# best = items[0]              # 用第一個元素初始化
# for item in items:
#     if item > best:
#         best = item          # 發現更好的就更新
# # 用途：找最大值、最小值等


# for num in numbers     # ❌ SyntaxError: expected ':'
#     print(num)

# for num in numbers:
# print(num)             # ❌ IndentationError: expected an indented block


# for num in [1, 2, 3]:
#     print(num)

# print(num)  # ⚠️ 這行不會報錯，但 num 的值是最後一輪的 3
#             # 這是 Python 的特性，但不建議依賴這個行為


# # ❌ 忘記先定義 total
# for num in [1, 2, 3]:
#     total += num       # NameError: name 'total' is not defined

# # ✅ 正確
# total = 0
# for num in [1, 2, 3]:
#     total += num


students = ["小明", "小華", "小美", "小傑", "小芳"]
scores = [85, 62, 93, 47, 78]

total = 0
pass_count = 0
highest = scores[0]
highest_student = students[0]

print("=== 成績報告 ===\n")

for i in range(len(students)):
    name = students[i]
    score = scores[i]
    total += score

    if score >= 60:
        status = "及格 ✅"
        pass_count += 1
    else:
        status = "不及格 ❌"

    if score > highest:
        highest = score
        highest_student = name

    print(f"{name}：{score} 分 → {status}")

average = total / len(students)

print(f"\n=== 統計結果 ===")
print(f"全班平均：{average:.1f} 分")
print(f"及格人數：{pass_count} / {len(students)} 人")
print(f"最高分：{highest_student}（{highest} 分）")
# 在這個範例中，我們使用迴圈來同時處理學生姓名和分數，並計算總分、及格人數、最高分等統計資訊。最後印出成績報告和統計結果。

word = "Hello"
count = 0

for char in word:
    if char == char.upper():
        count += 1

print(count)
# 在這個範例中，我們使用迴圈來遍歷字串中的每個字元，並計算其中大寫字母的數量。最後印出結果。


# 執行: python3 第 20 課：迴圈 for — 重複執行的魔法1.py

# === 預期輸出 (節錄) ===
# 我不會再遲到了
# 我不會再遲到了
# 我不會再遲到了
# 我不會再遲到了
# 我不會再遲到了
# 我不會再遲到了
# 我不會再遲到了
# 我不會再遲到了
# 我不會再遲到了
# 我不會再遲到了
# 我不會再遲到了
# 我不會再遲到了
# 我不會再遲到了
# 我不會再遲到了
# 我不會再遲到了
# 我不會再遲到了
# 我不會再遲到了
# 我不會再遲到了
# 我不會再遲到了
# 我不會再遲到了
# …（後略，完整輸出共 160 行）
