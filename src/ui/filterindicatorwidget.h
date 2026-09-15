#ifndef FILTERINDICATORWIDGET_H
#define FILTERINDICATORWIDGET_H

#include <QColor>
#include <QHash>
#include <QWidget>

// Compact filter indicator widget showing filter position,
// bandwidth shape, and shift position above a horizontal line
class FilterIndicatorWidget : public QWidget {
    Q_OBJECT

public:
    explicit FilterIndicatorWidget(QWidget *parent = nullptr);

    // Filter position (1, 2, or 3) - displayed as FIL1/FIL2/FIL3
    void setFilterPosition(int position);
    int filterPosition() const { return m_filterPosition; }

    // Bandwidth in Hz - controls shape morphing (triangle → trapezoid)
    void setBandwidth(int bandwidthHz);
    int bandwidth() const { return m_bandwidthHz; }

    // Shift position in decahertz (10 Hz units) - controls horizontal position
    // SSB: ~135 = 1350 Hz center, CW: ~50 = 500 Hz (pitch)
    void setShift(int shift);
    int shift() const { return m_shift; }

    // Mode affects shift center calculation (SSB vs CW)
    void setMode(const QString &mode);
    QString mode() const { return m_mode; }

    // Bandwidth range for normalization
    void setBandwidthRange(int minHz, int maxHz);

    // Shape color (for VFO A/B color coding)
    void setShapeColor(const QColor &fill, const QColor &outline);

    // Record the current mode's nominal (NORM) passband width in Hz, learned
    // when the operator presses NORM. The down-turned edge ticks then show
    // only when the live bandwidth returns to this learned width.
    void setNormBandwidth(int hz);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void drawBandwidthShape(QPainter &painter, int lineY, int lineWidth);
    // Mode's nominal (NORM) bandwidth in Hz, or 0 if unknown.
    int normBandwidthHz() const;
    // Down-turned yellow ticks at the shape's base edges when at NORM.
    void drawFilterBaseline(QPainter &painter, float leftX, float rightX, float lineY);

    int m_filterPosition = 2;
    int m_bandwidthHz = 2400;    // Current bandwidth in Hz
    int m_shift = 135;           // Shift in decahertz
    QString m_mode = "USB";      // Mode for shift center calculation
    int m_minBandwidthHz = 50;   // Minimum bandwidth (triangle)
    int m_maxBandwidthHz = 5000; // Maximum bandwidth (full trapezoid)

    // Learned NORM width per mode string, shared across both VFO indicators
    // (nominal is a property of the mode, not the receiver).
    static QHash<QString, int> s_normByMode;

    QColor m_lineColor{0xFF, 0xD0, 0x40};       // Gold #FFD040
    QColor m_textColor{0xFF, 0xD0, 0x40};       // Gold #FFD040
    QColor m_shapeColor{0xFF, 0xD0, 0x40, 128}; // Gold with 50% alpha
    QColor m_shapeOutline{0xFF, 0xD0, 0x40};    // Gold outline
};

#endif // FILTERINDICATORWIDGET_H
