# 變數：用來儲存資料的容器
# = 是賦值運算子，將右邊的值存進左邊的變數
x = 10          # 把 10 存進 x
name = "信安"   # 把 "信安" 存進 name

x += 5    # 直接說「把盒子裡的東西加 5」
x -= 3    # 直接說「把盒子裡的東西減 3」

x = 100
name = "信安"
is_student = True
pi = 3.14159
print(x)          # 輸出 100
print(name)       # 輸出 "信安"
print(is_student) # 輸出 True
print(pi)         # 輸出 3.14159


total = 100 + 200          # total = 300
area = 3.14 * 5 ** 2       # area = 78.5
greeting = "Hello" + " " + "World"  # greeting = "Hello World"
print(total)     # 輸出 300
print(area)      # 輸出 78.5
print(greeting)   # 輸出 "Hello World"

# 變數的命名規則：
# 1. 只能包含字母、數字和底線，且不能以數字開頭
# 2. 不能使用 Python 的保留字（如 if、for、while 等）
# 3. 建議使用有意義的名稱，讓程式更易讀 
count = 0
print(count)     # 0

count = count + 1    # 取出 count 的值（0），加 1，再存回 count
print(count)     # 1

count = count + 1    # 取出 count 的值（1），加 1，再存回 count
print(count)     # 2

count = count + 1
print(count)     # 3


# 複合賦值運算子, 可以簡化變數的更新過程
score = 0

score += 10     # score = score + 10 → 10
print(score)    # 10

score += 25     # score = score + 25 → 35
print(score)    # 35

score += 5      # score = score + 5 → 40
print(score)    # 40

# 字串的複合賦值運算子
message = "Hello"
message += " "        # message = "Hello "
message += "World"    # message = "Hello World"
message += "!"        # message = "Hello World!"
print(message)        # Hello World!


# 實用：逐步組合一段文字
result = ""
result += "姓名：信安\n"
result += "年齡：25\n"
result += "城市：台北\n"
print(result)
# 姓名：信安
# 年齡：25
# 城市：台北

# -= 減法賦值運算子
health = 100    # 遊戲角色的血量

health -= 20    # 被攻擊，扣 20 血
print(health)   # 80

health -= 35    # 又被攻擊，扣 35 血
print(health)   # 45

health -= 50    # 再被攻擊，扣 50 血
print(health)   # -5（角色死亡！）


# 實用：倒數計時
remaining = 10
remaining -= 1    # 9
remaining -= 1    # 8
remaining -= 1    # 7


# *= 乘法賦值運算子
money = 1000

money *= 2      # 翻倍 → 2000
print(money)    # 2000

money *= 1.1    # 加 10% 利息 → 2200.0
print(money)    # 2200.0


pattern = "★"
pattern *= 5     # pattern = "★" * 5
print(pattern)   # ★★★★★

# /= 除法賦值運算子
pizza = 8       # 8 片披薩

pizza /= 2      # 分給 2 個人
print(pizza)    # 4.0 ← 注意！/= 結果永遠是 float

pizza /= 2      # 再分一半
print(pizza)    # 2.0

# //= 整除賦值運算子
candies = 25    # 25 顆糖果

candies //= 4   # 分給 4 個人，每人幾顆？（不算零頭）
print(candies)   # 6

candies //= 2    # 再分一半
print(candies)   # 3

# %= 取餘數賦值運算子
x = 27

x %= 10         # x = 27 % 10 → 取個位數
print(x)        # 7

x %= 4          # x = 7 % 4
print(x)        # 3

# **= 指數賦值運算子
base = 2

base **= 3      # base = 2 ** 3 → 8
print(base)     # 8

base **= 2      # base = 8 ** 2 → 64
print(base)     # 64


# 一般寫法（三行）
x = 10
y = 20
z = 30

# Python 特色寫法（一行搞定！）
# 同時給多個變數賦值，從右邊的值依序存進左邊的變數
# 注意：右邊的值數量必須和左邊的變數數量一樣，否則會報錯！
x, y, z = 10, 20, 30

print(x)    # 10
print(y)    # 20
print(z)    # 30


# 一行接收多個值
name, age, city = "信安", 25, "台北"
print(f"{name}，{age}歲，住在{city}")


# 同時給多個變數賦值，右邊的值會依序存進左邊的變數
# 一般寫法
a = 0
b = 0
c = 0

# 簡化寫法
a = b = c = 0

print(a, b, c)    # 0 0 0


# 同時把三個計數器歸零, 適用於統計勝負平的遊戲
wins = losses = draws = 0


# Python 的魔法寫法！
a = 10
b = 20

# 交換 a 和 b 的值
# a, b = b, a
# 步驟 1：先計算右邊 → b, a → (20, 10)  ← 先把右邊打包起來
# 步驟 2：再賦值左邊 → a=20, b=10       ← 再分配給左邊
a, b = b, a    # 一行搞定！

print(a, b)    # 20 10

# 也可以同時輪轉多個變數的值
a, b, c = 1, 2, 3
a, b, c = c, a, b    # 輪轉！

print(a, b, c)        # 3 1 2


# 從串列中取出值（第 25 課會深入學）
coordinates = [10, 20]
x, y = coordinates

print(x)    # 10
print(y)    # 20

# 從多個回傳值取出（第 37 課會深入學）
# name, age = get_user_info()

# 實用：事件計數器
count = 0

# 每次事件發生就加 1
count += 1    # 1
count += 1    # 2
count += 1    # 3

print(f"事件發生了 {count} 次")    # 事件發生了 3 次


# 實用：累加消費金額
total = 0

# 逐筆加上不同金額
total += 150    # 第一筆消費
total += 280    # 第二筆消費
total += 95     # 第三筆消費

print(f"消費總額：{total} 元")    # 消費總額：525 元


# 實用：細菌分裂
bacteria = 1    # 一隻細菌

# 每小時分裂一次（數量翻倍）
bacteria *= 2    # 2
bacteria *= 2    # 4
bacteria *= 2    # 8
bacteria *= 2    # 16

print(f"細菌數量：{bacteria}")    # 細菌數量：16


# 實用：燃料消耗
fuel = 100      # 燃料 100%

fuel -= 15      # 消耗 15%
fuel -= 15      # 再消耗 15%
fuel -= 15      # 再消耗 15%

print(f"剩餘燃料：{fuel}%")    # 剩餘燃料：55%


# ========================================
# 🛒 購物車計算器
# ========================================

total = 0           # 總金額從 0 開始
item_count = 0      # 商品數從 0 開始

print("🛒 歡迎使用購物車！")
print("=" * 35)

# 第一項商品
item1 = input("第 1 項商品名稱：")
price1 = float(input(f"  {item1} 的價格："))
total += price1
item_count += 1

# 第二項商品
item2 = input("第 2 項商品名稱：")
price2 = float(input(f"  {item2} 的價格："))
total += price2
item_count += 1

# 第三項商品
item3 = input("第 3 項商品名稱：")
price3 = float(input(f"  {item3} 的價格："))
total += price3
item_count += 1

# 計算折扣
discount = 0
if total >= 1000:
    discount = total * 0.1     # 滿千打 9 折
    total -= discount           # 扣掉折扣金額

# 顯示結果
print()
print("=" * 35)
print("🧾 購物明細")
print("=" * 35)
print(f"  商品數量：{item_count} 項")
print(f"  小計：{total + discount:.0f} 元")
if discount > 0:
    print(f"  折扣（9折）：-{discount:.0f} 元")
print(f"  應付金額：{total:.0f} 元")
print("=" * 35)



# ========================================
# ⚔️ 簡易戰鬥模擬器
# ========================================

# ----- 角色初始化 -----
hero_hp = 100
hero_attack = 25

monster_hp = 80
monster_attack = 15

round_num = 0

print("⚔️ 戰鬥開始！")
print(f"  勇者 HP：{hero_hp}  |  怪物 HP：{monster_hp}")
print("=" * 40)

# ----- 第 1 回合 -----
round_num += 1
print(f"\n【第 {round_num} 回合】")
monster_hp -= hero_attack
hero_hp -= monster_attack
print(f"  勇者攻擊！怪物 HP：{monster_hp}")
print(f"  怪物反擊！勇者 HP：{hero_hp}")

# ----- 第 2 回合 -----
round_num += 1
print(f"\n【第 {round_num} 回合】")
monster_hp -= hero_attack
hero_hp -= monster_attack
print(f"  勇者攻擊！怪物 HP：{monster_hp}")
print(f"  怪物反擊！勇者 HP：{hero_hp}")

# ----- 第 3 回合 -----
round_num += 1
print(f"\n【第 {round_num} 回合】")
monster_hp -= hero_attack
hero_hp -= monster_attack
print(f"  勇者攻擊！怪物 HP：{monster_hp}")
print(f"  怪物反擊！勇者 HP：{hero_hp}")

# ----- 結算 -----
print()
print("=" * 40)
print("📊 戰鬥結算")
print(f"  經過 {round_num} 回合")
print(f"  勇者剩餘 HP：{hero_hp}")
print(f"  怪物剩餘 HP：{monster_hp}")

if monster_hp <= 0 and hero_hp > 0:
    print("🎉 勇者獲勝！")
elif hero_hp <= 0 and monster_hp > 0:
    print("💀 勇者戰敗...")
elif hero_hp <= 0 and monster_hp <= 0:
    print("💥 同歸於盡！")
else:
    print("⏳ 戰鬥尚未結束...")



# ========================================
# 🏦 存錢計劃模擬器
# ========================================

savings = float(input("目前存款（元）："))
monthly_save = float(input("每月存入（元）："))
interest_rate = float(input("年利率（%）：")) / 100

monthly_rate = interest_rate / 12   # 月利率

# 模擬 6 個月
month = 0

month += 1
savings += monthly_save
savings *= (1 + monthly_rate)
print(f"第 {month} 個月：{savings:,.0f} 元")

month += 1
savings += monthly_save
savings *= (1 + monthly_rate)
print(f"第 {month} 個月：{savings:,.0f} 元")

month += 1
savings += monthly_save
savings *= (1 + monthly_rate)
print(f"第 {month} 個月：{savings:,.0f} 元")

month += 1
savings += monthly_save
savings *= (1 + monthly_rate)
print(f"第 {month} 個月：{savings:,.0f} 元")

month += 1
savings += monthly_save
savings *= (1 + monthly_rate)
print(f"第 {month} 個月：{savings:,.0f} 元")

month += 1
savings += monthly_save
savings *= (1 + monthly_rate)
print(f"第 {month} 個月：{savings:,.0f} 元")


x = 10       # ✅ 賦值：把 10 存進 x
x == 10      # ✅ 比較：x 等於 10 嗎？→ True

# ❌ 常見錯誤：想賦值卻寫成比較
# if x == 5:     # 這是比較，不是賦值
#     ...


# ❌ total 還不存在，不能用 +=
# total += 100   # NameError: name 'total' is not defined

# ✅ 要先初始化
total = 0
total += 100     # 正確！


x = 10

x += 5      # ✅ x = x + 5 → 15

x =+ 5      # ⚠️ 這不會報錯，但意思完全不同！
             # 這是 x = (+5) → x 變成 5
             # +5 是「正的 5」，所以等於 x = 5


x = 10
x =- 3      # ⚠️ 這是 x = (-3) → x 變成 -3
             # 不是 x = x - 3！

x -= 3      # ✅ 這才是 x = x - 3 → x 變成 7


x = 10
x /= 2
print(x)        # 5.0 ← 不是 5！是 float

# 如果你需要整數結果，用 //=
x = 10
x //= 2
print(x)        # 5 ← 整數


x = 10
x += 5
print(x)

x *= 2
print(x)

x -= 8
print(x)

x //= 3
print(x)

x **= 2
print(x)

x %= 5
print(x)





