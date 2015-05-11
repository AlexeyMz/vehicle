import QtQuick 2.2
import QtQuick.Layouts 1.1
import QtQuick.Dialogs 1.1
import QtQuick.Controls 1.1
import "../styles" as Styles

ApplicationWindow {
    id: window
    title: qsTr("Vehicle")

    minimumWidth: 1024
    minimumHeight: 600

    visible: true

    Item {
        id: windowAnchors
        anchors.fill: parent
    }

    Image {
        id: background
        anchors.fill: windowAnchors
        fillMode: Image.PreserveAspectCrop
        z: -1
        source: "../images/background.png"
    }

    Item {
        id: menu

        property int iconSize: 64
        property int spacing: 10

        anchors.topMargin: 15
        anchors.bottomMargin: 10
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        anchors.top: windowAnchors.top
        anchors.left: windowAnchors.left
        anchors.right: parameters.left
        height: iconSize + anchors.topMargin + anchors.bottomMargin

        MessageDialog {
            id: messageDialog
            Component.onCompleted: close()
        }

        IconButton {
            id: openButton
            anchors.top: parent.top
            anchors.left: parent.left
            icon: "../images/open.png"
            size: Qt.size(parent.iconSize, parent.iconSize)
            tooltip: qsTr("Open configurations")

            onClicked: {
                if(openDialog.folder.toString().length === 0) {
                    if(saveDialog.folder.toString().length === 0) {
                        openDialog.folder = applicationDirUrl;
                    } else {
                        openDialog.folder = saveDialog.folder;
                    }
                }
                openDialog.open();
            }

            FileDialog {
                id: openDialog
                title: qsTr("Please choose a file")
                nameFilters: [ qsTr("XML files (*.xml)") ]
                selectFolder: false
                selectMultiple: false
                onAccepted: {
                    if(!solutionModel.load(fileUrl)) {
                        messageDialog.title = qsTr("Error");
                        messageDialog.text = qsTr("Failed to load configurations");
                        messageDialog.icon = StandardIcon.Critical;
                        messageDialog.detailedText = solutionModel.lastError();
                        messageDialog.open();
                    } else {
                        if(solutionModel.isOutdated()) {
                            messageDialog.title = qsTr("Warning");
                            messageDialog.text = qsTr("Configurations you have loaded were based on outdated data. Now they may not be available. Please, consult the staff for more information");
                            messageDialog.icon = StandardIcon.Warning;
                            messageDialog.open();
                        }
                        parameters.enabled = false;
                    }
                }
                Component.onCompleted: close()
            }
        }
        IconButton {
            id: saveButton
            anchors.top: parent.top
            anchors.left: openButton.right
            anchors.leftMargin: parent.spacing
            icon: "../images/save.png"
            size: Qt.size(parent.iconSize, parent.iconSize)
            tooltip: qsTr("Save configurations")

            onClicked: {
                if(saveDialog.folder.toString().length === 0) {
                    if(openDialog.folder.toString().length === 0) {
                        saveDialog.folder = applicationDirUrl;
                    } else {
                        saveDialog.folder = openDialog.folder;
                    }
                }
                saveDialog.open();
            }

            FileDialog {
                id: saveDialog
                title: qsTr("Please choose a file")
                nameFilters: [ qsTr("XML files (*.xml)") ]
                selectFolder: false
                selectExisting: false
                selectMultiple: false

                onAccepted: {
                    if(!solutionModel.save(fileUrl)) {
                        messageDialog.title = qsTr("Error");
                        messageDialog.text = qsTr("Failed to save configurations");
                        messageDialog.icon = StandardIcon.Critical;
                        messageDialog.detailedText = solutionModel.lastError();
                        messageDialog.open();
                    }
                }
                Component.onCompleted: close()
            }
        }
        IconButton {
            id: editModeButton
            anchors.top: parent.top
            anchors.left: saveButton.right
            anchors.leftMargin: parent.spacing
            icon: "../images/user.png"
            size: Qt.size(parent.iconSize, parent.iconSize)
            tooltip: qsTr("Enter the data edit mode")
            onClicked: {
                window.visibility = "Hidden";
                if(bridge.login()) {
                    parameterModel.openEditMode();
                }
                window.visibility = visibilityControl.state
            }
        }
        IconButton {
            id: sortDescButton
            anchors.top: parent.top
            anchors.right: sortAscButton.left
            anchors.rightMargin: parent.spacing
            icon: "../images/pricedesc.png"
            size: Qt.size(parent.iconSize, parent.iconSize)
            tooltip: qsTr("Sort price descending")
            onClicked: solutionModel.sort(0, Qt.DescendingOrder);
        }
        IconButton {
            id: sortAscButton
            anchors.top: parent.top
            anchors.right: parent.right
            icon: "../images/priceasc.png"
            tooltip: qsTr("Sort price ascending")
            size: Qt.size(parent.iconSize, parent.iconSize)
            onClicked: solutionModel.sort(0, Qt.AscendingOrder);
        }
    }

    SolutionsView {
        id: solutions
        anchors.left: windowAnchors.left
        anchors.top: menu.bottom
        anchors.bottom: windowAnchors.bottom
        anchors.right: parameters.left
        anchors.margins: 10

        model: solutionModel

        delegateColor: "#f5770a"
        delegateGradient: Gradient {
            GradientStop { position: 0.0; color: "#60ffcc00" }
            GradientStop { position: 1.0; color: "#60f5770a" }
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
        title: qsTr("PARAMETERS")
        style: Styles.GroupBoxStyle {}

        ParametersView {
            id: parametersList
            anchors.fill: parent
            model: parameterModel

            onParameterChanged: parameterModel.setParameterValue(name, value)
        }
    }

    Rectangle {
        id: parametersLock
        anchors.fill: parameters
        visible: !parameters.enabled
        gradient: solutions.delegateGradient
        opacity: 0.2
    }

    IconButton {
        id: parametersLockIcon
        anchors.centerIn: parametersLock
        visible: parametersLock.visible
        icon: "../images/unlock.png"
        size: Qt.size(96,96)

        onClicked: {
            solutionModel.restore();
            parameters.enabled = true;
        }
    }

    Text {
        visible: parametersLockIcon.visible
        anchors.horizontalCenter: parametersLockIcon.horizontalCenter
        anchors.top: parametersLockIcon.bottom
        anchors.topMargin: 5

        text: qsTr("Back to the configurations")
        height: 60
        font.pixelSize: 18
    }

    Rectangle {
        id: fullScreenHelp

        anchors.horizontalCenter: parent.horizontalCenter
        width: fullScreenHelpLabel.width * 1.5
        height: 50

        color: "#80dddddd"
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
            text: qsTr("You are in the full screen mode (Press F11 for windowed mode)")
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
                    state = "Maximized";
                else
                    state = "FullScreen";
                event.accepted = true;
            }
        }
        Keys.onEscapePressed: Qt.quit()

        states: [
        State {
            name: "Maximized"
            PropertyChanges {
                target: window
                visibility: "Maximized"
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
