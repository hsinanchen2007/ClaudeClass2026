# 24 課：已狀態迴圈 — 迴圈中的迴圈
# 這一課我們要介紹「迴圈中的迴圈」，也就是「巢狀迴圈」。巢狀迴圈的概念是：在一個迴圈裡面，再寫一個迴圈。
# 這樣的結構可以讓我們在外層迴圈的每一次執行中，內層迴圈都會完整地執行一次。這種結構非常適合用來處理多維資料或是需要多層次重複的情況。
for floor in range(1, 4):          # 外層：1 樓到 3 樓
    for room in range(1, 5):       # 內層：1 號房到 4 號房
        print(f"  {floor} 樓 {room} 號房")
    print(f"--- {floor} 樓巡完了 ---\n")


# 這裡我們用巢狀迴圈來印出九九乘法表。外層迴圈控制被乘數（1~9），內層迴圈控制乘數（1~9）。
# 每次內層迴圈執行完畢後，我們換行，這樣就能整齊地印出整個乘法表。
for i in range(1, 10):             # 被乘數：1~9
    for j in range(1, 10):         # 乘數：1~9
        print(f"{i} × {j} = {i*j:2d}", end="   ")
    print()                        # 每一列印完後換行


for i in range(1, 10):
    for j in range(1, i + 1):     # j 的上限隨 i 變化！
        print(f"{i}×{j}={i*j:<2d}", end="  ")
    print()
# 在這個例子中，內層迴圈的上限是 i + 1，這意味著每一列的乘數 j 都會從 1 到 i 變化。這樣就形成了九九乘法表的三角形排列。



rows = 4
cols = 6

for r in range(rows):
    for c in range(cols):
        print("★", end=" ")
    print()
# 這段程式碼使用巢狀迴圈來印出一個由星號組成的矩形。外層迴圈控制行數（4 行），內層迴圈控制列數（6 列）。每次內層迴圈執行完畢後，我們換行，這樣就能整齊地印出整個矩形。


rows = 5

for i in range(1, rows + 1):
    for j in range(i):
        print("★", end=" ")
    print()
# 這段程式碼使用巢狀迴圈來印出一個由星號組成的三角形。外層迴圈控制行數（5 行），內層迴圈的上限隨外層迴圈變化，這樣就形成了三角形的排列。


rows = 5

for i in range(rows, 0, -1):     # 5, 4, 3, 2, 1
    for j in range(i):
        print("★", end=" ")
    print()
# 這段程式碼使用巢狀迴圈來印出一個由星號組成的倒三角形。外層迴圈從 5 開始遞減到 1，內層迴圈的上限隨外層迴圈變化，這樣就形成了倒三角形的排列。


rows = 5

# 上半部（含中間最寬的一行）
for i in range(1, rows + 1):
    spaces = " " * (rows - i)
    stars = "★ " * i
    print(spaces + stars)

# 下半部
for i in range(rows - 1, 0, -1):
    spaces = " " * (rows - i)
    stars = "★ " * i
    print(spaces + stars)
# 這段程式碼使用巢狀迴圈來印出一個由星號組成的菱形。外層迴圈分為兩部分：上半部和下半部。內層迴圈根據當前行數計算需要印出的空格和星號，這樣就形成了菱形的排列。


# 二維串列（串列的串列）
scores = [
    [85, 72, 90],    # 小明的成績
    [68, 95, 78],    # 小華的成績
    [92, 88, 85],    # 小美的成績
]
names = ["小明", "小華", "小美"]
subjects = ["國文", "英文", "數學"]

for i in range(len(names)):                # 外層：每個學生
    print(f"\n{names[i]} 的成績：")
    student_total = 0
    for j in range(len(subjects)):         # 內層：每個科目
        print(f"  {subjects[j]}：{scores[i][j]} 分")
        student_total += scores[i][j]
    average = student_total / len(subjects)
    print(f"  → 平均：{average:.1f} 分")
# 這段程式碼使用巢狀迴圈來印出每個學生的成績和平均分數。外層迴圈控制學生，內層迴圈控制科目。
# 透過索引，我們可以從二維串列中取得每個學生在每個科目的成績，並計算平均分數。


for i in range(1, 4):
    print(f"外層 i = {i}")
    for j in range(1, 4):
        if j == 2:
            break              # 只跳出內層迴圈！
        print(f"  內層 j = {j}")
    print(f"  （內層結束）")    # 外層繼續
# 這段程式碼示範了在巢狀迴圈中使用 break 的效果。當 j 等於 2 時，內層迴圈會被跳出，但外層迴圈仍然會繼續執行。
# 這樣我們可以看到每次外層迴圈執行時，內層迴圈只會印出 j = 1 的情況，當 j = 2 時就會跳出內層迴圈。


# 在巢狀迴圈中，如果我們想要在找到特定條件時跳出所有迴圈，我們可以使用一個旗標變數來達成。
found = False

for i in range(5):
    for j in range(5):
        if i == 2 and j == 3:
            print(f"找到目標！位置 ({i}, {j})")
            found = True
            break          # 跳出內層
    if found:
        break              # 跳出外層


# 另一種方法是直接在函數中使用 return，這樣一旦找到目標就會直接結束整個函數，不需要額外的旗標變數。
# 這段程式碼示範了在巢狀迴圈中使用 return 的效果。當 i 等於 2 且 j 等於 3 時，函數會直接返回該位置的座標，並結束整個函數的執行。
# 這樣我們就不需要使用額外的旗標變數來控制迴圈的跳出，程式碼也會更簡潔明瞭。
def search():
    for i in range(5):
        for j in range(5):
            if i == 2 and j == 3:
                return (i, j)   # return 直接結束整個函數
    return None

result = search()
print(f"找到位置：{result}")


# 巢狀迴圈的時間複雜度
# 兩層迴圈
for i in range(100):        # 100 次
    for j in range(100):    # 100 次
        pass                # 總共 100 × 100 = 10,000 次

# 三層迴圈
for i in range(100):        # 100 次
    for j in range(100):    # 100 次
        for k in range(100): # 100 次
            pass             # 總共 100 × 100 × 100 = 1,000,000 次！


# 這段程式碼示範了在 while 迴圈中使用另一個 while 迴圈的結構。外層的 while 迴圈控制 i 的值，內層的 while 迴圈控制 j 的值。
i = 1

while i <= 3:
    j = 1
    while j <= 3:
        print(f"({i},{j})", end=" ")
        j += 1
    print()
    i += 1


rows = int(input("請輸入排數："))
seats_per_row = int(input("每排幾個座位："))

print(f"\n=== 座位表（{rows} 排 × {seats_per_row} 座）===\n")

total_seats = 0

for r in range(1, rows + 1):
    print(f"第 {r:2d} 排：", end="")
    for s in range(1, seats_per_row + 1):
        seat_number = (r - 1) * seats_per_row + s
        print(f"  [{seat_number:3d}]", end="")
        total_seats += 1
    print()

print(f"\n總共 {total_seats} 個座位")

# 找特定座位
target = int(input("\n查詢座位號碼："))
target_row = (target - 1) // seats_per_row + 1
target_seat = (target - 1) % seats_per_row + 1
print(f"座位 {target} 在第 {target_row} 排、第 {target_seat} 個位置")
# 這段程式碼示範了如何使用巢狀迴圈來印出一個座位表，並且計算總座位數。外層迴圈控制排數，內層迴圈控制每排的座位數。
# 在印出座位表後，我們還可以讓使用者輸入一個座位號碼，然後計算出該座位所在的排數和位置，這樣就能快速地找到特定座位的位置。


for i in range(3):
    for j in range(3):
        if i == j:
            continue
        print(f"({i},{j})", end=" ")
    print()
# 這段程式碼示範了在巢狀迴圈中使用 continue 的效果。當 i 和 j 相等時，內層迴圈會跳過該次迭代，繼續執行下一次迭代。
# 這樣我們就不會印出 (0,0)、(1,1)、(2,2) 這些座標，而是只印出 i 和 j 不相等的組合。

