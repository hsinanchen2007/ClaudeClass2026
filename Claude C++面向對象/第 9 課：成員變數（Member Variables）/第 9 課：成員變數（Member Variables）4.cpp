#include <iostream>
using namespace std;

class Matrix2x2 {
public:
    double data[2][2];  // 固定大小的陣列作為成員

    void set(double a, double b, double c, double d) {
        data[0][0] = a;  data[0][1] = b;
        data[1][0] = c;  data[1][1] = d;
    }

    void print() {
        cout << "| " << data[0][0] << "  " << data[0][1] << " |" << endl;
        cout << "| " << data[1][0] << "  " << data[1][1] << " |" << endl;
    }

    double determinant() {
        return data[0][0] * data[1][1] - data[0][1] * data[1][0];
    }
};

int main() {
    Matrix2x2 m;
    m.set(3, 7, 1, 5);

    m.print();
    cout << "行列式 = " << m.determinant() << endl;

    return 0;
}
