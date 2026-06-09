#include "WaylandInputMethod.h"

#include <QDateTime>

namespace
{
constexpr uint KeyStateReleased = 0;
constexpr uint KeyStatePressed = 1;
}

WaylandInputMethod::WaylandInputMethod(QObject *parent)
    : QWaylandClientExtensionTemplate<WaylandInputMethod>(1)
{
    setParent(parent);
    connect(this, &QWaylandClientExtensionTemplate<WaylandInputMethod>::activeChanged, this, [this]() {
        emit protocolReadyChanged(protocolReady());
    });
}

WaylandInputMethod::~WaylandInputMethod() = default;

bool WaylandInputMethod::contextActive() const
{
    return m_context != nullptr;
}

bool WaylandInputMethod::protocolReady() const
{
    return isActive();
}

void WaylandInputMethod::commitText(const QString &text)
{
    if (m_context) {
        m_context->commitText(text);
    }
}

void WaylandInputMethod::deleteSurroundingText(int beforeLength, uint afterLength)
{
    if (m_context) {
        m_context->deleteSurroundingText(beforeLength, afterLength);
    }
}

void WaylandInputMethod::sendKeysym(uint keysym)
{
    if (m_context) {
        m_context->sendKeysym(keysym);
    }
}

void WaylandInputMethod::zwp_input_method_v1_activate(struct ::zwp_input_method_context_v1 *id)
{
    auto *context = new WaylandInputMethodContext(id, this);
    connect(context, &WaylandInputMethodContext::updated, this, &WaylandInputMethod::contextUpdated);
    setContext(context);
}

void WaylandInputMethod::zwp_input_method_v1_deactivate(struct ::zwp_input_method_context_v1 *context)
{
    if (m_context && m_context->object() == context) {
        setContext(nullptr);
    }
}

void WaylandInputMethod::setContext(WaylandInputMethodContext *context)
{
    const bool wasActive = contextActive();
    m_context.reset(context);
    const bool isNowActive = contextActive();
    if (wasActive != isNowActive) {
        emit contextActiveChanged(isNowActive);
    }
}

WaylandInputMethodContext::WaylandInputMethodContext(struct ::zwp_input_method_context_v1 *id, QObject *parent)
    : QObject(parent)
    , QtWayland::zwp_input_method_context_v1(id)
{
}

WaylandInputMethodContext::~WaylandInputMethodContext() = default;

void WaylandInputMethodContext::commitText(const QString &text)
{
    commit_string(m_serial, text);
}

void WaylandInputMethodContext::deleteSurroundingText(int beforeLength, uint afterLength)
{
    if (beforeLength > 0) {
        delete_surrounding_text(-beforeLength, beforeLength);
    }
    if (afterLength > 0) {
        delete_surrounding_text(0, afterLength);
    }
    commit_string(m_serial, QString());
}

void WaylandInputMethodContext::sendKeysym(uint symbol)
{
    const uint time = static_cast<uint>(QDateTime::currentMSecsSinceEpoch() & 0xffffffff);
    keysym(m_serial, time, symbol, KeyStatePressed, 0);
    keysym(m_serial, time, symbol, KeyStateReleased, 0);
}

void WaylandInputMethodContext::zwp_input_method_context_v1_commit_state(uint serial)
{
    m_serial = serial;
    emit updated();
}

void WaylandInputMethodContext::zwp_input_method_context_v1_content_type(uint hint, uint purpose)
{
    Q_UNUSED(hint)
    Q_UNUSED(purpose)
    emit updated();
}

void WaylandInputMethodContext::zwp_input_method_context_v1_invoke_action(uint button, uint index)
{
    Q_UNUSED(button)
    Q_UNUSED(index)
    emit updated();
}

void WaylandInputMethodContext::zwp_input_method_context_v1_preferred_language(const QString &language)
{
    Q_UNUSED(language)
}

void WaylandInputMethodContext::zwp_input_method_context_v1_reset()
{
    emit updated();
}

void WaylandInputMethodContext::zwp_input_method_context_v1_surrounding_text(const QString &text, uint cursor, uint anchor)
{
    Q_UNUSED(text)
    Q_UNUSED(cursor)
    Q_UNUSED(anchor)
    emit updated();
}
