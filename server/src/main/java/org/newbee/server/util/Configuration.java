package org.newbee.server.util;

import org.newbee.server.model.*;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Date;
import java.util.List;

/**
 * Created by Huiyi on 2016/5/21.
 */
public class Configuration {

    public static String POSITIVE_SIDE_CHANNEL_ID = "10000";
    public static String NEGATIVE_SIDE_CHANNEL_ID = "10010";
    public static String DEBATE_CHANNEL_ID = "10086";

    public static String POSITIVE_SIDE_SUBJECT = "POSITIVE";
    public static String NEGATIVE_SIDE_SUBJECT = "NEGATIVE";
    public static String DEBATE_SUBJECT = "DEBATE";

    public static Debate DEBATE;
    public static List<Comment> COMMENTS;

    public static Date START_TIME;

    static {
        DEBATE = new Debate();
        DEBATE.setSubject(DEBATE_SUBJECT);
        DEBATE.setNegativeSideSubject(NEGATIVE_SIDE_SUBJECT);
        DEBATE.setPositiveSideSubject(POSITIVE_SIDE_SUBJECT);

        DebateStream debateStream = new DebateStream();
        PositiveStream positiveStream = new PositiveStream();
        NegativeStream negativeStream = new NegativeStream();

        DEBATE.setPositiveStream(positiveStream);
        DEBATE.setNegativeStream(negativeStream);
        DEBATE.setDebateStream(debateStream);

        COMMENTS = new ArrayList<>();
    }
}
