#include <stdio.h>

int main() {
    int age;
    char name[50];
    
    printf("請輸入你的名字: ");
    scanf("%s", name);
    
    printf("請輸入你的年齡: ");
    scanf("%d", &age);
    
    printf("你好 %s，你今年 %d 歲\n", name, age);
    
    return 0;
}
