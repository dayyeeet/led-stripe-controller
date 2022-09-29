
const colorPickerHtml = '<div class="color-wrapper"> <input type="color" id="color-ID" name="color-ID" class="color-container-picker" value="#DEDEDE"><br> <img src="../res/img/light_bulb.png" class="color-container-design"></div>';

function generateColorPickers(amount, accuracy)
{
    for(var i = 0; i < amount / accuracy; i++)
    {
        document.getElementById("color-container").innerHTML += colorPickerHtml.replaceAll('ID', '' + (i + 1));
    }
}

generateColorPickers(100, 5);