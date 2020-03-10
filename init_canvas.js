

console.log("what");

var Module = {
    onRuntimeInitialized: function() {
        // INIT?
        // var e = document.getElementById('loadingDiv');
        // e.style.visibility = 'hidden';
        window_on_resize = Module.cwrap('window_on_resize', 'void', ['number', 'number'])
		
		window.onload = window.onresize = function() {
			window_on_resize(window.innerWidth, window.innerHeight);
			console.log("canvas " + Module.canvas.width + " " + canvas.height);
			canvas.width  = window.innerWidth;
			canvas.height = window.innerHeight;
		}
		window_on_resize(window.innerWidth, window.innerHeight)
    },
    canvas: (function() {
        var canvas = document.getElementById('canvas');
        return canvas;
    })()
};

var start_function = function(o) {
    //    document.getElementById("fullScreenButton").style.visibility="visible";
    Module.ccall('mainf', null, null);
};

var script = document.createElement('script');
script.src = "index.js";
document.body.appendChild(script);



// When the user scrolls the page, execute myFunction
window.onscroll = function() {myFunction()};

// Get the header
var header = document.getElementById("myHeader");

// Get the offset position of the navbar
var sticky = header.offsetTop;

// Add the sticky class to the header when you reach its scroll position. Remove "sticky" when you leave the scroll position

function myFunction() {
    if (window.pageYOffset > sticky) {
        header.classList.add("sticky");
    } else {
        header.classList.remove("sticky");
    }
}

