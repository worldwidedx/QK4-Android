#include "ui/dtmfpopup.h"
#include "ui/k4styles.h"

#include <QGridLayout>
#include <QPushButton>

DtmfPopupWidget::DtmfPopupWidget(QWidget *parent) : K4PopupBase(parent) {
    auto *grid = new QGridLayout(this);
    grid->setContentsMargins(contentMargins());
    grid->setSpacing(7);
    const QStringList keys = {"1", "2", "3", "A", "4", "5", "6", "B",
                              "7", "8", "9", "C", "*", "0", "#", "D"};
    for (int i = 0; i < keys.size(); ++i) {
        auto *button = new QPushButton(keys.at(i), this);
        button->setMinimumSize(68, 48);
        button->setStyleSheet(K4Styles::menuBarButtonSmall());
        grid->addWidget(button, i / 4, i % 4);
        connect(button, &QPushButton::clicked, this, [this, key = keys.at(i)]() {
            emit digitRequested(key.at(0));
        });
    }
    auto *close = new QPushButton(QString::fromUtf8("\xE2\x86\xA9"), this);
    close->setMinimumHeight(45);
    close->setStyleSheet(K4Styles::menuBarButtonSmall());
    grid->addWidget(close, 4, 0, 1, 4);
    connect(close, &QPushButton::clicked, this, &K4PopupBase::hidePopup);
    initPopup();
}

QSize DtmfPopupWidget::contentSize() const { return QSize(320, 270); }
