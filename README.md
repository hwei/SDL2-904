SDL2-904
========

I am trying to write a game with SDL2.

Current Feature
---------------

* Draw a white rectangle at 60fps.


How to compile SDL2 for Mac OS X
-------------------------------

1. Download SDL2 from http://www.libsdl.org/download-2.0.php .

2. Extract the source pack and:
```sh
./configure CPPFLAGS='-I/opt/local/include' LDFLAGS='-L/opt/local/lib' LIBS='-lGL -lGLU' --without-x --disable-video-x11 --disable-x11-shared --disable-video-x11-xcursor --disable-video-x11-xinerama --disable-video-x11-xinput --disable-video-x11-xrandr --disable-video-x11-scrnsaver --disable-video-x11-xshape --disable-video-x11-vm
make install
```

3. Open `Xcode/SDl`, complie, and copy result `SDL2.framework` to `\Library\Frameworks`
