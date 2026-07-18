# 第 18 課：if-else 與 if-elif-else — 多重條件
# 這段程式碼會詢問使用者的年齡，然後根據年齡判斷是否已成年，並輸出相應的訊息。
# 使用者輸入的年齡會被轉換成整數，然後使用 if-else 條件語句來判斷年齡是否大於或等於 18。
# 如果年齡大於或等於 18，程式會輸出 "你已成年，可以投票！"；否則，程式會輸出 "你還未成年，再等幾年吧！"。
age = int(input("請輸入你的年齡："))

if age >= 18:
    print("你已成年，可以投票！")
else:
    print("你還未成年，再等幾年吧！")


# 判斷奇偶數
number = int(input("輸入一個整數："))

if number % 2 == 0:
    print(f"{number} 是偶數")
else:
    print(f"{number} 是奇數")


# 密碼驗證（改良版，比第 17 課多了「失敗」的回饋）
password = input("請輸入密碼：")

if password == "python123":
    print("密碼正確，歡迎進入！")
else:
    print("密碼錯誤，拒絕存取！")


# 成績評分
# 這段程式碼會根據分數來評分，使用了多重的 if-else 條件語句來判斷分數的範圍。
# 如果分數大於或等於 90，程式會輸出 "A"；如果分數大於或等於 80，程式會輸出 "B"；如果分數大於或等於 70，程式會輸出 "C"；如果分數大於或等於 60，程式會輸出 "D"；否則，程式會輸出 "F"。
# 這段程式碼的結構比較複雜，因為每個條件都嵌套在前一個條件的 else 區塊中，這樣會導致程式碼的可讀性降低。
# 這也是為什麼在實際開發中，我們通常會使用 if-elif-else 結構來處理多重條件，這樣可以讓程式碼更清晰易讀。
score = 85

if score >= 90:
    print("A")
else:
    if score >= 80:
        print("B")
    else:
        if score >= 70:
            print("C")
        else:
            if score >= 60:
                print("D")
            else:
                print("F")

# 成績評分（改良版，使用 if-elif-else 結構）
score = int(input("請輸入成績："))

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


# 這段程式碼展示了 if-elif-else 結構的優點，讓程式碼更清晰易讀。
# 在第一個範例中，條件的順序不正確，導致分數 95 被誤判為 D。
# 在第二個範例中，條件的順序正確，先判斷最高分數的條件，然後依次判斷較低的條件，這樣就能正確地評分了。
score = 95

if score >= 60:      # 95 >= 60 → True → 命中！
    print("D")       # 輸出 D... 但 95 分應該是 A 啊！
elif score >= 70:    # ← 根本不會檢查到這裡
    print("C")
elif score >= 80:
    print("B")
elif score >= 90:
    print("A")


score = 95

if score >= 90:      # 最嚴格的先判斷
    print("A")       # ✅ 95 >= 90 → True → 輸出 A
elif score >= 80:
    print("B")
elif score >= 70:
    print("C")
elif score >= 60:
    print("D")
else:
    print("F")


day = input("今天星期幾？(Mon/Tue/Wed/Thu/Fri/Sat/Sun)：")

if day == "Sat":
    print("星期六！去爬山")
elif day == "Sun":
    print("星期日！在家耍廢")
# 沒有 else → 輸入其他值時什麼都不做
# 這段程式碼會根據使用者輸入的星期幾來決定要做什麼活動。
# 如果使用者輸入 "Sat"，程式會輸出 "星期六！去爬山"；如果使用者輸入 "Sun"，程式會輸出 "星期日！在家耍廢"。
# 如果使用者輸入其他值，程式不會有任何反應，因為沒有 else 區塊來處理其他情況。


month = int(input("請輸入月份 (1-12)："))

# 這段程式碼會根據使用者輸入的月份來判斷季節。
# [] 中的內容是列表，表示該季節包含的月份。例如，春天包含 3、4、5 月；夏天包含 6、7、8 月；秋天包含 9、10、11 月；冬天包含 12、1、2 月。
# in 關鍵字用來檢查 month 是否在對應的列表中，如果是，就輸出該季節的名稱和表情符號。
if month in [3, 4, 5]:
    print("春天 🌸")
elif month in [6, 7, 8]:
    print("夏天 ☀️")
elif month in [9, 10, 11]:
    print("秋天 🍂")
elif month in [12, 1, 2]:
    print("冬天 ❄️")
else:
    print("無效的月份！")
# 這段程式碼會根據使用者輸入的月份來判斷季節。
# 如果使用者輸入的月份是 3、4 或 5，程式會輸出 "春天 🌸"；如果使用者輸入的月份是 6、7 或 8，程式會輸出 "夏天 ☀️"；
# 如果使用者輸入的月份是 9、10 或 11，程式會輸出 "秋天 🍂"；如果使用者輸入的月份是 12、1 或 2，程式會輸出 "冬天 ❄️"。


age = int(input("年齡："))
student = input("是否為學生？(y/n)：")

if age < 12:
    print("兒童票：100 元")
elif age >= 65:
    print("敬老票：150 元")
elif student == "y":
    print("學生票：200 元")
else:
    print("全票：300 元")
# 這段程式碼會根據使用者的年齡和是否為學生來判斷票價。
# 如果年齡小於 12，程式會輸出 "兒童票：100 元"；如果年齡大於或等於 65，程式會輸出 "敬老票：150 元"；
# 如果使用者是學生（輸入 "y"），程式會輸出 "學生票：200 元"；否則，程式會輸出 "全票：300 元"。

# 陷阱 1：多個 if vs if-elif
# 這段程式碼展示了多個獨立的 if 條件和 if-elif 條件的差異。
score = 95

# 寫法 A：多個獨立的 if（每個都會判斷）
if score >= 90:
    print("A")    # ✅ 會印
if score >= 80:
    print("B")    # ✅ 也會印！因為 95 也 >= 80
if score >= 70:
    print("C")    # ✅ 也會印！

# 寫法 B：if-elif（命中就停止）
if score >= 90:
    print("A")    # ✅ 會印
elif score >= 80:
    print("B")    # ❌ 不會印，因為上面已經命中了
elif score >= 70:
    print("C")    # ❌ 不會印


#if score >= 60:
#    print("及格")
#else score < 60:     # ❌ SyntaxError！else 後面不能寫條件
#    print("不及格")


# ✅ 正確寫法
if score >= 60:
    print("及格")
else:                # else 就是「其他所有情況」
    print("不及格")


#age = input("輸入年齡：")   # 回傳的是字串！
#
#if age >= 18:               # ❌ TypeError: '>=' not supported
#    print("成年")            #    between instances of 'str' and 'int'


# ✅ 正確寫法
age = int(input("輸入年齡："))  # 記得轉成 int

if age >= 18:
    print("成年")

# 這段程式碼會根據使用者輸入的身高和體重來計算 BMI，然後根據 BMI 的值來判斷體重狀況。
height = float(input("請輸入身高（公分）："))
weight = float(input("請輸入體重（公斤）："))

height_m = height / 100  # 公分轉公尺
bmi = weight / (height_m ** 2)

print(f"你的 BMI 是：{bmi:.1f}")

if bmi < 18.5:
    print("體重過輕，要多吃一點！")
elif bmi < 24:
    print("體重正常，繼續保持！")
elif bmi < 27:
    print("稍微過重，注意飲食！")
else:
    print("體重過重，建議運動加飲食控制")


score = int(input("輸入成績："))

if score >= 60:
    print("及格")
if score >= 70:
    print("良好")
if score >= 80:
    print("優秀")




