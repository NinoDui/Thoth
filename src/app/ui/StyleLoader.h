#pragma once

#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QTextStream>

class StyleLoader {
   public:
    static void attach(const QString& stylePath = ":/qss/style.qss") {
        QFile file(stylePath);
        if (file.open(QFile::ReadOnly | QFile::Text)) {
            QTextStream stream(&file);
            qApp->setStyleSheet(stream.readAll());
            file.close();
        } else {
            qCritical() << "Failed to load style file: " << stylePath;
        }
    }
};
