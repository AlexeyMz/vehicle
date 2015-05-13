function position()
{
    var y = 0;

    if(container.orientation === Qt.Vertical)
        y = scrollArea.visibleArea.yPosition * container.height;
    else
        y = scrollArea.visibleArea.xPosition * container.width;

    return y > 2 ? y : 2;
}

function size()
{
    var h, y;

    if(container.orientation === Qt.Vertical)
        h = scrollArea.visibleArea.heightRatio * container.height;
    else
        h = scrollArea.visibleArea.widthRatio * container.width;

    if(container.orientation === Qt.Vertical)
        y = scrollArea.visibleArea.yPosition * container.height;
    else
        y = scrollArea.visibleArea.xPosition * container.width;

    if(y > 3) {
        var t;
        if(container.orientation === Qt.Vertical)
            t = Math.ceil(container.height - 3 - y);
        else
            t = Math.ceil(container.width - 3 - y);

        return h > t ? t : h;
    } else {
        return h + y;
    }
}
