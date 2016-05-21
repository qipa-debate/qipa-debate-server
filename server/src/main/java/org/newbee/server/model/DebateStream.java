package org.newbee.server.model;

import com.fasterxml.jackson.annotation.JsonIgnore;

import java.util.Date;
import java.util.Dictionary;
import java.util.Hashtable;

/**
 * Created by Huiyi on 2016/5/21.
 */
public class DebateStream {
    private User positiveSpeaker;
    private long positiveSpeakerElapsedTime;
    private User negativeSpeaker;
    private long negativeSpeakerElapsedTime;

    public DebateStream() {
        this.speakers = new Hashtable<>();
    }

    @JsonIgnore
    private Dictionary<Integer, User> speakers;

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

    public Dictionary<Integer, User> getSpeakers() {
        return speakers;
    }

    public void setSpeakers(Dictionary<Integer, User> speakers) {
        this.speakers = speakers;
    }

    public long getPositiveSpeakerElapsedTime() {
        return positiveSpeakerElapsedTime;
    }

    public void setPositiveSpeakerElapsedTime(long positiveSpeakerElapsedTime) {
        this.positiveSpeakerElapsedTime = positiveSpeakerElapsedTime;
    }

    public long getNegativeSpeakerElapsedTime() {
        return negativeSpeakerElapsedTime;
    }

    public void setNegativeSpeakerElapsedTime(long negativeSpeakerElapsedTime) {
        this.negativeSpeakerElapsedTime = negativeSpeakerElapsedTime;
    }
}
