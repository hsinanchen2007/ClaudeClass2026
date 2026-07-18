# 空串列
empty_list = []
print(empty_list)    # 輸出：[]

# 存放整數
numbers = [10, 20, 30, 40, 50]
print(numbers)       # 輸出：[10, 20, 30, 40, 50]

# 存放字串
fruits = ["蘋果", "香蕉", "芒果", "葡萄"]
print(fruits)        # 輸出：['蘋果', '香蕉', '芒果', '葡萄']

# 存放布林值
flags = [True, False, True, True]
print(flags)         # 輸出：[True, False, True, True]

# 串列可以混合各種型別
mixed = ["小明", 18, 175.5, True]
print(mixed)         # 輸出：['小明', 18, 175.5, True]
# 索引：  0       1     2       3


# 使用 list() 函式將其他資料型別轉為串列
# 將字串轉為串列（每個字元變成一個元素）
chars = list("Hello")
print(chars)         # 輸出：['H', 'e', 'l', 'l', 'o']

# 將 range 轉為串列
nums = list(range(1, 6))
print(nums)          # 輸出：[1, 2, 3, 4, 5]

# 將 tuple 轉為串列（後面會學到 tuple）
from_tuple = list((10, 20, 30))
print(from_tuple)    # 輸出：[10, 20, 30]

# 快速建立重複元素的串列
zeros = [0] * 5
print(zeros)         # 輸出：[0, 0, 0, 0, 0]

# 初始化一排置物櫃，全部放 "空"
lockers = ["空"] * 8
print(lockers)       # 輸出：['空', '空', '空', '空', '空', '空', '空', '空']

# 二維資料：3 位同學各 4 科成績
grades = [
    [85, 92, 78, 90],   # 第 0 位同學
    [88, 76, 95, 82],   # 第 1 位同學
    [91, 85, 89, 94]    # 第 2 位同學
]
print(grades)
# 輸出：[[85, 92, 78, 90], [88, 76, 95, 82], [91, 85, 89, 94]]


fruits = ["蘋果", "香蕉", "芒果", "葡萄", "西瓜"]
#  索引：    0       1       2       3       4
print(fruits[0])     # 輸出：蘋果
print(fruits[2])     # 輸出：芒果
print(fruits[4])     # 輸出：西瓜

# 反向索引：從串列末尾開始數，-1 是最後一個元素
print(fruits[-1])    # 輸出：西瓜
print(fruits[-3])    # 輸出：芒果

print(fruits[0])     # 輸出：蘋果（第一個元素）
print(fruits[1])     # 輸出：香蕉（第二個元素）
print(fruits[4])     # 輸出：西瓜（最後一個元素）
# 嘗試存取不存在的索引會發生錯誤
# print(fruits[5])  # IndexError: list index out of range

print(fruits[-1])    # 輸出：西瓜（倒數第 1 個）
print(fruits[-2])    # 輸出：葡萄（倒數第 2 個）
print(fruits[-5])    # 輸出：蘋果（倒數第 5 個 = 第 1 個）


fruits = ["蘋果", "香蕉", "芒果"]

# 串列只有 3 個元素（索引 0, 1, 2）
# print(fruits[3])     # ❌ IndexError: list index out of range
# print(fruits[-4])    # ❌ IndexError: list index out of range

nums = [0, 10, 20, 30, 40, 50, 60, 70, 80, 90]
#  索引: 0   1   2   3   4   5   6   7   8   9

print(nums[2:6])     # 輸出：[20, 30, 40, 50]（索引 2 到 5）
print(nums[0:3])     # 輸出：[0, 10, 20]（索引 0 到 2）
print(nums[7:10])    # 輸出：[70, 80, 90]（索引 7 到 9）

print(nums[:4])      # 輸出：[0, 10, 20, 30]（從頭到索引 3）
print(nums[6:])      # 輸出：[60, 70, 80, 90]（從索引 6 到尾）
print(nums[:])       # 輸出：[0, 10, 20, 30, 40, 50, 60, 70, 80, 90]（完整複製）

print(nums[::2])     # 輸出：[0, 20, 40, 60, 80]（每隔 2 個取一個）
print(nums[1::2])    # 輸出：[10, 30, 50, 70, 90]（從索引 1 開始，每隔 2 個）
print(nums[::3])     # 輸出：[0, 30, 60, 90]（每隔 3 個取一個）

print(nums[::-1])    # 輸出：[90, 80, 70, 60, 50, 40, 30, 20, 10, 0]
# 步長為 -1，表示從後往前走

# 切片的範圍可以超出串列長度，但不會報錯
# 即使範圍超出，切片也不會報錯
print(nums[5:100])   # 輸出：[50, 60, 70, 80, 90]（自動到結尾為止）
print(nums[100:200]) # 輸出：[]（超出範圍，回傳空串列）


fruits = ["蘋果", "香蕉", "芒果", "葡萄"]
print(len(fruits))   # 輸出：4

# 使用 in 和 not in 來檢查元素是否存在於串列中
fruits = ["蘋果", "香蕉", "芒果", "葡萄"]

print("香蕉" in fruits)       # 輸出：True
print("草莓" in fruits)       # 輸出：False
print("草莓" not in fruits)   # 輸出：True


inventory = ["筆記本", "原子筆", "橡皮擦", "尺"]
item = input("請輸入要查詢的商品：")

if item in inventory:
    print(f"✅ {item} 有庫存")
else:
    print(f"❌ {item} 目前缺貨")


# 串列相加（串列合併）
list_a = [1, 2, 3]
list_b = [4, 5, 6]
combined = list_a + list_b
print(combined)      # 輸出：[1, 2, 3, 4, 5, 6]


fruits = ["蘋果", "香蕉", "芒果", "葡萄"]

# 方法一：直接走訪元素
for fruit in fruits:
    print(f"我喜歡吃{fruit}")

# 輸出：
# 我喜歡吃蘋果
# 我喜歡吃香蕉
# 我喜歡吃芒果
# 我喜歡吃葡萄


# 方法二：用 enumerate() 同時取得索引和元素
# enumerate() 會回傳一個包含 (索引, 元素) 的元組
# i 是索引，fruit 是元素
# enumerate() 的第一個參數是要走訪的串列，第二個參數是索引的起始值（預設為 0）
for i, fruit in enumerate(fruits):
    print(f"第 {i} 號：{fruit}")

# 輸出：
# 第 0 號：蘋果
# 第 1 號：香蕉
# 第 2 號：芒果
# 第 3 號：葡萄

grades = [
    [85, 92, 78],    # 索引 0：小明的成績
    [88, 76, 95],    # 索引 1：小華的成績
    [91, 85, 89]     # 索引 2：小美的成績
]

# 取出小華的成績（整個子串列）
print(grades[1])         # 輸出：[88, 76, 95]

# 取出小美的第 2 科成績
print(grades[2][1])      # 輸出：85
#      ↑ 先選第 2 列  ↑ 再選第 1 欄

# 走訪所有成績
for i, student in enumerate(grades):
    avg = sum(student) / len(student)
    print(f"學生 {i}：成績 {student}，平均 {avg:.1f}")

# 輸出：
# 學生 0：成績 [85, 92, 78]，平均 85.0
# 學生 1：成績 [88, 76, 95]，平均 86.3
# 學生 2：成績 [91, 85, 89]，平均 88.3

# 索引和切片只能用方括號，不能用小括號
colors = ["紅", "橙", "黃"]
print(colors[1])   # 輸出：橙（不是「紅」！）
print(colors[0])   # 輸出：紅（第一個元素的索引是 0）

# 錯誤示範：使用小括號會被當成函式呼叫，導致 TypeError
nums = [10, 20, 30]
# print(nums(1))   # ❌ TypeError: 'list' object is not callable
print(nums[1])     # ✅ 正確：用方括號

# 切片的「不包含結束」容易搞混
# 切片的結束索引是「不包含」的，所以切片 nums[1:3] 會包含索引 1 和 2，但不包含索引 3
# 這裡的切片範圍是從索引 1 到索引 2（不包含索引 3）
data = [10, 20, 30, 40, 50]
print(data[1:3])   # 輸出：[20, 30]（不包含索引 3 的 40！）


data = [10, 20, 30]
# data[10]        # ❌ IndexError
print(data[10:20]) # ✅ 回傳 []（切片不報錯）


# === 學生成績管理系統 ===

# 建立學生與成績資料
students = ["小明", "小華", "小美", "小強", "小芳"]
scores   = [85, 92, 78, 96, 88]

print("=" * 35)
print("📊 學生成績管理系統")
print("=" * 35)

# 1. 顯示所有成績
print("\n【所有學生成績】")
for i, name in enumerate(students):
    print(f"  {i + 1}. {name}：{scores[i]} 分")

# 2. 計算統計資料
total = sum(scores)
avg = total / len(scores)
highest = max(scores)
lowest = min(scores)

print(f"\n【統計資料】")
print(f"  全班總分：{total}")
print(f"  全班平均：{avg:.1f}")
print(f"  最高分：{highest}（{students[scores.index(highest)]}）")
print(f"  最低分：{lowest}（{students[scores.index(lowest)]}）")

# 3. 找出高於平均的同學
print(f"\n【高於平均（{avg:.1f}）的同學】")
for i, name in enumerate(students):
    if scores[i] > avg:
        print(f"  ✅ {name}：{scores[i]} 分")

# 4. 成績排名（用切片建立副本排序，不影響原串列）
sorted_pairs = sorted(zip(scores, students), reverse=True)
print(f"\n【成績排名】")
for rank, (score, name) in enumerate(sorted_pairs, 1):
    print(f"  第 {rank} 名：{name}（{score} 分）")






