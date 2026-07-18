# 整數（int）是用來表示沒有小數部分的數字，可以是正整數、負整數或零。以下是一些整數的例子：

# 正整數
age = 18
year = 2024
population = 23000000

# 負整數
temperature = -5
debt = -1000

# 零
count = 0

# 整數在 Python 中是非常常見的數據類型，可以用於各種計算和操作。你可以使用整數進行加法、減法、乘法、除法等運算，並且整數在 Python 中沒有大小限制，只受限於可用的內存。
# 以下是一些整數的使用示例：
# type() 函數可以用來檢查變量的數據類型，對於整數來說，type() 會返回 <class 'int'>。
age = 18
print(age)
print(type(age))

# Python 中的整數沒有大小限制，可以表示非常大的數字。以下是一個例子：
big_number = 99999999999999999999999999999999999999
print(big_number)
print(type(big_number))

# 在 Python 中，為了提高數字的可讀性，可以使用底線（_）來分隔數字的位數。這樣可以讓大數字更容易閱讀，而不會影響數值本身。例如：
# 不好閱讀
population = 23000000

# 好閱讀（底線不影響數值）
population = 23_000_000

print(population)

# 浮點數（float）是用來表示有小數部分的數字，可以是正浮點數、負浮點數或零。以下是一些浮點數的例子：

# 一般小數
pi = 3.14159
height = 175.5
price = 99.99

# 負數小數
temperature = -12.5

# 整數寫成小數形式
score = 100.0

# 浮點數在 Python 中也是非常常見的數據類型，可以用於各種計算和操作。你可以使用浮點數進行加法、減法、乘法、除法等運算，
# 並且浮點數在 Python 中的精度通常是雙精度（double precision），可以表示非常大的或非常小的數字。
# type() 函數可以用來檢查變量的數據類型，對於浮點數來說，type() 會返回 <class 'float'>。
height = 175.5
print(height)
print(type(height))

# 浮點數可以表示非常大的或非常小的數字，以下是一些例子：

# 一般寫法
speed_of_light = 300000000.0        # 光速（公尺/秒）
tiny_number = 0.000000001           # 很小的數

# 科學記號寫法
speed_of_light = 3e8                # 3 × 10^8
tiny_number = 1e-9                  # 1 × 10^-9

print(speed_of_light)
print(tiny_number)

# 在 Python 中，整數和浮點數可以進行混合運算，結果會根據運算的類型自動轉換為適當的類型。例如：
a = 10      # int
b = 3.0     # float

result = a + b
print(result)
print(type(result))

# 在 Python 中，除法運算符（/）會返回一個浮點數，即使兩個操作數都是整數。例如：
result = 10 / 2
print(result)
print(type(result))

# 在 Python 中，整數和浮點數之間可以進行類型轉換。你可以使用 int() 函數將浮點數轉換為整數，這會截斷小數部分，而不是四捨五入。例如：
price = 99.87

price_int = int(price)
print(price_int)
print(type(price_int))

print(int(3.9))     # 捨去，得到 3
print(int(-3.9))    # 捨去，得到 -3

# 你也可以使用 float() 函數將整數轉換為浮點數。例如：
age = 18
age_float = float(age)
print(age_float)
print(type(age_float))  

# 在 Python 中，整數和浮點數之間的運算會根據運算的類型自動轉換為適當的類型。例如：
print(10 + 5)       # 15
print(3.5 + 2.5)    # 6.0
print(10 + 3.5)     # 13.5（int + float = float）

print(10 + 5)       # 15
print(3.5 + 2.5)    # 6.0
print(10 + 3.5)     # 13.5（int + float = float）

print(4 * 3)        # 12
print(2.5 * 4)      # 10.0
print(7 * 0)        # 0

print(10 / 3)       # 3.3333333333333335
print(10 / 2)       # 5.0（即使除得盡也是 float）
print(7 / 2)        # 3.5

print(10 // 3)      # 3（不是 3.33...）
print(7 // 2)       # 3（不是 3.5）
print(15 // 4)      # 3（不是 3.75）

print(10 % 3)       # 1（10 = 3×3 + 1）
print(7 % 2)        # 1（7 = 2×3 + 1）
print(15 % 4)       # 3（15 = 4×3 + 3）
print(12 % 4)       # 0（整除沒有餘數）

# 你可以使用整數和浮點數進行各種運算，並且 Python 會根據運算的類型自動轉換為適當的類型。以下是一個例子，判斷一個數字是奇數還是偶數：
number = 17

remainder = number % 2

if remainder == 0:
    print("偶數")
else:
    print("奇數")

# 取餘數運算符（%）可以用來獲取兩個數字相除後的餘數。這在許多情況下非常有用，例如判斷一個數字是否是某個數的倍數，或者在循環中實現周期性行為。例如：
# 0, 1, 2, 0, 1, 2, 0, 1, 2...
print(0 % 3)    # 0
print(1 % 3)    # 1
print(2 % 3)    # 2
print(3 % 3)    # 0
print(4 % 3)    # 1
print(5 % 3)    # 2

# 在 Python 中，還有一個指數運算符（**），用於計算一個數字的幾次方。例如：
print(2 ** 3)       # 8（2 的 3 次方 = 2×2×2）
print(5 ** 2)       # 25（5 的平方）
print(10 ** 0)      # 1（任何數的 0 次方 = 1）
print(2 ** 10)      # 1024

# 指數運算符（**）也可以用來計算平方根，通過將指數設置為 0.5。例如：
# 計算平方
side = 5
area = side ** 2
print("正方形面積：", area)    # 25

# 計算立方
side = 3
volume = side ** 3
print("立方體體積：", volume)  # 27

# 計算平方根（0.5 次方）
number = 16
sqrt = number ** 0.5
print("16 的平方根：", sqrt)   # 4.0

# 在 Python 中，運算符的優先順序決定了表達式中各個運算的計算順序。以下是一些常見的運算符及其優先順序（從高到低）：
# 1. 括號（()） - 用於改變運算順序，括號內的運算會先被計算。
# 2. 指數（**） - 用於計算幾次方。
# 3. 乘法（*）、除法（/）、取餘數（%）和整除（//） - 這些運算具有相同的優先順序，從左到右計算。
# 4. 加法（+）和減法（-） - 這些運算具有相同的優先順序，從左到右計算。
# 以下是一些示例，展示了運算符的優先順序：
result = 2 + 3 * 4
print(result)    # 14（先乘後加）

result = (2 + 3) * 4
print(result)    # 20（括號優先）

result = 2 ** 3 * 2
print(result)    # 16（先次方，再乘）

result = 10 - 3 - 2
print(result)    # 5（同優先順序，從左到右）

# 可能搞混
result = 10 + 20 * 3 / 2

# 加上括號，更清楚
result = 10 + ((20 * 3) / 2)
print(result)    # 40.0

# 在 Python 中，浮點數的表示方式可能會導致一些看似奇怪的結果，這是因為浮點數在計算機中是以二進制形式存儲的，
# 而某些十進制數字無法精確地表示為二進制。這可能會導致一些運算結果看起來不太對。例如：
result = 0.1 + 0.2
print(result)

# 這是因為 0.1 和 0.2 在二進制中無法精確表示，導致計算結果有微小的誤差。這種情況在浮點數運算中是很常見的，
# 並且不是 Python 特有的問題，而是所有使用二進制浮點數表示的編程語言都會遇到的問題。
print(0.1 + 0.2)              # 0.30000000000000004
print(0.1 + 0.1 + 0.1 - 0.3)  # 5.551115123125783e-17（不是 0！）
print(0.7 + 0.1)              # 0.7999999999999999

# 如果你需要更精確的浮點數運算，可以使用 Python 的 decimal 模組，這個模組提供了更高精度的十進制浮點數類型。例如：
# from decimal import Decimal
# round(數字, 小數位數)
print(round(3.14159, 2))     # 3.14
print(round(3.14159, 3))     # 3.142
print(round(3.5))            # 4（不指定位數，取整數）

# 在比較浮點數時，直接使用 == 可能會導致錯誤，因為浮點數的表示方式可能會導致微小的誤差。
# 為了避免這種情況，你可以檢查兩個浮點數之間的差值是否足夠小。例如：
a = 0.1 + 0.2
b = 0.3

# ❌ 直接比較可能出錯
print(a == b)                # False

# ✅ 檢查差值是否足夠小
tolerance = 0.0001
print(abs(a - b) < tolerance)    # True

# 在處理金額等需要高精度的數字時，建議使用 decimal 模組來避免浮點數的誤差問題。例如：
from decimal import Decimal
price = Decimal('19.99')
tax = Decimal('0.15')
total = price + tax
print(total)    # 20.14

# 處理金額時，用「分」而不是「元」
price_cents = 150          # 1.50 元 = 150 分
tax_cents = 15             # 0.15 元 = 15 分
total_cents = price_cents + tax_cents    # 165 分

total_dollars = total_cents / 100        # 1.65 元
print(total_dollars)

# Python 提供了許多內建的數學函數，可以用來進行各種數學運算。以下是一些常見的數學函數：
# abs()：絕對值
print(abs(-10))         # 10
print(abs(5))           # 5
print(abs(-3.14))       # 3.14

# round()：四捨五入
print(round(3.4))       # 3
print(round(3.5))       # 4
print(round(3.14159, 2))  # 3.14

# max()：找最大值
print(max(10, 20, 5))   # 20
print(max(-1, -5, -3))  # -1

# min()：找最小值
print(min(10, 20, 5))   # 5
print(min(-1, -5, -3))  # -5

# pow()：次方（和 ** 一樣）
print(pow(2, 3))        # 8
print(pow(10, 2))       # 100

# math 模組提供了更多的數學函數，例如三角函數、對數、指數等。要使用 math 模組，你需要先導入它。例如：
import math

# 圓周率
print(math.pi)              # 3.141592653589793

# 自然對數的底 e
print(math.e)               # 2.718281828459045

# 平方根
print(math.sqrt(16))        # 4.0
print(math.sqrt(2))         # 1.4142135623730951

# 無條件捨去
print(math.floor(3.7))      # 3
print(math.floor(-3.2))     # -4

# 無條件進位
print(math.ceil(3.2))       # 4
print(math.ceil(-3.7))      # -3

# 取整數部分
print(math.trunc(3.7))      # 3
print(math.trunc(-3.7))     # -3

# 三角函數（弧度）
print(math.sin(math.pi / 2))  # 1.0
print(math.cos(0))            # 1.0

# 對數
print(math.log(math.e))     # 1.0（自然對數）
print(math.log10(100))      # 2.0（以 10 為底）

# 指數
import math

radius = 5    # 半徑

area = math.pi * radius ** 2           # 面積 = π × r²
circumference = 2 * math.pi * radius   # 周長 = 2 × π × r

print("======== 圓的計算 ========")
print("半徑：", radius)
print("面積：", round(area, 2))
print("周長：", round(circumference, 2))
print("==========================")

# 溫度轉換
# 攝氏轉華氏：F = C × 9/5 + 32
# 華氏轉攝氏：C = (F - 32) × 5/9

celsius = 28

fahrenheit = celsius * 9 / 5 + 32

print("======== 溫度轉換 ========")
print("攝氏：", celsius, "°C")
print("華氏：", round(fahrenheit, 1), "°F")
print("==========================")

# 把秒數轉換成 時:分:秒
total_seconds = 3725

hours = total_seconds // 3600
remaining = total_seconds % 3600
minutes = remaining // 60
seconds = remaining % 60

print("======== 時間轉換 ========")
print("總秒數：", total_seconds, "秒")
print("轉換後：", hours, "時", minutes, "分", seconds, "秒")
print("==========================")

# BMI 計算
height_cm = 175    # 身高（公分）
weight_kg = 70     # 體重（公斤）

# 身高轉換成公尺
height_m = height_cm / 100

# BMI = 體重 / 身高²
bmi = weight_kg / (height_m ** 2)

print("======== BMI 計算 ========")
print("身高：", height_cm, "公分")
print("體重：", weight_kg, "公斤")
print("BMI：", round(bmi, 1))
print("==========================")

