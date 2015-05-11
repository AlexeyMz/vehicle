#pragma once

#include <QtWidgets/QTreeView>

namespace vehicle {
namespace middleware {

class TreeView : public QTreeView
{
public:
    TreeView(QWidget* parent = 0);
protected:
    void mousePressEvent(QMouseEvent* event);
};

}
}
