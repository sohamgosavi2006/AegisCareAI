#ifndef DIGITALTWINWIDGET_H
#define DIGITALTWINWIDGET_H

#include <QWidget>
#include <QMap>
#include <QPainterPath>
#include <QTimer>
#include <QVariantAnimation>

class DigitalTwinWidget : public QWidget {
    Q_OBJECT
    Q_PROPERTY(double bodyRotation READ bodyRotation WRITE setBodyRotation)
    Q_PROPERTY(double zoomFactor READ zoomFactor WRITE setZoomFactor)
    Q_PROPERTY(QPointF zoomCenter READ zoomCenter WRITE setZoomCenter)

public:
    enum ViewMode {
        Front,
        Side,
        Back
    };

    struct Region {
        QString name;
        QPainterPath frontPath;
        QPainterPath sidePath;
        QPainterPath backPath;
        QString status = "Normal"; // "Normal", "Review", "Warning", "Critical"
        QString notes;
    };

    explicit DigitalTwinWidget(QWidget* parent = nullptr);
    virtual ~DigitalTwinWidget();

    void setRegionStatus(const QString& regionName, const QString& status, const QString& notes = "");
    QString getRegionStatus(const QString& regionName) const;
    void setViewMode(ViewMode mode);
    ViewMode viewMode() const { return m_viewMode; }

    void triggerRotation();
    void zoomToRegion(const QString& regionName);
    void resetZoom();

    // Animation accessors
    double bodyRotation() const { return m_bodyRotation; }
    void setBodyRotation(double angle);
    double zoomFactor() const { return m_zoomFactor; }
    void setZoomFactor(double factor);
    QPointF zoomCenter() const { return m_zoomCenter; }
    void setZoomCenter(const QPointF& center);

signals:
    void regionClicked(const QString& regionName);
    void regionHovered(const QString& regionName);
    void viewModeChanged(ViewMode mode);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private:
    ViewMode m_viewMode = Front;
    QMap<QString, Region> m_regions;
    QString m_hoveredRegion;
    QString m_selectedRegion;

    // Animation values
    double m_bodyRotation = 0.0; // 0 to 180 degrees
    double m_zoomFactor = 1.0;
    QPointF m_zoomCenter;
    
    QTimer* m_glowTimer = nullptr;
    double m_glowAlpha = 0.0;
    bool m_glowIncreasing = true;

    QVariantAnimation* m_rotateAnim = nullptr;
    QVariantAnimation* m_zoomAnim = nullptr;
    QVariantAnimation* m_zoomCenterAnim = nullptr;

    void initRegions();
    QPainterPath getCurrentPath(const Region& reg) const;
    QColor getStatusColor(const QString& status, int alpha = 255) const;
    void drawGrid(QPainter& painter);
    void drawHumanOutline(QPainter& painter);
    void drawOrgans(QPainter& painter);
    void drawHUD(QPainter& painter);
};

#endif // DIGITALTWINWIDGET_H
