struct Flags {
    unsigned int readable : 1;
    unsigned int writable : 1;
    unsigned int executable : 1;
};

template<typename T>
void wrapper(T&& arg) {
    // ...
}

int main() {
    Flags f = {1, 1, 0};

    // wrapper(f.readable);
    // 錯誤！位元欄位不能取址，不能綁定到非 const 參考
    // T&& 推導為 unsigned int&，需要位址 → 失敗

    // 解決方法：先複製到普通變數
    unsigned int r = f.readable;
    wrapper(r);              // OK
    wrapper(+f.readable);    // OK：一元 + 運算子產生 prvalue

    return 0;
}
