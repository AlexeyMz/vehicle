#include <QtWidgets/QInputDialog>
#include <QtWidgets/QAction>
#include <QtWidgets/QMenu>

#include <QtGui/QMouseEvent>

#include "treemodel.h"
#include "treeview.h"

namespace vehicle {
using namespace core;
namespace middleware {

TreeView::TreeView(QWidget* parent) : QTreeView(parent)
{
    setItemDelegate(new TreeItemDelegate(this));
    setExpandsOnDoubleClick(false);
    setAllColumnsShowFocus(false);
    setUniformRowHeights(true);
    setRootIsDecorated(true);
    setHeaderHidden(true);
    setAnimated(true);
}

void TreeView::setModel(QAbstractItemModel* model)
{
	connect(model, SIGNAL(dataChanged(QModelIndex,QModelIndex,QVector<int>)), this, SLOT(resizeToContents()));
	connect(model, SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(resizeToContents()));
	QTreeView::setModel(model);
}

void TreeView::resizeToContents()
{
	for(int i = 0; i < model()->rowCount(); ++i)
	{
		resizeColumnToContents(i);
		setColumnWidth(i, columnWidth(i) + 5);
	}
}

void TreeView::mousePressEvent(QMouseEvent* event)
{
    QTreeView::mousePressEvent(event);

    if(event->button() == Qt::RightButton)
    {
        QModelIndex index = indexAt(event->pos());
        if(index.isValid() && index.column() == 0)
        {
            QMenu* menu = new QMenu;
            connect(menu, SIGNAL(aboutToHide()), menu, SLOT(deleteLater()));

            if(model()->rowCount(index) > 1)
            {
                NodeKind kind = static_cast<NodeKind>(model()->data(index, TreeItem::Kind).toInt());
                if(kind == NodeKind::AND || kind == NodeKind::OR)
                {
                    QAction* setKindAction = new QAction(tr("Change the node kind"), menu);
                    connect(setKindAction, &QAction::triggered, [=]()
                    {
                        QStringList items;
                        items << tr("AND") << tr("OR");

                        int current;
                        if(kind == NodeKind::AND)
                            current = 0;
                        else if(kind == NodeKind::OR)
                            current = 1;

                        bool ok;
                        QString item = QInputDialog::getItem(0, tr("Change the node kind"), tr("Choose the node kind"), items, current, false, &ok);

                        if(ok && !item.isEmpty())
                        {
                            NodeKind kind = NodeKind::NONE;
                            if(item == items.at(0))
                                kind = NodeKind::AND;
                            else if(item == items.at(1))
                                kind = NodeKind::OR;

                            Q_ASSERT(kind != NodeKind::NONE);
                            model()->setData(index, static_cast<int>(kind), TreeItem::Kind);
                        }
                    });
                    menu->addAction(setKindAction);
                }
            }

            QAction* addChildAction = new QAction(QIcon(":/gui/images/add.png"), tr("Add child node"), menu);
            connect(addChildAction, &QAction::triggered, [=]()
            {
                model()->insertRow(model()->rowCount(index), index);
            });
            menu->addAction(addChildAction);

            QAction* removeNodeAction = new QAction(QIcon(":/gui/images/remove.png"), tr("Remove node"), menu);
            connect(removeNodeAction, &QAction::triggered, [=]()
            {
                model()->removeRow(index.row(), index.parent());
            });
            menu->addAction(removeNodeAction);

            if(menu->actions().size())
                menu->popup(event->globalPos());
            else
                delete menu;

        }
    }
}

}
}
