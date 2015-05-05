var component;
var tooltip;

var showDelay;
var hideDelay;
var fadeInDelay;
var fadeOutDelay;
var tip;

function show(x,y) {
    if(!component)
        component = Qt.createComponent("../qml/ToolTip.qml");

    if(component.status === Component.Ready) {
        tooltip = component.createObject(window);
        if(!tooltip) {
            console.warn("Error creating tooltip");
            console.warn(component.errorString());
            return false;
        }
        tooltip.text = tip;
        tooltip.fadeInDelay = fadeInDelay;
        tooltip.fadeOutDelay = fadeOutDelay;
        tooltip.showDelay = showDelay;
        tooltip.hideDelay = hideDelay;
        tooltip.x = x;
        tooltip.y = y;
        tooltip.show();
    } else {
        console.warn("Error loading tooltip component");
        console.warn(component.errorString());
        return false;
    }
}

function close() {
    if(tooltip)
        tooltip.hide();
}
