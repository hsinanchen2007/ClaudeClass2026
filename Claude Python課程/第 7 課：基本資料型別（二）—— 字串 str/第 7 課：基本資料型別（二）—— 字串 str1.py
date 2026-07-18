# 字串（str）是用來表示文字的資料類型，可以包含字母、數字、符號和空格。以下是一些字串的例子：
name = '小明'
message = 'Hello World'

print(name)
print(message)

# 在 Python 中，字串可以使用單引號（'）或雙引號（"）來定義。這兩種方式是等效的，可以根據需要選擇使用。例如：
name = "小明"
message = "Hello World"

print(name)
print(message)

# 如果字串中包含單引號，可以使用雙引號來定義字串，反之亦然。例如：
# 字串裡面有單引號 → 外面用雙引號
sentence = "It's a beautiful day"
print(sentence)    # It's a beautiful day

# 字串裡面有雙引號 → 外面用單引號
sentence = '他說："你好"'
print(sentence)    # 他說："你好"

# """三重引號（''' 或 """）可以用來定義多行字串，這在需要包含換行符或長文本時非常有用。例如：
poem = """靜夜思
床前明月光，
疑是地上霜。
舉頭望明月，
低頭思故鄉。"""

print(poem)

# 字串是不可變的（immutable），這意味著一旦創建了字串，就不能修改其中的內容。如果需要修改字串，可以創建一個新的字串。例如：
#name = "Hello"
#name[0] = "h"    # ❌ 錯誤！不能直接修改字串中的字元

# 如果需要修改字串，可以創建一個新的字串，例如：
name = "Hello"  
new_name = "h" + name[1:]  # 創建一個新的字串，將第一個字元改為小寫
print(new_name)  # hello

# len() 函數可以用來獲取字串的長度，即字串中包含的字元數量。例如：
name = "Hello"
print(len(name))    # 5

chinese = "你好"
print(len(chinese))  # 2

empty = ""
print(len(empty))    # 0

with_space = "Hello World"
print(len(with_space))  # 11（空格也算一個字元）

# 字串可以使用索引來訪問其中的字元，索引從 0 開始。例如：
text = "Hello"

print(text[0])     # H（第一個字元）
print(text[1])     # e（第二個字元）
print(text[4])     # o（第五個字元）

# 也可以使用負數索引從字串的末尾開始訪問字元，-1 表示最後一個字元，-2 表示倒數第二個字元，以此類推。例如：
text = "Hello"

print(text[-1])    # o（最後一個）
print(text[-2])    # l（倒數第二個）
print(text[-5])    # H（倒數第五個 = 第一個）

# 如果索引超出字串的範圍，會引發 IndexError 錯誤。例如：
#text = "Hello"
#print(text[10])    # ❌ 錯誤！

# 字串切片（slicing）可以用來獲取字串的一部分，使用冒號（:）來指定起始和結束索引。例如：
text = "Hello World"

print(text[0:5])     # Hello（索引 0 到 4）
print(text[6:11])    # World（索引 6 到 10）

# 切片的結束索引是非包含性的（exclusive），這意味著切片會包含起始索引的字元，但不包含結束索引的字元。例如：
text = "Hello World"
 
# 省略開始：從頭開始
print(text[:5])      # Hello（索引 0 到 4）

# 省略結束：到最後
print(text[6:])      # World（索引 6 到最後）

# 兩個都省略：整個字串
print(text[:])       # Hello World

# 你也可以使用負數索引來切片字串。例如：
text = "Hello World"

print(text[-5:])     # World（最後 5 個字元）
print(text[:-6])     # Hello（去掉最後 6 個字元）
print(text[-5:-2])   # Wor（倒數第 5 到倒數第 3）

# 步長（step）：可以指定切片的步長，默認為 1。例如：
text = "Hello World"

print(text[0:10:2])  # HloWr（每隔 2 個取一個）
print(text[::2])     # HloWrd（整個字串，每隔 2 個）
print(text[::3])     # HlWl（每隔 3 個）

# 反向切片：步長也可以是負數，這樣可以反向切片字串。例如：
text = "Hello"

print(text[::-1])    # olleH

# 字串連接（concatenation）可以使用加號（+）來將兩個字串連接在一起。例如：
first_name = "小"
last_name = "明"

full_name = last_name + first_name
print(full_name)    # 明小... 等等，應該是姓在前！

full_name = first_name + last_name
print(full_name)    # 小明... 還是不對，讓我重新想

last_name = "陳"      # 姓
first_name = "小明"   # 名

full_name = last_name + first_name
print(full_name)    # 陳小明

# 你也可以使用加號（+）來連接字串和其他類型的資料，但需要先將其他類型的資料轉換為字串。例如：
greeting = "Hello"
name = "World"

message = greeting + " " + name + "!"
print(message)    # Hello World!

# 字串可以使用乘號（*）來重複多次。例如：
line = "-" * 20
print(line)    # --------------------

pattern = "ha" * 5
print(pattern)  # hahahahaha

# 製作分隔線
print("=" * 30)
print("      歡迎使用本系統      ")
print("=" * 30)

# 當你嘗試將字串與其他類型的資料直接連接時，會引發 TypeError 錯誤。例如：
#age = 18
#message = "我今年" + age + "歲"    # ❌ 錯誤！

# 你需要使用 str() 函數將其他類型的資料轉換為字串。例如：
age = 18
message = "我今年" + str(age) + "歲"
print(message)    # 我今年18歲

# 你也可以使用 f-string（格式化字串）來更方便地連接字串和其他類型的資料。例如：
# 數字轉字串
num = 123
text = str(num)
print(text)         # "123"
print(type(text))   # <class 'str'>

# 字串轉數字
text = "456"
num = int(text)
print(num)          # 456
print(type(num))    # <class 'int'>

# 小數字串轉浮點數
text = "3.14"
num = float(text)
print(num)          # 3.14
print(type(num))    # <class 'float'>

# 使用換行符（\n）來換行
message = "第一行\n第二行\n第三行"
print(message)

# 使用制表符（\t）來對齊
print("姓名\t年齡\t城市")
print("小明\t18\t台北")
print("小華\t20\t高雄")

# 反斜線（\）是 Python 中的轉義字符，用於表示特殊字符或在字串中插入特殊符號。例如：
path = "C:\\Users\\Documents\\file.txt"
print(path)

# 如果你需要在字串中使用反斜線，可以使用雙反斜線（\\）來表示一個反斜線。例如：
# 在單引號字串中使用單引號
message = 'It\'s a beautiful day'
print(message)    # It's a beautiful day

# 在雙引號字串中使用雙引號
message = "他說：\"你好\""
print(message)    # 他說："你好"

# 你也可以使用原始字串（raw string）來避免轉義字符的影響，原始字串以 r 開頭。例如：
# 一般字串
path = "C:\new\test"
print(path)
# 結果會出問題，因為 \n 被當成換行

# 原始字串
path = r"C:\new\test"
print(path)
# 結果：C:\new\test

# 字串中可以使用 in 運算符來檢查某個子字串是否存在於字串中。例如：
sentence = "Hello World"

print("Hello" in sentence)     # True
print("World" in sentence)     # True
print("Python" in sentence)    # False
print("hello" in sentence)     # False（大小寫有差！）

# 你也可以使用 not in 運算符來檢查某個子字串是否不存在於字串中。例如：
sentence = "Hello World"

print("Python" not in sentence)    # True
print("Hello" not in sentence)     # False

# 你可以使用 if 語句來根據字串的內容執行不同的程式碼。例如：
email = "user@example.com"

if "@" in email:
    print("這看起來是一個電子郵件")
else:
    print("這不是有效的電子郵件")


# 字串大小寫轉換, upper()、lower()、title() 等方法可以用來轉換字串的大小寫。例如：
text = "Hello World"

print(text.upper())    # HELLO WORLD（全大寫）
print(text.lower())    # hello world（全小寫）
print(text.title())    # Hello World（每個單字首字大寫）

# 字串去除空白, strip()、lstrip()、rstrip() 等方法可以用來去除字串兩端或一端的空白。例如：
text = "   Hello World   "

print(text.strip())    # "Hello World"（去除頭尾空白）
print(text.lstrip())   # "Hello World   "（去除左邊空白）
print(text.rstrip())   # "   Hello World"（去除右邊空白）

# 字串替換, replace() 方法可以用來替換字串中的子字串。例如：
text = "Hello World"

print(text.replace("World", "Python"))    # Hello Python
print(text.replace("l", "L"))             # HeLLo WorLd

# 字串搜尋, find() 方法可以用來搜尋字串中的子字串，返回子字串的起始索引，如果找不到則返回 -1。例如：
text = "Hello World"

print(text.find("World"))    # 6（找到，回傳索引）
print(text.find("Python"))   # -1（找不到，回傳 -1）

# 字串計數, count() 方法可以用來計算字串中某個子字串出現的次數。例如：
text = "Hello World"

print(text.count("l"))       # 3（"l" 出現 3 次）
print(text.count("o"))       # 2（"o" 出現 2 次）

# 字串分割, split() 方法可以用來將字串分割成多個子字串，返回一個列表。例如：
# 建立字串
message = "Python Programming"

# 取得長度
print("字串長度：", len(message))

# 取得第一個和最後一個字元
print("第一個字元：", message[0])
print("最後一個字元：", message[-1])

# 切片
print("前 6 個字元：", message[:6])
print("後 11 個字元：", message[7:])

# 反轉
print("反轉：", message[::-1])

# 字串連接
name = "陳小明"
age = 20
city = "台北"
hobby = "寫程式"

# 建立分隔線
line = "=" * 24

# 印出資料卡
print(line)
print("     個 人 資 料 卡     ")
print(line)
print("姓名：" + name)
print("年齡：" + str(age) + " 歲")
print("城市：" + city)
print("興趣：" + hobby)
print(line)

# 字串格式化, f-string（格式化字串）可以用來更方便地插入變數到字串中。例如：
text = "The quick brown fox jumps over the lazy dog"

print("原始字串：", text)
print("-" * 40)

# 基本資訊
print("字串長度：", len(text))
print("全部大寫：", text.upper())
print("全部小寫：", text.lower())

# 字元統計
print("-" * 40)
print("字母 'o' 出現次數：", text.count("o"))
print("字母 'e' 出現次數：", text.count("e"))
print("單字 'the' 出現次數：", text.lower().count("the"))

# 檢查內容
print("-" * 40)
print("是否包含 'fox'：", "fox" in text)
print("是否包含 'cat'：", "cat" in text)

# 字串遮罩, 可以使用切片和字串連接來遮罩字串中的部分內容。例如：
password = "mySecretPassword123"

# 只顯示前兩個和後兩個字元，其他用 * 代替
length = len(password)
masked = password[:2] + "*" * (length - 4) + password[-2:]

print("原始密碼：", password)
print("遮罩後：", masked)

