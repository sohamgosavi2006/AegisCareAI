#include "UiTheme.h"

#include <QApplication>
#include <QFile>

void UiTheme::apply(const QString& themeName) {
    QFile file(QStringLiteral(":/styles/dark_theme.qss"));
    QString sheet;
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        sheet = QString::fromUtf8(file.readAll());
    }

    if (themeName.startsWith(QStringLiteral("Light"))) {
        sheet += QStringLiteral(R"(
            QWidget { color: #17212B; }
            QMainWindow, QScrollArea, QStackedWidget { background-color: #F4F7FA; }
            QFrame#ModernCard { background-color: #FFFFFF; border-color: #D8E2EC; }
            QLabel { color: #17212B; }
            QLineEdit, QTextEdit, QComboBox, QSpinBox, QDateEdit {
                background-color: #FFFFFF; color: #17212B; border-color: #B7C6D4;
            }
            QTableWidget { background-color: #FFFFFF; color: #17212B; border-color: #CBD7E2; }
            QHeaderView::section { background-color: #E7EEF5; color: #273746; }
            QPushButton { background-color: #E7EEF5; color: #17212B; border-color: #B7C6D4; }
            QListWidget { color: #273746; }
        )");
    }
    qApp->setStyleSheet(sheet);
}
