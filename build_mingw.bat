"C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\Common7\Tools\VsDevCmd.bat" &&^
cmake . -G "MinGW Makefiles" &^
mingw32-make.exe all &&^
strip -s -R .comment -R .gnu.version --strip-unneeded test.exe