#include "DigitalTwinWidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QDebug>
#include <QShowEvent>
#include <QHideEvent>
#include <cmath>

DigitalTwinWidget::DigitalTwinWidget(QWidget* parent) : QWidget(parent) {
    setMouseTracking(true);
    setMinimumSize(320, 480);
    m_zoomCenter = QPointF(200.0, 300.0);

    // Glow timer for pulsing warnings
    m_glowTimer = new QTimer(this);
    connect(m_glowTimer, &QTimer::timeout, this, [this]() {
        if (m_glowIncreasing) {
            m_glowAlpha += 8.0;
            if (m_glowAlpha >= 200.0) m_glowIncreasing = false;
        } else {
            m_glowAlpha -= 8.0;
            if (m_glowAlpha <= 60.0) m_glowIncreasing = true;
        }
        update();
    });
    m_glowTimer->start(40);

    // Persistent, parent-owned animations prevent dangling member pointers.
    // They are retargeted for each interaction and never self-delete.
    m_rotateAnim = new QVariantAnimation(this);
    m_zoomAnim = new QVariantAnimation(this);
    m_zoomCenterAnim = new QVariantAnimation(this);
    connect(m_rotateAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant& value) {
        setBodyRotation(value.toDouble());
    });
    connect(m_zoomAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant& value) {
        setZoomFactor(value.toDouble());
    });
    connect(m_zoomCenterAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant& value) {
        setZoomCenter(value.toPointF());
    });

    initRegions();
}

DigitalTwinWidget::~DigitalTwinWidget() {
    m_glowTimer->stop();
    m_rotateAnim->stop();
    m_zoomAnim->stop();
    m_zoomCenterAnim->stop();
}

void DigitalTwinWidget::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    if (!m_glowTimer->isActive()) m_glowTimer->start(80);
}

void DigitalTwinWidget::hideEvent(QHideEvent* event) {
    m_glowTimer->stop();
    m_rotateAnim->pause();
    m_zoomAnim->pause();
    m_zoomCenterAnim->pause();
    QWidget::hideEvent(event);
}

void DigitalTwinWidget::setRegionStatus(const QString& regionName, const QString& status, const QString& notes) {
    if (m_regions.contains(regionName)) {
        m_regions[regionName].status = status;
        m_regions[regionName].notes = notes;
        update();
    }
}

QString DigitalTwinWidget::getRegionStatus(const QString& regionName) const {
    if (m_regions.contains(regionName)) {
        return m_regions[regionName].status;
    }
    return "Normal";
}

void DigitalTwinWidget::setViewMode(ViewMode mode) {
    m_viewMode = mode;
    emit viewModeChanged(mode);
    update();
}

void DigitalTwinWidget::setBodyRotation(double angle) {
    m_bodyRotation = angle;
    
    // Switch views at transition thresholds
    if (m_bodyRotation >= 45.0 && m_bodyRotation < 135.0) {
        m_viewMode = Side;
    } else if (m_bodyRotation >= 135.0) {
        m_viewMode = Back;
    } else {
        m_viewMode = Front;
    }
    
    update();
}

void DigitalTwinWidget::setZoomFactor(double factor) {
    m_zoomFactor = factor;
    update();
}

void DigitalTwinWidget::setZoomCenter(const QPointF& center) {
    m_zoomCenter = center;
    update();
}

void DigitalTwinWidget::triggerRotation() {
    m_rotateAnim->stop();
    m_rotateAnim->setDuration(1800);
    m_rotateAnim->setStartValue(m_bodyRotation);
    m_rotateAnim->setKeyValueAt(0.5, 180.0);
    m_rotateAnim->setEndValue(0.0);
    m_rotateAnim->setEasingCurve(QEasingCurve::InOutQuad);
    m_rotateAnim->start();
}

void DigitalTwinWidget::zoomToRegion(const QString& regionName) {
    if (!m_regions.contains(regionName)) return;
    
    m_selectedRegion = regionName;
    
    // Find visual center of paths
    QPainterPath path = getCurrentPath(m_regions[regionName]);
    QRectF bounds = path.boundingRect();
    QPointF center = bounds.center();

    m_zoomAnim->stop();
    m_zoomCenterAnim->stop();

    // Zoom scale animation
    m_zoomAnim->setDuration(600);
    m_zoomAnim->setStartValue(m_zoomFactor);
    m_zoomAnim->setEndValue(2.2);
    m_zoomAnim->setEasingCurve(QEasingCurve::OutCubic);
    // Zoom focus center coordinate animation
    m_zoomCenterAnim->setDuration(600);
    m_zoomCenterAnim->setStartValue(m_zoomCenter);
    m_zoomCenterAnim->setEndValue(center);
    m_zoomCenterAnim->setEasingCurve(QEasingCurve::OutCubic);
    m_zoomAnim->start();
    m_zoomCenterAnim->start();
    
    emit regionClicked(regionName);
}

void DigitalTwinWidget::resetZoom() {
    m_selectedRegion = "";
    
    m_zoomAnim->stop();
    m_zoomCenterAnim->stop();

    m_zoomAnim->setDuration(500);
    m_zoomAnim->setStartValue(m_zoomFactor);
    m_zoomAnim->setEndValue(1.0);
    m_zoomAnim->setEasingCurve(QEasingCurve::OutQuad);
    m_zoomCenterAnim->setDuration(500);
    m_zoomCenterAnim->setStartValue(m_zoomCenter);
    m_zoomCenterAnim->setEndValue(QPointF(200.0, 300.0));
    m_zoomCenterAnim->setEasingCurve(QEasingCurve::OutQuad);
    m_zoomAnim->start();
    m_zoomCenterAnim->start();
}

void DigitalTwinWidget::initRegions() {
    // Coordinate scale baseline: width = 400, height = 600
    
    // 1. Head
    Region head;
    head.name = "Head";
    head.frontPath.addEllipse(170, 45, 60, 70);
    head.sidePath.addEllipse(165, 45, 70, 70);
    head.backPath.addEllipse(170, 45, 60, 70);
    m_regions.insert("Head", head);

    // 2. Brain
    Region brain;
    brain.name = "Brain";
    brain.frontPath.addEllipse(180, 50, 40, 45);
    brain.sidePath.addEllipse(170, 50, 50, 45);
    brain.backPath.addEllipse(180, 50, 40, 45);
    m_regions.insert("Brain", brain);

    // 3. Eyes
    Region eyes;
    eyes.name = "Eyes";
    eyes.frontPath.addEllipse(182, 72, 10, 8);
    eyes.frontPath.addEllipse(208, 72, 10, 8);
    eyes.sidePath.addEllipse(205, 72, 10, 8);
    eyes.backPath = QPainterPath(); // Not visible from back
    m_regions.insert("Eyes", eyes);

    // 4. Neck
    Region neck;
    neck.name = "Neck";
    neck.frontPath.addRect(190, 115, 20, 20);
    neck.sidePath.addRect(190, 115, 20, 20);
    neck.backPath.addRect(190, 115, 20, 20);
    m_regions.insert("Neck", neck);

    // 5. Chest / Ribcage boundary
    Region chest;
    chest.name = "Chest";
    chest.frontPath.addRoundedRect(155, 135, 90, 110, 15, 15);
    chest.sidePath.addRoundedRect(165, 135, 70, 110, 15, 15);
    chest.backPath.addRoundedRect(155, 135, 90, 110, 15, 15);
    m_regions.insert("Chest", chest);

    // 6. Heart
    Region heart;
    heart.name = "Heart";
    heart.frontPath.addEllipse(182, 160, 26, 30);
    heart.sidePath.addEllipse(186, 160, 22, 30);
    heart.backPath.addEllipse(192, 160, 26, 30);
    m_regions.insert("Heart", heart);

    // 7. Lungs
    Region lungs;
    lungs.name = "Lungs";
    // Left lung
    lungs.frontPath.addRoundedRect(162, 150, 22, 70, 8, 8);
    // Right lung
    lungs.frontPath.addRoundedRect(216, 150, 22, 70, 8, 8);
    // Side profile
    lungs.sidePath.addRoundedRect(178, 150, 44, 70, 8, 8);
    // Back profile
    lungs.backPath.addRoundedRect(162, 150, 22, 70, 8, 8);
    lungs.backPath.addRoundedRect(216, 150, 22, 70, 8, 8);
    m_regions.insert("Lungs", lungs);

    // 8. Abdomen
    Region abd;
    abd.name = "Abdomen";
    abd.frontPath.addRoundedRect(160, 245, 80, 85, 10, 10);
    abd.sidePath.addRoundedRect(170, 245, 60, 85, 10, 10);
    abd.backPath.addRoundedRect(160, 245, 80, 85, 10, 10);
    m_regions.insert("Abdomen", abd);

    // 9. Liver
    Region liver;
    liver.name = "Liver";
    QPolygonF lp;
    lp << QPointF(170, 255) << QPointF(215, 250) << QPointF(210, 275) << QPointF(175, 278);
    liver.frontPath.addPolygon(lp);
    liver.sidePath.addEllipse(176, 252, 38, 25);
    liver.backPath.addPolygon(lp);
    m_regions.insert("Liver", liver);

    // 10. Kidney
    Region kidney;
    kidney.name = "Kidney";
    // Kidney visible mostly from back (left & right)
    kidney.frontPath.addEllipse(180, 280, 12, 18);
    kidney.frontPath.addEllipse(208, 280, 12, 18);
    
    kidney.sidePath.addEllipse(188, 280, 12, 18);
    
    kidney.backPath.addEllipse(176, 278, 14, 22);
    kidney.backPath.addEllipse(210, 278, 14, 22);
    m_regions.insert("Kidney", kidney);

    // 11. Hands
    Region hands;
    hands.name = "Hands";
    // Left arm path
    hands.frontPath.addRect(128, 138, 24, 180);
    hands.frontPath.addEllipse(125, 318, 28, 28);
    // Right arm path
    hands.frontPath.addRect(248, 138, 24, 180);
    hands.frontPath.addEllipse(247, 318, 28, 28);

    // Side profile arms overlap
    hands.sidePath.addRect(185, 138, 30, 180);
    hands.sidePath.addEllipse(185, 318, 32, 32);

    // Back profile (same as front but flipped)
    hands.backPath.addRect(128, 138, 24, 180);
    hands.backPath.addEllipse(125, 318, 28, 28);
    hands.backPath.addRect(248, 138, 24, 180);
    hands.backPath.addEllipse(247, 318, 28, 28);
    m_regions.insert("Hands", hands);

    // 12. Legs
    Region legs;
    legs.name = "Legs";
    // Left Leg
    legs.frontPath.addRect(162, 330, 32, 210);
    // Right Leg
    legs.frontPath.addRect(206, 330, 32, 210);

    // Side profile overlapping
    legs.sidePath.addRect(184, 330, 42, 210);

    // Back profile
    legs.backPath.addRect(162, 330, 32, 210);
    legs.backPath.addRect(206, 330, 32, 210);
    m_regions.insert("Legs", legs);

    // 13. Feet
    Region feet;
    feet.name = "Feet";
    feet.frontPath.addRoundedRect(154, 540, 40, 20, 5, 5);
    feet.frontPath.addRoundedRect(206, 540, 40, 20, 5, 5);
    
    feet.sidePath.addRoundedRect(175, 540, 60, 20, 5, 5);
    
    feet.backPath.addRoundedRect(154, 540, 40, 20, 5, 5);
    feet.backPath.addRoundedRect(206, 540, 40, 20, 5, 5);
    m_regions.insert("Feet", feet);
}

QPainterPath DigitalTwinWidget::getCurrentPath(const Region& reg) const {
    switch (m_viewMode) {
        case Front: return reg.frontPath;
        case Side:  return reg.sidePath;
        case Back:  return reg.backPath;
    }
    return reg.frontPath;
}

QColor DigitalTwinWidget::getStatusColor(const QString& status, int alpha) const {
    if (status == "Critical") return QColor(231, 76, 60, alpha);  // Red
    if (status == "Warning")  return QColor(241, 196, 15, alpha); // Yellow
    if (status == "Review")   return QColor(52, 152, 219, alpha); // Light Blue
    return QColor(46, 204, 113, alpha);                           // Green
}

void DigitalTwinWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Paint solid background
    painter.fillRect(rect(), QColor(10, 17, 24));

    drawGrid(painter);

    // Save state for viewport transforms (zoom and rotation)
    painter.save();
    
    // Move to center of widget
    painter.translate(width() / 2, height() / 2);
    
    // Scale body size to fit widget viewport
    double scaleX = static_cast<double>(width()) / 450.0;
    double scaleY = static_cast<double>(height()) / 650.0;
    double baseScale = qMin(scaleX, scaleY);
    painter.scale(baseScale * m_zoomFactor, baseScale * m_zoomFactor);

    // Apply rotation compression for 3D spin simulation
    if (m_bodyRotation > 0.0) {
        // Linear compression toward 90deg, expand to 180deg
        double angleRad = (m_bodyRotation * M_PI) / 180.0;
        double compressX = std::cos(angleRad);
        painter.scale(compressX, 1.0);
    }

    // Apply zoom center offset translation
    painter.translate(-m_zoomCenter.x(), -m_zoomCenter.y());

    // Render human wireframe
    drawHumanOutline(painter);

    // Render organs inside wireframe
    drawOrgans(painter);

    painter.restore();
    
    // HUD Stats overlay
    drawHUD(painter);
}

void DigitalTwinWidget::drawGrid(QPainter& painter) {
    painter.setPen(QPen(QColor(0, 168, 255, 12), 1));
    int step = 30;
    for (int x = 0; x < width(); x += step) {
        painter.drawLine(x, 0, x, height());
    }
    for (int y = 0; y < height(); y += step) {
        painter.drawLine(0, y, width(), y);
    }
    
    // Tech corners
    painter.setPen(QPen(QColor(0, 168, 255, 60), 2));
    int cs = 15;
    // Top Left
    painter.drawLine(10, 10, 10 + cs, 10);
    painter.drawLine(10, 10, 10, 10 + cs);
    // Top Right
    painter.drawLine(width() - 10, 10, width() - 10 - cs, 10);
    painter.drawLine(width() - 10, 10, width() - 10, 10 + cs);
    // Bottom Left
    painter.drawLine(10, height() - 10, 10 + cs, height() - 10);
    painter.drawLine(10, height() - 10, 10, height() - 10 - cs);
    // Bottom Right
    painter.drawLine(width() - 10, height() - 10, width() - 10 - cs, height() - 10);
    painter.drawLine(width() - 10, height() - 10, width() - 10, height() - 10 - cs);
}

void DigitalTwinWidget::drawHumanOutline(QPainter& painter) {
    // Draw mannequin frame skeleton lines (glowing dark cyan)
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(QColor(0, 168, 255, 40), 2, Qt::SolidLine));
    
    // Draw body segments
    painter.drawLine(200, 115, 200, 330); // Spine
    painter.drawLine(140, 138, 260, 138); // Shoulders
    painter.drawLine(160, 330, 240, 330); // Hips

    // Draw main body outline
    painter.setPen(QPen(QColor(0, 168, 255, 70), 2, Qt::SolidLine));
    
    // General silhouette path combining outline regions
    QPainterPath outline;
    outline.addEllipse(170, 45, 60, 70); // Head
    outline.addRect(190, 115, 20, 20); // Neck
    outline.addRoundedRect(148, 135, 104, 195, 20, 20); // Torso
    
    painter.drawPath(outline);
}

void DigitalTwinWidget::drawOrgans(QPainter& painter) {
    // Loop through organs and draw them
    for (auto it = m_regions.begin(); it != m_regions.end(); ++it) {
        Region reg = it.value();
        QPainterPath path = getCurrentPath(reg);
        if (path.isEmpty()) continue;

        bool isHovered = (reg.name == m_hoveredRegion);
        bool isSelected = (reg.name == m_selectedRegion);
        
        QColor baseColor = getStatusColor(reg.status);
        
        // Setup fill & border colors
        QColor fillColor;
        QColor strokeColor;

        if (reg.status == "Critical" || reg.status == "Warning") {
            // Pulse warning organs
            fillColor = getStatusColor(reg.status, static_cast<int>(m_glowAlpha * 0.3));
            strokeColor = getStatusColor(reg.status, static_cast<int>(m_glowAlpha * 0.9));
        } else {
            fillColor = baseColor;
            fillColor.setAlpha(isHovered ? 80 : 35);
            strokeColor = baseColor;
            strokeColor.setAlpha(isHovered ? 255 : 160);
        }

        if (isSelected) {
            strokeColor = QColor(0, 210, 255, 255);
            fillColor = QColor(0, 210, 255, 70);
        }

        // Draw Glow Shadow Under Organ
        if (isHovered || isSelected) {
            painter.setPen(QPen(QColor(0, 210, 255, 50), 6));
            painter.setBrush(Qt::NoBrush);
            painter.drawPath(path);
        }

        painter.setPen(QPen(strokeColor, isSelected ? 3 : 2));
        painter.setBrush(QBrush(fillColor));
        painter.drawPath(path);
    }
}

void DigitalTwinWidget::drawHUD(QPainter& painter) {
    painter.setPen(QColor(0, 168, 255, 200));
    painter.setFont(QFont("Monospace", 9, QFont::Bold));
    
    // Draw active view state
    QString viewStr = "VIEW: FRONT";
    if (m_viewMode == Side) viewStr = "VIEW: SIDE";
    else if (m_viewMode == Back) viewStr = "VIEW: BACK";
    
    painter.drawText(25, 30, "SYSTEM: DIGITAL_TWIN_HUD");
    painter.drawText(25, 45, viewStr);
    
    if (m_zoomFactor > 1.0) {
        painter.drawText(25, 60, QString("ZOOM: %1X").arg(m_zoomFactor, 0, 'f', 1));
    }
    
    if (!m_hoveredRegion.isEmpty()) {
        painter.setPen(QColor(0, 210, 255));
        painter.drawText(25, height() - 40, "TARGET ACTIVE: " + m_hoveredRegion.toUpper());
        painter.setPen(getStatusColor(getRegionStatus(m_hoveredRegion)));
        painter.drawText(25, height() - 25, "STATUS: " + getRegionStatus(m_hoveredRegion).toUpper());
    } else {
        painter.setPen(QColor(0, 168, 255, 120));
        painter.drawText(25, height() - 25, "TARGET READY: HOVER OR REGION CLICK");
    }
}

void DigitalTwinWidget::mouseMoveEvent(QMouseEvent* event) {
    // Transform coordinates back from drawing scale space to inspect hover matches
    double scaleX = static_cast<double>(width()) / 450.0;
    double scaleY = static_cast<double>(height()) / 650.0;
    double baseScale = qMin(scaleX, scaleY);
    
    // Apply inverse translation (mouse relative to widget center)
    double mx = (event->pos().x() - width() / 2.0) / (baseScale * m_zoomFactor) + m_zoomCenter.x();
    double my = (event->pos().y() - height() / 2.0) / (baseScale * m_zoomFactor) + m_zoomCenter.y();
    QPointF modelPos(mx, my);

    QString oldHover = m_hoveredRegion;
    m_hoveredRegion = "";

    for (auto it = m_regions.begin(); it != m_regions.end(); ++it) {
        QPainterPath path = getCurrentPath(it.value());
        if (!path.isEmpty() && path.contains(modelPos)) {
            m_hoveredRegion = it.key();
            break;
        }
    }

    if (m_hoveredRegion != oldHover) {
        if (!m_hoveredRegion.isEmpty()) {
            emit regionHovered(m_hoveredRegion);
        }
        update();
    }

    QWidget::mouseMoveEvent(event);
}

void DigitalTwinWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        if (!m_hoveredRegion.isEmpty()) {
            zoomToRegion(m_hoveredRegion);
        } else {
            resetZoom();
        }
    }
    QWidget::mousePressEvent(event);
}

void DigitalTwinWidget::leaveEvent(QEvent* event) {
    if (!m_hoveredRegion.isEmpty()) {
        m_hoveredRegion = "";
        update();
    }
    QWidget::leaveEvent(event);
}

void DigitalTwinWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    update();
}
