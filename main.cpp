#include "mainwindow.h"
#include "DatabaseManager.h"

#include <QApplication>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Instantiate and start backend
    DatabaseManager dbManager;

    // Initialize database file, pragmas, and schema
    if (!dbManager.initDatabase()) {
        QMessageBox::critical(nullptr, "Database Error", "Could not initialize local data storage. The application will close.");
        return -1; // Exit app with error code
    }

    // Pass DatabaseManager reference into MainWindow
    MainWindow mainWindow(dbManager);
    mainWindow.show();

    // Run Qt event loop
    int result = app.exec();

    // When app.exev() exits, mainWindow is destroyed first,
    // followed by dbManager ( DatabaseManager destructor closes SQLite connection safely)
    return result;

}
