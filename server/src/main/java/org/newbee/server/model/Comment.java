package org.newbee.server.model;

/**
 * Created by Huiyi on 2016/5/21.
 */
public class Comment {
    private int userId;
    private String content;

    public Comment(int userId, String content) {
        this.userId = userId;
        this.content = content;
    }

    public int getUserId() {
        return userId;
    }

    public void setUserId(int userId) {
        this.userId = userId;
    }

    public String getContent() {
        return content;
    }

    public void setContent(String content) {
        this.content = content;
    }
}
