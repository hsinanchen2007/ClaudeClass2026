#include <stdio.h>

struct Point {
    int x;
    int y;
};

int main() {
    // C 語言中，宣告變數需要 struct 關鍵字
    struct Point p1;
    p1.x = 10;
    p1.y = 20;
    
    // 或者使用 typedef
    // typedef struct Point Point;
    
    printf("(%d, %d)\n", p1.x, p1.y);
    return 0;
}
