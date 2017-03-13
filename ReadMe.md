# JavaScriptCore

The JavaScriptCore library is part of the [WebKit project](http://www.webkit.org/) and thus Open Source.

This repo aims to building JavaScriptCore for Android while staying on a somewhat up-to-date version.


## Build for iOS

1. Run sudo Tools/Scripts/configure-xcode-for-ios-development in the Terminal to enable Xcode to build command line tools for iOS. Otherwise you will see the error message: target specifies product type ‘com.apple.product-type.tool’.
2. Open WebKit.xcworkspace, and choose JavaScriptCore target (Mac, iOS device or simulator).

## Build for Android
1. Install CMake 3.6.x or use cmake in ndk's directory.
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
	
	#x86
	cmake -DCMAKE_TOOLCHAIN_FILE="../android_toolchain/android.toolchain.cmake" -DANDROID_NATIVE_API_LEVEL="16" -DANDROID_NDK=${ANDROID_NDK} -DANDROID_ABI=x86 -DCMAKE_BUILD_TYPE=MinSizeRel -DANDROID=TRUE -DANDROID_STL=gnustl_static -DENABLE_WEBKIT=0 -DENABLE_WEBCORE=0 -DENABLE_TOOLS=0 -DENABLE_WEBKIT=0 -DENABLE_WEBKIT2=0 -DENABLE_API_TESTS=0 -DPORT=Android -DPYTHON_EXECUTABLE=$(which python) -DPERL_EXECUTABLE=$(which perl) -DRUBY_EXECUTABLE=$(which ruby) -DBISON_EXECUTABLE=$(which bison) -DGPERF_EXECUTABLE=$(which Gperf) -DSHARED_CORE=0 -DENABLE_JIT=1 -DCMAKE_CXX_FLAGS="-D__STDC_LIMIT_MACROS" -DCMAKE_INSTALL_PREFIX=../AndroidModulesRelease/JavaScriptCore ..
    ```
5. make, ar and strip your library.

I'm using WTF's RunLoop for GC timers on Android, so you need to call RunLoop::iterate() in your app's runloop on js thread.

You can checkout the prebuilt branch to use my prebuilt libraries for Mac, iOS and Android.
