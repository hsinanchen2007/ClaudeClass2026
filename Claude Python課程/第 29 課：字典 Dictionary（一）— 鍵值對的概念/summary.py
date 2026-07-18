# ============================================================
# 第 29 課：字典 Dictionary（一）— 鍵值對的概念 — 重點總結
# ============================================================

# ============================================================
# 【重點 1】字典的概念 — 用「名稱」存取，而非「位置」
# 字典用 {} 定義，儲存「鍵(key) : 值(value)」的配對。
# ============================================================

# 串列：用位置存取（要記住 0 是誰）
phones_list = ["0912-345-678", "0923-456-789"]
print(phones_list[0])                         # 不直覺

# ★ 字典：用名稱存取（直覺！）
phones_dict = {"小明": "0912-345-678", "小華": "0923-456-789"}
print(phones_dict["小明"])                     # 0912-345-678

# ============================================================
# 【重點 2】字典的建立
# ============================================================

# --- 用 {} 直接建立（最常見） ---
empty = {}                                    # 空字典
contacts = {
    "小明": "0912-345-678",
    "小華": "0923-456-789",
}

# --- 值可以是任何型別 ---
student = {
    "name": "小明",                            # 字串
    "age": 18,                                 # 整數
    "scores": [85, 92, 78],                    # 串列
    "address": {"city": "台北", "district": "大安區"}  # 巢狀字典
}

# --- 用 dict() 建構 ---
person = dict(name="小明", age=18, city="台北")
# {'name': '小明', 'age': 18, 'city': '台北'}

# --- 用元組串列 ---
pairs = [("name", "小華"), ("age", 20)]
person2 = dict(pairs)

# --- ★ 用 zip() 合併兩個串列 ---
keys = ["name", "age", "city"]
values = ["小美", 19, "台中"]
person3 = dict(zip(keys, values))

# --- fromkeys()：所有 key 設同一個預設值 ---
scores = dict.fromkeys(["國文", "英文", "數學"], 0)
# {'國文': 0, '英文': 0, '數學': 0}

empty_scores = dict.fromkeys(["國文", "英文"], None)
# {'國文': None, '英文': None}

# ============================================================
# 【重點 3】存取值 — dict[key] vs dict.get(key)
# ============================================================

contacts = {"小明": "0912", "小華": "0923"}

# --- dict[key]：key 不存在 → KeyError ---
print(contacts["小明"])           # 0912
# print(contacts["小強"])         # ❌ KeyError: '小強'

# --- ★ get(key, default)：key 不存在 → 回傳 None 或預設值（不報錯） ---
print(contacts.get("小強"))               # None
print(contacts.get("小強", "查無此人"))    # 查無此人
print(contacts.get("小明"))               # 0912（key 存在正常回傳）

# ============================================================
# 【重點 4】in / not in — 檢查 key 是否存在
# ⚠️ in 檢查的是 key，不是 value！
# ============================================================

contacts = {"小明": "0912", "小華": "0923"}

print("小明" in contacts)            # True
print("小強" in contacts)            # False
print("小強" not in contacts)        # True

# ⚠️ in 只查 key
print("0912" in contacts)            # False（0912 是 value，不是 key）

# ✅ 要檢查 value 是否存在
print("0912" in contacts.values())   # True

# --- 實用：查詢前先檢查 ---
name = "小明"
if name in contacts:
    print(f"{name} 的電話：{contacts[name]}")
else:
    print(f"找不到 {name}")

# ============================================================
# 【重點 5】新增、修改、刪除
# ============================================================

contacts = {"小明": "0912"}

# --- 新增：key 不存在 → 建立新鍵值對 ---
contacts["小華"] = "0923"
print(contacts)                      # {'小明': '0912', '小華': '0923'}

# --- 修改：key 已存在 → 更新值 ---
contacts["小明"] = "0999"
print(contacts)                      # {'小明': '0999', '小華': '0923'}

# --- del：根據 key 刪除 ---
del contacts["小華"]
print(contacts)                      # {'小明': '0999'}
# del contacts["小強"]               # ❌ KeyError

# --- pop(key, default)：刪除並回傳值 ---
contacts = {"小明": "0912", "小華": "0923", "小美": "0934"}
removed = contacts.pop("小華")
print(removed)                       # 0923
print(contacts)                      # {'小明': '0912', '小美': '0934'}

# pop 可給預設值避免 KeyError
result = contacts.pop("小強", "查無此人")
print(result)                        # 查無此人

# --- clear()：清空字典 ---
contacts.clear()
print(contacts)                      # {}

# ============================================================
# 【重點 6】遍歷字典 — keys()、values()、items()
# ============================================================

student = {"name": "小明", "age": 18, "city": "台北"}

# --- for 預設走訪 key ---
for key in student:
    print(f"{key} → {student[key]}")

# --- keys()：所有鍵 ---
print(student.keys())                # dict_keys(['name', 'age', 'city'])

# --- values()：所有值 ---
print(student.values())              # dict_values(['小明', 18, '台北'])

# --- ★ items()：所有鍵值對 + 拆包（推薦寫法！） ---
for key, value in student.items():
    print(f"{key}: {value}")

# ============================================================
# 【重點 7】update() 與 | 合併字典
# ============================================================

contacts = {"小明": "0912", "小華": "0923"}
new_data = {"小美": "0934", "小明": "0999"}

# --- update()：原地合併（重複 key 被覆蓋） ---
contacts.update(new_data)
print(contacts)                      # {'小明': '0999', '小華': '0923', '小美': '0934'}

# --- | 運算子（Python 3.9+）：產生新字典 ---
a = {"x": 1, "y": 2}
b = {"y": 3, "z": 4}
c = a | b                           # {'x': 1, 'y': 3, 'z': 4}（重複 key 取右邊）
print(a)                             # {'x': 1, 'y': 2}（原字典不變）

# --- |= 原地合併（等同 update） ---
a |= b                              # a 被修改

# ============================================================
# 【重點 8】★ key 的限制
# ============================================================

# key 必須是不可變型別（immutable）
d1 = {"name": "小明"}               # ✅ 字串
d2 = {1: "一月", 2: "二月"}          # ✅ 整數
d3 = {(25, 121): "台北"}             # ✅ 元組

# ❌ 可變型別不能當 key
# {[1, 2]: "bad"}                   # ❌ TypeError: unhashable type: 'list'
# {{"a": 1}: "bad"}                 # ❌ TypeError: unhashable type: 'dict'

# --- ★ 重複 key → 後面覆蓋前面 ---
data = {"a": 1, "b": 2, "a": 3}
print(data)                          # {'a': 3, 'b': 2}
print(len(data))                     # 2

# --- 值可以是任何型別，也可以重複 ---
# key 不能重複，但 value 可以

# ============================================================
# 【重點 9】len() 與 {} 注意事項
# ============================================================

contacts = {"小明": "0912", "小華": "0923"}
print(len(contacts))                 # 2（鍵值對的數量）

# ⚠️ 空的 {} 是 dict，不是 set！
a = {}
print(type(a))                       # <class 'dict'>
b = set()                           # 要建立空集合必須用 set()
print(type(b))                       # <class 'set'>

# ============================================================
# 【重點 10】⚠️ 走訪時不能修改字典大小
# ============================================================

data = {"a": 1, "b": 2, "c": 3, "d": 4}

# ❌ 走訪時刪除 → RuntimeError
# for key in data:
#     if data[key] < 3:
#         del data[key]               # RuntimeError: dictionary changed size during iteration

# ✅ 安全做法一：走訪 keys 的副本
for key in list(data.keys()):
    if data[key] < 3:
        del data[key]
print(data)                           # {'c': 3, 'd': 4}

# ✅ 安全做法二：字典推導式建新字典
data = {"a": 1, "b": 2, "c": 3, "d": 4}
filtered = {k: v for k, v in data.items() if v >= 3}
print(filtered)                       # {'c': 3, 'd': 4}

# ============================================================
# 小結：
# 1. 字典用 {} 定義，儲存 key: value 配對
# 2. 用 key 存取值：dict[key] 或 dict.get(key, default)
# 3. ★ get() 不報錯，dict[key] 不存在會 KeyError
# 4. in 檢查的是 key，不是 value
# 5. 新增/修改：dict[key] = value（key 不存在就新增，存在就修改）
# 6. 刪除：del dict[key]、pop(key)、clear()
# 7. ★ 遍歷推薦用 items() + 拆包
# 8. update() 或 | 合併字典，重複 key 被覆蓋
# 9. key 必須不可變（str, int, tuple ✅ / list, dict ❌）
# 10. ⚠️ 空 {} 是 dict 不是 set
# 11. ⚠️ 走訪時不能修改字典大小，要走訪副本或建新字典
# ============================================================
