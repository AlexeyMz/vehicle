#include <QtXml/QDomDocument>
#include <QtXml/QDomElement>
#include <QtCore/QFile>

#include "../bridge.h"
#include "xmlparser.h"

namespace vehicle {
using namespace middleware;
using namespace core;
namespace utils {

// Meyers' singleton is thread-safe in C++11
XmlParser* XmlParser::instance()
{
    static XmlParser instance_;
    return &instance_;
}

QString XmlParser::lastError() const
{
    return error_;
}

AOTree* XmlParser::loadModel(const QString& fileName)
{
    Q_ASSERT(!fileName.isEmpty());

    error_.clear();

    QFile file(fileName);
    if(file.open(QIODevice::ReadOnly))
    {
        int line, column;
        QDomDocument xml;
        if(xml.setContent(file.readAll(), &error_, &line, &column))
        {
            QDomElement vehiclemodel = xml.firstChildElement("vehicle-model");
            if(!vehiclemodel.isNull())
            {
                QDomElement andortree = vehiclemodel.firstChildElement("and-or-tree");
                if(!andortree.isNull())
                {
                    AOTree* tree = readMarks(&andortree);
                    if(tree == nullptr)
                        error_ = error_.arg(fileName);
                    return tree;
                }
                else
                    error_ = QObject::tr("The file %1 is not a correct (the second node must be an 'and-or-tree')").arg(fileName);
            }
            else
                error_ = QObject::tr("The file %1 is not a correct (the first node must be a 'vehicle-model')").arg(fileName);
        }
        else
        {
            error_.prepend(QObject::tr("Error loading %1: ").arg(fileName));
            error_.append(QString(" (%1:%2)").arg(line).arg(column));
        }

        file.close();
    }
    else
        error_ = QObject::tr("Cannot open file %1 (%2)").arg(fileName).arg(file.errorString());

    return nullptr;
}

AOTree* XmlParser::readMarks(QDomElement* andortree)
{
    Q_ASSERT(andortree);

    AOTree* tree = nullptr;

    QDomElement root = andortree->firstChildElement("node");
    if(!root.isNull() && root.attribute("type").compare("mark", Qt::CaseInsensitive) == 0)
    {
        QString rootName = root.attribute("name");
        if(!rootName.isEmpty())
        {
            QDomElement markElement = root.firstChildElement("node");
            if(!markElement.isNull())
            {
                while(!markElement.isNull())
                {
                    if(!tree)
                    {
                        tree = new AOTree;
                        tree->setRoot(tree->create(NodeKind::OR, decimal2(0), NodeItem(rootName.toStdString())));
                    }

                    if(markElement.attribute("type").compare("AND", Qt::CaseInsensitive) != 0)
                    {
                        error_ = QObject::tr("The file %1 is not a correct (children of a 'mark' node must have a type 'AND')");
                        break;
                    }

                    QString name = markElement.attribute("name");
                    if(name.isEmpty())
                    {
                        error_ = QObject::tr("The file %1 is not a correct (children of a 'mark' node must have a 'name' attribute)");
                        break;
                    }

                    AOTree::node_t* markNode = tree->create(NodeKind::AND, decimal2(0), NodeItem(name.toStdString()));
                    tree->getRoot()->attach(markNode);

                    if(!readModelElement(&markElement, tree, markNode))
                        break;

                    markElement = markElement.nextSiblingElement("node");
                }

                if(tree && !error_.isEmpty())
                {
                    delete tree;
                    tree = nullptr;
                }
            }
            else
                error_ = QObject::tr("The file %1 is not a correct (a node with a type 'mark' must a have at least one child)");
        }
        else
            error_ = QObject::tr("The file %1 is not a correct (a node with a type 'mark' must have a 'name' attribute)");
    }
    else
        error_ = QObject::tr("The file %1 is not a correct (the child of an 'and-or-tree' node must be a node with a type 'mark')");

    return tree;
}

bool XmlParser::readModelElement(QDomElement* markElement, AOTree* tree, AOTree::node_t* markNode)
{
    Q_ASSERT(markElement && tree && markNode);

    QDomElement modelElement = markElement->firstChildElement("node");
    if(!modelElement.isNull() && modelElement.attribute("type").compare("model", Qt::CaseInsensitive) == 0)
    {
        QString modelName = modelElement.attribute("name");
        if(!modelName.isEmpty())
        {
            AOTree::node_t* modelNode = tree->create(NodeKind::OR, decimal2(0), NodeItem(modelName.toStdString()));
            markNode->attach(modelNode);

            QDomElement specificModelElement = modelElement.firstChildElement("node");
            while(!specificModelElement.isNull())
            {
                NodeKind kind = NodeKind::NONE;
                if(specificModelElement.attribute("type").compare("AND", Qt::CaseInsensitive) == 0)
                    kind = NodeKind::AND;
                else if(specificModelElement.attribute("type").compare("OR", Qt::CaseInsensitive) == 0)
                    kind = NodeKind::OR;

                QString name = specificModelElement.attribute("name");
                if(name.isEmpty())
                {
                    error_ = QObject::tr("The file %1 is not a correct (node at line ") + QString::number(specificModelElement.lineNumber()) + QObject::tr(" must have a 'name' attribute)");
                    return false;
                }
                QString valueStr = specificModelElement.attribute("value");
                int value = 0;
                if(!valueStr.isEmpty())
                    value = valueStr.toInt();

                AOTree::node_t* specificModelNode = tree->create(kind, decimal2(value), NodeItem(name.toStdString()));
                modelNode->attach(specificModelNode);

                if(kind != NodeKind::NONE)
                    if(!readChildren(&specificModelElement, tree, specificModelNode))
                        return false;

                specificModelElement = specificModelElement.nextSiblingElement("node");
            }
            return true;
        }
        else
        {
            error_ = QObject::tr("The file %1 is not a correct ('model' node must have a 'name' attribute)");
            return false;
        }
    }
    else
    {
        error_ = QObject::tr("The file %1 is not a correct (the first child of each 'mark' node child must have a type 'model')");
        return false;
    }
}

bool XmlParser::readChildren(QDomElement* element, AOTree* tree, AOTree::node_t* parent)
{
    Q_ASSERT(element && tree && parent);

    QDomElement elementChild = element->firstChildElement("node");
    while(!elementChild.isNull())
    {
        NodeKind kind = NodeKind::NONE;
        if(elementChild.attribute("type").compare("AND", Qt::CaseInsensitive) == 0)
            kind = NodeKind::AND;
        else if(elementChild.attribute("type").compare("OR", Qt::CaseInsensitive) == 0)
            kind = NodeKind::OR;

        QString name = elementChild.attribute("name");
        if(name.isEmpty())
        {
            error_ = QObject::tr("The file %1 is not a correct (node at line ") + QString::number(elementChild.lineNumber()) + QObject::tr(" must have a 'name' attribute)");
            return false;
        }
        QString valueStr = elementChild.attribute("value");
        int value = 0;
        if(!valueStr.isEmpty())
            value = valueStr.toInt();

        AOTree::node_t* nodeChild = tree->create(kind, decimal2(value), NodeItem(name.toStdString()));
        parent->attach(nodeChild);

        if(kind != NodeKind::NONE)
            if(!readChildren(&elementChild, tree, nodeChild))
                return false;

        elementChild = elementChild.nextSiblingElement("node");
    }

    return true;
}

bool XmlParser::saveModel(AOTree* model, const QString& fileName)
{
    Q_ASSERT(model);
    Q_ASSERT(!fileName.isEmpty());

    error_.clear();

    static QMap<NodeKind,QString> kindToStr;
    kindToStr[NodeKind::NONE] = "NONE";
    kindToStr[NodeKind::AND] = "AND";
    kindToStr[NodeKind::OR] = "OR";

    static std::function<void(AOTree::node_t*,QDomDocument*,QDomElement*)> expandNode =
    [](AOTree::node_t* parentNode, QDomDocument* doc, QDomElement* parentElement)
    {
        for(size_t i = 0; i < parentNode->childCount(); ++i)
        {
            auto child = parentNode->child(i);
            QDomElement childElement = doc->createElement("node");
            childElement.setAttribute("name", child->getValue().name().c_str());
            childElement.setAttribute("type", kindToStr.value(child->getKind()));
            childElement.setAttribute("value", child->ownKey().getAsInteger());

            expandNode(child, doc, &childElement);
            parentElement->appendChild(childElement);
        }
    };

    QFile file(fileName);
    if(file.open(QIODevice::WriteOnly))
    {
        QDomDocument xml;

        QDomElement vehiclemodel = xml.createElement("vehicle-model");
        QDomElement andortree = xml.createElement("and-or-tree");

        AOTree::node_t* root = model->getRoot();
        Q_ASSERT(root);

        QDomElement markElement = xml.createElement("node");
        markElement.setAttribute("name", root->getValue().name().c_str());
        markElement.setAttribute("type", "mark");

        for(size_t i = 0; i < root->childCount(); ++i)
        {
            auto childI = root->child(i);
            QDomElement specificMarkElement = xml.createElement("node");
            specificMarkElement.setAttribute("name", childI->getValue().name().c_str());
            specificMarkElement.setAttribute("type", kindToStr.value(childI->getKind()));

            for(size_t j = 0; j < childI->childCount(); ++j)
            {
                auto childIJ = childI->child(j);
                QDomElement modelElement = xml.createElement("node");
                modelElement.setAttribute("name", childIJ->getValue().name().c_str());
                modelElement.setAttribute("type", "model");

                for(size_t k = 0; k < childIJ->childCount(); ++k)
                {
                    auto childIJK = childIJ->child(k);
                    QDomElement specificModelElement = xml.createElement("node");
                    specificModelElement.setAttribute("name", childIJK->getValue().name().c_str());
                    specificModelElement.setAttribute("type", kindToStr.value(childIJK->getKind()));
                    specificModelElement.setAttribute("value", childIJK->ownKey().getAsInteger());

                    expandNode(childIJK, &xml, &specificModelElement);
                    modelElement.appendChild(specificModelElement);
                }

                specificMarkElement.appendChild(modelElement);
            }

            markElement.appendChild(specificMarkElement);
        }

        andortree.appendChild(markElement);
        vehiclemodel.appendChild(andortree);
        xml.appendChild(vehiclemodel);

        file.write(xml.toByteArray());
        file.close();
    }
    else
        error_ = QObject::tr("Cannot open file %1 for writing (%2)").arg(fileName).arg(file.errorString());

    return error_.isEmpty();
}

SolutionModel* XmlParser::loadSolutions(const QString& fileName, bool* isOutdated)
{
    Q_ASSERT(!fileName.isEmpty());

    error_.clear();

    QFile file(fileName);
    if(file.open(QIODevice::ReadOnly))
    {
        int line, column;
        QDomDocument xml;
        if(xml.setContent(file.readAll(), &error_, &line, &column))
        {
            QDomElement vehiclesolutions = xml.firstChildElement("vehicle-solutions");
            if(!vehiclesolutions.isNull())
            {
                QString treeHash = vehiclesolutions.attribute("tree");
                if(!treeHash.isEmpty())
                {
                    if(isOutdated)
                    {
                        if(treeHash.compare(ModelQmlBridge::modelMD5Hash().constData()) != 0)
                            *isOutdated = true;
                        else
                            *isOutdated = false;
                    }

                    SolutionModel* solutionModel = nullptr;
                    QDomElement solution = vehiclesolutions.firstChildElement("solution");
                    while(!solution.isNull())
                    {
                        if(!solutionModel)
                            solutionModel = new SolutionModel;

                        QDomElement fullDescrElement = solution.firstChildElement("full-description");
                        if(fullDescrElement.isNull())
                        {
                            error_ = QObject::tr("The file %1 is not a correct ('solution' node at line %2 must contains a 'full-description' child')").arg(fileName).arg(solution.lineNumber());
                            break;
                        }
                        QStringList fullDescr = fullDescrElement.attribute("value").split(";;");
                        if(fullDescr.isEmpty())
                        {
                            error_ = QObject::tr("The file %1 is not a correct ('full-description' element at line %2 must contains a 'value' attribute')").arg(fileName).arg(fullDescrElement.lineNumber());
                            break;
                        }
                        QDomElement shortDescrElement = solution.firstChildElement("short-description");
                        if(shortDescrElement.isNull())
                        {
                            error_ = QObject::tr("The file %1 is not a correct ('solution' node at line %2 must contains a 'short-description' child')").arg(fileName).arg(solution.lineNumber());
                            break;
                        }
                        QString shortDescr = shortDescrElement.attribute("value");
                        if(shortDescr.isEmpty())
                        {
                            error_ = QObject::tr("The file %1 is not a correct ('short-description' element at line %2 must contains a 'value' attribute')").arg(fileName).arg(shortDescrElement.lineNumber());
                            break;
                        }
                        QDomElement priceElement = solution.firstChildElement("price");
                        if(priceElement.isNull())
                        {
                            error_ = QObject::tr("The file %1 is not a correct ('solution' node at line %2 must contains a 'price' child')").arg(fileName).arg(solution.lineNumber());
                            break;
                        }
                        QString priceStr = priceElement.attribute("value");
                        if(priceStr.isEmpty())
                        {
                            error_ = QObject::tr("The file %1 is not a correct ('price' element at line %2 must contains a 'value' attribute')").arg(fileName).arg(priceElement.lineNumber());
                            break;
                        }
                        bool ok = false;
                        int price = priceStr.toInt(&ok);
                        if(!ok)
                        {
                            error_ = QObject::tr("The file %1 is not a correct (a 'price' attribute of node at line %2 must contains a number)").arg(fileName).arg(solution.lineNumber());
                            break;
                        }
                        QDomElement modelElement = solution.firstChildElement("model");
                        if(modelElement.isNull())
                        {
                            error_ = QObject::tr("The file %1 is not a correct ('solution' node at line %2 must contains a 'model' child')").arg(fileName).arg(solution.lineNumber());
                            break;
                        }
                        QString model = modelElement.attribute("value");
                        if(model.isEmpty())
                        {
                            error_ = QObject::tr("The file %1 is not a correct ('model' element at line %2 must contains a 'value' attribute')").arg(fileName).arg(modelElement.lineNumber());
                            break;
                        }
                        QDomElement markElement = solution.firstChildElement("mark");
                        if(markElement.isNull())
                        {
                            error_ = QObject::tr("The file %1 is not a correct ('solution' node at line %2 must contains a 'mark' child')").arg(fileName).arg(solution.lineNumber());
                            break;
                        }
                        QString mark = markElement.attribute("value");
                        if(mark.isEmpty())
                        {
                            error_ = QObject::tr("The file %1 is not a correct ('mark' element at line %2 must contains a 'value' attribute')").arg(fileName).arg(markElement.lineNumber());
                            break;
                        }
                        QDomElement hashElement = solution.firstChildElement("hash");
                        if(hashElement.isNull())
                        {
                            error_ = QObject::tr("The file %1 is not a correct ('solution' node at line %2 must contains a 'hash' child')").arg(fileName).arg(solution.lineNumber());
                            break;
                        }
                        QString hash = hashElement.attribute("value");
                        if(hash.isEmpty())
                        {
                            error_ = QObject::tr("The file %1 is not a correct ('hash' element at line %2 must contains a 'value' attribute')").arg(fileName).arg(hashElement.lineNumber());
                            break;
                        }

                        solution = solution.nextSiblingElement("solution");

                        internal::SolutionInitializer data = { fullDescr, shortDescr, decimal2(price), hash.toLocal8Bit(), model, mark };
                        Solution* s = new Solution(qMove(data));
                        solutionModel->addSolution(s);
                    }

                    if(!error_.isEmpty())
                        delete solutionModel;
                    else
                        return solutionModel;
                }
                else
                    error_ = QObject::tr("The file %1 is not a correct ('vehicle-solutions' node must contains a 'tree' attribute')").arg(fileName);
            }
            else
                error_ = QObject::tr("The file %1 is not a correct (the first node must be a 'vehicle-solutions')").arg(fileName);
        }
        else
            error_.append(QString(" (%1:%2)").arg(line).arg(column));

        file.close();
    }
    else
        error_ = QObject::tr("Cannot open file %1 (%2)").arg(fileName).arg(file.errorString());

    return nullptr;
}

bool XmlParser::saveSolutions(SolutionModel* smodel, const QString& fileName)
{
    Q_ASSERT(smodel);
    Q_ASSERT(!fileName.isEmpty());

    error_.clear();

    QFile file(fileName);
    if(file.open(QIODevice::WriteOnly))
    {
        QDomDocument xml;

        QDomElement vehiclesolutions = xml.createElement("vehicle-solutions");
        vehiclesolutions.setAttribute("tree", ModelQmlBridge::modelMD5Hash().constData());

        for(int i = 0; i < smodel->rowCount(); ++i)
        {
            QStringList fullDescr = smodel->data(smodel->index(i, 0), SolutionModel::FullDescriptionRole).toStringList();
            QString shortDescr = smodel->data(smodel->index(i, 0), SolutionModel::ShortDescriptionRole).toString();
            int price = smodel->data(smodel->index(i, 0), SolutionModel::PriceRole).toInt();
            QString model = smodel->data(smodel->index(i, 0), SolutionModel::ModelRole).toString();
            QString mark = smodel->data(smodel->index(i, 0), SolutionModel::MarkRole).toString();
            QString hash = smodel->data(smodel->index(i, 0), SolutionModel::HashRole).toString();

            QDomElement fullDescrElement = xml.createElement("full-description");
            fullDescrElement.setAttribute("value", fullDescr.join(";;"));
            QDomElement shortDescrElement = xml.createElement("short-description");
            shortDescrElement.setAttribute("value", shortDescr);
            QDomElement priceElement = xml.createElement("price");
            priceElement.setAttribute("value", QString::number(price));
            QDomElement modelElement = xml.createElement("model");
            modelElement.setAttribute("value", model);
            QDomElement markElement = xml.createElement("mark");
            markElement.setAttribute("value", mark);
            QDomElement hashElement = xml.createElement("hash");
            hashElement.setAttribute("value", hash);

            QDomElement solution = xml.createElement("solution");
            solution.appendChild(fullDescrElement);
            solution.appendChild(shortDescrElement);
            solution.appendChild(priceElement);
            solution.appendChild(modelElement);
            solution.appendChild(markElement);
            solution.appendChild(hashElement);

            vehiclesolutions.appendChild(solution);
        }

        xml.appendChild(vehiclesolutions);

        file.write(xml.toByteArray());
        file.close();
    }
    else
        error_ = QObject::tr("Cannot open file %1 for writing (%2)").arg(fileName).arg(file.errorString());

    return error_.isEmpty();
}

} // namespace utils
} // namespace vehicle
