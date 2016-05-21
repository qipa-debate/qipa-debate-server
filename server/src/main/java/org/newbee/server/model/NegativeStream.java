package org.newbee.server.model;

import java.util.ArrayList;
import java.util.List;

/**
 * Created by Huiyi on 2016/5/21.
 */
public class NegativeStream {
    private List<User> negativeSpeakers;

    public NegativeStream() {
        negativeSpeakers = new ArrayList<>();
        negativeSpeakers.add(new User(4, null));
        negativeSpeakers.add(new User(5, null));
        negativeSpeakers.add(new User(6, null));
    }

    public List<User> getNegativeSpeakers() {
        return negativeSpeakers;
    }

    public void setNegativeSpeakers(List<User> negativeSpeakers) {
        this.negativeSpeakers = negativeSpeakers;
    }
}
