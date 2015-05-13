import QtQuick 2.2
import "../js/scrollbar.js" as Scroll

Item {
    id: container

    property int orientation: Qt.Vertical
    property var scrollArea

    property alias background: background.color

    property alias sliderColor: slider.color
    property alias sliderBorder: slider.border
    property alias sliderGradient: slider.gradient

    visible: height > Scroll.size()

    Rectangle { id: background; anchors.fill: parent; opacity: 0.3 }

    Rectangle {
        id: slider
        x: container.orientation == Qt.Vertical ? 2 : Scroll.position()
        width: container.orientation == Qt.Vertical ? container.width - 4 : Scroll.size()
        y: container.orientation == Qt.Vertical ? Scroll.position() : 2
        height: container.orientation == Qt.Vertical ? Scroll.size() : container.height - 4

        MouseArea {
            anchors.fill: parent
            drag.target: parent
            drag.axis: container.orientation == Qt.Vertical ? Drag.YAxis : Drag.XAxis
            drag.minimumY: 0
            drag.maximumY: container.height - slider.height
            drag.minimumX: 0
            drag.maximumX: container.width - slider.width

            onPositionChanged: {
                if (pressedButtons == Qt.LeftButton) {
                    scrollArea.contentY = (container.orientation == Qt.Vertical ?
                           (slider.y * scrollArea.contentHeight / container.height) :
                           (slider.x * scrollArea.contentWidth / container.width))
                }
            }
        }
    }

    states: State {
        name: "visible"
        when: container.orientation == Qt.Vertical ? scrollArea.movingVertically : scrollArea.movingHorizontally
        PropertyChanges { target: container; opacity: 1.0 }
    }

    transitions: Transition {
        from: "visible"; to: ""
        NumberAnimation { properties: "opacity"; duration: 600 }
    }
}
