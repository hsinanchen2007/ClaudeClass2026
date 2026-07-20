# 第 31 課：集合 Set — 不重複的元素集合
# 基本集合

# ═══════════════════════════════════════════════════════════════════════════
# 【面試題】集合 Set
# ───────────────────────────────────────────────────────────────────────────
# 🔥 Q1. set 的核心用途與常用運算？
#     答：三件事——**去重**、**O(1) 成員測試**、**集合代數**。
#     實測 `A={1,2,3}, B={3,4}`：`A|B` → `{1,2,3,4}`（聯集）、
#     `A&B` → `{3}`（交集）、`A-B` → `{1,2}`（差集）、
#     `A^B` → `{1,2,4}`（對稱差）。
#     追問：為什麼 set 的 `in` 比 list 快？（雜湊查找 vs 線性掃描）
#
# 🔥 Q2. 空的 `{}` 是 dict 還是 set？
#     答：**dict**。實測 `type({})` 是 dict。
#     要建立空 set 只能用 **`set()`**。原因是 dict 這個語法出現得更早，
#     `{}` 已經被佔用了。
#     追問：`{1,2}` 與 `{1:2}` 分別是什麼？（前者 set、後者 dict）
#
# ⚠️ 陷阱. `list(set([3,1,2,1]))` 的輸出順序是什麼？
#     答：**不保證任何順序**——set 是無序的。
#     實測本機得到 `[1, 2, 3]` 看起來像「排序過」，
#     但那只是小整數的 hash 值恰好等於自身造成的巧合，**絕不可依賴**。
#     字串元素的順序甚至會隨 PYTHONHASHSEED 每次執行而變動。
#     為什麼會錯：拿幾次整數實驗的結果，推論成「set 會自動排序」。
#     要**保序去重**請用 `list(dict.fromkeys(seq))`——
#     實測 `[3,1,2,1]` → `[3, 1, 2]`，這才是有保證的做法（dict 3.7+ 保插入序）。
# ═══════════════════════════════════════════════════════════════════════════

fruits = {"蘋果", "香蕉", "芒果", "葡萄"}
print(fruits)         # {'芒果', '蘋果', '葡萄', '香蕉'}（順序可能不同）
print(type(fruits))   # <class 'set'>

# 自動去重
nums = {1, 2, 3, 2, 1, 4, 3, 5}
print(nums)           # {1, 2, 3, 4, 5}（重複的自動消失）


# ❌ 這是空字典，不是空集合！
empty_braces = {}
print(type(empty_braces))   # <class 'dict'>

# ✅ 空集合要用 set()
empty_set = set()
print(type(empty_set))      # <class 'set'>


# 從串列轉換（自動去重！）
numbers = set([1, 2, 3, 2, 1, 4])
print(numbers)        # {1, 2, 3, 4}

# 從字串轉換（每個字元變成一個元素）
chars = set("mississippi")
print(chars)          # {'m', 'i', 's', 'p'}（去重後只剩 4 個）

# 從元組轉換
from_tuple = set((10, 20, 30, 20, 10))
print(from_tuple)     # {10, 20, 30}

# 從 range 轉換
from_range = set(range(1, 6))
print(from_range)     # {1, 2, 3, 4, 5}


# 跟串列推導式一樣，只是用 {} 而不是 []
squares = {n ** 2 for n in range(1, 6)}
print(squares)        # {1, 4, 9, 16, 25}

# 帶條件的推導式
evens = {n for n in range(1, 21) if n % 2 == 0}
print(evens)          # {2, 4, 6, 8, 10, 12, 14, 16, 18, 20}


# ✅ 可以放入集合的型別
valid = {42, 3.14, "hello", True, (1, 2, 3)}
print(valid)

# ❌ 串列不能放入集合
# bad = {[1, 2, 3]}
# TypeError: unhashable type: 'list'

# ❌ 字典不能放入集合
# bad = {{"a": 1}}
# TypeError: unhashable type: 'dict'

# ❌ 集合也不能放入集合
# bad = {{1, 2, 3}}
# TypeError: unhashable type: 'set'


# 集合的特點：無序、不可重複、不可索引
fruits = {"蘋果", "香蕉", "芒果"}

# ❌ 集合沒有索引
# print(fruits[0])
# TypeError: 'set' object is not subscriptable

# ❌ 也沒有切片
# print(fruits[0:2])

# ✅ 可以走訪
for fruit in fruits:
    print(fruit)

# ✅ 可以檢查是否存在（而且超快！）
print("香蕉" in fruits)    # True


# 集合的新增元素方法：add()
fruits = {"蘋果", "香蕉"}

fruits.add("芒果")
print(fruits)       # {'蘋果', '香蕉', '芒果'}

# 新增已存在的元素 → 沒有任何效果（不報錯）
fruits.add("香蕉")
print(fruits)       # {'蘋果', '香蕉', '芒果'}（沒有重複的香蕉）


# 集合的新增多個元素方法：update()
fruits = {"蘋果", "香蕉"}

# 從串列加入
fruits.update(["芒果", "葡萄"])
print(fruits)       # {'蘋果', '香蕉', '芒果', '葡萄'}

# 從另一個集合加入
fruits.update({"西瓜", "蘋果"})    # 蘋果已存在，不會重複
print(fruits)       # {'蘋果', '香蕉', '芒果', '葡萄', '西瓜'}

# 從字串加入（每個字元）
chars = set()
chars.update("hello")
print(chars)        # {'h', 'e', 'l', 'o'}


# 集合的刪除元素方法：remove()
fruits = {"蘋果", "香蕉", "芒果"}

fruits.remove("香蕉")
print(fruits)       # {'蘋果', '芒果'}

# fruits.remove("草莓")   # ❌ KeyError: '草莓'


# 集合的安全刪除方法：discard()
fruits = {"蘋果", "香蕉", "芒果"}

fruits.discard("香蕉")
print(fruits)       # {'蘋果', '芒果'}

fruits.discard("草莓")    # 不存在也沒關係
print(fruits)       # {'蘋果', '芒果'}（安全，不報錯）


# 集合的隨機刪除方法：pop()
fruits = {"蘋果", "香蕉", "芒果", "葡萄"}

removed = fruits.pop()
print(f"被移除：{removed}")   # 被移除的元素是隨機的
print(f"剩餘：{fruits}")

# 空集合 pop 會報錯
# set().pop()   # ❌ KeyError: 'pop from an empty set'


# 集合的清空方法：clear()
fruits = {"蘋果", "香蕉", "芒果"}
fruits.clear()
print(fruits)       # set()


# 聯集（Union）：所有元素合在一起
# 方法一：| 運算子
A = {1, 2, 3, 4, 5}
B = {4, 5, 6, 7, 8}

# 方法一：| 運算子
print(A | B)           # {1, 2, 3, 4, 5, 6, 7, 8}

# 方法二：union() 方法
print(A.union(B))      # {1, 2, 3, 4, 5, 6, 7, 8}


# 交集（Intersection）：兩個集合共有的元素
# 方法一：& 運算子
print(A & B)               # {4, 5}

# 方法二：intersection() 方法
print(A.intersection(B))   # {4, 5}


# 差集（Difference）：在 A 但不在 B 的元素
# A - B：在 A 但不在 B
print(A - B)               # {1, 2, 3}
print(A.difference(B))     # {1, 2, 3}

# B - A：在 B 但不在 A（方向不同，結果不同！）
print(B - A)               # {6, 7, 8}
print(B.difference(A))     # {6, 7, 8}


# 對稱差集（Symmetric Difference）：在 A 或 B 但不在兩者共有的元素
# 方法一：^ 運算子
print(A ^ B)                       # {1, 2, 3, 6, 7, 8}

# 方法二：symmetric_difference() 方法
print(A.symmetric_difference(B))   # {1, 2, 3, 6, 7, 8}


# 原地聯集、交集、差集、對稱差集
A = {1, 2, 3, 4, 5}
B = {4, 5, 6, 7, 8}

# 原地聯集
A |= B                          # 等同 A.update(B)
print(A)                         # {1, 2, 3, 4, 5, 6, 7, 8}

# 原地交集
A = {1, 2, 3, 4, 5}
A &= B                          # 等同 A.intersection_update(B)
print(A)                         # {4, 5}

# 原地差集
A = {1, 2, 3, 4, 5}
A -= B                           # 等同 A.difference_update(B)
print(A)                         # {1, 2, 3}

# 原地對稱差集
A = {1, 2, 3, 4, 5}
A ^= B                          # 等同 A.symmetric_difference_update(B)
print(A)                         # {1, 2, 3, 6, 7, 8}


# 子集（Subset）和真子集（Proper Subset）
A = {1, 2, 3}
B = {1, 2, 3, 4, 5}
C = {1, 2, 3}

print(A <= B)              # True（A 是 B 的子集）
print(A.issubset(B))       # True

print(A <= C)              # True（相等也算子集）
print(A < C)               # False（A < C 是「真子集」，相等不算）
print(A < B)               # True（A 是 B 的真子集）


# 超集（Superset）和真超集（Proper Superset）
print(B >= A)              # True（B 是 A 的超集）
print(B.issuperset(A))     # True


# 不相交（Disjoint）：兩個集合沒有任何共同元素
X = {1, 2, 3}
Y = {4, 5, 6}
Z = {3, 4, 5}

print(X.isdisjoint(Y))    # True（完全沒有共同元素）
print(X.isdisjoint(Z))    # False（有共同的 3）


# 集合的應用：去重
# 串列去重：轉成集合再轉回來
names = ["小明", "小華", "小美", "小明", "小華", "小芳", "小美"]

unique = list(set(names))
print(unique)   # ['小芳', '小華', '小美', '小明']（順序可能不同）


# 保持順序的去重（第 26 課題目四的進化版！）
names = ["小明", "小華", "小美", "小明", "小華", "小芳", "小美"]

seen = set()
unique_ordered = []
for name in names:
    if name not in seen:
        seen.add(name)
        unique_ordered.append(name)

print(unique_ordered)   # ['小明', '小華', '小美', '小芳']（保持原順序）


# 檢查帳號是否已被使用
registered_users = {"alice", "bob", "cindy", "david"}

new_user = input("請輸入帳號：")
if new_user.lower() in registered_users:
    print("此帳號已被使用")
else:
    print("帳號可用！")
    registered_users.add(new_user.lower())


# 比較兩份名單的差異
old_members = {"小明", "小華", "小美", "小強", "小芳"}
new_members = {"小華", "小美", "小偉", "小芳", "小玲"}

joined = new_members - old_members
print(f"新加入的：{joined}")       # {'小偉', '小玲'}

left = old_members - new_members
print(f"退出的：{left}")           # {'小明', '小強'}

stayed = old_members & new_members
print(f"留下的：{stayed}")         # {'小華', '小美', '小芳'}

all_ever = old_members | new_members
print(f"曾經參加的：{all_ever}")   # 全部 7 個人



# 檢查使用者輸入是否在允許範圍內
valid_options = {"A", "B", "C", "D"}

answer = input("請選擇答案（A/B/C/D）：").upper()
if answer in valid_options:
    print(f"你選了 {answer}")
else:
    print("無效的選項！")


# 比較兩個人的興趣愛好
alice_hobbies = {"游泳", "閱讀", "程式設計", "登山"}
bob_hobbies = {"籃球", "程式設計", "電玩", "閱讀"}

common = alice_hobbies & bob_hobbies
print(f"共同興趣：{common}")            # {'程式設計', '閱讀'}

only_alice = alice_hobbies - bob_hobbies
print(f"只有 Alice 喜歡的：{only_alice}")  # {'游泳', '登山'}

all_hobbies = alice_hobbies | bob_hobbies
print(f"所有興趣：{all_hobbies}")        # 全部 6 個


# 建立 frozenset
fs = frozenset([1, 2, 3, 4, 5])
print(fs)           # frozenset({1, 2, 3, 4, 5})

# ❌ 不能修改
# fs.add(6)         # AttributeError
# fs.remove(1)      # AttributeError

# ✅ 可以做集合運算（回傳新的 frozenset）
fs2 = frozenset([4, 5, 6, 7])
print(fs & fs2)     # frozenset({4, 5})
print(fs | fs2)     # frozenset({1, 2, 3, 4, 5, 6, 7})

# ✅ 可以當字典的 key 或放入另一個集合
d = {frozenset([1, 2]): "pair_a"}
s = {frozenset([1, 2]), frozenset([3, 4])}


# 空集合的正確建立方式
a = {}        # 這是空字典！
b = set()     # 這才是空集合！

print(type(a))   # <class 'dict'>
print(type(b))   # <class 'set'>


# 集合沒有索引，不能直接存取
fruits = {"蘋果", "香蕉", "芒果"}
# print(fruits[0])   # ❌ TypeError: 'set' object is not subscriptable

# ✅ 如果需要索引存取，先轉成串列
fruits_list = list(fruits)
print(fruits_list[0])   # 可以，但順序不保證


# 集合的去重特性
data = [3, 1, 4, 1, 5, 9, 2, 6, 5, 3]
unique = list(set(data))
print(unique)   # 順序不保證！可能是 [1, 2, 3, 4, 5, 6, 9]


# ❌ 串列不能放入集合
# s = {[1, 2, 3]}   # TypeError: unhashable type: 'list'

# ✅ 要用元組
s = {(1, 2, 3)}    # OK


# 集合推導式和字典推導式的區別：有沒有冒號
# 集合推導式（沒有冒號）
s = {x for x in range(5)}
print(type(s))   # <class 'set'>

# 字典推導式（有冒號）
d = {x: x for x in range(5)}
print(type(d))   # <class 'dict'>



# === 社群好友分析器 ===

# 每個人的好友名單
friends = {
    "Alice":  {"Bob", "Cindy", "David", "Eva"},
    "Bob":    {"Alice", "Cindy", "Frank"},
    "Cindy":  {"Alice", "Bob", "David", "Eva", "Grace"},
    "David":  {"Alice", "Cindy", "Grace"},
    "Eva":    {"Alice", "Cindy", "Frank", "Grace"}
}

print("=" * 50)
print("👥 社群好友分析器")
print("=" * 50)

# 1. 顯示每個人的好友數量
print("\n【好友數量】")
for person, friend_set in sorted(friends.items()):
    print(f"  {person}: {len(friend_set)} 位好友")

# 2. 找出兩人的共同好友
person1, person2 = "Alice", "Eva"
common = friends[person1] & friends[person2]
print(f"\n【{person1} 和 {person2} 的共同好友】")
print(f"  {common if common else '沒有共同好友'}")

# 3. 推薦好友（對方的好友中，你還不認識的人）
target = "Bob"
print(f"\n【推薦 {target} 認識的新朋友】")
potential = set()
for friend in friends[target]:
    if friend in friends:
        # 朋友的朋友，扣掉已經是好友的和自己
        potential |= friends[friend]

# 排除已經是好友的和自己
recommendations = potential - friends[target] - {target}
print(f"  你可能認識：{recommendations}")

# 4. 找出「社交蝴蝶」（好友最多的人）
most_social = max(friends.items(), key=lambda x: len(x[1]))
print(f"\n【社交蝴蝶】")
print(f"  {most_social[0]}，擁有 {len(most_social[1])} 位好友")

# 5. 找出所有人的共同好友（全體交集）
print("\n【所有人都認識的人】")
all_friends = list(friends.values())
common_all = all_friends[0]
for friend_set in all_friends[1:]:
    common_all = common_all & friend_set

if common_all:
    print(f"  {common_all}")
else:
    print("  沒有所有人都認識的共同好友")

# 6. 社群中的所有人（聯集）
all_people = set(friends.keys())
for friend_set in friends.values():
    all_people |= friend_set
print(f"\n【社群總人數】")
print(f"  共 {len(all_people)} 人：{sorted(all_people)}")


# 執行: python3 第 31 課：集合 Set — 不重複的元素集合1.py

# === 預期輸出 (節錄) ===
# {'蘋果', '葡萄', '芒果', '香蕉'}
# <class 'set'>
# {1, 2, 3, 4, 5}
# <class 'dict'>
# <class 'set'>
# {1, 2, 3, 4}
# {'m', 's', 'p', 'i'}
# {10, 20, 30}
# {1, 2, 3, 4, 5}
# {1, 4, 9, 16, 25}
# {2, 4, 6, 8, 10, 12, 14, 16, 18, 20}
# {True, 3.14, 'hello', 42, (1, 2, 3)}
# 蘋果
# 芒果
# 香蕉
# True
# {'蘋果', '芒果', '香蕉'}
# {'蘋果', '芒果', '香蕉'}
# {'蘋果', '葡萄', '芒果', '香蕉'}
# {'蘋果', '香蕉', '西瓜', '葡萄', '芒果'}
# …（後略，完整輸出共 53 行）
# ⚠️ 本檔需要互動輸入（input()），以上為未輸入時的輸出；請自行執行並輸入資料。
# ⚠️ set 不保證順序（字串元素還受 hash randomization 影響），每次執行的排列可能與上面不同，這是正常的。
