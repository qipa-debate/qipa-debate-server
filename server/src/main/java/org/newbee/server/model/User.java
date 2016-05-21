package org.newbee.server.model;

/**
 * Created by Huiyi on 2016/5/21.
 */
public class User {
    private int userId;
    private String streamId;

    public User(int userId, String streamId) {
        this.userId = userId;
        this.streamId = streamId;
    }

    public int getUserId() {
        return userId;
    }

    public void setUserId(int userId) {
        this.userId = userId;
    }

    public String getStreamId() {
        return streamId;
    }

    public void setStreamId(String streamId) {
        this.streamId = streamId;
    }
}
