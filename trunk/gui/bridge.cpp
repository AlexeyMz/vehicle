#include <QtQml/QQmlContext>
#include <QtQml/QQmlEngine>
#include <QtQml/QQml.h>

#include "middleware/solutionmodel.h"
#include "datamodel/AndOrTree.hpp"
#include "bridge.h"

namespace vehicle {
using namespace middleware;
namespace core {

ModelQmlBridge::ModelQmlBridge(QQmlEngine* parent) : QObject(parent)
{
    qmlRegisterUncreatableType<Parameter>("model.qml.bridge.utils", 1, 0, "Parameter", "Parameter::Type enum");
    qRegisterMetaType<ParameterModel*>("ParameterModel*");
    qRegisterMetaType<SolutionModel*>("SolutionModel*");

    // TODO : Данные должен предоставлять XML-парсер
    tree_ = new AOTree;
    tree_->setRoot(tree_->create(NodeKind::OR, dec::decimal2(0), NodeItem("Mark"))
         ->attach(tree_->create(NodeKind::AND, dec::decimal2(0), NodeItem("BMW"))
             ->attach(tree_->create(NodeKind::OR, dec::decimal2(0), NodeItem("Model"))
                 ->attach(tree_->create(NodeKind::AND, dec::decimal2(2900000), NodeItem("X6"))
                     ->attach(tree_->create(NodeKind::OR, dec::decimal2(0), NodeItem("Engine"))
                         ->attach(tree_->create(NodeKind::AND, dec::decimal2(0), NodeItem("Gas"))
                              ->attach(tree_->create(NodeKind::OR, dec::decimal2(0), NodeItem("Engine power"))
                                  ->append(NodeKind::NONE, dec::decimal2(503000), NodeItem("306 hp"))
                                  ->append(NodeKind::NONE, dec::decimal2(1248000), NodeItem("450 hp"))))
                         ->attach(tree_->create(NodeKind::AND, dec::decimal2(0), NodeItem("Diesel"))
                              ->attach(tree_->create(NodeKind::OR, dec::decimal2(0), NodeItem("Engine power"))
                                  ->append(NodeKind::NONE, dec::decimal2(541000), NodeItem("249 hp"))
                                  ->append(NodeKind::NONE, dec::decimal2(858000), NodeItem("313 hp"))
                                  ->append(NodeKind::NONE, dec::decimal2(1675000), NodeItem("381 hp")))))
                     ->attach(tree_->create(NodeKind::AND, dec::decimal2(0), NodeItem("Options"))
                         ->attach(tree_->create(NodeKind::OR, dec::decimal2(0), NodeItem("Electopackage"))
                              ->append(NodeKind::NONE, dec::decimal2(30000), NodeItem("Front power windows"))
                              ->append(NodeKind::NONE, dec::decimal2(210000), NodeItem("Full electropackage")))
                         ->attach(tree_->create(NodeKind::OR, dec::decimal2(0), NodeItem("Audio system"))
                              ->append(NodeKind::NONE, dec::decimal2(70000), NodeItem("Harman"))
                              ->append(NodeKind::NONE, dec::decimal2(190000), NodeItem("Bang & Olufsen")))))
                ->attach(tree_->create(NodeKind::AND, dec::decimal2(2300000), NodeItem("X5"))
                    ->attach(tree_->create(NodeKind::OR, dec::decimal2(0), NodeItem("Engine"))
                        ->attach(tree_->create(NodeKind::AND, dec::decimal2(0), NodeItem("Gas"))
                             ->attach(tree_->create(NodeKind::OR, dec::decimal2(0), NodeItem("Engine power"))
                                 ->append(NodeKind::NONE, dec::decimal2(410000), NodeItem("306 hp"))
                                 ->append(NodeKind::NONE, dec::decimal2(1102000), NodeItem("450 hp"))))
                        ->attach(tree_->create(NodeKind::AND, dec::decimal2(0), NodeItem("Diesel"))
                             ->attach(tree_->create(NodeKind::OR, dec::decimal2(0), NodeItem("Engine power"))
                                 ->append(NodeKind::NONE, dec::decimal2(525000), NodeItem("218 hp"))
                                 ->append(NodeKind::NONE, dec::decimal2(671000), NodeItem("249 hp"))
                                 ->append(NodeKind::NONE, dec::decimal2(786000), NodeItem("313 hp"))
                                 ->append(NodeKind::NONE, dec::decimal2(1259500), NodeItem("381 hp")))))
                    ->attach(tree_->create(NodeKind::AND, dec::decimal2(0), NodeItem("Options"))
                        ->attach(tree_->create(NodeKind::OR, dec::decimal2(0), NodeItem("TV"))
                                 ->append(NodeKind::NONE, dec::decimal2(81017), NodeItem("Yes"))
                                 ->append(NodeKind::NONE, dec::decimal2(0), NodeItem("No")))
                        ->attach(tree_->create(NodeKind::OR, dec::decimal2(0), NodeItem("Electopackage"))
                             ->append(NodeKind::NONE, dec::decimal2(35000), NodeItem("Front power windows"))
                             ->append(NodeKind::NONE, dec::decimal2(206700), NodeItem("Full electropackage")))
                        ->attach(tree_->create(NodeKind::OR, dec::decimal2(0), NodeItem("Audio system"))
                             ->append(NodeKind::NONE, dec::decimal2(34576), NodeItem("Hi-Fi Audio system"))
                             ->append(NodeKind::NONE, dec::decimal2(76000), NodeItem("Harman"))
                             ->append(NodeKind::NONE, dec::decimal2(278446), NodeItem("Bang & Olufsen"))))))));

    parameterModel_ = new ParameterModel(tree_, this);

    parent->rootContext()->setContextProperty("solutionModel", parameterModel_->solutionModel());
    parent->rootContext()->setContextProperty("parameterModel", parameterModel_);

#ifdef _DEBUG
    std::cout << "TREE:" << std::endl;
    std::cout << *tree_ << std::endl;
#endif
}

ModelQmlBridge::~ModelQmlBridge()
{
    delete parameterModel_;
}

}
}
