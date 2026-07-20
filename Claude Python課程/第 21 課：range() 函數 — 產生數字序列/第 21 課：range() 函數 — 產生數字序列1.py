# 21 課：range() 函數 — 產生數字序列

# ═══════════════════════════════════════════════════════════════════════════
# 【面試題】range() 函數
# ───────────────────────────────────────────────────────────────────────────
# 🔥 Q1. `range()` 回傳的是 list 嗎？
#     答：**不是**。Python 3 的 `range` 是**惰性的序列物件**，
#     只記住 start／stop／step，用到才算。
#     實測 `sys.getsizeof(range(10**6))` = **48 bytes**，
#     而 `sys.getsizeof(list(range(10**6)))` = **8,000,056 bytes**。
#     追問：Python 2 的 `xrange` 是什麼？（就是現在的 `range`）
#
# 🔥 Q2. `range` 與 generator 差在哪？
#     答：關鍵差別是 **range 可以重複走訪**——實測連續兩次 `list(r)`
#     都得到完整結果；generator 用完就空了（第二次得到 `[]`）。
#     range 還支援 `len()`、索引、`in` 測試，它是完整的序列型別。
#     追問：`range` 的 `in` 是線性掃描嗎？（對整數而言不是，
#     它可以用算術直接判斷，不需要逐一走訪）
#
# ⚠️ 陷阱. `range(1, 5)` 會包含 5 嗎？
#     答：**不會**。range 是**左閉右開**，`range(1,5)` 產生 1,2,3,4。
#     所以 `range(n)` 剛好產生 n 個數（0 到 n-1），
#     這正好對應「長度為 n 的序列」的合法索引範圍。
#     為什麼會錯：直覺把 stop 讀成「跑到這裡為止（含）」。
#     記住這個設計的好處：`range(a,b)` 的元素個數剛好是 `b-a`，
#     而且 `range(0,3)` 與 `range(3,6)` 能無縫接起來不重疊。
# ═══════════════════════════════════════════════════════════════════════════

for num in [1, 2, 3, 4, 5]:
    print(num)

# 這樣寫太麻煩了，尤其當你想要產生一個很長的數字序列時。
for num in [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, ...]:  # 😱 瘋了
    print(num)

# Python 提供了一個內建函數叫做 range()，可以用來產生一個數字序列。
for num in range(100):
    print(num)

# range() 函數會產生從 0 開始的數字序列，直到指定的數字（不包含該數字）。
for i in range(5):
    print(i)

# 你也可以指定起始數字和結束數字。
for i in range(1, 6):
    print(i)    

# range() 函數還可以接受第三個參數，表示步長。
for i in range(0, 10, 2):
    print(i)
# 上面的程式碼會印出 0、2、4、6、8，因為步長是 2。

# 如果你想要產生一個反向的數字序列，可以使用負的步長。
for i in range(10, 0, -1):
    print(i)

for i in range(1, 6):
    print(i)


# 從 3 到 7
for i in range(3, 8):    # 3, 4, 5, 6, 7
    print(i, end=" ")
print()  # 換行
# 輸出：3 4 5 6 7

# 從 10 到 15
for i in range(10, 16):  # 10, 11, 12, 13, 14, 15
    print(i, end=" ")
print()
# 輸出：10 11 12 13 14 15

# 從 5 到 3（不會產生任何數字）
for i in range(5, 3):   # 5 已經 >= 3 了，產生不出東西
    print(i)
# 什麼都不會印出來（迴圈 0 次）

# 每次跳 2（偶數）
for i in range(0, 11, 2):
    print(i, end=" ")
print()
# 輸出：0 2 4 6 8 10

# 每次跳 3
for i in range(1, 16, 3):
    print(i, end=" ")
print()
# 輸出：1 4 7 10 13

# 每次跳 5
for i in range(0, 51, 5):
    print(i, end=" ")
print()
# 輸出：0 5 10 15 20 25 30 35 40 45 50


# 從 5 倒數到 1
for i in range(5, 0, -1):
    print(i, end=" ")
print()
# 輸出：5 4 3 2 1

# 倒數計時！
for i in range(10, 0, -1):
    print(f"{i}...")
print("發射！🚀")
# 輸出：
# 10...
# 9...
# 8...
# 7...
# 6...
# 5...
# 4...
# 3...
# 2...
# 1...
# 發射！🚀


range(10, 0, -1)   # 10, 9, 8, 7, 6, 5, 4, 3, 2, 1（不包含 0）
range(10, -1, -1)  # 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0（要包含 0 就讓 stop = -1）


# 陷阱 1：想要 1 到 5，但忘記 +1
for i in range(1, 5):   # 只到 4！
    print(i, end=" ")
# 輸出：1 2 3 4（沒有 5！）

# ✅ 正確
for i in range(1, 6):   # 要到 5，stop 就要寫 6
    print(i, end=" ")
# 輸出：1 2 3 4 5

# # 陷阱 2：step 為 0
# for i in range(1, 5, 0):  # ❌ ValueError: range() arg 3 must not be zero
#     print(i)

# 陷阱 3：方向與 step 不一致
for i in range(1, 10, -1):  # start < stop，但 step 是負的
    print(i)                 # → 什麼都不會印（空的 range）

for i in range(10, 1, 1):   # start > stop，但 step 是正的
    print(i)                 # → 什麼都不會印（空的 range）


# range() 函數會產生一個 range 物件，而不是一個列表。
print(range(5))        # 輸出：range(0, 5)，不是 [0, 1, 2, 3, 4]
print(type(range(5)))  # 輸出：<class 'range'>


# 如果你想要把 range 物件轉換成列表，可以使用 list() 函數。
print(list(range(5)))         # [0, 1, 2, 3, 4]
print(list(range(1, 6)))      # [1, 2, 3, 4, 5]
print(list(range(0, 10, 2)))  # [0, 2, 4, 6, 8]
print(list(range(5, 0, -1)))  # [5, 4, 3, 2, 1]


fruits = ["蘋果", "香蕉", "櫻桃", "芒果"]

for i in range(len(fruits)):
    print(f"第 {i + 1} 個水果是：{fruits[i]}")
# 輸出：
# 第 1 個水果是：蘋果
# 第 2 個水果是：香蕉
# 第 3 個水果是：櫻桃
# 第 4 個水果是：芒果


# range() 函數可以用來產生一個數字序列，然後我們可以使用這些數字作為索引來訪問列表中的元素。
# 這種用法在需要同時訪問索引和元素的情況下非常有用。
names = ["小明", "小華", "小美"]
scores = [85, 72, 93]

for i in range(len(names)):
    print(f"{names[i]} 的成績是 {scores[i]} 分")


n = 7

for i in range(1, 10):
    print(f"{n} × {i} = {n * i}")
# 輸出：
# 7 × 1 = 7
# 7 × 2 = 14
# 7 × 3 = 21
# 7 × 4 = 28
# 7 × 5 = 35
# 7 × 6 = 42
# 7 × 7 = 49
# 7 × 8 = 56
# 7 × 9 = 63


# 1 到 20 的偶數
print("偶數：", end="")
for i in range(2, 21, 2):
    print(i, end=" ")
print()
# 輸出：偶數：2 4 6 8 10 12 14 16 18 20

# 1 到 20 的奇數
print("奇數：", end="")
for i in range(1, 21, 2):
    print(i, end=" ")
print()
# 輸出：奇數：1 3 5 7 9 11 13 15 17 19


times = int(input("你想說幾次 Hello？"))

for _ in range(times):
    print("Hello! 👋")
# 上面的程式碼中，我們使用了 _ 作為迴圈變數，表示我們不需要使用這個變數的值。


rows = int(input("請輸入層數："))

for i in range(1, rows + 1):
    spaces = " " * (rows - i)       # 前面的空格
    stars = "*" * (2 * i - 1)       # 星號數量
    print(spaces + stars)
# 輸出（假設輸入 5）：
#     * 
#    ***
#   *****
#  *******
# *********

# range(8)            # → ？個數字
# range(3, 12)        # → ？個數字
# range(0, 20, 4)     # → ？個數字


# 執行: python3 第 21 課：range() 函數 — 產生數字序列1.py

# === 預期輸出 (節錄) ===
# 1
# 2
# 3
# 4
# 5
# 1
# 2
# 3
# 4
# 5
# 6
# 7
# 8
# 9
# 10
# 11
# Ellipsis
# 0
# 1
# 2
# …（後略，完整輸出共 189 行）
# ⚠️ 本檔需要互動輸入（input()），以上為未輸入時的輸出；請自行執行並輸入資料。
