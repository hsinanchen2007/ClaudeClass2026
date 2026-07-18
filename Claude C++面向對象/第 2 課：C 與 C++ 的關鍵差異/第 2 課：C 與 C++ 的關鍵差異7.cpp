#include <stdio.h>
#include <stdbool.h>  // C99 需要這個標頭檔

int main() {
    bool flag = true;   // 需要 stdbool.h
    int old_style = 1;  // C89 常用 int 代替
    
    if (flag) {
        printf("flag is true\n");
    }
    
    return 0;
}
