#pragma once

#include <QObject>
#include <QPointer>
#include <QString>
#include <QtWaylandClient/QWaylandClientExtensionTemplate>

#include <memory>

#include <qwayland-input-method-unstable-v1.h>

class WaylandInputMethodContext;

class WaylandInputMethod : public QWaylandClientExtensionTemplate<WaylandInputMethod>, public QtWayland::zwp_input_method_v1
{
    Q_OBJECT
    Q_PROPERTY(bool contextActive READ contextActive NOTIFY contextActiveChanged)
    Q_PROPERTY(bool protocolReady READ protocolReady NOTIFY protocolReadyChanged)

public:
    explicit WaylandInputMethod(QObject *parent = nullptr);
    ~WaylandInputMethod() override;

    bool contextActive() const;
    bool protocolReady() const;

    void commitText(const QString &text);
    void deleteSurroundingText(int beforeLength, uint afterLength);
    void sendKeysym(uint keysym);

signals:
    void contextActiveChanged(bool active);
    void protocolReadyChanged(bool ready);
    void contextUpdated();
    void contextInvoked();

private:
    void zwp_input_method_v1_activate(struct ::zwp_input_method_context_v1 *id) override;
    void zwp_input_method_v1_deactivate(struct ::zwp_input_method_context_v1 *context) override;
    void setContext(WaylandInputMethodContext *context);

    std::unique_ptr<WaylandInputMethodContext> m_context;
};

class WaylandInputMethodContext : public QObject, public QtWayland::zwp_input_method_context_v1
{
    Q_OBJECT

public:
    explicit WaylandInputMethodContext(struct ::zwp_input_method_context_v1 *id, QObject *parent = nullptr);
    ~WaylandInputMethodContext() override;

    void commitText(const QString &text);
    void deleteSurroundingText(int beforeLength, uint afterLength);
    void sendKeysym(uint keysym);

signals:
    void updated();
    void invoked();

private:
    void zwp_input_method_context_v1_commit_state(uint serial) override;
    void zwp_input_method_context_v1_content_type(uint hint, uint purpose) override;
    void zwp_input_method_context_v1_invoke_action(uint button, uint index) override;
    void zwp_input_method_context_v1_preferred_language(const QString &language) override;
    void zwp_input_method_context_v1_reset() override;
    void zwp_input_method_context_v1_surrounding_text(const QString &text, uint cursor, uint anchor) override;

    uint m_serial = 0;
};
