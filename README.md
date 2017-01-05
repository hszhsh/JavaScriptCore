# JavaScriptCore

The JavaScriptCore library is part of the [WebKit project](http://www.webkit.org/) and thus Open Source. However, in the sources you get from the [WebKit SVN](https://svn.webkit.org/repository/webkit/trunk), the XCode project files are curiously missing an iOS compile target. The sources you get from [opensource.apple.com](http://opensource.apple.com/release/ios-601/) are missing the project files altogether. You can't compile it at all. 

This repo aims to re-produce the missing iOS targets while staying on a somewhat up-to-date version.


## How to Compile for iOS

1. Open `WebKit.xcworkspace`.
2. Build target `JavaScriptCore_iOS`.
3. If error occurs, go to DerivedData directory and change libJSCLLIntOffsetsExtractor.a to JSCLLIntOffsetsExtractor.
4. Rebuild the target and `JavaScriptCore.framework` will be generated.

## How to Compile for Android
1. Install CMake 3.6.x
2. Set ANDROID_NDK variable to you android ndk path (I'm using ndk version r13b)<br/>`export ANDROID_NDK=path/to/ndk`
3. mkdir build_android && cd build_android
4. Run cmake command (make sure you have perf, bison, python, ruby and perl installed)
	```
	#armeabi
    cmake -DCMAKE_TOOLCHAIN_FILE="../android_toolchain/android.toolchain.cmake" -DANDROID_NATIVE_API_LEVEL="16" -DANDROID_NDK=${ANDROID_NDK} -DANDROID_ABI=armeabi -DANDROID_ARM_MODE=arm -DCMAKE_BUILD_TYPE=MinSizeRel -DANDROID=TRUE -DANDROID_STL=gnustl_static -DENABLE_WEBKIT=0 -DENABLE_WEBCORE=0 -DENABLE_TOOLS=0 -DENABLE_WEBKIT=0 -DENABLE_WEBKIT2=0 -DENABLE_API_TESTS=0 -DPORT=Android -DPYTHON_EXECUTABLE=$(which python) -DPERL_EXECUTABLE=$(which perl) -DRUBY_EXECUTABLE=$(which ruby) -DBISON_EXECUTABLE=$(which bison) -DGPERF_EXECUTABLE=$(which Gperf) -DSHARED_CORE=0 -DENABLE_JIT=0 -DCMAKE_CXX_FLAGS="-D__STDC_LIMIT_MACROS" -DCMAKE_INSTALL_PREFIX=../AndroidModulesRelease/JavaScriptCore ..
	
	#armeabi-v7a
	cmake -DCMAKE_TOOLCHAIN_FILE="../android_toolchain/android.toolchain.cmake" -DANDROID_NATIVE_API_LEVEL="16" -DANDROID_NDK=${ANDROID_NDK} -DANDROID_ABI=armeabi-v7a -DANDROID_ARM_MODE=thumb -DCMAKE_BUILD_TYPE=MinSizeRel -DANDROID=TRUE -DANDROID_STL=gnustl_static -DENABLE_WEBKIT=0 -DENABLE_WEBCORE=0 -DENABLE_TOOLS=0 -DENABLE_WEBKIT=0 -DENABLE_WEBKIT2=0 -DENABLE_API_TESTS=0 -DPORT=Android -DPYTHON_EXECUTABLE=$(which python) -DPERL_EXECUTABLE=$(which perl) -DRUBY_EXECUTABLE=$(which ruby) -DBISON_EXECUTABLE=$(which bison) -DGPERF_EXECUTABLE=$(which Gperf) -DSHARED_CORE=0 -DENABLE_JIT=0 -DCMAKE_CXX_FLAGS="-D__STDC_LIMIT_MACROS" -DCMAKE_INSTALL_PREFIX=../AndroidModulesRelease/JavaScriptCore ..
	
	#arm64-v8a
	cmake -DCMAKE_TOOLCHAIN_FILE="../android_toolchain/android.toolchain.cmake" -DANDROID_NATIVE_API_LEVEL="21" -DANDROID_NDK=${ANDROID_NDK} -DANDROID_ABI=arm64-v8a -DCMAKE_BUILD_TYPE=MinSizeRel -DANDROID=TRUE -DANDROID_STL=gnustl_static -DENABLE_WEBKIT=0 -DENABLE_WEBCORE=0 -DENABLE_TOOLS=0 -DENABLE_WEBKIT=0 -DENABLE_WEBKIT2=0 -DENABLE_API_TESTS=0 -DPORT=Android -DPYTHON_EXECUTABLE=$(which python) -DPERL_EXECUTABLE=$(which perl) -DRUBY_EXECUTABLE=$(which ruby) -DBISON_EXECUTABLE=$(which bison) -DGPERF_EXECUTABLE=$(which Gperf) -DSHARED_CORE=0 -DENABLE_JIT=1 -DCMAKE_CXX_FLAGS="-D__STDC_LIMIT_MACROS" -DCMAKE_INSTALL_PREFIX=../AndroidModulesRelease/JavaScriptCore ..
    ```
5. make, ar and strip your library.
