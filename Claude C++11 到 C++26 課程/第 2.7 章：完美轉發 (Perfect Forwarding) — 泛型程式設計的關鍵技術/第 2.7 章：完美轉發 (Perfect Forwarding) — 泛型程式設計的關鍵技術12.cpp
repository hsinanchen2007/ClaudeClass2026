#include <iostream>
#include <string>
#include <functional>
#include <unordered_map>
#include <vector>
#include <utility>
#include <memory>
#include <typeindex>

// ===== 事件基類 =====
struct Event {
    virtual ~Event() = default;
    virtual std::string name() const = 0;
};

// ===== 具體事件型別 =====
struct ClickEvent : Event {
    int x, y;
    std::string button;

    // 完美轉發建構子
    template<typename S>
    ClickEvent(int x, int y, S&& btn)
        : x(x), y(y), button(std::forward<S>(btn)) {}

    std::string name() const override { return "ClickEvent"; }
};

struct MessageEvent : Event {
    std::string sender;
    std::string content;

    template<typename S1, typename S2>
    MessageEvent(S1&& s, S2&& c)
        : sender(std::forward<S1>(s))
        , content(std::forward<S2>(c)) {}

    std::string name() const override { return "MessageEvent"; }
};

// ===== 事件匯流排 =====
class EventBus {
    using Handler = std::function<void(const Event&)>;
    std::unordered_map<std::type_index, std::vector<Handler>> handlers_;

public:
    // 註冊處理器
    template<typename EventType, typename Func>
    void on(Func&& handler) {
        handlers_[typeid(EventType)].push_back(
            [h = std::forward<Func>(handler)](const Event& e) {
                h(static_cast<const EventType&>(e));
            }
        );
    }

    // 發射事件：完美轉發引數，直接在內部建構事件
    template<typename EventType, typename... Args>
    void emit(Args&&... args) {
        // 完美轉發到事件的建構子
        EventType event(std::forward<Args>(args)...);

        std::cout << "發射 " << event.name() << "\n";

        auto it = handlers_.find(typeid(EventType));
        if (it != handlers_.end()) {
            for (auto& handler : it->second) {
                handler(event);
            }
        }
    }
};

int main() {
    EventBus bus;

    // 註冊處理器
    bus.on<ClickEvent>([](const ClickEvent& e) {
        std::cout << "  處理點擊: (" << e.x << "," << e.y
                  << ") button=" << e.button << "\n";
    });

    bus.on<MessageEvent>([](const MessageEvent& e) {
        std::cout << "  處理訊息: [" << e.sender << "] " << e.content << "\n";
    });

    // 發射事件——引數直接完美轉發到建構子
    std::cout << "--- 右值引數 ---\n";
    bus.emit<ClickEvent>(100, 200, std::string("left"));
    bus.emit<MessageEvent>(std::string("Alice"), std::string("Hello!"));

    std::cout << "\n--- 左值引數 ---\n";
    std::string button = "right";
    std::string sender = "Bob";
    std::string content = "World!";
    bus.emit<ClickEvent>(50, 75, button);       // 複製 button
    bus.emit<MessageEvent>(sender, content);     // 複製 sender 和 content

    std::cout << "\n--- 混合引數 ---\n";
    bus.emit<MessageEvent>(sender, std::string("Goodbye!"));  // 複製 sender，移動 content

    // 驗證左值沒被破壞
    std::cout << "\n--- 驗證 ---\n";
    std::cout << "button  = \"" << button  << "\"\n";
    std::cout << "sender  = \"" << sender  << "\"\n";
    std::cout << "content = \"" << content << "\"\n";

    return 0;
}
