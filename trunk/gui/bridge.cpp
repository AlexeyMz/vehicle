#include <QtCore/QCryptographicHash>
#include <QtCore/QSettings>
#include <QtCore/QFile>

#include <QtWidgets/QApplication>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QDialog>
#include <QtWidgets/QLabel>

#include <QtQml/QQmlContext>
#include <QtQml/QQmlEngine>
#include <QtQml/QQml.h>

#include "middleware/solutionmodel.h"
#include "datamodel/AndOrTree.hpp"
#include "utils/xmlparser.h"
#include "bridge.h"

namespace vehicle {
using namespace middleware;
using namespace utils;
namespace core {

ModelQmlBridge::ModelQmlBridge(QObject* parent) : QObject(parent), parameterModel_(0)
{
    qmlRegisterUncreatableType<Parameter>("model.qml.bridge.utils", 1, 0, "Parameter", "Parameter::Type enum");
    qRegisterMetaType<ParameterModel*>("ParameterModel*");
    qRegisterMetaType<SolutionModel*>("SolutionModel*");
}

void ModelQmlBridge::initialize(QQmlEngine* engine)
{
    /*tree_ = new AOTree;
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
                             ->append(NodeKind::NONE, dec::decimal2(278446), NodeItem("Bang & Olufsen"))))))));*/

    QString home(qApp->applicationDirPath());

#ifdef _DEBUG
    std::cout << "Model source: " << home.toLocal8Bit().constData() << "/data.xml" << std::endl;
#endif


    tree_ = XmlParser::instance()->loadModel(home + "/data.xml");
    if(tree_ == nullptr)
        qFatal("%s", XmlParser::instance()->lastError().toLocal8Bit().constData());
    else
    {
        parameterModel_ = new ParameterModel(tree_, this);

        engine->rootContext()->setContextProperty("solutionModel", parameterModel_->solutionModel());
        engine->rootContext()->setContextProperty("parameterModel", parameterModel_);
        engine->rootContext()->setContextProperty("bridge", this);

        engine->rootContext()->setContextProperty("applicationDirUrl", QUrl::fromLocalFile(home));

#ifdef _DEBUG
        std::cout << "TREE:" << std::endl;
        std::cout << *tree_ << std::endl;
#endif
    }
}

QByteArray ModelQmlBridge::modelMD5Hash()
{
    QByteArray hash;

    QCryptographicHash cryp(QCryptographicHash::Md5);
    QFile file(qApp->applicationDirPath() + "/data.xml");
    if(file.open(QIODevice::ReadOnly))
    {
        cryp.addData(file.readAll());
        hash = cryp.result().toHex();
        file.close();
    }

    return hash;
}

bool ModelQmlBridge::login()
{
    QDialog* loginDialog = new QDialog;
    loginDialog->setWindowTitle(tr("Login"));
    loginDialog->setModal(true);

    QLineEdit* pwdEdit = new QLineEdit(loginDialog);
    pwdEdit->setEchoMode(QLineEdit::Password);
    pwdEdit->setClearButtonEnabled(true);

    QLabel* pwdLabel = new QLabel(tr("Password: "), loginDialog);
    pwdLabel->setBuddy(pwdEdit);

    QLabel* label = new QLabel(loginDialog);
    label->setText(tr("The access to the data edit mode is allowed only for staff. Please enter the password."));

    QPushButton* okButton = new QPushButton(tr("OK"), loginDialog);
    QPushButton* cancelButton = new QPushButton(tr("Cancel"), loginDialog);

    connect(cancelButton, SIGNAL(clicked()), loginDialog, SLOT(reject()));
    connect(okButton, &QPushButton::clicked, [=]()
    {
        QByteArray pwdHash = QCryptographicHash::hash(pwdEdit->text().toLocal8Bit(), QCryptographicHash::Md5);
        QSettings settings(QSettings::NativeFormat, QSettings::UserScope, "Vehicle", "Settings");
        QByteArray originalHash = settings.value("password", QCryptographicHash::hash("password", QCryptographicHash::Md5)).toByteArray();

        if(originalHash == pwdHash)
            loginDialog->accept();
        else
            QMessageBox::critical(0, tr("Error"), tr("Wrong password!"));
    });

    QHBoxLayout* pwdLayout = new QHBoxLayout;
    pwdLayout->addWidget(pwdLabel, 0);
    pwdLayout->addWidget(pwdEdit, 1);

    QHBoxLayout* buttonsLayout = new QHBoxLayout;
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(okButton);
    buttonsLayout->addWidget(cancelButton);

    QVBoxLayout* layout = new QVBoxLayout(loginDialog);
    layout->addWidget(label, 0);
    layout->addLayout(pwdLayout, 0);
    layout->addLayout(buttonsLayout, 1);

    pwdEdit->setFocus();
    loginDialog->setFixedSize(loginDialog->sizeHint());

    bool result = loginDialog->exec() == QDialog::Accepted;
    delete loginDialog;
    return result;
}

ModelQmlBridge::~ModelQmlBridge()
{
    delete parameterModel_;
}

}
}
