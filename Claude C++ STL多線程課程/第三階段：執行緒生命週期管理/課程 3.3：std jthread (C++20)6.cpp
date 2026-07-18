class jthread {
public:
    // 建構
    jthread() noexcept;
    
    template<typename F, typename... Args>
    explicit jthread(F&& f, Args&&... args);
    
    // 解構：自動 request_stop() + join()
    ~jthread();
    
    // 移動（不可複製）
    jthread(jthread&&) noexcept;
    jthread& operator=(jthread&&) noexcept;
    
    // 停止機制
    stop_source get_stop_source() noexcept;
    stop_token get_stop_token() const noexcept;
    bool request_stop() noexcept;
    
    // 與 std::thread 相同的介面
    bool joinable() const noexcept;
    void join();
    void detach();
    id get_id() const noexcept;
    native_handle_type native_handle();
    
    static unsigned int hardware_concurrency() noexcept;
};
