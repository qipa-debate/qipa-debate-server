package io.agora.rtc;

public class RtcEngine {

  private long mAgoraSdkEngine;

  static {
    System.loadLibrary("agora-rtc-server-jni");
  }

	private RtcEngine() {
    this.mAgoraSdkEngine = 0;
	}

  public static synchronized RtcEngine create(IRtcEngineEventHandler handler) {
    RtcEngine sEngine = new RtcEngine();
    sEngine.mAgoraSdkEngine = nativeCreate(handler);
    return sEngine;
  }

  public void destroy() {
    if (this.mAgoraSdkEngine != 0) {
      nativeDestroy(this.mAgoraSdkEngine);
    }

    this.mAgoraSdkEngine = 0;
  }

  public void joinChannel(String vendorID, String vendorKey,
      String channelName, int uid, short minAllowedUdpPort,
      short maxAllowedUdpPort, boolean audioMixed,
      int sampleRates) {
    if (this.mAgoraSdkEngine != 0) {
      nativeJoinChannel(this.mAgoraSdkEngine, vendorID, vendorKey, channelName,
          uid, minAllowedUdpPort, maxAllowedUdpPort, audioMixed, sampleRates);
    }
  }

  public void joinChannel(String vendorKey, String channelName, int uid,
      boolean audioMixed, int sampleRates) {
    this.joinChannel("", vendorKey, channelName, uid, (short)0, (short)0,
        audioMixed, sampleRates);
  }

  public void leave() {
    if (this.mAgoraSdkEngine != 0) {
      nativeLeave(this.mAgoraSdkEngine);
    }
  }

  public void poll() {
    if (this.mAgoraSdkEngine != 0) {
      nativePoll(this.mAgoraSdkEngine);
    }
  }

  protected void finalize() {
    if (this.mAgoraSdkEngine != 0) {
      nativeDestroy(this.mAgoraSdkEngine);
      this.mAgoraSdkEngine = 0;
    }
  }

  private static native long nativeCreate(IRtcEngineEventHandler handler);

	private static native void nativeJoinChannel(long sdkInstance,
      String vendorID, String vendorKey, String channelName, int uid,
      short minAllowedUdpPort, short maxAllowedUdpPort, boolean audioMixed,
      int sampleRates);

	private static native void nativeLeave(long sdkInstance);
	private static native void nativePoll(long sdkInstance);
	private static native void nativeDestroy(long sdkInstance);
}

