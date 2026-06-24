/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.11.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMdiArea>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QMdiArea *mdiArea;
    QListWidget *listWidget;
    QScrollBar *verticalScrollBar;
    QScrollBar *verticalScrollBar_2;
    QMenuBar *menubar;
    QMenu *menuFIle;
    QMenu *menuEdit;
    QMenu *menuCompany;
    QMenu *menuWindows;
    QMenu *menuInvoice;
    QMenu *menuReports;
    QMenu *menuHelp;
    QStatusBar *statusbar;
    QDockWidget *dockWidget_2;
    QWidget *dockWidgetContents_3;
    QHBoxLayout *horizontalLayout;
    QPushButton *pushButton;
    QPushButton *pushButton_2;
    QPushButton *pushButton_3;
    QPushButton *pushButton_4;
    QPushButton *pushButton_5;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(846, 649);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        mdiArea = new QMdiArea(centralwidget);
        mdiArea->setObjectName("mdiArea");
        mdiArea->setGeometry(QRect(180, 10, 651, 531));
        listWidget = new QListWidget(centralwidget);
        listWidget->setObjectName("listWidget");
        listWidget->setGeometry(QRect(10, 10, 161, 192));
        verticalScrollBar = new QScrollBar(centralwidget);
        verticalScrollBar->setObjectName("verticalScrollBar");
        verticalScrollBar->setGeometry(QRect(160, 10, 16, 191));
        verticalScrollBar->setOrientation(Qt::Orientation::Vertical);
        verticalScrollBar_2 = new QScrollBar(centralwidget);
        verticalScrollBar_2->setObjectName("verticalScrollBar_2");
        verticalScrollBar_2->setGeometry(QRect(820, 10, 16, 531));
        verticalScrollBar_2->setOrientation(Qt::Orientation::Vertical);
        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName("menubar");
        menubar->setGeometry(QRect(0, 0, 846, 21));
        menuFIle = new QMenu(menubar);
        menuFIle->setObjectName("menuFIle");
        menuEdit = new QMenu(menubar);
        menuEdit->setObjectName("menuEdit");
        menuCompany = new QMenu(menubar);
        menuCompany->setObjectName("menuCompany");
        menuWindows = new QMenu(menubar);
        menuWindows->setObjectName("menuWindows");
        menuInvoice = new QMenu(menubar);
        menuInvoice->setObjectName("menuInvoice");
        menuReports = new QMenu(menubar);
        menuReports->setObjectName("menuReports");
        menuHelp = new QMenu(menubar);
        menuHelp->setObjectName("menuHelp");
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName("statusbar");
        MainWindow->setStatusBar(statusbar);
        dockWidget_2 = new QDockWidget(MainWindow);
        dockWidget_2->setObjectName("dockWidget_2");
        dockWidgetContents_3 = new QWidget();
        dockWidgetContents_3->setObjectName("dockWidgetContents_3");
        horizontalLayout = new QHBoxLayout(dockWidgetContents_3);
        horizontalLayout->setObjectName("horizontalLayout");
        pushButton = new QPushButton(dockWidgetContents_3);
        pushButton->setObjectName("pushButton");

        horizontalLayout->addWidget(pushButton);

        pushButton_2 = new QPushButton(dockWidgetContents_3);
        pushButton_2->setObjectName("pushButton_2");

        horizontalLayout->addWidget(pushButton_2);

        pushButton_3 = new QPushButton(dockWidgetContents_3);
        pushButton_3->setObjectName("pushButton_3");

        horizontalLayout->addWidget(pushButton_3);

        pushButton_4 = new QPushButton(dockWidgetContents_3);
        pushButton_4->setObjectName("pushButton_4");

        horizontalLayout->addWidget(pushButton_4);

        pushButton_5 = new QPushButton(dockWidgetContents_3);
        pushButton_5->setObjectName("pushButton_5");

        horizontalLayout->addWidget(pushButton_5);

        dockWidget_2->setWidget(dockWidgetContents_3);
        MainWindow->addDockWidget(Qt::DockWidgetArea::TopDockWidgetArea, dockWidget_2);

        menubar->addAction(menuFIle->menuAction());
        menubar->addAction(menuEdit->menuAction());
        menubar->addAction(menuCompany->menuAction());
        menubar->addAction(menuWindows->menuAction());
        menubar->addAction(menuInvoice->menuAction());
        menubar->addAction(menuReports->menuAction());
        menubar->addAction(menuHelp->menuAction());

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "MainWindow", nullptr));
        menuFIle->setTitle(QCoreApplication::translate("MainWindow", "FIle", nullptr));
        menuEdit->setTitle(QCoreApplication::translate("MainWindow", "Edit", nullptr));
        menuCompany->setTitle(QCoreApplication::translate("MainWindow", "Company", nullptr));
        menuWindows->setTitle(QCoreApplication::translate("MainWindow", "Windows", nullptr));
        menuInvoice->setTitle(QCoreApplication::translate("MainWindow", "Invoice", nullptr));
        menuReports->setTitle(QCoreApplication::translate("MainWindow", "Reports", nullptr));
        menuHelp->setTitle(QCoreApplication::translate("MainWindow", "Help", nullptr));
        pushButton->setText(QCoreApplication::translate("MainWindow", "Create Invoice", nullptr));
        pushButton_2->setText(QCoreApplication::translate("MainWindow", "Payment", nullptr));
        pushButton_3->setText(QCoreApplication::translate("MainWindow", "Statements", nullptr));
        pushButton_4->setText(QCoreApplication::translate("MainWindow", "Reports", nullptr));
        pushButton_5->setText(QCoreApplication::translate("MainWindow", "Customers", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
