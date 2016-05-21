package org.newbee.server.model;

import java.util.ArrayList;
import java.util.List;

/**
 * Created by Huiyi on 2016/5/21.
 */
public class Debate {
    private String subject;
    private String positiveSideSubject;
    private String negativeSideSubject;

    private PositiveStream positiveStream;
    private NegativeStream negativeStream;
    private DebateStream debateStream;

    public String getSubject() {
        return subject;
    }

    public void setSubject(String subject) {
        this.subject = subject;
    }

    public String getPositiveSideSubject() {
        return positiveSideSubject;
    }

    public void setPositiveSideSubject(String positiveSideSubject) {
        this.positiveSideSubject = positiveSideSubject;
    }

    public String getNegativeSideSubject() {
        return negativeSideSubject;
    }

    public void setNegativeSideSubject(String negativeSideSubject) {
        this.negativeSideSubject = negativeSideSubject;
    }

    public PositiveStream getPositiveStream() {
        return positiveStream;
    }

    public void setPositiveStream(PositiveStream positiveStream) {
        this.positiveStream = positiveStream;
    }

    public NegativeStream getNegativeStream() {
        return negativeStream;
    }

    public void setNegativeStream(NegativeStream negativeStream) {
        this.negativeStream = negativeStream;
    }

    public DebateStream getDebateStream() {
        return debateStream;
    }

    public void setDebateStream(DebateStream debateStream) {
        this.debateStream = debateStream;
    }
}
