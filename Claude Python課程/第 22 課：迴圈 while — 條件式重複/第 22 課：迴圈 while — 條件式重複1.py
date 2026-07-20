# 22 課：迴圈 while — 條件式重複
# 迴圈 while 的語法如下：
# while 條件式:

# ═══════════════════════════════════════════════════════════════════════════
# 【面試題】迴圈 while
# ───────────────────────────────────────────────────────────────────────────
# 🔥 Q1. `for` 與 `while` 各適用什麼情境？
#     答：`for` 用於走訪**已知的可迭代物件**（迭代次數在進迴圈前就確定）；
#     `while` 用於**依條件重複**、次數事先不確定的場合，
#     例如等待使用者輸入、數值收斂、讀取到檔案結束。
#     追問：兩者可以互相改寫嗎？（理論上可以，但硬把 for 寫成 while
#     要自己維護索引，容易出錯也不 Pythonic）
#
# 🔥 Q2. 怎麼寫一個「至少執行一次」的迴圈？
#     答：Python **沒有** do-while。慣用寫法是
#     `while True:` 搭配在迴圈尾端用 `if 條件: break`，
#     把終止判斷放到真正需要的位置。
#     追問：`while True` 會不會很危險？（只要 break 條件正確就不會；
#     真正危險的是「條件變數在迴圈內從未被更新」）
#
# ⚠️ 陷阱. 什麼情況下 while 迴圈會意外變成無窮迴圈？
#     答：最常見的是**忘記更新條件變數**——例如 `while count > 0:` 裡面
#     漏掉 `count -= 1`。次常見的是用**浮點數**當終止條件：
#     `while x != 1.0:` 可能因為浮點誤差永遠不相等（0.1+0.2 實測就不等於 0.3）。
#     為什麼會錯：把「條件遲早會變成假」當成理所當然。
#     浮點比較請用 `<`／`>` 或 `math.isclose()`，不要用 `!=`／`==`。
# ═══════════════════════════════════════════════════════════════════════════

count = 5

while count > 0:
    print(f"{count}...")
    count -= 1       # 每一輪把 count 減 1

print("發射！🚀")

# ① 初始化：給條件一個起始狀態
# ② 條件：決定什麼時候停下來
# ③ 更新：讓條件有機會變成 False
count = 5           # ① 初始化：給條件一個起始狀態

while count > 0:    # ② 條件：決定什麼時候停下來
    print(count)
    count -= 1      # ③ 更新：讓條件有機會變成 False


# # ❌ 危險！無窮迴圈！
# count = 5
# while count > 0:
#     print(count)
#     # 忘記寫 count -= 1
#     # count 永遠是 5，永遠 > 0
#     # 這個程式會一直印 5 5 5 5 5...

# # 錯誤 1：忘記更新變數
# x = 1
# while x <= 10:
#     print(x)
#     # 忘記 x += 1

# # 錯誤 2：更新方向錯誤
# x = 10
# while x > 0:
#     print(x)
#     x += 1      # 應該是 -= 1，寫成 += 1 反而越來越大

# # 錯誤 3：條件寫錯
# x = 1
# while x != 10:   # 如果 x 每次加 3：1, 4, 7, 10 ✅ 能停
#     print(x)      # 但如果 x 每次加 2：1, 3, 5, 7, 9, 11... 永遠不等於 10！
#     x += 2

# 練習：密碼驗證
password = input("請輸入密碼：")

while password != "python123":
    print("密碼錯誤，請重試！")
    password = input("請輸入密碼：")

print("密碼正確，歡迎進入！✅")

# 練習：年齡驗證
age = int(input("請輸入年齡（1-150）："))

while age < 1 or age > 150:
    print("輸入不合理！請重新輸入。")
    age = int(input("請輸入年齡（1-150）："))

print(f"你的年齡是 {age} 歲")

# 練習：猜數字遊戲
import random  # 先記住這行可以用隨機數，第 53 課會詳細學

answer = random.randint(1, 10)  # 產生 1~10 的隨機數
guess = 0
attempts = 0

print("=== 猜數字遊戲（1~10）===")

while guess != answer:
    guess = int(input("猜一個數字："))
    attempts += 1

    if guess > answer:
        print("太大了！往小的猜 ⬇️")
    elif guess < answer:
        print("太小了！往大的猜 ⬆️")
    else:
        print(f"答對了！🎉 你猜了 {attempts} 次")


# 練習：從 1 加到 n，直到總和超過 100
total = 0
n = 0

while total < 100:
    n += 1
    total += n

print(f"從 1 加到 {n}，總和首次超過 100")
print(f"總和是：{total}")


# 練習：簡易計算機
print("=== 簡易計算機 ===")

choice = ""

while choice != "q":
    print("\n選項：")
    print("1. 加法")
    print("2. 減法")
    print("q. 離開")
    choice = input("請選擇：")

    if choice == "1":
        a = float(input("第一個數："))
        b = float(input("第二個數："))
        print(f"結果：{a + b}")
    elif choice == "2":
        a = float(input("第一個數："))
        b = float(input("第二個數："))
        print(f"結果：{a - b}")
    elif choice == "q":
        print("再見！👋")
    else:
        print("無效選項！")


# 練習：無窮迴圈，直到使用者輸入 'quit'
while True:
    user_input = input("請輸入（輸入 'quit' 離開）：")
    if user_input == "quit":
        break          # 跳出迴圈，下一課詳細講
    print(f"你輸入了：{user_input}")


# 練習：使用 while 和 else
# while 迴圈也可以搭配 else，當條件式變成 False 時會執行 else 區塊的程式碼。
# 注意：如果迴圈是被 break 跳出，else 區塊不會執行。
count = 1

while count <= 3:
    print(f"第 {count} 次")
    count += 1
else:
    print("迴圈正常結束！")


# 練習：for 迴圈和 while 迴圈的等價寫法
# for 版本
for i in range(1, 6):
    print(i)

# 等價的 while 版本
i = 1
while i <= 5:
    print(i)
    i += 1

# 可以看到 while 版本需要自己處理初始化和更新，而 for 自動幫你處理了。所以當你知道次數或有明確範圍時，for 更簡潔也更安全。

# 練習：條件一開始就是 False 的 while 迴圈
count = 0

while count > 0:     # 0 > 0? False → 迴圈一次都不執行
    print(count)
    count -= 1
# 什麼都不會印


# ❌ 只在迴圈外問一次，迴圈內沒有再問
password = input("密碼：")

while password != "1234":
    print("錯誤！")
    # 忘記再次 input → password 永遠不變 → 無窮迴圈！


# ✅ 迴圈內也要讓使用者重新輸入
password = input("密碼：")

while password != "1234":
    print("錯誤！")
    password = input("密碼：")  # 給使用者重試的機會


x = 10

while x > 0:
    print(x)
    x += 1   # 越來越大，永遠 > 0 → 無窮迴圈！


balance = 10000  # 初始餘額

print("=== 歡迎使用 Python ATM ===")

choice = ""

while choice != "4":
    print(f"\n目前餘額：{balance} 元")
    print("1. 存款")
    print("2. 提款")
    print("3. 查詢餘額")
    print("4. 離開")
    choice = input("請選擇功能：")

    if choice == "1":
        amount = int(input("存款金額："))
        if amount > 0:
            balance += amount
            print(f"已存入 {amount} 元 ✅")
        else:
            print("金額必須大於 0！")

    elif choice == "2":
        amount = int(input("提款金額："))
        if amount > balance:
            print("餘額不足！❌")
        elif amount <= 0:
            print("金額必須大於 0！")
        else:
            balance -= amount
            print(f"已提出 {amount} 元 ✅")

    elif choice == "3":
        print(f"你的餘額是 {balance} 元")

    elif choice == "4":
        print("感謝使用，再見！👋")

    else:
        print("無效選項，請重新選擇！")


x = 10

while x > 0:
    x -= 3
    print(x)


# 執行: python3 第 22 課：迴圈 while — 條件式重複1.py

# === 預期輸出 ===
# 5...
# 4...
# 3...
# 2...
# 1...
# 發射！🚀
# 5
# 4
# 3
# 2
# 1
# 請輸入密碼：
# ⚠️ 本檔需要互動輸入（input()），以上為未輸入時的輸出；請自行執行並輸入資料。
