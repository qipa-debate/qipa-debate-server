Step 0: substitute the VENDOR KEY string with your KEY on line 253 of demo.cpp
Step 1: compile the demo:
  g++ demo.cpp -o demo -Llibs -lvoip -std=c++0x -Iinclude -lpthread

Step 2: run the demo:
  1) let $PSTN_BIN_PATH denote the directory where |applite| |libagorartc.so|
     and |libHDACEngine.so| reside,
    export LD_LIBRARY_PATH=${PSTN_BIN_PATH}
    export PATH=${PATH}:${PSTN_BIN_PATH}
  2) run
    ./demo
# This demo will join the channel PSTN_CHANNEL, and write down all received voice data
# into a PCM file.  To view the logging message, see the file where syslog prints to

Step 3:
  If you want to convert this raw PCM-encoded data into other popular audio
  format, you can try to use Audacity/ffmpeg and some other similar audio
  editing software. As an example, use ffmpeg to convert:
    ffmpeg -f s16le -ar 16k -ac 1 -i input.pcm output.wav # for 16K sampling rate
  or,
    ffmpeg -f s16le -ar 8k -ac 1 -i input.pcm output.wav # for 8K sampling rate

NOTE:
  If you do not want to control the recording process programmingly, you can
  just invoke |applite| directly, like this:
    applite --batch --samples 8000|16000 --uid 0 --audio_mix 0|1 \
      --name CHANNEL_NAME --key YOUR_KEY \
      --path DIR_TO_STORE_PCM_FILE
      --slice SLICE_BY
      --codec [MP3|PCM]
      --idle [idle]
  where the meaning of arguments passed to the applite, can be referred to the
  descriptions in agora_sdk_i.h.

 Return code:
   1) -1(127): Failed to create the audio engine;
   2) -2(126): Failed to join the channel;
   3) -3(125): Failed to create a timer;
   4) -4(124): Failed to join the channel in 10 seconds;
   5) -5(123): Solo channel;
   6) -6(122): Audio engine error.
