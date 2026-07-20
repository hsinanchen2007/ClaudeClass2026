# ============================================================
# 第 30 課：字典 Dictionary（二）— 操作與方法 — 重點總結
# ============================================================

# ═══════════════════════════════════════════════════════════════════════════
# 【面試題】字典的合併、拷貝與更新
# ───────────────────────────────────────────────────────────────────────────
# 🔥 Q1. 合併兩個 dict 有哪些寫法？
#     答：`d1.update(d2)`（**就地**修改 d1、回傳 None）、
#     `{**d1, **d2}`（產生新 dict）、`d1 | d2`（3.9+，語意同前者但更清楚）。
#     三者遇到重複 key 都是**後者覆蓋前者**。
#     追問：這些合併是深拷貝嗎？（不是，全都是淺拷貝——
#     內層的可變物件仍然共用）
#
# 🔥 Q2. 怎麼安全地刪除 dict 的鍵？
#     答：`del d[k]` 與 `d.pop(k)` 在 key 不存在時都拋 KeyError；
#     `d.pop(k, None)` 給了預設值就不會拋。
#     `popitem()` 移除並回傳**最後**插入的一對（3.7+ 因插入序保證而確定）。
#     追問：能在走訪 dict 的過程中刪除 key 嗎？
#     （不行——實測迭代中改變大小會拋
#     `RuntimeError: dictionary changed size during iteration`；
#     要先 `for k in list(d):` 取快照）
#
# ⚠️ 陷阱. 淺拷貝一個 dict 之後改內層的 list，原本的會變嗎？
#     答：**會**。`dict(d)`／`d.copy()`／`{**d}` 都只複製最外層，
#     內層的可變物件仍共用同一個參考。
#     實測同樣的道理在 list 上：淺拷貝後 `s[0] is o[0]` → True。
#     為什麼會錯：把「拷貝」想成整棵樹都複製。
#     需要完全獨立必須用 `copy.deepcopy(d)`。
# ═══════════════════════════════════════════════════════════════════════════


# ============================================================
# 【重點 1】★ setdefault(key, default) — get + 自動新增
# key 存在 → 回傳現有值，不修改
# key 不存在 → 新增 key:default，並回傳 default
# ============================================================

contacts = {"小明": "0912", "小華": "0923"}

# key 已存在：回傳現有值，不修改字典
result = contacts.setdefault("小明", "0000")
print(result)                        # 0912（現有值）
print(contacts)                      # 不變

# key 不存在：新增並回傳預設值
result = contacts.setdefault("小美", "0934")
print(result)                        # 0934
print(contacts)                      # {..., '小美': '0934'}（新增了！）

# ★ get() vs setdefault() 差異：
d = {"a": 1}
d.get("b", 99)                      # 回傳 99，但字典不變
d.setdefault("b", 99)               # 回傳 99，而且字典新增了 "b": 99

# --- ★★★ 超實用：分組收集（setdefault 最經典用法） ---
students = [
    ("小明", "資工"), ("小華", "電機"),
    ("小美", "資工"), ("小強", "機械"),
]

groups = {}
for name, dept in students:
    groups.setdefault(dept, []).append(name)
    # dept 不存在 → 建立空串列 → 在串列上 append
    # dept 已存在 → 回傳現有串列 → 在串列上 append

print(groups)
# {'資工': ['小明', '小美'], '電機': ['小華'], '機械': ['小強']}

# ============================================================
# 【重點 2】popitem() — 刪除最後一對鍵值
# ============================================================

contacts = {"小明": "0912", "小華": "0923", "小美": "0934"}
last = contacts.popitem()
print(last)                          # ('小美', '0934')（回傳元組）
print(contacts)                      # {'小明': '0912', '小華': '0923'}

# ⚠️ 空字典呼叫 popitem() → KeyError
# {}.popitem()                       # ❌ KeyError

# ============================================================
# 【重點 3】★ copy() — 淺複製（跟串列一樣有別名陷阱）
# ============================================================

# --- ⚠️ 賦值是別名 ---
original = {"a": 1, "b": [2, 3]}
alias = original
alias["c"] = 99
print(original)                      # {'a': 1, 'b': [2, 3], 'c': 99} ← 被影響！

# --- ✅ copy() 建立獨立副本 ---
original = {"a": 1, "b": [2, 3]}
copied = original.copy()
copied["c"] = 99
print(original)                      # {'a': 1, 'b': [2, 3]} ← 不受影響

# --- ⚠️ 淺複製：內層可變物件仍共享！ ---
copied["b"].append(999)
print(original)                      # {'a': 1, 'b': [2, 3, 999]} ← 內層被改了！

# --- ✅ 深複製：完全獨立 ---
import copy
original = {"a": {"x": 1}}
deep = copy.deepcopy(original)
deep["a"]["x"] = 999
print(original)                      # {'a': {'x': 1}} ← 安全

# ============================================================
# 【重點 4】★★★ 字典推導式（Dictionary Comprehension）
# 語法：{key_expr: value_expr for item in iterable if condition}
# ============================================================

# --- 基本推導式 ---
squares = {n: n ** 2 for n in range(1, 6)}
# {1: 1, 2: 4, 3: 9, 4: 16, 5: 25}

# --- 轉換 ---
names = ["Alice", "Bob", "Catherine"]
name_lengths = {name: len(name) for name in names}
# {'Alice': 5, 'Bob': 3, 'Catherine': 9}

# --- 帶條件篩選 ---
scores = {"小明": 85, "小華": 45, "小美": 92, "小強": 58}
passed = {name: score for name, score in scores.items() if score >= 60}
# {'小明': 85, '小美': 92}

# --- ★ key-value 互換 ---
original = {"apple": "蘋果", "banana": "香蕉"}
reversed_dict = {v: k for k, v in original.items()}
# {'蘋果': 'apple', '香蕉': 'banana'}

# --- 兩個串列合成字典 ---
keys = ["name", "age"]
values = ["小明", 18]
d = {k: v for k, v in zip(keys, values)}

# ⚠️ 忘記冒號 → 變成集合推導式！
result = {n for n in range(5)}       # ← 這是 set，不是 dict！
result = {n: n ** 2 for n in range(5)}  # ← 這才是 dict

# ============================================================
# 【重點 5】多層巢狀字典
# ============================================================

school = {
    "資工系": {
        "小明": {"國文": 85, "英文": 92, "數學": 78},
        "小美": {"國文": 88, "英文": 79, "數學": 95},
    },
    "電機系": {
        "小華": {"國文": 90, "英文": 76, "數學": 82},
    },
}

# --- 逐層用 key 深入 ---
print(school["資工系"]["小明"]["數學"])   # 78

# --- ⚠️ 任何一層 key 不存在 → KeyError ---
# print(school["機械系"]["小強"]["國文"])  # ❌ KeyError

# --- ✅ 安全存取：逐層用 get() ---
dept = school.get("機械系")
if dept:
    student = dept.get("小強")
    if student:
        print(student.get("國文", "無成績"))
else:
    print("查無此科系")

# --- 走訪巢狀字典 ---
for dept, students in school.items():
    print(f"\n{dept}：")
    for name, scores in students.items():
        print(f"  {name}：{scores}")

# ============================================================
# 【重點 6】★ 字典的經典應用模式
# ============================================================

# --- 模式 1：計數器（統計出現次數） ---
text = "mississippi"
counter = {}
for char in text:
    counter[char] = counter.get(char, 0) + 1
print(counter)                       # {'m': 1, 'i': 4, 's': 4, 'p': 2}

# --- 模式 2：累加器（按 key 累加值） ---
orders = [("Alice", 150), ("Bob", 200), ("Alice", 80)]
totals = {}
for customer, amount in orders:
    totals[customer] = totals.get(customer, 0) + amount
print(totals)                        # {'Alice': 230, 'Bob': 200}

# --- 模式 3：查表法（取代冗長 if-elif） ---
day_names = {1: "星期一", 2: "星期二", 3: "星期三"}
print(day_names.get(3, "無效"))       # 星期三

# --- 模式 4：反向索引（用 setdefault 分組） ---
employees = {
    "Alice": ["Python", "SQL"],
    "Bob": ["Java", "Python"],
}

skill_index = {}
for person, skills in employees.items():
    for skill in skills:
        skill_index.setdefault(skill, []).append(person)
print(skill_index["Python"])          # ['Alice', 'Bob']

# ============================================================
# 【重點 7】字典排序 — 用 sorted()
# ============================================================

scores = {"小明": 85, "小華": 92, "小美": 78}

# ⚠️ 字典沒有 sort() 方法！
# scores.sort()                      # ❌ AttributeError

# ✅ 用 sorted() + items() + lambda
ranked = sorted(scores.items(), key=lambda x: x[1], reverse=True)
print(ranked)                         # [('小華', 92), ('小明', 85), ('小美', 78)]

# 如果需要結果是字典
ranked_dict = dict(sorted(scores.items(), key=lambda x: x[1], reverse=True))

# ============================================================
# 【重點 8】⚠️ 常見錯誤
# ============================================================

# --- ❌ 直接用 dict[key] += 1，key 不存在會 KeyError ---
counter = {}
# counter["h"] += 1                  # ❌ KeyError: 'h'

# ✅ 做法一：get()
counter["h"] = counter.get("h", 0) + 1

# ✅ 做法二：先檢查
if "h" not in counter:
    counter["h"] = 0
counter["h"] += 1

# ✅ 做法三：setdefault()
counter.setdefault("h", 0)
counter["h"] += 1

# --- ⚠️ copy() 是淺複製，內層可變物件仍共享 ---
# 需要完全獨立 → 用 copy.deepcopy()

# --- ⚠️ 推導式忘記冒號 → 變成 set ---
# {n for n in range(5)}              # set，不是 dict！
# {n: n**2 for n in range(5)}        # ✅ dict

# ============================================================
# 小結：
# 1. ★ setdefault(key, default)：不存在就新增，存在就回傳
#    最常用於分組收集：groups.setdefault(key, []).append(val)
# 2. popitem()：刪除最後一對鍵值並回傳元組
# 3. copy()：淺複製（內層仍共享），深複製用 deepcopy()
# 4. ★ 字典推導式：{key: val for x in iter if cond}
#    可做篩選、轉換、key-value 互換
# 5. 巢狀字典用多重 key 存取，安全存取用 get()
# 6. ★ 四大應用模式：計數器、累加器、查表法、反向索引
# 7. 排序用 sorted(dict.items(), key=lambda x: x[1])
# 8. ⚠️ 計數前要用 get() 或 setdefault() 初始化
# 9. ⚠️ 推導式 {n for ...} 是 set，{n: v for ...} 才是 dict
# ============================================================


# 執行: python3 summary.py

# === 預期輸出 ===
# 0912
# {'小明': '0912', '小華': '0923'}
# 0934
# {'小明': '0912', '小華': '0923', '小美': '0934'}
# {'資工': ['小明', '小美'], '電機': ['小華'], '機械': ['小強']}
# ('小美', '0934')
# {'小明': '0912', '小華': '0923'}
# {'a': 1, 'b': [2, 3], 'c': 99}
# {'a': 1, 'b': [2, 3]}
# {'a': 1, 'b': [2, 3, 999]}
# {'a': {'x': 1}}
# 78
# 查無此科系
#
# 資工系：
#   小明：{'國文': 85, '英文': 92, '數學': 78}
#   小美：{'國文': 88, '英文': 79, '數學': 95}
#
# 電機系：
#   小華：{'國文': 90, '英文': 76, '數學': 82}
# {'m': 1, 'i': 4, 's': 4, 'p': 2}
# {'Alice': 230, 'Bob': 200}
# 星期三
# ['Alice', 'Bob']
# [('小華', 92), ('小明', 85), ('小美', 78)]
