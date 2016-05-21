package io.agora.rtc;

public abstract class IRtcEngineEventHandler {
    public void onJoinSuccess(String cname, int uid, String msg) {
    };

    public void onError(int error, String errMsg) {
    };

    public void onVoiceData(int uid, byte[] buffer) {
    };
}
