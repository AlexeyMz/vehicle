import QtQuick 2.0

Item {
    id: iconButton

    width: img.width
    height: img.height

    property alias icon: img.source
    property alias size: img.sourceSize

    signal clicked

    Image {
        id: img
        antialiasing: true
        fillMode: Image.PreserveAspectFit
    }

    Behavior on opacity { NumberAnimation { duration: 100 } }

    MouseArea {
        id: mouseArea
        hoverEnabled: true
        anchors.fill: parent
        onPressed: parent.scale = 0.98
        onReleased: parent.scale = 1
        onClicked: parent.clicked()
        cursorShape: Qt.PointingHandCursor
    }

    states: [
        State {
            name: "disabled"
            when: enabled == false
            PropertyChanges { target: iconButton; opacity: 0.4; }
        },
        State {
            name: "hovered"
            when: mouseArea.containsMouse
            PropertyChanges { target: iconButton; opacity: 1; }
        },
        State {
            name: "normal"
            when: mouseArea.containsMouse === false
            PropertyChanges { target: iconButton; opacity: 0.8; }
        }
    ]
}
