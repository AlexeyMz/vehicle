#pragma once

#include <QtWidgets/QTreeView>

namespace vehicle {
namespace middleware {

class TreeView : public QTreeView
{
	Q_OBJECT
public:
    TreeView(QWidget* parent = 0);
	void setModel(QAbstractItemModel* model);
private slots:
	void resizeToContents();
protected:
    void mousePressEvent(QMouseEvent* event);
};

}
}
