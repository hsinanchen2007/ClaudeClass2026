# Python 排序範例, sort() 方法會直接修改原本的串列，並且沒有回傳值。
nums = [38, 15, 72, 4, 56]
print(f"排序前：{nums}")      # [38, 15, 72, 4, 56]

nums.sort()
print(f"升冪排序：{nums}")    # [4, 15, 38, 56, 72]

nums.sort(reverse=True)
print(f"降冪排序：{nums}")    # [72, 56, 38, 15, 4]


# 英文：按字母順序
words = ["banana", "apple", "cherry", "date"]
words.sort()
print(words)    # ['apple', 'banana', 'cherry', 'date']

# 中文：按 Unicode 編碼，不是筆畫或注音
names = ["小華", "小明", "小美", "小強"]
names.sort()
print(names)    # 依 Unicode 排序，結果可能不直覺


# sorted() 函式會回傳一個新的串列，原本的串列不受影響。
nums = [38, 15, 72, 4, 56]

new_sorted = sorted(nums)
print(f"新串列：{new_sorted}")   # [4, 15, 38, 56, 72]
print(f"原串列：{nums}")         # [38, 15, 72, 4, 56] ← 不變！

# 也支援 reverse 參數
desc = sorted(nums, reverse=True)
print(f"降冪：{desc}")           # [72, 56, 38, 15, 4]


# ❌ 常見錯誤：把 sort() 的回傳值拿去用
nums = [3, 1, 2]
result = nums.sort()
print(result)    # None（sort 沒有回傳值！）

# ✅ 正確做法
nums = [3, 1, 2]
nums.sort()          # 原地排序，不需要接回傳值
print(nums)          # [1, 2, 3]

# 或用 sorted()
nums = [3, 1, 2]
result = sorted(nums)   # 回傳新串列
print(result)            # [1, 2, 3]


# sort() 和 sorted() 都接受一個 key 參數，可以指定一個函式來決定排序的依據。
# 依照字串長度排序
fruits = ["banana", "kiwi", "apple", "fig", "cherry"]
fruits.sort(key=len)
print(fruits)   # ['fig', 'kiwi', 'apple', 'banana', 'cherry']

# 依照絕對值排序
numbers = [-5, 3, -1, 8, -3]
numbers.sort(key=abs)
print(numbers)  # [-1, 3, -3, -5, 8]


# 實用範例：學生依成績排序
students = [
    ("小明", 85),
    ("小華", 92),
    ("小美", 78),
    ("小強", 96)
]

# 依照成績（第二個元素）排序
students.sort(key=lambda x: x[1], reverse=True)
for name, score in students:
    print(f"{name}：{score} 分")

# 輸出：
# 小強：96 分
# 小華：92 分
# 小明：85 分
# 小美：78 分


# reverse() 原地反轉
nums = [1, 2, 3, 4, 5]
nums.reverse()
print(nums)      # [5, 4, 3, 2, 1]


# reversed() 回傳反轉迭代器（原串列不變）
nums = [1, 2, 3, 4, 5]

# 這一行程式碼將 nums 串列中的所有元素以相反順序排列，並存入新的變數 rev。
# 
# 運作方式是由內而外執行的。首先，reversed(nums) 會回傳一個反轉迭代器（reverse iterator）——這是一個惰性物件，
# 它會從 nums 的最後一個元素開始，逐一往前產出元素，但本身並不會在記憶體中建立一個新串列。
# 單獨使用這個迭代器的話，你無法透過索引來存取元素，也沒辦法直接 print 出可讀的內容。
# 
# 因此，外層的 list() 建構子會將這個迭代器包起來，把它產出的所有元素收集成一個完整的新串列。
# 在整個過程中，原本的 nums 串列完全不會被修改——這是一種**非破壞性（non-mutating）**的反轉方式。
# 這跟 nums.reverse() 有很重要的區別：reverse() 是原地反轉，會直接修改 nums 本身，而且回傳值是 None。
# 如果你需要保留原始串列不變，同時又想得到一份反轉後的副本，list(reversed(nums)) 就是最慣用的寫法。
# 
# 另一個等效的替代方案是切片語法 nums[::-1]，同樣能產生一個新的反轉串列而不影響原本的資料。
rev = list(reversed(nums))
# rev = reversed(nums)

print(rev)       # [5, 4, 3, 2, 1]
print(nums)      # [1, 2, 3, 4, 5] ← 不變

# 切片語法反轉串列
nums = [1, 2, 3, 4, 5]
rev = nums[::-1]
print(rev)       # [5, 4, 3, 2, 1]
print(nums)      # [1, 2, 3, 4, 5] ← 不變


# index() 方法會回傳指定元素在串列中第一次出現的位置（索引）。如果元素不存在，則會引發 ValueError。
fruits = ["蘋果", "香蕉", "芒果", "香蕉", "葡萄"]

print(fruits.index("芒果"))     # 輸出：2
print(fruits.index("香蕉"))     # 輸出：1（回傳第一個出現的位置）


# 如果要找第二個出現的位置，可以指定起始索引：
# index(值, 起始索引, 結束索引)
print(fruits.index("香蕉", 2))       # 輸出：3（從索引 2 開始找）
print(fruits.index("香蕉", 2, 5))    # 輸出：3（從索引 2 到 4 之間找）


# ❌ 錯誤做法：直接搜尋可能會引發例外
# fruits.index("草莓")   # ❌ ValueError: '草莓' is not in list

# ✅ 安全做法：先檢查再搜尋
target = "草莓"
if target in fruits:
    pos = fruits.index(target)
    print(f"{target} 在索引 {pos}")
else:
    print(f"找不到 {target}")


# count() 方法會回傳指定元素在串列中出現的次數。如果元素不存在，則回傳 0。
nums = [1, 3, 5, 3, 7, 3, 9]
print(nums.count(3))     # 輸出：3（3 出現了 3 次）
print(nums.count(5))     # 輸出：1
print(nums.count(100))   # 輸出：0（不存在就回傳 0，不會報錯）


# 實用範例：統計選票
votes = ["A", "B", "A", "C", "A", "B", "A", "C", "B", "A"]

for candidate in ["A", "B", "C"]:
    c = votes.count(candidate)
    bar = "█" * c
    print(f"候選人 {candidate}：{c} 票 {bar}")

# 輸出：
# 候選人 A：5 票 █████
# 候選人 B：3 票 ███
# 候選人 C：2 票 ██


# sum() 函式會回傳串列中所有元素的總和。len() 函式會回傳串列中元素的數量。兩者結合可以計算平均值。
nums = [23, 45, 12, 67, 34, 89, 56]
avg = sum(nums) / len(nums)
print(f"平均值：{avg:.1f}")   # 輸出：平均值：46.6


# max() 函式會回傳串列中最大值，min() 函式會回傳串列中最小值。對於字串來說，則是按照字母順序比較。
words = ["banana", "apple", "cherry"]
print(max(words))    # cherry（字母順序最大）
print(min(words))    # apple（字母順序最小）


# 串列推導式（List Comprehension）是一種簡潔的語法，可以用來從一個可迭代物件（如串列、字串、範圍等）快速生成新的串列。
# 
# 基本語法如下：
# [表達式 for 變數 in 可迭代物件 if 條件]
# 
# 其中，表達式是你想要產生的新元素的形式，變數是用來迭代可迭代物件的元素，條件則是可選的，用來過濾元素。
# 這種語法讓你可以在一行程式碼中完成複雜的串列生成邏輯，通常比傳統的 for 迴圈更簡潔易讀。 
# 
# 以下是一些使用串列推導式的範例：    
# 傳統寫法：建立 1~5 的平方
squares = []
for n in range(1, 6):
    squares.append(n ** 2)
print(squares)   # [1, 4, 9, 16, 25]

# 推導式寫法：一行搞定
squares = [n ** 2 for n in range(1, 6)]
print(squares)   # [1, 4, 9, 16, 25]


# 把字串串列全部轉大寫
fruits = ["apple", "banana", "cherry"]
upper_fruits = [f.upper() for f in fruits]
print(upper_fruits)   # ['APPLE', 'BANANA', 'CHERRY']


# 串列推導式（List Comprehension）帶條件的推導式：只保留偶數的平方
# 這裡的 if 條件會過濾掉所有不符合條件的元素，只有 n 是偶數的情況下，n ** 2 才會被加入到新的串列中。
# 這種寫法讓你可以在一行程式碼中同時完成元素的生成和過濾，通常比傳統的 for 迴圈更簡潔易讀。
# 例如，以下程式碼會從 1 到 10 中篩選出偶數，並將它們的平方存入新的串列 evens：
# 傳統寫法：篩選偶數
nums = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
evens = []
for n in nums:
    if n % 2 == 0:
        evens.append(n)
print(evens)   # [2, 4, 6, 8, 10]

# 推導式寫法
evens = [n for n in nums if n % 2 == 0]
print(evens)   # [2, 4, 6, 8, 10]


# 這裡的 if 條件會過濾掉所有不符合條件的元素，只有 s 的值大於或等於 60 的情況下，s + 5 才會被加入到新的串列 boosted 中。
# 篩選及格成績（>= 60）並加 5 分獎勵
scores = [45, 72, 88, 55, 91, 63, 38]
boosted = [s + 5 for s in scores if s >= 60]
print(boosted)   # [77, 93, 96, 68]


# 需求：從名單中挑出名字長度 >= 3 的人，並轉成大寫
names = ["Al", "Bob", "Catherine", "Di", "Edward"]

# --- 傳統寫法 ---
result = []
for name in names:
    if len(name) >= 3:
        result.append(name.upper())
print(result)   # ['BOB', 'CATHERINE', 'EDWARD']

# --- 推導式 ---
result = [name.upper() for name in names if len(name) >= 3]
print(result)   # ['BOB', 'CATHERINE', 'EDWARD']


# join() 方法會將串列中的元素以指定的分隔符連接成一個新的字串。分隔符是 join() 方法前面的字串。
words = ["Hello", "World", "Python"]

result = " ".join(words)
print(result)           # Hello World Python

result2 = "-".join(words)
print(result2)          # Hello-World-Python

result3 = "".join(words)
print(result3)          # HelloWorldPython


# join() 的元素必須是字串，如果有非字串元素會引發 TypeError。
# 如果元素不是字串，要先轉換
nums = [1, 2, 3, 4, 5]
# "-".join(nums)              # ❌ TypeError
result = "-".join(str(n) for n in nums)
print(result)                 # 1-2-3-4-5


# split() 方法會將字串按照指定的分隔符切割成一個串列。預設的分隔符是空白（空格、換行、制表符等）。
sentence = "Python 是 很棒的 程式語言"
words = sentence.split()
print(words)   # ['Python', '是', '很棒的', '程式語言']

data = "85,92,78,90,88"
scores = data.split(",")
print(scores)  # ['85', '92', '78', '90', '88']

# 搭配推導式轉成整數
scores_int = [int(s) for s in data.split(",")]
print(scores_int)   # [85, 92, 78, 90, 88]


mixed = [3, "hello", 1]
# mixed.sort()   # ❌ TypeError: '<' not supported between instances of 'str' and 'int'
# Python 3 不允許不同型別之間直接比較大小


# ❌ 沒有方括號 → 這會變成「生成器」，不是串列
# result = n ** 2 for n in range(5)   # SyntaxError

# ✅ 要加方括號
result = [n ** 2 for n in range(5)]


words = ["Hello", "World"]

# ❌ 錯誤：join 是字串的方法，不是串列的
# words.join(" ")

# ✅ 正確：在分隔符字串上呼叫 join
" ".join(words)


# === 文字統計分析器 ===

text = "Python is a great programming language and Python is easy to learn"

# 1. 拆成單字串列
words = text.lower().split()
print(f"原文（小寫）拆解：{words}")
print(f"總單字數：{len(words)}")

# 2. 找出不重複的單字
unique = []
for w in words:
    if w not in unique:
        unique.append(w)
print(f"\n不重複單字（{len(unique)} 個）：{unique}")

# 3. 統計每個單字出現次數
print("\n【單字頻率統計】")
# 依出現次數降冪排序
unique_sorted = sorted(unique, key=lambda w: words.count(w), reverse=True)
for w in unique_sorted:
    c = words.count(w)
    bar = "█" * c
    print(f"  {w:15s} {c} 次 {bar}")

# 4. 篩選出長度 >= 5 的單字
long_words = [w for w in unique if len(w) >= 5]
print(f"\n長度 >= 5 的單字：{long_words}")

# 5. 把所有單字首字母大寫後合併
title_case = " ".join([w.capitalize() for w in words])
print(f"\n標題格式：{title_case}")





