#ifndef SIDECONTROLPANEL_H
#define SIDECONTROLPANEL_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QTimer>

class DualControlButton;
class QGridLayout;
class MonOverlay;
class BalOverlay;

/**
 * SideControlPanel - Left-side vertical control panel for QK4
 *
 * Contains 3 groups of 2 DualControlButtons each:
 * - Group 1 (Global/Orange): WPM/PTCH, PWR/DLY
 * - Group 2 (MainRx/Cyan): BW/HI, SHFT/LO (linked pair - swap together)
 * - Group 3 (RF-SQL): M.RF/M.SQL, S.SQL/S.RF
 *
 * Group behavior:
 * - Only one button per group shows the colored indicator bar at a time
 * - Clicking a button makes it active (gets the bar) and swaps its labels
 * - BW/SHFT are linked: when one swaps, the other swaps too
 * - Scrolling on the active button changes its value
 */
class SideControlPanel : public QWidget {
    Q_OBJECT

public:
    explicit SideControlPanel(QWidget *parent = nullptr);
    ~SideControlPanel() = default;

    // Mode-dependent display (CW mode shows WPM/PTCH, Voice mode shows MIC/CMP)
    void setDisplayMode(bool isCWMode);

    // Update displayed values (call with current radio state)
    // CW mode values
    void setWpm(int wpm);
    void setPitch(double pitch);
    // Voice mode values
    void setMicGain(int gain);     // 0-80
    void setCompression(int comp); // 0-30
    void setPower(double power);
    void setDelay(double delay);
    void setBandwidth(double bw);
    void setHighCut(double hi); // kHz
    void setShift(double shift);
    void setLowCut(double lo); // kHz
    void setMainRfGain(int gain);
    void setMainSquelch(int sql);
    void setSubSquelch(int sql);
    void setSubRfGain(int gain);

    // Update status area (mirrors header data)
    void setTime(const QString &time);
    void setPowerReading(double watts);
    void setSwr(double swr);
    void setVoltage(double volts);
    void setCurrent(double amps);

    // Set which receiver is active for filter controls
    void setActiveReceiver(bool isSubRx);

    // Volume control
    int volume() const;
    int subVolume() const;

    // Monitor level (MON overlay)
    void updateMonitorLevel(int mode, int level);
    void updateMonitorMode(int mode);

    // Balance (BAL overlay + button)
    void updateBalance(int mode, int offset);

    // Cancel an alternate-action hold when a containing phone panel begins scrolling.
    void cancelPendingLongPress();

signals:
    // Icon button signals
    void helpClicked();
    void connectClicked();

    // TX Function button signals (left-click = primary, right-click = secondary)
    void tuneClicked();    // TUNE - SW16;
    void tuneLpClicked();  // TUNE LP - SW131;
    void xmitClicked();    // XMIT - SW30;
    void testClicked();    // TEST - SW132;
    void atuClicked();     // ATU - SW158;
    void atuTuneClicked(); // ATU TUNE - SW40;
    void voxClicked();     // VOX - SW50;
    void qskClicked();     // QSK - SW134;
    void antClicked();     // ANT - SW60;
    void remAntClicked();  // REM ANT - TBD
    void rxAntClicked();   // RX ANT - SW70;
    void subAntClicked();  // SUB ANT - SW157;

    // Value changed signals (emitted when user scrolls to change value)
    // CW mode signals
    void wpmChanged(int delta);
    void pitchChanged(int delta);
    // Voice mode signals
    void micGainChanged(int delta);
    void compressionChanged(int delta);
    void powerChanged(int delta);
    void delayChanged(int delta);
    void bandwidthChanged(int delta);
    void highCutChanged(int delta);
    void shiftChanged(int delta);
    void lowCutChanged(int delta);
    void mainRfGainChanged(int delta);
    void mainSquelchChanged(int delta);
    void subSquelchChanged(int delta);
    void subRfGainChanged(int delta);
    void volumeChanged(int value);    // 0-100 (Main RX / VFO A)
    void subVolumeChanged(int value); // 0-100 (Sub RX / VFO B)

    // SW command signals (MON/NORM/BAL buttons)
    void swCommandRequested(const QString &command);

    // Monitor level change (ML command)
    void monLevelChangeRequested(int mode, int level);

    // Balance change (BAL overlay mode toggle or scroll)
    void balChangeRequested(int mode, int offset);

private slots:
    // Group 1: WPM/PWR - handle activation and scrolling
    void onWpmBecameActive();
    void onPwrBecameActive();
    void onWpmScrolled(int delta);
    void onPwrScrolled(int delta);

    // Group 2: BW/SHFT - linked pair, swap together
    void onBwBecameActive();
    void onShiftBecameActive();
    void onBwScrolled(int delta);
    void onShiftScrolled(int delta);
    void onBwClicked();
    void onShiftClicked();

    // Group 3: MainRf/SubSql
    void onMainRfBecameActive();
    void onSubSqlBecameActive();
    void onMainRfScrolled(int delta);
    void onSubSqlScrolled(int delta);

protected:
    // Event filter for right-click handling on TX function buttons
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void setupUi();
    void triggerSecondary(QObject *watched);
    void configureAdjustmentSlider(DualControlButton *button, QSlider *slider);
    void setGroup1Active(DualControlButton *activeBtn);
    void setGroup2Active(DualControlButton *activeBtn);
    void setGroup3Active(DualControlButton *activeBtn);
    QPushButton *createIconButton(const QString &text);
    QWidget *createTxFunctionButton(const QString &mainText, const QString &subText, QPushButton *&btnOut);

    // Track mode (CW shows WPM/PTCH, Voice shows MIC/CMP)
    bool m_isCWMode = true;

    // Track which function is currently primary for each button
    // (needed to emit correct signal on scroll)
    bool m_wpmIsPrimary = true;    // WPM/PTCH (CW) or MIC/CMP (Voice)
    bool m_pwrIsPrimary = true;    // PWR or DLY
    bool m_bwIsPrimary = true;     // BW or HI
    bool m_shiftIsPrimary = true;  // SHFT or LO
    bool m_mainRfIsPrimary = true; // M.RF or M.SQL
    bool m_subSqlIsPrimary = true; // S.SQL or S.RF

    // Group 1: Global (CW/Power)
    DualControlButton *m_wpmBtn;
    DualControlButton *m_pwrBtn;

    // Group 2: Filter (BW/Shift) - linked pair
    DualControlButton *m_bwBtn;
    DualControlButton *m_shiftBtn;

    // Group 3: RF/Squelch
    DualControlButton *m_mainRfBtn;
    DualControlButton *m_subSqlBtn;

    QSlider *m_wpmSlider = nullptr;
    QSlider *m_pwrSlider = nullptr;
    QSlider *m_bwSlider = nullptr;
    QSlider *m_shiftSlider = nullptr;
    QSlider *m_mainRfSlider = nullptr;
    QSlider *m_subSqlSlider = nullptr;

    int m_wpmValue = 20;
    int m_pitchValue = 600;
    int m_micValue = 0;
    int m_compressionValue = 0;
    double m_powerValue = 0.0;
    int m_delayValue = 0;
    int m_bandwidthValue = 2400;
    int m_highCutValue = 2400;
    int m_shiftValue = 0;
    int m_lowCutValue = 0;
    int m_mainRfValue = 0;
    int m_mainSqlValue = 0;
    int m_subSqlValue = 0;
    int m_subRfValue = 0;

    // Status labels
    QLabel *m_timeLabel;
    QLabel *m_powerSwrLabel;
    QLabel *m_voltageCurrentLabel;

    // Icon buttons (at bottom of panel)
    QPushButton *m_helpBtn;
    QPushButton *m_connectBtn;

    // TX Function buttons (2x3 grid)
    QPushButton *m_tuneBtn;
    QPushButton *m_xmitBtn;
    QPushButton *m_atuTuneBtn;
    QPushButton *m_voxBtn;
    QPushButton *m_antBtn;
    QPushButton *m_rxAntBtn;

    QTimer *m_longPressTimer = nullptr;
    QObject *m_longPressTarget = nullptr;
    bool m_longPressHandled = false;
    bool m_suppressNextRelease = false;

    // Volume sliders
    QSlider *m_volumeSlider;
    QLabel *m_volumeLabel;
    QSlider *m_subVolumeSlider;
    QLabel *m_subVolumeLabel;

    // MON/NORM/BAL buttons (above volume sliders)
    QPushButton *m_monBtn;
    QPushButton *m_normBtn;
    QPushButton *m_balBtn;

    // Overlay widgets
    MonOverlay *m_monOverlay;
    BalOverlay *m_balOverlay;
};

#endif // SIDECONTROLPANEL_H
