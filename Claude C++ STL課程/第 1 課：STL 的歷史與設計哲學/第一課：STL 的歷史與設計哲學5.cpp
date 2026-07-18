#include <stdio.h>
#include <stdlib.h>

// 比較函數（給 qsort 用）
int compare(const void* a, const void* b) {
    return (*(int*)a - *(int*)b);
}

int main() {
    int* numbers = NULL;
    int capacity = 4;
    int size = 0;
    
    // 動態配置
    numbers = (int*)malloc(capacity * sizeof(int));
    if (numbers == NULL) {
        fprintf(stderr, "記憶體配置失敗\n");
        return 1;
    }
    
    // 模擬輸入
    int inputs[] = {5, 2, 8, 1, 9, 3, 7, 4, 6};
    int input_count = sizeof(inputs) / sizeof(inputs[0]);
    
    for (int i = 0; i < input_count; ++i) {
        // 需要擴展嗎？
        if (size >= capacity) {
            capacity *= 2;
            int* temp = (int*)realloc(numbers, capacity * sizeof(int));
            if (temp == NULL) {
                fprintf(stderr, "記憶體重新配置失敗\n");
                free(numbers);
                return 1;
            }
            numbers = temp;
        }
        numbers[size++] = inputs[i];
    }
    
    // 排序
    qsort(numbers, size, sizeof(int), compare);
    
    // 找最大值、計算總和
    int max = numbers[0];
    int sum = 0;
    for (int i = 0; i < size; ++i) {
        if (numbers[i] > max) max = numbers[i];
        sum += numbers[i];
    }
    
    printf("排序後: ");
    for (int i = 0; i < size; ++i) {
        printf("%d ", numbers[i]);
    }
    printf("\n");
    printf("最大值: %d\n", max);
    printf("總和: %d\n", sum);
    
    // 別忘了釋放記憶體！
    free(numbers);
    
    return 0;
}
