#include <cstdint>
#include <memory>

#include <fcitx-utils/dbus/bus.h>
#include <fcitx/addonfactory.h>
#include <fcitx/addonmanager.h>
#include <fcitx/event.h>
#include <fcitx/inputcontext.h>
#include <fcitx/instance.h>

namespace
{
constexpr auto Service = "org.kde.KdeOsk";
constexpr auto ObjectPath = "/Keyboard";
constexpr auto Interface = "org.kde.KdeOsk.Keyboard";
constexpr auto FocusRectMethod = "setTextFocusRect";
}

class KdeOskBridge final : public fcitx::AddonInstance
{
public:
    explicit KdeOskBridge(fcitx::Instance *instance)
        : instance_(instance)
        , bus_(fcitx::dbus::BusType::Session)
    {
        if (bus_.isOpen()) {
            bus_.attachEventLoop(&instance_->eventLoop());
        }

        focusInWatcher_ = instance_->watchEvent(
            fcitx::EventType::InputContextFocusIn,
            fcitx::EventWatcherPhase::Default,
            [this](fcitx::Event &event) { handleInputContextEvent(event, true); });
        focusOutWatcher_ = instance_->watchEvent(
            fcitx::EventType::InputContextFocusOut,
            fcitx::EventWatcherPhase::Default,
            [this](fcitx::Event &event) { handleInputContextEvent(event, false); });
        cursorRectWatcher_ = instance_->watchEvent(
            fcitx::EventType::InputContextCursorRectChanged,
            fcitx::EventWatcherPhase::Default,
            [this](fcitx::Event &event) { handleCursorRectChanged(event); });
    }

    ~KdeOskBridge() override
    {
        if (bus_.isOpen()) {
            bus_.detachEventLoop();
        }
    }

private:
    void handleInputContextEvent(fcitx::Event &event, bool active)
    {
        auto &inputContextEvent = static_cast<fcitx::InputContextEvent &>(event);
        sendFocusRect(inputContextEvent.inputContext(), active);
    }

    void handleCursorRectChanged(fcitx::Event &event)
    {
        auto &inputContextEvent = static_cast<fcitx::InputContextEvent &>(event);
        fcitx::InputContext *inputContext = inputContextEvent.inputContext();
        if (inputContext != nullptr && inputContext->hasFocus()) {
            sendFocusRect(inputContext, true);
        }
    }

    void sendFocusRect(fcitx::InputContext *inputContext, bool active)
    {
        if (!bus_.isOpen()) {
            return;
        }

        int32_t x = 0;
        int32_t y = 0;
        int32_t width = 0;
        int32_t height = 0;
        if (active && inputContext != nullptr) {
            const fcitx::Rect &rect = inputContext->cursorRect();
            x = rect.left();
            y = rect.top();
            width = rect.width();
            height = rect.height();
        }

        auto message = bus_.createMethodCall(Service, ObjectPath, Interface, FocusRectMethod);
        message << active << x << y << width << height;
        message.send();
    }

    fcitx::Instance *instance_ = nullptr;
    fcitx::dbus::Bus bus_;
    std::unique_ptr<fcitx::HandlerTableEntry<fcitx::EventHandler>> focusInWatcher_;
    std::unique_ptr<fcitx::HandlerTableEntry<fcitx::EventHandler>> focusOutWatcher_;
    std::unique_ptr<fcitx::HandlerTableEntry<fcitx::EventHandler>> cursorRectWatcher_;
};

class KdeOskBridgeFactory final : public fcitx::AddonFactory
{
public:
    fcitx::AddonInstance *create(fcitx::AddonManager *manager) override
    {
        return new KdeOskBridge(manager->instance());
    }
};

FCITX_ADDON_FACTORY(KdeOskBridgeFactory)
