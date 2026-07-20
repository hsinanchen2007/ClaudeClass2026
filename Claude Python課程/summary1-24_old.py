# ╔════════════════════════════════════════════════════════════════╗
# ║   Python 基礎語法完整總結（第 1 ~ 24 課）                    ║
# ║   涵蓋：資料型別、運算子、流程控制、迴圈                     ║
# ║   學完這些，即可進入 Data Structure 的學習                    ║
# ╚════════════════════════════════════════════════════════════════╝

# ═══════════════════════════════════════════════════════════════════════════
# 【面試題】Python 基礎總複習（第 1 ~ 24 課）
# ───────────────────────────────────────────────────────────────────────────
# 🔥 Q1. Python 是編譯型還是直譯型？是動態型別還是強型別？
#     答：先編譯成 bytecode 再由虛擬機直譯，說「純直譯」不精確。
#     型別上是**動態型別 + 強型別**——變數不需宣告型別，
#     但不做跨型別隱式轉換（實測 `1 + '1'` → TypeError）。
#     追問：那 `1 + True` 為什麼可以？（bool 是 int 的子類，不是弱型別）
#
# 🔥 Q2. `/`、`//`、`%` 對負數的行為是什麼？
#     答：`/` 恆回傳 float；`//` 是**向下取整**，實測 `-7 // 2` → **-4**；
#     `%` 的結果符號跟**除數**走，實測 `-7 % 2` → **1**（C/Java 給 -1）。
#     注意 `int(-3.99)` → **-3** 是**向零截斷**，方向與 `//` 相反。
#     追問：為什麼這樣設計？（保證 `a == (a//b)*b + a%b` 恆成立）
#
# ⚠️ 陷阱. `2 ** 3 ** 2` 與 `True or False and False` 分別是多少？
#     答：實測 **512** 與 **True**。
#     前者因為 `**` 是**右結合**（等於 `2**(3**2)`）；
#     後者因為 `and` 的優先序**高於** `or`（等於 `True or (False and False)`）。
#     為什麼會錯：預設「所有運算子都左結合、邏輯運算子地位平等」。
#     同源陷阱：`-2 ** 2` 實測是 **-4**（`**` 優先於一元負號）。
# ═══════════════════════════════════════════════════════════════════════════



# ============================================================
# 一、輸出 print()                               【第 1, 3, 4 課】
# ============================================================

# --- 基本輸出 ---
print("Hello World")                        # 印出文字，自動換行
print(123)                                   # 印出數字
print(True)                                  # 印出布林值

# --- 多個參數（自動用空格分隔） ---
print("姓名", "小明", "年齡", 18)            # 姓名 小明 年齡 18

# --- sep：自訂分隔符 ---
print(2025, 3, 1, sep="/")                   # 2025/3/1
print("A", "B", "C", sep="")                 # ABC（無分隔）

# --- end：改變結尾（預設是 \n 換行） ---
print("Hello", end=" ")
print("World")                               # Hello World（同一行）

# --- 轉義字元 ---
print("第一行\n第二行")                       # \n 換行
print("姓名\t年齡")                           # \t Tab 對齊
print("C:\\Users\\Documents")                 # \\ 顯示反斜線
print('It\'s OK')                             # \' 轉義引號

# --- 原始字串 r"..." 忽略轉義 ---
print(r"C:\new\test")                         # C:\new\test（\n 不換行）


# ============================================================
# 二、變數與命名規則                              【第 5 課】
# ============================================================

# --- 用 = 賦值 ---
name = "小明"                                 # 字串
age = 18                                      # 整數
height = 175.5                                # 浮點數
is_student = True                             # 布林值

# --- 變數可重新賦值 ---
score = 60
score = 85                                    # 覆蓋舊值

# --- 多變數同時賦值 ---
a, b, c = 1, 2, 3                             # 一行賦不同值
x = y = z = 0                                 # 多變數同值

# --- ★ 變數交換（Python 特有） ---
a, b = b, a                                   # 一行搞定交換

# --- 命名規則 ---
# ✅ snake_case（推薦）：my_name, total_score, is_passed
# ✅ 只能包含字母、數字、底線，不能以數字開頭
# ✅ 區分大小寫：name ≠ Name ≠ NAME
# ❌ 不能用保留字：if, for, while, True, False, None 等

# --- type() 查看型別 ---
print(type(name))                             # <class 'str'>
print(type(age))                              # <class 'int'>
print(type(height))                           # <class 'float'>
print(type(is_student))                       # <class 'bool'>


# ============================================================
# 三、資料型別                                【第 6, 7, 8 課】
# ============================================================

# ── 3.1 整數 int ──
age = 18
big = 23_000_000                              # 底線增加可讀性，等於 23000000
# Python 整數沒有大小限制

# ── 3.2 浮點數 float ──
pi = 3.14159
speed = 3e8                                   # 科學記號 3 × 10⁸ = 300000000.0

# ⚠️ 浮點數精度問題
print(0.1 + 0.2)                              # 0.30000000000000004（不是 0.3！）
print(round(0.1 + 0.2, 1))                    # 0.3 ← 用 round() 修正

# 金額計算推薦 Decimal
from decimal import Decimal
total = Decimal('19.99') + Decimal('0.15')     # 精確計算 → 20.14

# ── 3.3 字串 str ──
s1 = '單引號'
s2 = "雙引號"                                  # 完全等效
s3 = """三引號
可以換行"""                                    # 多行字串

# 字串不可變（immutable）
# s1[0] = "X"                                 # ❌ TypeError

# len() 長度
print(len("Hello"))                           # 5
print(len("你好"))                             # 2

# 索引（0 開始，負索引從 -1）
text = "Python"
print(text[0])                                # P（第一個）
print(text[-1])                               # n（最後一個）

# 切片 [start:end:step]（包含 start，不包含 end）
text = "Hello World"
print(text[0:5])                              # Hello
print(text[6:])                               # World
print(text[::-1])                             # dlroW olleH（★ 反轉）

# 連接 + 與重複 *
print("Hello" + " " + "World")               # Hello World
print("-" * 20)                               # --------------------
# ⚠️ 字串 + 數字 → TypeError，需用 str() 轉型

# in / not in 檢查子字串
print("Py" in "Python")                      # True
print("java" not in "Python")                # True

# 常用方法
"Hello".upper()                               # HELLO
"Hello".lower()                               # hello
"  Hi  ".strip()                              # "Hi"（去除頭尾空白）
"Hello".replace("l", "L")                     # HeLLo
"Hello".find("lo")                            # 3（找到回傳索引，找不到回傳 -1）
"Hello".count("l")                            # 2

# ── 3.4 布林值 bool ──
# 只有 True 和 False（大寫開頭！）
print(5 > 3)                                  # True
print(5 == 3)                                 # False

# ★ Falsy 值（轉成 False）：
#   0, 0.0, "", None, [], False
# ★ Truthy 值（轉成 True）：其他所有值
print(bool(0))                                # False
print(bool(""))                               # False
print(bool("0"))                              # True ← ⚠️ 字串 "0" 不是空的！
print(bool("False"))                          # True ← ⚠️ 字串 "False" 也不是空的！

# 布林值可參與運算（True=1, False=0）
print(True + True)                            # 2
print(sum([True, False, True]))               # 2（計算 True 的數量）


# ============================================================
# 四、型別轉換                                    【第 9 課】
# ============================================================

# --- int()：轉整數 ---
print(int("123"))                             # 123
print(int(3.9))                               # 3 ← ⚠️ 截斷，不是四捨五入！
print(int(True))                              # 1
# int("3.14")                                 # ❌ ValueError（有小數點不行）
print(int(float("3.14")))                     # 3 ← ✅ 先 float 再 int

# --- float()：轉浮點數 ---
print(float("3.14"))                          # 3.14
print(float(5))                               # 5.0

# --- str()：轉字串（最安全，什麼都能轉） ---
print(str(123))                               # "123"
print(str(True))                              # "True"

# --- bool()：轉布林值 ---
print(bool(0))                                # False
print(bool("hello"))                          # True

# ★ 最常用：input() 永遠回傳字串，要做計算必須轉型
age = int(input("請輸入年齡："))               # 一行搞定
height = float(input("請輸入身高："))


# ============================================================
# 五、輸入與格式化                                【第 10 課】
# ============================================================

# --- input() 取得輸入（永遠回傳 str） ---
name = input("你的名字：")
name = input("你的名字：").strip()             # ★ 加 .strip() 去除空格

# --- ★★★ f-string（最推薦的格式化方式） ---
name = "小明"
age = 18
print(f"我叫{name}，今年{age}歲")              # 我叫小明，今年18歲

# f-string 中可放表達式
a, b = 10, 3
print(f"{a} + {b} = {a + b}")                 # 10 + 3 = 13
print(f"{a} ÷ {b} = {a / b:.2f}")             # 10 ÷ 3 = 3.33（保留兩位小數）

# 格式化數字
print(f"{3.14159:.2f}")                        # 3.14（保留 2 位小數）
print(f"{0.85:.0%}")                           # 85%（百分比格式）
print(f"{42:05d}")                             # 00042（補零到 5 位）


# ============================================================
# 六、註解                                        【第 11 課】
# ============================================================

# 單行註解：# 開頭
x = 10    # 行尾註解

# 多行註解：用 ''' 或 """
"""
這是多行註解
可以寫很多行
"""

# 常用標記
# TODO: 之後要實作的功能
# FIXME: 這裡有已知的 bug 需要修復

# ★ 好註解解釋「為什麼」，壞註解是廢話
# ❌ x = 10  # 把 10 存到 x（廢話）
# ✅ x = 10  # 預設值，用於測試


# ============================================================
# 七、算術運算子                                  【第 12 課】
# ============================================================

print(10 + 3)       # 13   加法
print(10 - 3)       # 7    減法
print(10 * 3)       # 30   乘法
print(10 / 3)       # 3.33 除法（★ 永遠回傳 float！）
print(10 // 3)      # 3    整除（地板除法，捨去小數）
print(10 % 3)       # 1    取餘數
print(2 ** 3)       # 8    指數（2³）

# ★ int + float → float
print(10 + 3.0)     # 13.0

# ★ / 即使整除也回傳 float
print(10 / 2)       # 5.0（不是 5）

# ⚠️ 負數整除往更小方向取整
print(-7 // 2)      # -4（不是 -3！）

# ⚠️ 除以 0 → ZeroDivisionError
# print(10 / 0)     # ❌

# % 常見用途
print(17 % 2)       # 1 → 奇數（餘數 0 = 偶數）
print(12345 % 10)   # 5 → 取個位數
for i in range(6):
    print(i % 3, end=" ")  # 0 1 2 0 1 2 → 循環
print()

# 開根號
print(9 ** 0.5)     # 3.0（√9）

# 字串的 + 和 * 不是數學運算
print("3" + "4")    # "34"（拼接）
print("3" * 4)      # "3333"（重複）

# 常用數學函數
print(abs(-10))          # 10（絕對值）
print(round(3.14159, 2)) # 3.14（四捨五入）
print(max(10, 20, 5))    # 20（最大值）
print(min(10, 20, 5))    # 5（最小值）

import math
print(math.sqrt(16))     # 4.0（平方根）
print(math.floor(3.7))   # 3（無條件捨去）
print(math.ceil(3.2))    # 4（無條件進位）


# ============================================================
# 八、比較運算子                                  【第 13 課】
# ============================================================

# 結果都是布林值
print(5 == 5)       # True   等於
print(5 != 3)       # True   不等於
print(5 > 3)        # True   大於
print(5 < 3)        # False  小於
print(5 >= 5)       # True   大於等於
print(5 <= 5)       # True   小於等於

# ⚠️ = 賦值 vs == 比較
x = 10              # 賦值
print(x == 10)      # 比較 → True

# ⚠️ 符號順序不能反：>= ✅  => ❌

# int 和 float 可直接比較
print(1 == 1.0)     # True

# 字串比較：按 ASCII 編碼，大小寫敏感
print("apple" < "banana")   # True
print("apple" == "Apple")   # False ← 大小寫不同！
# 數字(48-57) < 大寫(65-90) < 小寫(97-122)

# ⚠️ 字串和數字不能比大小
# print("5" > 3)    # ❌ TypeError

# ★ 連鎖比較（Python 特有）
x = 5
print(1 < x < 10)           # True（等同 1 < x and x < 10）
print(18 <= age <= 65)       # 範圍判斷

# 浮點數比較要小心
print(0.1 + 0.2 == 0.3)     # False 😱
print(round(0.1 + 0.2, 10) == round(0.3, 10))  # True ✅


# ============================================================
# 九、邏輯運算子                                  【第 14 課】
# ============================================================

# and：兩邊都 True 才 True（全部滿足）
print(True and True)         # True
print(True and False)        # False

# or：至少一邊 True 就 True（任一滿足）
print(True or False)         # True
print(False or False)        # False

# not：取反
print(not True)              # False
print(not False)             # True

# 優先順序：not > and > or（建議加括號！）
print(not True and False)            # False  → (not True) and False
print(True or True and False)        # True   → True or (True and False)

# ★ 短路求值
# and：第一個 False → 直接回傳，不看第二個
# or ：第一個 True  → 直接回傳，不看第二個
x = 0
if x != 0 and 10 / x > 2:           # x==0 時 and 短路 → 不會除以零
    print("OK")

# ★ and/or 回傳值不一定是布林值！
print(1 and 2)               # 2  （都 Truthy → 回傳最後一個）
print(0 and 2)               # 0  （0 是 Falsy → 停在 0）
print(0 or "hi")             # "hi"（0 Falsy → 回傳 "hi"）
print("" or "預設值")         # "預設值"（空字串 Falsy）

# ★ 用 or 設預設值（超實用！）
name = input("名字：").strip() or "匿名用戶"


# ============================================================
# 十、賦值運算子                                  【第 15 課】
# ============================================================

x = 10                       # 基本賦值

# ★ 複合賦值運算子
x += 5       # x = x + 5  → 15（最常用！）
x -= 3       # x = x - 3  → 12
x *= 2       # x = x * 2  → 24
x /= 4       # x = x / 4  → 6.0（⚠️ 永遠是 float！）
x //= 2      # x = x // 2 → 3.0
x %= 2       # x = x % 2  → 1.0
x **= 3      # x = x ** 3 → 1.0

# += 也適用於字串
msg = "Hello"
msg += " World"              # "Hello World"

# ⚠️ += 和 =+ 不一樣！
x = 10
x += 5       # x = 15 ← 累加
# x =+ 5     # x = +5 = 5 ← 賦正值（不是加法！）

# ⚠️ 使用 += 前要先初始化
total = 0                    # 先初始化
total += 100                 # 才能 +=


# ============================================================
# 十一、運算子優先順序                            【第 16 課】
# ============================================================

# ★ 由高到低：
# 1. ()        括號 — 最優先
# 2. **        指數（★ 從右到左！）
# 3. +x -x     正負號
# 4. * / // %  乘除取餘 — 同級左到右
# 5. + -       加減 — 同級左到右
# 6. == != > < >= <=  比較
# 7. not
# 8. and
# 9. or
# 10. =        賦值（最後）

print(2 + 3 * 4)             # 14（先乘後加）
print((2 + 3) * 4)           # 20（括號優先）

# ⚠️ ** 從右到左計算！
print(2 ** 3 ** 2)           # 512 → 2 ** (3**2) = 2⁹

# ⚠️ ** 比負號優先！
print(-3 ** 2)               # -9 → -(3²)
print((-3) ** 2)             # 9  → (-3)²

# 先算術 → 再比較 → 最後邏輯
result = 3 + 2 > 4 and 10 - 5 == 5
# → (5 > 4) and (5 == 5) → True and True → True

# ★ 最重要的規則：有疑慮就加括號！


# ============================================================
# 十二、條件判斷 if                          【第 17, 18, 19 課】
# ============================================================

# ── 12.1 基本 if（條件為 True 時執行） ──
age = 20
if age >= 18:
    print("成年了")                            # ← 縮排 4 格，屬於 if
print("這行不管如何都執行")                     # ← 沒有縮排，不屬於 if

# ★ 縮排是 Python 核心語法！標準 4 格空格
# ⚠️ if 結尾要加冒號 :

# ── 12.2 if-else（二選一） ──
if age >= 18:
    print("成年")
else:                                          # ⚠️ else 後面不能加條件！
    print("未成年")

# ── 12.3 if-elif-else（多重條件） ──
# ★ 從上到下判斷，命中一個就停止
# ★ 條件從最嚴格開始！
score = 85
if score >= 90:
    print("A")
elif score >= 80:                              # ★ 命中這個就停止
    print("B")
elif score >= 70:
    print("C")
elif score >= 60:
    print("D")
else:
    print("F")

# ⚠️ 多個獨立 if vs if-elif 差異超大！
score = 95
# 獨立 if → 每個都判斷 → 輸出 A, B, C（全命中）
# if-elif → 命中就停 → 只輸出 A

# ── 12.4 用 in 做多值匹配 ──
month = 3
if month in [3, 4, 5]:
    print("春天")

# ── 12.5 巢狀 if ──
# 先判斷外層，成立後再判斷內層
username = "admin"
if username == "admin":
    password = input("密碼：")
    if password == "1234":
        print("登入成功")
    else:
        print("密碼錯誤")          # ← 能區分哪個錯
else:
    print("查無此帳號")

# ★ 巢狀 vs and 的選擇：
# 需要區分錯誤原因 → 巢狀
# 只需要知道通過/不通過 → and

# ── 12.6 ★ 扁平化技巧（避免巢狀太深） ──
# 用 if-elif 反向排除不符合的條件
has_ticket = True  # 補上示範用的值（原版未定義，執行到這裡會 NameError）
height = 150
if not has_ticket:
    print("請購票")
elif age < 12:
    print("年齡不足")
elif height < 140:
    print("身高不足")
else:
    print("歡迎搭乘！")

# ── 12.7 Truthy/Falsy 直接當條件 ──
name = "小明"
if name:                     # 非空字串 → True
    print(f"你好，{name}")

items = []
if not items:                # 空串列 → False → not False → True
    print("清單是空的")


# ============================================================
# 十三、for 迴圈                                  【第 20 課】
# ============================================================

# 語法：for 變數 in 可迭代物件:

# --- 遍歷列表 ---
fruits = ["蘋果", "香蕉", "櫻桃"]
for fruit in fruits:
    print(fruit)

# --- 遍歷字串 ---
for char in "Python":
    print(char, end=" ")     # P y t h o n
print()

# --- 不需要迴圈變數時用 _ ---
for _ in range(3):
    print("重複三次")

# --- for + if ---
for score in [85, 47, 93]:
    if score >= 60:
        print(f"{score} 及格")
    else:
        print(f"{score} 不及格")

# ★★★ 三大經典模式 ★★★

# 模式 1：累加器（求和）
total = 0                    # ← 先初始化為 0
for num in [10, 20, 30]:
    total += num
print(total)                 # 60

# 模式 2：計數器（計數）
count = 0                    # ← 先初始化為 0
for score in [85, 47, 93, 55]:
    if score >= 60:
        count += 1
print(f"及格 {count} 人")    # 及格 2 人

# 模式 3：追蹤器（找最大/最小值）
nums = [34, 67, 12, 89]
max_val = nums[0]            # ← 先假設第一個是最大
for n in nums:
    if n > max_val:
        max_val = n
print(max_val)               # 89

# --- 用索引遍歷（需要索引時） ---
names = ["小明", "小華"]
scores = [85, 72]
for i in range(len(names)):
    print(f"{names[i]}：{scores[i]} 分")


# ============================================================
# 十四、range() 函數                              【第 21 課】
# ============================================================

# range(stop)           → 0 到 stop-1
# range(start, stop)    → start 到 stop-1
# range(start, stop, step) → 每次跳 step
# ★ 永遠不包含 stop！

list(range(5))           # [0, 1, 2, 3, 4]
list(range(1, 6))        # [1, 2, 3, 4, 5]
list(range(0, 10, 2))    # [0, 2, 4, 6, 8]（偶數）
list(range(1, 10, 2))    # [1, 3, 5, 7, 9]（奇數）

# 反向序列：步長用負數
list(range(5, 0, -1))    # [5, 4, 3, 2, 1]

# ⚠️ 陷阱
# range(1, 5)  → 1,2,3,4（沒有 5！要寫 range(1, 6)）
# range(1, 10, -1) → 空（方向與 step 不一致）
# range(1, 5, 0)   → ValueError（step 不能為 0）

# range() 是物件，不是列表
print(type(range(5)))    # <class 'range'>
print(list(range(5)))    # 用 list() 轉成列表


# ============================================================
# 十五、while 迴圈                                【第 22 課】
# ============================================================

# ★ while 三要素：① 初始化 ② 條件 ③ 更新

count = 5                   # ① 初始化
while count > 0:            # ② 條件
    print(count)
    count -= 1              # ③ 更新（⚠️ 忘了 = 無窮迴圈！）

# --- 輸入驗證（重複直到正確） ---
password = input("密碼：")
while password != "1234":
    print("錯誤！")
    password = input("密碼：")   # ← 迴圈內也要 input！
print("正確！")

# --- while True + break（最常見模式） ---
while True:
    cmd = input("指令（quit 離開）：")
    if cmd == "quit":
        break                    # ★ 唯一出口
    print(f"執行：{cmd}")

# --- while...else ---
# else 在迴圈「正常結束」時執行，被 break 跳出則不執行
count = 1
while count <= 3:
    print(count)
    count += 1
else:
    print("正常結束！")           # ← 這會執行

# ★ for vs while 選擇：
# for ：已知次數、遍歷序列（更簡潔安全）
# while：不確定次數、等待條件（更靈活）

# ⚠️ 條件一開始 False → 一次都不執行


# ============================================================
# 十六、break 與 continue                         【第 23 課】
# ============================================================

# --- break：立即跳出整個迴圈 ---
for num in [4, 7, 2, 9, 1]:
    if num == 9:
        print("找到 9！")
        break                    # 後面的 1 不會被檢查
    print(f"檢查 {num}")

# --- continue：跳過當前這一輪，繼續下一輪 ---
for i in range(1, 6):
    if i == 3:
        continue                 # 跳過 3
    print(i, end=" ")           # 1 2 4 5
print()

# ★ for/while...else：正常結束執行 else，break 跳出不執行
for attempt in range(1, 4):
    pw = input(f"第 {attempt} 次：")
    if pw == "1234":
        print("成功！")
        break                    # break → else 不執行
else:
    print("帳號鎖定！")          # 3 次都錯 → 正常結束 → 執行

# 判斷質數
num = 7
for i in range(2, num):
    if num % i == 0:
        print(f"{num} 不是質數")
        break
else:
    print(f"{num} 是質數！")     # 沒有 break → 是質數

# ⚠️ break/continue 只能在迴圈裡用
# ⚠️ while + continue 要注意在 continue 前更新變數（否則無窮迴圈）


# ============================================================
# 十七、巢狀迴圈                                  【第 24 課】
# ============================================================

# 外層每執行一次，內層完整跑一遍
# 總次數 = 外層 × 內層

# ★ 九九乘法表
for i in range(1, 10):
    for j in range(1, 10):
        print(f"{i}×{j}={i*j:2d}", end="  ")
    print()

# --- 直角三角形 ---
for i in range(1, 6):
    print("★ " * i)
# ★
# ★ ★
# ★ ★ ★
# ★ ★ ★ ★
# ★ ★ ★ ★ ★

# --- 處理二維資料 ---
scores = [[85, 72], [68, 95], [92, 88]]
for row in scores:
    for score in row:
        print(score, end=" ")
    print()

# ⚠️ break/continue 只影響所在的那一層迴圈

# ★ 跳出所有迴圈的方法：
# 方法 1：旗標變數
found = False
for i in range(5):
    for j in range(5):
        if i == 2 and j == 3:
            found = True
            break
    if found:
        break

# 方法 2：包在函數裡用 return（更優雅）
def search():
    for i in range(5):
        for j in range(5):
            if i == 2 and j == 3:
                return (i, j)
    return None


# ╔════════════════════════════════════════════════════════════════╗
# ║                     ★ 總結速查表 ★                           ║
# ╚════════════════════════════════════════════════════════════════╝
#
# 【資料型別】
#   int     整數        18, -5, 23_000_000
#   float   浮點數      3.14, 3e8, -0.5
#   str     字串        "Hello", '你好', """多行"""
#   bool    布林值      True, False
#
# 【型別轉換】
#   int()   截斷小數（非四捨五入）     int(3.9) → 3
#   float() 轉浮點數                  float("3.14") → 3.14
#   str()   轉字串（最安全）           str(123) → "123"
#   bool()  轉布林值                  bool(0) → False
#
# 【Falsy 值】 0, 0.0, "", None, [], False → 其他都是 Truthy
#
# 【算術】 +  -  *  /(float)  //(整除)  %(餘數)  **(指數)
# 【比較】 ==  !=  >  <  >=  <=    連鎖：1 < x < 10
# 【邏輯】 and  or  not             優先：not > and > or
# 【賦值】 =  +=  -=  *=  /=  //=  %=  **=
#
# 【優先順序】 () > ** > ±正負 > */% > +- > 比較 > not > and > or
#
# 【條件】
#   if 條件:                    單一條件
#   if-else:                    二選一
#   if-elif-else:               多重條件（命中就停）
#   巢狀 if:                    能區分不同錯誤
#
# 【迴圈】
#   for x in 可迭代:            遍歷序列
#   for i in range(n):          重複 n 次
#   while 條件:                 條件式重複
#   while True: + break         無窮迴圈 + 出口
#
# 【迴圈控制】
#   break     跳出整個迴圈
#   continue  跳過當前一輪
#   else      正常結束才執行（break 跳出不執行）
#
# 【三大模式】
#   累加器：total = 0  → total += num
#   計數器：count = 0  → count += 1
#   追蹤器：best = lst[0] → 比較更新
#
# 【常用函數】
#   print()  輸出（sep, end）
#   input()  輸入（永遠 str）
#   len()    長度
#   type()   查型別
#   range()  數字序列（不含 stop）
#   abs()    絕對值
#   round()  四捨五入
#   max()    最大值
#   min()    最小值
#
# 【常見錯誤】
#   ⚠️ = 賦值 vs == 比較
#   ⚠️ input() 回傳字串，做運算前要轉型
#   ⚠️ if 和迴圈結尾要加冒號 :
#   ⚠️ 縮排不一致 → IndentationError
#   ⚠️ 0.1 + 0.2 ≠ 0.3（浮點精度問題）
#   ⚠️ int(3.9) = 3 不是 4（截斷非四捨五入）
#   ⚠️ while 忘記更新變數 → 無窮迴圈
#   ⚠️ break/continue 只影響當前那一層迴圈
#
# ════════════════════════════════════════════════════════════════


# 執行: python3 summary1-24_old.py

# === 預期輸出 (節錄) ===
# Hello World
# 123
# True
# 姓名 小明 年齡 18
# 2025/3/1
# ABC
# Hello World
# 第一行
# 第二行
# 姓名	年齡
# C:\Users\Documents
# It's OK
# C:\new\test
# <class 'str'>
# <class 'int'>
# <class 'float'>
# <class 'bool'>
# 0.30000000000000004
# 0.3
# 5
# …（後略，完整輸出共 49 行）
# ⚠️ 本檔需要互動輸入（input()），以上為未輸入時的輸出；請自行執行並輸入資料。
