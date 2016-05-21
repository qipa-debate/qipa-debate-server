1. 如何运行演示程序
a) 导出环境变量
  export LD_LIBRARY_PATH=.
  export PATH=${PATH}:./

b) 修改演示程序 AgoraChannel.java 的代码，将 VENDOR_KEY 替换为向 Agora.io
   申请的实际 key
c) 编译演示程序. 注: ${version} 现在是 1.2.2.6
javac -classpath agora-rtc-server-java-${version}.jar AgoraChannel.java

d) 运行演示程序
java -classpath agora-rtc-server-java-${version}.jar:. AgoraChannel

2. 接口说明

RtcEngine 是 Agora Inc. 服务器端音频 SDK 主要接口，基本方法如下:

package io.agora.rtc;
public class RtcEngine {
 public static synchronized RtcEngine create(IRtcEngineEventHandler handler);
 public void destroy();
 public void joinChannel(String vendorID, String vendorKey, String channelName,
     int uid, short minAllowedUdpPort, short maxAllowedUdpPort,
     boolean audioMixed, int sampleRates);

 public void leave();
 public void destroy();
 public void poll();
}

// 回调接口
public abstract class IRtcEngineEventHandler {
 public void onJoinSuccess(String cname, int uid, String msg) {
 };

 public void onError(int error, String errMsg) {
 };

 public void onVoiceData(int uid, byte[] buffer) {
 };
}

函数接口的具体说明见 4).

1) 基本步骤

步骤 1: 创建 RtcEngine 实例, 传入回调接口 (回调接口的实现由调用者提供)
 callback = get_IRtcEngineEventHandler_from_somewhere();
 engine = RtcEngine.create(callback);

步骤 2: 加入频道 (具体参数含义见接口说明)
 channel_name = "test123";
 engine.joinChannel("", your_vendor_key, channel_name, 0, 0, 0, true, 8000);

步骤 3: 启动定时器，开始轮询结果
 while (true) {
   Thread.sleep(10); // sleep 10 ms.
   engine.poll();
 }

 Server SDK 尽量保持简单的接口和实现，内部没有自己的线程事件循环，所以需要调用
 者能够主动去定时轮询事件。在调用poll 时，IRtcEngineEventHandler 的接口会被调用.

步骤4: Server SDK 的退出
 要退出循环, 可以异步的设置一些标志位，在循环中检查标志位，如果发现标志位被设置,
 则退出循环，并销毁频道。

 while (!g_quit_flag) {
   Thread.sleep(10);
   engine.poll();
 }
 engine.leave();

步骤 5: 重新加入频道
  重复步骤 2 -> 4

步骤 6: 实例的销毁
  engine.destroy();

2) 多个频道的创建:
 如果需要同时进入多个频道，则可以创建多个 RtcEngine 实例，并在每个实例上执行 1)
 的步骤 1-6.

3) 线程安全性:
 在不同 RtcEngine 实例上的操作完全独立互不影响，而同一个 RtcEngine 实例的调用
 应在同一线程完成。
 建议多线程下的实现逻辑 (参考 demo 示例)
 a) 主线程：
 start:
  侦听接收信令事件
  如果需要录音 channel:
     创建一个工作线程,在工作线程中调用RtcEngine
  如果需要停止录音:
     设置该频道的停止标志位，等待工作线程处理并退出
  goto start;
 b) 工作线程:
   engine = RtcEngine.create(callback);
   engine.joinChannel();
   while (!this_channel.guit_flag) {
     Thread.sleep(10);
     engine.poll();
   }
   engine.leave();
   engine.destroy();

4) 接口说明:
public class RtcEngine {
 // 静态函数，创建 RtcEngine 实例
 // 参数 handler, 是回调接口
 public static synchronized RtcEngine create(IRtcEngineEventHandler handler);

 // 销毁 RtcEngine 实例，销毁后的 RtcEngine 不能再调用任何函数
 public void destroy();

 // 加入频道
 // 参数:
 // String vendorID: 暂时未用，传递空字符串即可
 // String vendorKey: 从 Agora 网站申请来的厂商 key
 // String channelName: 要加入的频道名，该频道名由厂商任意指定，
 //  同一个厂商的同一个频道名代表一通会话
 // int uid: 指定加入该频道时的用户 32 位标识符，该标识符需要在同一频道中
 //   保证唯一, 传入 0 则由系统产生
 // minAllowedUdpPort/maxAllowedUdpPort: 指定 SDK 内部需要使用的本地的 UDP 端口,
 //  SDK 内部需要和服务器通过多个 UDP 端口，如果不需要控制端口的分配策略, 直接传
 //  入 0 即可，SDK 直接由系统分配端口.
 // boolean audioMixed: 是否需要混音。如果混音，则 Server SDK 产生的语音数据为
 //  混音后的数据; 否则，每个用户的声音数据分别交给 onVoiceData
 // int sampleRates: 指定产生语音数据的采样率，目前支持两种采样率 8000/16000
 // 返回值:
      无返回值
 // joinChannel 在本地资源初始化过程中，如果失败，会调用 IRtcEngineEventHandler::onError,
 //  否则向服务器发送 Join 请求，然后立即返回，joinChannel 的服务器成功或者超时，
 //  会在后续的 poll 中，回调 onError
 public void joinChannel(String vendorID, String vendorKey, String channelName,
     int uid, short minAllowedUdpPort, short maxAllowedUdpPort,
     boolean audioMixed, int sampleRates);

 // 调用离开频道, 要求已经调用 joinChannel
 public void leave();

 // 销毁 RtcEngine 实例
 public void destroy();

 // poll 读取 RtcEngine 状态，回调相应的接口
 // 该函数中，
 // 1) 如果服务器返回加入频道成功, 则调用 IRtcEngineEventHandler::onJoinSuccess;
 // 2) 如果加入频道失败，或者其它错误，则调用 IRtcEngineEventHandler::onError;
 // 3) 如果有语音数据，则调用 IRtcEngineEventHandler::onVoiceData
 public void poll();
}

public abstract class IRtcEngineEventHandler {
 // 加入频道成功; 在 RtcEngine::poll 中回调
 // 参数 cname, 是用户加入的频道名;
 // 参数 uid, 是Server SDK 用户在该频道中的标识符
 // 参数 msg, 保留
 public void onJoinSuccess(String cname, int uid, String msg) {
 };

 // 频道出现错误的回调, 可能会在 IRtcEngineEventHandler::joinSuccess/poll 中调用
 // 参数 error, errMsg 分别是错误码和错误信息
 // 该函数被调用时，表示 RtcEngine 出错不可恢复，应该设置标志位，退出该频道。
 public void onError(int error, String errMsg) {
 };

 // 该函数在有语音数据到达时，被 IRtcEngineEventHandler::poll 回调
 // 参数 uid，代表该语音数据来自频道内的 uid 表示的用户; 如果是混音后的数据,
 //  该 uid 始终为 0
 // 参数 buffer 是语音的 PCM 数据，每个采样点的数据为 16 位有符号小头在前的整数
 public void onVoiceData(int uid, byte[] buffer) {
 };
}

