#include <iostream>
using namespace std;

// ✅ 適合用 struct —— 純資料集合
struct Point {
    double x = 0;
    double y = 0;
};

struct Color {
    int r = 0;
    int g = 0;
    int b = 0;
    int a = 255;
};

struct Config {
    int width = 800;
    int height = 600;
    bool fullscreen = false;
    int fps = 60;
};

struct DateInfo {
    int year = 2025;
    int month = 1;
    int day = 1;
};

int main() {
    Point p;
    p.x = 3.5;
    p.y = 7.2;
    cout << "Point(" << p.x << ", " << p.y << ")" << endl;

    Color red;
    red.r = 255;
    cout << "Color(" << red.r << ", " << red.g << ", " << red.b << ")" << endl;

    Config cfg;
    cout << cfg.width << "x" << cfg.height
         << (cfg.fullscreen ? " 全螢幕" : " 視窗") << endl;

    return 0;
}
