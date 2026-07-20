# ============================================================
# 第 7 課：基本資料型別（二）—— 字串 str — 重點總結
# ============================================================

# ═══════════════════════════════════════════════════════════════════════════
# 【面試題】字串格式化與可變／不可變
# ───────────────────────────────────────────────────────────────────────────
# 🔥 Q1. f-string、% 與 .format() 該用哪一個？
#     答：優先 **f-string**（3.6 起），最快也最好讀。
#     實測 `f"{3.14159:.2f}"` → `'3.14'`、`f"{'x'!r}"` → `"'x'"`（`!r` 取 repr）。
#     `%` 是舊式 C 風格，`.format()` 是過渡期方案，維護舊碼才會遇到。
#     追問：f-string 裡要印出大括號怎麼寫？（`{{` 和 `}}`）
#
# 🔥 Q2. Python 有哪些不可變型別？「不可變」到底指什麼？
#     答：不可變＝`int`、`float`、`str`、`tuple`、`frozenset`、`bytes`；
#     可變＝`list`、`dict`、`set`、`bytearray`。
#     關鍵：不可變不是「名稱不能重新賦值」，而是**物件本身無法就地修改**。
#     追問：為什麼字串要設計成不可變？（可 hash 能當 dict key、可安全共享、
#     多執行緒下不必擔心被別人改掉）
#
# ⚠️ 陷阱. 兩個內容相同的字串，用 `is` 比較會是 True 嗎？
#     答：**不一定**。實測本機：字面值 `'hello' is 'hello'` → True（編譯期
#     interning），但執行期建立的 `''.join(['h','e','l','l','o']) is 'hello'`
#     → **False**，而 `==` → True。
#     為什麼會錯：string interning 是 **CPython 的實作細節、非語言保證**，
#     且會隨版本變動；拿 `is` 比字串就是在賭實作。3.12 起對字面值用 `is`
#     還會發出 SyntaxWarning。比較內容一律用 `==`。
# ═══════════════════════════════════════════════════════════════════════════


# ============================================================
# 【重點 1】字串的定義
# 字串用 ' 或 " 定義，三重引號 ''' 或 """ 可定義多行字串。
# ============================================================

name = '小明'             # 單引號
message = "Hello World"   # 雙引號 — 完全等效

# --- 字串中包含引號 → 交替使用 ---
sentence = "It's a beautiful day"    # 字串含 ' → 外面用 "
sentence = '他說："你好"'              # 字串含 " → 外面用 '

# --- 三重引號定義多行字串 ---
poem = """靜夜思
床前明月光，
疑是地上霜。"""
print(poem)

# ============================================================
# 【重點 2】字串是不可變的（immutable）
# 不能直接修改字串中的某個字元，只能創建新字串。
# ============================================================

name = "Hello"
# name[0] = "h"          # ❌ TypeError!
new_name = "h" + name[1:]  # ✅ 創建新字串
print(new_name)            # hello

# ============================================================
# 【重點 3】len() — 取得字串長度
# ============================================================

print(len("Hello"))        # 5
print(len("你好"))         # 2（中文字也是 1 個字元）
print(len(""))             # 0（空字串）
print(len("Hello World"))  # 11（空格也算一個字元）

# ============================================================
# 【重點 4】索引（index）— 用位置存取字元
# 索引從 0 開始，負索引從 -1（最後一個）開始。
# ============================================================

text = "Hello"
print(text[0])             # H（第一個字元）
print(text[4])             # o（第五個字元）
print(text[-1])            # o（最後一個）
print(text[-2])            # l（倒數第二個）
# print(text[10])          # ❌ IndexError! 超出範圍

# ============================================================
# 【重點 5】切片（slicing）— 取出子字串
# 語法：text[start:end]（包含 start，不包含 end）
# ============================================================

text = "Hello World"
print(text[0:5])           # Hello（索引 0~4）
print(text[6:11])          # World（索引 6~10）
print(text[:5])            # Hello（省略 start → 從頭開始）
print(text[6:])            # World（省略 end → 到最後）
print(text[:])             # Hello World（完整複製）

# --- 負索引切片 ---
print(text[-5:])           # World（最後 5 個字元）
print(text[:-6])           # Hello（去掉最後 6 個）

# --- 步長（step） ---
print(text[::2])           # HloWrd（每隔 2 個取一個）
print(text[::-1])          # dlroW olleH（★ 反轉字串！）

# ============================================================
# 【重點 6】字串連接 + 與重複 *
# ============================================================

# --- + 連接字串 ---
last_name = "陳"
first_name = "小明"
full_name = last_name + first_name
print(full_name)           # 陳小明

greeting = "Hello" + " " + "World" + "!"
print(greeting)            # Hello World!

# --- * 重複字串 ---
line = "-" * 20
print(line)                # --------------------
print("哈" * 3)            # 哈哈哈

# --- ⚠️ 字串 + 數字會報錯，需用 str() 轉型 ---
age = 18
# message = "我今年" + age + "歲"     # ❌ TypeError!
message = "我今年" + str(age) + "歲"   # ✅ 我今年18歲

# ============================================================
# 【重點 7】轉義字元
# ============================================================

# \n  — 換行
print("第一行\n第二行")

# \t  — Tab（對齊用）
print("姓名\t年齡\t城市")
print("小明\t18\t台北")

# \\  — 顯示反斜線
print("C:\\Users\\Documents")  # C:\Users\Documents

# \'  — 在單引號字串中顯示單引號
print('It\'s a beautiful day')

# \"  — 在雙引號字串中顯示雙引號
print("他說：\"你好\"")

# --- 原始字串 r"..." — 忽略轉義字元 ---
path = r"C:\new\test"      # \n 不會被當成換行
print(path)                 # C:\new\test

# ============================================================
# 【重點 8】in / not in — 檢查子字串是否存在
# ============================================================

sentence = "Hello World"
print("Hello" in sentence)      # True
print("Python" in sentence)     # False
print("hello" in sentence)      # False（⚠️ 大小寫有差！）
print("Python" not in sentence) # True

# --- 實用範例：驗證電子郵件 ---
email = "user@example.com"
if "@" in email:
    print("這看起來是一個電子郵件")

# ============================================================
# 【重點 9】常用字串方法
# ============================================================

text = "Hello World"

# --- 大小寫轉換 ---
print(text.upper())        # HELLO WORLD（全大寫）
print(text.lower())        # hello world（全小寫）
print(text.title())        # Hello World（每個單字首字大寫）

# --- 去除空白 ---
text2 = "   Hello World   "
print(text2.strip())       # "Hello World"（去除頭尾空白）
print(text2.lstrip())      # "Hello World   "（去左邊空白）
print(text2.rstrip())      # "   Hello World"（去右邊空白）

# --- 替換 ---
print(text.replace("World", "Python"))  # Hello Python
print(text.replace("l", "L"))          # HeLLo WorLd

# --- 搜尋 ---
print(text.find("World"))   # 6（找到，回傳起始索引）
print(text.find("Python"))  # -1（找不到，回傳 -1）

# --- 計數 ---
print(text.count("l"))      # 3（"l" 出現 3 次）
print(text.count("o"))      # 2（"o" 出現 2 次）

# ============================================================
# 【重點 10】字串與數字互轉
# ============================================================

# --- 數字 → 字串：str() ---
num = 123
text = str(num)
print(text, type(text))     # "123" <class 'str'>

# --- 字串 → 整數：int() ---
text = "456"
num = int(text)
print(num, type(num))       # 456 <class 'int'>

# --- 字串 → 浮點數：float() ---
text = "3.14"
num = float(text)
print(num, type(num))       # 3.14 <class 'float'>

# ============================================================
# 【重點 11】實作範例 — 字串遮罩
# ============================================================

password = "mySecretPassword123"
length = len(password)
masked = password[:2] + "*" * (length - 4) + password[-2:]
print("原始密碼：", password)   # mySecretPassword123
print("遮罩後：", masked)       # my***************23

# ============================================================
# 小結：
# 1. 字串用 ' 或 " 定義，三重引號支援多行
# 2. 字串不可變（immutable），只能創建新字串
# 3. len() 長度、[i] 索引、[start:end:step] 切片
# 4. [::-1] 反轉字串
# 5. + 連接、* 重複、str(數字) 轉型
# 6. in / not in 檢查子字串
# 7. upper/lower/strip/replace/find/count 常用方法
# 8. 轉義字元：\n \t \\ \'  \"，r"..." 原始字串
# ============================================================


# 執行: python3 summary.py

# === 預期輸出 (節錄) ===
# 靜夜思
# 床前明月光，
# 疑是地上霜。
# hello
# 5
# 2
# 0
# 11
# H
# o
# o
# l
# Hello
# World
# Hello
# World
# Hello World
# World
# Hello
# HloWrd
# …（後略，完整輸出共 55 行）
