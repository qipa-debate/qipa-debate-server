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
import java.util.ArrayList;
import java.util.List;

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
    }

    @RequestMapping(value = "", method = RequestMethod.GET)
    public Debate getDebateInformation() {
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
            boolean isActive = node.get("is_active").asBoolean();

            if (isActive) {
                if (isOnPositiveSide(userId)) {
                    Configuration.DEBATE.getDebateStream().setPositiveSpeaker(new User(userId, streamId));
                } else {
                    Configuration.DEBATE.getDebateStream().setNegativeSpeaker(new User(userId, streamId));
                }
            }

            if (isOnPositiveSide(userId)) {
                Configuration.DEBATE.getPositiveStream().getPositiveSpeakers().get(userId - 1).setStreamId(streamId);
            } else {
                Configuration.DEBATE.getNegativeStream().getNegativeSpeakers().get(userId - 4).setStreamId(streamId);
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

    private boolean isOnPositiveSide(int userId) {
        if (userId <= 3)
            return true;
        else
            return false;
    }
}
