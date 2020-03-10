

cl -MD -Zi canvas.c src\glad.c -I.\libs\include\ -I.\include\  /link user32.lib Gdi32.lib winmm.lib shell32.lib gdi32.lib  opengl32.lib  glfw3.lib -LIBPATH:.\libs\lib-vc2015
