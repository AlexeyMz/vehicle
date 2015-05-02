import QtQuick 2.2
import QtQuick.Layouts 1.1
import QtQuick.Controls 1.1

// м.б. использовать XmlListModel?
/*
 XmlListModel {
     id: feedModel
     source: "http://rss.news.yahoo.com/rss/oceania"
     query: "/rss/channel/item"
     XmlRole { name: "title"; query: "title/string()" }
     XmlRole { name: "link"; query: "link/string()" }
     XmlRole { name: "description"; query: "description/string()" }
 }
 */

ApplicationWindow {
    id: window
    title: qsTr("Vehicle")

    minimumWidth: 800
    minimumHeight: 600

    visible: true

    Item {
        id: windowAnchors
        anchors.fill: parent
    }

    SolutionsView {
        id: solutions
        anchors.left: windowAnchors.left
        anchors.top: windowAnchors.top
        anchors.bottom: windowAnchors.bottom
        anchors.right: parameters.left
        anchors.margins: 10

        model: solutionModel

        delegateColor: "#dfdfdf"
        delegateGradient: Gradient {
            GradientStop { position: 0.0; color: "#dfdfdf" }
            GradientStop { position: 1.0; color: "#dadada" }
        }
    }

    GroupBox {
        id: parameters
        anchors.right: windowAnchors.right
        anchors.bottom: windowAnchors.bottom
        anchors.top: windowAnchors.top
        anchors.topMargin: 5
        anchors.margins: 10
        width: 300
        title: qsTr("Parameters")

        ParametersView {
            id: parametersList
            anchors.fill: parent
            model: parameterModel

            onParameterChanged: {
                parameterModel.setParameterValue(name, value);
            }
        }
    }

    Rectangle {
        id: fullScreenHelp

        anchors.horizontalCenter: parent.horizontalCenter
        width: fullScreenHelpLabel.width * 1.5
        height: 50

        color: "#dddddd"
        radius: 3
        border.color: "#888888"
        border.width: 1
        opacity: 0.8

        Behavior on y {
            NumberAnimation {
                easing.type: "OutCubic"
                duration: 500
            }
        }

        Label {
            id: fullScreenHelpLabel
            text: qsTr("You are in the full screen mode (Press F11 for Windowed mode)")
            anchors.centerIn: parent
            font.pixelSize: 16
        }

        Timer {
            id: fullScrenHelpHideTimer
            interval: 2000
            running: false
            repeat: false
            onTriggered: {
                if(!fullScreenHelpMouseArea.containsMouse && parent.y > 0)
                    parent.hide();
            }
        }

        function show() {
            y = 30;
        }

        function hide() {
            y = -height;
        }
    }

    MouseArea {
        id: fullScreenHelpMouseArea
        anchors.horizontalCenter: window.horizontalCenter
        anchors.top: window.top
        width: window.width
        height: fullScreenHelp.y > 0 ? fullScreenHelp.height * 2 + fullScreenHelp.y / 2 : 5
        hoverEnabled: true
        onEntered: {
            if(visibilityControl.state === "FullScreen")
                fullScreenHelp.show();
        }
        onExited: fullScreenHelp.hide()
    }

    Item {
        id: visibilityControl
        anchors.fill: parent
        focus: true
        z: 100

        Keys.onPressed: {
            if(event.key === Qt.Key_F11) {
                if(state === "FullScreen")
                    state = "Windowed";
                else
                    state = "FullScreen";
                event.accepted = true;
            }
        }
        Keys.onEscapePressed: Qt.quit()

        states: [
        State {
            name: "Windowed"
            PropertyChanges {
                target: window
                visibility: "Windowed"
            }
            StateChangeScript {
                script: {
                    fullScreenHelp.hide();
                }
            }
        },
        State {
            name: "FullScreen"
            PropertyChanges {
                target: window
                visibility: "FullScreen"
            }
            StateChangeScript {
                script: {
                    fullScreenHelp.show();
                    fullScrenHelpHideTimer.start();
                }
            }
        }]

        Component.onCompleted: {
            state = "FullScreen";
        }
    }
}
