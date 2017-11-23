/**
*该工程是为了解决低版本OpenCv下打开RTSP视频流图像花屏的异常
*/

setp 1:你需要自行安装ffmpeg3.3及OpenCv(低版本），我的版本是2.4.10
setp 2:A,该项目在Ubuntu 16.04 x64下编译测试通过，你可以git克隆下来，直接执行sudo ./build.sh
       B,该项目也可作为Visual Studio项目打开及编译测试
step 3:该项目主要功能包括，实现自定义VideoCapture（主要解决OpenCv下打开RTSP视频流图像花屏）、
       RTSP转HLS推流及RTSP转RTMP推流（你需要安装服务器流，我用的是nginx1.12.0）、
       OpenCv表盘指针校正及刻度识别、MQTT消息推送等等
