# Wow client
Wow client targeting wow TBC 2.4.3 8606

## dependencies
| name  | purpose |
| ------------ | --- |
| [zlib](http://zlib.net) | compression / decompression of zlib data |
| [libpng](http://www.libpng.org/pub/png/libpng.html) | PNG icon reading |
| [freetype](https://freetype.org) | font rasterizing |
| [glfw](https://www.glfw.org) | window management |
| [lua](https://www.lua.org) | interfaces lua interpreter |
| [libxml2](http://www.xmlsoft.org/libxslt/index.html) | interfaces xml parsing |
| [jks](https://github.com/acazuc/jks) | standard collections, mathematic helpers |
| [gfx](https://github.com/acazuc/gfx) | OpenGL / D3D abstraction layer for rendering, xlib / win32 / glfw abstraction layer for window management |
| [libwow](https://github.com/acazuc/libwow) | wow files parsing |
| [portaudio](http://portaudio.com) | audio interface |
| [libsamplerate](http://libsndfile.github.io/libsamplerate) | audio PCM resampling |
| [jkssl](https://github.com/acazuc/jkssl) | cryptography (bignumbers, hash) |
| [jkl](https://github.com/acazuc/jkl) | library compilation toolchain |

## how to build
- clone [jkl](https://github.com/acazuc/jkl) in `lib/sl_lib`: `git clone https://github.com/acazuc/jkl lib/sl_lib`
- clone [jkssl](https://github.com/acazuc/jkssl) in `lib/sl_lib/jkssl/jkssl`: `git clone https://github.com/acazuc/jkssl lib/sl_lib/jkssl/jkssl`
- clone [libwow](https://github.com/acazuc/libwow) in `lib/sl_lib/libwow/libwow`: `git clone https://github.com/acazuc/libwow lib/sl_lib/libwow/libwow`
- clone [jks](https://github.com/acazuc/jks) in `lib/sl_lib/jks/jks`: `git clone https://github.com/acazuc/jks lib/sl_lib/jks/jks`
- clone [gfx](https://github.com/acazuc/gfx) in `lib/sl_lib/gfx/gfx`: `git clone https://github.com/acazuc/gfx lib/sl_lib/gfx/gfx`
- `cp config.sample config`
- `make lib`
- `make`
