package org.newbee.server.model;

/**
 * Created by Huiyi on 2016/5/21.
 */
public class DebateStream {
    private User positiveSpeaker;
    private User negativeSpeaker;

    public User getPositiveSpeaker() {
        return positiveSpeaker;
    }

    public void setPositiveSpeaker(User positiveSpeaker) {
        this.positiveSpeaker = positiveSpeaker;
    }

    public User getNegativeSpeaker() {
        return negativeSpeaker;
    }

    public void setNegativeSpeaker(User negativeSpeaker) {
        this.negativeSpeaker = negativeSpeaker;
    }
}
