// Virtual Stream Deck: Qt Widgets UI with Windows global hotkeys/actions.

#include "Actions.h"
#include "App.h"
#include "Config.h"

#include <QtCore/QAbstractNativeEventFilter>
#include <QtCore/QPropertyAnimation>
#include <QtGui/QCloseEvent>
#include <QtGui/QGuiApplication>
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtGui/QScreen>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStyle>
#include <QtWidgets/QSystemTrayIcon>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QVBoxLayout>

#include <algorithm>

namespace
{
    constexpr UINT HOTKEY_MODIFIERS = MOD_CONTROL | MOD_ALT;
    constexpr int HOTKEY_TOGGLE_KEY = VK_SPACE;
    constexpr int HOTKEY_EDITOR_KEY = 'E';

    QString ToQString(const std::wstring& text)
    {
        return QString::fromStdWString(text);
    }

    std::wstring ToWString(const QString& text)
    {
        return text.toStdWString();
    }

    QColor ToQColor(COLORREF color)
    {
        return QColor(GetRValue(color), GetGValue(color), GetBValue(color));
    }

    QString ButtonStyleSheet()
    {
        const ThemeColors colors = CurrentThemeColors();
        return QString(
            "QWidget { background: %1; color: %2; }"
            "QLineEdit, QComboBox, QTableWidget { background: %3; color: %2; border: 1px solid %4; padding: 3px; }"
            "QPushButton { background: %5; color: %2; border: 1px solid %4; border-radius: 4px; padding: 5px 10px; }"
            "QPushButton:hover { background: %6; }"
            "QHeaderView::section { background: %5; color: %2; border: 1px solid %4; padding: 4px; }")
            .arg(ToQColor(colors.editorBackground).name(),
                ToQColor(colors.text).name(),
                ToQColor(colors.controlBackground).name(),
                ToQColor(colors.buttonBorder).name(),
                ToQColor(colors.buttonFill).name(),
                ToQColor(colors.buttonActiveFill).name());
    }

    QStringList ActionNames()
    {
        return { "website", "program", "folder", "hotkey", "text", "next_page", "none" };
    }

    QStringList PanelPositionLabels()
    {
        QStringList labels;
        for (int i = 0; i <= static_cast<int>(PanelPosition::BottomRight); ++i)
        {
            labels.push_back(ToQString(PanelPositionToLabel(static_cast<PanelPosition>(i))));
        }
        return labels;
    }

    QStringList ThemeLabels()
    {
        return {
            ToQString(ThemeToLabel(AppTheme::Dark)),
            ToQString(ThemeToLabel(AppTheme::Coffee)),
            ToQString(ThemeToLabel(AppTheme::Beige)),
            ToQString(ThemeToLabel(AppTheme::BlueGray))
        };
    }

    QMessageBox::StandardButton Information(QWidget* owner, const QString& text)
    {
        return QMessageBox::information(owner, "Virtual Stream Deck", text);
    }
}

class EditorWindow;

class PanelWindow final : public QWidget
{
public:
    explicit PanelWindow(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setWindowTitle("Virtual Stream Deck");
        setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
        setAttribute(Qt::WA_TranslucentBackground, false);
        setWindowOpacity(0.93);
        setFocusPolicy(Qt::StrongFocus);
        resize(PanelWidth(), PanelHeight());
    }

    void setEditor(EditorWindow* editor)
    {
        editor_ = editor;
    }

    void showPanel()
    {
        QRect target = targetGeometry();
        const bool wasVisible = isVisible();
        resize(target.size());
        move(wasVisible ? target.topLeft() : QPoint(target.x(), screenWorkArea().bottom() + 8));
        show();
        raise();
        activateWindow();
        setFocus();
        g_visible = true;

        if (!wasVisible)
        {
            auto* animation = new QPropertyAnimation(this, "geometry", this);
            animation->setDuration(80);
            animation->setStartValue(geometry());
            animation->setEndValue(target);
            animation->start(QAbstractAnimation::DeleteWhenStopped);
        }
        else
        {
            setGeometry(target);
        }
    }

    void hidePanel()
    {
        if (!isVisible())
        {
            g_visible = false;
            return;
        }

        g_visible = false;
        QRect end = geometry();
        end.moveTop(screenWorkArea().bottom() + 8);

        auto* animation = new QPropertyAnimation(this, "geometry", this);
        animation->setDuration(80);
        animation->setStartValue(geometry());
        animation->setEndValue(end);
        connect(animation, &QPropertyAnimation::finished, this, &QWidget::hide);
        animation->start(QAbstractAnimation::DeleteWhenStopped);
    }

    void handlePanelHotkey()
    {
        if (g_visible && isVisible())
        {
            selectNextButton();
        }
        else
        {
            showPanel();
        }
    }

    void openEditor();

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        const ThemeColors colors = CurrentThemeColors();
        painter.fillRect(rect(), ToQColor(colors.panelBackground));

        QFont titleFont("Segoe UI", 14, QFont::DemiBold);
        painter.setFont(titleFont);
        painter.setPen(ToQColor(colors.text));

        QRect titleRect(PADDING, PADDING, width() - PADDING * 2 - SETTINGS_SIZE - 8, 30);
        const QString title = g_pages.empty() ? "Virtual Stream Deck" : ToQString(g_pages[g_currentPage].name);
        painter.drawText(titleRect, Qt::AlignLeft | Qt::AlignVCenter, title);

        drawSettingsButton(painter);

        if (g_pages.empty())
        {
            return;
        }

        QFont buttonFont("Segoe UI", 10, QFont::DemiBold);
        painter.setFont(buttonFont);
        for (int i = 0; i < ButtonCount(); ++i)
        {
            drawButton(painter, buttonRect(i), g_pages[g_currentPage].buttons[i], i == g_selectedButton);
        }
    }

    void keyPressEvent(QKeyEvent* event) override
    {
        switch (event->key())
        {
        case Qt::Key_Tab:
            selectNextButton();
            break;
        case Qt::Key_Return:
        case Qt::Key_Enter:
            ExecuteButton(g_selectedButton);
            update();
            hidePanel();
            break;
        case Qt::Key_Escape:
            hidePanel();
            break;
        case Qt::Key_Left:
            moveSelection(-1, 0);
            break;
        case Qt::Key_Right:
            moveSelection(1, 0);
            break;
        case Qt::Key_Up:
            moveSelection(0, -1);
            break;
        case Qt::Key_Down:
            moveSelection(0, 1);
            break;
        default:
            QWidget::keyPressEvent(event);
            break;
        }
    }

    void mouseMoveEvent(QMouseEvent* event) override
    {
        const int index = hitTestButton(event->pos());
        if (index >= 0 && index != g_selectedButton)
        {
            g_selectedButton = index;
            update();
        }
    }

    void mousePressEvent(QMouseEvent* event) override
    {
        if (settingsRect().contains(event->pos()))
        {
            return;
        }

        const int index = hitTestButton(event->pos());
        if (index >= 0)
        {
            g_selectedButton = index;
            update();
        }
    }

    void mouseReleaseEvent(QMouseEvent* event) override
    {
        if (settingsRect().contains(event->pos()))
        {
            openEditor();
            return;
        }

        const int index = hitTestButton(event->pos());
        if (index >= 0)
        {
            g_selectedButton = index;
            ExecuteButton(index);
            update();
            hidePanel();
        }
    }

    void changeEvent(QEvent* event) override
    {
        if (event->type() == QEvent::ActivationChange && !isActiveWindow() && g_visible)
        {
            hidePanel();
        }
        QWidget::changeEvent(event);
    }

private:
    QRect screenWorkArea() const
    {
        QScreen* screen = QGuiApplication::screenAt(pos());
        if (!screen)
        {
            screen = QGuiApplication::primaryScreen();
        }
        return screen ? screen->availableGeometry() : QRect(0, 0, 1280, 720);
    }

    QRect targetGeometry() const
    {
        const QRect work = screenWorkArea();
        const int margin = 24;
        const int panelWidth = PanelWidth();
        const int panelHeight = PanelHeight();
        int x = work.right() - panelWidth - margin + 1;
        int y = work.top() + (work.height() - panelHeight) / 2;

        switch (g_panelPosition)
        {
        case PanelPosition::TopLeft:
            x = work.left() + margin;
            y = work.top() + margin;
            break;
        case PanelPosition::TopCenter:
            x = work.left() + (work.width() - panelWidth) / 2;
            y = work.top() + margin;
            break;
        case PanelPosition::TopRight:
            x = work.right() - panelWidth - margin + 1;
            y = work.top() + margin;
            break;
        case PanelPosition::LeftCenter:
            x = work.left() + margin;
            y = work.top() + (work.height() - panelHeight) / 2;
            break;
        case PanelPosition::Center:
            x = work.left() + (work.width() - panelWidth) / 2;
            y = work.top() + (work.height() - panelHeight) / 2;
            break;
        case PanelPosition::RightCenter:
            x = work.right() - panelWidth - margin + 1;
            y = work.top() + (work.height() - panelHeight) / 2;
            break;
        case PanelPosition::BottomLeft:
            x = work.left() + margin;
            y = work.bottom() - panelHeight - margin + 1;
            break;
        case PanelPosition::BottomCenter:
            x = work.left() + (work.width() - panelWidth) / 2;
            y = work.bottom() - panelHeight - margin + 1;
            break;
        case PanelPosition::BottomRight:
            x = work.right() - panelWidth - margin + 1;
            y = work.bottom() - panelHeight - margin + 1;
            break;
        }

        return QRect(x, y, panelWidth, panelHeight);
    }

    QRect buttonRect(int index) const
    {
        const int titleHeight = 42;
        const int gridTop = PADDING + titleHeight;
        const int cellWidth = (PanelWidth() - PADDING * 2 - GAP * (g_gridSize - 1)) / g_gridSize;
        const int cellHeight = (PanelHeight() - gridTop - PADDING - GAP * (g_gridSize - 1)) / g_gridSize;
        const int row = index / g_gridSize;
        const int col = index % g_gridSize;
        return QRect(PADDING + col * (cellWidth + GAP), gridTop + row * (cellHeight + GAP), cellWidth, cellHeight);
    }

    QRect settingsRect() const
    {
        return QRect(PanelWidth() - PADDING - SETTINGS_SIZE, PADDING, SETTINGS_SIZE, SETTINGS_SIZE);
    }

    int hitTestButton(const QPoint& point) const
    {
        for (int i = 0; i < ButtonCount(); ++i)
        {
            if (buttonRect(i).contains(point))
            {
                return i;
            }
        }
        return -1;
    }

    void drawButton(QPainter& painter, const QRect& rect, const ButtonAction& button, bool active)
    {
        const ThemeColors colors = CurrentThemeColors();
        painter.setPen(QPen(ToQColor(active ? colors.buttonActiveBorder : colors.buttonBorder), active ? 2 : 1));
        painter.setBrush(ToQColor(active ? colors.buttonActiveFill : colors.buttonFill));
        painter.drawRoundedRect(rect.adjusted(1, 1, -1, -1), 12, 12);

        QRect textRect = rect.adjusted(8, 8, -8, -8);
        if (!button.imagePath.empty())
        {
            const QString imagePath = ToQString(ExpandEnvironment(button.imagePath));
            QPixmap image(imagePath);
            if (!image.isNull())
            {
                const int boxSize = 42;
                QRect imageRect(rect.left() + (rect.width() - boxSize) / 2, rect.top() + 14, boxSize, boxSize);
                painter.drawPixmap(imageRect, image.scaled(imageRect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
            }
            textRect.setTop(textRect.top() + 54);
        }

        painter.setPen(ToQColor(button.type == ActionType::None ? colors.mutedText : colors.text));
        painter.drawText(textRect, Qt::AlignCenter | Qt::TextWordWrap, ToQString(button.title.empty() ? L"None" : button.title));
    }

    void drawSettingsButton(QPainter& painter)
    {
        const ThemeColors colors = CurrentThemeColors();
        const QRect rect = settingsRect();
        painter.setPen(QPen(ToQColor(colors.settingsBorder), 1));
        painter.setBrush(ToQColor(colors.settingsFill));
        painter.drawRoundedRect(rect, 8, 8);
        painter.setPen(ToQColor(colors.text));
        painter.drawText(rect, Qt::AlignCenter, "...");
    }

    void selectNextButton()
    {
        g_selectedButton = (g_selectedButton + 1) % ButtonCount();
        update();
    }

    void moveSelection(int dx, int dy)
    {
        int row = g_selectedButton / g_gridSize;
        int col = g_selectedButton % g_gridSize;
        row = std::clamp(row + dy, 0, g_gridSize - 1);
        col = std::clamp(col + dx, 0, g_gridSize - 1);
        g_selectedButton = row * g_gridSize + col;
        update();
    }

    EditorWindow* editor_ = nullptr;
};

class EditorWindow final : public QMainWindow
{
public:
    explicit EditorWindow(PanelWindow& panel, QWidget* parent = nullptr)
        : QMainWindow(parent), panel_(panel)
    {
        setWindowTitle("Налаштування Virtual Stream Deck");
        resize(1200, 720);
        buildUi();
        refreshStyle();
        loadEditorPage();
    }

    void openForCurrentPage()
    {
        panel_.hidePanel();
        if (g_pages.empty())
        {
            AddDefaultButtons();
            NormalizePages();
        }
        g_editorPage = std::clamp(g_currentPage, 0, static_cast<int>(g_pages.size()) - 1);
        loadEditorPage();
        showNormal();
        raise();
        activateWindow();
    }

    void refreshStyle()
    {
        setStyleSheet(ButtonStyleSheet());
    }

protected:
    void closeEvent(QCloseEvent* event) override
    {
        hide();
        event->ignore();
    }

private:
    void buildUi()
    {
        auto* central = new QWidget(this);
        auto* root = new QVBoxLayout(central);

        auto* top = new QGridLayout();
        pageCombo_ = new QComboBox(central);
        pageNameEdit_ = new QLineEdit(central);
        auto* addPageButton = new QPushButton("Додати сторінку", central);
        gridSizeCombo_ = new QComboBox(central);
        panelPositionCombo_ = new QComboBox(central);
        themeCombo_ = new QComboBox(central);

        for (int size = MIN_GRID_SIZE; size <= MAX_GRID_SIZE; ++size)
        {
            gridSizeCombo_->addItem(QString("%1 x %1").arg(size), size);
        }
        panelPositionCombo_->addItems(PanelPositionLabels());
        themeCombo_->addItems(ThemeLabels());

        top->addWidget(new QLabel("Сторінка:", central), 0, 0);
        top->addWidget(pageCombo_, 0, 1);
        top->addWidget(new QLabel("Назва:", central), 0, 2);
        top->addWidget(pageNameEdit_, 0, 3);
        top->addWidget(addPageButton, 0, 4);
        top->addWidget(new QLabel("Сітка:", central), 0, 5);
        top->addWidget(gridSizeCombo_, 0, 6);
        top->addWidget(new QLabel("Позиція:", central), 1, 0);
        top->addWidget(panelPositionCombo_, 1, 1);
        top->addWidget(new QLabel("Тема:", central), 1, 2);
        top->addWidget(themeCombo_, 1, 3);
        root->addLayout(top);

        table_ = new QTableWidget(MAX_BUTTON_COUNT, 6, central);
        table_->setHorizontalHeaderLabels({ "#", "Назва кнопки", "Тип", "Значення / посилання / шлях", "Зображення", "" });
        table_->verticalHeader()->hide();
        table_->horizontalHeader()->setStretchLastSection(false);
        table_->setColumnWidth(0, 42);
        table_->setColumnWidth(1, 155);
        table_->setColumnWidth(2, 115);
        table_->setColumnWidth(3, 300);
        table_->setColumnWidth(4, 250);
        table_->setColumnWidth(5, 130);

        editors_.resize(MAX_BUTTON_COUNT);
        for (int i = 0; i < MAX_BUTTON_COUNT; ++i)
        {
            auto* number = new QTableWidgetItem(QString::number(i + 1));
            number->setFlags(number->flags() & ~Qt::ItemIsEditable);
            table_->setItem(i, 0, number);

            editors_[i].title = new QLineEdit(table_);
            editors_[i].type = new QComboBox(table_);
            editors_[i].type->addItems(ActionNames());
            editors_[i].value = new QLineEdit(table_);
            editors_[i].image = new QLineEdit(table_);

            auto* buttonBox = new QWidget(table_);
            auto* buttonLayout = new QGridLayout(buttonBox);
            buttonLayout->setContentsMargins(0, 0, 0, 0);
            editors_[i].valueBrowse = new QPushButton("...", buttonBox);
            editors_[i].browse = new QPushButton("...", buttonBox);
            editors_[i].help = new QPushButton("?", buttonBox);
            buttonLayout->addWidget(editors_[i].valueBrowse, 0, 0);
            buttonLayout->addWidget(editors_[i].browse, 0, 1);
            buttonLayout->addWidget(editors_[i].help, 0, 2);

            table_->setCellWidget(i, 1, editors_[i].title);
            table_->setCellWidget(i, 2, editors_[i].type);
            table_->setCellWidget(i, 3, editors_[i].value);
            table_->setCellWidget(i, 4, editors_[i].image);
            table_->setCellWidget(i, 5, buttonBox);

            connect(editors_[i].valueBrowse, &QPushButton::clicked, this, [this, i] { browseButtonValue(i); });
            connect(editors_[i].browse, &QPushButton::clicked, this, [this, i] { browseButtonImage(i); });
            connect(editors_[i].help, &QPushButton::clicked, this, [this, i] { showHelpForAction(i); });
        }

        root->addWidget(table_, 1);

        auto* bottom = new QHBoxLayout();
        auto* importButton = new QPushButton("Імпорт сету", central);
        auto* exportButton = new QPushButton("Експорт сету", central);
        auto* saveButton = new QPushButton("Зберегти", central);
        auto* closeButton = new QPushButton("Закрити", central);
        auto* exitButton = new QPushButton("Вийти", central);
        bottom->addWidget(importButton);
        bottom->addWidget(exportButton);
        bottom->addStretch();
        bottom->addWidget(saveButton);
        bottom->addWidget(closeButton);
        bottom->addWidget(exitButton);
        root->addLayout(bottom);

        setCentralWidget(central);

        connect(pageCombo_, &QComboBox::currentIndexChanged, this, [this](int index) {
            if (loading_ || index < 0)
            {
                return;
            }
            collectEditorPage();
            g_editorPage = index;
            loadEditorPage();
        });
        connect(gridSizeCombo_, &QComboBox::currentIndexChanged, this, [this](int) {
            if (loading_)
            {
                return;
            }
            collectEditorPage();
            NormalizePages();
            loadEditorPage();
            panel_.resize(PanelWidth(), PanelHeight());
            panel_.update();
            if (g_visible)
            {
                panel_.showPanel();
            }
        });
        connect(themeCombo_, &QComboBox::currentIndexChanged, this, [this](int) {
            if (loading_)
            {
                return;
            }
            collectTheme();
            refreshStyle();
            panel_.update();
        });
        connect(addPageButton, &QPushButton::clicked, this, [this] { addEditorPage(); });
        connect(importButton, &QPushButton::clicked, this, [this] { importButtonSet(); });
        connect(exportButton, &QPushButton::clicked, this, [this] { exportButtonSet(); });
        connect(saveButton, &QPushButton::clicked, this, [this] { saveEditorConfig(); });
        connect(closeButton, &QPushButton::clicked, this, &QWidget::hide);
        connect(exitButton, &QPushButton::clicked, qApp, &QApplication::quit);
    }

    void collectPanelPosition()
    {
        g_panelPosition = static_cast<PanelPosition>(panelPositionCombo_->currentIndex());
    }

    void collectGridSize()
    {
        g_gridSize = ClampGridSize(gridSizeCombo_->currentData().toInt());
    }

    void collectTheme()
    {
        g_theme = static_cast<AppTheme>(themeCombo_->currentIndex());
    }

    void collectEditorPage()
    {
        if (g_editorPage < 0 || g_editorPage >= static_cast<int>(g_pages.size()))
        {
            return;
        }

        collectPanelPosition();
        collectGridSize();
        collectTheme();

        Page& page = g_pages[g_editorPage];
        page.name = ToWString(pageNameEdit_->text());
        for (int i = 0; i < MAX_BUTTON_COUNT; ++i)
        {
            ButtonAction& button = page.buttons[i];
            button.title = ToWString(editors_[i].title->text());
            button.type = ParseActionType(ToWString(editors_[i].type->currentText()));
            button.value = ToWString(editors_[i].value->text());
            button.imagePath = ToWString(editors_[i].image->text());
        }
    }

    void loadEditorPage()
    {
        if (g_pages.empty())
        {
            return;
        }

        loading_ = true;
        g_editorPage = std::clamp(g_editorPage, 0, static_cast<int>(g_pages.size()) - 1);

        pageCombo_->clear();
        for (const Page& page : g_pages)
        {
            pageCombo_->addItem(ToQString(page.name));
        }
        pageCombo_->setCurrentIndex(g_editorPage);
        pageNameEdit_->setText(ToQString(g_pages[g_editorPage].name));
        panelPositionCombo_->setCurrentIndex(static_cast<int>(g_panelPosition));
        gridSizeCombo_->setCurrentIndex(g_gridSize - MIN_GRID_SIZE);
        themeCombo_->setCurrentIndex(static_cast<int>(g_theme));

        const Page& page = g_pages[g_editorPage];
        for (int i = 0; i < MAX_BUTTON_COUNT; ++i)
        {
            const ButtonAction& button = page.buttons[i];
            editors_[i].title->setText(ToQString(button.title));
            editors_[i].type->setCurrentText(ToQString(ActionTypeToString(button.type)));
            editors_[i].value->setText(ToQString(button.value));
            editors_[i].image->setText(ToQString(button.imagePath));
            table_->setRowHidden(i, i >= ButtonCount());
        }

        loading_ = false;
    }

    void addEditorPage()
    {
        collectEditorPage();
        Page page;
        page.name = L"Нова сторінка";
        page.buttons.resize(MAX_BUTTON_COUNT);
        page.buttons[ButtonCount() - 1] = { L"Next", ActionType::NextPage, L"0", L"" };
        g_pages.push_back(page);
        g_editorPage = static_cast<int>(g_pages.size()) - 1;
        loadEditorPage();
    }

    void saveEditorConfig()
    {
        collectEditorPage();
        NormalizePages();
        if (g_currentPage >= static_cast<int>(g_pages.size()))
        {
            g_currentPage = 0;
        }

        if (!SaveButtons())
        {
            QMessageBox::critical(this, "Virtual Stream Deck", "Не вдалося зберегти data\\buttons.txt.");
            return;
        }

        refreshStyle();
        panel_.resize(PanelWidth(), PanelHeight());
        panel_.update();
        if (g_visible)
        {
            panel_.showPanel();
        }
        Information(this, "Налаштування збережено.");
    }

    void importButtonSet()
    {
        const QString path = QFileDialog::getOpenFileName(this, "Імпорт сету кнопок", QString(), "Virtual Stream Deck sets (*.txt);;All files (*.*)");
        if (path.isEmpty())
        {
            return;
        }

        if (!LoadButtonsFromFile(ToWString(path)))
        {
            QMessageBox::critical(this, "Virtual Stream Deck", "Не вдалося імпортувати сет. Перевірте формат txt-файлу.");
            return;
        }

        g_currentPage = 0;
        g_editorPage = 0;
        refreshStyle();
        loadEditorPage();
        panel_.resize(PanelWidth(), PanelHeight());
        panel_.update();

        if (!SaveButtons())
        {
            QMessageBox::warning(this, "Virtual Stream Deck", "Сет імпортовано, але не вдалося зберегти його як поточний data\\buttons.txt.");
            return;
        }

        Information(this, "Сет імпортовано і застосовано.");
    }

    void exportButtonSet()
    {
        collectEditorPage();
        NormalizePages();

        const QString path = QFileDialog::getSaveFileName(this, "Експорт сету кнопок", QString(), "Virtual Stream Deck sets (*.txt);;All files (*.*)");
        if (path.isEmpty())
        {
            return;
        }

        if (!SaveButtonsToFile(ToWString(path)))
        {
            QMessageBox::critical(this, "Virtual Stream Deck", "Не вдалося експортувати сет у вибраний файл.");
            return;
        }

        Information(this, "Сет експортовано. Цим txt-файлом можна поділитися.");
    }

    void showHelpForAction(int index)
    {
        const ActionType type = ParseActionType(ToWString(editors_[index].type->currentText()));
        QString message;
        switch (type)
        {
        case ActionType::Website:
            message = "Для website введіть повну адресу сайту, наприклад:\nhttps://youtube.com";
            break;
        case ActionType::Program:
            message = "Для program введіть команду або шлях до exe:\nnotepad.exe\nC:\\Program Files\\App\\app.exe";
            break;
        case ActionType::Folder:
            message = "Для folder введіть шлях до папки:\n%USERPROFILE%\\Documents\nD:\\Projects";
            break;
        case ActionType::Hotkey:
            message = "Для hotkey введіть комбінацію через '+':\nctrl+c\nctrl+shift+esc\nalt+tab";
            break;
        case ActionType::Text:
            message = "Для text введіть текст, який потрібно надрукувати через SendInput.";
            break;
        case ActionType::NextPage:
            message = "Для next_page введіть номер сторінки з нуля:\n0 - перша сторінка\n1 - друга сторінка";
            break;
        case ActionType::None:
        default:
            message = "none вимикає кнопку. Поле значення можна залишити порожнім.";
            break;
        }

        message += "\n\nЗображення: вкажіть шлях до PNG, JPG, BMP або ICO. Можна натиснути '...' для вибору файлу.";
        QMessageBox::information(this, "Підказка для кнопки", message);
    }

    void browseButtonImage(int index)
    {
        const QString path = QFileDialog::getOpenFileName(this, "Оберіть зображення для кнопки", QString(),
            "Images (*.png *.jpg *.jpeg *.bmp *.ico);;All files (*.*)");
        if (!path.isEmpty())
        {
            editors_[index].image->setText(path);
        }
    }

    void browseButtonValue(int index)
    {
        const ActionType type = ParseActionType(ToWString(editors_[index].type->currentText()));
        if (type == ActionType::Program)
        {
            const QString path = QFileDialog::getOpenFileName(this, "Оберіть програму або файл", QString(), "Programs (*.exe);;All files (*.*)");
            if (!path.isEmpty())
            {
                editors_[index].value->setText(path);
            }
            return;
        }

        if (type == ActionType::Folder)
        {
            const QString path = QFileDialog::getExistingDirectory(this, "Оберіть папку");
            if (!path.isEmpty())
            {
                editors_[index].value->setText(path);
            }
            return;
        }

        Information(this, "Вибір через Провідник доступний для типів program і folder.\nДля зображення використовуйте кнопку '...' у колонці 'Зображення'.");
    }

    struct ButtonEditorWidgets
    {
        QLineEdit* title = nullptr;
        QComboBox* type = nullptr;
        QLineEdit* value = nullptr;
        QLineEdit* image = nullptr;
        QPushButton* valueBrowse = nullptr;
        QPushButton* browse = nullptr;
        QPushButton* help = nullptr;
    };

    PanelWindow& panel_;
    QComboBox* pageCombo_ = nullptr;
    QLineEdit* pageNameEdit_ = nullptr;
    QComboBox* panelPositionCombo_ = nullptr;
    QComboBox* gridSizeCombo_ = nullptr;
    QComboBox* themeCombo_ = nullptr;
    QTableWidget* table_ = nullptr;
    std::vector<ButtonEditorWidgets> editors_;
    bool loading_ = false;
};

void PanelWindow::openEditor()
{
    if (editor_)
    {
        editor_->openForCurrentPage();
    }
}

class QtHotkeyFilter final : public QAbstractNativeEventFilter
{
public:
    QtHotkeyFilter(PanelWindow& panel, EditorWindow& editor)
        : panel_(panel), editor_(editor)
    {
    }

    bool nativeEventFilter(const QByteArray&, void* message, qintptr*) override
    {
        MSG* msg = static_cast<MSG*>(message);
        if (!msg || msg->message != WM_HOTKEY)
        {
            return false;
        }

        if (msg->wParam == HOTKEY_ID)
        {
            panel_.handlePanelHotkey();
            return true;
        }

        if (msg->wParam == EDIT_HOTKEY_ID)
        {
            editor_.openForCurrentPage();
            return true;
        }

        return false;
    }

private:
    PanelWindow& panel_;
    EditorWindow& editor_;
};

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    app.setApplicationName("Virtual Stream Deck");

    LoadButtons();
    NormalizePages();
    RefreshEditorBrushes();

    PanelWindow panel;
    g_hWnd = reinterpret_cast<HWND>(panel.winId());

    EditorWindow editor(panel);
    panel.setEditor(&editor);
    QtHotkeyFilter hotkeyFilter(panel, editor);
    app.installNativeEventFilter(&hotkeyFilter);

    if (!RegisterHotKey(g_hWnd, HOTKEY_ID, HOTKEY_MODIFIERS, HOTKEY_TOGGLE_KEY))
    {
        QMessageBox::warning(nullptr, "Virtual Stream Deck", "Не вдалося зареєструвати Ctrl + Alt + Space.");
    }
    if (!RegisterHotKey(g_hWnd, EDIT_HOTKEY_ID, HOTKEY_MODIFIERS, HOTKEY_EDITOR_KEY))
    {
        QMessageBox::warning(nullptr, "Virtual Stream Deck", "Не вдалося зареєструвати Ctrl + Alt + E для редактора.");
    }

    QSystemTrayIcon tray;
    QIcon trayIcon("kursova.ico");
    if (trayIcon.isNull())
    {
        trayIcon = QApplication::style()->standardIcon(QStyle::SP_ComputerIcon);
    }
    tray.setIcon(trayIcon);
    tray.setToolTip("Virtual Stream Deck");

    QMenu menu;
    QAction* showAction = menu.addAction("Показати панель");
    QAction* settingsAction = menu.addAction("Налаштування");
    menu.addSeparator();
    QAction* exitAction = menu.addAction("Вийти");
    tray.setContextMenu(&menu);

    QObject::connect(showAction, &QAction::triggered, &panel, [&panel] { panel.showPanel(); });
    QObject::connect(settingsAction, &QAction::triggered, &editor, [&editor] { editor.openForCurrentPage(); });
    QObject::connect(exitAction, &QAction::triggered, &app, &QApplication::quit);
    QObject::connect(&tray, &QSystemTrayIcon::activated, &panel, [&panel](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::DoubleClick)
        {
            panel.showPanel();
        }
    });
    tray.show();

    const int result = app.exec();
    UnregisterHotKey(g_hWnd, HOTKEY_ID);
    UnregisterHotKey(g_hWnd, EDIT_HOTKEY_ID);
    DeleteObject(g_editorBackgroundBrush);
    DeleteObject(g_editorControlBrush);
    return result;
}
