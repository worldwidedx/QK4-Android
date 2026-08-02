#include "inwindowdialog.h"

#include "k4styles.h"

#include <QEventLoop>
#include <QFrame>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

InWindowDialog::InWindowDialog(QWidget *parent)
    : QWidget(parent) {
    setObjectName("inWindowDialogOverlay");
    setAttribute(Qt::WA_StyledBackground, true);
    setFocusPolicy(Qt::StrongFocus);
    setStyleSheet(QString(
        "#inWindowDialogOverlay { background-color: rgba(0, 0, 0, 150); }"
        "#inWindowDialogPanel { background-color: %1; border: 1px solid %2; border-radius: 7px; }")
                          .arg(K4Styles::Colors::Background, K4Styles::Colors::DialogBorder));

    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(8, 6, 8, 6);
    outer->addStretch(1);

    auto *row = new QHBoxLayout();
    row->addStretch(1);
    m_panel = new QFrame(this);
    m_panel->setObjectName("inWindowDialogPanel");
    m_panel->setAttribute(Qt::WA_StyledBackground, true);
    row->addWidget(m_panel);
    row->addStretch(1);
    outer->addLayout(row);
    outer->addStretch(1);
    hide();
}

QWidget *InWindowDialog::contentWidget() const {
    return m_panel;
}

void InWindowDialog::setPanelSize(const QSize &size) {
    m_panel->setFixedSize(size);
}

int InWindowDialog::exec() {
    if (!parentWidget())
        return Rejected;

    setGeometry(parentWidget()->rect());
    m_result = Rejected;
    show();
    raise();
    setFocus(Qt::OtherFocusReason);

    QEventLoop eventLoop;
    m_eventLoop = &eventLoop;
    eventLoop.exec();
    m_eventLoop = nullptr;
    return m_result;
}

void InWindowDialog::accept() {
    done(Accepted);
}

void InWindowDialog::reject() {
    done(Rejected);
}

void InWindowDialog::done(int result) {
    m_result = result;
    hide();
    if (result == Accepted)
        emit accepted();
    else
        emit rejected();
    emit finished(result);
    if (m_eventLoop)
        m_eventLoop->quit();
}

void InWindowDialog::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape || event->key() == Qt::Key_Back) {
        reject();
        return;
    }
    QWidget::keyPressEvent(event);
}

void showInWindowMessage(QWidget *parent, const QString &title, const QString &message) {
    if (!parent)
        return;

    InWindowDialog dialog(parent);
    QWidget *panel = dialog.contentWidget();
    auto *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(14, 12, 14, 12);
    layout->setSpacing(10);

    auto *titleLabel = new QLabel(title, panel);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(QString("color: %1; font-size: 17px; font-weight: bold;")
                                  .arg(K4Styles::Colors::AccentAmber));
    layout->addWidget(titleLabel);

    auto *messageLabel = new QLabel(message, panel);
    messageLabel->setWordWrap(true);
    messageLabel->setTextFormat(Qt::AutoText);
    messageLabel->setOpenExternalLinks(true);
    messageLabel->setStyleSheet(QString("color: %1; font-size: 13px;").arg(K4Styles::Colors::TextWhite));
    layout->addWidget(messageLabel, 1);

    auto *close = new QPushButton("CLOSE", panel);
    close->setMinimumHeight(36);
    close->setStyleSheet(K4Styles::menuBarButton());
    layout->addWidget(close);
    QObject::connect(close, &QPushButton::clicked, &dialog, &InWindowDialog::accept);

    const QSize available = parent->size() - QSize(20, 16);
    const int width = qMin(560, qMax(280, available.width()));
    const int height = qMin(360, qMax(150, layout->sizeHint().height()));
    dialog.setPanelSize(QSize(width, qMin(height, available.height())));
    dialog.exec();
}
