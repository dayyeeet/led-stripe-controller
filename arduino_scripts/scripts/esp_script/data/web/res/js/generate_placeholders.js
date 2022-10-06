
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
    document.getElementById("ssid").value = placeHolderWifiName;

    //Check if resolution is a number and only then set it
    if(/^[0-9]$/.test(placeHolderResolution))
    document.getElementById("resolution").value = placeHolderResolution;

    //Check if LEDCount is a number and only then set it
    if(/^[0-9]$/.test(placeHolderLEDCount))
    document.getElementById("ledcount").value = placeHolderLEDCount;

    //Check if gpio is a number and only then set it
    if(/^[0-9]$/.test(placeHolderLEDGPIO))
    document.getElementById("gpio").value = placeHolderLEDGPIO;

    //Check if brightness is a number and only then set it
    if(/^[0-9]$/.test(placeHolderBrightness))
    document.getElementById("brightness").value = placeHolderBrightness;

    //Check if placeHolderGlobalColor is a HEX color code
    if(/^#[0-9A-F]{6}$/i.test(placeHolderGlobalColor))
    document.getElementById("global-color").value = placeHolderGlobalColor;
}

function fixColorCustomEffectPlaceHolders()
{
    const globalPicker = document.getElementById("global-color");
    const colorPickers = document.getElementsByClassName("color-container-picker");
    const replacedColorPlaceHolders = "%REPLACED_COLOR_PLACEHOLDERS%";
    const current = 0;
    for(const colorPicker of colorPickers)
    {
        if(colorPicker != globalPicker && !/^true/.test(replacedColorPlaceHolders))
        {
            colorPicker.value = "#DEDEDE";
        }
    }
}

generatePlaceHolders();