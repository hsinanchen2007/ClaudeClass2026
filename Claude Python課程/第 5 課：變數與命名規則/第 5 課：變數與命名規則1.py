# 第五課：變數與命名規則
# 在程式中，我們經常需要使用一些資料來進行計算或顯示結果。這些資料可以是數字、文字或其他類型的資訊。為了方便使用和管理這些資料，我們可以將它們存儲在變數中。
# 變數是一個用來儲存資料的容器，我們可以給它取一個名字，然後將資料賦值給它。這樣，我們就可以通過變數的名字來使用這些資料，而不需要每次都重複輸入。
# 在 Python 中，變數的命名規則如下：

# ═══════════════════════════════════════════════════════════════════════════
# 【面試題】變數的本質與命名規則
# ───────────────────────────────────────────────────────────────────────────
# 🔥 Q1. Python 的變數到底是什麼？「賦值」發生了什麼事？
#     答：變數是**名稱綁定（name binding）**——把一個名字指向某個物件，
#     不是「裝值的盒子」。所以 `a = [1]; b = a` 之後兩個名字指向**同一個物件**，
#     實測 `b.append(2)` 後 `a` 也變成 `[1, 2]`、`a is b` → True。
#     追問：那要怎麼真正複製？（`a.copy()`／`a[:]`／`copy.deepcopy`）
#
# 🔥 Q2. 變數命名有哪些規則與慣例？
#     答：規則是硬性的——只能用字母／數字／底線、不能數字開頭、不能用關鍵字
#     （本機 CPython 3.14.4 實測 `keyword.kwlist` 共 **35 個**）、區分大小寫。
#     慣例來自 PEP 8：變數與函式 `snake_case`、常數 `UPPER_CASE`、類別 `PascalCase`。
#     追問：什麼是 soft keyword？（實測 3.14.4 有 `_`、`case`、`match`、`type`
#     四個，它們有語法意義但仍可當一般變數名）
#
# ⚠️ 陷阱. 「變數要先宣告型別」在 Python 成立嗎？
#     答：不成立。Python 是動態型別，名稱不綁型別，同一個名字可以先後指向
#     int 再指向 str。型別是**物件**的屬性，不是名稱的屬性。
#     為什麼會錯：帶著 C/Java 的心智模型，把名稱想成「有型別的記憶體格子」，
#     於是無法解釋為什麼 `x = 1` 之後 `x = "a"` 完全合法。
# ═══════════════════════════════════════════════════════════════════════════

print("學生姓名：陳小明")
print("================")
print("陳小明 的成績是 95 分")
print("恭喜 陳小明 通過考試！")

# 使用變數可以讓程式更具可讀性和可維護性，因為我們可以給變數取一個有意義的名字，讓人一眼就能看出它代表什麼資料。
name = "陳小明"
print("學生姓名：", name)
print("================")
print(name, "的成績是 95 分")
print("恭喜", name, "通過考試！")

# 在 Python 中，變數的命名規則如下：
# 1. 變數名稱只能包含字母、數字和底線（_），但不能以數字開頭。
# 2. 變數名稱不能使用 Python 的保留字（例如：if、for、while 等）。
# 3. 變數名稱應該具有描述性，能夠清楚地表達變數所代表的資料或用途。
# 4. 變數名稱區分大小寫，例如：name 和 Name 是兩個不同的變數。
# 5. 變數名稱應該使用小寫字母，並且可以使用底線來分隔單詞，例如：student_name、age、height 等。

# 儲存文字
name = "小明"
city = "台北"

# 儲存整數
age = 18
year = 2024

# 儲存小數
height = 175.5
pi = 3.14159

# 儲存布林值（對或錯）
is_student = True
is_married = False

# 在 Python 中，變數是一個用來儲存資料的容器，我們可以給它取一個名字，然後將資料賦值給它。這樣，我們就可以通過變數的名字來使用這些資料，而不需要每次都重複輸入。
name = "小明"
age = 18

print(name)
print(age)

# 使用變數可以讓程式更具可讀性和可維護性，因為我們可以給變數取一個有意義的名字，讓人一眼就能看出它代表什麼資料。
name = "小明"
age = 18

print("我的名字是", name)
print("我今年", age, "歲")

# 變數名稱區分大小寫，例如：name 和 Name 是兩個不同的變數。
# 儲存文字
name = "小明"
print("name")    # 印出文字 "name"
print(name)      # 印出變數 name 的值

# 變數名稱應該使用小寫字母，並且可以使用底線來分隔單詞，例如：student_name、age、height 等。
score = 60
print("原本的分數：", score)

score = 85
print("補考後的分數：", score)

score = 100
print("再次補考後：", score)

# 變數可以用來儲存各種類型的資料，例如文字、數字、布林值等。這些資料可以在程式中進行運算、比較或其他操作。
a = 10
b = 3

print(a + b)    # 加法
print(a - b)    # 減法
print(a * b)    # 乘法
print(a / b)    # 除法

# 變數也可以用來儲存計算的結果，這樣我們就可以重複使用這些結果，而不需要每次都重新計算。
price = 100
quantity = 5

total = price * quantity

print("單價：", price)
print("數量：", quantity)
print("總價：", total)

# 變數的值可以隨時改變，我們可以對變數進行重新賦值，這樣它就會儲存新的資料。
count = 10
print("原本：", count)

count = count + 1
print("加 1 之後：", count)

count = count + 1
print("再加 1：", count)

# 變數的命名規則如下：
# 1. 變數名稱只能包含字母、數字和底線（_），但不能以數字開頭。
# 2. 變數名稱不能使用 Python 的保留字（例如：if、for、while 等）。
# 3. 變數名稱應該具有描述性，能夠清楚地表達變數所代表的資料或用途。
# 4. 變數名稱區分大小寫，例如：name 和 Name 是兩個不同的變數。
# 5. 變數名稱應該使用小寫字母，並且可以使用底線來分隔單詞，例如：student_name、age、height 等。
a = 1
b = 2
c = 3

# 也可以同時給多個變數賦值，這樣可以讓程式更簡潔。
a, b, c = 1, 2, 3

print(a)
print(b)
print(c)

# 也可以將多個變數同時賦值為相同的值，這樣可以節省程式碼的行數。
x = y = z = 0

print(x)
print(y)
print(z)

# ❌ 不好：看不出這是什麼
a = 18
b = "小明"
c = 175.5

# ✅ 好：一看就知道
age = 18
name = "小明"
height = 175.5

# ✅ Python 推薦的風格（snake_case）
my_name = "小明"
student_age = 18
total_score = 95
is_passed = True

# 駝峰式（camelCase）- 常見於 JavaScript
myName = "小明"
studentAge = 18

# 帕斯卡式（PascalCase）- Python 用於類別名稱
MyName = "小明"
StudentAge = 18

# ❌ 不好（除非是很短的程式）
x = 100
y = 5
z = x * y

# ✅ 好
price = 100
quantity = 5
total = price * quantity
print("單價：", price)
print("數量：", quantity)   
print("總價：", total)

# ❌ 太長了
the_total_price_of_all_items_in_the_shopping_cart = 500

# ✅ 適中
cart_total = 500

# 變數的值可以是不同類型的資料，我們可以使用 type() 函數來查看變數的資料型別。
name = "小明"
age = 18
height = 175.5
is_student = True

print(type(name))
print(type(age))
print(type(height))
print(type(is_student))

# 建立變數
name = "陳小明"
age = 20
city = "台北"
hobby = "打籃球"

# 印出資料卡
print("╔════════════════════════╗")
print("║      個人資料卡        ║")
print("╠════════════════════════╣")
print("║ 姓名：", name)
print("║ 年齡：", age, "歲")
print("║ 城市：", city)
print("║ 興趣：", hobby)
print("╚════════════════════════╝")


# 商品資訊
item_name = "Python 程式設計入門"
item_price = 450
quantity = 2
discount = 0.9    # 九折

# 計算
subtotal = item_price * quantity
final_price = subtotal * discount

# 印出收據
print("======== 購物收據 ========")
print("商品：", item_name)
print("單價：", item_price, "元")
print("數量：", quantity)
print("--------------------------")
print("小計：", subtotal, "元")
print("折扣：九折")
print("應付：", final_price, "元")
print("==========================")


a = 5
b = 10

print("交換前：")
print("a =", a)
print("b =", b)

# 交換
temp = a    # 先把 a 存到 temp
a = b       # 把 b 的值給 a
b = temp    # 把 temp（原本的 a）給 b

print()
print("交換後：")
print("a =", a)
print("b =", b)

a = 5
b = 10

a, b = b, a    # 一行搞定！

print("a =", a)
print("b =", b)


# 執行: python3 第 5 課：變數與命名規則1.py

# === 預期輸出 (節錄) ===
# 學生姓名：陳小明
# ================
# 陳小明 的成績是 95 分
# 恭喜 陳小明 通過考試！
# 學生姓名： 陳小明
# ================
# 陳小明 的成績是 95 分
# 恭喜 陳小明 通過考試！
# 小明
# 18
# 我的名字是 小明
# 我今年 18 歲
# name
# 小明
# 原本的分數： 60
# 補考後的分數： 85
# 再次補考後： 100
# 13
# 7
# 30
# …（後略，完整輸出共 66 行）
