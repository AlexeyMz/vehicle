import QtQuick 2.2

Rectangle {
    id: tooltip

    width: tooltipText.width + 10
    height: tooltipText.height + 6

    border.color: "darkgray"
    border.width: 1

    color: "white"

    visible: false

    property alias fadeInDelay: fadein.duration
    property alias fadeOutDelay: fadeout.duration
    property alias text: tooltipText.text
    property alias showDelay: showTimer.interval
    property alias hideDelay: hideTimer.interval

    Timer {
        id: showTimer
        interval: 1000
        running: false
        onTriggered: {
            if(!tooltip.visible) {
                tooltip.visible = true;
                fadein.start();
            }
        }
    }

    Timer {
        id: hideTimer
        interval: 100
        running: false
        onTriggered: {
            if(tooltip.visible) {
                fadeout.start();
            }
        }
    }

    function show() {
        hideTimer.stop();
        if(!tooltip.visible) {
            showTimer.start();
        }
    }

    function hide() {
        showTimer.stop();
        if(tooltip.visible) {
            hideTimer.start();
        }
    }

    Text {
        id: tooltipText
        color: "#555555";
        renderType: Text.NativeRendering
        anchors.centerIn: parent
    }

    NumberAnimation {
        id: fadein
        target: tooltip
        property: "opacity"
        easing.type: Easing.InQuad
        duration: 300
        from: 0
        to: 1
    }

    NumberAnimation {
        id: fadeout
        target: tooltip
        property: "opacity"
        easing.type: Easing.InQuad
        duration: 300
        from: 1
        to: 0

        onStopped: tooltip.destroy()
    }
}
