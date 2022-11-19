if not exist %PICO_SDK_PATH% (
	echo "ERROR PICO_SDK_PATH env. not active."
)
if not exist build (
	mkdir build
)
cd build
cmake -G "NMake Makefiles" ..
