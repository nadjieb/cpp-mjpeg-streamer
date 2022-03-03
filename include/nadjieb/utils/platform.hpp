#pragma once

#if defined _MSC_VER || defined __MINGW32__
#define NADJIEB_MJPEG_STREAMER_PLATFORM_WINDOWS
#pragma comment(lib, "ws2_32")
#elif defined __APPLE_CC__ || defined __APPLE__
#define NADJIEB_MJPEG_STREAMER_PLATFORM_DARWIN
#else
#define NADJIEB_MJPEG_STREAMER_PLATFORM_LINUX
#endif
