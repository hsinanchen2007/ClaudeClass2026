# 字典（Dictionary）是一種資料結構，可以用來儲存「鍵值對」（key-value pair）。
# 每個鍵（key）對應一個值（value），你可以透過鍵來存取對應的值。
# 這種結構非常適合用來表示具有明確名稱的資料，例如人的名字和電話號碼。

# 串列：用「位置」存取（第 0 個、第 1 個...）
phones_list = ["0912-345-678", "0923-456-789"]
print(phones_list[0])   # 你要記住「0 是小明」

# 字典：用「名稱」存取（直覺多了！）
phones_dict = {"小明": "0912-345-678", "小華": "0923-456-789"}
print(phones_dict["小明"])   # 直接用名字查

# 如果鍵不存在，會拋出 KeyError
# print(phones_dict["小李"])   # KeyError: '小李'
# 可以使用 get() 方法來避免 KeyError，當鍵不存在時會回傳 None 或指定的預設值
print(phones_dict.get("小李"))  # None
print(phones_dict.get("小李", "沒有這個人"))  # 沒有這個人

# 字典的鍵必須是不可變類型（例如字串、數字、元組），而值可以是任何類型。
# 字典是無序的（在 Python 3.7 之前），但從 Python 3.7 開始，字典保持插入順序。
# 字典的鍵必須是唯一的，如果你使用相同的鍵，後面的值會覆蓋前面的值。
phones_dict["小明"] = "0987-654-321"  # 更新小明的電話號碼
print(phones_dict["小明"])  # 0987-654-321  

# 字典的常用方法：
# keys()：返回字典中所有的鍵    
print(phones_dict.keys())  # dict_keys(['小明', '小華'])
# values()：返回字典中所有的值
print(phones_dict.values())  # dict_values(['0987-654-321', '0923-456-789'])
# items()：返回字典中所有的鍵值對
print(phones_dict.items())  # dict_items([('小明', '0987-654-321'), ('小華', '0923-456-789')])
# del：刪除字典中的鍵值對
del phones_dict["小華"]  # 刪除小華的電話號碼
print(phones_dict)  # {'小明': '0987-654-321'}  
# 字典的使用非常廣泛，可以用來儲存和管理各種資料，例如學生的成績、商品的價格等等。

# 空字典
empty = {}
print(empty)          # {}
print(type(empty))    # <class 'dict'>

# 通訊錄
contacts = {
    "小明": "0912-345-678",
    "小華": "0923-456-789",
    "小美": "0934-567-890"
}
print(contacts)
# {'小明': '0912-345-678', '小華': '0923-456-789', '小美': '0934-567-890'}


# 學生資料：值是不同型別
student = {
    "name": "小明",          # 值是字串
    "age": 18,               # 值是整數
    "height": 175.5,         # 值是浮點數
    "is_active": True,       # 值是布林值
    "scores": [85, 92, 78],  # 值是串列
    "address": {             # 值是另一個字典（巢狀字典）
        "city": "台北",
        "district": "大安區"
    }
}
print(student["name"])                # 小明
print(student["scores"])              # [85, 92, 78]
print(student["address"]["city"])     # 台北


# 建立字典的幾種方法：
# 用關鍵字引數（key 必須是合法的變數名）
person = dict(name="小明", age=18, city="台北")
print(person)   # {'name': '小明', 'age': 18, 'city': '台北'}

# 用元組串列（每個元組是一對 key-value）
pairs = [("name", "小華"), ("age", 20), ("city", "高雄")]
person2 = dict(pairs)
print(person2)  # {'name': '小華', 'age': 20, 'city': '高雄'}


# 用 zip() 函數將兩個串列組合成字典
# zip() 函數會將兩個串列中的元素一一配對，形成一個由元組組成的迭代器，每個元組包含一對 key-value。
# 例如，第一個串列中的第一個元素會成為鍵，第二個串列中的第一個元素會成為對應的值，以此類推。
keys = ["name", "age", "city"]
values = ["小美", 19, "台中"]

person3 = dict(zip(keys, values))
print(person3)   # {'name': '小美', 'age': 19, 'city': '台中'}

# fromkeys() 方法可以用來建立一個新的字典，並為指定的鍵設定相同的預設值。
# 所有 key 預設同一個值
subjects = ["國文", "英文", "數學"]
scores = dict.fromkeys(subjects, 0)
print(scores)   # {'國文': 0, '英文': 0, '數學': 0}

# 不給預設值就是 None
empty_scores = dict.fromkeys(subjects)
print(empty_scores)   # {'國文': None, '英文': None, '數學': None}


# 字典的鍵值對可以是任何類型的資料，這使得字典非常靈活和強大。
# 例如，你可以使用字典來儲存一個人的聯絡資訊，其中鍵是聯絡方式的類型（例如「電話」、「電子郵件」），
# 而值則是對應的聯絡資訊。
contacts = {
    "小明": "0912-345-678",
    "小華": "0923-456-789",
    "小美": "0934-567-890"
}

# 透過鍵來存取對應的值
print(contacts["小明"])    # 0912-345-678
print(contacts["小美"])    # 0934-567-890


# print(contacts["小強"])
# ❌ KeyError: '小強'
# 使用 get() 方法來避免 KeyError，當鍵不存在時會回傳 None 或指定的預設值
print(contacts.get("小強"))  # None 


# 如果你想要在鍵不存在時回傳一個自訂的預設值，可以在 get() 方法中提供第二個參數：
contacts = {"小明": "0912-345-678", "小華": "0923-456-789"}

# 鍵存在：正常回傳值
print(contacts.get("小明"))           # 0912-345-678

# 鍵不存在：回傳 None（不報錯）
print(contacts.get("小強"))           # None

# 鍵不存在：回傳自訂預設值
print(contacts.get("小強", "查無此人"))  # 查無此人


contacts = {"小明": "0912-345-678", "小華": "0923-456-789"}

print("小明" in contacts)     # True
print("小強" in contacts)     # False
print("小強" not in contacts) # True


# in 檢查的是 key，不是 value
print("0912-345-678" in contacts)   # False（這是值，不是鍵）


# 使用 in 來檢查鍵是否存在，然後存取對應的值
contacts = {"小明": "0912-345-678", "小華": "0923-456-789"}
name = input("請輸入要查詢的姓名：")

if name in contacts:
    print(f"{name} 的電話：{contacts[name]}")
else:
    print(f"找不到 {name} 的資料")


# 字典的鍵值對可以被修改，這使得字典非常適合用來儲存和管理動態資料。
contacts = {"小明": "0912-345-678"}

# 新增：key 不存在 → 建立新的鍵值對
contacts["小華"] = "0923-456-789"
print(contacts)
# {'小明': '0912-345-678', '小華': '0923-456-789'}

# 修改：key 已存在 → 更新值
contacts["小明"] = "0999-999-999"
print(contacts)
# {'小明': '0999-999-999', '小華': '0923-456-789'}


# del 關鍵字可以用來刪除字典中的鍵值對。當你使用 del contacts["小華"] 時，會從字典中移除鍵 "小華" 以及對應的值。
contacts = {"小明": "0912", "小華": "0923", "小美": "0934"}

del contacts["小華"]
print(contacts)   # {'小明': '0912', '小美': '0934'}

# del contacts["小強"]   # ❌ KeyError: '小強'


# pop() 方法也可以用來刪除字典中的鍵值對，但它會返回被刪除的值。
contacts = {"小明": "0912", "小華": "0923", "小美": "0934"}

removed = contacts.pop("小華")
print(f"已刪除：{removed}")   # 已刪除：0923
print(contacts)               # {'小明': '0912', '小美': '0934'}

# 鍵不存在時可以給預設值（不報錯）
result = contacts.pop("小強", "查無此人")
print(result)                 # 查無此人


# clear() 方法可以用來清空字典中的所有鍵值對，使字典變成一個空字典。
contacts = {"小明": "0912", "小華": "0923"}
contacts.clear()
print(contacts)   # {}


# 鍵必須是不可變型別（immutable type），因為字典需要確保鍵的哈希值（hash value）不變，以便能夠正確地存取對應的值。
# ✅ 字串當鍵（最常見）
d1 = {"name": "小明", "age": 18}

# ✅ 整數當鍵
d2 = {1: "一月", 2: "二月", 3: "三月"}

# ✅ 元組當鍵（因為元組不可變）
d3 = {(25.03, 121.56): "台北", (22.63, 120.30): "高雄"}

# ❌ 串列不能當鍵（因為串列可變）
# d4 = {[1, 2]: "bad"}
# TypeError: unhashable type: 'list'

# ❌ 字典也不能當鍵
# d5 = {{"a": 1}: "bad"}
# TypeError: unhashable type: 'dict'


# 如果有重複的 key，後面的值會覆蓋前面的
data = {"a": 1, "b": 2, "a": 3}
print(data)       # {'a': 3, 'b': 2}
print(len(data))  # 2（只有兩個 key）


# 值可以是任何型別，也可以重複
flexible = {
    "string": "hello",
    "number": 42,
    "list": [1, 2, 3],
    "tuple": (4, 5, 6),
    "dict": {"nested": True},
    "bool": False,
    "none": None,
    "duplicate_value": 42    # 值可以和上面的 "number" 一樣
}
print(flexible)


# 字典的迴圈預設走訪的是鍵（key），你可以透過鍵來存取對應的值。
student = {"name": "小明", "age": 18, "city": "台北"}

# for 迴圈預設走訪的是 key
for key in student:
    print(f"{key} → {student[key]}")

# 輸出：
# name → 小明
# age → 18
# city → 台北

# keys()：明確取得所有鍵
print(student.keys())
# dict_keys(['name', 'age', 'city'])

for key in student.keys():
    print(key, end=" ")
# name age city


# values()：明確取得所有值
print(student.values())
# dict_values(['小明', 18, '台北'])

for value in student.values():
    print(value, end=" ")
# 小明 18 台北


# items()：明確取得所有鍵值對（推薦寫法）
print(student.items())
# dict_items([('name', '小明'), ('age', 18), ('city', '台北')])

# 搭配拆包，優雅地走訪（推薦寫法）
for key, value in student.items():
    print(f"{key}: {value}")

# 輸出：
# name: 小明
# age: 18
# city: 台北


# len() 函數可以用來計算字典中鍵值對的數量，也就是字典的長度。
contacts = {"小明": "0912", "小華": "0923", "小美": "0934"}
print(len(contacts))   # 3


# update() 方法可以用來將一個字典的鍵值對更新到另一個字典中。如果有重複的鍵，後面的值會覆蓋前面的值。
contacts = {"小明": "0912", "小華": "0923"}
new_data = {"小美": "0934", "小明": "0999"}   # 小明的號碼有更新

contacts.update(new_data)
print(contacts)
# {'小明': '0999', '小華': '0923', '小美': '0934'}
# 小明的值被更新，小美是新增的


# Python 3.9 引入了字典合併運算子（|）和原地合併運算子（|=），可以用來更簡潔地合併字典。
# | 運算子會產生一個新的字典，包含兩個字典的鍵值對，如果有重複的鍵，後面的值會覆蓋前面的值。
# |= 運算子會在原地修改左邊的字典，將右邊字典的鍵值對合併進去，同樣如果有重複的鍵，後面的值會覆蓋前面的值。
# 這兩個運算子提供了一種更簡潔和直觀的方式來合併字典，特別是在需要創建新字典或更新現有字典時非常有用。
a = {"x": 1, "y": 2}
b = {"y": 3, "z": 4}

c = a | b          # 產生新字典
print(c)           # {'x': 1, 'y': 3, 'z': 4}（重複 key 取右邊的）
print(a)           # {'x': 1, 'y': 2}（原字典不變）

a |= b             # 原地合併（等同 a.update(b)）
print(a)           # {'x': 1, 'y': 3, 'z': 4}


# 當你嘗試存取一個不存在的鍵時，會拋出 KeyError。為了避免這種情況，你可以使用 get() 方法來安全地存取值。
student = {"name": "小明", "age": 18}

# ❌ KeyError
# print(student["score"])

# ✅ 用 get()
print(student.get("score", "未設定"))   # 未設定


# 注意：空字典 {} 是 dict 類型，不是 set 類型。要建立空集合，必須使用 set()。
a = {}
print(type(a))   # <class 'dict'>（是字典，不是集合！）

b = set()
print(type(b))   # <class 'set'>（這才是空集合）


# in 檢查的是鍵（key），不是值（value）。
# 如果你想要檢查值是否存在，可以使用 values() 方法來取得所有的值，然後再使用 in 來檢查。
scores = {"國文": 85, "英文": 92}

print(85 in scores)         # False（85 是值，不是鍵）
print("國文" in scores)     # True（"國文" 是鍵）

# 如果要檢查值是否存在
print(85 in scores.values())    # True


# 在迴圈中修改字典會導致錯誤，因為字典的大小在迭代過程中改變了。
# 如果你需要在迴圈中修改字典，可以考慮以下幾種安全的做法：
# 1. 走訪字典的副本（例如 list(data.keys())），這樣就不會影響原字典的大小。
# 2. 使用字典推導式（dictionary comprehension）來建立一個新的字典，然後再替換原字典。
# 3. 收集要刪除的鍵，然後在迴圈結束後再刪除它們。
# 這些方法都可以避免在迴圈中修改字典導致的錯誤，確保程式的正確性和穩定性。
data = {"a": 1, "b": 2, "c": 3, "d": 4}

# ❌ 走訪時刪除會報錯
# for key in data:
#     if data[key] < 3:
#         del data[key]
# RuntimeError: dictionary changed size during iteration

# ✅ 安全做法：走訪副本的 keys
for key in list(data.keys()):
    if data[key] < 3:
        del data[key]
print(data)   # {'c': 3, 'd': 4}

# ✅ 或者用字典推導式建立新字典
data = {"a": 1, "b": 2, "c": 3, "d": 4}
filtered = {k: v for k, v in data.items() if v >= 3}
print(filtered)   # {'c': 3, 'd': 4}



# === 英漢迷你字典 ===

dictionary = {
    "apple": "蘋果",
    "banana": "香蕉",
    "cat": "貓",
    "dog": "狗",
    "elephant": "大象"
}

while True:
    print("\n📖 英漢迷你字典")
    print("1. 查詢單字")
    print("2. 新增單字")
    print("3. 修改翻譯")
    print("4. 刪除單字")
    print("5. 顯示所有單字")
    print("6. 離開")

    choice = input("請選擇功能（1-6）：")

    if choice == "1":
        word = input("  請輸入英文單字：").lower()
        result = dictionary.get(word)
        if result:
            print(f"  📗 {word} → {result}")
        else:
            print(f"  ❌ 查無「{word}」")

    elif choice == "2":
        word = input("  請輸入英文單字：").lower()
        if word in dictionary:
            print(f"  ⚠️ 「{word}」已存在，翻譯為「{dictionary[word]}」")
        else:
            meaning = input("  請輸入中文翻譯：")
            dictionary[word] = meaning
            print(f"  ✅ 已新增：{word} → {meaning}")

    elif choice == "3":
        word = input("  請輸入要修改的英文單字：").lower()
        if word in dictionary:
            print(f"  目前翻譯：{word} → {dictionary[word]}")
            new_meaning = input("  請輸入新的中文翻譯：")
            dictionary[word] = new_meaning
            print(f"  ✏️ 已更新：{word} → {new_meaning}")
        else:
            print(f"  ❌ 查無「{word}」，請先新增")

    elif choice == "4":
        word = input("  請輸入要刪除的英文單字：").lower()
        removed = dictionary.pop(word, None)
        if removed:
            print(f"  🗑️ 已刪除：{word} → {removed}")
        else:
            print(f"  ❌ 查無「{word}」")

    elif choice == "5":
        if not dictionary:
            print("  📭 字典是空的")
        else:
            print(f"  --- 共 {len(dictionary)} 個單字 ---")
            for eng, chi in sorted(dictionary.items()):
                print(f"  {eng:15s} → {chi}")

    elif choice == "6":
        print("👋 再見！")
        break

    else:
        print("  ❌ 請輸入 1-6 的數字")






