package org.newbee.server.model;

import java.util.ArrayList;
import java.util.List;

/**
 * Created by Huiyi on 2016/5/21.
 */
public class PositiveStream {
    private List<User> positiveSpeakers;

    public PositiveStream() {
        positiveSpeakers = new ArrayList<>();
        positiveSpeakers.add(new User(1, null));
        positiveSpeakers.add(new User(2, null));
        positiveSpeakers.add(new User(3, null));
    }

    public List<User> getPositiveSpeakers() {
        return positiveSpeakers;
    }

    public void setPositiveSpeakers(List<User> positiveSpeakers) {
        this.positiveSpeakers = positiveSpeakers;
    }
}
