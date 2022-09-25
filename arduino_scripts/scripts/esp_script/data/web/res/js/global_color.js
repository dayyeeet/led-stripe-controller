
let firstColor;

function changeColorGlobal(){
    const colorPickers = document.getElementsByClassName("color-container-picker");
    const globalPicker = document.getElementById("global-color");
    firstColor = globalPicker.value;
    globalPicker.addEventListener('change', (e) => {
        let color = e.target.value;
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
                   globalPicker.value = firstColor;
            });
        }
    }
};


changeColorGlobal();