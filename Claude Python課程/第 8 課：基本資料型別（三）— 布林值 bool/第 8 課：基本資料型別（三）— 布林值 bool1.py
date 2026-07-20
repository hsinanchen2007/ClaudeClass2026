# 布林值（Boolean）是一種基本資料類型，表示真（True）或假（False）的值。在Python中，布林值通常用於條件判斷和邏輯運算。

# ═══════════════════════════════════════════════════════════════════════════
# 【面試題】布林值 bool
# ───────────────────────────────────────────────────────────────────────────
# 🔥 Q1. bool 在 Python 中是什麼型別？
#     答：`bool` 是 **`int` 的子類**。實測 `isinstance(True, int)` → True、
#     `True + True` → 2、`False == 0` → True。
#     所以布林值可以直接參與算術，`sum([True, False, True])` 就是在數 True 的個數。
#     追問：`1 is True` 呢？（False——`is` 比的是身分，且兩者型別不同）
#
# 🔥 Q2. 哪些值是 falsy？
#     答：實測 `bool()` 為 False 的有：`0`、`0.0`、`''`、`[]`、`{}`、`set()`、
#     `None`、`False`（還有 `0j`、`()`，以及自訂 `__bool__`／`__len__` 回傳假值的物件）。
#     其餘一律 truthy。
#     追問：怎麼判斷一個物件是不是 falsy？（`bool(x)` 或直接 `if x:`）
#
# ⚠️ 陷阱. `bool('False')` 是什麼？
#     答：實測 **True**。`bool('0')` 也是 **True**。
#     字串的真假只看**長度**——只有空字串 `''` 才是 falsy，內容是什麼完全不影響。
#     為什麼會錯：把字串「內容的語意」誤當成「布林值」，
#     於是寫出 `if input("y/n"):` 這種永遠成立的判斷。
# ═══════════════════════════════════════════════════════════════════════════

is_student = True
is_raining = False

print(is_student)   # 輸出：True
print(is_raining)   # 輸出：False

# 布林值在Python中是大小寫敏感的，必須使用大寫的True和False來表示布林值。
a = True
print(type(a))   # 輸出：<class 'bool'>

b = False
print(type(b))   # 輸出：<class 'bool'>

# 布林值可以用於條件判斷，例如if語句中。
# 比較數字
print(5 > 3)      # True （5 大於 3 嗎？是的！）
print(5 < 3)      # False（5 小於 3 嗎？不是！）
print(5 == 5)     # True （5 等於 5 嗎？是的！）
print(5 != 3)     # True （5 不等於 3 嗎？是的！）
print(5 >= 5)     # True （5 大於或等於 5 嗎？是的！）
print(5 <= 3)     # False（5 小於或等於 3 嗎？不是！）

# = and == 是不同的運算符號，請不要混淆！
# = 是「賦值」，把右邊的值存到左邊的變數
x = 10          # 把 10 存進 x

# == 是「比較」，檢查左右兩邊是否相等
print(x == 10)  # True（x 的值等於 10 嗎？是的！）
print(x == 5)   # False（x 的值等於 5 嗎？不是！）

# 布林值也可以用於比較字串。
print("apple" == "apple")   # True
print("apple" == "Apple")   # False（大小寫不同！）
print("abc" < "abd")        # True（按字母順序比較）

# 在Python中，以下的值會被視為False（假），其他的值都會被視為True（真）。
print(bool(0))        # False ← 數字 0
print(bool(0.0))      # False ← 浮點數 0.0
print(bool(""))       # False ← 空字串
print(bool(None))     # False ← None（什麼都沒有）
print(bool([]))       # False ← 空串列（之後會學到）
print(bool(False))    # False ← False 本身

# 其他的值都會被視為 True（真）。
print(bool(1))        # True ← 非零數字
print(bool(-5))       # True ← 負數也是 True！
print(bool(3.14))     # True ← 非零浮點數
print(bool("hello"))  # True ← 有內容的字串
print(bool(" "))      # True ← 連空格也算有內容！
print(bool(True))     # True ← True 本身

# 布林值在Python中也可以參與數學運算，True會被視為1，False會被視為0。
print(True + True)    # 2  （1 + 1）
print(True + False)   # 1  （1 + 0）
print(False + False)  # 0  （0 + 0）
print(True * 10)      # 10 （1 * 10）
print(True == 1)      # True
print(False == 0)     # True

# 計算有多少個 True
results = [True, False, True, True, False]
print(sum(results))   # 3（有 3 個 True）

# 布林值在條件判斷中非常有用，可以幫助我們根據不同的情況執行不同的程式碼。
age = 20
is_adult = age >= 18   # is_adult 會是 True

if is_adult:
    print("你已經成年了！")
else:
    print("你還未成年。")

# 輸出：你已經成年了！

# 下面是一個模擬登入的例子，使用布林值來表示使用者是否已經登入。
is_logged_in = False

# 模擬登入
password = "1234"
if password == "1234":
    is_logged_in = True

print(is_logged_in)   # True

# 最後來看看一個簡單的遊戲迴圈，使用布林值來控制遊戲是否結束。
game_over = False

# 這段先看看就好，之後會詳細學
while not game_over:
    print("遊戲進行中...")
    game_over = True    # 改成 True 就會停止

# 練習：判斷以下表達式的布林值。
print(10 > 5)
print(3 == 3.0)
print("hello" == "Hello")
print(bool(0))
print(bool("False"))     # 注意：這是字串 "False"！
print(True + True + False)


# 執行: python3 第 8 課：基本資料型別（三）— 布林值 bool1.py

# === 預期輸出 (節錄) ===
# True
# False
# <class 'bool'>
# <class 'bool'>
# True
# False
# True
# True
# True
# False
# True
# False
# True
# False
# True
# False
# False
# False
# False
# False
# …（後略，完整輸出共 43 行）
