# ============================================================
# 第 14 課：邏輯運算子 — and、or、not — 重點總結
# ============================================================

# ============================================================
# 【重點 1】and — 兩邊都 True，結果才 True
# 用途：多個條件「全部」都要滿足
# ============================================================

print(True and True)         # True
print(True and False)        # False
print(False and True)        # False
print(False and False)       # False

# --- 實用：帳號密碼都正確才能登入 ---
username = "admin"
password = "1234"
if username == "admin" and password == "1234":
    print("登入成功！")

# --- 實用：成績在 80~89 之間 ---
score = 85
if score >= 80 and score <= 89:
    print("等級：B")
# 等同於連鎖比較
if 80 <= score <= 89:
    print("等級：B")

# ============================================================
# 【重點 2】or — 至少一邊 True，結果就 True
# 用途：多個條件只要「任一」滿足即可
# ============================================================

print(True or True)          # True
print(True or False)         # True
print(False or True)         # True
print(False or False)        # False

# --- 實用：65 歲以上或 6 歲以下免費 ---
age = 70
if age >= 65 or age <= 6:
    print("免費入場！")

# --- 實用：接受多組密碼 ---
password = "guest"
if password == "abc123" or password == "admin" or password == "guest":
    print("登入成功")

# ============================================================
# 【重點 3】not — 取反（True ↔ False）
# ============================================================

print(not True)              # False
print(not False)             # True

# --- 實用：檢查是否不是被封鎖的帳號 ---
banned = "hacker123"
username = "normal_user"
if not username == banned:   # 等同於 username != banned
    print("歡迎登入！")

# --- 實用：檢查輸入是否為空 ---
name = "".strip()
if not name:                 # not "" → not False → True
    print("名字不能為空！")

# --- 實用：遊戲迴圈 ---
game_over = False
if not game_over:
    print("遊戲繼續中...")

# ============================================================
# 【重點 4】★ 優先順序：not > and > or
# 建議：有疑慮就加括號！
# ============================================================

# --- not 最先算 ---
print(not True and False)        # False
# 解析：(not True) and False → False and False → False

print(not (True and False))      # True
# 解析：not (False) → True

# --- and 比 or 先算 ---
print(True or True and False)    # True
# 解析：True or (True and False) → True or False → True

print((True or True) and False)  # False
# 解析：True and False → False

# --- 完整範例 ---
result = True or False and not False
# 步驟 1：not False → True
# 步驟 2：False and True → False
# 步驟 3：True or False → True
print(result)                    # True

# --- ✅ 建議加括號，更清楚 ---
# ❌ 不清楚
# result = a > 5 and b < 10 or c == 0
# ✅ 加括號
# result = (a > 5 and b < 10) or (c == 0)

# ============================================================
# 【重點 5】★ 短路求值（Short-Circuit Evaluation）
# and：第一個是 False → 直接回傳，不看第二個
# or ：第一個是 True  → 直接回傳，不看第二個
# ============================================================

# --- and 的短路 ---
False and print("不會被執行")      # print 不會執行
True and print("會被執行！")       # 輸出：會被執行！

# --- or 的短路 ---
True or print("不會被執行")        # print 不會執行
False or print("會被執行！")       # 輸出：會被執行！

# --- ★ 避免除以零錯誤（短路保護） ---
x = 0
if x != 0 and 10 / x > 2:
    print("OK")
# 當 x == 0 時，x != 0 是 False
# and 短路 → 不會去算 10 / x → 不會報錯！

# ============================================================
# 【重點 6】★ and / or 回傳的不一定是布林值！
# and：回傳第一個 Falsy 值，都 Truthy 則回傳最後一個
# or ：回傳第一個 Truthy 值，都 Falsy 則回傳最後一個
# ============================================================

# --- and 的回傳值 ---
print(1 and 2)               # 2（都 Truthy → 回傳最後一個）
print(1 and 2 and 3)         # 3
print(0 and 2)               # 0（0 是 Falsy → 停在這裡回傳）
print("" and "hello")        # ""（空字串是 Falsy）
print("hi" and "hello")      # "hello"（都 Truthy → 回傳最後一個）

# --- or 的回傳值 ---
print(1 or 2)                # 1（第一個 Truthy → 直接回傳）
print(0 or 2)                # 2（0 是 Falsy → 繼續看 → 2 是 Truthy）
print(0 or "" or "hi")       # "hi"（前兩個 Falsy → 回傳 "hi"）
print(0 or "" or [])         # []（全部 Falsy → 回傳最後一個）

# --- ★ 用 or 設定預設值（超實用！） ---
name = input("請輸入名字（可留空）：").strip()
display_name = name or "匿名用戶"
# 如果 name 為空（Falsy），就用 "匿名用戶"
print(f"歡迎，{display_name}！")

# ============================================================
# 【重點 7】綜合應用 — 票價計算
# ============================================================

age = 25
is_student = False

# 兒童（<12）或長者（>=65）→ 免費
# 學生 → 半價（150 元）
# 其他 → 全票（300 元）
if age < 12 or age >= 65:
    price = 0
elif is_student:
    price = 150
else:
    price = 300
print(f"票價：{price} 元")

# ============================================================
# 【重點 8】綜合應用 — 密碼強度檢查
# ============================================================

password = "Hello123"
has_length = len(password) >= 8
has_upper = password != password.lower()    # 包含大寫
has_digit = any(c.isdigit() for c in password)  # 包含數字

if has_length and has_upper and has_digit:
    print("密碼強度：強")
elif (has_length and has_upper) or (has_length and has_digit) or (has_upper and has_digit):
    print("密碼強度：中")
else:
    print("密碼強度：弱")

# ============================================================
# 小結：
# 1. and：兩邊都 True → True（用於全部滿足）
# 2. or ：至少一邊 True → True（用於任一滿足）
# 3. not：取反 True ↔ False
# 4. 優先順序：not > and > or（建議加括號）
# 5. 短路求值：and 遇 False 停，or 遇 True 停
# 6. and/or 回傳值不一定是布林值！
# 7. ★ 用 or 設預設值：name or "預設"
# 8. ★ 短路可避免除以零等錯誤
# ============================================================
