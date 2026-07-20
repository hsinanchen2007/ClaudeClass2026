# 資料結構比較與選擇 — 什麼時候用什麼？

# ═══════════════════════════════════════════════════════════════════════════
# 【面試題】資料結構的比較與選擇
# ───────────────────────────────────────────────────────────────────────────
# 🔥 Q1. list、tuple、dict、set 的核心差異？
#     答：
#       list  `[]`     可變、有序、可重複、可索引、不可 hash
#       tuple `()`     不可變、有序、可重複、可索引、元素全不可變時可 hash
#       dict  `{k:v}`  可變、**插入序（3.7+ 語言保證）**、key 不可重複
#       set   `set()`  可變、**無序**、元素不重複且必須 hashable
#     追問：空 set 為什麼不能寫 `{}`？（那是空 dict，只能用 `set()`）
#
# 🔥 Q2. 為什麼大 list 的 `in` 很慢，set 卻很快？
#     答：list 的 `in` 是**線性掃描 O(n)**；
#     set／dict 的 `in` 是**雜湊查找、平均 O(1)**。
#     資料量大又要頻繁做成員測試時，先 `s = set(lst)` 再查會快非常多。
#     追問：那建 set 的成本值得嗎？（建立本身是 O(n)——
#     只查一次不值得，要查很多次才划算）
#
# ⚠️ 陷阱. 需要「去重」時直接用 set 有什麼副作用？
#     答：**順序會丟失**。set 不保證任何順序，
#     實測本機 `list(set([3,1,2,1]))` 得 `[1,2,3]` 看似排序，
#     那只是小整數 hash 的巧合，字串元素還會隨 hash randomization 變動。
#     為什麼會錯：只看到「去重」這個需求，忽略了「原順序」也是需求。
#     要保序去重請用 `list(dict.fromkeys(seq))`——
#     實測 `[3,1,2,1]` → `[3,1,2]`，靠的是 dict 3.7+ 的插入序保證。
# ═══════════════════════════════════════════════════════════════════════════

my_list  = [1, 2, 3, 2, 1]        # 串列
my_tuple = (1, 2, 3, 2, 1)        # 元組
my_dict  = {"a": 1, "b": 2}       # 字典
my_set   = {1, 2, 3}              # 集合


# 比較資料結構的效率：串列 vs 集合 vs 字典
import time

# 建立大量資料
size = 1_000_000

big_list = list(range(size))
big_set = set(range(size))
big_dict = {i: True for i in range(size)}

target = 999_999   # 找最後一個元素（最差情況）

# 串列的 in：O(n) — 要從頭找到尾
start = time.time()
result = target in big_list
list_time = time.time() - start

# 集合的 in：O(1) — 直接算出位置
start = time.time()
result = target in big_set
set_time = time.time() - start

# 字典的 in：O(1) — 跟集合一樣快
start = time.time()
result = target in big_dict
dict_time = time.time() - start

print(f"串列 in：{list_time:.6f} 秒")
print(f"集合 in：{set_time:.6f} 秒")
print(f"字典 in：{dict_time:.6f} 秒")


# 比較資料結構的記憶體使用：串列 vs 元組 vs 集合 vs 字典
import sys

data = list(range(1000))

as_list  = list(data)
as_tuple = tuple(data)
as_set   = set(data)
as_dict  = {x: None for x in data}

print(f"串列：{sys.getsizeof(as_list):>6} bytes")
print(f"元組：{sys.getsizeof(as_tuple):>6} bytes")
print(f"集合：{sys.getsizeof(as_set):>6} bytes")
print(f"字典：{sys.getsizeof(as_dict):>6} bytes")


# 實際應用：文字分析中的資料結構選擇
text = "the cat sat on the mat the cat"
words = text.split()
# 用串列來存所有單詞（有重複）
word_list = words
# 用集合來存獨特單詞（無重複）
unique_words = set(words)
# 用字典來計數每個單詞出現的次數
word_count = {}
for word in words:
    if word in word_count:
        word_count[word] += 1
    else:
        word_count[word] = 1
print("所有單詞（串列）：", word_list)
print("獨特單詞（集合）：", unique_words)
print("單詞計數（字典）：", word_count)

# 結論：選擇資料結構的原則
# - 如果需要有序且可修改的集合，使用串列。  
# - 如果需要有序但不可修改的集合，使用元組。  
# - 如果需要快速查找且不關心順序，使用集合。
# - 如果需要鍵值對結構，使用字典。


# 練習：實作一個簡單的文字分析工具，統計每個單詞出現的次數，並列出獨特單詞。
# 方法一：用串列（笨拙）
# 這種方法效率很差，因為每次檢查單詞是否已經存在都要遍歷整個串列。
unique_words = []
counts = []

for word in words:
    if word in unique_words:
        idx = unique_words.index(word)
        counts[idx] += 1
    else:
        unique_words.append(word)
        counts.append(1)

for i in range(len(unique_words)):
    print(f"{unique_words[i]}: {counts[i]}")


# 方法二：用字典（正確選擇）
counter = {}
for word in words:
    counter[word] = counter.get(word, 0) + 1

for word, count in counter.items():
    print(f"{word}: {count}")


# 方法三：用集合輔助（只要唯一單字，不要計數）
unique = set(words)
print(f"共 {len(unique)} 個不重複單字：{unique}")


# 練習：比較兩個串列的元素，找出它們的交集、差集和聯集。
list_a = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
list_b = [8, 9, 10, 11, 12, 13, 14, 15]


# 方法一：雙重迴圈（O(n²)）
common = []
for a in list_a:
    for b in list_b:
        if a == b:
            common.append(a)
print(common)   # [8, 9, 10]


# 方法二：串列 + in（O(n²)）
common = [x for x in list_a if x in list_b]
print(common)   # [8, 9, 10]


# 方法三：集合交集（O(n)）
common = set(list_a) & set(list_b)
print(common)   # {8, 9, 10}


# 方法四：集合差集（O(n)）
diff_a = set(list_a) - set(list_b)
original = [1, 2, 3, 2, 1]


# 串列 → 其他
to_tuple = tuple(original)       # (1, 2, 3, 2, 1)
to_set   = set(original)         # {1, 2, 3}
# 串列不能直接轉字典（需要鍵值對）

# 元組 → 其他
t = (1, 2, 3, 2, 1)
to_list  = list(t)               # [1, 2, 3, 2, 1]
to_set   = set(t)                # {1, 2, 3}

# 集合 → 其他
s = {3, 1, 2}
to_list  = list(s)               # [1, 2, 3]（順序不保證）
to_tuple = tuple(s)              # (1, 2, 3)（順序不保證）

# 字典 → 其他
d = {"a": 1, "b": 2, "c": 3}
to_list_keys   = list(d)              # ['a', 'b', 'c']（只有 key）
to_list_values = list(d.values())     # [1, 2, 3]
to_list_items  = list(d.items())      # [('a', 1), ('b', 2), ('c', 3)]
to_set_keys    = set(d)               # {'a', 'b', 'c'}



# 去重但保留串列格式
data = [3, 1, 4, 1, 5, 9, 2, 6, 5]
unique_sorted = sorted(set(data))
print(unique_sorted)   # [1, 2, 3, 4, 5, 6, 9]

# 字典依值排序 → 回傳排序後的 (key, value) 串列
scores = {"小明": 85, "小華": 92, "小美": 78}
ranked = sorted(scores.items(), key=lambda x: x[1], reverse=True)
print(ranked)   # [('小華', 92), ('小明', 85), ('小美', 78)]

# 兩個串列 → 字典
keys = ["name", "age", "city"]
vals = ["小明", 18, "台北"]
d = dict(zip(keys, vals))
print(d)   # {'name': '小明', 'age': 18, 'city': '台北'}

# 字典的 key 和 value 互換
original = {"a": 1, "b": 2, "c": 3}
flipped = {v: k for k, v in original.items()}
print(flipped)   # {1: 'a', 2: 'b', 3: 'c'}



# 每一列是一個字典（像資料庫的一筆記錄）
students = [
    {"name": "小明", "dept": "資工", "score": 85},
    {"name": "小華", "dept": "電機", "score": 92},
    {"name": "小美", "dept": "資工", "score": 78},
    {"name": "小強", "dept": "機械", "score": 96}
]

# 查找、篩選、排序都很直覺
cs_students = [s for s in students if s["dept"] == "資工"]
top_student = max(students, key=lambda s: s["score"])
print(f"最高分：{top_student['name']}（{top_student['score']}）")



# 每個科系對應一個集合（自動去重）
dept_members = {
    "資工": {"小明", "小美", "小偉"},
    "電機": {"小華", "小芳"},
    "機械": {"小強", "小玲"}
}

# 快速檢查某人在哪個科系
def find_dept(name):
    for dept, members in dept_members.items():
        if name in members:      # 集合的 in 是 O(1)
            return dept
    return "查無此人"

print(find_dept("小美"))   # 資工
print(find_dept("小志"))   # 查無此人



# 訂單按日期分組
orders = [
    ("2024-01-15", "商品A", 100),
    ("2024-01-15", "商品B", 200),
    ("2024-01-16", "商品C", 150),
    ("2024-01-16", "商品A", 100),
    ("2024-01-17", "商品B", 200)
]

daily = {}
for date, item, price in orders:
    daily.setdefault(date, []).append((item, price))

for date, items in daily.items():
    total = sum(price for _, price in items)
    print(f"{date}：{len(items)} 筆訂單，合計 NT${total}")
# 2024-01-15：2 筆訂單，合計 NT$300
# 2024-01-16：2 筆訂單，合計 NT$250
# 2024-01-17：1 筆訂單，合計 NT$200


# 用座標元組當 key，記錄地圖上每個位置的資訊
game_map = {
    (0, 0): "起點",
    (1, 0): "草地",
    (2, 0): "河流",
    (0, 1): "森林",
    (1, 1): "村莊",
    (2, 1): "山脈"
}

player_pos = (1, 1)
print(f"玩家位置：{game_map.get(player_pos, '未知區域')}")
# 玩家位置：村莊



# # ❌ 每次 in 都是 O(n)
# blacklist = ["spam1", "spam2", "spam3", ...]   # 假設有 10000 個
# for email in all_emails:                       # 假設有 50000 封
#     if email in blacklist:                     # O(n) × O(m) = O(nm)
#         block(email)

# # ✅ 改用集合，in 變成 O(1)
# blacklist = {"spam1", "spam2", "spam3", ...}   # 集合
# for email in all_emails:
#     if email in blacklist:                     # O(1) × O(m) = O(m)
#         block(email)


# ❌ 每次查找都要遍歷
students = [("小明", 85), ("小華", 92), ("小美", 78)]

def find_score(name):
    for n, s in students:
        if n == name:
            return s
    return None

# ✅ 用字典，一步到位
students = {"小明": 85, "小華": 92, "小美": 78}
score = students.get("小華")   # O(1)
print(score)   # 92



# ❌ 用字典模擬索引存取（多此一舉）
data = {0: "a", 1: "b", 2: "c", 3: "d"}
print(data[2])   # c

# ✅ 直接用串列
data = ["a", "b", "c", "d"]
print(data[2])   # c


# ❌ 用迴圈 + in 去重（O(n²)）
data = [1, 2, 3, 1, 2, 4, 5, 3]
unique = []
for x in data:
    if x not in unique:    # O(n) 每次
        unique.append(x)

# ✅ 不需要順序時，直接用集合
unique = list(set(data))

# ✅ 需要保序時，用集合輔助
seen = set()
unique = []
for x in data:
    if x not in seen:      # O(1) 每次
        seen.add(x)
        unique.append(x)



# === 課程管理系統 ===
# 展示四種資料結構的組合搭配

# --- 資料定義 ---
# 串列 + 字典：學生資料（有序的記錄集合）
students = [
    {"id": "S001", "name": "Alice", "dept": "資工"},
    {"id": "S002", "name": "Bob",   "dept": "電機"},
    {"id": "S003", "name": "Cindy", "dept": "資工"},
    {"id": "S004", "name": "David", "dept": "機械"},
    {"id": "S005", "name": "Eva",   "dept": "電機"}
]

# 元組串列：選課紀錄（學生ID, 課程名）— 元組表示固定的一筆紀錄
enrollments = [
    ("S001", "Python"), ("S001", "資料結構"), ("S001", "線性代數"),
    ("S002", "Java"),   ("S002", "Python"),   ("S002", "電路學"),
    ("S003", "Python"), ("S003", "資料結構"), ("S003", "機器學習"),
    ("S004", "材料力學"), ("S004", "Python"),
    ("S005", "電路學"), ("S005", "Java"),     ("S005", "Python")
]

# 集合：可用課程清單（不重複）
available_courses = {e[1] for e in enrollments}

print("=" * 55)
print("📚 課程管理系統")
print("=" * 55)

# === 1. 字典查表：快速找學生 ===
student_lookup = {s["id"]: s for s in students}

sid = "S003"
info = student_lookup.get(sid)
if info:
    print(f"\n🔍 查詢 {sid}：{info['name']}（{info['dept']}）")

# === 2. 字典 + 集合：每位學生選了哪些課 ===
student_courses = {}
for sid, course in enrollments:
    student_courses.setdefault(sid, set()).add(course)

print("\n【各學生選課】")
for sid, courses in student_courses.items():
    name = student_lookup[sid]["name"]
    print(f"  {name}: {', '.join(sorted(courses))}")

# === 3. 集合運算：找共同選課 ===
alice_courses = student_courses["S001"]
cindy_courses = student_courses["S003"]

common = alice_courses & cindy_courses
only_alice = alice_courses - cindy_courses
only_cindy = cindy_courses - alice_courses

print(f"\n【Alice vs Cindy 選課比較】")
print(f"  共同選課：{common}")
print(f"  只有 Alice：{only_alice}")
print(f"  只有 Cindy：{only_cindy}")

# === 4. 字典計數：各課程修課人數 ===
course_count = {}
for _, course in enrollments:
    course_count[course] = course_count.get(course, 0) + 1

print("\n【課程熱門度排行】")
ranked = sorted(course_count.items(), key=lambda x: x[1], reverse=True)
for rank, (course, count) in enumerate(ranked, 1):
    bar = "█" * (count * 3)
    print(f"  {rank}. {course:8s} {count} 人 {bar}")

# === 5. 字典 + 串列：按科系分組 ===
dept_groups = {}
for s in students:
    dept_groups.setdefault(s["dept"], []).append(s["name"])

print("\n【各科系學生】")
for dept, names in dept_groups.items():
    print(f"  {dept}：{', '.join(names)}")

# === 6. 集合：找出沒人選的課（如果有其他課的話）
all_possible = {"Python", "Java", "資料結構", "線性代數", "機器學習",
                "電路學", "材料力學", "演算法", "作業系統"}
not_chosen = all_possible - available_courses
print(f"\n【沒人選的課程】")
print(f"  {not_chosen if not_chosen else '所有課程都有人選'}")


# 執行: python3 第 32 課：資料結構比較與選擇 — 什麼時候用什麼？1.py

# === 預期輸出 (節錄) ===
# 串列 in：0.004282 秒
# 集合 in：0.000001 秒
# 字典 in：0.000001 秒
# 串列：  8056 bytes
# 元組：  8048 bytes
# 集合： 32984 bytes
# 字典： 36952 bytes
# 所有單詞（串列）： ['the', 'cat', 'sat', 'on', 'the', 'mat', 'the', 'cat']
# 獨特單詞（集合）： {'cat', 'the', 'sat', 'on', 'mat'}
# 單詞計數（字典）： {'the': 3, 'cat': 2, 'sat': 1, 'on': 1, 'mat': 1}
# the: 3
# cat: 2
# sat: 1
# on: 1
# mat: 1
# the: 3
# cat: 2
# sat: 1
# on: 1
# mat: 1
# …（後略，完整輸出共 72 行）
# ⚠️ 計時數字每次執行都不同，重點是兩者的數量級差距（list 的 in 為 O(n)、set 為平均 O(1)）。
# ⚠️ set 不保證順序（字串元素還受 hash randomization 影響），每次執行的排列可能與上面不同，這是正常的。
