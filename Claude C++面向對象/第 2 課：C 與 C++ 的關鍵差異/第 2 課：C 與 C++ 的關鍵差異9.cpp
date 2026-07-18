#include <stdio.h>

// C 語言中，每個函數必須有不同的名稱
int add_int(int a, int b) {
    return a + b;
}

double add_double(double a, double b) {
    return a + b;
}

int main() {
    printf("%d\n", add_int(3, 5));
    printf("%f\n", add_double(3.14, 2.86));
    return 0;
}
