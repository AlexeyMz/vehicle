#pragma once

#include <QtCore/QObject>

#include "middleware/parametermodel.h"

class QQmlEngine;

namespace vehicle {
namespace core {

///
/// \class ModelQmlBridge
/// \brief "Мост" между моделями и QML.
/// При инициализации осуществляет загрузку данных из data.xml
/// (находящейся в папке с исполняемым файлом приложения),
/// инициализирует и добавляет в контекст QML через движок
/// QML модели решений и параметров
///
///
class ModelQmlBridge : public QObject
{
    Q_OBJECT

public:
    explicit ModelQmlBridge(QObject* parent);
    ~ModelQmlBridge();

    ///
    /// \brief Возвращает MD5-хэш модели данных (файла data.xml)
    ///
    static QByteArray modelMD5Hash();
    ///
    /// \brief Осуществляет базовую инициализую, добавляет
    /// через движок QML \p engine в контекст QML модели
    /// параметров и решений, себя самого (для доступа к
    /// функции login() из QML)
    /// \param engine - движок QML
    /// \param error - текст ошибки, если таковая имела место быть
    /// \return false, если инициализация не удалась (ошибка в \p error)
    ///
    bool initialize(QQmlEngine* engine, QString* error = 0);

    ///
    /// \brief Выдает приглашение на ввод пароля,
    /// MD5-хэш пароля хранится в реестре
    /// \note по-умолчанию пароль "password"
    ///
    Q_INVOKABLE static bool login();

private:
    middleware::ParameterModel* parameterModel_;
    middleware::AOTree* tree_;
};

} // namespace core
} // namespace vehicle
