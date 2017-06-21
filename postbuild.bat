:: Postbuild script for Kino. Moves all the custom dll 
:: dependencies from /frameworks to /bin.

echo "---- Kino Postbuild Script START ----"

set roboArgs=/njs /njh /np /fp

:: Libusb
robocopy "frameworks/libusb/dll/" "bin/" "libusb-1.0.dll" %roboArgs%

:: OpenCV
robocopy "frameworks/opencv/dll/" "bin/" "opencv_world320.dll" %roboArgs%
robocopy "frameworks/opencv/dll/" "bin/" "opencv_world320d.dll" %roboArgs%
robocopy "frameworks/opencv/dll/" "bin/" "opencv_ffmpeg320_64.dll" %roboArgs%

:: Darknet
robocopy "addons/ofxDarknet/libs/3rdparty/dll/x64/" "bin/" "pthreadVC2.dll" %roboArgs%

echo "----- Kino Postbuild Script END -----"