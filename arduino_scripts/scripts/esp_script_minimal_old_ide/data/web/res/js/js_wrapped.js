
let firstColor;

function changeColorGlobal(){
    const colorPickers = document.getElementsByClassName("color-container-picker");
    const globalPicker = document.getElementById("global-color");
    firstColor = globalPicker.value;
    globalPicker.addEventListener('change', (e) => {
        let color = e.target.value;
        globalPicker.parentElement.style.opacity = 1;
        for(const colorPicker of colorPickers)
           {
             if(colorPicker != e.target)
             {
                colorPicker.value = color;
             }

           }
    });

    for(const colorPicker of colorPickers)
    {
        if(colorPicker != globalPicker)
        {
            colorPicker.addEventListener('change', (e) => {
                   globalPicker.parentElement.style.opacity = 0.35;
            });
        }
    }
};


function generatePlaceHolders()
{
    generateNonColorCustomEffectPlaceHolders();
    fixColorCustomEffectPlaceHolders();
    fixGlobalColorOpacity();
}

const placeHolderWifiName = "%WIFI_SSID%";
const placeHolderBrightness = "%BRIGHTNESS%";
const placeHolderResolution = "1";
const placeHolderLEDCount = "10";
const placeHolderLEDGPIO = "%LED_GPIO%";
const placeHolderGlobalColor = "%GLOBAL_COLOR%";

function generateNonColorCustomEffectPlaceHolders()
{


    //Check if wifi SSId is not the default one
    setToPlaceHolder("ssid", placeHolderWifiName);

    //Check if resolution is a number and only then set it
    if(/^[0-9]+$/.test(placeHolderResolution))
    setToPlaceHolder("resolution", placeHolderResolution);

    //Check if LEDCount is a number and only then set it
    if(/^[0-9]+$/.test(placeHolderLEDCount))
    setToPlaceHolder("ledcount", placeHolderLEDCount);
    
    //Check if gpio is a number and only then set it
    if(/^[0-9]+$/.test(placeHolderLEDGPIO))
    setToPlaceHolder("gpio", placeHolderLEDGPIO);

    //Check if brightness is a number and only then set it
    if(/^(0|[1-9][0-9]?|100)$/.test(placeHolderBrightness))
    setToPlaceHolder("brightness", placeHolderBrightness);

    //Check if placeHolderGlobalColor is a HEX color code
    if(/^#[0-9A-Fa-f]{6}$/.test(placeHolderGlobalColor))
    setToPlaceHolder("global-color", placeHolderGlobalColor);
}


const colorPickerHtml = '<div class="color-wrapper"> <input type="color" id="color-ID" name="color-ID" class="color-container-picker" value="COLOR_ID"><br> <img src="../res/img/light_bulb.png" class="color-container-design"></div>';
const colors = ["%C_1%", "%C_2%", "%C_3%", "%C_4%", "%C_5%", "%C_6%", "%C_7%", "%C_8%", "%C_9%", "%C_10%", "%C_11%", "%C_12%", "%C_13%", "%C_14%", "%C_15%", "%C_16%", "%C_17%", "%C_18%",
    "%C_19%", "%C_20%", "%C_21%", "%C_22%", "%C_23%", "%C_24%", "%C_25%", "%C_26%", "%C_27%", "%C_28%", "%C_29%", "%C_30%", "%C_31%", "%C_32%"
               ]


function generateColorPickers(amount, accuracy)
{
    let realAmount = 0;
    let realAccuracy = 0;

    if(parseInt(amount) > 0) realAmount = amount; else realAmount = 1;
    if(parseInt(accuracy) > 0) realAccuracy = accuracy; else realAccuracy = 1;

    for(var i = 0; i < realAmount / realAccuracy; i++)
    {
        document.getElementById("color-container").innerHTML += colorPickerHtml.replaceAll('COLOR_ID', colors[i]).replaceAll('ID', '' + (i + 1));
    }
}

function generateColorPickersWithPlaceHolders()
{
    generateColorPickers(placeHolderLEDCount, placeHolderResolution);
}

function setToPlaceHolder(name, value)
{
    document.getElementById(name).value = value;
    document.getElementById(name).defaultValue = value;
}

function fixColorCustomEffectPlaceHolders()
{
    const globalPicker = document.getElementById("global-color");
    const colorPickers = document.getElementsByClassName("color-container-picker");
    let x = 0;
    for(const colorPicker of colorPickers)
    {
        if(/^#000000$/.test(colorPicker.value))
        if(colorPicker != globalPicker)
        {
            colorPicker.value = globalPicker.value;
            colorPicker.defaultValue = globalPicker.value;
            x++;
        }

    }
}

function fixGlobalColorOpacity()
{
    const colorPickers = document.getElementsByClassName("color-container-picker");
    const globalPicker = document.getElementById("global-color");
    for(const colorPicker of colorPickers)
    {
        if(globalPicker.value != colorPicker.value)
        {
            globalPicker.parentElement.style.opacity = 0.35;
            return;
        }
    }
}
