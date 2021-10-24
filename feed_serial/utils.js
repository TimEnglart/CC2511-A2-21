const { transform } = require('pathologist');


// Apply All Transforms to the SVG paths
const pathologiseSVG = (svgString) => {
    const reText = /<text[\s\S]*?<\/text>/g;
    const reStyle = /<style[\s\S]*?<\/style>/g;
    const removedText = svgString.replace(reText, '');
    const removedStyle = removedText.replace(reStyle, '');
    return transform(removedStyle);
}


// Rescale the Value based on the max and min values of the current and new scale
const rescale = (value, maxCurrentScale, minCurrentScale, maxNewScale, minNewScale) => {
    return minNewScale + (value - minCurrentScale) * ((maxNewScale - minNewScale) / (maxCurrentScale - minCurrentScale))
}
 
// Scale an Array of Points From their previous scale to a new scale
const scalePoints = (points, maxX, maxY, minX, minY, maxXBounds, maxYBounds, minXBounds, minYBounds) => {
    const scaledPoints = [];
    for(const [x, y] of points)
    {
        const scaledX = rescale(+x, maxX, minX, maxXBounds, minXBounds);
        const scaledY = rescale(+y, maxY, minY, maxYBounds, minYBounds);

        // If the scale is outside of scope reduce that axis bounds by 10%
        if(scaledX > maxXBounds || scaledX < minXBounds)
        {
            return scalePoints(points, maxX, maxY, minX, minY, 
                maxXBounds - (maxXBounds * 0.1), maxYBounds, minXBounds + (minXBounds * 0.1), minYBounds);
        }
        if(scaledY > maxYBounds || scaledY < minYBounds)
        {
            return scalePoints(points, maxX, maxY, minX, minY, 
                maxXBounds, maxYBounds - (maxYBounds * 0.1), minXBounds, minYBounds + (minYBounds * 0.1));
        }
        scaledPoints.push([scaledX, scaledY]);
    }
    return scaledPoints;
}

module.exports = {
    scalePoints,
    rescale,
    pathologiseSVG
};