#ifndef INWINDOWDIALOG_H
#define INWINDOWDIALOG_H

#include <QString>
#include <QWidget>

class QEventLoop;
class QFrame;
class QKeyEvent;

// A modal presentation that stays inside the existing QWidget top-level.
// Android's QRhi-backed window must not create a second native EGL surface
// while accessibility is inspecting the main window.
class InWindowDialog : public QWidget {
    Q_OBJECT

public:
    enum DialogCode { Rejected = 0, Accepted = 1 };

    explicit InWindowDialog(QWidget *parent);

    QWidget *contentWidget() const;
    void setPanelSize(const QSize &size);
    int exec();

public slots:
    void accept();
    void reject();
    void done(int result);

signals:
    void accepted();
    void rejected();
    void finished(int result);

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    QFrame *m_panel = nullptr;
    QEventLoop *m_eventLoop = nullptr;
    int m_result = Rejected;
};

void showInWindowMessage(QWidget *parent, const QString &title, const QString &message);

#endif // INWINDOWDIALOG_H
