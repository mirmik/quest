#!/usr/bin/env python
import licant
import licant.java_make

toolpath = "/home/mirmik/Android/Sdk/ndk/22.1.7171670/toolchains/llvm/prebuilt/linux-x86_64/bin/"

toolchain = licant.cxx_make.toolchain(
	cxx = toolpath + "aarch64-linux-android26-clang",
	cc = toolpath + "aarch64-linux-android26-clang"
)

VARS = {
	"NDK_HOME" : "~/Android/Sdk/ndk/22.1.7171670/",
	"OVR_HOME" : "~/src/ovr_sdk_mobile_1.44.0",
	"ANDROID_HOME" : "~/Android/Sdk/"
}

licant.cxx_shared_library("libmain.so",
	toolchain = toolchain,
	sources = [
		"src/main/cpp/*.c"
	],
	include_paths = [
		"{NDK_HOME}/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/include/".format(**VARS),
		"{OVR_HOME}/VrApi/Include".format(**VARS),
		"{NDK_HOME}/platforms/android-26/arch-arm64/usr/lib".format(**VARS),
		"{OVR_HOME}/VrApi/Libs/Android/arm64-v8a/Debug".format(**VARS)
	],
	libdirs = [
		"{NDK_HOME}/platforms/android-26/arch-arm64/usr/lib".format(**VARS),
		"{OVR_HOME}/VrApi/Libs/Android/arm64-v8a/Debug".format(**VARS)
	],
	libs = ["android", "log", "vrapi"]
)


jtools = licant.java_make.android_toolchain(
	javac = "/home/mirmik/soft/android-studio/jre/bin/javac",
	dx = "/home/mirmik/Android/Sdk/build-tools/30.0.3/dx"
)

joptions = licant.java_make.java_options(
	toolchain = jtools,
	javac_flags = "-classpath {ANDROID_HOME}/platforms/android-26/android.jar".format(**VARS)
)

licant.java_make.javac(
	opts = joptions,
	tgt="src/main/java/com/makepad/hello_quest/MainActivity.class", 
	srcs=["src/main/java/com/makepad/hello_quest/MainActivity.java"]
)

licant.java_make.dex(
	opts = joptions,
	tgt = "classes.dex",
	src = "src/main/java"
)


licant.copy(
	src = "{OVR_HOME}/VrApi/Libs/Android/arm64-v8a/Debug/libvrapi.so".format(**VARS),
	tgt = "libvrapi.so"
)

#licant.android_apk(
#	opts=joptions,
#	srcs=[
#		"classes.dex",
#		"libmain.so",
#		"libvrapi.so"
#	]
#)

licant.ex(
	"hello_quest.apk"
)