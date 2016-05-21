package org.newbee.server.controller;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import org.newbee.server.model.Comment;
import org.newbee.server.model.Debate;
import org.newbee.server.model.User;
import org.newbee.server.util.Configuration;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestMethod;
import org.springframework.web.bind.annotation.RestController;

import java.io.IOException;
import java.util.*;

/**
 * Created by Huiyi on 2016/5/21.
 */
@RestController
@RequestMapping(value = "/debate")
public class DebateController {

    private static Logger LOG = LoggerFactory.getLogger(DebateController.class);

    @RequestMapping(value = "test", method = RequestMethod.GET)
    public String getAttachmentURL() throws Exception {
        return "success";
    }

    @RequestMapping(value = "update", method = RequestMethod.POST)
    public void updateChannel(@RequestBody String connInfo) {
        LOG.debug(connInfo);
        decode(connInfo);
        checkStart();
    }

    @RequestMapping(value = "", method = RequestMethod.GET)
    public Debate getDebateInformation() {
        updateSpeakerInformation();
        updateSpeakerElapsedTime();
        return Configuration.DEBATE;
    }

    @RequestMapping(value = "comment", method = RequestMethod.POST)
    public void addComment(@RequestBody String commentInfo) {
        LOG.debug(commentInfo);
        Comment c = decodeComment(commentInfo);
        Configuration.COMMENTS.add(c);
    }

    @RequestMapping(value = "comment", method = RequestMethod.GET)
    public List<Comment> getComments() {
        List<Comment> comments = new ArrayList<>();
        int size = Configuration.COMMENTS.size();
        for (int i = size >= 5 ? (size - 5) : 0; i < size; ++i) {
            comments.add(Configuration.COMMENTS.get(i));
        }

        return comments;
    }

    private void decode(String connInfo) {
        ObjectMapper mapper = new ObjectMapper();
        try {
            JsonNode node = mapper.readTree(connInfo);
            int userId = node.get("user_id").asInt();
            String streamId = node.get("stream_id").asText();
            String channelId = node.get("channel_id").asText();

            if (channelId.equals(Configuration.DEBATE_CHANNEL_ID)) {
                Configuration.DEBATE.getDebateStream().getSpeakers().put(userId, new User(userId, streamId));
            } else if (channelId.equals(Configuration.POSITIVE_SIDE_CHANNEL_ID)) {
                Configuration.DEBATE.getPositiveStream().getPositiveSpeakers().put(userId, new User(userId, streamId));
            } else {
                Configuration.DEBATE.getNegativeStream().getNegativeSpeakers().put(userId, new User(userId, streamId));
            }
        }  catch (JsonProcessingException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private Comment decodeComment(String commentInfo) {
        ObjectMapper mapper = new ObjectMapper();
        Comment c = null;
        try {
            JsonNode node = mapper.readTree(commentInfo);
            int userId = node.get("user_id").asInt();
            String content = node.get("content").asText();

            c = new Comment(userId, content);
        } catch (JsonProcessingException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        }

        return c;
    }

    private void checkStart() {
        if (Configuration.DEBATE.getNegativeStream().getNegativeSpeakers().size() == 3
         && Configuration.DEBATE.getPositiveStream().getPositiveSpeakers().size() == 3
         && Configuration.DEBATE.getDebateStream().getSpeakers().size() == 6
         && Configuration.START_TIME == null) {
            Configuration.START_TIME = new Date();
            changeCurrentSpeaker(1, 2);
        }
    }

    private void updateSpeakerInformation() {
        if (Configuration.START_TIME == null)
            return;

        Date now = new Date();
        long diff = now.getTime() - Configuration.START_TIME.getTime();
        long halfMinutes = diff / (1000 * 30);

        if (halfMinutes < 2) {
            changeCurrentSpeaker(1, 2);
        } else if (halfMinutes < 4) {
            changeCurrentSpeaker(3, 4);
        } else {
            changeCurrentSpeaker(5, 6);
        }
    }

    private void changeCurrentSpeaker(int positive, int negative) {
        Dictionary<Integer, User> speakers = Configuration.DEBATE.getDebateStream().getSpeakers();
        Configuration.DEBATE.getDebateStream().setPositiveSpeaker(speakers.get(positive));
        Configuration.DEBATE.getDebateStream().setNegativeSpeaker(speakers.get(negative));
    }

    private void updateSpeakerElapsedTime() {
        if (Configuration.START_TIME == null)
            return;

        long diff = new Date().getTime() - Configuration.START_TIME.getTime();
        long intervals = diff / (1000 * 30);
        long elapsedTime = (intervals + 1) * 30 * 1000 - diff;
        if (intervals % 2 == 0) {
            Configuration.DEBATE.getDebateStream().setPositiveSpeakerElapsedTime(elapsedTime);
            Configuration.DEBATE.getDebateStream().setNegativeSpeakerElapsedTime(30 * 1000);
        } else {
            Configuration.DEBATE.getDebateStream().setPositiveSpeakerElapsedTime(0);
            Configuration.DEBATE.getDebateStream().setNegativeSpeakerElapsedTime(elapsedTime);
        }
    }
}
