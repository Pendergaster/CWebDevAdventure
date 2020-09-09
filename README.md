# CWebDevAdventure
adventure to web developement

This project was done as part of school course.
Goal of this project was to implement HTTPS server using C-programming language.
We desided to use [openssl](https://www.openssl.org/) to implement cryptography.

This server was able to compile, run and return output of the C-code written in the browser.
Server is able to respond to queries and serve pages.
We used [libtcc](https://bellard.org/tcc/) to dynamically compile and run the C code in server.
**This is obviously really unsecure**, but this was fun project to try and to learn from!

There is also WebGL based backgrounds which were compiled using [emscripten](https://emscripten.org/).
