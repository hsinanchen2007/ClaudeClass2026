# 第 23 課：break 與 continue — 跳出與跳過
# 在迴圈中，break 用來完全跳出迴圈，而 continue 則是跳過當前的迭代，繼續下一次迭代。
# 這兩個關鍵字在控制迴圈流程時非常有用，可以幫助我們更靈活地處理資料。

# ═══════════════════════════════════════════════════════════════════════════
# 【面試題】break 與 continue
# ───────────────────────────────────────────────────────────────────────────
# 🔥 Q1. `break` 與 `continue` 差在哪？
#     答：`break` 跳出**整個**迴圈（不再有下一輪）；
#     `continue` 只跳過本次剩餘的程式碼，直接進入下一輪。
#     兩者都**只影響最內層**的那個迴圈。
#     追問：Python 有 labeled break 嗎？（沒有——要一次跳出多層得用
#     旗標變數、把迴圈包成函式用 return，或用 for-else）
#
# 🔥 Q2. `for...else` 的 else 什麼時候執行？
#     答：迴圈**跑完且沒有被 break** 才執行；一旦 break 就跳過。
#     實測兩種情況都符合。典型用途是搜尋：找到就 break，
#     else 區塊負責處理「整個都找過了還是沒有」。
#     追問：這個 else 該叫什麼比較貼切？（`nobreak`——這是社群公認的
#     命名失誤，也是它反直覺的根源）
#
# ⚠️ 陷阱. 在 `while` 迴圈裡用 `continue` 有什麼特別要小心的？
#     答：`continue` 會**跳過後面所有程式碼**直接回到條件判斷——
#     如果計數器的遞增剛好寫在 continue 後面，就永遠不會執行，
#     直接變成無窮迴圈。for 迴圈沒這問題（迭代器自己會前進）。
#     為什麼會錯：把 while 的 continue 想成跟 for 一樣安全。
#     for 的「下一輪」由迭代器保證，while 的「下一輪」得靠你自己更新變數。
# ═══════════════════════════════════════════════════════════════════════════

numbers = [4, 7, 2, 9, 1, 5, 8]

for num in numbers:
    print(f"檢查 {num}...")
    if num == 9:
        print(f"找到 9 了！停止搜尋 🎯")
        break

print("搜尋結束")


while True:                             # 故意寫成無窮迴圈
    command = input("請輸入指令（quit 離開）：")

    if command == "quit":
        print("再見！👋")
        break                           # 唯一的出口

    print(f"你輸入了：{command}")

print("程式已結束")

# 下面是一個使用 break 的範例，模擬密碼嘗試的情況。
max_attempts = 3

for attempt in range(1, max_attempts + 1):
    password = input(f"第 {attempt} 次嘗試，請輸入密碼：")

    if password == "python123":
        print("登入成功！✅")
        break
else:
    # 注意：這個 else 是跟 for 對齊的！
    print("嘗試次數用完，帳號已鎖定！🔒")


# 這裡的 else 是 for 的 else，只有當 for 正常結束（沒有被 break 打斷）時才會執行。
max_attempts = 3

for attempt in range(1, max_attempts + 1):
    password = input(f"第 {attempt} 次嘗試：")

    if password == "python123":
        print("登入成功！✅")
        break                        # 密碼對了 → break → else 不執行
else:
    print("帳號已鎖定！🔒")           # 3 次都沒對 → 迴圈正常結束 → else 執行


# 這個範例使用 break 來檢查一個數字是否為質數。
num = int(input("輸入一個大於 1 的整數："))

for i in range(2, num):
    if num % i == 0:
        print(f"{num} 不是質數（可以被 {i} 整除）")
        break
else:
    print(f"{num} 是質數！")


# continue 的範例：印出 1 到 10，但跳過 5。
for i in range(1, 11):
    if i == 5:
        continue       # 跳過 5
    print(i, end=" ")
print()


# 這個範例使用 continue 來印出 1 到 20 的偶數。
for i in range(1, 21):
    if i % 2 != 0:     # 如果是奇數
        continue        # 跳過
    print(i, end=" ")
print()


# 這裡我們有一個包含成績的資料清單，但其中有些資料是無效的（非數字或空字串）。我們使用 continue 來跳過這些無效資料，只計算有效的成績。
data = ["85", "hello", "72", "", "93", "abc", "68"]

total = 0
count = 0

for item in data:
    if not item.isdigit():   # 如果不是純數字
        print(f"  跳過無效資料：'{item}'")
        continue
    score = int(item)
    total += score
    count += 1
    print(f"  有效成績：{score}")

print(f"\n有效成績 {count} 筆，平均：{total / count:.1f}")


# 這個範例結合了 break 和 continue，模擬一個簡單的文字輸入系統。使用者可以輸入文字，當輸入 "quit" 時結束程式；
# 如果輸入的文字太短（少於 3 個字），則提示並跳過處理。
while True:
    text = input("輸入文字（quit 離開）：")

    if text == "quit":
        break               # 結束迴圈

    if len(text) < 3:
        print("太短了，至少要 3 個字！")
        continue             # 跳過下面的處理，回到 while 開頭

    print(f"你輸入了「{text}」，長度 {len(text)} 個字")


# 這裡我們使用 break 和 continue 來展示它們在迴圈中的效果。
# break 的效果
print("=== break ===")
for i in range(1, 6):
    if i == 3:
        break
    print(i, end=" ")
print()
# 輸出：1 2

# continue 的效果
print("=== continue ===")
for i in range(1, 6):
    if i == 3:
        continue
    print(i, end=" ")
print()
# 輸出：1 2 4 5


# 重要提醒：break 和 continue 只能用在迴圈裡面！如果在迴圈外面使用，會導致 SyntaxError。
# if True:
#     break      # ❌ SyntaxError: 'break' outside loop
# # break 和 continue 只能用在 for 或 while 裡面！


# 注意：continue 會跳過當前的迭代，但不會跳出整個迴圈，所以迴圈仍然會繼續執行下一次迭代。
# x = 0
# while x < 10:
#     x += 1
#     if x == 5:
#         continue    # 跳過 print，但 x 已經 +1 了，所以沒問題 ✅
#     print(x)

# 反過來，如果在 continue 前面沒有改變迴圈條件的變數，可能會導致無窮迴圈，因為條件永遠不會改變。
# x = 0
# while x < 10:
#     if x == 5:
#         continue    # ❌ 跳回 while 開頭，但 x 還是 5！永遠卡在這裡！
#     print(x)
#     x += 1


# ❌ 不好的寫法：邏輯混亂
for i in range(10):
    if i == 3:
        continue
    if i == 7:
        break
    if i % 2 == 0:
        continue
    print(i)

# 你能一眼看出這會印什麼嗎？很難吧。
# ✅ 好的寫法：清晰明確

# ✅ 更清楚的寫法：用條件整合
for i in range(7):        # 直接限制範圍，不需要 break
    if i != 3 and i % 2 != 0:
        print(i)


# 最後，這裡有一個實際應用 break 和 continue 的範例：簡易文章搜尋系統。
# 使用者輸入關鍵字，程式會搜尋文章清單並印出包含該關鍵字的文章。
articles = [
    "Python 是一種簡單易學的程式語言",
    "Java 被廣泛用於企業開發",
    "Python 擁有豐富的第三方套件",
    "JavaScript 是網頁開發的核心",
    "Python 的資料分析功能非常強大",
    "C++ 適合系統程式開發",
]

print("=== 簡易文章搜尋 ===")

while True:
    keyword = input("\n搜尋關鍵字（quit 離開）：")

    if keyword == "quit":
        print("感謝使用！👋")
        break

    if len(keyword) == 0:
        print("請輸入關鍵字！")
        continue

    print(f"\n搜尋「{keyword}」的結果：")
    found = False

    for i, article in enumerate(articles, 1):
        if keyword in article:
            print(f"  [{i}] {article}")
            found = True

    if not found:
        print("  找不到相關文章 😢")


# 這裡的範例展示了 break 和 continue 的基本用法，以及它們在實際程式中的應用。透過這些範例，
# 你可以更清楚地理解這兩個關鍵字如何影響迴圈的流程，並學會在適當的情況下使用它們來控制程式的行為。
# 練習：請嘗試修改上面的文章搜尋程式，讓它在找到第一篇符合條件的文章後就停止搜尋（使用 break），
# 或者讓它跳過包含特定關鍵字的文章（使用 continue）。這樣可以幫助你更熟悉這兩個關鍵字的使用方式。
for i in range(5):
    if i == 2:
        continue
    if i == 4:
        break
    print(i, end=" ")


# 執行: python3 第 23 課：break 與 continue — 跳出與跳過1.py

# === 預期輸出 ===
# 檢查 4...
# 檢查 7...
# 檢查 2...
# 檢查 9...
# 找到 9 了！停止搜尋 🎯
# 搜尋結束
# 請輸入指令（quit 離開）：
# ⚠️ 本檔需要互動輸入（input()），以上為未輸入時的輸出；請自行執行並輸入資料。
