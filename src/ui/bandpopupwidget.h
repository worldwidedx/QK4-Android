#ifndef BANDPOPUPWIDGET_H
#define BANDPOPUPWIDGET_H

#include "k4popupbase.h"
#include <QList>
#include <QMap>
#include <QPushButton>

/**
 * @brief Band selection popup with 14 bands in 2 rows.
 *
 * Layout:
 * Row 1: 1.8, 3.5, 7, 14, 21, 28, MEM
 * Row 2: GEN, 5, 10, 18, 24, 50, XVTR
 */
class BandPopupWidget : public K4PopupBase {
    Q_OBJECT

public:
    explicit BandPopupWidget(QWidget *parent = nullptr);

    // Set the currently selected band by name
    void setSelectedBand(const QString &bandName);
    QString selectedBand() const { return m_selectedBand; }

    // Band number methods for K4 BN command
    void setSelectedBandByNumber(int bandNum);
    int getBandNumber(const QString &bandName) const;
    QString getBandName(int bandNum) const;

signals:
    void bandSelected(const QString &bandName);

protected:
    QSize contentSize() const override;

private:
    void setupUi();
    QPushButton *createBandButton(const QString &text);
    void updateButtonStyles();
    void onBandButtonClicked();

    QMap<QString, QPushButton *> m_buttonMap;

    QString m_selectedBand;
};

#endif // BANDPOPUPWIDGET_H
