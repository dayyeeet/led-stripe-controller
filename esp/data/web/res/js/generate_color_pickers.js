
const colorPickerHtml = '<div class="color-wrapper"> <input type="color" id="color-ID" name="color-ID" class="color-container-picker" value="COLOR_ID"><br> <img src="../res/img/light_bulb.png" class="color-container-design"></div>';

function generateColorPickers(amount, accuracy)
{
    let realAmount = 0;
    let realAccuracy = 0;

    if(/^[0-9]$/.test(amount)) realAmount = amount; else realAmount = 1;
    if(/^[0-9]$/.test(accuracy)) realAccuracy = accuracy; else realAccuracy = 1;

    for(var i = 0; i < realAmount / realAccuracy; i++)
    {
        document.getElementById("color-container").innerHTML += colorPickerHtml.replaceAll('ID', '' + (i + 1));
    }
}

generateColorPickers("%AMOUNT%", "%ACCURACY%");