# 比較運算子用來比較兩個值，並回傳布林值（True 或 False）。以下是一些常見的比較運算子：
# - `==`：等於, 用於檢查兩個值是否相等。
# - `!=`：不等於, 用於檢查兩個值是否不相等。
# - `>`：大於, 用於檢查左邊的值是否大於右邊的值。
# - `<`：小於, 用於檢查左邊的值是否 小於右邊的值。
# - `>=`：大於或等於, 用於檢查左邊的值是否大於或等於右邊的值。
# - `<=`：小於或等於, 用於檢查左邊的值是否小於或等於右邊的值。

# ═══════════════════════════════════════════════════════════════════════════
# 【面試題】比較運算子
# ───────────────────────────────────────────────────────────────────────────
# 🔥 Q1. `=`、`==`、`is` 三者差在哪？
#     答：`=` 是賦值（名稱綁定）；`==` 比**值**（會呼叫 `__eq__`）；
#     `is` 比**身分**（是不是同一個物件）。
#     經驗法則：只對 `None`／`True`／`False`／sentinel 用 `is`，其餘一律 `==`。
#     追問：為什麼 `x is None` 優於 `x == None`？
#     （`__eq__` 可以被覆寫成任何行為，`is` 無法被欺騙，而且更快）
#
# 🔥 Q2. Python 支援鏈式比較嗎？
#     答：支援。`1 < 2 < 3` 實測 True，等價於 `(1<2) and (2<3)`，
#     而且**中間的運算元只會被求值一次**（實測有副作用的函式只被呼叫 1 次）。
#     這與 C 語言不同：C 的 `1<2<3` 是 `(1<2)<3`，語意完全不一樣。
#     追問：那 `a < b < c` 中若 b 是很貴的函式呼叫，寫成 and 版本會怎樣？
#     （會被呼叫兩次，鏈式寫法反而更有效率）
#
# ⚠️ 陷阱. 比較兩個內容相同的 list，`==` 和 `is` 會給一樣的答案嗎？
#     答：**不會**。`[1,2] == [1,2]` 是 True（內容相同），
#     但 `[1,2] is [1,2]` 是 False（兩個不同物件）。
#     實測連空的都一樣：`[] is []` → **False**。
#     為什麼會錯：把 `is` 讀成「等於」。它問的是「是不是同一個東西」，
#     不是「長得一不一樣」。
# ═══════════════════════════════════════════════════════════════════════════

print(5 == 5)        # True
print(5 == 3)        # False
print(1.0 == 1)      # True ← Python 認為 1.0 和 1 值相等
print(True == 1)     # True ← True 的數值就是 1
print(False == 0)    # True ← False 的數值就是 0


# = 是「賦值」：把右邊的值存到左邊的變數
x = 10        # 把 10 存到 x

# == 是「比較」：問左右兩邊是否相等
x == 10       # 問：x 等於 10 嗎？→ True

# 一個超經典的錯誤
x = 5

# ❌ 想比較卻寫成賦值
# if x = 10:       # SyntaxError!

# ✅ 正確的比較
if x == 10:
    print("x 是 10")

# 不等於（!=）運算子用來檢查兩個值是否不相等，當它們不相等時回傳 True，否則回傳 False。
print(5 != 3)        # True （5 不等於 3？是的！）
print(5 != 5)        # False（5 不等於 5？不對，它們相等）
print("cat" != "dog")  # True
print(1 != True)     # False ← True 等於 1，所以 1 != True 是 False

a = 5
b = 3

print(a == b)    # False
print(a != b)    # True

# 永遠是一個 True 一個 False

# 大於（>）運算子用來檢查左邊的值是否大於右邊的值，當左邊的值大於右邊的值時回傳 True，否則回傳 False。
print(10 > 5)        # True （10 大於 5？是的！）
print(5 > 10)        # False（5 大於 10？不是！）
print(5 > 5)         # False（5 大於 5？不是！等於不算大於！）

# 小於（<）運算子用來檢查左邊的值是否小於右邊的值，當左邊的值小於右邊的值時回傳 True，否則回傳 False。
print(3 < 10)        # True （3 小於 10？是的！）
print(10 < 3)        # False（10 小於 3？不是！）
print(5 < 5)         # False（5 小於 5？不是！等於不算小於！）

# 大於或等於（>=）運算子用來檢查左邊的值是否大於或等於右邊的值，當左邊的值大於或等於右邊的值時回傳 True，否則回傳 False。
print(10 >= 5)       # True （10 ≥ 5？是的！）
print(5 >= 5)        # True （5 ≥ 5？是的！等於也算！✅）
print(3 >= 5)        # False（3 ≥ 5？不是！）

# 小於或等於（<=）運算子用來檢查左邊的值是否小於或等於右邊的值，當左邊的值小於或等於右邊的值時回傳 True，否則回傳 False。
print(3 <= 10)       # True （3 ≤ 10？是的！）
print(5 <= 5)        # True （5 ≤ 5？是的！等於也算！✅）
print(10 <= 5)       # False（10 ≤ 5？不是！）


# ✅ 正確寫法
print(5 >= 3)    # 大於等於
print(5 <= 3)    # 小於等於
print(5 != 3)    # 不等於

# ❌ 錯誤寫法（符號順序反了）
# print(5 =>) 　 # SyntaxError!
# print(5 =<)    # SyntaxError!
# print(5 =!)    # SyntaxError!

# int 和 float 可以直接比較, Python 會自動把 int 轉成 float 來比較。
print(1 == 1.0)      # True ← Python 認為值相等
print(5 > 4.9)       # True
print(3.14 < 4)      # True
print(10 >= 10.0)    # True


# 字串相等比較
print("apple" == "apple")     # True
print("apple" == "Apple")     # False ← 大小寫不同！

# 字串大小比較（按字母順序）
print("apple" < "banana")     # True  ← a 排在 b 前面
print("cat" > "car")          # True  ← 前兩字一樣，t > r
print("abc" < "abd")          # True  ← 前兩字一樣，c < d


# 大寫字母 A-Z 的編碼：65-90
# 小寫字母 a-z 的編碼：97-122
# 數字字元 0-9 的編碼：48-57
# 所有大寫字母都「小於」小寫字母
print("Z" < "a")        # True ← Z 的編碼 90 < a 的編碼 97
print("Apple" < "apple") # True ← A(65) < a(97)

# 數字字元比大寫字母更「小」
print("9" < "A")         # True ← 9(57) < A(65)

# 長度不同的情況
print("app" < "apple")    # True
# 前 3 個字都一樣，但 "app" 比較短
# 較短的字串被認為「比較小」
print("app" > "apple")    # False

# 記住：True = 1，False = 0
print(True == 1)       # True
print(False == 0)      # True
print(True > False)    # True  （1 > 0）
print(True > 0.5)      # True  （1 > 0.5）
print(False < 0.1)     # True  （0 < 0.1）


# 字串和數字不能比大小
# print("5" > 3)       # ❌ TypeError!
# print("abc" < 10)    # ❌ TypeError!

# 但是可以比較是否相等（結果一定是 False）
print("5" == 5)        # False（型別不同）
print("True" == True)  # False（字串 vs 布林值）


# Python 支援連鎖比較（chained comparison），可以同時比較多個值。
# 數學寫法：1 < x < 10
# Python 也可以這樣寫！

x = 5
print(1 < x < 10)      # True （1 < 5 < 10？是的！）
print(1 < x < 3)       # False（1 < 5 < 3？5 不小於 3！）

# 等同於
print(1 < x and x < 10)  # True（and 是下一課會學的邏輯運算子）


age = 25

# 判斷年齡是否在 18 到 65 之間
print(18 <= age <= 65)     # True

# 判斷成績是否在 60 到 100 之間
score = 85
print(60 <= score <= 100)  # True

# 判斷三個數是否依序排列
a, b, c = 1, 2, 3
print(a < b < c)           # True

a, b, c = 1, 3, 2
print(a < b < c)           # False（3 不小於 2）

# 實際應用：年齡判斷
age = int(input("請輸入你的年齡："))

if age >= 18:
    print("✅ 你已成年，可以進入")
else:
    print("❌ 你未成年，禁止進入")

# 實際應用：密碼驗證
correct_password = "python123"
user_input = input("請輸入密碼：")

if user_input == correct_password:
    print("✅ 密碼正確，歡迎登入！")
else:
    print("❌ 密碼錯誤，請重試。")

# 實際應用：成績等級判斷
score = float(input("請輸入成績："))

if score >= 90:
    print("等級：A（優秀）")
elif score >= 80:
    print("等級：B（良好）")
elif score >= 70:
    print("等級：C（中等）")
elif score >= 60:
    print("等級：D（及格）")
else:
    print("等級：F（不及格）")

# 實際應用：體溫判斷
temperature = float(input("請輸入體溫："))

if 36.0 <= temperature <= 37.5:
    print("✅ 體溫正常")
elif temperature > 37.5:
    print("⚠️ 體溫偏高，建議就醫")
else:
    print("⚠️ 體溫偏低，請注意保暖")


# 比較運算子也可以用來把比較結果存起來，之後再使用。
age = 20
score = 85

# 把比較結果存起來
is_adult = age >= 18
is_pass = score >= 60
is_excellent = score >= 90

print(f"已成年：{is_adult}")        # 已成年：True
print(f"及格：{is_pass}")           # 及格：True
print(f"優秀：{is_excellent}")      # 優秀：False

# 之後可以在 if 中使用
if is_adult and is_pass:
    print("成年且及格！")


x = 10

# ❌ 想比較卻用了賦值
# if x = 10:          # SyntaxError!

# ✅ 正確用比較
if x == 10:
    print("x 是 10")


# ❌ 順序錯誤
# print(5 => 3)       # SyntaxError!
# print(5 =< 3)       # SyntaxError!

# ✅ 正確順序：比較符號在前，等號在後
print(5 >= 3)          # True
print(5 <= 3)          # False


# ❌ 字串和數字不能比大小
# print("10" > 5)      # TypeError!

# ✅ 先轉型再比較
print(int("10") > 5)   # True


user_input = "Yes"

# ❌ 可能匹配不到
if user_input == "yes":
    print("你選了是")
# 使用者輸入 "Yes"，但你比較的是 "yes"，結果是 False！

# ✅ 統一轉小寫再比較
if user_input.lower() == "yes":
    print("你選了是")


# ❌ 直接比較浮點數可能出錯
print(0.1 + 0.2 == 0.3)    # False 😱

# ✅ 用 round() 處理後再比較
print(round(0.1 + 0.2, 10) == round(0.3, 10))   # True


# ========================================
# 程式名稱：數字比較器
# 功能：比較兩個數字的大小關係
# ========================================

print("=" * 35)
print("      🔢 數字比較器")
print("=" * 35)

# ----- 取得輸入 -----
num1 = float(input("請輸入第一個數字："))
num2 = float(input("請輸入第二個數字："))

# ----- 進行所有比較 -----
print()
print(f"  {num1} == {num2} → {num1 == num2}")
print(f"  {num1} != {num2} → {num1 != num2}")
print(f"  {num1} >  {num2} → {num1 > num2}")
print(f"  {num1} <  {num2} → {num1 < num2}")
print(f"  {num1} >= {num2} → {num1 >= num2}")
print(f"  {num1} <= {num2} → {num1 <= num2}")

# ----- 顯示結論 -----
print()
print("-" * 35)
if num1 > num2:
    print(f"  結論：{num1} 比較大")
elif num1 < num2:
    print(f"  結論：{num2} 比較大")
else:
    print(f"  結論：兩個數字一樣大")
print("=" * 35)



# ========================================
# 程式名稱：數字比較器
# 功能：比較兩個數字的大小關係
# ========================================

print("=" * 35)
print("      🔢 數字比較器")
print("=" * 35)

# ----- 取得輸入 -----
num1 = float(input("請輸入第一個數字："))
num2 = float(input("請輸入第二個數字："))

# ----- 進行所有比較 -----
print()
print(f"  {num1} == {num2} → {num1 == num2}")
print(f"  {num1} != {num2} → {num1 != num2}")
print(f"  {num1} >  {num2} → {num1 > num2}")
print(f"  {num1} <  {num2} → {num1 < num2}")
print(f"  {num1} >= {num2} → {num1 >= num2}")
print(f"  {num1} <= {num2} → {num1 <= num2}")

# ----- 顯示結論 -----
print()
print("-" * 35)
if num1 > num2:
    print(f"  結論：{num1} 比較大")
elif num1 < num2:
    print(f"  結論：{num2} 比較大")
else:
    print(f"  結論：兩個數字一樣大")
print("=" * 35)


# 執行: python3 第 13 課：比較運算子 — 大於、小於、等於、不等於1.py

# === 預期輸出 (節錄) ===
# True
# False
# True
# True
# True
# True
# False
# True
# False
# False
# True
# True
# False
# False
# True
# False
# False
# True
# True
# False
# …（後略，完整輸出共 55 行）
# ⚠️ 本檔需要互動輸入（input()），以上為未輸入時的輸出；請自行執行並輸入資料。
