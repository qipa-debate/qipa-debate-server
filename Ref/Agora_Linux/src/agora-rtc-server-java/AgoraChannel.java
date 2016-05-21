import io.agora.rtc.*;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;

import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;

public class AgoraChannel {
    public AgoraChannel() {
      this.mQuit = new AtomicBoolean();
      this.mQuit.set(false);
      this.hookExit();
      this.engine = RtcEngine.create(this.mDataHandler);
    }

    private final ScheduledExecutorService mScheduler = Executors.newScheduledThreadPool(1);

    private AtomicBoolean mQuit;

    public void joinChannel(String vkey, String channel, int uid,
        boolean audioMixed, int sampleRates) {
      if (this.engine != null) {
        this.engine.joinChannel(vkey, channel, uid, audioMixed, sampleRates);
        this.mScheduler.scheduleAtFixedRate(this.mTickerTask, 2000, 20,
            TimeUnit.MILLISECONDS);
      }
    }

    public void leave() {
      this.mQuit.set(true);
      try {
        Thread.sleep(200);
      } catch (Exception e) {
      }
    }

    public void destroy() {
      this.engine.destroy();
      this.engine = null;
    }

    private final Runnable mTickerTask = new Runnable() {
        @Override
        public void run() {
             if (AgoraChannel.this.mQuit.get()) {
                 RtcEngine engine = AgoraChannel.this.engine;
                 engine.leave();
                 engine.destroy();

                 AgoraChannel.this.engine = null;

                 mScheduler.shutdown();
                 return;
             }
             AgoraChannel.this.engine.poll();
        }
    };

    private final IRtcEngineEventHandler mDataHandler = new IRtcEngineEventHandler() {

        File recorderFile = null;

        boolean first = true;
        String channelName = null;

        @Override
        public void onJoinSuccess(String cname, int uid, String msg) {
            channelName = cname;
            System.out.println("onJoinSuccess " + cname + " " + (uid & 0xFFFFFFFFL) + " " + msg);
            recorderFile = new File(System.currentTimeMillis() + ".pcm");
            try {
                if (!recorderFile.exists()) {
                    recorderFile.createNewFile();
                }
            } catch (IOException e) {
            }
            AgoraChannel.this.mQuit.set(false);
        }

        @Override
        public void onError(int error, String errMsg) {
            System.out.println("onError " + error + " " + errMsg);
            AgoraChannel.this.mQuit.set(true);
        }

        @Override
        public void onVoiceData(int uid, byte[] buffer) {
            // System.out.println("onVoiceData " + (uid & 0xFFFFFFFFL) + " " + buffer.length + " " + System.currentTimeMillis());
            System.out.println("onVoiceData from " + channelName + ", length: " + buffer.length);

            if (first) {
                System.out.println("onVoiceData first " + System.currentTimeMillis());
                first = false;
            }
            try {
                FileOutputStream out = new FileOutputStream(recorderFile, true);
                out.write(buffer, 0, buffer.length);
                out.close();
            } catch (Exception e) {
            }
        }
    };

    private static final String VENDOR_KEY = "PLEASE_INPUT_YOUR_KEY";

    private RtcEngine engine = null;


    private void hookExit() {
        Runtime.getRuntime().addShutdownHook(new Thread() {
            public void run() {
                AgoraChannel.this.mQuit.set(true);
                try {
                  Thread.sleep(200);
                } catch (Exception e) {
                }

                System.out.println("User canceled, leave now");
                // RtcEngine engine = AgoraChannel.this.engine;
                // if (engine != null) {
                //     try {
                //         Thread.sleep(200);
                //     } catch (Exception e) {
                //     }
                //     // engine.leave();
                //     // engine.destroy();
                //     System.out.println("User canceled, leave now");
                // }
            }
        });
    }

    public static void main(String[] args) {
        System.out.println("Recorder started init");

        AgoraChannel recorder1 = new AgoraChannel();
        System.out.println("Recorder 1 started engine created");

        recorder1.joinChannel(VENDOR_KEY, "99999", 0, true, 8000);

        AgoraChannel recorder2 = new AgoraChannel();

        System.out.println("Recorder 2 started engine created");
        recorder2.joinChannel(VENDOR_KEY, "88888", 0, true, 8000);

        try {
            // Thread.sleep(Long.MAX_VALUE);
            Thread.sleep(20000);
            recorder1.leave();
            Thread.sleep(30000);
            recorder2.leave();

            recorder1.destroy();
            recorder2.destroy();
        } catch (Exception e) {
          System.out.println("Exception captured");
        }

        System.out.println("Recorder exit done");
    }
}
