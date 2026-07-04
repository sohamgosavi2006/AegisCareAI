#include "../src/core/DatabaseManager.h"
#include "../src/ui/LoginWindow.h"

#include <QApplication>
#include <QEventLoop>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QTemporaryDir>
#include <QTimer>

namespace {

void waitForEvents(int milliseconds) {
    QEventLoop loop;
    QTimer::singleShot(milliseconds, &loop, &QEventLoop::quit);
    loop.exec();
}

QPushButton* findButton(LoginWindow& window, const QString& text) {
    for (QPushButton* button : window.findChildren<QPushButton*>()) {
        if (button->text() == text) return button;
    }
    return nullptr;
}

} // namespace

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("AegisCareUiTest"));
    app.setOrganizationName(QStringLiteral("AegisCare"));

    QTemporaryDir directory;
    if (!directory.isValid() ||
        !DatabaseManager::instance().init(directory.filePath(QStringLiteral("login-test.db")))) {
        return 1;
    }

    int loginSignals = 0;
    LoginWindow operatorLogin;
    QObject::connect(&operatorLogin, &LoginWindow::loginSuccessful,
                     [&loginSignals]() { ++loginSignals; });
    operatorLogin.show();
    QPushButton* loginButton = findButton(operatorLogin, QStringLiteral("INITIALIZE SESSION"));
    if (!loginButton) return 2;
    loginButton->click();
    loginButton->click();
    waitForEvents(650);
    if (loginSignals != 1) return 3;

    int adminSignals = 0;
    LoginWindow adminLogin;
    QObject::connect(&adminLogin, &LoginWindow::loginSuccessful,
                     [&adminSignals]() { ++adminSignals; });
    adminLogin.show();
    const auto edits = adminLogin.findChildren<QLineEdit*>();
    QComboBox* role = adminLogin.findChild<QComboBox*>();
    if (edits.size() < 2 || !role) return 4;
    edits[0]->setText("admin");
    edits[1]->setText("admin");
    role->setCurrentText("Administrator");
    QPushButton* adminButton = findButton(adminLogin, QStringLiteral("INITIALIZE SESSION"));
    if (!adminButton) return 4;
    adminButton->click();
    adminButton->click();
    waitForEvents(650);
    return adminSignals == 1 ? 0 : 5;
}
