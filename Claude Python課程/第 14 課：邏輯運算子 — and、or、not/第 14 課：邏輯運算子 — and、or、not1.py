# 第 14 課：邏輯運算子 — and、or、not
# 邏輯運算子（logical operators）是用來處理布林值的運算子，主要有三種：and、or 和 not。
# and 運算子：當兩邊的布林值都是 True 時，結果才會是 True，否則就是 False。
# or 運算子：當兩邊的布林值至少有一邊是 True 時，結果就會是 True，否則就是 False。
# not 運算子：用來取反布林值，如果原本是 True，使用 not 後就會變成 False；如果原本是 False，使用 not 後就會變成 True。
# 以下是 and 運算子的範例：
print(True and True)     # True  ← 兩邊都是 True → True
print(True and False)    # False ← 有一邊是 False → False
print(False and True)    # False ← 有一邊是 False → False
print(False and False)   # False ← 兩邊都是 False → False

age = int(input("請輸入年齡："))
has_license = input("有駕照嗎？(yes/no)：").lower() == "yes"

# 年齡 >= 18 「而且」有駕照 → 可以開車
can_drive = age >= 18 and has_license

if can_drive:
    print("✅ 你可以合法開車！")
else:
    print("❌ 你不能開車。")
 
username = input("帳號：")
password = input("密碼：")

# 帳號「而且」密碼都正確才能登入
if username == "admin" and password == "1234":
    print("✅ 登入成功！")
else:
    print("❌ 帳號或密碼錯誤。")

score = float(input("請輸入成績："))

# 成績在 80 到 89 之間 → B 等級
if score >= 80 and score <= 89:
    print("等級：B")

# 💡 上面等同於連續比較（第 13 課學的）
if 80 <= score <= 89:
    print("等級：B")

# 以下是 or 運算子的範例：
print(True or True)      # True  ← 兩邊都 True → 當然 True
print(True or False)     # True  ← 有一邊 True → True
print(False or True)     # True  ← 有一邊 True → True
print(False or False)    # False ← 兩邊都 False → False

age = int(input("請輸入年齡："))

# 65 歲以上「或者」6 歲以下 → 免費
if age >= 65 or age <= 6:
    print("🎉 您可以免費入場！")
else:
    print("💰 入場費：100 元")

is_member = input("是否為會員？(yes/no)：").lower() == "yes"
spent = float(input("本次消費金額："))

# 是會員「或者」消費滿 1000 → 享 VIP 優惠
if is_member or spent >= 1000:
    print("🌟 恭喜獲得 VIP 優惠！")
else:
    print("下次消費滿 1000 或加入會員即可享 VIP 優惠。")

password = input("請輸入密碼：")

# 接受多組密碼中的任何一組
if password == "abc123" or password == "admin" or password == "guest":
    print("✅ 登入成功")
else:
    print("❌ 密碼錯誤")

# 以下是 not 運算子的範例：
print(not True)      # False
print(not False)     # True

banned_users = "hacker123"
username = input("請輸入帳號：")

# 「不是」被封鎖的帳號 → 可以登入
if not username == banned_users:
    print("✅ 歡迎登入！")
else:
    print("🚫 你的帳號已被封鎖。")

# 💡 上面也可以寫成
if username != banned_users:
    print("✅ 歡迎登入！")

name = input("請輸入名字：").strip()

# 如果名字是空的...
if not name:    # not "" → not False → True
    print("⚠️ 名字不能為空！")
else:
    print(f"你好，{name}！")

game_over = False

if not game_over:
    print("🎮 遊戲繼續中...")
else:
    print("💀 遊戲結束！")

age = int(input("請輸入年齡："))
is_student = input("是否為學生？(yes/no)：").lower() == "yes"

# 票價規則：
# 1. 兒童（< 12）或長者（>= 65）→ 免費
# 2. 學生 → 半價（150 元）
# 3. 其他 → 全票（300 元）

if age < 12 or age >= 65:
    price = 0
    category = "免費"
elif is_student:
    price = 150
    category = "學生票"
else:
    price = 300
    category = "全票"

print(f"票種：{category}")
print(f"票價：{price} 元")

password = input("請設定密碼：")

has_length = len(password) >= 8              # 長度 >= 8
has_upper = password != password.lower()     # 包含大寫（轉小寫後跟原本不同）
has_digit = False
for char in password:                        # 包含數字
    if char.isdigit():
        has_digit = True

print(f"長度足夠：{has_length}")
print(f"包含大寫：{has_upper}")
print(f"包含數字：{has_digit}")

# 三個條件「全部」滿足 → 強密碼
if has_length and has_upper and has_digit:
    print("✅ 密碼強度：強")
# 至少滿足兩個 → 中等
elif (has_length and has_upper) or (has_length and has_digit) or (has_upper and has_digit):
    print("⚠️ 密碼強度：中")
else:
    print("❌ 密碼強度：弱")

# 
# 問題：下面的結果是什麼？
result = True or False and False

# 💡 提示：and 的優先級高於 or
# 所以會先計算 False and False → False
# 再計算 True or False → True
print(result)  # True


# 問題：下面的結果是什麼？
result = True or False and False
# 💡 提示：and 的優先級高於 or
# 所以會先計算 False and False → False
# 再計算 True or False → True
print(result)  # True

# 問題：下面的結果是什麼？
# not 最先算
print(not True and False)         # False
# 解析：(not True) and False → False and False → False

print(not (True and False))       # True
# 解析：not (False) → True

# and 比 or 先算
print(True or True and False)     # True
# 解析：True or (True and False) → True or False → True

print((True or True) and False)   # False
# 解析：True and False → False

# 這種沒有括號的表達式很難理解，容易出錯！
# ❌ 不清楚，要靠記住優先順序才能理解
a = b = c = False
result = a > 5 and b < 10 or c == 0

# ✅ 加上括號，一目了然！
result = (a > 5 and b < 10) or (c == 0)


# 邏輯運算子還有一個特性叫做「短路求值」（short-circuit evaluation），意思是說在計算邏輯表達式時，如果已經可以確定結果了，就不會繼續計算下去。
# and：如果第一個是 False，就不看第二個了
# （因為不管第二個是什麼，結果都是 False）
False and print("我不會被執行")    # print 不會執行！
True and print("我會被執行！")     # 輸出：我會被執行！

# or：如果第一個是 True，就不看第二個了
# （因為不管第二個是什麼，結果都是 True）
True or print("我不會被執行")      # print 不會執行！
False or print("我會被執行！")     # 輸出：我會被執行！


# 避免除以零的錯誤
x = 0

# ❌ 直接除會報錯
# if x != 0 and 10 / x > 2:

# ✅ 短路求值保護了你！
if x != 0 and 10 / x > 2:
    print("OK")
# 當 x == 0 時，x != 0 是 False
# and 短路 → 不會去算 10 / x → 不會報錯！


# and：遇到第一個 Falsy 值就停下來回傳它；都是 Truthy 就回傳最後一個
print(1 and 2)           # 2 ← 都是 Truthy，回傳最後一個
print(1 and 2 and 3)     # 3
print(0 and 2)           # 0 ← 0 是 Falsy，停在這裡回傳
print("" and "hello")    # "" ← 空字串是 Falsy
print("hi" and "hello")  # "hello" ← 都是 Truthy，回傳最後一個


# or：遇到第一個 Truthy 值就停下來回傳它；都是 Falsy 就回傳最後一個
print(1 or 2)            # 1 ← 第一個是 Truthy，停在這裡回傳
print(0 or 2 or 3)       # 2 ← 第一個是 Falsy，第二個是 Truthy，停在這裡回傳
print(0 or "" or None)   # None ← 都是 Falsy，回傳最後一個
print("" or "hello")     # "hello" ← 第一個是 Falsy，第二個是 Truthy，回傳第二個

# or：遇到第一個 Truthy 值就停下來回傳它；都是 Falsy 就回傳最後一個
print(1 or 2)            # 1 ← 1 是 Truthy，直接回傳
print(0 or 2)            # 2 ← 0 是 Falsy，繼續看，2 是 Truthy
print(0 or "" or "hi")   # "hi" ← 前兩個都 Falsy，回傳 "hi"
print(0 or "" or [])     # [] ← 全部 Falsy，回傳最後一個
print("" or "預設名字")   # "預設名字"
# 這種特性可以用來設定預設值


name = input("請輸入名字（可留空）：").strip()

# 如果名字為空（""是 Falsy），就用預設值
display_name = name or "匿名用戶"

print(f"歡迎，{display_name}！")

# 問題：下面的結果是什麼？
print(True and True)
print(True and False)
print(False or True)
print(False or False)
print(not True)
print(not False)
print(True or False and False)
print(not True or True)
print(not (True or True))
print("" or "hello")
print("hi" and "bye")
print(0 or "" or 42 or "test")

True and print("A")
False and print("B")
True or print("C")
False or print("D")
not False and print("E")
not True or print("F")
