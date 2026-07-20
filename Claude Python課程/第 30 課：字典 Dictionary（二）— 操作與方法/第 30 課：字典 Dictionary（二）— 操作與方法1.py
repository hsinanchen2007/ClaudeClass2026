# ═══════════════════════════════════════════════════════════════════════════
# 【面試題】字典 Dictionary（二）：操作與方法
# ───────────────────────────────────────────────────────────────────────────
# 🔥 Q1. `d[key]` 與 `d.get(key)` 差在哪？
#     答：key 不存在時 `d[key]` 拋 **KeyError**；
#     `d.get(key)` 安靜回傳 **None**，`d.get(key, default)` 回傳指定預設值。
#     判準很簡單：**key 缺失算不算錯誤**？算錯就用 `[]`（早點炸出來），
#     可以容錯就用 `get`。
#     追問：`d.get('z')` 回傳 None，與「key 存在但值就是 None」怎麼區分？
#     （用 `'z' in d` 判斷，或給一個獨特的 sentinel 當 default）
#
# 🔥 Q2. 怎麼走訪 dict？
#     答：`for k in d`（預設走 key）、`d.keys()`、`d.values()`、
#     `d.items()`（同時拿 key 與 value，最常用）。
#     Python 3 中這些回傳的是 **view 物件**，不是 list。
#     追問：view 與 list 差在哪？（view 是**動態**的——實測取得
#     `d.keys()` 後再新增 key，該 view 也會反映出來；要凍結快照得 `list(...)`）
#
# ⚠️ 陷阱. `setdefault()` 與 `defaultdict` 差在哪？
#     答：`d.setdefault('b', [])` 會在 key 不存在時**寫入**該預設值並回傳它
#     （實測執行後 dict 真的多了一個 key）；
#     `collections.defaultdict(list)` 則是在**存取缺失 key 時**自動建立，
#     所以 `dd['x'].append(1)` 可以直接用。
#     為什麼會錯：把 `setdefault` 當成 `get` 的別名——
#     它名字裡有 set，是真的會改變 dict 的。
# ═══════════════════════════════════════════════════════════════════════════

contacts = {"小明": "0912", "小華": "0923"}

# key 已存在：回傳現有值，不修改
result = contacts.setdefault("小明", "0000")
print(result)       # 0912（回傳現有值）
print(contacts)     # {'小明': '0912', '小華': '0923'}（沒被改）

# key 不存在：新增並回傳預設值
result = contacts.setdefault("小美", "0934")
print(result)       # 0934（回傳新設定的值）
print(contacts)     # {'小明': '0912', '小華': '0923', '小美': '0934'}


# get() 與 setdefault() 的差異
# get()：不修改字典，回傳預設值
# setdefault()：修改字典，回傳預設值
d = {"a": 1}

# get()：不存在時回傳預設值，但不會修改字典
d.get("b", 99)
print(d)           # {'a': 1}（字典沒變）

# setdefault()：不存在時回傳預設值，而且會新增到字典
d.setdefault("b", 99)
print(d)           # {'a': 1, 'b': 99}（字典被修改了）


# 把學生按科系分組
students = [
    ("小明", "資工"),
    ("小華", "電機"),
    ("小美", "資工"),
    ("小強", "機械"),
    ("小芳", "電機"),
    ("小偉", "資工")
]

groups = {}
for name, dept in students:
    groups.setdefault(dept, []).append(name)
    # 如果 dept 不存在 → 建立空串列 []
    # 不管是新建還是已存在 → 都回傳那個串列
    # 最後在串列上 .append(name)

print(groups)
# {'資工': ['小明', '小美', '小偉'], '電機': ['小華', '小芳'], '機械': ['小強']}


# popitem()：隨機刪除並回傳一對 key-value
contacts = {"小明": "0912", "小華": "0923", "小美": "0934"}

last = contacts.popitem()
print(last)         # ('小美', '0934')（回傳一個元組）
print(contacts)     # {'小明': '0912', '小華': '0923'}

# 空字典呼叫 popitem() 會報錯
# {}.popitem()     # ❌ KeyError: 'popitem(): dictionary is empty'


# copy()：淺複製（shallow copy）
original = {"a": 1, "b": [2, 3]}

# ❌ 賦值只是建立別名（同串列的問題）
alias = original
alias["c"] = 99
print(original)   # {'a': 1, 'b': [2, 3], 'c': 99} ← 被影響！

# ✅ copy() 建立獨立副本
original = {"a": 1, "b": [2, 3]}
copied = original.copy()
copied["c"] = 99
print(original)   # {'a': 1, 'b': [2, 3]} ← 不受影響


original = {"a": 1, "b": [2, 3]}
copied = original.copy()

# 修改內層的串列 → 原字典也會被影響！
copied["b"].append(999)
print(original)   # {'a': 1, 'b': [2, 3, 999]} ← 內層被改了！


# 傳統寫法：建立 1~5 的平方字典
squares = {}
for n in range(1, 6):
    squares[n] = n ** 2
print(squares)   # {1: 1, 2: 4, 3: 9, 4: 16, 5: 25}

# 字典推導式（Dictionary Comprehension）
# 推導式語法：{key_expr: value_expr for item in iterable}
# 推導式寫法
squares = {n: n ** 2 for n in range(1, 6)}
print(squares)   # {1: 1, 2: 4, 3: 9, 4: 16, 5: 25}


# 把名單轉成「名字 → 名字長度」的字典
names = ["Alice", "Bob", "Catherine", "David"]
name_lengths = {name: len(name) for name in names}
print(name_lengths)
# {'Alice': 5, 'Bob': 3, 'Catherine': 9, 'David': 5}


# 只保留及格（>= 60）的成績
scores = {"小明": 85, "小華": 45, "小美": 92, "小強": 58, "小芳": 73}

passed = {name: score for name, score in scores.items() if score >= 60}
print(passed)
# {'小明': 85, '小美': 92, '小芳': 73}


original = {"apple": "蘋果", "banana": "香蕉", "cherry": "櫻桃"}

# 把 key 和 value 互換
reversed_dict = {v: k for k, v in original.items()}
print(reversed_dict)
# {'蘋果': 'apple', '香蕉': 'banana', '櫻桃': 'cherry'}


# 把兩個串列合成字典
keys = ["name", "age", "city"]
values = ["小明", 18, "台北"]

# 方法一：dict(zip(...))（第 29 課學的）
d1 = dict(zip(keys, values))

# 方法二：字典推導式
d2 = {k: v for k, v in zip(keys, values)}

print(d1)   # {'name': '小明', 'age': 18, 'city': '台北'}
print(d2)   # {'name': '小明', 'age': 18, 'city': '台北'}


# 多層巢狀字典
# 學校 → 科系 → 學生 → 成績
# 例如：school["資工系"]["小明"]["數學"] → 78
# 這裡的 school 是一個字典，裡面有兩個 key（資工系、電機系），每個 key 對應的 value 又是另一個字典（學生），
# 學生字典裡的 key 是學生名字，value 又是成績字典，成績字典裡的 key 是科目，value 是分數。
# 這種多層巢狀的結構可以用來表示複雜的資料，例如學校的組織架構、公司的部門和員工資料等等。
school = {
    "資工系": {
        "小明": {"國文": 85, "英文": 92, "數學": 78},
        "小美": {"國文": 88, "英文": 79, "數學": 95}
    },
    "電機系": {
        "小華": {"國文": 90, "英文": 76, "數學": 82},
        "小芳": {"國文": 72, "英文": 88, "數學": 91}
    }
}

# 逐層用 key 深入
print(school["資工系"]["小明"]["數學"])    # 78

# 逐步拆解
dept = school["資工系"]           # 取得資工系的所有學生
student = dept["小明"]            # 取得小明的成績
math_score = student["數學"]      # 取得數學成績
print(math_score)                 # 78


# 直接用 key 深入（不安全）
# 如果任何一層的 key 不存在，就會直接報 KeyError，程式崩潰。
# ❌ 危險：如果 "機械系" 不存在就直接 KeyError
# print(school["機械系"]["小強"]["國文"])

# ✅ 安全方式一：逐層用 get()
dept = school.get("機械系")
if dept:
    student = dept.get("小強")
    if student:
        print(student.get("國文", "無成績"))
    else:
        print("查無此學生")
else:
    print("查無此科系")
# 輸出：查無此科系

# ✅ 安全方式二：用 try-except（第 49 課會詳細教）
try:
    print(school["機械系"]["小強"]["國文"])
except KeyError as e:
    print(f"找不到：{e}")
# 輸出：找不到：'機械系'


# 如果只是想列出學生的名字和分數，可以用兩層迴圈來遍歷字典，這樣就不需要關心巢狀結構的細節。
school = {
    "資工系": {"小明": 85, "小美": 92},
    "電機系": {"小華": 78, "小芳": 88}
}

for dept, students in school.items():
    print(f"\n📚 {dept}：")
    for name, score in students.items():
        print(f"  {name}：{score} 分")

# 輸出：
# 📚 資工系：
#   小明：85 分
#   小美：92 分
#
# 📚 電機系：
#   小華：78 分
#   小芳：88 分


# 字典的應用：統計字元出現次數
# 統計每個字元出現的次數
text = "mississippi"

counter = {}
for char in text:
    counter[char] = counter.get(char, 0) + 1

print(counter)
# {'m': 1, 'i': 4, 's': 4, 'p': 2}


# 把訂單按照客戶分組，並計算每位客戶的消費總額
orders = [
    ("Alice", 150),
    ("Bob", 200),
    ("Alice", 80),
    ("Cindy", 300),
    ("Bob", 120),
    ("Alice", 95)
]

totals = {}
for customer, amount in orders:
    totals[customer] = totals.get(customer, 0) + amount

for customer, total in totals.items():
    print(f"{customer}：NT${total}")

# 輸出：
# Alice：NT$325
# Bob：NT$320
# Cindy：NT$300



# ❌ 冗長的 if-elif 寫法
def get_day_name_bad(num):
    if num == 1:
        return "星期一"
    elif num == 2:
        return "星期二"
    elif num == 3:
        return "星期三"
    # ... 還要寫四個

# ✅ 用字典查表（簡潔、高效、好維護）
day_names = {
    1: "星期一", 2: "星期二", 3: "星期三",
    4: "星期四", 5: "星期五", 6: "星期六", 7: "星期日"
}

num = 3
print(day_names.get(num, "無效的數字"))   # 星期三



# 建立「技能 → 擁有該技能的人」的反向索引
employees = {
    "Alice": ["Python", "SQL", "Excel"],
    "Bob": ["Java", "Python", "Docker"],
    "Cindy": ["Python", "SQL", "Docker", "AWS"]
}

skill_index = {}
for person, skills in employees.items():
    for skill in skills:
        skill_index.setdefault(skill, []).append(person)

print("誰會 Python？", skill_index["Python"])
print("誰會 Docker？", skill_index["Docker"])
print("誰會 AWS？", skill_index["AWS"])

# 輸出：
# 誰會 Python？ ['Alice', 'Bob', 'Cindy']
# 誰會 Docker？ ['Bob', 'Cindy']
# 誰會 AWS？ ['Cindy']


text = "hello"
counter = {}

# ❌ 第一次遇到新字元就報錯
# for char in text:
#     counter[char] += 1     # KeyError: 'h'（h 還不存在）

# ✅ 正確做法一：用 get()
for char in text:
    counter[char] = counter.get(char, 0) + 1

# ✅ 正確做法二：先檢查
counter = {}
for char in text:
    if char not in counter:
        counter[char] = 0
    counter[char] += 1

# ✅ 正確做法三：用 setdefault()
counter = {}
for char in text:
    counter.setdefault(char, 0)
    counter[char] += 1


# ❌ 忘記冒號 → 這會變成集合推導式！
result = {n for n in range(5)}
print(type(result))   # <class 'set'>（不是字典！）

# ✅ 字典推導式要有 key: value
result = {n: n ** 2 for n in range(5)}
print(type(result))   # <class 'dict'>



original = {"a": {"x": 1}}
copied = original.copy()

# 修改內層 → 原字典也被影響
copied["a"]["x"] = 999
print(original)   # {'a': {'x': 999}} ← 被改了！

# 安全做法：用 deepcopy
import copy
original = {"a": {"x": 1}}
deep_copied = copy.deepcopy(original)
deep_copied["a"]["x"] = 999
print(original)   # {'a': {'x': 1}} ← 安全


scores = {"小明": 85, "小華": 92, "小美": 78}

# ❌ 字典沒有 sort() 方法
# scores.sort()   # AttributeError

# ✅ 用 sorted() 搭配 items()
# 依照值排序（成績由高到低）
ranked = sorted(scores.items(), key=lambda x: x[1], reverse=True)
print(ranked)   # [('小華', 92), ('小明', 85), ('小美', 78)]

# 如果需要結果是字典
ranked_dict = dict(sorted(scores.items(), key=lambda x: x[1], reverse=True))
print(ranked_dict)   # {'小華': 92, '小明': 85, '小美': 78}


# === 班級成績分析系統 ===

# 原始資料：每筆是 (姓名, 科系, 成績)
raw_data = [
    ("小明", "資工", 85), ("小華", "電機", 92),
    ("小美", "資工", 78), ("小強", "機械", 96),
    ("小芳", "電機", 88), ("小偉", "資工", 91),
    ("小玲", "機械", 73), ("小傑", "電機", 80),
    ("小萱", "資工", 95), ("小豪", "機械", 67)
]

# === 1. 按科系分組 ===
dept_groups = {}
for name, dept, score in raw_data:
    dept_groups.setdefault(dept, []).append((name, score))

print("=" * 45)
print("📊 班級成績分析系統")
print("=" * 45)

# === 2. 各科系統計 ===
print("\n【各科系成績統計】")
print(f"{'科系':^6}{'人數':^6}{'平均':^8}{'最高':^8}{'最低':^8}")
print("-" * 38)

dept_stats = {}
for dept, members in dept_groups.items():
    scores = [score for _, score in members]
    stats = {
        "count": len(scores),
        "avg": sum(scores) / len(scores),
        "max": max(scores),
        "min": min(scores),
        "members": members
    }
    dept_stats[dept] = stats
    print(f"{dept:^6}{stats['count']:^6}{stats['avg']:^8.1f}"
          f"{stats['max']:^8}{stats['min']:^8}")

# === 3. 各科系排名 ===
print("\n【各科系成績排名】")
for dept, stats in dept_stats.items():
    print(f"\n📚 {dept}：")
    ranked = sorted(stats["members"], key=lambda x: x[1], reverse=True)
    for rank, (name, score) in enumerate(ranked, 1):
        medal = "🥇" if rank == 1 else "🥈" if rank == 2 else "🥉" if rank == 3 else "  "
        print(f"  {medal} 第 {rank} 名：{name}（{score} 分）")

# === 4. 全班 Top 3 ===
print("\n【全班 Top 3】")
all_sorted = sorted(raw_data, key=lambda x: x[2], reverse=True)
for rank, (name, dept, score) in enumerate(all_sorted[:3], 1):
    print(f"  🏆 第 {rank} 名：{name}（{dept}）— {score} 分")

# === 5. 成績分布統計 ===
print("\n【成績分布】")
distribution = {"90+": 0, "80-89": 0, "70-79": 0, "60-69": 0, "<60": 0}
for _, _, score in raw_data:
    if score >= 90:
        distribution["90+"] += 1
    elif score >= 80:
        distribution["80-89"] += 1
    elif score >= 70:
        distribution["70-79"] += 1
    elif score >= 60:
        distribution["60-69"] += 1
    else:
        distribution["<60"] += 1

for level, count in distribution.items():
    bar = "█" * (count * 3)
    print(f"  {level:>6}：{count} 人 {bar}")


# 執行: python3 第 30 課：字典 Dictionary（二）— 操作與方法1.py

# === 預期輸出 (節錄) ===
# 0912
# {'小明': '0912', '小華': '0923'}
# 0934
# {'小明': '0912', '小華': '0923', '小美': '0934'}
# {'a': 1}
# {'a': 1, 'b': 99}
# {'資工': ['小明', '小美', '小偉'], '電機': ['小華', '小芳'], '機械': ['小強']}
# ('小美', '0934')
# {'小明': '0912', '小華': '0923'}
# {'a': 1, 'b': [2, 3], 'c': 99}
# {'a': 1, 'b': [2, 3]}
# {'a': 1, 'b': [2, 3, 999]}
# {1: 1, 2: 4, 3: 9, 4: 16, 5: 25}
# {1: 1, 2: 4, 3: 9, 4: 16, 5: 25}
# {'Alice': 5, 'Bob': 3, 'Catherine': 9, 'David': 5}
# {'小明': 85, '小美': 92, '小芳': 73}
# {'蘋果': 'apple', '香蕉': 'banana', '櫻桃': 'cherry'}
# {'name': '小明', 'age': 18, 'city': '台北'}
# {'name': '小明', 'age': 18, 'city': '台北'}
# 78
# …（後略，完整輸出共 85 行）
