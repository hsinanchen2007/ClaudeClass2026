class DetachingThread {
    std::thread t;
    
public:
    template<typename Func, typename... Args>
    explicit DetachingThread(Func&& f, Args&&... args)
        : t(std::forward<Func>(f), std::forward<Args>(args)...) {}
    
    DetachingThread(DetachingThread&&) = default;
    DetachingThread& operator=(DetachingThread&&) = default;
    
    DetachingThread(const DetachingThread&) = delete;
    DetachingThread& operator=(const DetachingThread&) = delete;
    
    ~DetachingThread() {
        if (t.joinable()) {
            t.detach();
        }
    }
};
