# 印出不同類型的資料
print("Hello!")        # 印出字串
print(123)             # 印出整數
print(3.14)            # 印出浮點數
print(True)            # 印出布林值

# 印出多個資料
name = "信安"
age = 25

# 用逗號分隔，print 會自動用「空格」把它們連在一起
print("我叫", name, "，今年", age, "歲")
# 輸出：我叫 信安 ，今年 25 歲

# 字串拼接方式（需要手動轉型）
print("我叫" + name + "，今年" + str(age) + "歲")
# 輸出：我叫信安，今年25歲

# 逗號方式（不需要轉型，但會多空格）
print("我叫", name, "，今年", age, "歲")
# 輸出：我叫 信安 ，今年 25 歲

# sep 參數：控制多個資料之間的分隔符號
# 預設：空格分隔
print("A", "B", "C")
# 輸出：A B C

# 用 sep 自訂分隔符號
print("A", "B", "C", sep="-")
# 輸出：A-B-C

print("A", "B", "C", sep="★")
# 輸出：A★B★C

print("A", "B", "C", sep="")
# 輸出：ABC（沒有任何分隔）

print("A", "B", "C", sep="\n")
# 輸出：
# A
# B
# C

# 練習：印出日期，格式為「年/月/日」
year = 2025
month = 3
day = 1

print(year, month, day, sep="/")
# 輸出：2025/3/1

print(year, month, day, sep="-")
# 輸出：2025-3-1

print(year, month, day, sep=".")
# 輸出：2025.3.1

print(year, month, day, sep="")
# 輸出：202531

# end 參數：控制 print 結尾的字串
# 預設：每個 print 結尾會換行
print("Hello")
print("World")
# 輸出：
# Hello
# World

# 用 end 改變結尾
print("Hello", end=" ")
print("World")
# 輸出：Hello World（在同一行！）

print("Hello", end="→")
print("World")
# 輸出：Hello→World

print("載入中", end="")
print("...", end="")
print("完成！")
# 輸出：載入中...完成！

# 練習：印出倒數計時，格式為「倒數：3...2...1...發射！」
print("倒數：", end="")
print("3", end="...")
print("2", end="...")
print("1", end="...")
print("發射！")
# 輸出：倒數：3...2...1...發射！

# 綜合練習：印出水果清單，格式為「蘋果、香蕉、橘子。」
print("蘋果", "香蕉", "橘子", sep="、", end="。\n")
# 輸出：蘋果、香蕉、橘子。

print("A", "B", "C", sep=" + ", end=" = ?\n")
# 輸出：A + B + C = ?

# 練習：印出多行文字，並在段落之間留一行空白
print()   # 印出一個空行，用來讓畫面更整齊

print("第一段")
print()
print("第二段")
# 輸出：
# 第一段
#
# 第二段

# 特殊字元：\n、\t、\\、\"、\'
# \n = 換行
print("第一行\n第二行\n第三行")
# 輸出：
# 第一行
# 第二行
# 第三行

# \t = Tab 鍵（一段固定寬度的空格）
print("姓名\t年齡\t城市")
print("信安\t25\t台北")
print("小明\t30\t高雄")
# 輸出：
# 姓名	年齡	城市
# 信安	25	台北
# 小明	30	高雄

# \\ = 顯示一個反斜線
print("檔案路徑：C:\\Users\\Documents")
# 輸出：檔案路徑：C:\Users\Documents

# \" 或 \' = 在字串中顯示引號
print("他說：\"你好！\"")
# 輸出：他說："你好！"

# 練習：印出以下圖案
# 方法一：使用三引號
print("""
============================
     歡迎使用 Python 程式
============================
""")

# 方法二：使用 \n
print("============================\n     歡迎使用 Python 程式\n============================")

# 練習：讓使用者輸入名字，然後印出問候語
name = input("請輸入你的名字：")
print("你好，" + name + "！")

# 練習：讓使用者輸入兩個數字，然後印出它們的和
num = input("請輸入數字：")    # 假設輸入 42
print(num)                      # 42
print(type(num))                # <class 'str'> ← 是字串！

print(num + num)                # "4242" ← 字串拼接，不是數學加法！
print(int(num) + int(num))      # 84     ← 轉型後才是數學加法

# 練習：讓使用者輸入年齡，並把它轉換成整數
# 寫法一：分兩行（初學者友好，容易理解）
text = input("請輸入年齡：")
age = int(text)

# 寫法二：一行搞定（最常用的慣用寫法）
age = int(input("請輸入年齡："))

# 寫法三：浮點數版本
height = float(input("請輸入身高（cm）："))

# 練習：讓使用者輸入名字，示範有提示文字和沒有提示文字的差別
# 有提示文字（推薦）
name = input("請輸入名字：")

# 沒有提示文字（程式會暫停等待輸入，但使用者不知道要做什麼）
name = input()

# 練習：示範使用者輸入前後有多餘空格的問題，以及如何用 strip() 去除空格
name = input("請輸入名字：")   # 使用者輸入 "  信安  "
print("【" + name + "】")       # 【  信安  】← 有多餘空格！

# 使用 strip() 去除前後空格
name = input("請輸入名字：").strip()
print("【" + name + "】")       # 【信安】✅

# 練習：示範不同的字串拼接方式
name = "信安"
age = 25

# 方法一：字串拼接（麻煩，需要 str()）
print("我叫" + name + "，今年" + str(age) + "歲")

# 方法二：逗號分隔（簡單，但會多空格）
print("我叫", name, "，今年", age, "歲")

# 方法三：f-string（最推薦！簡潔又好讀 ✨）
print(f"我叫{name}，今年{age}歲")

# 練習：示範 f-string 的進階用法，包含數字格式化
a = 10
b = 3

print(f"{a} + {b} = {a + b}")        # 10 + 3 = 13
print(f"{a} × {b} = {a * b}")        # 10 × 3 = 30
print(f"{a} ÷ {b} = {a / b:.2f}")    # 10 ÷ 3 = 3.33（保留兩位小數）

# 綜合練習：讓使用者輸入名字和年齡，然後印出問候語和明年的年齡
name = input("你的名字：")
age = int(input("你的年齡："))

print(f"你好，{name}！你明年就 {age + 1} 歲了。")

# 綜合練習：讓使用者輸入姓名、年齡、城市和興趣，然後印出一張個人資料卡
print("=" * 30)
print("      個人資料填寫")
print("=" * 30)

name = input("請輸入姓名：").strip()
age = int(input("請輸入年齡："))
city = input("請輸入居住城市：").strip()
hobby = input("請輸入興趣：").strip()

print()
print("=" * 30)
print("      你的個人資料卡")
print("=" * 30)
print(f"  姓名：{name}")
print(f"  年齡：{age}")
print(f"  城市：{city}")
print(f"  興趣：{hobby}")
print(f"  出生年：約 {2025 - age} 年")
print("=" * 30)

# 綜合練習：讓使用者輸入三科成績，然後印出總分和平均分數
print("📊 成績計算器")
print("-" * 25)

chinese = float(input("國文成績："))
english = float(input("英文成績："))
math_score = float(input("數學成績："))

total = chinese + english + math_score
average = total / 3

print()
print("📋 成績報告")
print("-" * 25)
print(f"  國文：{chinese}")
print(f"  英文：{english}")
print(f"  數學：{math_score}")
print("-" * 25)
print(f"  總分：{total}")
print(f"  平均：{average:.1f}")
print("-" * 25)

# 練習：讓使用者輸入寬度，然後用 print() 和字串重複畫出簡單圖形
# print() 搭配字串重複，畫出簡單圖形
width = int(input("請輸入寬度（5-20）："))

print("=" * width)
print("*" * width)
print("=" * width)

# 練習：示範使用者輸入的資料類型問題，以及如何用 int() 轉換成數字
num = input("輸入數字：")   # 輸入 5
print(num * 3)               # "555" 不是 15！

# 修正
num = int(input("輸入數字："))
print(num * 3)               # 15 ✅

# 練習：示範 f-string 的字串插值功能，以及不使用 f-string 時的問題
name = "信安"
print("你好，{name}")     # 輸出：你好，{name} ← 大括號原樣輸出！
print(f"你好，{name}")    # 輸出：你好，信安 ✅

# 練習：示範 print() 的參數位置問題，sep 和 end 必須放在最後面
# ❌ sep 和 end 必須放在最後面
# print(sep="-", "A", "B")     # SyntaxError!

# ✅ 正確寫法
print("A", "B", sep="-")     # A-B

# ❌ Python 3 中 print 是函數，必須加括號
# print "Hello"     # SyntaxError!

# ✅ 正確
print("Hello")







