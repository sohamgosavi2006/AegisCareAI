#ifndef MODERNCARD_H
#define MODERNCARD_H

#include <QFrame>

class ModernCard : public QFrame {
    Q_OBJECT
public:
    explicit ModernCard(QWidget* parent = nullptr);
    virtual ~ModernCard();

    void setHoverEffect(bool enable);

protected:
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    bool m_hoverEffect = true;
};

#endif // MODERNCARD_H
