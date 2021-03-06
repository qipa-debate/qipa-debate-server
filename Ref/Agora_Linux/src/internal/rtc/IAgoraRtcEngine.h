//
//  Agora Rtc Engine SDK
//
//  Created by Sting Feng in 2015-02.
//  Copyright (c) 2015 Agora IO. All rights reserved.
//

#ifndef AGORA_RTC_ENGINE_H
#define AGORA_RTC_ENGINE_H

#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define AGORA_CALL __cdecl
#if defined(AGORARTC_EXPORT)
#define AGORA_API extern "C" __declspec(dllexport)
#else
#define AGORA_API extern "C" __declspec(dllimport)
#endif
#elif defined(__APPLE__)
#define AGORA_API __attribute__((visibility("default"))) extern "C"
#define AGORA_CALL
#else
#define AGORA_API extern "C"
#define AGORA_CALL
#endif

namespace agora {
    namespace util {

template<class T>
class AutoPtr {
    typedef T value_type;
    typedef T* pointer_type;
public:
    AutoPtr(pointer_type p=0)
        :ptr_(p)
    {}
    ~AutoPtr() {
        if (ptr_)
            ptr_->release();
    }
    operator bool() const { return ptr_ != (pointer_type)0; }
    value_type& operator*() const {
        return *get();
    }

    pointer_type operator->() const {
        return get();
    }

    pointer_type get() const {
        return ptr_;
    }

    pointer_type release() {
        pointer_type tmp = ptr_;
        ptr_ = 0;
        return tmp;
    }

    void reset(pointer_type ptr = 0) {
        if (ptr != ptr_ && ptr_)
            ptr_->release();
        ptr_ = ptr;
    }
private:
    AutoPtr(const AutoPtr&);
    AutoPtr& operator=(const AutoPtr&);
private:
    pointer_type ptr_;
};
class IString {
public:
    virtual bool empty() const = 0;
    virtual const char* c_str() = 0;
    virtual const char* data() = 0;
    virtual size_t length() = 0;
    virtual void release() = 0;
};
typedef AutoPtr<IString> AString;

    }//namespace util
namespace rtc {
    typedef unsigned int uid_t;
    typedef void* view_t;

enum INTERFACE_ID_TYPE
{
    AGORA_IID_AUDIO_DEVICE_MANAGER = 1,
    AGORA_IID_VIDEO_DEVICE_MANAGER = 2,
    AGORA_IID_RTC_ENGINE_PARAMETER = 3,
};

enum WARN_CODE_TYPE
{
    WARN_NO_AVAILABLE_CHANNEL = 103,
    WARN_LOOKUP_CHANNEL_TIMEOUT = 104,
    WARN_LOOKUP_CHANNEL_REJECTED = 105,
    WARN_OPEN_CHANNEL_TIMEOUT = 106,
    WARN_OPEN_CHANNEL_REJECTED = 107,
    WARN_REQUEST_DEFERRED = 108,
    WARN_ADM_RUNTIME_PLAYOUT_WARNING = 1014,
    WARN_ADM_RUNTIME_RECORDING_WARNING = 1016,
    WARN_ADM_RECORD_AUDIO_SILENCE = 1019,
    WARN_ADM_PLAYOUT_MALFUNCTION = 1020,
    WARN_ADM_RECORD_MALFUNCTION = 1021,
    WARN_APM_HOWLING = 1051,
};

enum ERROR_CODE_TYPE
{
    ERR_OK = 0,
    //1~1000
    ERR_FAILED = 1,
    ERR_INVALID_ARGUMENT = 2,
    ERR_NOT_READY = 3,
    ERR_NOT_SUPPORTED = 4,
    ERR_REFUSED = 5,
    ERR_BUFFER_TOO_SMALL = 6,
    ERR_NOT_INITIALIZED = 7,
    ERR_INVALID_VIEW = 8,
    ERR_NO_PERMISSION = 9,
    ERR_TIMEDOUT = 10,
    ERR_CANCELED = 11,
    ERR_TOO_OFTEN = 12,
    ERR_BIND_SOCKET = 13,
    ERR_NET_DOWN = 14,
    ERR_NET_NOBUFS = 15,
    ERR_INIT_VIDEO = 16,
    ERR_JOIN_CHANNEL_REJECTED = 17,
    ERR_LEAVE_CHANNEL_REJECTED = 18,
	ERR_ALREADY_IN_USE = 19,
    ERR_INVALID_VENDOR_KEY = 101,
    ERR_INVALID_CHANNEL_NAME = 102,
    ERR_DYNAMIC_KEY_TIMEOUT = 109,
    ERR_INVALID_DYNAMIC_KEY = 110,
    //1001~2000
    ERR_LOAD_MEDIA_ENGINE = 1001,
    ERR_START_CALL = 1002,
    ERR_START_CAMERA = 1003,
    ERR_START_VIDEO_RENDER = 1004,
    ERR_ADM_GENERAL_ERROR = 1005,
    ERR_ADM_JAVA_RESOURCE = 1006,
    ERR_ADM_SAMPLE_RATE = 1007,
    ERR_ADM_INIT_PLAYOUT = 1008,
    ERR_ADM_START_PLAYOUT = 1009,
    ERR_ADM_STOP_PLAYOUT = 1010,
    ERR_ADM_INIT_RECORDING = 1011,
    ERR_ADM_START_RECORDING = 1012,
    ERR_ADM_STOP_RECORDING = 1013,
    ERR_ADM_RUNTIME_PLAYOUT_ERROR = 1015,
    ERR_ADM_RUNTIME_RECORDING_ERROR = 1017,
    ERR_ADM_RECORD_AUDIO_FAILED = 1018,
    ERR_ADM_INIT_LOOPBACK  = 1022,
    ERR_ADM_START_LOOPBACK  = 1023,
    // 1025, as warning for interruption of adm on ios
    // 1026, as warning for route change of adm on ios
  
    // VDM error code starts from 1500
    ERR_VDM_CAMERA_NOT_AUTHORIZED  = 1501,
};

enum LOG_FILTER_TYPE
{
	LOG_FILTER_CONSOLE = 0x8000,
    LOG_FILTER_DEBUG = 0x0800,
    LOG_FILTER_INFO = 0x0001,
    LOG_FILTER_WARN = 0x0002,
    LOG_FILTER_ERROR = 0x0004,
    LOG_FILTER_CRITICAL = 0x0008,
	LOG_FILTER_MASK = 0x880f,
};

enum MAX_DEVICE_ID_LENGTH_TYPE
{
    MAX_DEVICE_ID_LENGTH = 128
};

enum QUALITY_REPORT_FORMAT_TYPE
{
    QUALITY_REPORT_JSON = 0,
    QUALITY_REPORT_HTML = 1,
};

enum MEDIA_ENGINE_EVENT_CODE_TYPE
{
    MEDIA_ENGINE_RECORDING_ERROR = 0,
    MEDIA_ENGINE_PLAYOUT_ERROR = 1,
    MEDIA_ENGINE_RECORDING_WARNING = 2,
    MEDIA_ENGINE_PLAYOUT_WARNING = 3
};

enum MEDIA_DEVICE_STATE_TYPE
{
    MEDIA_DEVICE_STATE_ACTIVE = 1,
    MEDIA_DEVICE_STATE_DISABLED = 2,
    MEDIA_DEVICE_STATE_NOT_PRESENT = 4,
    MEDIA_DEVICE_STATE_UNPLUGGED = 8
};

enum MEDIA_DEVICE_TYPE
{
    UNKNOWN_AUDIO_DEVICE = -1,
    AUDIO_PLAYOUT_DEVICE = 0,
    AUDIO_RECORDING_DEVICE = 1,
    VIDEO_RENDER_DEVICE = 2,
    VIDEO_CAPTURE_DEVICE = 3,
};

enum QUALITY_TYPE
{
    QUALITY_UNKNOWN = 0,
    QUALITY_EXCELLENT = 1,
    QUALITY_GOOD = 2,
    QUALITY_POOR = 3,
    QUALITY_BAD = 4,
    QUALITY_VBAD = 5,
    QUALITY_DOWN = 6,
};

enum RENDER_MODE_TYPE
{
    RENDER_MODE_HIDDEN = 1,
    RENDER_MODE_FIT = 2,
    RENDER_MODE_ADAPTIVE = 3,
};

enum VIDEO_PROFILE_TYPE
{                                   // res       fps  kbps
    VIDEO_PROFILE_120P = 0,         // 160x120   15   80
    VIDEO_PROFILE_120P_2 = 1,        // 120x160   15   80
    VIDEO_PROFILE_120P_3 = 2,        // 120x120   15   60
    VIDEO_PROFILE_180P = 10,        // 320x180   15   160
    VIDEO_PROFILE_180P_2 = 11,        // 180x320   15   160
    VIDEO_PROFILE_180P_3 = 12,        // 180x180   15   120
    VIDEO_PROFILE_240P = 20,        // 320x240   15   200
    VIDEO_PROFILE_240P_2 = 21,        // 240x320   15   200
    VIDEO_PROFILE_240P_3 = 22,        // 240x240   15   160
    VIDEO_PROFILE_360P = 30,        // 640x360   15   400
    VIDEO_PROFILE_360P_2 = 31,        // 360x640   15   400
    VIDEO_PROFILE_360P_3 = 32,        // 360x360   15   300
    VIDEO_PROFILE_360P_4 = 33,        // 640x360   30   800
    VIDEO_PROFILE_360P_5 = 34,        // 360x640   30   800
    VIDEO_PROFILE_360P_6 = 35,        // 360x360   30   600
    VIDEO_PROFILE_480P = 40,        // 640x480   15   500
    VIDEO_PROFILE_480P_2 = 41,        // 480x640   15   500
    VIDEO_PROFILE_480P_3 = 42,        // 480x480   15   400
    VIDEO_PROFILE_480P_4 = 43,        // 640x480   30   1000
    VIDEO_PROFILE_480P_5 = 44,        // 480x640   30   1000
    VIDEO_PROFILE_480P_6 = 45,        // 480x480   30   800
	VIDEO_PROFILE_480P_7 = 46,		// 640x480 15 1000
	VIDEO_PROFILE_720P = 50,        // 1280x720  15   1000
    VIDEO_PROFILE_720P_2 = 51,        // 720x1280  15   1000
    VIDEO_PROFILE_720P_3 = 52,        // 1280x720  30   2000
    VIDEO_PROFILE_720P_4 = 53,        // 720x1280  30   2000
    VIDEO_PROFILE_1080P = 60,        // 1920x1080 15   1500
    VIDEO_PROFILE_1080P_2 = 61,        // 1080x1920 15   1500
    VIDEO_PROFILE_1080P_3 = 62,        // 1920x1080 30   3000
    VIDEO_PROFILE_1080P_4 = 63,        // 1080x1920 30   3000
    VIDEO_PROFILE_1080P_5 = 64,        // 1920x1080 60   6000
    VIDEO_PROFILE_1080P_6 = 65,        // 1080x1920 60   6000
    VIDEO_PROFILE_4K = 70,            // 3840x2160 30   8000
    VIDEO_PROFILE_4K_2 = 71,        // 2160x3080 30   8000
    VIDEO_PROFILE_4K_3 = 72,        // 3840x2160 60   16000
    VIDEO_PROFILE_4K_4 = 73,        // 2160x3840 60   16000
    VIDEO_PROFILE_DEFAULT = VIDEO_PROFILE_360P,
};

enum CHANNEL_PROFILE_TYPE
{
    CHANNEL_PROFILE_FREE = 0,
    CHANNEL_PROFILE_BROADCASTER = 1,
    CHANNEL_PROFILE_AUDIENCE = 2,
};

enum USER_OFFLINE_REASON_TYPE
{
    USER_OFFLINE_QUIT = 0,
    USER_OFFLINE_DROPPED = 1,
};

struct AudioVolumeInfo
{
    uid_t uid;
    unsigned int volume; // [0,255]
};

struct RtcStats
{
    unsigned int duration;
    unsigned int txBytes;
    unsigned int rxBytes;
    unsigned short txKBitRate;
    unsigned short rxKBitRate;
    unsigned int lastmileQuality;
    unsigned int users;
    double cpuAppUsage;
    double cpuTotalUsage;
};

struct LocalVideoStats
{
    int sentBitrate;
    int sentFrameRate;
};

struct RemoteVideoStats
{
    uid_t uid;
    int delay;
	int width;
	int height;
	int receivedBitrate;
	int receivedFrameRate;
};

#if !defined(__ANDROID__)
struct VideoCanvas
{
    view_t view;
    int renderMode;
    uid_t uid;
    void *priv; // private data (underlying video engine denotes it)

    VideoCanvas()
        : view(NULL)
        , renderMode(RENDER_MODE_HIDDEN)
        , uid(0)
        , priv(NULL)
    {}
    VideoCanvas(view_t v, int m, uid_t u)
        : view(v)
        , renderMode(m)
        , uid(u)
        , priv(NULL)
    {}
};
#else
struct VideoCanvas;
#endif

class IPacketObserver
{
public:

	struct Packet
	{
		const unsigned char* buffer;
		unsigned int size;
	};
	/**
	* called by sdk before the audio packet is sent to other participants
	* @param [in,out] packet:
	*      buffer *buffer points the data to be sent
	*      size of buffer data to be sent
	* @return returns true to send out the packet, returns false to discard the packet
	*/
	virtual bool onSendAudioPacket(Packet& packet) = 0;
	/**
	* called by sdk before the video packet is sent to other participants
	* @param [in,out] packet:
	*      buffer *buffer points the data to be sent
	*      size of buffer data to be sent
	* @return returns true to send out the packet, returns false to discard the packet
	*/
	virtual bool onSendVideoPacket(Packet& packet) = 0;
	/**
	* called by sdk when the audio packet is received from other participants
	* @param [in,out] packet
	*      buffer *buffer points the data to be sent
	*      size of buffer data to be sent
	* @return returns true to process the packet, returns false to discard the packet
	*/
	virtual bool onReceiveAudioPacket(Packet& packet) = 0;
	/**
	* called by sdk when the video packet is received from other participants
	* @param [in,out] packet
	*      buffer *buffer points the data to be sent
	*      size of buffer data to be sent
	* @return returns true to process the packet, returns false to discard the packet
	*/
	virtual bool onReceiveVideoPacket(Packet& packet) = 0;
};


/**
* the event call back interface
*/
class IRtcEngineEventHandler
{
public:
    virtual ~IRtcEngineEventHandler() {}

    /**
    * when join channel success, the function will be called
    * @param [in] channel
    *        the channel name you have joined
    * @param [in] uid_t
    *        the UID of you in this channel
    * @param [in] elapsed
    *        the time elapsed in ms from the joinChannel been called to joining completed
    */
    virtual void onJoinChannelSuccess(const char* channel, uid_t uid, int elapsed) {
        (void)channel;
        (void)uid;
        (void)elapsed;
    }

    /**
    * when join channel success, the function will be called
    * @param [in] channel
    *        the channel name you have joined
    * @param [in] uid_t
    *        the UID of you in this channel
    * @param [in] elapsed
    *        the time elapsed in ms elapsed
    */
    virtual void onRejoinChannelSuccess(const char* channel, uid_t uid, int elapsed) {
        (void)channel;
        (void)uid;
        (void)elapsed;
    }

    /**
    * when warning message coming, the function will be called
    * @param [in] warn
    *        warning code
    * @param [in] msg
    *        the warning message
    */
    virtual void onWarning(int warn, const char* msg) {
        (void)warn;
        (void)msg;
    }

    /**
    * when error message come, the function will be called
    * @param [in] err
    *        error code
    * @param [in] msg
    *        the error message
    */
    virtual void onError(int err, const char* msg) {
        (void)err;
        (void)msg;
    }

    /**
    * when audio quality message come, the function will be called
    * @param [in] uid
    *        the uid of the peer
    * @param [in] quality
    *        the quality of the user 0~5 the higher the better
    * @param [in] delay
    *        the average time of the audio packages delayed
    * @param [in] lost
    *        the rate of the audio packages lost
    */
    virtual void onAudioQuality(uid_t uid, int quality, unsigned short delay, unsigned short lost) {
        (void)uid;
        (void)quality;
        (void)delay;
        (void)lost;
    }

    /**
    * when the audio volume information come, the function will be called
    * @param [in] speakers
    *        the array of the speakers' audio volume information
    * @param [in] speakerNumber
    *        the count of speakers in this array
    * @param [in] totalVolume
    *        the total volume of all users
    */
    virtual void onAudioVolumeIndication(const AudioVolumeInfo* speakers, unsigned int speakerNumber, int totalVolume) {
        (void)speakers;
        (void)speakerNumber;
        (void)totalVolume;
    }

    /**
    * when the audio volume information come, the function will be called
    * @param [in] speakers
    *        the array of the speakers' audio volume information
    * @param [in] speakerNumber
    *        the count of speakers in this array
    * @param [in] totalVolume
    *        the total volume of all users
    */
    virtual void onLeaveChannel(const RtcStats& stat) {
        (void)stat;
    }

    /**
    * when the information of the RTC engine stats come, the function will be called
    * @param [in] stat
    *        the RTC engine stats
    */
    virtual void onRtcStats(const RtcStats& stat) {
        (void)stat;
    }

    /**
    * when the audio device state changed(plugged or removed), the function will be called
    * @param [in] deviceId
    *        the ID of the state changed audio device
    * @param [in] deviceType
    *        the type of the audio device(playout device or record device)
    * @param [in] deviceState
    *        the device is been removed or added
    */
    virtual void onAudioDeviceStateChanged(const char* deviceId, int deviceType, int deviceState) {
        (void)deviceId;
        (void)deviceType;
        (void)deviceState;
    }

    /**
    * when the video device state changed(plugged or removed), the function will be called
    * @param [in] deviceId
    *        the ID of the state changed video device
    * @param [in] deviceType
    *        not used
    * @param [in] deviceState
    *        the device is been removed or added
    */
    virtual void onVideoDeviceStateChanged(const char* deviceId, int deviceType, int deviceState) {
        (void)deviceId;
        (void)deviceType;
        (void)deviceState;
    }

    /**
    * report the network quality
    * @param [in] quality
    *        the score of the network quality 0~5 the higher the better
    */
    virtual void onNetworkQuality(int quality) {
        (void)quality;
    }

    /**
    * when the first local video frame displayed, the function will be called
    * @param [in] width
    *        the width of the video frame
    * @param [in] height
    *        the height of the video frame
    * @param [in] elapsed
    *        the time elapsed from channel joined in ms
    */
    virtual void onFirstLocalVideoFrame(int width, int height, int elapsed) {
        (void)width;
        (void)height;
        (void)elapsed;
    }

    /**
    * when the first remote video frame decoded, the function will be called
    * @param [in] uid
    *        the UID of the remote user
    * @param [in] width
    *        the width of the video frame
    * @param [in] height
    *        the height of the video frame
    * @param [in] elapsed
    *        the time elapsed from channel joined in ms
    */
    virtual void onFirstRemoteVideoDecoded(uid_t uid, int width, int height, int elapsed) {
        (void)uid;
        (void)width;
        (void)height;
        (void)elapsed;
    }

    /**
    * when the first remote video frame displayed, the function will be called
    * @param [in] uid
    *        the UID of the remote user
    * @param [in] width
    *        the width of the video frame
    * @param [in] height
    *        the height of the video frame
    * @param [in] elapsed
    *        the time elapsed from remote user called joinChannel in ms
    */
    virtual void onFirstRemoteVideoFrame(uid_t uid, int width, int height, int elapsed) {
        (void)uid;
        (void)width;
        (void)height;
        (void)elapsed;
    }

    /**
    * when any other user joined in the same channel, the function will be called
    * @param [in] uid
    *        the UID of the remote user
    * @param [in] elapsed
    *        the time elapsed from the
  *    @param [in] height
    *        the height of the video frame
    * @param [in] elapsed
    *        the time elapsed from remote used called joinChannel to joining completed in ms
    */
    virtual void onUserJoined(uid_t uid, int elapsed) {
        (void)uid;
        (void)elapsed;
    }

    /**
    * when user offline(exit channel or offline by accident), the function will be called
    * @param [in] uid
    *        the UID of the remote user
    */
    virtual void onUserOffline(uid_t uid, USER_OFFLINE_REASON_TYPE reason) {
        (void)uid;
        (void)reason;
    }

    /**
    * when remote user muted the audio stream, the function will be called
    * @param [in] uid
    *        the UID of the remote user
    * @param [in] muted
    *        true: the remote user muted the audio stream, false: the remote user unmuted the audio stream
    */
    virtual void onUserMuteAudio(uid_t uid, bool muted) {
        (void)uid;
        (void)muted;
    }

    /**
    * when remote user muted the video stream, the function will be called
    * @param [in] uid
    *        the UID of the remote user
    * @param [in] muted
    *        true: the remote user muted the video stream, false: the remote user unmuted the video stream
    */
    virtual void onUserMuteVideo(uid_t uid, bool muted) {
        (void)uid;
        (void)muted;
    }

	/**
	* when remote user enable video function, the function will be called
	* @param [in] uid
	*        the UID of the remote user
	* @param [in] enabled
	*        true: the remote user has enabled video function, false: the remote user has disabled video function
	*/
	virtual void onUserEnableVideo(uid_t uid, bool enabled) {
		(void)uid;
		(void)enabled;
	}
	
	/**
    * when api call executed completely, the function will be called
    * @param [in] api
    *        the api name
    * @param [in] error
    *        error code while 0 means OK
    */
    virtual void onApiCallExecuted(const char* api, int error) {
        (void)api;
        (void)error;
    }

    /**
	* reported local video stats
	* @param [in] stats
    *        the latest local video stats
    */
	virtual void onLocalVideoStats(const LocalVideoStats& stats) {
		(void)stats;
    }

    /**
    * reported remote video stats
    * @param [in] stats
	*        the latest remote video stats
	*/
	virtual void onRemoteVideoStats(const RemoteVideoStats& stats) {
		(void)stats;
    }

    /**
    * when the camera is ready to work, the function will be called
    */
    virtual void onCameraReady() {}

    /**
    * when all video stopped, the function will be called then you can repaint the video windows
    */
    virtual void onVideoStopped() {}

    /**
    * when the network can not worked well, the function will be called
    */
    virtual void onConnectionLost() {}

    /**
    * when local user disconnected by accident, the function will be called(then SDK will try to reconnect itself)
    */
    virtual void onConnectionInterrupted() {}
    
	virtual void onStartRecordingService(int error) {
		(void)error;
    }
    
	virtual void onStopRecordingService(int error) {
		(void)error;
    }
    
    virtual void onRefreshRecordingServiceStatus(int status, int error) {
        (void)status;
		(void)error;
    }
};

/**
* the video device collection interface
*/
class IVideoDeviceCollection
{
public:
    /**
    * get the audio device count
    * @return returns the audio device count
    */
    virtual int getCount() = 0;

    /**
    * get audio device information
    * @param [in] index
    *        the index of the device in the device list
    * @param [in out] deviceName
    *        the device name, UTF8 format
    * @param [in out] deviceId
    *        the device ID, UTF8 format
    * @return return 0 if success or an error code
    */
    virtual int getDevice(int index, char deviceName[MAX_DEVICE_ID_LENGTH], char deviceId[MAX_DEVICE_ID_LENGTH]) = 0;

    /**
    * set current active audio device
    * @param [in] deviceId
    *        the deviceId of the device you want to active currently
    * @return return 0 if success or an error code
    */
    virtual int setDevice(const char deviceId[MAX_DEVICE_ID_LENGTH]) = 0;

    /**
    * release the resource
    */
    virtual void release() = 0;
};

class IVideoDeviceManager
{
public:

    /**
    * create the IVideoDeviceCollection interface pointer
    * @return return the IVideoDeviceCollection interface or nullptr if failed
    */
    virtual IVideoDeviceCollection* enumerateVideoDevices() = 0;

    /**
    * active the video device for current using
    * @param [in] deviceId
    *        the deviceId of the device you want to active currently
    * @return return 0 if success or the error code.
    */
    virtual int setDevice(const char deviceId[MAX_DEVICE_ID_LENGTH]) = 0;

    /**
    * get the current active video device
    * @param [in out] deviceId
    *        the device id of the current active video device
    * @return return 0 if success or an error code
    */
    virtual int getDevice(char deviceId[MAX_DEVICE_ID_LENGTH]) = 0;

    /**
    * test the video capture device to know whether it can worked well
    * @param [in] hwnd
    *        the HWND of the video-display window
    * @return return 0 if success or an error code
    */
    virtual int startDeviceTest(view_t hwnd) = 0;

    /**
    * stop the video device testing
    * @return return 0 if success or an error code
    */
    virtual int stopDeviceTest() = 0;

    /**
    * release the resource
    */
    virtual void release() = 0;
};

class IAudioDeviceCollection
{
public:
    /**
    * get the available devices count
    * @return return the device count
    */
    virtual int getCount() = 0;

    /**
    * get video device information
    * @param [in] index
    *        the index of the device in the device list
    * @param [in out] deviceName
    *        the device name, UTF8 format
    * @param [in out] deviceId
    *        the device ID, UTF8 format
    * @return return 0 if success or an error code
    */
    virtual int getDevice(int index, char deviceName[MAX_DEVICE_ID_LENGTH], char deviceId[MAX_DEVICE_ID_LENGTH]) = 0;

    /**
    * active the device for current using
    * @param [in] deviceId
    *        the deviceId of the device you want to active currently
    * @return return 0 if success or the error code.
    */
    virtual int setDevice(const char deviceId[MAX_DEVICE_ID_LENGTH]) = 0;

    /**
    * release the resource
    */
    virtual void release() = 0;
};

class IAudioDeviceManager
{
public:
    /**
    * create the IAudioDeviceCollection interface pointer of the playback devices
    * @return return the IVideoDeviceCollection interface or nullptr if failed
    */
    virtual IAudioDeviceCollection* enumeratePlaybackDevices() = 0;

    /**
    * create the IAudioDeviceCollection interface pointer of the Recording devices
    * @return return the IVideoDeviceCollection interface or nullptr if failed
    */
    virtual IAudioDeviceCollection* enumerateRecordingDevices() = 0;

    /**
    * active the playback device for current using
    * @param [in] deviceId
    *        the deviceId of the playback device you want to active currently
    * @return return 0 if success or the error code.
    */
    virtual int setPlaybackDevice(const char deviceId[MAX_DEVICE_ID_LENGTH]) = 0;

    /**
    * get the current active playback device
    * @param [in out] deviceId
    *        the device id of the current active video device
    * @return return 0 if success or an error code
    */
    virtual int getPlaybackDevice(char deviceId[MAX_DEVICE_ID_LENGTH]) = 0;

    /**
    * set current playback device volume
    * @param [in] volume
    *        the volume you want to set 0-255
    * @return return 0 if success or an error code
    */
    virtual int setPlaybackDeviceVolume(int volume) = 0;

    /**
    * get current playback device volume
    * @param [in out] *volume
    *        the current playback device volume 0-255
    * @return return 0 if success or an error code
    */
    virtual int getPlaybackDeviceVolume(int *volume) = 0;

    /**
    * active the recording audio device for current using
    * @param [in] deviceId
    *        the deviceId of the recording audio device you want to active currently
    * @return return 0 if success or the error code.
    */
    virtual int setRecordingDevice(const char deviceId[MAX_DEVICE_ID_LENGTH]) = 0;

    /**
    * get the current active recording device
    * @param [in out] deviceId
    *        the device id of the current active recording audio device
    * @return return 0 if success or an error code
    */
    virtual int getRecordingDevice(char deviceId[MAX_DEVICE_ID_LENGTH]) = 0;

    /**
    * set current recording device volume
    * @param [in] volume
    *        the volume you want to set 0-255
    * @return return 0 if success or an error code
    */
    virtual int setRecordingDeviceVolume(int volume) = 0;

    /**
    * get current recording device volume
    * @param [in out] *volume
    *        the current recording device volume 0-255
    * @return return 0 if success or an error code
    */
    virtual int getRecordingDeviceVolume(int *volume) = 0;

    /**
    * test the playback audio device to know whether it can worked well
    * @param [in] testAudioFilePath
    *        the path of the .wav file
    * @return return 0 if success and you can hear the sound of the .wav file or an error code.
    */
    virtual int startPlaybackDeviceTest(const char* testAudioFilePath) = 0;

    /**
    * stop the playback audio device testing
    * @return return 0 if success or an error code
    */
    virtual int stopPlaybackDeviceTest() = 0;

    /**
    * test the recording audio device to know whether it can worked well
    * @param [in] indicationInterval
    *        the period in ms of the call back cycle
    * @return return 0 if success or an error code
    */
    virtual int startRecordingDeviceTest(int indicationInterval) = 0;

    /**
    * stop the recording audio device testing
    * @return return 0 if success or an error code
    */
    virtual int stopRecordingDeviceTest() = 0;

    /**
    * release the resource
    */
    virtual void release() = 0;
};

struct RtcEngineContext
{
    IRtcEngineEventHandler* eventHandler;
    const char* vendorKey;
};


class IRtcEngine
{
public:
    /**
    * release the engine resource
    */
    virtual void release() = 0;

    /**
    * initialize the engine
    * @param [in] context
    *        the RTC engine context
    * @return return 0 if success or an error code
    */
    virtual int initialize(const RtcEngineContext& context) = 0;

    /**
    * get the pointer of the device manager object.
    * @param [in] iid
    *        the iid of the interface you want to get
    * @param [in out] inter
    *       the pointer of the pointer you want to point to DeviceManager object
    * @return return 0 if success or an error code
    */
    virtual int queryInterface(INTERFACE_ID_TYPE iid, void** inter) = 0;

    /**
    * get the version information of the SDK
    * @param [in out] build
    *        the build number
    * @return return the version number string in char format
    */
    virtual const char* getVersion(int* build) = 0;

    /**
    * get the version information of the SDK
    * @param [in out] build
    *        the build number
    * @return return the version number string in char format
    */
    virtual const char* getErrorDescription(int code) = 0;

    /**
    * join the channel, if the channel have not been created, it will been created automatically
  * @param [in] key
    *        the vendor key, if you have initialized the engine with an available vendor key, it can be null here
    * @param [in] channel
    *        the channel number
  * @param [in] info
    *        the additional information, it can be null here
    * @param [in] uid
    *        the uid of you, if 0 the system will automatically allocate one for you
    * @return return 0 if success or an error code
    */
    virtual int joinChannel(const char* key, const char* channel, const char* info, uid_t uid) = 0;

    /**
    * leave the current channel
    * @return return 0 if success or an error code
    */
    virtual int leaveChannel() = 0;

    /**
    * renew the dynamic key for the current channel, it used for dynamic key only
    * @param [in] key
    *        the dynamic key get from the authenticate server
    * @return return 0 if success or an error code
    */
    virtual int renewChannelDynamicKey(const char* key) = 0;


    virtual int setChannelProfile(CHANNEL_PROFILE_TYPE profile) = 0;

    /**
    * start the echo testing, if every thing goes well you can hear your echo from the server
    * @return return 0 if success or an error code
    */
    virtual int startEchoTest() = 0;

    /**
    * stop the echo testing
    * @return return 0 if success or an error code
    */
    virtual int stopEchoTest() = 0;

    /**
    * start the network testing
    * @return return 0 if success or an error code
    */
    virtual int enableNetworkTest() = 0;

    /**
    * stop the network testing
    * @return return 0 if success or an error code
    */
    virtual int disableNetworkTest() = 0;

    /**
    * enable local and remote video showing
    * @return return 0 if success or an error code
    */
    virtual int enableVideo() = 0;

    /**
    * disable local and remote video showing
    * @return return 0 if success or an error code
    */
    virtual int disableVideo() = 0;

    /**
    * start the local video previewing
    * @return return 0 if success or an error code
    */
    virtual int startPreview() = 0;

    /**
    * stop the local video previewing
    * @return return 0 if success or an error code
    */
    virtual int stopPreview() = 0;

    virtual int setVideoProfile(VIDEO_PROFILE_TYPE profile) = 0;
    /**
    * set the remote video canvas
    * @param [in] canvas
    *        the canvas information
    * @return return 0 if success or an error code
    */
    virtual int setupRemoteVideo(const VideoCanvas& canvas) = 0;

    /**
    * set the local video canvas
    * @param [in] canvas
    *        the canvas information
    * @return return 0 if success or an error code
    */
    virtual int setupLocalVideo(const VideoCanvas& canvas) = 0;

    /**
    * get self call id in the current channel
    * @param [in out] callId
    *        the self call Id
    * @return return 0 if success or an error code
    */
    virtual int getCallId(agora::util::AString& callId) = 0;

    virtual int rate(const char* callId, int rating, const char* description) = 0; // 0~10
    virtual int complain(const char* callId, const char* description) = 0;
    virtual int makeQualityReportUrl(const char* channel, uid_t listenerUid, uid_t speakerUid, QUALITY_REPORT_FORMAT_TYPE format, agora::util::AString& url) = 0;

    /**
    * register a packet observer while the packet arrived or ready to be sent, the observer can touch the packet data
    * @param [in] observer
    *        the pointer of the observer object
    * @return return 0 if success or an error code
    */
    virtual int registerPacketObserver(IPacketObserver* observer) = 0;

	virtual int setVideoRenderFactory(void* factory) = 0;
};


class IRtcEngineParameter
{
public:
    /**
    * release the resource
    */
    virtual void release() = 0;

    /**
    * set bool value of the json
    * @param [in] key
    *        the key name
    * @param [in] value
    *        the value
    * @return return 0 if success or an error code
    */
    virtual int setBool(const char* key, bool value) = 0;

    /**
    * set int value of the json
    * @param [in] key
    *        the key name
    * @param [in] value
    *        the value
    * @return return 0 if success or an error code
    */
    virtual int setInt(const char* key, int value) = 0;

    /**
    * set unsigned int value of the json
    * @param [in] key
    *        the key name
    * @param [in] value
    *        the value
    * @return return 0 if success or an error code
    */
    virtual int setUInt(const char* key, unsigned int value) = 0;

    /**
    * set double value of the json
    * @param [in] key
    *        the key name
    * @param [in] value
    *        the value
    * @return return 0 if success or an error code
    */
    virtual int setNumber(const char* key, double value) = 0;

    /**
    * set string value of the json
    * @param [in] key
    *        the key name
    * @param [in] value
    *        the value
    * @return return 0 if success or an error code
    */
    virtual int setString(const char* key, const char* value) = 0;

    /**
    * set object value of the json
    * @param [in] key
    *        the key name
    * @param [in] value
    *        the value
    * @return return 0 if success or an error code
    */
    virtual int setObject(const char* key, const char* value) = 0;

    /**
    * get bool value of the json
    * @param [in] key
    *        the key name
    * @param [in out] value
    *        the value
    * @return return 0 if success or an error code
    */
    virtual int getBool(const char* key, bool& value) = 0;

    /**
    * get int value of the json
    * @param [in] key
    *        the key name
    * @param [in out] value
    *        the value
    * @return return 0 if success or an error code
    */
    virtual int getInt(const char* key, int& value) = 0;

    /**
    * get unsigned int value of the json
    * @param [in] key
    *        the key name
    * @param [in out] value
    *        the value
    * @return return 0 if success or an error code
    */
    virtual int getUInt(const char* key, unsigned int& value) = 0;

    /**
    * get double value of the json
    * @param [in] key
    *        the key name
    * @param [in out] value
    *        the value
    * @return return 0 if success or an error code
    */
    virtual int getNumber(const char* key, double& value) = 0;

    /**
    * get string value of the json
    * @param [in] key
    *        the key name
    * @param [in out] value
    *        the value
    * @return return 0 if success or an error code
    */
    virtual int getString(const char* key, agora::util::AString& value) = 0;

    /**
    * get a child object value of the json
    * @param [in] key
    *        the key name
    * @param [in out] value
    *        the value
    * @return return 0 if success or an error code
    */
    virtual int getObject(const char* key, agora::util::AString& value) = 0;

    /**
    * get array value of the json
    * @param [in] key
    *        the key name
    * @param [in out] value
    *        the value
    * @return return 0 if success or an error code
    */
    virtual int getArray(const char* key, agora::util::AString& value) = 0;

    /**
    * set parameters of the sdk or engine
    * @param [in] parameters
    *        the parameters
    * @return return 0 if success or an error code
    */
    virtual int setParameters(const char* parameters) = 0;

    /**
    * set profile to control the RTC engine
    * @param [in] profile
    *        the profile
    * @param [in] merge
    *        if merge with the original value
    * @return return 0 if success or an error code
    */
    virtual int setProfile(const char* profile, bool merge) = 0;
};

class AAudioDeviceManager : public agora::util::AutoPtr<IAudioDeviceManager>
{
public:
    AAudioDeviceManager(IRtcEngine& engine)
    {
        IAudioDeviceManager* p;
        if (!engine.queryInterface(AGORA_IID_AUDIO_DEVICE_MANAGER, (void**)&p))
            reset(p);
    }
};

class AVideoDeviceManager : public agora::util::AutoPtr<IVideoDeviceManager>
{
public:
    AVideoDeviceManager(IRtcEngine& engine)
    {
        IVideoDeviceManager* p;
        if (!engine.queryInterface(AGORA_IID_VIDEO_DEVICE_MANAGER, (void**)&p))
            reset(p);
    }
};

class AParameter : public agora::util::AutoPtr<IRtcEngineParameter>
{
public:
    AParameter(IRtcEngine& engine)
    {
        IRtcEngineParameter* p;
        if (!engine.queryInterface(AGORA_IID_RTC_ENGINE_PARAMETER, (void**)&p))
            reset(p);
    }
    AParameter(IRtcEngineParameter* p)
        :agora::util::AutoPtr<IRtcEngineParameter>(p)
    {}
};

class RtcEngineParameters
{
public:
    RtcEngineParameters(IRtcEngine& engine)
        :m_parameter(engine){}

    /**
    * mute/unmute the local stream capturing
    * @param [in] mute
    *        true:  mute
    *       false: unmute
    * @return return 0 if success or an error code
    */
    int muteLocalAudioStream(bool mute) {
        return setParameters("{\"rtc.audio.mute_me\":%s,\"che.audio.mute_me\":%s}", mute ? "true" : "false", mute ? "true" : "false");
    }
    // mute/unmute all peers. unmute will clear all muted peers specified mutePeer() interface
  /**
    * mute/unmute all the remote audio stream receiving
    * @param [in] mute
    *        true:  mute
    *       false: unmute
    * @return return 0 if success or an error code
    */
    int muteAllRemoteAudioStreams(bool mute) {
        return m_parameter->setBool("rtc.audio.mute_peers", mute);
    }

    /**
    * mute/unmute one remote audio stream receiving
    * @param [in] uid
    *        the uid of the remote user you want to mute/unmute
    * @param [in] mute
    *        true:  mute
    *       false: unmute
    * @return return 0 if success or an error code
    */
    int muteRemoteAudioStream(uid_t uid, bool mute) {
        return setObject("rtc.audio.mute_peer", "{\"uid\":%u,\"mute\":%s}", uid, mute?"true":"false");
    }

    /**
    * mute/unmute local video stream sending
    * @param [in] mute
    *        true:  mute
    *       false: unmute
    * @return return 0 if success or an error code
    */
    int muteLocalVideoStream(bool mute) {
        return setParameters("{\"rtc.video.mute_me\":%s,\"che.video.local.send\":%s}", mute ? "true" : "false", mute ? "false" : "true");
    }

    /**
    * mute/unmute all the remote video stream receiving
    * @param [in] mute
    *        true:  mute
    *       false: unmute
    * @return return 0 if success or an error code
    */
    int muteAllRemoteVideoStreams(bool mute) {
        return m_parameter->setBool("rtc.video.mute_peers", mute);
    }

    /**
    * mute/unmute one remote video stream receiving
    * @param [in] uid
    *        the uid of the remote user you want to mute/unmute
    * @param [in] mute
    *        true:  mute
    *       false: unmute
    * @return return 0 if success or an error code
    */
    int muteRemoteVideoStream(uid_t uid, bool mute) {
        return setObject("rtc.video.mute_peer", "{\"uid\":%u,\"mute\":%s}", uid, mute ? "true" : "false");
    }

    /**
    * set play sound volume
    * @param [in] volume
    *        the volume 0~255
    * @return return 0 if success or an error code
    */
    int setPlaybackDeviceVolume(int volume) {// [0,255]
        return m_parameter->setInt("che.audio.output.volume", volume);
    }

    /**
    * enable or disable the audio volume indication
    * @param [in] interval
    *        the period of the call back cycle, in ms
    *        interval <= 0: disable
    *        interval >  0: enable
    * @param [in] smooth
    *        the smooth parameter
    * @return return 0 if success or an error code
    */
    int enableAudioVolumeIndication(int interval, int smooth) { // in ms: <= 0: disable, > 0: enable, interval in ms
        if (interval < 0)
            interval = 0;
        return setObject("che.audio.volume_indication", "{\"interval\":%d,\"smooth\":%d}", interval, smooth);
    }

    /**
    * start recording the audio stream
    * @param [in] filePath
    *        the .wav file path you want to saved
    * @return return 0 if success or an error code
    */
    int startAudioRecording(const char* filePath) {
        return m_parameter->setString("che.audio.start_recording", filePath);
    }

    /**
    * stop recording the audio stream
    * @return return 0 if success or an error code
    */
    int stopAudioRecording() {
        return m_parameter->setBool("che.audio.stop_recording", true);
    }

	/**
	* start screen capture
	* @return return 0 if success or an error code
	*/
	int startScreenCapture() {
		return m_parameter->setBool("che.video.screen_capture", true);
	}

	/**
	* stop screen capture
	* @return return 0 if success or an error code
	*/
	int stopScreenCapture() {
		return m_parameter->setBool("che.video.screen_capture", false);
	}

    /**
    * set path to save the log file
    * @param [in] filePath
    *        the .log file path you want to saved
    * @return return 0 if success or an error code
    */
    int setLogFile(const char* filePath) {
        return m_parameter->setString("rtc.log_file", filePath);
    }

    /**
    * set the log information filter level
    * @param [in] filter
    *        the filter level
    * @return return 0 if success or an error code
    */
    int setLogFilter(unsigned int filter) {
		return m_parameter->setUInt("rtc.log_filter", filter&LOG_FILTER_MASK);
    }

    /**
    * set local video render mode
    * @param [in] renderMode
    *        the render mode
    * @return return 0 if success or an error code
    */
    int setLocalRenderMode(RENDER_MODE_TYPE renderMode) {
        return setRemoteRenderMode(0, renderMode);
    }

    /**
    * set remote video render mode
    * @param [in] renderMode
    *        the render mode
    * @return return 0 if success or an error code
    */
    int setRemoteRenderMode(uid_t uid, RENDER_MODE_TYPE renderMode) {
        return setObject("che.video.peer.render_mode", "{\"uid\":%u,\"mode\":%d}", uid, renderMode);
    }
    
	int startRecordingService(const char* key) {
		return m_parameter->setString("rtc.recording_service.start", key);
    }
    
    int stopRecordingService(const char* key) {
        return m_parameter->setString("rtc.recording_service.stop", key);
    }
    
    int refreshRecordingServiceStatus() {
        return m_parameter->setBool("rtc.recording_service.refresh", true);
    }

protected:
    AParameter& parameter() {
        return m_parameter;
    }
    int setParameters(const char* format, ...) {
        char buf[512];
        va_list args;
        va_start(args, format);
        vsnprintf(buf, sizeof(buf)-1, format, args);
        va_end(args);
        return m_parameter->setParameters(buf);
    }
    int setObject(const char* key, const char* format, ...) {
        char buf[512];
        va_list args;
        va_start(args, format);
        vsnprintf(buf, sizeof(buf)-1, format, args);
        va_end(args);
        return m_parameter->setObject(key, buf);
    }
    int enableLocalVideoCapture(bool enabled) {
        return m_parameter->setBool("che.video.local.capture", enabled);
    }
    int enableLocalVideoRender(bool enabled) {
        return m_parameter->setBool("che.video.local.render", enabled);
    }
    int enableLocalVideoSend(bool enabled) {
        return muteLocalVideoStream(!enabled);
    }
    int stopAllRemoteVideo() {
        return m_parameter->setBool("che.video.peer.stop_render", true);
    }
private:
    AParameter m_parameter;
};

} //namespace rtc
} // namespace agora

/**
* to get the version number of the SDK
* @param [in out] build
*        the build number of Agora SDK
* @return returns the string of the version of the SDK
*/
AGORA_API const char* AGORA_CALL getAgoraRtcEngineVersion(int* build);

/**
* create the RTC engine object and return the pointer
* @return returns the pointer of the RTC engine object
*/
AGORA_API agora::rtc::IRtcEngine* AGORA_CALL createAgoraRtcEngine();

/**
* create the RTC engine object and return the pointer
* @param [in] err
*        the error code
* @return returns the description of the error code
*/
AGORA_API const char* AGORA_CALL getAgoraRtcEngineErrorDescription(int err);

#endif
