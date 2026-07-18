#include <stdio.h>

int main() {
    /* C89 要求變數必須在區塊開頭宣告 */
    int i;
    int sum;
    
    sum = 0;
    for (i = 0; i < 10; i++) {
        sum += i;
    }
    
    printf("Sum = %d\n", sum);
    return 0;
}
