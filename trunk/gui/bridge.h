#pragma once

#include <QtCore/QObject>

#include "middleware/parametermodel.h"

class QQmlEngine;

namespace vehicle {
namespace core {

class ModelQmlBridge : public QObject
{
    Q_OBJECT

public:
    explicit ModelQmlBridge(QObject* parent);
    ~ModelQmlBridge();

    static QByteArray modelMD5Hash();
    void initialize(QQmlEngine* engine);

    Q_INVOKABLE static bool login();

private:
    middleware::ParameterModel* parameterModel_;
    middleware::AOTree* tree_;
};

} // namespace core
} // namespace vehicle
