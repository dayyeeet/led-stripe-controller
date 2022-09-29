
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

           }S
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


changeColorGlobal();