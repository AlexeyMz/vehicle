import QtQuick 2.2
import QtQuick.Controls 1.1
import QtQuick.Controls.Styles 1.1

import model.qml.bridge.utils 1.0
import "../styles" as Styles

Item {
    signal parameterChanged(string name, string value)
    property alias model: listView.model

    ListView {
        id: listView

        property int margins: 2

        anchors.margins: margins
        anchors.fill: parent
        interactive: false
        spacing: 5

        add: Transition {
            NumberAnimation { property: "opacity"; from: 0; to: 1.0; duration: 400 }
            NumberAnimation { property: "scale"; from: 0; to: 1.0; duration: 400 }
        }

        remove: Transition {
            NumberAnimation { property: "opacity"; from: 1.0; to: 0; duration: 400 }
            NumberAnimation { property: "scale"; from: 1.0; to: 0; duration: 400 }
        }

        delegate: Row {
            property var contextModel: model

            Text {
                id: parameterName
                readonly property string name: contextModel.Parameter.name

                //renderType: Text.NativeRendering
                text: name + ": "
                width: contextModel.NameWidth
                elide: Text.ElideRight
                height: 25
                verticalAlignment: Text.AlignVCenter
            }
            ComboBox {
                anchors.verticalCenter: parameterName.verticalCenter
                visible: contextModel.Parameter.type === Parameter.ListType
                model: contextModel.Parameter.list
                width: listView.width - parameterName.width - listView.margins
                style: Styles.ComboBoxStyle {}

                onCurrentIndexChanged: {
                    parameterChanged(parameterName.name, currentText);
                }
            }
            CheckBox {
                anchors.verticalCenter: parameterName.verticalCenter
                visible: contextModel.Parameter.type === Parameter.BooleanType
                style: Styles.CheckBoxStyle {}

                onClicked: {
                    parameterChanged(parameterName.name, checked ? qsTr("Yes") : qsTr("No"));
                }
            }
            Text {
                // assert(contextModel.type != Parameter.UnknownType)
                visible: contextModel.Parameter.type === Parameter.UnknownType
                text: "UNKNOWN TYPE!"
            }
        }
    }
}
