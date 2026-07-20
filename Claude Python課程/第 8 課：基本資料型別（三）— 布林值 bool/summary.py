# ============================================================
# 第 8 課：基本資料型別（三）— 布林值 bool — 重點總結
# ============================================================

# ═══════════════════════════════════════════════════════════════════════════
# 【面試題】真假值判斷的實務陷阱
# ───────────────────────────────────────────────────────────────────────────
# 🔥 Q1. `if x:` 與 `if x is not None:` 差在哪？
#     答：語意完全不同。`if x:` 判斷的是「truthy」，
#     所以 `x = 0`、`x = []`、`x = ''` 都會走進 else；
#     `if x is not None:` 只排除 None，空值仍算「有給」。
#     函式參數的預設值檢查一定要用後者，否則「使用者真的傳了 0」會被吃掉。
#     追問：那為什麼慣例是用 `is` 而不是 `== None`？（`__eq__` 可被覆寫，
#     `is` 比身分、無法被欺騙也更快）
#
# 🔥 Q2. `and` 與 `or` 回傳的是布林值嗎？
#     答：**不是**，回傳的是**運算元本身**（短路求值）。
#     實測 `0 or 'a'` → `'a'`、`'x' and 'y'` → `'y'`、`[] or {}` → `{}`。
#     `or` 給第一個 truthy（都假就給最後一個）；`and` 給第一個 falsy（都真就給最後一個）。
#     追問：`x = a or default` 在 `a = 0` 時有什麼 bug？
#     （0 是 falsy 會誤用 default，應寫 `a if a is not None else default`）
#
# ⚠️ 陷阱. `{1, 1.0, True}` 有幾個元素？
#     答：實測 **1 個**，結果是 `{1}`。因為 `1 == 1.0 == True` 且三者 hash 相同，
#     set／dict 視為同一個 key，並**保留最先插入的那個**。
#     為什麼會錯：以為 set 是用「型別 + 值」去重，實際上是用 **hash 與 `==`**。
#     同理實測 `{True: 'a', 1: 'b'}` → `{True: 'b'}`：key 留最早的、value 被覆蓋。
# ═══════════════════════════════════════════════════════════════════════════


# ============================================================
# 【重點 1】布林值只有兩個值：True 和 False
# 注意：必須大寫開頭（True/False），不能寫成 true/false。
# ============================================================

is_student = True
is_raining = False

print(is_student)          # True
print(type(is_student))    # <class 'bool'>

# ============================================================
# 【重點 2】比較運算產生布林值
# ============================================================

print(5 > 3)               # True
print(5 < 3)               # False
print(5 == 5)              # True （等於）
print(5 != 3)              # True （不等於）
print(5 >= 5)              # True （大於或等於）
print(5 <= 3)              # False（小於或等於）

# --- ⚠️ = 和 == 的差別 ---
x = 10              # = 是「賦值」
print(x == 10)      # == 是「比較」→ True
print(x == 5)       # False

# --- 字串也可以比較 ---
print("apple" == "apple")   # True
print("apple" == "Apple")   # False（⚠️ 大小寫不同！）
print("abc" < "abd")        # True（按字母順序比較）

# ============================================================
# 【重點 3】Truthy 和 Falsy — 哪些值被視為 False？
# ============================================================

# --- 以下值被視為 False（Falsy） ---
print(bool(0))             # False — 數字 0
print(bool(0.0))           # False — 浮點數 0.0
print(bool(""))            # False — 空字串
print(bool(None))          # False — None
print(bool([]))            # False — 空串列
print(bool(False))         # False — False 本身

# --- 其他值都被視為 True（Truthy） ---
print(bool(1))             # True — 非零數字
print(bool(-5))            # True — 負數也是 True！
print(bool(3.14))          # True — 非零浮點數
print(bool("hello"))       # True — 有內容的字串
print(bool(" "))           # True — ⚠️ 空格也算有內容！

# --- ⚠️ 常見陷阱 ---
print(bool("0"))           # True ← 字串 "0" 不是空的！
print(bool("False"))       # True ← 字串 "False" 也不是空的！

# ============================================================
# 【重點 4】布林值可參與數學運算
# True = 1，False = 0
# ============================================================

print(True + True)         # 2（1 + 1）
print(True + False)        # 1（1 + 0）
print(True * 10)           # 10（1 * 10）
print(True == 1)           # True
print(False == 0)          # True

# --- 計算有多少個 True ---
results = [True, False, True, True, False]
print(sum(results))        # 3（有 3 個 True）

# ============================================================
# 【重點 5】布林值在條件判斷中的應用
# ============================================================

# --- 基本 if/else ---
age = 20
is_adult = age >= 18       # is_adult = True

if is_adult:
    print("你已經成年了！")
else:
    print("你還未成年。")

# --- 模擬登入 ---
is_logged_in = False
password = "1234"
if password == "1234":
    is_logged_in = True
print(is_logged_in)        # True

# --- 遊戲迴圈中使用布林值 ---
game_over = False
while not game_over:
    print("遊戲進行中...")
    game_over = True       # 改成 True 就會停止

# ============================================================
# 【重點 6】常見考題 — 判斷布林值
# ============================================================

print(10 > 5)              # True
print(3 == 3.0)            # True  — int 和 float 值相等
print("hello" == "Hello")  # False — 大小寫不同
print(bool(0))             # False — 0 是 Falsy
print(bool("False"))       # True  — 字串 "False" 不是空的！
print(True + True + False) # 2（1 + 1 + 0）

# ============================================================
# 小結：
# 1. 布林值只有 True 和 False，必須大寫
# 2. 比較運算（==, !=, >, <, >=, <=）產生布林值
# 3. = 是賦值，== 是比較
# 4. Falsy 值：0, 0.0, "", None, [], False
# 5. 其他值都是 Truthy（包括 "0" 和 "False"！）
# 6. True = 1, False = 0，可參與數學運算
# 7. sum() 可計算列表中 True 的數量
# ============================================================


# 執行: python3 summary.py

# === 預期輸出 (節錄) ===
# True
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
# False
# True
# …（後略，完整輸出共 41 行）
