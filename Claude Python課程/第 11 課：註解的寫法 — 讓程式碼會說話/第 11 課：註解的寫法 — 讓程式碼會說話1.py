# Python's single line comment #, and multi-line comment with triple quotes ''' or """.
oil_temp = 180  # 油溫設定為 180 度（筷子冒泡的程度）

# 這是一行註解
# 計算圓的面積
radius = 5
area = 3.14159 * radius ** 2
print(area)

radius = 5                        # 圓的半徑
area = 3.14159 * radius ** 2      # 面積 = π × r²
print(area)                       # 印出結果

name = "信安"
age = 25
# city = "台北"           # 暫時不需要這個變數
# hobby = "打籃球"        # 之後再加回來

print(f"我是{name}，{age}歲")


# 這是註解 ← Python 會忽略

print("# 這不是註解")   # ← 字串裡面的 # 會正常印出來
# 輸出：# 這不是註解

print("請輸入密碼：# 至少 8 個字元")
# 輸出：請輸入密碼：# 至少 8 個字元

# ✅ 好的寫法（# 後面有空格，比較好讀）
# 計算總金額
price = 100
quantity = 5

total = price * quantity

# ❌ 不好的寫法（擠在一起，不好讀）
#計算總金額
total = price * quantity

# ================================
# 程式名稱：BMI 計算器
# 作者：信安
# 日期：2025-03-01
# 說明：根據身高體重計算 BMI 指數
# ================================

height = float(input("請輸入身高（cm）："))
weight = float(input("請輸入體重（kg）："))
bmi = weight / (height / 100) ** 2
print(f"你的 BMI 是：{bmi:.1f}")

# ================================
"""
程式名稱：BMI 計算器
作者：信安
日期：2025-03-01
說明：根據身高體重計算 BMI 指數
"""

height = float(input("請輸入身高（cm）："))
weight = float(input("請輸入體重（kg）："))
bmi = weight / (height / 100) ** 2
print(f"你的 BMI 是：{bmi:.1f}")

# 使用 0.85 是因為要扣除 15% 的稅金
gross_income = 100000  # 總收入
net_income = gross_income * 0.85  # 扣除稅金後的淨收入
print(f"扣除稅金後的淨收入是：{net_income}")

# BMI = 體重(kg) ÷ 身高(m)²
bmi = weight / (height / 100) ** 2
print(f"你的 BMI 是：{bmi:.1f}")

# TODO: 之後要加上輸入驗證，防止使用者輸入負數
age = int(input("請輸入年齡："))

# FIXME: 這裡的計算在閏年時會出錯
years = int(input("請輸入年數："))
days = years * 365

# ===== 第一步：取得使用者輸入 =====
name = input("姓名：")
age = int(input("年齡："))
height = float(input("身高（cm）："))

# ===== 第二步：進行計算 =====
birth_year = 2025 - age
height_m = height / 100

# ===== 第三步：輸出結果 =====
print(f"姓名：{name}")
print(f"出生年：約 {birth_year} 年")
print(f"身高：{height_m} 公尺")

# ❌ 壞的註解（廢話！程式碼已經很清楚了）
age = 25            # 把 25 存到 age
name = "信安"       # 把 "信安" 存到 name
print(name)         # 印出 name

# ✅ 好的註解（解釋「為什麼」或補充背景）
age = 25            # 預設年齡，用於測試

# ❌ 不需要註解
# 印出歡迎訊息
print("歡迎光臨！")

# ✅ 程式碼本身就很清楚，不需要額外說明
print("歡迎光臨！")

# ❌ 最危險的情況：註解和程式碼不一致！
# 計算稅後收入（稅率 10%）   ← 註解說 10%
salary = 1000
income = salary * 0.85       # ← 程式碼是 15%！到底哪個對？


# ====================================
# ❌ 壞的註解風格
# ====================================

x = 10          # 設定 x 為 10（廢話）
y = x * 2       # y 等於 x 乘以 2（廢話）
z = y + 5       # z 等於 y 加 5（廢話）
print(z)        # 印出 z（廢話）


# ====================================
# ✅ 好的註解風格
# ====================================

# 商品定價策略：基本價加倍後加上 5 元包裝費
base_price = 10
selling_price = base_price * 2
final_price = selling_price + 5     # 包裝費固定 5 元
print(final_price)


# ================================
# 程式名稱：餐廳小費計算器
# 作者：信安
# 日期：2025-03-01
# 功能：根據餐費和服務品質計算小費
# ================================

# ----- 取得使用者輸入 -----
meal_cost = float(input("請輸入餐費金額："))
service = input("服務品質（好/普通/差）：").strip()

# ----- 根據服務品質決定小費比例 -----
# 好 → 20%、普通 → 15%、差 → 10%
if service == "好":
    tip_rate = 0.20
elif service == "普通":
    tip_rate = 0.15
else:
    tip_rate = 0.10    # 預設給 10%

# ----- 計算最終金額 -----
tip = meal_cost * tip_rate
total = meal_cost + tip

# ----- 顯示結果 -----
print()
print(f"餐費：{meal_cost:.0f} 元")
print(f"小費（{tip_rate:.0%}）：{tip:.0f} 元")
# :.0% 會自動把 0.20 顯示成 20%
print(f"總計：{total:.0f} 元")

name = input("Name: ").strip()
h = float(input("Height(cm): "))
w = float(input("Weight(kg): "))

h_m = h / 100
bmi = w / h_m ** 2

print(f"Hi {name}!")
print(f"Your BMI: {bmi:.1f}")

if bmi < 18.5:
    print("體重過輕")
elif bmi < 24:
    print("正常範圍")
elif bmi < 27:
    print("過重")
else:
    print("肥胖")




