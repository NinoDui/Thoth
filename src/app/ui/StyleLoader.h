#pragma once

#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QStringList>
#include <QTextStream>

class StyleLoader {
   public:
    static void attach(const QStringList& stylePaths = {
                           ":/qss/base/tokens.qss",
                           ":/qss/components/controls.qss",
                           ":/qss/components/playback.qss",
                           ":/qss/screens/main_window.qss",
                       }) {
        QString combinedStyle;
        for (const auto& stylePath : stylePaths) {
            combinedStyle += readStyle(stylePath);
            combinedStyle += "\n";
        }
        qApp->setStyleSheet(combinedStyle);
    }

   private:
    static QString readStyle(const QString& stylePath) {
        QFile file(stylePath);
        if (file.open(QFile::ReadOnly | QFile::Text)) {
            QTextStream stream(&file);
            QString style = stream.readAll();
            file.close();
            return style;
        } else {
            qCritical() << "Failed to load style file: " << stylePath;
        }
        return {};
    }
};
