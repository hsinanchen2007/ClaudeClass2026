#include <stdio.h>
#include <stdlib.h>

int main() {
    // 配置單一變數
    int* p = (int*)malloc(sizeof(int));
    *p = 42;
    printf("值: %d\n", *p);
    free(p);
    
    // 配置陣列
    int* arr = (int*)malloc(5 * sizeof(int));
    for (int i = 0; i < 5; i++) {
        arr[i] = i * 10;
    }
    free(arr);
    
    return 0;
}
