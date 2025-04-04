
function generatePlaceHolders()
{
    generateNonColorCustomEffectPlaceHolders();
    fixColorCustomEffectPlaceHolders();
}

function generateNonColorCustomEffectPlaceHolders()
{
    const placeHolderWifiName = "%WIFI_SSID%";
    const placeHolderBrightness = "%BRIGHTNESS%";
    const placeHolderResolution = "%RESOLUTION%";
    const placeHolderLEDCount = "%LED_COUNT%";
    const placeHolderLEDGPIO = "%LED_GPIO%";
    const placeHolderGlobalColor = "%COLOR_GLOBAL%";

    //Check if wifi SSId is not the default one
    if(!/^%WIFI_SSID%/.test(placeHolderWifiName))
    setToPlaceHolder("ssid", placeHolderWifiName);

    //Check if resolution is a number and only then set it
    if(/^[0-9]$/.test(placeHolderResolution))
    setToPlaceHolder("resolution", placeHolderResolution);

    //Check if LEDCount is a number and only then set it
    if(/^[0-9]$/.test(placeHolderLEDCount))
    setToPlaceHolder("ledcount", placeHolderLEDCount);
    
    //Check if gpio is a number and only then set it
    if(/^[0-9]$/.test(placeHolderLEDGPIO))
    setToPlaceHolder("gpio", placeHolderLEDGPIO);

    //Check if brightness is a number and only then set it
    if(/^[0-9]$/.test(placeHolderBrightness))
    setToPlaceHolder("brightness", placeHolderBrightness);

    //Check if placeHolderGlobalColor is a HEX color code
    if(/^#[0-9A-F]{6}$/i.test(placeHolderGlobalColor))
    setToPlaceHolder("global-color", placeHolderGlobalColor);
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
    for(const colorPicker of colorPickers)
    {
        if(colorPicker != globalPicker)
        {
            colorPicker.value = "#DEDEDE";
            colorPicker.defaultValue = "#DEDEDE";
        }
    }
}

generatePlaceHolders();
