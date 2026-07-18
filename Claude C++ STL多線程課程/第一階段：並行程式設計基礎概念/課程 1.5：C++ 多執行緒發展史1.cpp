// C++11 之前的 POSIX 多執行緒寫法
#include <pthread.h>
#include <stdio.h>

void* threadFunction(void* arg) {
    int* value = (int*)arg;
    printf("執行緒收到值: %d\n", *value);
    return NULL;
}

int main() {
    pthread_t thread;
    int value = 42;
    
    // 建立執行緒
    pthread_create(&thread, NULL, threadFunction, &value);
    
    // 等待執行緒結束
    pthread_join(thread, NULL);
    
    return 0;
}
