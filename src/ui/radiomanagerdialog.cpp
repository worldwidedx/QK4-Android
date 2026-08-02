#include "radiomanagerdialog.h"
#include "fnpopupwidget.h"
#include "k4styles.h"
#include "network/protocol.h"
#include <QBoxLayout>
#include <QApplication>
#include <QDialogButtonBox>
#include <QFrame>
#include <QScrollArea>
#include <QScroller>
#include <QScrollerProperties>
#include <QScreen>
#include <QResizeEvent>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <utility>

RadioManagerDialog::RadioManagerDialog(QWidget *parent) : QWidget(parent), m_currentIndex(-1) {
    setupUi();
    refreshList();
    updateButtonStates();

    connect(RadioSettings::instance(), &RadioSettings::radiosChanged, this, &RadioManagerDialog::refreshList);
}

void RadioManagerDialog::setupUi() {
    const bool compact = K4Styles::isCompactLayout();
    setObjectName("radioManagerOverlay");
    if (compact) {
        // A landscape phone can have less than 360 logical pixels after the
        // system bars. Never force the dialog taller than that viewport.
        setMinimumSize(320, 240);
        if (QScreen *sc = screen()) {
            const QSize avail = sc->availableGeometry().size();
            resize(qMax(320, avail.width() - 24), qMax(240, avail.height() - 24));
        } else {
            resize(420, 700);
        }
    } else {
        resize(580, 395);
    }

    // Dark popup surface theme
    setStyleSheet(QString("#radioManagerOverlay { background-color: %1; }").arg(K4Styles::Colors::Background));

    auto *dialogLayout = new QVBoxLayout(this);
    dialogLayout->setContentsMargins(0, 0, 0, 0);
    dialogLayout->setSpacing(0);

    auto *scrollArea = new QScrollArea(this);
    scrollArea->setFrameShape(QFrame::NoFrame);
    // The Android app is landscape-first. Keep the compact page sized to the
    // viewport; scrolling is only a fallback for unusually small displays.
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setStyleSheet(QString("QScrollArea { background-color: %1; border: none; }").arg(K4Styles::Colors::Background));
    scrollArea->viewport()->setAutoFillBackground(true);
    scrollArea->viewport()->setStyleSheet(QString("background-color: %1;").arg(K4Styles::Colors::Background));
    dialogLayout->addWidget(scrollArea);

    auto *contentWidget = new QWidget(scrollArea);
    contentWidget->setObjectName("radioManagerContent");
    contentWidget->setStyleSheet(QString("background-color: %1;").arg(K4Styles::Colors::Background));
    scrollArea->setWidget(contentWidget);
    if (compact) {
        contentWidget->setMinimumWidth(qMax(300, width() - 20));
        contentWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }

    auto *mainLayout = new QVBoxLayout(contentWidget);
    mainLayout->setSpacing(K4Styles::Dimensions::PopupContentMargin);
    mainLayout->setContentsMargins(K4Styles::Dimensions::PaddingLarge, K4Styles::Dimensions::PaddingLarge,
                                   K4Styles::Dimensions::PaddingLarge, K4Styles::Dimensions::PaddingLarge);

    // Landscape phone: list on the left, edit form on the right. Stacking
    // these sections vertically was what pushed the action buttons offscreen.
    QBoxLayout *topLayout = new QHBoxLayout();
    topLayout->setSpacing(compact ? K4Styles::Dimensions::PaddingMedium : K4Styles::Dimensions::DialogMargin);

    // === LEFT SIDE: Available Servers ===
    auto *leftSection = new QVBoxLayout();
    leftSection->setSpacing(K4Styles::Dimensions::PopupButtonSpacing);

    auto *serversTitle = new QLabel("Available Servers", this);
    serversTitle->setStyleSheet(QString("QLabel { color: %1; font-weight: bold; font-size: %2px; }")
                                    .arg(K4Styles::Colors::AccentAmber)
                                    .arg(K4Styles::Dimensions::FontSizePopup));
    leftSection->addWidget(serversTitle);

    m_radioList = new QListWidget(this);
    if (compact) {
        m_radioList->setMinimumWidth(150);
        m_radioList->setMaximumWidth(190);
        m_radioList->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    } else {
        m_radioList->setMinimumWidth(180);
        m_radioList->setMaximumWidth(200);
    }
    m_radioList->setStyleSheet(
        QString("QListWidget { "
                "  background-color: %1; "
                "  color: %2; "
                "  border: 1px solid %3; "
                "  border-radius: 4px; "
                "  padding: 4px; "
                "} "
                "QListWidget::item { "
                "  padding: %4px; "
                "} "
                "QListWidget::item:selected { "
                "  background-color: %5; "
                "  color: %1; "
                "} "
                "QListWidget::item:hover { "
                "  background-color: %6; "
                "}")
            .arg(K4Styles::Colors::DarkBackground, K4Styles::Colors::TextWhite, K4Styles::Colors::DialogBorder)
            .arg(K4Styles::Dimensions::PaddingSmall)
            .arg(K4Styles::Colors::AccentAmber, K4Styles::Colors::GradientBottom));
    leftSection->addWidget(m_radioList);
    topLayout->addLayout(leftSection);

    // === RIGHT SIDE: Edit Connect ===
    auto *rightSection = new QVBoxLayout();
    rightSection->setSpacing(K4Styles::Dimensions::PopupButtonSpacing);

    auto *editTitle = new QLabel("Edit Connect", this);
    editTitle->setStyleSheet(QString("QLabel { color: %1; font-weight: bold; font-size: %2px; }")
                                 .arg(K4Styles::Colors::AccentAmber)
                                 .arg(K4Styles::Dimensions::FontSizePopup));
    auto *editTitleRow = new QHBoxLayout();
    editTitleRow->setContentsMargins(0, 0, 0, 0);
    editTitleRow->addWidget(editTitle);
    editTitleRow->addStretch();
    m_macrosButton = new QPushButton(compact ? "F1-F8" : "F1-F8 Macros", this);
    m_macrosButton->setFixedHeight(26);
    m_macrosButton->setFocusPolicy(Qt::NoFocus);
    m_macrosButton->setToolTip("Configure the F1 through F8 buttons in the FN menu");
    m_macrosButton->setStyleSheet(K4Styles::menuBarButton());
    editTitleRow->addWidget(m_macrosButton);
    rightSection->addLayout(editTitleRow);

    // Form fields - label on LEFT of text box
    auto *formLayout = new QGridLayout();
    formLayout->setHorizontalSpacing(K4Styles::Dimensions::PaddingMedium);
    formLayout->setVerticalSpacing(K4Styles::Dimensions::PaddingMedium);

    const int lineEditMinWidth = compact ? 110 : 150;
    QString lineEditStyle = QString("QLineEdit { "
                                    "  background-color: #FFFFFF; "
                                    "  color: #111111; "
                                    "  border: 1px solid %1; "
                                    "  border-radius: 4px; "
                                    "  padding: %2px; "
                                    "  min-width: %3px; "
                                    "} "
                                    "QLineEdit::placeholder { color: #6A6A6A; }")
                                .arg(K4Styles::Colors::DialogBorder)
                                .arg(K4Styles::Dimensions::PaddingSmall)
                                .arg(lineEditMinWidth);

    QString labelStyle = QString("QLabel { color: %1; font-size: %2px; }")
                             .arg(K4Styles::Colors::TextGray)
                             .arg(K4Styles::Dimensions::FontSizeButton);

    // Row 0: Name
    auto *nameLabel = new QLabel("Name", this);
    nameLabel->setStyleSheet(labelStyle);
    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setStyleSheet(lineEditStyle);
    m_nameEdit->setInputMethodHints(Qt::ImhNoAutoUppercase | Qt::ImhNoPredictiveText);
    m_nameEdit->setPlaceholderText("Server Name");
    formLayout->addWidget(nameLabel, 0, 0);
    formLayout->addWidget(m_nameEdit, 0, 1);

    // Row 1: Host or IP
    auto *hostLabel = new QLabel("Host or IP", this);
    hostLabel->setStyleSheet(labelStyle);
    m_hostEdit = new QLineEdit(this);
    m_hostEdit->setStyleSheet(lineEditStyle);
    m_hostEdit->setInputMethodHints(Qt::ImhNoAutoUppercase | Qt::ImhNoPredictiveText);
    m_hostEdit->setPlaceholderText("192.168.1.100");
    formLayout->addWidget(hostLabel, 1, 0);
    formLayout->addWidget(m_hostEdit, 1, 1);

    // Row 2: Port
    auto *portLabel = new QLabel("Port", this);
    portLabel->setStyleSheet(labelStyle);
    m_portEdit = new QLineEdit(this);
    m_portEdit->setStyleSheet(lineEditStyle);
    m_portEdit->setInputMethodHints(Qt::ImhDigitsOnly);
    m_portEdit->setPlaceholderText("64242");
    m_portEdit->setMaximumWidth(80);
    formLayout->addWidget(portLabel, 2, 0);
    formLayout->addWidget(m_portEdit, 2, 1);

    // Row 3: Password
    auto *passwordLabel = new QLabel("Password", this);
    passwordLabel->setStyleSheet(labelStyle);
    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setStyleSheet(lineEditStyle);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    // Let the Android IME own Shift and case state. Password mode keeps text
    // hidden, while these hints only disable prediction and sentence casing.
    m_passwordEdit->setInputMethodHints(Qt::ImhNoAutoUppercase | Qt::ImhNoPredictiveText | Qt::ImhSensitiveData);
    m_passwordEdit->setPlaceholderText("Password");
    auto *passwordContainer = new QWidget(this);
    auto *passwordLayout = new QHBoxLayout(passwordContainer);
    passwordLayout->setContentsMargins(0, 0, 0, 0);
    passwordLayout->setSpacing(K4Styles::Dimensions::PaddingSmall);
    passwordLayout->addWidget(m_passwordEdit, 1);
    auto *showPassword = new QCheckBox("Show", passwordContainer);
    showPassword->setStyleSheet(QString("QCheckBox { color: %1; font-size: %2px; spacing: 4px; }")
                                    .arg(K4Styles::Colors::TextGray)
                                    .arg(K4Styles::Dimensions::FontSizeButton));
    passwordLayout->addWidget(showPassword);
    connect(showPassword, &QCheckBox::toggled, this, [this](bool visible) {
        m_passwordEdit->setEchoMode(visible ? QLineEdit::Normal : QLineEdit::Password);
    });
    formLayout->addWidget(passwordLabel, 3, 0);
    formLayout->addWidget(passwordContainer, 3, 1);

    // Row 4: ID (only visible when TLS is checked)
    m_identityLabel = new QLabel("ID", this);
    m_identityLabel->setStyleSheet(labelStyle);
    m_identityEdit = new QLineEdit(this);
    m_identityEdit->setStyleSheet(lineEditStyle);
    m_identityEdit->setInputMethodHints(Qt::ImhNoAutoUppercase | Qt::ImhNoPredictiveText);
    m_identityEdit->setPlaceholderText("Identity (optional)");
    formLayout->addWidget(m_identityLabel, compact ? 3 : 4, compact ? 2 : 0);
    formLayout->addWidget(m_identityEdit, compact ? 3 : 4, compact ? 3 : 1);

    // Row 5: TLS Checkbox (below ID field)
    m_tlsCheckbox = new QCheckBox("Use TLS (Encrypted)", this);
    m_tlsCheckbox->setStyleSheet(QString("QCheckBox { color: %1; font-size: %2px; spacing: %3px; } "
                                         "QCheckBox::indicator { width: 14px; height: 14px; }")
                                     .arg(K4Styles::Colors::TextGray)
                                     .arg(K4Styles::Dimensions::FontSizeButton)
                                     .arg(K4Styles::Dimensions::BorderRadiusLarge));
    formLayout->addWidget(m_tlsCheckbox, compact ? 2 : 5, compact ? 2 : 0, 1, 2);

    // Row 6: Encode Mode dropdown
    auto *encodeModeLabel = new QLabel("Audio Mode", this);
    encodeModeLabel->setStyleSheet(labelStyle);
    m_encodeModeCombo = new QComboBox(this);
    m_encodeModeCombo->setStyleSheet(
        QString("QComboBox { "
                "  background-color: #FFFFFF; "
                "  color: #111111; "
                "  border: 1px solid %1; "
                "  border-radius: 4px; "
                "  padding: %2px; "
                "} "
                "QComboBox::drop-down { "
                "  border: none; "
                "  width: 20px; "
                "} "
                "QComboBox::down-arrow { "
                "  image: none; "
                "  border-left: 5px solid transparent; "
                "  border-right: 5px solid transparent; "
                "  border-top: 5px solid #111111; "
                "} "
                "QComboBox QAbstractItemView { "
                "  background-color: #FFFFFF; "
                "  color: #111111; "
                "  selection-background-color: %3; "
                "}")
            .arg(K4Styles::Colors::DialogBorder)
            .arg(K4Styles::Dimensions::PaddingSmall)
            .arg(K4Styles::Colors::AccentAmber));
    m_encodeModeCombo->addItem("EM3 - Opus Float", 3); // Default
    m_encodeModeCombo->addItem("EM2 - Opus Int", 2);
    m_encodeModeCombo->addItem("EM1 - RAW 16-bit", 1);
    m_encodeModeCombo->addItem("EM0 - RAW 32-bit", 0);
    m_encodeModeCombo->setCurrentIndex(0); // EM3 default
    formLayout->addWidget(encodeModeLabel, compact ? 0 : 6, compact ? 2 : 0);
    formLayout->addWidget(m_encodeModeCombo, compact ? 0 : 6, compact ? 3 : 1);

    // Row 7: Streaming Latency dropdown
    auto *streamingLatencyLabel = new QLabel("Streaming Latency", this);
    streamingLatencyLabel->setStyleSheet(labelStyle);
    m_streamingLatencyCombo = new QComboBox(this);
    m_streamingLatencyCombo->setStyleSheet(m_encodeModeCombo->styleSheet());
    for (int i = 0; i <= 7; i++) {
        m_streamingLatencyCombo->addItem(QString::number(i), i);
    }
    m_streamingLatencyCombo->setCurrentIndex(3); // Default: 3
    formLayout->addWidget(streamingLatencyLabel, compact ? 1 : 7, compact ? 2 : 0);
    formLayout->addWidget(m_streamingLatencyCombo, compact ? 1 : 7, compact ? 3 : 1);

    // Initially hide ID field (shown when TLS is checked)
    m_identityLabel->setVisible(false);
    m_identityEdit->setVisible(false);

    rightSection->addLayout(formLayout);
    if (!compact) {
        rightSection->addStretch();
    }
    topLayout->addLayout(rightSection);
    if (!compact) {
        topLayout->addStretch();
    }

    mainLayout->addLayout(topLayout);

    // === BOTTOM: Button Row ===
    m_connectButton = new QPushButton("Connect", this);
    m_connectButton->setStyleSheet(K4Styles::dialogButton());

    m_newButton = new QPushButton("Add", this);
    m_newButton->setStyleSheet(K4Styles::dialogButton());

    m_saveButton = new QPushButton("Save Profile", this);
    m_saveButton->setStyleSheet(K4Styles::dialogButton());

    m_deleteButton = new QPushButton(compact ? "Remove" : "Delete", this);
    m_deleteButton->setStyleSheet(K4Styles::dialogButton());

    // Back button - smaller with curved arrow
    m_backButton = new QPushButton(QString::fromUtf8("\xE2\x86\xA9"), this); // ↩ Curved arrow
    m_backButton->setStyleSheet(K4Styles::dialogButton());
    m_backButton->setFixedSize(K4Styles::Dimensions::ButtonHeightMedium, K4Styles::Dimensions::ButtonHeightMedium);
    m_backButton->setToolTip("Back / Exit");

    // Android's IME may activate a dialog's default button when the user
    // finishes password entry. Saving and connecting must remain explicit.
    for (auto *button : {m_connectButton, m_newButton, m_saveButton, m_deleteButton, m_backButton}) {
        button->setAutoDefault(false);
        button->setDefault(false);
        button->setFocusPolicy(Qt::NoFocus);
    }
    connect(m_passwordEdit, &QLineEdit::returnPressed, this, [this]() {
        m_passwordEdit->clearFocus();
        updateButtonStates();
    });

    if (compact) {
        auto *buttonLayout = new QHBoxLayout();
        buttonLayout->setContentsMargins(K4Styles::Dimensions::PaddingSmall, K4Styles::Dimensions::PaddingSmall,
                                         K4Styles::Dimensions::PaddingSmall, K4Styles::Dimensions::PaddingSmall);
        buttonLayout->setSpacing(K4Styles::Dimensions::PaddingSmall);
        buttonLayout->addWidget(m_connectButton);
        buttonLayout->addWidget(m_newButton);
        buttonLayout->addWidget(m_saveButton);
        buttonLayout->addWidget(m_deleteButton);
        buttonLayout->addWidget(m_backButton);
        // Pin actions above the scrollable content. Even if an unusually small
        // display needs form scrolling, profile management never leaves the
        // viewport.
        dialogLayout->insertLayout(0, buttonLayout);
    } else {
        auto *buttonLayout = new QHBoxLayout();
        buttonLayout->setSpacing(16); // More spacing between buttons
        buttonLayout->addWidget(m_connectButton);
        buttonLayout->addWidget(m_newButton);
        buttonLayout->addWidget(m_saveButton);
        buttonLayout->addWidget(m_deleteButton);
        buttonLayout->addWidget(m_backButton);
        mainLayout->addLayout(buttonLayout);
    }

    if (compact) {
        scrollArea->viewport()->setAttribute(Qt::WA_AcceptTouchEvents);
        QScroller::grabGesture(scrollArea->viewport(), QScroller::TouchGesture);
        if (QScroller *scroller = QScroller::scroller(scrollArea->viewport())) {
            QScrollerProperties properties = scroller->scrollerProperties();
            properties.setScrollMetric(QScrollerProperties::MousePressEventDelay, 0.25);
            properties.setScrollMetric(QScrollerProperties::DragStartDistance, 0.0015);
            scroller->setScrollerProperties(properties);
        }
    }

    // Connections
    connect(m_connectButton, &QPushButton::clicked, this, &RadioManagerDialog::onConnectClicked);
    connect(m_newButton, &QPushButton::clicked, this, &RadioManagerDialog::onNewClicked);
    connect(m_saveButton, &QPushButton::clicked, this, &RadioManagerDialog::onSaveClicked);
    connect(m_deleteButton, &QPushButton::clicked, this, &RadioManagerDialog::onDeleteClicked);
    connect(m_backButton, &QPushButton::clicked, this, &RadioManagerDialog::onBackClicked);
    connect(m_macrosButton, &QPushButton::clicked, this, &RadioManagerDialog::onMacrosClicked);
    connect(m_radioList, &QListWidget::itemSelectionChanged, this, &RadioManagerDialog::onSelectionChanged);
    connect(m_radioList, &QListWidget::itemDoubleClicked, this, &RadioManagerDialog::onItemDoubleClicked);
    connect(m_tlsCheckbox, &QCheckBox::toggled, this, &RadioManagerDialog::onTlsCheckboxToggled);

    // Update button states when host field changes
    connect(m_hostEdit, &QLineEdit::textChanged, this, [this]() { updateButtonStates(); });
}

void RadioManagerDialog::refreshList() {
    m_radioList->clear();

    const auto radios = RadioSettings::instance()->radios();
    for (const auto &radio : radios) {
        m_radioList->addItem(radio.name.isEmpty() ? radio.host : radio.name);
    }

    int lastIndex = RadioSettings::instance()->lastSelectedIndex();
    if (lastIndex >= 0 && lastIndex < m_radioList->count()) {
        m_radioList->setCurrentRow(lastIndex);
        m_currentIndex = lastIndex;
        populateFieldsFromSelection();
    }

    updateButtonStates();
}

void RadioManagerDialog::onConnectClicked() {
    QString host = m_hostEdit->text().trimmed();
    if (!host.isEmpty()) {
        // Check if this is a disconnect request (selected radio is already connected)
        if (!m_connectedHost.isEmpty() && host == m_connectedHost) {
            emit disconnectRequested();
            emit closeRequested();
            return;
        }

        RadioEntry entry;
        entry.name = m_nameEdit->text().trimmed();
        entry.host = host;
        entry.password = m_passwordEdit->text();
        QString portText = m_portEdit->text().trimmed();
        entry.useTls = m_tlsCheckbox->isChecked();
        entry.identity = m_identityEdit->text();
        entry.encodeMode = m_encodeModeCombo->currentData().toInt();
        entry.streamingLatency = m_streamingLatencyCombo->currentData().toInt();

        // Set port based on TLS mode if not specified
        if (portText.isEmpty()) {
            entry.port = entry.useTls ? K4Protocol::TLS_PORT : K4Protocol::DEFAULT_PORT;
        } else {
            entry.port = portText.toUShort();
        }

        if (m_currentIndex >= 0) {
            RadioSettings::instance()->setLastSelectedIndex(m_currentIndex);
        }
        emit connectRequested(entry);
        emit closeRequested();
    }
}

void RadioManagerDialog::onNewClicked() {
    m_currentIndex = -1;
    clearFields();
    m_radioList->blockSignals(true);
    m_radioList->clearSelection();
    m_radioList->setCurrentRow(-1);
    m_radioList->blockSignals(false);
    m_nameEdit->setFocus();
    updateButtonStates();
}

void RadioManagerDialog::onSaveClicked() {
    QString name = m_nameEdit->text().trimmed();
    QString host = m_hostEdit->text().trimmed();
    QString password = m_passwordEdit->text();
    QString portText = m_portEdit->text().trimmed();
    bool useTls = m_tlsCheckbox->isChecked();
    QString identity = m_identityEdit->text();

    if (name.isEmpty()) {
        name = host; // Use host as name if no name provided
    }

    if (host.isEmpty()) {
        return; // Can't save without host
    }

    RadioEntry entry;
    entry.name = name;
    entry.host = host;
    entry.password = password;
    entry.useTls = useTls;
    entry.identity = identity;
    entry.encodeMode = m_encodeModeCombo->currentData().toInt();
    entry.streamingLatency = m_streamingLatencyCombo->currentData().toInt();

    // Set port based on TLS mode if not specified
    if (portText.isEmpty()) {
        entry.port = useTls ? K4Protocol::TLS_PORT : K4Protocol::DEFAULT_PORT;
    } else {
        entry.port = portText.toUShort();
    }

    if (m_currentIndex >= 0 && m_currentIndex < RadioSettings::instance()->radios().size()) {
        // Update existing
        RadioSettings::instance()->updateRadio(m_currentIndex, entry);
    } else {
        // Add new
        RadioSettings::instance()->addRadio(entry);
        m_currentIndex = RadioSettings::instance()->radios().size() - 1;
    }

    // Re-select the saved item
    if (m_currentIndex >= 0 && m_currentIndex < m_radioList->count()) {
        m_radioList->setCurrentRow(m_currentIndex);
    }
    updateButtonStates();
}

void RadioManagerDialog::onDeleteClicked() {
    if (m_currentIndex >= 0 && m_currentIndex < RadioSettings::instance()->radios().size()) {
        RadioSettings::instance()->removeRadio(m_currentIndex);
        clearFields();
        m_currentIndex = -1;
    }
    updateButtonStates();
}

void RadioManagerDialog::onBackClicked() {
    emit closeRequested();
}

void RadioManagerDialog::onSelectionChanged() {
    int row = m_radioList->currentRow();
    if (row >= 0) {
        m_currentIndex = row;
        populateFieldsFromSelection();
    }
    updateButtonStates();
}

void RadioManagerDialog::onItemDoubleClicked(QListWidgetItem *item) {
    Q_UNUSED(item)
    onConnectClicked();
}

void RadioManagerDialog::updateButtonStates() {
    bool hasSelection = m_currentIndex >= 0 && m_currentIndex < RadioSettings::instance()->radios().size();
    QString host = m_hostEdit->text().trimmed();
    bool hasHost = !host.isEmpty();

    // Check if the selected radio is the connected one
    bool isConnectedRadio = !m_connectedHost.isEmpty() && host == m_connectedHost;

    m_connectButton->setEnabled(hasHost);
    m_connectButton->setText(isConnectedRadio ? "Disconnect" : "Connect");
    m_deleteButton->setEnabled(hasSelection);
    m_saveButton->setEnabled(hasHost);
}

void RadioManagerDialog::clearFields() {
    m_nameEdit->clear();
    m_hostEdit->clear();
    m_portEdit->clear();
    m_passwordEdit->clear();
    m_tlsCheckbox->setChecked(false);
    m_identityEdit->clear();
    m_identityLabel->setVisible(false);
    m_identityEdit->setVisible(false);
    m_encodeModeCombo->setCurrentIndex(0);       // Reset to EM3 (default)
    m_streamingLatencyCombo->setCurrentIndex(3); // Reset to SL3 (default)
}

void RadioManagerDialog::populateFieldsFromSelection() {
    if (m_currentIndex >= 0 && m_currentIndex < RadioSettings::instance()->radios().size()) {
        const RadioEntry &radio = RadioSettings::instance()->radios().at(m_currentIndex);
        m_nameEdit->setText(radio.name);
        m_hostEdit->setText(radio.host);
        m_portEdit->setText(QString::number(radio.port));
        m_passwordEdit->setText(radio.password);
        m_tlsCheckbox->setChecked(radio.useTls);
        m_identityEdit->setText(radio.identity);
        m_identityLabel->setVisible(radio.useTls);
        m_identityEdit->setVisible(radio.useTls);
        // Set encode mode combo to match saved value
        int encodeModeIndex = m_encodeModeCombo->findData(radio.encodeMode);
        if (encodeModeIndex >= 0) {
            m_encodeModeCombo->setCurrentIndex(encodeModeIndex);
        }
        // Set streaming latency combo to match saved value
        int latencyIndex = m_streamingLatencyCombo->findData(radio.streamingLatency);
        if (latencyIndex >= 0) {
            m_streamingLatencyCombo->setCurrentIndex(latencyIndex);
        }
    }
}

void RadioManagerDialog::onTlsCheckboxToggled(bool checked) {
    // Show/hide ID field based on TLS checkbox state
    m_identityLabel->setVisible(checked);
    m_identityEdit->setVisible(checked);

    // Auto-update port if it was at default
    QString portText = m_portEdit->text().trimmed();
    if (portText.isEmpty() || portText == QString::number(K4Protocol::DEFAULT_PORT) ||
        portText == QString::number(K4Protocol::TLS_PORT)) {
        m_portEdit->setText(QString::number(checked ? K4Protocol::TLS_PORT : K4Protocol::DEFAULT_PORT));
    }
}

void RadioManagerDialog::onMacrosClicked() {
    // Keep the phone setup workflow self-contained: F1-F8 are the only
    // macros exposed here, and map directly to the paired FN-menu buttons.
    if (m_macroEditor) {
        m_macroEditor->raise();
        return;
    }

    // This must remain a child widget. A top-level QDialog creates another
    // Android EGL/RHI surface and can deadlock with Android accessibility.
    auto *editor = new QWidget(this);
    m_macroEditor = editor;
    editor->setObjectName("macroEditorOverlay");
    editor->setAttribute(Qt::WA_StyledBackground, true);
    editor->setStyleSheet(QString("#macroEditorOverlay { background-color: %1; }")
                              .arg(K4Styles::Colors::Background));
    editor->setGeometry(rect());
    connect(editor, &QObject::destroyed, this, [this]() { m_macroEditor = nullptr; });

    const bool compact = K4Styles::isCompactLayout();

    auto *layout = new QVBoxLayout(editor);
    layout->setContentsMargins(K4Styles::Dimensions::PaddingLarge, K4Styles::Dimensions::PaddingLarge,
                               K4Styles::Dimensions::PaddingLarge, K4Styles::Dimensions::PaddingLarge);
    layout->setSpacing(K4Styles::Dimensions::PaddingMedium);

    auto *help = new QLabel("Set the label shown in FN and the CAT command sent when that F key is tapped.", editor);
    help->setWordWrap(true);
    help->setStyleSheet(QString("QLabel { color: %1; font-size: %2px; }")
                            .arg(K4Styles::Colors::TextGray)
                            .arg(K4Styles::Dimensions::FontSizeButton));
    layout->addWidget(help);

    auto *scrollArea = new QScrollArea(editor);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setStyleSheet(QString("QScrollArea { background-color: %1; border: none; }"
                                      "QScrollBar:vertical { background-color: %2; width: 8px; }"
                                      "QScrollBar::handle:vertical { background-color: %3; border-radius: 4px; }")
                                  .arg(K4Styles::Colors::Background)
                                  .arg(K4Styles::Colors::DarkBackground)
                                  .arg(K4Styles::Colors::GradientTop));
    scrollArea->viewport()->setStyleSheet(
        QString("background-color: %1;").arg(K4Styles::Colors::Background));
    auto *content = new QWidget(scrollArea);
    content->setStyleSheet(QString("background-color: %1;").arg(K4Styles::Colors::Background));
    auto *grid = new QGridLayout(content);
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setHorizontalSpacing(K4Styles::Dimensions::PaddingMedium);
    grid->setVerticalSpacing(K4Styles::Dimensions::PaddingSmall);

    const QString headerStyle = QString("QLabel { color: %1; font-weight: bold; font-size: %2px; }")
                                    .arg(K4Styles::Colors::AccentAmber)
                                    .arg(K4Styles::Dimensions::FontSizeButton);
    for (const auto &[column, text] : {std::pair<int, QString>{0, "Key"}, {1, "Button label"}, {2, "CAT command"}}) {
        auto *header = new QLabel(text, content);
        header->setStyleSheet(headerStyle);
        grid->addWidget(header, 0, column);
    }

    const QString editStyle = QString("QLineEdit { background: %1; color: %2; border: 1px solid %3; "
                                      "border-radius: 4px; padding: %4px; } "
                                      "QLineEdit:focus { border-color: %5; } "
                                      "QLineEdit::placeholder { color: %6; } "
                                      "QLineEdit::selection { background: %5; color: %7; }")
                                  .arg(K4Styles::Colors::GradientMid1)
                                  .arg(K4Styles::Colors::TextWhite)
                                  .arg(K4Styles::Colors::DialogBorder)
                                  .arg(K4Styles::Dimensions::PaddingSmall)
                                  .arg(K4Styles::Colors::AccentAmber)
                                  .arg(K4Styles::Colors::TextFaded)
                                  .arg(K4Styles::Colors::TextDark);
    const QVector<QString> ids = {MacroIds::FnF1, MacroIds::FnF2, MacroIds::FnF3, MacroIds::FnF4,
                                  MacroIds::FnF5, MacroIds::FnF6, MacroIds::FnF7, MacroIds::FnF8};
    // Eight macro rows must fit above the fixed Save/Cancel bar in the
    // landscape phone editor. Desktop sizing remains unchanged.
    const int compactFieldHeight = compact ? 42 : 0;
    QVector<QPair<QLineEdit *, QLineEdit *>> fields;
    fields.reserve(ids.size());
    for (int i = 0; i < ids.size(); ++i) {
        auto *key = new QLabel(QString("F%1").arg(i + 1), content);
        key->setStyleSheet(QString("QLabel { color: %1; font-weight: bold; }").arg(K4Styles::Colors::TextWhite));
        auto *label = new QLineEdit(content);
        auto *command = new QLineEdit(content);
        const MacroEntry macro = RadioSettings::instance()->macro(ids.at(i));
        label->setText(macro.label);
        command->setText(macro.command);
        label->setMaxLength(12);
        command->setMaxLength(64);
        label->setPlaceholderText(QString("F%1").arg(i + 1));
        command->setPlaceholderText("Example: KY CQ CQ DE ...;");
        label->setStyleSheet(editStyle);
        command->setStyleSheet(editStyle);
        if (compactFieldHeight > 0) {
            label->setFixedHeight(compactFieldHeight);
            command->setFixedHeight(compactFieldHeight);
            grid->setRowMinimumHeight(i + 1, compactFieldHeight);
        }
        label->setInputMethodHints(Qt::ImhNoPredictiveText);
        command->setInputMethodHints(Qt::ImhNoPredictiveText);
        grid->addWidget(key, i + 1, 0);
        grid->addWidget(label, i + 1, 1);
        grid->addWidget(command, i + 1, 2);
        fields.append(qMakePair(label, command));
    }
    grid->setColumnStretch(1, 1);
    grid->setColumnStretch(2, 3);
    scrollArea->setWidget(content);
    if (compact) {
        // F1-F8 must remain reachable on a landscape phone.  Keep normal
        // finger scrolling available for unusually small displays, too.
        scrollArea->viewport()->setAttribute(Qt::WA_AcceptTouchEvents);
        QScroller::grabGesture(scrollArea->viewport(), QScroller::TouchGesture);
    }
    layout->addWidget(scrollArea, 1);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, editor);
    buttons->setStyleSheet(K4Styles::dialogButton());
    connect(buttons, &QDialogButtonBox::accepted, editor, [editor, ids, fields]() {
        for (int i = 0; i < ids.size(); ++i) {
            RadioSettings::instance()->setMacro(ids.at(i), fields.at(i).first->text().trimmed(),
                                                fields.at(i).second->text().trimmed());
        }
        editor->deleteLater();
    });
    connect(buttons, &QDialogButtonBox::rejected, editor, &QObject::deleteLater);
    layout->addWidget(buttons);

    editor->show();
    editor->raise();
}

void RadioManagerDialog::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    if (m_macroEditor) {
        m_macroEditor->setGeometry(rect());
    }
}

RadioEntry RadioManagerDialog::selectedRadio() const {
    if (m_currentIndex >= 0) {
        return RadioSettings::instance()->radios().at(m_currentIndex);
    }
    return RadioEntry();
}

bool RadioManagerDialog::hasSelection() const {
    return m_currentIndex >= 0;
}

void RadioManagerDialog::setConnectedHost(const QString &host) {
    m_connectedHost = host;
    updateButtonStates();
}
