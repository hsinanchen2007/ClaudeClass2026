# 在Python中，資料類型（Data Type）是用來定義變數所能儲存的資料種類。不同的資料類型有不同的特性和用途。以下是一些常見的基本資料類型：
# 1. 整數（int）：用來表示整數值，例如：-3、0、42。
# 2. 浮點數（float）：用來表示帶有小數點的數值，例如：3.14、-0.001、2.0。
# 3. 字串（str）：用來表示文字資料，例如："Hello, World!"、'Python'。
# 4. 布林值（bool）：用來表示真（True）或假（False）的值，常用於條件判斷和邏輯運算。
# 5. 列表（list）：用來表示有序的資料集合，可以包含不同類型的元素，例如：[1, 2, 3]、["apple", "banana", "cherry"]。
# 6. 元組（tuple）：用來表示有序且不可變的資料集合，例如：(1, 2, 3)。
# 7. 字典（dict）：用來表示無序的鍵值對集合，例如：{"name": "Alice", "age": 30}。
# 8. 集合（set）：用來表示無序且不重複的元素集合，例如：{1, 2, 3}。
# 這些資料類型在Python中非常重要，因為它們決定了我們如何儲存和操作資料。了解不同的資料類型可以幫助我們更有效地編寫程式碼，並避免一些常見的錯誤。

# 你請使用者輸入年齡
age = input("請輸入你的年齡：")
print(type(age))   # <class 'str'> ← input() 回傳的永遠是字串！

# 嘗試做數學運算
#print(age + 10)    # ❌ TypeError! 字串不能和數字相加

# 解決方法：把字串轉換成整數
a = "123"
b = int(a)

print(a, type(a))   # 123 <class 'str'>
print(b, type(b))   # 123 <class 'int'>

# 現在可以做數學運算了！
print(b + 10)        # 133

# 轉換浮點數時，是直接去掉小數部分（截斷），不是四捨五入！
print(int(3.9))     # 3  ← 不是四捨五入！是直接去掉小數
print(int(3.1))     # 3
print(int(-2.7))    # -2 ← 往零的方向截斷
print(int(-2.1))    # -2

print(int(True))    # 1
print(int(False))   # 0

# 字串內容不是合法的整數
#int("abc")         # ValueError: invalid literal for int()
#int("3.14")        # ValueError! ← 有小數點的字串不能直接轉 int
#int("")            # ValueError! ← 空字串不行
#int("12 34")       # ValueError! ← 有空格不行
#int("one")         # ValueError! ← 英文數字不行

# 轉換字串成浮點數
# 兩步驟轉換
text = "3.14"
# int(text)         # ❌ 會報錯

result = int(float(text))  # ✅ 先轉 float，再轉 int
print(result)              # 3

# 直接轉換成 float 就可以了
a = "3.14"
b = float(a)

print(b, type(b))   # 3.14 <class 'float'>

print(float(5))      # 5.0
print(float(0))      # 0.0
print(float(-10))    # -10.0

# 轉換布林值成數字，True 會變成 1，False 會變成 0。
print(float(True))   # 1.0
print(float(False))  # 0.0

# 整數形式的字串也能轉成 float
print(float("100"))    # 100.0

# 科學記號
print(float("1e3"))    # 1000.0（1 × 10³）
print(float("2.5e2"))  # 250.0（2.5 × 10²）

#float("abc")      # ValueError!
#float("")         # ValueError!
#float("12,345")   # ValueError! ← 逗號不行

# 轉換成字串
a = str(123)
print(a, type(a))     # 123 <class 'str'>

b = str(3.14)
print(b, type(b))     # 3.14 <class 'str'>

c = str(-50)
print(c, type(c))     # -50 <class 'str'>

# 轉換布林值成字串
print(str(True))      # "True"
print(str(False))     # "False"
print(type(str(True))) # <class 'str'>

# 把字串轉換成布林值，空字串會被視為 False，其他的字串都會被視為 True。
age = 25
# print("我今年" + age + "歲")     # ❌ TypeError!
print("我今年" + str(age) + "歲")   # ✅ 我今年25歲

# 把數字轉換成布林值，0 會被視為 False，其他的數字都會被視為 True。
print(bool(0))        # False
print(bool(0.0))      # False
print(bool(""))       # False ← 空字串
print(bool(None))     # False
print(bool([]))       # False ← 空串列

# 其他的值都會被視為 True。
print(bool(1))        # True
print(bool(-5))       # True ← 任何非零數字
print(bool(3.14))     # True
print(bool("hello"))  # True ← 有內容的字串
print(bool("0"))      # True ← ⚠️ 字串 "0" 不是空的！
print(bool(" "))      # True ← ⚠️ 空格也算有內容！
print(bool("False"))  # True ← ⚠️ 字串 "False" 不是空的！

# 最重要的實際應用：input() + 型別轉換, 讓使用者輸入的資料能夠正確地被程式處理。
# 取得使用者輸入（永遠是字串！）
birth_year = input("請輸入你的出生年份：")

# 轉換成整數才能做數學運算
birth_year = int(birth_year)

# 計算年齡
age = 2025 - birth_year
print("你今年大約 " + str(age) + " 歲")

# 你請使用者輸入身高和體重，計算 BMI（身體質量指數）。
# 取得輸入
height = float(input("請輸入身高（公分）："))
weight = float(input("請輸入體重（公斤）："))

# 計算 BMI（身高要轉換成公尺）
height_m = height / 100
bmi = weight / (height_m ** 2)

# 顯示結果（四捨五入到小數第一位）
print("你的 BMI 是：" + str(round(bmi, 1)))

# 取得商品資訊
item_name = input("商品名稱：")
price = float(input("商品單價："))
quantity = int(input("購買數量："))

# 計算總價
total = price * quantity

# 顯示結果
print("------- 購物明細 -------")
print("商品：" + item_name)
print("單價：" + str(price) + " 元")
print("數量：" + str(quantity) + " 個")
print("總計：" + str(total) + " 元")

# 不用分兩行，可以直接包在一起, 但要注意可讀性，太複雜的話還是建議分成兩行。
age = int(input("請輸入年齡："))
height = float(input("請輸入身高："))

# 等同於：
# temp = input("請輸入年齡：")   # 先取得字串
# age = int(temp)                 # 再轉成整數

# 練習：判斷以下表達式的布林值。
print(int(9.99))
print(float("10"))
print(str(True))
print(bool(""))
print(bool("0"))
print(int(True) + int(False))
print(int("3" + "4"))


