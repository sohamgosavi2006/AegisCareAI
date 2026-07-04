#include <QApplication>
#include <QFile>
#include <QDebug>
#include <QDir>
#include <QPointer>
#include <QSettings>
#include <QFont>
#include "ui/UiTheme.h"
#include "DatabaseManager.h"
#include "LoginWindow.h"
#include "MainWindow.h"

int main(int argc, char *argv[]) {
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QApplication app(argc, argv);
    app.setApplicationName("AegisCareAI");
    app.setOrganizationName("AegisCare");

    // Initialize Database
    DatabaseManager& db = DatabaseManager::instance();
    if (!db.init("aegiscare.db")) {
        qCritical() << "Fatal: Failed to initialize SQLite database.";
        return -1;
    }

    QFont appFont = app.font();
    appFont.setPointSizeF(qMax(10.0, appFont.pointSizeF()));
    app.setFont(appFont);
    QSettings().setValue("ui/theme", "Dark Medical Theme (Default)");
    UiTheme::apply("Dark Medical Theme (Default)");

    // Create and Show Login Window
    LoginWindow loginWindow;
    QPointer<MainWindow> mainWindow;

    QObject::connect(&loginWindow, &LoginWindow::loginSuccessful, [&loginWindow, &mainWindow](const QString& username, const QString& role, const QString& fullName) {
        qDebug() << "Successfully logged in. Instantiating central workstation panel.";
        
        MainWindow* mainWin = new MainWindow();
        mainWin->setOperatorInfo(username, role, fullName);
        mainWin->show();
        mainWindow = mainWin;
        loginWindow.hide();
    });

    loginWindow.show();

    const int result = app.exec();
    delete mainWindow.data();

    return result;
}
