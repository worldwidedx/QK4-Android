#ifndef DTMFPOPUP_H
#define DTMFPOPUP_H

#include "ui/k4popupbase.h"

class DtmfPopupWidget : public K4PopupBase {
    Q_OBJECT
public:
    explicit DtmfPopupWidget(QWidget *parent = nullptr);

signals:
    void digitRequested(QChar digit);

protected:
    QSize contentSize() const override;
};

#endif
