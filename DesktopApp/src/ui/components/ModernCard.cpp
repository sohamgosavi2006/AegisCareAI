#include "ModernCard.h"
#include <QEvent>

ModernCard::ModernCard(QWidget* parent) : QFrame(parent) {
    setObjectName("ModernCard");
    setFrameShape(QFrame::StyledPanel);
    setFrameShadow(QFrame::Plain);

    // Native child-window composition on macOS can leave stale rectangular
    // backing-store images when many nested graphics effects are scrolled or
    // switched in a QStackedWidget. Cards use borders/contrast instead; this
    // is visually consistent and avoids the expensive off-screen surfaces.
}

ModernCard::~ModernCard() {}

void ModernCard::setHoverEffect(bool enable) {
    m_hoverEffect = enable;
}

void ModernCard::enterEvent(QEnterEvent* event) {
    if (m_hoverEffect) setProperty("hovered", true);
    QFrame::enterEvent(event);
}

void ModernCard::leaveEvent(QEvent* event) {
    if (m_hoverEffect) setProperty("hovered", false);
    QFrame::leaveEvent(event);
}
