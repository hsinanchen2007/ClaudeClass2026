# python tuple 範例
# 空元組

# ═══════════════════════════════════════════════════════════════════════════
# 【面試題】元組 Tuple
# ───────────────────────────────────────────────────────────────────────────
# 🔥 Q1. list 與 tuple 該怎麼選？
#     答：tuple 用於**固定、異質、有語意的紀錄**（座標 `(x, y)`、
#     函式的多回傳值）以及**需要 hash 的場合**（當 dict key、放進 set）；
#     list 用於**同質、數量會變**的集合。tuple 較省記憶體、建立也較快。
#     追問：為什麼 tuple 能當 dict key 而 list 不行？
#     （因為可變物件的 hash 會隨內容改變，Python 直接禁止它們 hashable）
#
# 🔥 Q2. 單元素 tuple 怎麼寫？
#     答：**`(1,)`**——關鍵是**逗號**，不是括號。
#     實測 `type((1))` 是 **int**、`type((1,))` 才是 **tuple**。
#     其實連括號都可以省，`x = 1,` 一樣是 tuple。
#     追問：這和函式回傳多值有什麼關係？
#     （`return a, b` 其實就是回傳一個 tuple，呼叫端再拆包）
#
# ⚠️ 陷阱. tuple 真的不可變嗎？`t = (1, [2,3], 4)` 之後 `t[1].append(99)`？
#     答：**會成功**！實測 `t` 變成 `(1, [2, 3, 99], 4)`。
#     tuple 的不可變性指的是「**它持有的參考不能換**」，
#     內部若是可變物件，該物件本身仍可被就地修改。
#     為什麼會錯：把「不可變」理解成「裡面的東西都凍結了」。
#     實際上凍結的只有那一排參考。連帶結果：含 list 的 tuple
#     **不可 hash**（實測 `hash((1,[2]))` 拋 TypeError），不能當 dict key。
# ═══════════════════════════════════════════════════════════════════════════

empty = ()
print(empty)          # 輸出：()
print(type(empty))    # 輸出：<class 'tuple'>

# 存放多個元素
colors = ("紅", "橙", "黃", "綠", "藍")
print(colors)         # 輸出：('紅', '橙', '黃', '綠', '藍')

# 混合型別
person = ("小明", 18, 175.5, True)
print(person)         # 輸出：('小明', 18, 175.5, True)

# 單元素元組
single = ("只有一個元素",)  
print(single)         # 輸出：('只有一個元素',)
print(type(single))   # 輸出：<class 'tuple'>

# 不加逗號會被當成字串
not_tuple = ("只有一個元素")
print(not_tuple)      # 輸出：只有一個元素
print(type(not_tuple)) # 輸出：<class 'str'>


# ❌ 這不是元組，只是一個加了括號的字串！
not_tuple = ("hello")
print(type(not_tuple))   # <class 'str'>

# ✅ 一個元素的元組，必須加逗號
single = ("hello",)
print(type(single))      # <class 'tuple'>
print(single)            # ('hello',)

# 其實逗號才是關鍵，括號可以省略
single2 = "hello",
print(type(single2))     # <class 'tuple'>


# Python 看到逗號分隔的值，自動視為元組
coordinates = 25.03, 121.56
print(coordinates)        # (25.03, 121.56)
print(type(coordinates))  # <class 'tuple'>

# 多個值
data = "小明", 18, "台北"
print(data)               # ('小明', 18, '台北')
print(type(data))         # <class 'tuple'>


# 從串列轉換
from_list = tuple([1, 2, 3, 4, 5])
print(from_list)          # (1, 2, 3, 4, 5)

# 從字串轉換（每個字元變成一個元素）
from_str = tuple("Hello")
print(from_str)           # ('H', 'e', 'l', 'l', 'o')

# 從 range 轉換
from_range = tuple(range(1, 6))
print(from_range)         # (1, 2, 3, 4, 5)


# 元組的索引
seasons = ("春", "夏", "秋", "冬")
#  正向索引：  0     1     2     3
#  負向索引： -4    -3    -2    -1

print(seasons[0])      # 春
print(seasons[2])      # 秋
print(seasons[-1])     # 冬
print(seasons[-3])     # 夏


# 元組的切片
nums = (0, 10, 20, 30, 40, 50, 60, 70, 80, 90)

print(nums[2:6])       # (20, 30, 40, 50)
print(nums[:4])        # (0, 10, 20, 30)
print(nums[6:])        # (60, 70, 80, 90)
print(nums[::2])       # (0, 20, 40, 60, 80)
print(nums[::-1])      # (90, 80, 70, 60, 50, 40, 30, 20, 10, 0)


# 元組的走訪
colors = ("紅", "橙", "黃", "綠", "藍")

# 直接走訪
for color in colors:
    print(color, end=" ")
# 輸出：紅 橙 黃 綠 藍

print()

# 帶索引走訪
for i, color in enumerate(colors):
    print(f"第 {i} 個：{color}")


# 元組是不可變的
fruits = ("蘋果", "香蕉", "芒果")

# ❌ 不能修改
# fruits[1] = "草莓"
# TypeError: 'tuple' object does not support item assignment


# ❌ 不能刪除
# del fruits[1]
# TypeError: 'tuple' object doesn't support item deletion

# ✅ 可以重新賦值整個元組
fruits = ("蘋果", "草莓", "芒果")
print(fruits)  # ('蘋果', '草莓', '芒果')

fruits = ("蘋果", "香蕉", "芒果")

# ❌ 元組沒有這些方法
# fruits.append("葡萄")     # AttributeError
# fruits.remove("香蕉")     # AttributeError
# fruits.insert(0, "草莓")  # AttributeError
# fruits.pop()              # AttributeError
# del fruits[0]             # TypeError


# 如果需要修改元組的內容，可以先轉成串列，修改後再轉回元組
# 這樣就能「變相」修改元組了
original = ("蘋果", "香蕉", "芒果")
print(f"原始：{original}")

# 步驟 1：轉成串列
temp = list(original)

# 步驟 2：修改串列
temp[1] = "草莓"
temp.append("葡萄")

# 步驟 3：轉回元組
modified = tuple(temp)
print(f"修改後：{modified}")
# 輸出：修改後：('蘋果', '草莓', '芒果', '葡萄')


# 元組的串接和重複
a = (1, 2, 3)
b = (4, 5, 6)
c = a + b
print(c)      # (1, 2, 3, 4, 5, 6)

d = a * 3
print(d)      # (1, 2, 3, 1, 2, 3, 1, 2, 3)


# 元組的解包
# 把元組的元素一次賦值給多個變數
person = ("小明", 18, "台北")

name, age, city = person
print(name)     # 小明
print(age)      # 18
print(city)     # 台北


# 傳統做法（需要暫時變數）
a = 10
b = 20
temp = a
a = b
b = temp
print(a, b)     # 20 10

# Python 拆包做法（一行搞定！）
a = 10
b = 20
a, b = b, a
print(a, b)     # 20 10


# 拆包也可以用在函式回傳多個值的情況
nums = (1, 2, 3, 4, 5, 6, 7)

first, second, *rest = nums
print(first)     # 1
print(second)    # 2
print(rest)      # [3, 4, 5, 6, 7]（注意：是串列！）

# 取頭尾，中間全部收集
first, *middle, last = nums
print(first)     # 1
print(middle)    # [2, 3, 4, 5, 6]
print(last)      # 7

# 只取最後兩個
*others, second_last, last = nums
print(others)       # [1, 2, 3, 4, 5]
print(second_last)  # 6
print(last)         # 7


# 元組的走訪時也可以直接拆包
students = [
    ("小明", 85),
    ("小華", 92),
    ("小美", 78),
    ("小強", 96)
]

# 走訪時直接拆包
for name, score in students:
    status = "✅ 優秀" if score >= 90 else "📘 普通"
    print(f"{name}：{score} 分 {status}")

# 輸出：
# 小明：85 分 📘 普通
# 小華：92 分 ✅ 優秀
# 小美：78 分 📘 普通
# 小強：96 分 ✅ 優秀


# 函數可以回傳元組，呼叫端拆包接收
def min_max(numbers):
    return min(numbers), max(numbers)

data = [23, 45, 12, 67, 34]
lo, hi = min_max(data)
print(f"最小：{lo}，最大：{hi}")
# 輸出：最小：12，最大：67


# 元組的內建方法, count()
nums = (1, 3, 5, 3, 7, 3, 9)
print(nums.count(3))     # 3
print(nums.count(5))     # 1
print(nums.count(100))   # 0


# 元組的內建方法, index()
colors = ("紅", "橙", "黃", "綠", "藍")
print(colors.index("黃"))    # 2
print(colors.index("紅"))    # 0
# colors.index("紫")         # ❌ ValueError


# 元組的優點：不可變性
# 一週的天數，這是固定事實，不應該被改
DAYS_OF_WEEK = ("一", "二", "三", "四", "五", "六", "日")

# 如果用串列，某段程式碼可能不小心改了它
# days[0] = "Monday"   # 用元組就不怕這種意外


# 元組的優點：可以當字典的 key
# ✅ 元組可以當字典的 key（因為不可變）
location_names = {
    (25.03, 121.56): "台北",
    (22.63, 120.30): "高雄"
}
print(location_names[(25.03, 121.56)])   # 台北

# ❌ 串列不能當字典的 key（因為可變）
# bad_dict = {[25.03, 121.56]: "台北"}
# TypeError: unhashable type: 'list'


# 元組的優點：佔用空間較小
import sys

my_list = [1, 2, 3, 4, 5]
my_tuple = (1, 2, 3, 4, 5)

# 同樣的元素，串列佔用的空間比元組大
# 因為串列需要額外的空間來管理可變性（如增刪元素），而元組不需要這些額外的管理結構。
# 因此，元組在內存中佔用的空間較小，這也是它的一個優點。
# 注意：實際佔用空間會因 Python 版本和實現而異，但通常元組比串列更節省空間。
# 在 CPython 中，元組的內部結構比串列更簡單，因為它不需要支持動態增刪元素的功能。
# 因此，元組的內部結構更緊湊，佔用的內存也更少。
# sys.getsizeof() 函數可以用來查看物件佔用的內存大小，通常會發現同樣元素的元組佔用的空間比串列小。
print(f"串列佔用：{sys.getsizeof(my_list)} bytes")   # 較大
print(f"元組佔用：{sys.getsizeof(my_tuple)} bytes")   # 較小


# 元組常用來表示「一筆紀錄」的欄位組合
student = ("小明", 18, "資工系")     # 姓名、年齡、科系

# 串列常用來表示「同類資料的集合」
scores = [85, 92, 78, 90]           # 一堆成績


# 注意：單元素元組必須加逗號，否則會被當成普通資料型別
a = (42)
print(type(a))    # <class 'int'> ← 不是元組！

b = (42,)
print(type(b))    # <class 'tuple'> ← 這才是元組


# 元組是不可變的，不能修改元素
t = (1, 2, 3)
# t[0] = 10      # ❌ TypeError: 'tuple' object does not support item assignment
# t.append(4)    # ❌ AttributeError: 'tuple' object has no attribute 'append'


point = (10, 20, 30)

# ❌ 變數數量必須與元素數量一致
# x, y = point
# ValueError: too many values to unpack (expected 2)

# ✅ 正確
x, y, z = point

# ✅ 或者用 * 收集多餘的
x, *rest = point
print(x)       # 10
print(rest)    # [20, 30]


# 元組本身不可變，但裡面的串列是可變的！
tricky = (1, 2, [3, 4])

# ❌ 不能替換元組的元素
# tricky[2] = [5, 6]   # TypeError

# ⚠️ 但可以修改裡面那個串列的內容！
tricky[2].append(5)
print(tricky)          # (1, 2, [3, 4, 5])



# === 學生資料管理系統 ===
# 每位學生的資料是元組（不可修改的記錄）
# 所有學生存在串列中（可以新增/移除學生）

students = [
    ("小明", "資工系", 85, 92, 78),
    ("小華", "電機系", 90, 88, 95),
    ("小美", "機械系", 76, 82, 91),
    ("小強", "資工系", 95, 89, 93),
    ("小芳", "電機系", 68, 74, 80)
]

print("=" * 50)
print("📊 學生資料管理系統")
print("=" * 50)

# 1. 顯示所有學生資料（使用拆包）
print("\n【全部學生】")
print(f"{'姓名':^6}{'科系':^8}{'國文':^6}{'英文':^6}{'數學':^6}{'平均':^8}")
print("-" * 42)

for name, dept, ch, en, math in students:
    avg = (ch + en + math) / 3
    print(f"{name:^6}{dept:^8}{ch:^6}{en:^6}{math:^6}{avg:^8.1f}")

# 2. 找出各科最高分
subjects = ("國文", "英文", "數學")
print("\n【各科最高分】")
for i, subject in enumerate(subjects):
    # 每科成績在元組的第 2+i 個位置
    best_score = max(students, key=lambda s: s[2 + i])
    name = best_score[0]
    score = best_score[2 + i]
    print(f"  {subject}：{name}（{score} 分）")

# 3. 依照平均分數排名
print("\n【平均分數排名】")
ranked = sorted(students, key=lambda s: sum(s[2:]) / 3, reverse=True)
for rank, (name, dept, ch, en, math) in enumerate(ranked, 1):
    avg = (ch + en + math) / 3
    print(f"  第 {rank} 名：{name}（{dept}）— 平均 {avg:.1f}")

# 4. 篩選特定科系
target_dept = "資工系"
print(f"\n【{target_dept}的學生】")
cs_students = [s for s in students if s[1] == target_dept]
for name, dept, ch, en, math in cs_students:
    print(f"  {name}：國{ch} 英{en} 數{math}")


# 執行: python3 第 28 課：元組 Tuple — 不可變的串列1.py

# === 預期輸出 (節錄) ===
# ()
# <class 'tuple'>
# ('紅', '橙', '黃', '綠', '藍')
# ('小明', 18, 175.5, True)
# ('只有一個元素',)
# <class 'tuple'>
# 只有一個元素
# <class 'str'>
# <class 'str'>
# <class 'tuple'>
# ('hello',)
# <class 'tuple'>
# (25.03, 121.56)
# <class 'tuple'>
# ('小明', 18, '台北')
# <class 'tuple'>
# (1, 2, 3, 4, 5)
# ('H', 'e', 'l', 'l', 'o')
# (1, 2, 3, 4, 5)
# 春
# …（後略，完整輸出共 99 行）
