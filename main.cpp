#include <QApplication>
#include <QFile>
#include <mpg123.h>
#include "GUI/MainWindow.h"

int main(int argc, char* argv[]) {
    // Initialize mpg123 library
    mpg123_init();
    
    // Create Qt application
    QApplication app(argc, argv);
    app.setApplicationName("Audio Editor");
    app.setApplicationVersion("1.0");
    app.setOrganizationName("AudioEditor");
    
    // Set application-wide style
    app.setStyle("Fusion");
    
    // Dark palette for Fusion style
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(30, 30, 30));
    darkPalette.setColor(QPalette::WindowText, QColor(224, 224, 224));
    darkPalette.setColor(QPalette::Base, QColor(45, 45, 45));
    darkPalette.setColor(QPalette::AlternateBase, QColor(37, 37, 37));
    darkPalette.setColor(QPalette::ToolTipBase, QColor(45, 45, 45));
    darkPalette.setColor(QPalette::ToolTipText, QColor(224, 224, 224));
    darkPalette.setColor(QPalette::Text, QColor(224, 224, 224));
    darkPalette.setColor(QPalette::Button, QColor(45, 45, 45));
    darkPalette.setColor(QPalette::ButtonText, QColor(224, 224, 224));
    darkPalette.setColor(QPalette::BrightText, QColor(255, 0, 0));
    darkPalette.setColor(QPalette::Link, QColor(0, 188, 212));
    darkPalette.setColor(QPalette::Highlight, QColor(0, 188, 212));
    darkPalette.setColor(QPalette::HighlightedText, QColor(0, 0, 0));
    app.setPalette(darkPalette);
    
    // Create and show main window
    MainWindow mainWindow;
    mainWindow.show();
    
    // Run application
    int result = app.exec();
    
    // Cleanup mpg123
    mpg123_exit();
    
    return result;
}