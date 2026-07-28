#include "frequencydisplaywidget.h"
#include "k4styles.h"
#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QFontMetrics>

FrequencyDisplayWidget::FrequencyDisplayWidget(QWidget *parent)
    : QWidget(parent), m_digits("00000000"), m_normalColor(K4Styles::Colors::TextWhite),
      m_editColor(K4Styles::Colors::VfoACyan) {

    // Set up font with tabular figures for consistent digit widths
    m_font = K4Styles::Fonts::dataFont(K4Styles::Dimensions::FontSizeFrequency);

    // Calculate character widths for click detection
    QFontMetrics fm(m_font);
    m_charWidth = fm.horizontalAdvance('0');
    m_dotWidth = fm.horizontalAdvance('.');

    // Widget configuration
    setFocusPolicy(Qt::ClickFocus);
    setCursor(Qt::PointingHandCursor);
    setAttribute(Qt::WA_Hover);

    // Size hint based on max display width (XX.XXX.XXX = 8 digits + 2 dots)
    int width = m_charWidth * 8 + m_dotWidth * 2 + 4; // +4 for padding
    setMinimumWidth(width);
    setFixedHeight(K4Styles::Dimensions::MenuItemHeight);
}

void FrequencyDisplayWidget::setFrequency(const QString &frequency) {
    parseFrequency(frequency);
    update();
}

QString FrequencyDisplayWidget::frequency() const {
    return m_digits;
}

QString FrequencyDisplayWidget::displayText() const {
    return formatWithDots();
}

void FrequencyDisplayWidget::setEditModeColor(const QColor &color) {
    m_editColor = color;
    if (m_cursorPosition >= 0) {
        update();
    }
}

void FrequencyDisplayWidget::setNormalColor(const QColor &color) {
    m_normalColor = color;
    if (m_cursorPosition < 0) {
        update();
    }
}

void FrequencyDisplayWidget::setTuningRateDigit(int digitFromRight) {
    if (m_tuningRateDigit != digitFromRight) {
        m_tuningRateDigit = digitFromRight;
        update();
    }
}

bool FrequencyDisplayWidget::isEditing() const {
    return m_cursorPosition >= 0;
}

void FrequencyDisplayWidget::parseFrequency(const QString &freq) {
    // Remove dots and any non-digit characters
    QString digits;
    for (const QChar &c : freq) {
        if (c.isDigit()) {
            digits.append(c);
        }
    }

    // Pad left with zeros to 8 digits, or truncate if too long
    while (digits.length() < 8) {
        digits.prepend('0');
    }
    if (digits.length() > 8) {
        digits = digits.right(8);
    }

    m_digits = digits;
}

QString FrequencyDisplayWidget::formatWithDots() const {
    // Format: X.XXX.XXX or XX.XXX.XXX
    // Input: "07024980" -> Output: "7.024.980"
    // Input: "14024980" -> Output: "14.024.980"

    QString result;
    int startIdx = 0;

    // Skip leading zero for <10MHz frequencies
    if (m_digits[0] == '0') {
        startIdx = 1;
    }

    for (int i = startIdx; i < 8; ++i) {
        result.append(m_digits[i]);

        // Insert dots after positions 1 (or 0 if started at 1) and 4
        int posFromRight = 7 - i;
        if (posFromRight == 6 || posFromRight == 3) {
            if (i < 7) { // Don't add trailing dot
                result.append('.');
            }
        }
    }

    return result;
}

int FrequencyDisplayWidget::digitIndexFromCharIndex(int charIndex) const {
    // Map character index in display string to digit index (0-7)
    QString display = formatWithDots();
    if (charIndex < 0 || charIndex >= display.length()) {
        return -1;
    }

    // Count digits before this position
    int digitCount = 0;
    for (int i = 0; i < charIndex; ++i) {
        if (display[i] != '.') {
            digitCount++;
        }
    }

    // If this character is a dot, return -1
    if (display[charIndex] == '.') {
        return -1;
    }

    // Adjust for leading zero skip
    int offset = (m_digits[0] == '0') ? 1 : 0;
    return offset + digitCount;
}

QRect FrequencyDisplayWidget::charRectAt(int charIndex) const {
    QString display = formatWithDots();
    if (charIndex < 0 || charIndex >= display.length()) {
        return QRect();
    }

    // Calculate X position by summing widths of preceding characters
    int x = 0;
    for (int i = 0; i < charIndex; ++i) {
        x += (display[i] == '.') ? m_dotWidth : m_charWidth;
    }

    int charW = (display[charIndex] == '.') ? m_dotWidth : m_charWidth;
    return QRect(x, 0, charW, height());
}

int FrequencyDisplayWidget::digitPositionFromX(int x) const {
    QString display = formatWithDots();

    int currentX = 0;
    for (int i = 0; i < display.length(); ++i) {
        int charW = (display[i] == '.') ? m_dotWidth : m_charWidth;

        if (x >= currentX && x < currentX + charW) {
            int digitIdx = digitIndexFromCharIndex(i);
            if (digitIdx >= 0) {
                return digitIdx;
            }
            // Clicked on a dot - find nearest digit
            // Check left neighbor first
            if (i > 0) {
                int leftDigit = digitIndexFromCharIndex(i - 1);
                if (leftDigit >= 0) {
                    return leftDigit;
                }
            }
            // Check right neighbor
            if (i + 1 < display.length()) {
                int rightDigit = digitIndexFromCharIndex(i + 1);
                if (rightDigit >= 0) {
                    return rightDigit;
                }
            }
        }
        currentX += charW;
    }

    // Click beyond display - select last digit
    return 7;
}

void FrequencyDisplayWidget::enterEditMode(int digitPosition) {
    if (digitPosition < 0 || digitPosition > 7) {
        return;
    }

    m_originalDigits = m_digits;
    m_cursorPosition = digitPosition;
    setFocus();
    grabMouse(); // Capture all mouse events to detect clicks outside
    update();
}

void FrequencyDisplayWidget::exitEditMode(bool send) {
    if (m_cursorPosition < 0) {
        return; // Not in edit mode
    }

    releaseMouse(); // Release mouse grab

    if (send) {
        // Remove leading zeros for the signal (but keep at least one digit)
        QString digits = m_digits;
        while (digits.length() > 1 && digits[0] == '0') {
            digits.remove(0, 1);
        }
        emit frequencyEntered(digits);
    } else {
        // Restore original frequency
        m_digits = m_originalDigits;
        emit editingCancelled();
    }

    m_cursorPosition = -1;
    clearFocus();
    update();
}

void FrequencyDisplayWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setFont(m_font);

    QString display = formatWithDots();

    // Draw each character
    int x = 0;
    int digitIdx = (m_digits[0] == '0') ? 1 : 0; // Start digit index

    for (int i = 0; i < display.length(); ++i) {
        QChar c = display[i];
        int charW = (c == '.') ? m_dotWidth : m_charWidth;

        // Determine color for this character
        QColor charColor;
        if (m_cursorPosition >= 0) {
            // Edit mode: all characters in edit color
            charColor = m_editColor;
        } else if (c == '.') {
            // Dots always in normal color
            charColor = m_normalColor;
        } else {
            // Normal mode: check if this digit should be grayed (tuning rate indicator)
            // digitIdx is 0-7 from left, convert to position from right: 7 - digitIdx
            int posFromRight = 7 - digitIdx;
            if (m_tuningRateDigit >= 0 && posFromRight <= m_tuningRateDigit) {
                // This digit is at or below tuning rate - show in gray
                charColor = QColor(K4Styles::Colors::TextGray);
            } else {
                charColor = m_normalColor;
            }
        }
        p.setPen(charColor);

        // Draw the character
        QRect charRect(x, 0, charW, height());
        p.drawText(charRect, Qt::AlignCenter, c);

        // Draw cursor underline if this is the selected digit (edit mode)
        if (c != '.' && (digitIdx == m_cursorPosition ||
                         (m_cursorPosition < 0 && digitIdx == m_touchStepPosition))) {
            int underlineY = height() - 4;
            p.fillRect(x + 2, underlineY, charW - 4, 2, m_editColor);
        }

        // Advance digit index (only for non-dot characters)
        if (c != '.') {
            digitIdx++;
        }

        x += charW;
    }
}

void FrequencyDisplayWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        // Check if click is inside widget bounds
        QRect widgetRect(0, 0, width(), height());
        bool insideWidget = widgetRect.contains(event->pos());

        if (m_cursorPosition >= 0 && !insideWidget) {
            // In edit mode but clicked outside - cancel
            exitEditMode(false);
            return;
        }

        if (insideWidget) {
            int digitPos = digitPositionFromX(event->pos().x());
            if (digitPos >= 0) {
#ifdef Q_OS_ANDROID
                // A phone has no physical keyboard. Keep the digit visibly
                // selected and let the dedicated touch controls step it.
                m_touchStepPosition = digitPos;
                emit tuningDigitSelected(7 - digitPos);
                update();
#else
                if (m_cursorPosition < 0) {
                    // Not in edit mode - enter it
                    enterEditMode(digitPos);
                } else {
                    // Already in edit mode - move cursor
                    m_cursorPosition = digitPos;
                    update();
                }
#endif
            }
        }
    }
    QWidget::mousePressEvent(event);
}

void FrequencyDisplayWidget::keyPressEvent(QKeyEvent *event) {
    if (m_cursorPosition < 0) {
        QWidget::keyPressEvent(event);
        return;
    }

    int key = event->key();

    if (key >= Qt::Key_0 && key <= Qt::Key_9) {
        // Replace digit at cursor position
        m_digits[m_cursorPosition] = QChar('0' + (key - Qt::Key_0));

        // Advance cursor (stop at end)
        if (m_cursorPosition < 7) {
            m_cursorPosition++;
        }
        update();
    } else if (key == Qt::Key_Left) {
        if (m_cursorPosition > 0) {
            m_cursorPosition--;
            update();
        }
    } else if (key == Qt::Key_Right) {
        if (m_cursorPosition < 7) {
            m_cursorPosition++;
            update();
        }
    } else if (key == Qt::Key_Home) {
        m_cursorPosition = 0;
        update();
    } else if (key == Qt::Key_End) {
        m_cursorPosition = 7;
        update();
    } else if (key == Qt::Key_Return || key == Qt::Key_Enter) {
        exitEditMode(true); // Send frequency
    } else if (key == Qt::Key_Escape) {
        exitEditMode(false); // Cancel
    } else {
        QWidget::keyPressEvent(event);
    }
}

void FrequencyDisplayWidget::wheelEvent(QWheelEvent *event) {
    if (m_cursorPosition >= 0) {
        event->ignore(); // In edit mode — don't tune
        return;
    }
    int steps = m_wheelAccumulator.accumulate(event);
    if (steps != 0)
        emit frequencyScrolled(steps);
    event->accept();
}

void FrequencyDisplayWidget::focusOutEvent(QFocusEvent *event) {
    // Cancel edit mode when focus is lost
    if (m_cursorPosition >= 0) {
        exitEditMode(false);
    }
    QWidget::focusOutEvent(event);
}
