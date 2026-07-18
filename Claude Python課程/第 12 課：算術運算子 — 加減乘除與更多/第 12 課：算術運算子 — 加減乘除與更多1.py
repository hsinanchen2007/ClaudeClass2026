# 算術運算子 — 加減乘除與更多

# 數字相加
print(10 + 3)       # 13
print(1.5 + 2.3)    # 3.8
print(10 + 3.0)     # 13.0 ← int + float → float

# 字串「相加」（拼接，不是數學加法！）
print("Hello" + " " + "World")   # Hello World

# ⚠️ 數字和字串不能相加
# print(10 + "3")   # ❌ TypeError!
print(10 + int("3"))  # ✅ 13

# 數字相減

print(10 - 3)       # 7
print(1.5 - 0.3)    # 1.2
print(3 - 10)       # -7（可以得到負數）

# 負號也是用 -
x = -5
print(x)            # -5
print(-x)           # 5（負負得正）

# 數字相乘
print(10 * 3)       # 30
print(2.5 * 4)      # 10.0 ← 有 float 參與，結果就是 float

# 字串「相乘」（重複！）
print("哈" * 3)      # 哈哈哈
print("-" * 30)      # ------------------------------ （畫分隔線）
print("Go! " * 3)   # Go! Go! Go!

# ⚠️ 字串只能乘以整數
# print("Hi" * 2.5)  # ❌ TypeError! 不能乘以浮點數

# 數字相除
print(10 / 3)       # 3.3333333333333335
print(10 / 2)       # 5.0 ← 注意！不是 5，是 5.0
print(7 / 2)        # 3.5
print(1 / 3)        # 0.3333333333333333

print(10 / 2)       # 5.0（不是 5！）
print(type(10 / 2)) # <class 'float'>

print(10 / 5)       # 2.0
print(100 / 10)     # 10.0

# print(10 / 0)     # ❌ ZeroDivisionError: division by zero
# print(0 / 0)      # ❌ ZeroDivisionError

# 整數除法 //（地板除法）, 會將結果「砍掉小數部分」，得到一個整數（或是向下取整的浮點數）
print(10 / 3)       # 3.3333... ← 一般除法
print(10 // 3)      # 3         ← 整數除法，砍掉小數

print(7 / 2)        # 3.5
print(7 // 2)       # 3

print(10 / 4)       # 2.5
print(10 // 4)      # 2

# 正數的情況：看起來像「砍掉小數」
print(7 // 2)        # 3（3.5 → 往下 → 3）
print(10 // 3)       # 3（3.33 → 往下 → 3）

# ⚠️ 負數的情況：注意！往更小的方向
print(-7 // 2)       # -4（不是 -3！）
# 解釋：-7 ÷ 2 = -3.5 → 往下取整 → -4（比 -3.5 更小的整數）

print(-10 // 3)      # -4（不是 -3！）
# 解釋：-10 ÷ 3 = -3.33 → 往下取整 → -4

print(10.0 // 3)     # 3.0 ← 有 float 參與，結果是 float，但沒小數
print(7 // 2.0)      # 3.0

# 餘數運算子 %，得到除法的餘數
print(10 % 3)    # 1  （10 ÷ 3 = 3 餘 1）
print(7 % 2)     # 1  （7 ÷ 2 = 3 餘 1）
print(8 % 4)     # 0  （8 ÷ 4 = 2 餘 0，整除！）
print(15 % 6)    # 3  （15 ÷ 6 = 2 餘 3）
print(3 % 5)     # 3  （3 ÷ 5 = 0 餘 3）

# 用 Python 驗證, 整數除法和餘數運算的關係：
a = 10
b = 3
print(a // b)            # 3（商）
print(a % b)             # 1（餘數）
print(b * (a // b) + a % b)  # 10 ← 完美還原！

# 餘數運算子 % 的一個重要用途：判斷奇偶數
num = 7
print(num % 2)      # 1 → 奇數（除以 2 有餘數）

num = 8
print(num % 2)      # 0 → 偶數（除以 2 沒餘數）

# 判斷公式：
# 餘數是 0 → 偶數
# 餘數是 1 → 奇數

# 某數能被 5 整除嗎？
print(15 % 5)       # 0 → 可以整除 ✅
print(17 % 5)       # 2 → 不能整除 ❌

# 判斷閏年（簡化版）：能被 4 整除
year = 2024
print(year % 4)     # 0 → 2024 能被 4 整除

# 讓數字在 0~2 之間循環：0, 1, 2, 0, 1, 2, 0...
for i in range(9):
    print(i % 3, end=" ")
# 輸出：0 1 2 0 1 2 0 1 2

# 餘數運算子 % 的另一個重要用途：取出數字的某幾位數
num = 12345
print(num % 10)     # 5（取個位數）
print(num % 100)    # 45（取最後兩位數）

# 指數運算子 **，用來計算「次方」
# 基本用法
print(2 ** 3)        # 8     （2³ = 2×2×2 = 8）
print(5 ** 2)        # 25    （5² = 5×5 = 25）
print(10 ** 4)       # 10000 （10⁴ = 10000）
print(3 ** 0)        # 1     （任何數的 0 次方都是 1）

# 小數次方
print(9 ** 0.5)      # 3.0   （開根號！9 的 0.5 次方 = √9 = 3）
print(27 ** (1/3))   # 3.0   （開三次根號！27^(1/3) = ∛27 = 3）

# 負的次方
print(2 ** -1)       # 0.5   （2⁻¹ = 1/2 = 0.5）
print(2 ** -3)       # 0.125 （2⁻³ = 1/8 = 0.125）

# 綜合應用：計算圓的面積、複利等
# 計算圓的面積：π × r²
import math
radius = 5
area = math.pi * radius ** 2
print(f"圓的面積：{area:.2f}")   # 圓的面積：78.54

# 計算複利：本金 × (1 + 利率) ^ 年數
principal = 10000       # 本金 1 萬元
rate = 0.05             # 年利率 5%
years = 10              # 存 10 年
result = principal * (1 + rate) ** years
print(f"10 年後的金額：{result:.2f} 元")
# 10 年後的金額：16288.95 元

# 規則：只要有一邊是 float，結果就是 float
print(10 + 3)        # 13    ← int + int = int
print(10 + 3.0)      # 13.0  ← int + float = float
print(10.0 + 3.0)    # 13.0  ← float + float = float

# 唯一例外：/ 除法永遠回傳 float
print(10 / 2)        # 5.0   ← 即使整除也是 float

# 注意：浮點數的精確度問題
print(0.1 + 0.2)     # 0.30000000000000004 ← 不是 0.3！😱
print(0.1 + 0.2 == 0.3)  # False！

# 方法 1：用 round() 四捨五入
print(round(0.1 + 0.2, 1))      # 0.3 ✅

# 方法 2：比較時用 round
print(round(0.1 + 0.2, 10) == 0.3)  # True ✅

# ===== 時間轉換器 =====
total_seconds = int(input("請輸入總秒數："))

# 計算時、分、秒
hours = total_seconds // 3600       # 1 小時 = 3600 秒
remaining = total_seconds % 3600    # 扣掉整時數後剩餘的秒數
minutes = remaining // 60           # 剩餘秒數中有幾個完整的分鐘
seconds = remaining % 60            # 最後剩餘的秒數

print(f"{total_seconds} 秒 = {hours} 小時 {minutes} 分鐘 {seconds} 秒")

# ===== 貨幣換算器 =====
# ===== 硬幣找零計算器 =====
change = int(input("請輸入找零金額（元）："))

# 從大幣值開始算
coins_50 = change // 50
change = change % 50            # 扣掉 50 元硬幣後剩餘的金額

coins_10 = change // 10
change = change % 10

coins_5 = change // 5
change = change % 5

coins_1 = change                # 剩下的都用 1 元

print(f"50 元硬幣：{coins_50} 個")
print(f"10 元硬幣：{coins_10} 個")
print(f"5 元硬幣：{coins_5} 個")
print(f"1 元硬幣：{coins_1} 個")

# ===== 複利計算器 =====
principal = float(input("本金（元）："))
rate = float(input("年利率（%）：")) / 100   # 把百分比轉成小數
years = int(input("存款年數："))

# 複利公式：本金 × (1 + 利率) ^ 年數
final = principal * (1 + rate) ** years
profit = final - principal

print()
print(f"本金：{principal:,.0f} 元")
print(f"年利率：{rate:.2%}")
print(f"存款年數：{years} 年")
print(f"最終金額：{final:,.2f} 元")
print(f"獲利：{profit:,.2f} 元")

# 比較除法和整數除法的差異
print(7 / 2)     # 3.5  ← 一般除法（有小數）
print(7 // 2)    # 3    ← 整數除法（砍掉小數）

# 注意：除以 0 會引發錯誤
x = 0
# print(10 / x)   # ❌ ZeroDivisionError
# print(10 // x)  # ❌ ZeroDivisionError
# print(10 % x)   # ❌ ZeroDivisionError

# + 對字串是拼接
print("3" + "4")     # "34"（不是 7！）

# * 對字串是重複
print("3" * 4)       # "3333"（不是 12！）

# 浮點數的精確度問題
print(0.1 + 0.2)             # 0.30000000000000004
print(round(0.1 + 0.2, 1))   # 0.3 ✅ 用 round() 解決

# 綜合應用
print(15 // 4)
print(15 % 4)
print(2 ** 10)
print(10 / 3)
print(10 // 3)
print(10 % 3)
print(7 ** 0)
print(9 ** 0.5)





