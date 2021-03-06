/*
    
    This Passes Coordinates from a list ot points to the PICO

*/
const fs = require('fs');
const predefinedImages = require('./predefined_images');
const { write, open } = require('./serial');
const { scalePoints } = require('./utils');

// Set the Min and Max Steps for the PICO board (Same as in pico.h)
const MAX_STEPS_X = 10, MAX_STEPS_Y = 10, MAX_STEPS_Z = 1, MIN_STEPS_X = 0, MIN_STEPS_Y = 0, MIN_STEPS_Z = 0;

(async () => {

    // Handle the Command Arguments
    const args = process.argv.slice(2);
    const dumpImage = (args[0] || '').toLowerCase() === 'dump';
    const image = predefinedImages[dumpImage ? args[1] : args[0]];
    if(!image)
    {
        console.log(`Invalid Image Provided\nRun one of the following commands to run the script:\n${
            Object.keys(predefinedImages)
                .filter(image => image !== 'generate')
                .map(image => `yarn start ${image}`)
                .join('\n')
        }`);
        return;
    }

    // Open Serial Connection
    await open();

    // Get to the Automated Draw Menu
    await write("s\n");
    
    // Reset the to the Origin
    await write(`${MIN_STEPS_X},${MIN_STEPS_Y},${MIN_STEPS_Z};`);

    // Process Points
    const paths = image;
    const dump = {};
    
    // As there are multiple paths with are not connected we need to iterate through each of them seperately to get a good scale
    const pathIterator = Object.entries(paths);
    
    // Setup Scale Variables
    let maxX = 0, maxY = 0, lowX = Number.MAX_SAFE_INTEGER, lowY = Number.MAX_SAFE_INTEGER;
    
    console.log(`Processing ${pathIterator.length} Paths...`)
    for(const [, pathPoints] of pathIterator)
    {
        // Get the Scale of the Current Points   
        for(const [x, y] of pathPoints)
        {
            if(x > maxX) maxX = +x;
            if(x < lowX) lowX = +x;
            if(y > maxY) maxY = +y;
            if(y < lowY) lowY = +y;
        }
    }
    console.log(`MAX_X: ${maxX}, MAX_Y: ${maxY}, LOW_X: ${lowX}, LOW_Y: ${lowY}`);

    for(const [i, [key, points]] of pathIterator.entries())
    {
        console.log(`Normalising & Scaling Path: ${key} (#${i + 1} / ${pathIterator.length})`);
        // Normalise, Scale the Points
        const scaled = scalePoints(points, maxX, maxY, lowX, lowY, MAX_STEPS_X, MAX_STEPS_Y, MIN_STEPS_X, MIN_STEPS_Y);
        
        // Dump this path so we can visualise it
        // dump[key] = scaled;

        // Round so the pico can process
        // Sometimes the rounded number does not match what it should be. Using Higher Maxes Fixes This
        const processedPoints = scaled.map(([x, y], i, arr) => {
            const roundedX = +(Math.round(x * 32.) / 32.).toFixed(5),
                roundedY = +(Math.round(y * 32.) / 32.).toFixed(5);

            // Bascially If there is no difference in 3 points sequentially remove the middle point
            if(i > 0 && i < scaled.length - 1)
            {
                let previousIndex = i - 1;
                let [previousX, previousY] = arr[previousIndex];
                const [nextX, nextY] = scaled[i + 1];
                

                // If the Previous Step was Removed go back another step
                while(previousX === MIN_STEPS_X - 1 && previousY === MIN_STEPS_Y - 1)
                {
                    [previousX, previousY] = arr[--previousIndex];
                }
                    
                // All Points Match. Remove Middle Element all together
                if(
                    (previousX === roundedX && previousY === roundedY) &&
                    (nextX === roundedX && nextY === roundedY)
                ) 
                    return [MIN_STEPS_X - 1, MIN_STEPS_Y - 1];

                // BEWARE: Below will probably fattern out any sharp deviations that stay on one of the same axis
                // Like: ___/\/\_____
                // Would Become: _________

                // Remove Redundant Y Movement Instructions
                if(previousX === roundedX && nextX === roundedX)
                {
                    arr[previousIndex][1] = previousY + roundedY - previousY; // Add the extra y 
                    return [MIN_STEPS_X - 1, MIN_STEPS_Y - 1];
                }
                // Remove Redundant X Movement Instructions
                if(previousY === roundedY && nextY === roundedY)
                {
                    arr[previousIndex][0] = previousX + roundedX - previousX; // Add the extra x 
                    return [MIN_STEPS_X - 1, MIN_STEPS_Y - 1];
                }
            }
            return [roundedX, roundedY];
        }).filter(([x, y]) => x !== MIN_STEPS_X - 1 && y !== MIN_STEPS_Y - 1);
        
        dump[key] = processedPoints;

        // Send All the Scaled Points to the PICO
        console.log(`Sending: ${processedPoints.length} Steps (Originally: ${scaled.length} Steps)`);
        
        if(dumpImage)
            continue; // Skip the Serial Transmission so we can dump
        
        // Get the First and Last Step of the Path so we can handle the Z Lift Accordingly
        const [firstElementX, firstElementY] = processedPoints.shift(), 
            [lastElementX, lastElementY] = processedPoints.pop();

        // Setup the Inital X & Y
        await write(`${firstElementX},${firstElementY},${MIN_STEPS_Z};`);
        // Place the Z
        await write(`${firstElementX},${firstElementY},${MAX_STEPS_Z};`);

        // Iterate Over All the Other Points
        for(const [i, [x, y]] of processedPoints.entries())
        {
            const stepInstruction = `${x},${y},${MAX_STEPS_Z};`
            await write(stepInstruction);
            console.log(`${stepInstruction} (#${i + 1})`);
        }

        // Handle the Last Element
        await write(`${lastElementX},${lastElementY},${MAX_STEPS_Z};`);
        // Lift the Z
        await write(`${lastElementX},${lastElementY},${MIN_STEPS_Z};`);
    }
    fs.writeFileSync('dump.js', `var obj = ${JSON.stringify(dump)}; var loaded = true;`);
})();







