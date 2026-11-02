#include <QApplication>
#include <QMessageBox>
#include "lms/common.hpp"
#include "lms/ui/login_dialog.hpp"
#include "lms/ui/main_window.hpp"

#include <QCoreApplication>
#include <gtest/gtest.h>

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);

    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}


int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName("Moderated LMS");
    app.setStyleSheet(R"(
    QWidget {
        color: #000000;
        background-color: #FFFFFF;
        font-family: "DejaVu Sans";
        font-size: 10pt;
    }

    QMainWindow, QWidget#centralWidget {
        background-color: #FFFFFF;
        color: #000000;
    }

    QLabel {
        color: #000000;
        background-color: transparent;
    }

    QPushButton {
        color: #000000;
        background-color: #FFFFFF;
        border: 1px solid #000000;
        border-radius: 4px;
        padding: 8px 12px;
    }

    QPushButton:hover {
        background-color: #EEEEEE;
    }

    QPushButton:pressed {
        background-color: #DDDDDD;
    }

    QLineEdit, QTextEdit, QPlainTextEdit,
    QComboBox, QSpinBox, QDoubleSpinBox,
    QDateEdit, QDateTimeEdit {
        color: #000000;
        background-color: #FFFFFF;
        border: 1px solid #999999;
        selection-color: #000000;
        selection-background-color: #DDDDDD;
        padding: 5px;
    }

    QComboBox QAbstractItemView {
        color: #000000;
        background-color: #FFFFFF;
        selection-color: #000000;
        selection-background-color: #DDDDDD;
    }

    QTabWidget::pane {
        background-color: #FFFFFF;
        border: 1px solid #999999;
    }

    QTabBar::tab {
        color: #000000;
        background-color: #FFFFFF;
        border: 1px solid #999999;
        padding: 8px 14px;
    }

    QTabBar::tab:selected {
        color: #000000;
        background-color: #EEEEEE;
        border-bottom: 2px solid #000000;
    }

    QTableWidget {
        color: #000000;
        background-color: #FFFFFF;
        alternate-background-color: #F7F7F7;
        gridline-color: #CCCCCC;
        selection-background-color: #DDDDDD;
        selection-color: #000000;
    }

    QTableWidget::item {
        color: #000000;
        background-color: transparent;
        padding: 5px;
    }

    QHeaderView::section {
        color: #000000;
        background-color: #EEEEEE;
        border: 1px solid #CCCCCC;
        padding: 7px;
        font-weight: bold;
    }

    QScrollBar:vertical,
    QScrollBar:horizontal {
        background-color: #FFFFFF;
    }

    QToolTip {
        color: #000000;
        background-color: #FFFFFF;
        border: 1px solid #000000;
    }
   )");


    if (!initDatabase()) {
        QMessageBox::critical(nullptr, "Database error",
            "Could not initialise SQLite database. Check write permissions and Qt SQLite driver.");
        return 1;
    }

    LoginDialog login;
    if (login.exec() != QDialog::Accepted) return 0;

    MainWindow window(login.userId, login.userRole, login.userName, login.username);
    window.show();
    return app.exec();
}
