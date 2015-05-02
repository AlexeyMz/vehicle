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
    explicit ModelQmlBridge(QQmlEngine* parent);
    ~ModelQmlBridge();

private:
    middleware::ParameterModel* parameterModel_;
    middleware::AOTree* tree_;
};

} // namespace core
} // namespace vehicle
