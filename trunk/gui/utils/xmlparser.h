#pragma once

#include <QtCore/QObject>

#include "../middleware/solutionmodel.h"

class QDomElement;

namespace vehicle {
namespace utils {

class XmlParser
{
public:
    static XmlParser* instance();

    ///
    /// \brief Возвращает описание последней возникшей ошибки
    ///
    QString lastError() const;

    ///
    /// \brief Конвертирует XML-файл в AOTree
    /// \param fileName - имя файла с XML представлением модели
    /// \return nullptr если загрузка не удалась (\see lastError())
    ///
    middleware::AOTree* loadModel(const QString& fileName);

    ///
    /// \brief Конвертирует AOTree в XML и сохраняет в указанный файл
    /// \param model - модель для сохранения
    /// \param fileName - имя файла, в который будет сохранено XML представление модели
    /// \return false если сохранение не удалось (\see lastError())
    ///
    bool saveModel(middleware::AOTree* model, const QString& fileName);

    ///
    /// \brief Загружает сохраненные решения из файла
    /// \param fileName - имя файла с XML представлением решений
    /// \param isOutdated - устанавливается в true, если загруженные решения были построены
    /// не по текущей модели данных, т.е. они устарели
    /// \return nullptr если загрузка не удалась (\see lastError())
    ///
    middleware::SolutionModel* loadSolutions(const QString& fileName, bool* isOutdated = 0);

    ///
    /// \brief Сохраняет решения в указанном файле в XML формате
    /// \param model - модель решений для сохранения
    /// \param fileName - имя файла, в который будут сохранены решения
    /// \return false если сохранение не удалось (\see lastError())
    ///
    bool saveSolutions(middleware::SolutionModel* model, const QString& fileName);

private:
    middleware::AOTree* readMarks(QDomElement* andortree);
    bool readModelElement(QDomElement* markElement, middleware::AOTree* tree, middleware::AOTree::node_t* markNode);
    bool readChildren(QDomElement* element, middleware::AOTree* tree, middleware::AOTree::node_t* parent);

    // Singleton routine
    XmlParser() {}
    XmlParser(const XmlParser&);
    XmlParser &operator=(const XmlParser&);
    ~XmlParser() {}
    // ---

    QString error_;
};

} // namespace utils
} // namespace vehicle
