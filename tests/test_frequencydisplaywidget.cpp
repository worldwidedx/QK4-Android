#include "ui/frequencydisplaywidget.h"

#include <QtTest>

class TestFrequencyDisplayWidget : public QObject {
    Q_OBJECT

private slots:
    void formats_data();
    void formats();
};

void TestFrequencyDisplayWidget::formats_data() {
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("expected");

    QTest::newRow("40 metres") << QStringLiteral("7024980") << QStringLiteral("7.024.980");
    QTest::newRow("20 metres") << QStringLiteral("14074000") << QStringLiteral("14.074.000");
    QTest::newRow("2 metres") << QStringLiteral("144200000") << QStringLiteral("144.200.000");
    QTest::newRow("23 centimetres") << QStringLiteral("1296000000") << QStringLiteral("1.296.000.000");
}

void TestFrequencyDisplayWidget::formats() {
    QFETCH(QString, input);
    QFETCH(QString, expected);
    FrequencyDisplayWidget widget;
    widget.setFrequency(input);
    QCOMPARE(widget.displayText(), expected);
}

QTEST_MAIN(TestFrequencyDisplayWidget)
#include "test_frequencydisplaywidget.moc"
