# OOP：先掌握值語意，再使用多型

物件導向不是「所有東西都做成 class hierarchy」。現代 C++ 更常先設計可複製、可移動、維持不變式的 value type；只有當呼叫端必須透過共同介面操作不同 runtime 型別時，才引入 virtual polymorphism。

## 封裝的目的

private 不是為了隱藏而隱藏，而是讓物件能保證不變式。若餘額不能為負、尺寸必須大於零，就不要讓外部任意寫欄位。

## Rule of Zero

若成員都由標準容器、string、smart pointer 等 RAII 型別管理，通常不需要自行定義 destructor/copy/move。這是最可靠的預設。

## Runtime polymorphism

- base destructor 若可能透過 base pointer 刪除 derived object，必須是 virtual。
- ownership 應由 unique_ptr/shared_ptr 明確表示。
- virtual function 建議加 <code>override</code>，讓簽章錯誤在編譯期被抓到。
- clone 可提供多型物件的深複製，但成本與所有權要清楚。

## Composition 優先於 inheritance

「有一個」通常用 composition；「是一種」且需要 substitutability 才考慮 public inheritance。若 derived 破壞 base contract，即使語法允許也不是良好繼承。

## 範例索引

本章共 29 個可獨立編譯執行的 `.cpp`；`summary.cpp` 是面試前速查。

<details>
<summary>展開全部檔案</summary>

- [`10_ConstMember.cpp`](10_ConstMember.cpp)
- [`11_StaticMember.cpp`](11_StaticMember.cpp)
- [`12_Friend.cpp`](12_Friend.cpp)
- [`13_OperatorOverloading.cpp`](13_OperatorOverloading.cpp)
- [`14_Inheritance.cpp`](14_Inheritance.cpp)
- [`15_AccessAndInheritance.cpp`](15_AccessAndInheritance.cpp)
- [`16_VirtualFunction.cpp`](16_VirtualFunction.cpp)
- [`17_AbstractClass.cpp`](17_AbstractClass.cpp)
- [`18_VirtualDestructor.cpp`](18_VirtualDestructor.cpp)
- [`19_RAII.cpp`](19_RAII.cpp)
- [`1_ClassAndObject.cpp`](1_ClassAndObject.cpp)
- [`20_UniquePtr.cpp`](20_UniquePtr.cpp)
- [`21_SharedPtr.cpp`](21_SharedPtr.cpp)
- [`22_MoveSemantics.cpp`](22_MoveSemantics.cpp)
- [`23_RuleOfThreeFiveZero.cpp`](23_RuleOfThreeFiveZero.cpp)
- [`24_ClassTemplate.cpp`](24_ClassTemplate.cpp)
- [`25_StlWithCustomClass.cpp`](25_StlWithCustomClass.cpp)
- [`26_Singleton.cpp`](26_Singleton.cpp)
- [`27_Factory.cpp`](27_Factory.cpp)
- [`28_LRUCache.cpp`](28_LRUCache.cpp)
- [`2_MemberAndMethod.cpp`](2_MemberAndMethod.cpp)
- [`3_Encapsulation.cpp`](3_Encapsulation.cpp)
- [`4_Constructor.cpp`](4_Constructor.cpp)
- [`5_Destructor.cpp`](5_Destructor.cpp)
- [`6_ThisPointer.cpp`](6_ThisPointer.cpp)
- [`7_InitializerList.cpp`](7_InitializerList.cpp)
- [`8_CopyConstructor.cpp`](8_CopyConstructor.cpp)
- [`9_AssignmentOperator.cpp`](9_AssignmentOperator.cpp)
- [`summary.cpp`](summary.cpp)

</details>

## 練習

1. 為 Money 加入同幣別加法，跨幣別時拒絕。
2. 在 Shape hierarchy 新增 Triangle，確認既有迴圈不用修改。
3. 比較 composition 範例改成 inheritance 後，介面是否反而變得不合理。
