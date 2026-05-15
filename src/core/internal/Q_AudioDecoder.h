#pragma once

#include <QObject>
#include <string>
#include <vector>

class Q_AudioDecoder : public QObject {
    Q_OBJECT
   public:
    explicit Q_AudioDecoder(QObject* parent = nullptr);

    std::vector<float> decodeToMono16kFloat(const std::string& path);
};
