import QtQuick 2.2
import QtQuick.Controls 1.1

Item {
    property alias delegateGradient: listView.delegateGradient
    property alias delegateColor: listView.delegateColor
    property alias model: listView.model

    ListView {
        id: listView

        property Gradient delegateGradient
        property color delegateColor

        snapMode: ListView.SnapToItem
        anchors.fill: parent
        cacheBuffer: 200
        spacing: 10

        add: Transition {
            NumberAnimation { property: "opacity"; from: 0; to: 1.0; duration: 400 }
            NumberAnimation { property: "scale"; from: 0; to: 1.0; duration: 400 }
        }

        remove: Transition {
            NumberAnimation { property: "opacity"; from: 1.0; to: 0; duration: 400 }
            NumberAnimation { property: "scale"; from: 1.0; to: 0; duration: 400 }
        }

        displaced: Transition {
            NumberAnimation { property: "x,y"; duration: 400; easing.type: Easing.OutBack }
        }

        delegate: solutionDelegate

        /*section {
            property: "mark"
            criteria: ViewSection.FullString
            delegate: Rectangle {
                width: parent.width
                height: 50
                Text {
                    renderType: Text.NativeRendering
                    anchors.centerIn: parent
                    font.pixelSize: 20
                    font.bold: true
                    text: section
                }
            }
        }*/
    }

    // Delegate for the solutions.  This delegate has two modes:
    // 1. List mode (default), which just shows the picture, title, price and short description of the solution.
    // 2. Details mode, which shows detailed description instead of short.
    Component {
        id: solutionDelegate

        Item {
            id: solution

            // Create a property to contain the visibility of the details.
            // We can bind multiple element's opacity to this one property,
            // rather than having a "PropertyChanges" line for each element
            // we want to fade.
            property real detailsOpacity: 0

            width: ListView.view.width
            height: 100

            Rectangle {
                id: background
                anchors.fill: parent
                color: delegateColor
                gradient: delegateGradient
            }

            // This is highligh item
            Rectangle {
                anchors.fill: background
                color: delegateColor
                gradient: delegateGradient
                border.color: Qt.darker(background.color)
                border.width: 1

                opacity: mouseArea.containsMouse || detailsOpacity ? 1.0 : 0.0

                Behavior on opacity { NumberAnimation { duration: 400; } }
            }

            // This mouse region covers the entire delegate.
            // When clicked it changes mode to 'Details'. If we are already
            // in Details mode, then no change will happen.
            MouseArea {
                id: mouseArea
                anchors.fill: parent
                onClicked: solution.state = 'Details';
                hoverEnabled: true
            }

            // Lay out the page: picture, title and price at the top,
            // and full description at the bottom.  Note that elements that
            // should not be visible in the list mode have their opacity set to solution.detailsOpacity.
            Text {
                id: titleField
                font.bold: true
                font.pixelSize: 16
                anchors.top: background.top
                anchors.topMargin: 10
                anchors.left: background.left
                anchors.leftMargin: 10
                text: model.Mark + " " + model.Model
            }

            Text {
                id: priceField
                font.bold: true
                font.pixelSize: 16
                anchors.top: background.top
                anchors.topMargin: 10
                anchors.right: background.right
                anchors.rightMargin: 10
                text: Number(model.Price).toLocaleCurrencyString(Qt.locale())
            }

            property string shortDescription: model.ShortDescription
            property var fullDescription: model.FullDescription

            Text {
                anchors.left: titleField.left
                anchors.bottomMargin: 10
                anchors.top: titleField.bottom
                anchors.topMargin: titleField.anchors.topMargin

                width: background.width - priceField.width - priceField.anchors.rightMargin - titleField.anchors.leftMargin
                verticalAlignment: TextEdit.AlignTop
                elide: Text.ElideRight
                maximumLineCount: 1
                font.pixelSize: 14
                text: shortDescription

                opacity: 1.0 - solution.detailsOpacity
            }

            ListView {
                anchors.left: titleField.left
                anchors.leftMargin: titleField.anchors.leftMargin
                anchors.bottom: background.bottom
                anchors.bottomMargin: 10
                anchors.top: titleField.bottom
                anchors.topMargin: titleField.anchors.topMargin
                anchors.right: priceField.left
                anchors.rightMargin: priceField.anchors.rightMargin
                interactive: false

                model: fullDescription

                delegate: Text {
                    width: ListView.view.width
                    height: 25
                    elide: Text.ElideRight
                    font.pixelSize: 14
                    text: modelData
                    textFormat: Text.RichText
                }

                opacity: solution.detailsOpacity
            }

            // A button to close the detailed view, i.e. set the state back to default ('').
            IconButton {
                id: closeButton

                anchors.top: background.top
                anchors.right: background.right
                anchors.margins: 5

                size: Qt.size(32,32)
                icon: "../images/close.png"
                opacity: solution.detailsOpacity
                visible: solution.detailsOpacity != 0

                onClicked: solution.state = '';
            }

            states: State {
                name: "Details"

                //PropertyChanges { target: vehicleImage; width: 130; height: 130 }
                PropertyChanges { target: solution; detailsOpacity: 1; height: listView.height }

                PropertyChanges { target: priceField; anchors.right: closeButton.left }

                // Move the list so that this item is at the top.
                PropertyChanges { target: solution.ListView.view; explicit: true; contentY: solution.y }

                // Disallow flicking while we're in detailed view
                PropertyChanges { target: solution.ListView.view; interactive: false }
            }

            transitions: Transition {
                // Make the state changes smooth
                NumberAnimation { duration: 300; properties: "detailsOpacity,height,contentY" }
            }
        }
    }
}
