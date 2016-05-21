package org.newbee.server.model;

import java.util.Dictionary;
import java.util.Hashtable;

/**
 * Created by Huiyi on 2016/5/21.
 */
public class PositiveStream {
    private Dictionary<Integer, User> positiveSpeakers;

    public PositiveStream() {
        setPositiveSpeakers(new Hashtable<>());
        getPositiveSpeakers().put(1, new User(1, null));
        getPositiveSpeakers().put(3, new User(3, null));
        getPositiveSpeakers().put(5, new User(5, null));
    }

    public Dictionary<Integer, User> getPositiveSpeakers() {
        return positiveSpeakers;
    }

    public void setPositiveSpeakers(Dictionary<Integer, User> positiveSpeakers) {
        this.positiveSpeakers = positiveSpeakers;
    }
}
