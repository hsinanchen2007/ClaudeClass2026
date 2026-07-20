# ============================================================
# 第 27 課：串列 List（三）— 常用方法與切片 — 重點總結
# ============================================================

# ═══════════════════════════════════════════════════════════════════════════
# 【面試題】排序、切片與拷貝
# ───────────────────────────────────────────────────────────────────────────
# 🔥 Q1. 淺拷貝與深拷貝差在哪？
#     答：淺拷貝（`copy.copy()`／`L[:]`／`L.copy()`／`list(L)`）
#     只複製**最外層**，內層元素仍**共用參考**；
#     深拷貝（`copy.deepcopy()`）**遞迴**複製所有層級。
#     實測 `o=[[1,2],[3,4]]` 改 `o[0]` 後：淺拷貝跟著變、深拷貝不受影響；
#     `s[0] is o[0]` → True、`d[0] is o[0]` → False。
#     追問：deepcopy 遇到循環參考會無限遞迴嗎？（不會，內部用 memo dict 記錄）
#
# 🔥 Q2. 排序時怎麼指定規則？
#     答：用 `key` 參數指定「拿什麼去比」，例如
#     `sorted(words, key=len)`、`sorted(data, key=lambda x: x[1])`；
#     `reverse=True` 則反轉順序。key 對每個元素只計算一次，
#     比舊式的 `cmp` 有效率得多。
#     追問：要先按 A 升冪、再按 B 降冪怎麼寫？
#     （利用排序**穩定**的特性分兩次排，或用 `key=lambda x: (x.a, -x.b)`）
#
# ⚠️ 陷阱. `sorted(['10', '9', '2'])` 的結果是什麼？
#     答：實測 **`['10', '2', '9']`**——字串是按**字典序**逐字元比較，
#     `'1' < '2' < '9'`，跟數值大小無關。要按數值排必須 `sorted(x, key=int)`。
#     為什麼會錯：看到內容是數字就以為會按數值排。
#     延伸：混型別排序在 Python 3 直接拋
#     `TypeError: '<' not supported between instances of 'str' and 'int'`。
# ═══════════════════════════════════════════════════════════════════════════


# ============================================================
# 【重點 1】排序 — sort() 與 sorted()
# sort()  → 原地排序（修改原串列，回傳 None）
# sorted() → 回傳新串列（原串列不變）
# ============================================================

# --- sort()：原地排序 ---
nums = [38, 15, 72, 4, 56]
nums.sort()                          # 升冪（小到大）
print(nums)                          # [4, 15, 38, 56, 72]

nums.sort(reverse=True)              # 降冪（大到小）
print(nums)                          # [72, 56, 38, 15, 4]

# --- sorted()：回傳新串列，原串列不變 ---
nums = [38, 15, 72, 4, 56]
new_sorted = sorted(nums)
print(new_sorted)                    # [4, 15, 38, 56, 72]
print(nums)                          # [38, 15, 72, 4, 56] ← 不變！

desc = sorted(nums, reverse=True)    # 降冪
print(desc)                          # [72, 56, 38, 15, 4]

# --- 英文按字母順序排序 ---
words = ["banana", "apple", "cherry"]
words.sort()
print(words)                         # ['apple', 'banana', 'cherry']

# ⚠️ 中文按 Unicode 編碼排序，結果可能不直覺

# --- ❌ sort() 回傳 None，不能接回傳值！ ---
nums = [3, 1, 2]
result = nums.sort()
print(result)                        # None（不是 [1,2,3]！）

# ✅ 要用 sorted() 接回傳值
result = sorted([3, 1, 2])
print(result)                        # [1, 2, 3]

# ============================================================
# 【重點 2】★ key 參數 — 自訂排序依據
# sort() 和 sorted() 都支援 key 參數。
# ============================================================

# --- 依字串長度排序 ---
fruits = ["banana", "kiwi", "apple", "fig", "cherry"]
fruits.sort(key=len)
print(fruits)                        # ['fig', 'kiwi', 'apple', 'banana', 'cherry']

# --- 依絕對值排序 ---
numbers = [-5, 3, -1, 8, -3]
numbers.sort(key=abs)
print(numbers)                       # [-1, 3, -3, -5, 8]

# --- ★ 用 lambda 排序元組串列 ---
students = [
    ("小明", 85),
    ("小華", 92),
    ("小美", 78),
    ("小強", 96)
]

# 依成績（第二個元素）降冪排序
students.sort(key=lambda x: x[1], reverse=True)
for name, score in students:
    print(f"{name}：{score} 分")
# 小強：96、小華：92、小明：85、小美：78

# ============================================================
# 【重點 3】反轉 — reverse()、reversed()、[::-1]
# ============================================================

# --- reverse()：原地反轉（修改原串列，回傳 None） ---
nums = [1, 2, 3, 4, 5]
nums.reverse()
print(nums)                          # [5, 4, 3, 2, 1]

# --- reversed()：回傳反轉迭代器（原串列不變） ---
nums = [1, 2, 3, 4, 5]
rev = list(reversed(nums))           # 需要 list() 轉換
print(rev)                           # [5, 4, 3, 2, 1]
print(nums)                          # [1, 2, 3, 4, 5] ← 不變

# --- [::-1]：切片反轉（最簡潔，回傳新串列） ---
nums = [1, 2, 3, 4, 5]
rev = nums[::-1]
print(rev)                           # [5, 4, 3, 2, 1]
print(nums)                          # [1, 2, 3, 4, 5] ← 不變

# ★ 三種反轉比較：
# nums.reverse()      → 原地修改，回傳 None
# list(reversed(nums)) → 回傳新串列，原串列不變
# nums[::-1]           → 回傳新串列，原串列不變（最簡潔）

# ============================================================
# 【重點 4】搜尋 — index() 與 count()
# ============================================================

# --- index(value)：回傳第一次出現的索引 ---
fruits = ["蘋果", "香蕉", "芒果", "香蕉", "葡萄"]
print(fruits.index("芒果"))          # 2
print(fruits.index("香蕉"))          # 1（第一個出現的位置）

# --- index 可指定搜尋範圍 ---
print(fruits.index("香蕉", 2))      # 3（從索引 2 開始找）
print(fruits.index("香蕉", 2, 5))   # 3（從索引 2 到 4 之間找）

# ⚠️ 元素不存在 → ValueError
# fruits.index("草莓")              # ❌ ValueError

# ✅ 安全做法：先檢查再搜尋
target = "草莓"
if target in fruits:
    pos = fruits.index(target)
    print(f"{target} 在索引 {pos}")
else:
    print(f"找不到 {target}")

# --- count(value)：回傳出現次數 ---
nums = [1, 3, 5, 3, 7, 3, 9]
print(nums.count(3))                # 3（出現 3 次）
print(nums.count(100))              # 0（不存在回傳 0，不會報錯）

# --- 實用：統計選票 ---
votes = ["A", "B", "A", "C", "A", "B"]
for candidate in ["A", "B", "C"]:
    print(f"候選人 {candidate}：{votes.count(candidate)} 票")

# ============================================================
# 【重點 5】統計函數 — sum()、max()、min()、len()
# ============================================================

nums = [23, 45, 12, 67, 34, 89, 56]
print(sum(nums))                     # 326（總和）
print(max(nums))                     # 89（最大值）
print(min(nums))                     # 12（最小值）
print(len(nums))                     # 7（長度）

avg = sum(nums) / len(nums)
print(f"平均值：{avg:.1f}")          # 46.6

# max/min 也適用於字串（按字母順序）
words = ["banana", "apple", "cherry"]
print(max(words))                    # cherry
print(min(words))                    # apple

# ============================================================
# 【重點 6】★★★ 串列推導式（List Comprehension）
# 語法：[表達式 for 變數 in 可迭代物件 if 條件]
# ============================================================

# --- 基本推導式 ---
# 傳統寫法
squares = []
for n in range(1, 6):
    squares.append(n ** 2)
# 推導式（一行搞定）
squares = [n ** 2 for n in range(1, 6)]
print(squares)                       # [1, 4, 9, 16, 25]

# --- 字串轉大寫 ---
fruits = ["apple", "banana", "cherry"]
upper_fruits = [f.upper() for f in fruits]
print(upper_fruits)                  # ['APPLE', 'BANANA', 'CHERRY']

# --- 帶條件的推導式（篩選） ---
nums = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
evens = [n for n in nums if n % 2 == 0]
print(evens)                         # [2, 4, 6, 8, 10]

# --- 同時篩選 + 轉換 ---
scores = [45, 72, 88, 55, 91, 63, 38]
boosted = [s + 5 for s in scores if s >= 60]
print(boosted)                       # [77, 93, 96, 68]

# --- 篩選長名字並轉大寫 ---
names = ["Al", "Bob", "Catherine", "Di", "Edward"]
result = [name.upper() for name in names if len(name) >= 3]
print(result)                        # ['BOB', 'CATHERINE', 'EDWARD']

# ============================================================
# 【重點 7】字串與串列互轉 — join() 與 split()
# ============================================================

# --- join()：串列 → 字串（用分隔符連接） ---
# ★ join 是字串的方法，不是串列的！
words = ["Hello", "World", "Python"]
print(" ".join(words))               # Hello World Python
print("-".join(words))               # Hello-World-Python
print("".join(words))                # HelloWorldPython

# ⚠️ join 的元素必須是字串
nums = [1, 2, 3, 4, 5]
# "-".join(nums)                     # ❌ TypeError
result = "-".join(str(n) for n in nums)
print(result)                        # 1-2-3-4-5

# --- split()：字串 → 串列（按分隔符切割） ---
sentence = "Python 是 很棒的 程式語言"
words = sentence.split()             # 預設按空白分割
print(words)                         # ['Python', '是', '很棒的', '程式語言']

data = "85,92,78,90,88"
scores = data.split(",")             # 按逗號分割
print(scores)                        # ['85', '92', '78', '90', '88']

# ★ split + 推導式 → 字串轉數字串列
scores_int = [int(s) for s in data.split(",")]
print(scores_int)                    # [85, 92, 78, 90, 88]

# ============================================================
# 【重點 8】⚠️ 常見錯誤
# ============================================================

# --- ❌ sort() 回傳 None ---
nums = [3, 1, 2]
result = nums.sort()
print(result)                        # None
# ✅ 用 sorted() 或直接 sort() 不接回傳值

# --- ❌ 混合型別排序 ---
mixed = [3, "hello", 1]
# mixed.sort()                       # ❌ TypeError: '<' not supported

# --- ❌ 推導式忘記方括號 ---
# result = n ** 2 for n in range(5)  # ❌ SyntaxError
result = [n ** 2 for n in range(5)]  # ✅ 要加 []

# --- ❌ join 的呼叫方式搞反 ---
words = ["Hello", "World"]
# words.join(" ")                    # ❌ 這不對
" ".join(words)                      # ✅ 在分隔符上呼叫 join

# ============================================================
# 小結：
# 1. sort() 原地排序（回傳 None），sorted() 回傳新串列
# 2. ★ key 參數可自訂排序依據：key=len、key=abs、key=lambda
# 3. 反轉三種：reverse()（原地）、reversed()（迭代器）、[::-1]（新串列）
# 4. index() 找位置（不存在 → ValueError），count() 計次數（不存在 → 0）
# 5. sum/max/min/len 常用統計函數
# 6. ★★★ 串列推導式：[表達式 for x in 可迭代 if 條件]
# 7. join() 串列→字串，split() 字串→串列
# 8. ⚠️ sort()/reverse() 回傳 None，不能接回傳值
# 9. ⚠️ 混合型別不能排序，join 元素必須是字串
# ============================================================


# 執行: python3 summary.py

# === 預期輸出 (節錄) ===
# [4, 15, 38, 56, 72]
# [72, 56, 38, 15, 4]
# [4, 15, 38, 56, 72]
# [38, 15, 72, 4, 56]
# [72, 56, 38, 15, 4]
# ['apple', 'banana', 'cherry']
# None
# [1, 2, 3]
# ['fig', 'kiwi', 'apple', 'banana', 'cherry']
# [-1, 3, -3, -5, 8]
# 小強：96 分
# 小華：92 分
# 小明：85 分
# 小美：78 分
# [5, 4, 3, 2, 1]
# [5, 4, 3, 2, 1]
# [1, 2, 3, 4, 5]
# [5, 4, 3, 2, 1]
# [1, 2, 3, 4, 5]
# 2
# …（後略，完整輸出共 49 行）
