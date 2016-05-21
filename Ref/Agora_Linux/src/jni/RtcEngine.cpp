#include <jni.h>
#include <sys/time.h>
#include <cassert>

#include "RtcEngine.h"
#include "agora_sdk_i.h"

#include "Util.h"

using namespace std;
using namespace agora;
using namespace agora::pstn;

#define GET_JNI_ENV_OBJECT();                                   \
    JNIEnv* jni_env = NULL;                                     \
    CHECK_POINTER_VOID(jvm_, "mpVM is NULL!");                  \
    if (JNI_OK != jvm_->AttachCurrentThread((void **) &jni_env, NULL))    \
    {                                                           \
        LOG(ERROR, "can't execute AttachCurrentThread!");   \
        return;                                             \
    }

struct AgoraHandler : private IAgoraSdkEventHandler {
 public:
  AgoraHandler();
  ~AgoraHandler();

  bool SetJavaCallback(JNIEnv *env, jobject handler);
  IAgoraSdk* GetAgoraSdk() { return sdk_; }

  virtual void onJoinSuccess(const char *cname, uint32_t uid, const char *msg);
  virtual void onError(int rescode, const char *msg);
  virtual void onVoiceData(uint32_t uid, const char *pbuffer, uint32_t length);
 private:
  IAgoraSdk *sdk_;

  JavaVM *jvm_;
  jobject handler_;

  jmethodID onJoinSuccess_;
  jmethodID onError_;
  jmethodID onVoiceData_;
};

AgoraHandler::AgoraHandler() {
  sdk_ = IAgoraSdk::createInstance(this);

  jvm_ = NULL;
  handler_ = NULL;
  onJoinSuccess_ = NULL;
  onError_ = NULL;
  onVoiceData_ = NULL;
}

AgoraHandler::~AgoraHandler() {
  if (sdk_) {
    // sdk_->leave();
    IAgoraSdk::destroyInstance(sdk_);
  }

  if (handler_) {
    GET_JNI_ENV_OBJECT();
    jni_env->DeleteGlobalRef(handler_);
  }
}

bool AgoraHandler::SetJavaCallback(JNIEnv *env, jobject handler) {
  jclass cls = env->GetObjectClass(handler);
  if (cls == NULL) {
    LOG(ERROR, "No java class found for %p", handler);
    return false;
  }

  jmethodID on_success_method = env->GetMethodID(cls, "onJoinSuccess",
      "(Ljava/lang/String;ILjava/lang/String;)V");
  if (!on_success_method) {
    LOG(ERROR, "No onJoinSucess defined!");
    return false;
  }

  jmethodID on_error_method = env->GetMethodID(cls, "onError",
      "(ILjava/lang/String;)V");
  if (!on_error_method) {
    LOG(ERROR, "No onError method defined!");
    return false;
  }

  jmethodID on_voice_method = env->GetMethodID(cls, "onVoiceData",
      "(I[B)V");
  if (!on_voice_method) {
    LOG(ERROR, "No onVoiceData method defined!");
    return false;
  }

  if (env->GetJavaVM(&jvm_) != JNI_OK) {
    LOG(ERROR, "Cannot get java virtual Machine!");
    return false;
  }

  handler_ = env->NewGlobalRef(handler);

  onJoinSuccess_ = on_success_method;
  onError_ = on_error_method;
  onVoiceData_ = on_voice_method;
  return true;
}

void AgoraHandler::onJoinSuccess(const char *cname, uint32_t uid, const char *msg) {
  LOG(INFO, "User %u joined the channel %s: %s", uid, cname, msg);

  GET_JNI_ENV_OBJECT();

  jstring jmsg = jni_env->NewStringUTF(msg);
  jstring jname = jni_env->NewStringUTF(cname);

  jni_env->CallVoidMethod(handler_, onJoinSuccess_, jname, uid, jmsg);

  jni_env->DeleteLocalRef(jname);
  jni_env->DeleteLocalRef(jmsg);
}

void AgoraHandler::onError(int rescode, const char *msg) {
  LOG(ERROR, "onError: %d, %s", rescode, msg);

  GET_JNI_ENV_OBJECT();

  jstring newString = jni_env->NewStringUTF(msg);
  jni_env->CallVoidMethod(handler_, onError_, rescode, newString);
  jni_env->DeleteLocalRef(newString);
}

void AgoraHandler::onVoiceData(uint32_t uid, const char *buf,
    uint32_t length) {
  GET_JNI_ENV_OBJECT();

  jbyteArray obj = jni_env->NewByteArray(length);
  jni_env->SetByteArrayRegion(obj, 0, length, (jbyte *)buf);
  jni_env->CallVoidMethod(handler_, onVoiceData_, uid, obj);
  jni_env->DeleteLocalRef(obj);
}

JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved) {
  (void)reserved;
  JNIEnv *env = NULL;

  jint result = -1;

  if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
    LOG(ERROR, "ERROR: GetEnv failed\n");
    return result;
  }

  assert(env != NULL);

  /* success -- return valid version number */
  result = JNI_VERSION_1_4;

  return result;
}

JNIEXPORT jlong JNICALL Java_io_agora_rtc_RtcEngine_nativeCreate(JNIEnv *env,
    jclass cls, jobject handler) {
  (void)cls;
  AgoraHandler *channel = new AgoraHandler();

  if (!channel->SetJavaCallback(env, handler)) {
    delete channel;
    return 0L;
  }

  return reinterpret_cast<jlong>(channel);
}

JNIEXPORT void JNICALL Java_io_agora_rtc_RtcEngine_nativeJoinChannel(
    JNIEnv *env, jclass cls, jlong sdkInstance, jstring jVendorID,
    jstring jVendorKey, jstring jChannelName, jint uid,
    jshort minAllowedUdpPort, jshort maxAllowedUdpPort, jboolean audio_mixed,
    jint sampleRates) {
  (void)cls;

  const char* vendorID;

  if (jVendorID == NULL) {
    vendorID = NULL;
  } else {
    vendorID = env->GetStringUTFChars(jVendorID, 0);
  }

  AgoraHandler *channel = reinterpret_cast<AgoraHandler*>(sdkInstance);

  IAgoraSdk *pSdk = channel->GetAgoraSdk();

  const char* vendorKey = env->GetStringUTFChars(jVendorKey, 0);
  const char* channelName = env->GetStringUTFChars(jChannelName, 0);

  pSdk->joinChannel(vendorID, vendorKey, channelName, uid, minAllowedUdpPort,
      maxAllowedUdpPort, audio_mixed, sampleRates);

  if (jVendorID != NULL) {
    env->ReleaseStringUTFChars(jVendorID, vendorID);
  }

  env->ReleaseStringUTFChars(jVendorKey, vendorKey);
  env->ReleaseStringUTFChars(jChannelName, channelName);
}

JNIEXPORT void JNICALL Java_io_agora_rtc_RtcEngine_nativeLeave
(JNIEnv *env, jclass cls, jlong instance) {
  (void)env;
  (void)cls;
  
  AgoraHandler *channel = reinterpret_cast<AgoraHandler *>(instance);
  IAgoraSdk *pSdk = channel->GetAgoraSdk();

  pSdk->leave();
}

JNIEXPORT void JNICALL Java_io_agora_rtc_RtcEngine_nativePoll
(JNIEnv *env, jclass cls, jlong instance) {
  (void)env;
  (void)cls;

  AgoraHandler *channel = reinterpret_cast<AgoraHandler *>(instance);
  IAgoraSdk *pSdk = channel->GetAgoraSdk();

  pSdk->onTimer();
}

JNIEXPORT void JNICALL Java_io_agora_rtc_RtcEngine_nativeDestroy
(JNIEnv *env, jclass cls, jlong instance) {
  (void)env;
  (void)cls;

  AgoraHandler *channel = reinterpret_cast<AgoraHandler *>(instance);
  delete channel;
}

