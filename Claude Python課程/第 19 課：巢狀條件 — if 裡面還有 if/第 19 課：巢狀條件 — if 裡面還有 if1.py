# 已狀條件 if 裡面還有 if
# 這裡我們要先判斷使用者是否有票，如果有票的話才會繼續判斷身高是否符合搭乘雲霄飛車的要求。

# ═══════════════════════════════════════════════════════════════════════════
# 【面試題】巢狀條件
# ───────────────────────────────────────────────────────────────────────────
# 🔥 Q1. 巢狀 if 與用 `and` 合併，什麼時候該選哪個？
#     答：純粹要「兩個條件都成立」時，`and` 更簡潔；
#     但需要**區分是哪一個條件失敗**時，必須用巢狀——
#     `if user == "admin": if pwd == "1234": ... else: 密碼錯` 能分別回報
#     「查無帳號」與「密碼錯誤」，合併成 `and` 就只剩籠統的「登入失敗」。
#     追問：登入功能實務上該分開提示嗎？（不該——那會洩漏帳號是否存在，
#     這是「程式邏輯能區分」與「應該告訴使用者」的差別）
#
# 🔥 Q2. 巢狀太深時怎麼重構？
#     答：把層層巢狀改寫成**扁平的 elif 逐條排除**：
#     `if not has_ticket: ... elif age < 12: ... elif height < 140: ... else: 通過`。
#     邏輯完全等價，但縮排從五層變一層，每個拒絕理由也一目了然。
#     追問：這個模式叫什麼？（guard clause／early rejection）
#
# ⚠️ 陷阱. `if a and b:` 在 a 為假時，b 會被求值嗎？
#     答：**不會**——這正是短路求值，也是巢狀 if 與 `and` 真正等價的原因。
#     所以 `if i < len(L) and L[i] > 0:` 是安全的，順序反過來就會 IndexError。
#     為什麼會錯：以為 `and` 兩邊都會先算好再判斷。
#     一旦這樣理解，就會誤以為必須寫成巢狀 if 才安全，
#     或反過來把有副作用的呼叫放在 `and` 右邊，結果它時有時無地被執行。
# ═══════════════════════════════════════════════════════════════════════════

has_ticket = input("你有票嗎？(y/n)：")

if has_ticket == "y":
    height = int(input("你的身高是幾公分？"))
    if height >= 140:
        print("歡迎搭乘雲霄飛車！🎢")
    else:
        print("抱歉，身高不足 140 公分，無法搭乘")
else:
    print("請先去購票處買票！")


username = input("帳號：")

if username == "admin":
    password = input("密碼：")
    if password == "1234":
        print("登入成功！歡迎管理員 👋")
    else:
        print("密碼錯誤！")
else:
    print("查無此帳號！")


is_member = input("是否為會員？(y/n)：")
total = int(input("消費金額："))

if is_member == "y":
    if total >= 1000:
        discount = total * 0.8
        print(f"會員 + 滿千 → 打 8 折，應付 {discount:.0f} 元")
    elif total >= 500:
        discount = total * 0.9
        print(f"會員 + 滿五百 → 打 9 折，應付 {discount:.0f} 元")
    else:
        discount = total * 0.95
        print(f"會員基本優惠 → 打 95 折，應付 {discount:.0f} 元")
else:
    if total >= 1000:
        discount = total * 0.9
        print(f"非會員 + 滿千 → 打 9 折，應付 {discount:.0f} 元")
    else:
        discount = total
        print(f"非會員原價，應付 {discount:.0f} 元")


# 巢狀寫法
# if age >= 18:
#    if has_id:
#        print("可以進入")
# 用 and 簡化（效果完全一樣）
#if age >= 18 and has_id:
#    print("可以進入")


# 巢狀寫法（能區分錯誤原因）
username = input("帳號：")

if username == "admin":
    password = input("密碼：")
    if password == "1234":
        print("登入成功！")
    else:
        print("密碼錯誤！")      # ← 知道是密碼的問題
else:
    print("查無此帳號！")          # ← 知道是帳號的問題


# 用 and（無法區分錯誤原因）
if username == "admin" and password == "1234":
    print("登入成功！")
else:
    print("登入失敗！")  # ← 不知道是帳號錯還是密碼錯


# if has_ticket:
#     if age >= 12:
#         if height >= 140:
#             if not has_heart_disease:
#                 if not is_pregnant:
#                     print("歡迎搭乘！")
#                 else:
#                     print("孕婦不能搭乘")
#             else:
#                 print("心臟病患者不能搭乘")
#         else:
#             print("身高不足")
#     else:
#         print("年齡不足")
# else:
#     print("請先購票")


has_ticket = True
age = 15
height = 155
has_heart_disease = False
is_pregnant = False

# 一層一層排除不符合的情況
if not has_ticket:
    print("請先購票")
elif age < 12:
    print("年齡不足")
elif height < 140:
    print("身高不足")
elif has_heart_disease:
    print("心臟病患者不能搭乘")
elif is_pregnant:
    print("孕婦不能搭乘")
else:
    print("歡迎搭乘！")



print("=== 電影票價系統 ===")
day = input("今天星期幾？(weekday/weekend)：")
age = int(input("請輸入年齡："))
is_student = input("是否為學生？(y/n)：")

if day == "weekend":
    # 週末票價
    if age < 6:
        price = 0
        print("6 歲以下免費！")
    elif age >= 65:
        price = 150
        print("敬老票：150 元")
    else:
        price = 350
        print(f"週末全票：{price} 元")
else:
    # 平日票價
    if age < 6:
        price = 0
        print("6 歲以下免費！")
    elif age >= 65:
        price = 100
        print("敬老票：100 元")
    elif is_student == "y":
        price = 180
        print("平日學生票：180 元")
    else:
        price = 280
        print(f"平日全票：{price} 元")

print(f"\n最終票價：{price} 元")


# 執行: python3 第 19 課：巢狀條件 — if 裡面還有 if1.py

# === 預期輸出 ===
# 你有票嗎？(y/n)：
# ⚠️ 本檔需要互動輸入（input()），以上為未輸入時的輸出；請自行執行並輸入資料。
