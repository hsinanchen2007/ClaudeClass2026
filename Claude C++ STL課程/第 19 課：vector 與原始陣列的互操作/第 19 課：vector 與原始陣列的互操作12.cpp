#include <iostream>
#include <fstream>
#include <vector>

int main() {
    // ===== 寫入二進位檔 =====
    {
        std::vector<int> data = {100, 200, 300, 400, 500};

        std::ofstream ofs("data.bin", std::ios::binary);
        if (ofs) {
            // 直接把 vector 的底層記憶體寫入檔案
            ofs.write(reinterpret_cast<const char*>(data.data()),
                      data.size() * sizeof(int));
            std::cout << "寫入 " << data.size() << " 個 int 到 data.bin" << std::endl;
        }
    }

    // ===== 讀取二進位檔 =====
    {
        std::ifstream ifs("data.bin", std::ios::binary);
        if (ifs) {
            // 先取得檔案大小
            ifs.seekg(0, std::ios::end);
            size_t file_size = ifs.tellg();
            ifs.seekg(0, std::ios::beg);

            // 計算元素數量
            size_t count = file_size / sizeof(int);

            // 配置 vector 並讀入
            std::vector<int> data(count);
            ifs.read(reinterpret_cast<char*>(data.data()),
                     count * sizeof(int));

            std::cout << "從 data.bin 讀取：";
            for (int x : data) std::cout << x << " ";
            std::cout << std::endl;
        }
    }

    return 0;
}
