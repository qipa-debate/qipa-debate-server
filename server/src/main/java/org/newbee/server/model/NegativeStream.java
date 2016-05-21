package org.newbee.server.model;

import java.util.Dictionary;
import java.util.Hashtable;

/**
 * Created by Huiyi on 2016/5/21.
 */
public class NegativeStream {
    private Dictionary<Integer, User> negativeSpeakers;

    public NegativeStream() {
        setNegativeSpeakers(new Hashtable<>());
        getNegativeSpeakers().put(2, new User(2, null));
        getNegativeSpeakers().put(4, new User(4, null));
        getNegativeSpeakers().put(6, new User(6, null));
    }

    public Dictionary<Integer, User> getNegativeSpeakers() {
        return negativeSpeakers;
    }

    public void setNegativeSpeakers(Dictionary<Integer, User> negativeSpeakers) {
        this.negativeSpeakers = negativeSpeakers;
    }
}
