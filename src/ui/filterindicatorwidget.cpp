#include "filterindicatorwidget.h"
#include "k4styles.h"
#include <QPainter>
#include <QPolygonF>
#include <algorithm>

QHash<QString, int> FilterIndicatorWidget::s_normByMode;

FilterIndicatorWidget::FilterIndicatorWidget(QWidget *parent) : QWidget(parent) {
    setFixedSize(62, 62); // 50 * 1.25 = 62
}

void FilterIndicatorWidget::setFilterPosition(int position) {
    if (position >= 1 && position <= 3 && position != m_filterPosition) {
        m_filterPosition = position;
        update();
    }
}

void FilterIndicatorWidget::setBandwidth(int bandwidthHz) {
    int clamped = std::clamp(bandwidthHz, m_minBandwidthHz, m_maxBandwidthHz);
    if (clamped != m_bandwidthHz) {
        m_bandwidthHz = clamped;
        update();
    }
}

void FilterIndicatorWidget::setShift(int shift) {
    // Shift is in decahertz (10 Hz units), range typically 0-300+ for SSB
    int clamped = std::clamp(shift, 0, 400);
    if (clamped != m_shift) {
        m_shift = clamped;
        update();
    }
}

void FilterIndicatorWidget::setMode(const QString &mode) {
    if (mode != m_mode) {
        m_mode = mode;
        update();
    }
}

void FilterIndicatorWidget::setBandwidthRange(int minHz, int maxHz) {
    m_minBandwidthHz = minHz;
    m_maxBandwidthHz = maxHz;
    update();
}

void FilterIndicatorWidget::setShapeColor(const QColor &fill, const QColor &outline) {
    m_shapeColor = fill;
    m_shapeOutline = outline;
    update();
}

void FilterIndicatorWidget::setNormBandwidth(int hz) {
    if (hz <= 0)
        return;
    s_normByMode.insert(m_mode, hz);
    update();
}

void FilterIndicatorWidget::drawBandwidthShape(QPainter &painter, int lineY, int lineWidth) {
    // Shape height
    const float shapeHeight = 16.0f;

    // K4 uses discrete steps with more granularity in USB/SSB range (1500-3500 Hz)
    // Max visual growth reached at ~2.70 kHz

    float baseWidth;
    float topWidth;

    // Determine which step we're at based on bandwidth
    // More granular steps in the USB/SSB working range
    int step;
    if (m_bandwidthHz <= 200) {
        step = 0; // Triangle (CW narrow)
    } else if (m_bandwidthHz <= 400) {
        step = 1; // Small trapezoid
    } else if (m_bandwidthHz <= 600) {
        step = 2;
    } else if (m_bandwidthHz <= 900) {
        step = 3;
    } else if (m_bandwidthHz <= 1200) {
        step = 4;
    } else if (m_bandwidthHz <= 1600) {
        step = 5;
    } else if (m_bandwidthHz <= 2000) {
        step = 6;
    } else if (m_bandwidthHz <= 2400) {
        step = 7;
    } else if (m_bandwidthHz <= 2800) {
        step = 8;
    } else if (m_bandwidthHz <= 3200) {
        step = 9;
    } else {
        step = 10; // Max (3200+ Hz)
    }

    if (step == 0) {
        // Triangle
        baseWidth = 16.0f;
        topWidth = 0.0f;
    } else {
        // Trapezoid - discrete steps from small to full width
        // K4 shapes are compact - don't reach full line width until max
        const float maxBase = static_cast<float>(lineWidth) * 0.85f; // Don't go full width
        const float minBase = 16.0f;                                 // Start close to triangle size
        const int maxSteps = 10;

        // Linear interpolation across steps
        float stepNorm = static_cast<float>(step - 1) / static_cast<float>(maxSteps - 1);
        baseWidth = minBase + stepNorm * (maxBase - minBase);

        // Top ratio also grows with steps
        const float minTopRatio = 0.40f;
        const float maxTopRatio = 0.70f; // Slightly narrower top ratio
        float topToBaseRatio = minTopRatio + stepNorm * (maxTopRatio - minTopRatio);
        topWidth = baseWidth * topToBaseRatio;
    }

    // Calculate center X position based on shift
    // K4 IF shift is in decahertz (10 Hz units): shift=150 means 1500 Hz
    // Default center varies by mode:
    // - SSB/DATA: ~135 (1350 Hz passband center)
    // - CW: ~50 (500 Hz, based on CW pitch)
    // - AM/FM: Always centered (ignore shift value - K4 doesn't use shift for AM/FM)
    float centerX = width() / 2.0f;

    // AM/FM modes are always carrier-centered, ignore shift
    if (m_mode != "AM" && m_mode != "FM") {
        int defaultShift;
        if (m_mode == "CW" || m_mode == "CW-R") {
            defaultShift = 50; // 500 Hz (CW pitch)
        } else {
            defaultShift = 135; // 1350 Hz (SSB/DATA)
        }
        const float shiftRange = 100.0f; // ±1000 Hz visual range
        float shiftNorm = static_cast<float>(m_shift - defaultShift) / shiftRange;
        shiftNorm = std::clamp(shiftNorm, -1.0f, 1.0f);
        float maxShiftPx = (lineWidth - baseWidth) / 2.0f;
        if (maxShiftPx > 0) {
            centerX += shiftNorm * maxShiftPx;
        }
    }

    // Calculate shape vertices
    const float gapAboveLine = 4.0f;
    float bottomY = lineY - gapAboveLine;
    float topY = bottomY - shapeHeight;

    // FSK/AFSK: the K4 draws the same passband trapezoid as other modes but
    // with a notch in the top edge, so the mark/space tones show as two peaks
    // at the top corners. At the narrow end it collapses to a single triangle;
    // as BW widens the top spreads into a plateau with two corner peaks. Drawn
    // centred (the pair straddles the passband centre), matching the radio.
    if (m_mode.startsWith(QLatin1String("FSK")) || m_mode.startsWith(QLatin1String("AFSK"))) {
        const float fcx = width() / 2.0f;
        // FSK always resolves two tone peaks; their separation scales with the
        // filter bandwidth. Map the FSK working range (~150-800 Hz) to a peak
        // spacing that starts clearly apart and grows, capped so the widest
        // setting still fits the 62px widget instead of clipping.
        const float bwMin = 150.0f, bwMax = 800.0f;
        const float t = std::clamp((static_cast<float>(m_bandwidthHz) - bwMin) / (bwMax - bwMin), 0.0f, 1.0f);
        const float halfTop = 9.0f + t * 12.0f;   // peaks: ~18px..42px apart
        const float halfBase = halfTop + 5.0f;    // sides slope outward below the peaks
        const float tl = fcx - halfTop, tr = fcx + halfTop;
        const float bl = fcx - halfBase, br = fcx + halfBase;
        const float valleyY = topY + shapeHeight * 0.30f;
        painter.setPen(Qt::NoPen);
        painter.setBrush(m_shapeColor);
        QPolygonF shape;
        shape << QPointF(bl, bottomY) << QPointF(tl, topY) << QPointF(fcx, valleyY) << QPointF(tr, topY)
              << QPointF(br, bottomY);
        painter.drawPolygon(shape);
        drawFilterBaseline(painter, bl, br, lineY);
        return;
    }

    float bottomLeft = centerX - baseWidth / 2.0f;
    float bottomRight = centerX + baseWidth / 2.0f;
    float topLeft = centerX - topWidth / 2.0f;
    float topRight = centerX + topWidth / 2.0f;

    // Build polygon
    QPolygonF shape;
    if (topWidth < 1.0f) {
        // Triangle (apex at top)
        shape << QPointF(centerX, topY) << QPointF(bottomRight, bottomY) << QPointF(bottomLeft, bottomY);
    } else {
        // Trapezoid (4 points)
        shape << QPointF(topLeft, topY) << QPointF(topRight, topY) << QPointF(bottomRight, bottomY)
              << QPointF(bottomLeft, bottomY);
    }

    // Draw filled shape (no outline)
    painter.setPen(Qt::NoPen);
    painter.setBrush(m_shapeColor);
    painter.drawPolygon(shape);

    drawFilterBaseline(painter, bottomLeft, bottomRight, lineY);
}

int FilterIndicatorWidget::normBandwidthHz() const {
    // The nominal width learned when the operator last pressed NORM in this
    // mode is authoritative; the per-mode guesses below are only a fallback
    // for a mode NORM has not been pressed in yet this session.
    auto it = s_normByMode.constFind(m_mode);
    if (it != s_normByMode.constEnd())
        return it.value();
    if (m_mode == "FM" || m_mode.startsWith(QLatin1String("PSK")))
        return 0; // no NORM marker
    if (m_mode.startsWith(QLatin1String("FSK")) || m_mode.startsWith(QLatin1String("AFSK")))
        return 300;
    if (m_mode == "CW" || m_mode == "CW-R")
        return 400;
    if (m_mode == "AM")
        return 6000;
    return 2700; // SSB / DATA nominal
}

void FilterIndicatorWidget::drawFilterBaseline(QPainter &painter, float leftX, float rightX, float lineY) {
    // The K4 draws a fixed-length yellow reference line, the same for every
    // mode (the coloured filter shape varies, this line does not). Centre it
    // under the current shape and give it a constant half-width.
    const float cx = (leftX + rightX) / 2.0f;
    const float half = 22.0f; // fixed: CW and LSB lines are identical length
    const float lx = cx - half;
    const float rx = cx + half;

    painter.setBrush(Qt::NoBrush);
    QPen pen(m_lineColor, 2);
    pen.setJoinStyle(Qt::RoundJoin); // clean corner, no miter spike above the flat
    painter.setPen(pen);

    // When the passband is at (near) the mode's NORM width, the two ends turn
    // downward. A tolerance (~10%, min 40 Hz) absorbs small differences between
    // the radio's actual nominal and our per-mode fallback so both VFOs show
    // the ends at their default width. Drawn as one polyline so the corners
    // join cleanly and the legs never rise above the flat line.
    const int norm = normBandwidthHz();
    const int tol = qMax(40, norm / 10);
    if (norm > 0 && qAbs(m_bandwidthHz - norm) <= tol) {
        const float len = 5.0f;
        const float out = 2.0f;
        const QPointF pts[4] = {
            QPointF(lx - out, lineY + len),
            QPointF(lx, lineY),
            QPointF(rx, lineY),
            QPointF(rx + out, lineY + len),
        };
        painter.drawPolyline(pts, 4);
    } else {
        painter.drawLine(QPointF(lx, lineY), QPointF(rx, lineY));
    }
}

void FilterIndicatorWidget::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int h = height();

    // Line parameters
    // Preserve breathing room above the phone's always-visible antenna row.
    int lineY = K4Styles::isCompactLayout() ? 36 : 40;
    int lineWidth = 58; // 38 + 20 (10px wider on each side)

    // Draw bandwidth shape; the yellow passband line (and NORM ends) are drawn
    // with it so the line width matches the current filter.
    drawBandwidthShape(painter, lineY, lineWidth);

    // FIL text below line
    QFont textFont = font();
    textFont.setPixelSize(K4Styles::Dimensions::FontSizeButton);
    textFont.setBold(true);
    painter.setFont(textFont);
    painter.setPen(m_textColor);

    QString text = QString("FIL%1").arg(m_filterPosition);
    int textY = lineY + 3 + 2; // 3 = passband line thickness (see drawFilterBaseline)
    QRectF textRect(0, textY, w, h - textY);
    painter.drawText(textRect, Qt::AlignHCenter | Qt::AlignTop, text);
}
