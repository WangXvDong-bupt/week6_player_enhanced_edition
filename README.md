# week6_player_enhanced_edition
# 项目说明
- 1.程序代码包含父类Player，子类AudioPlayer、VideoPlayer及测试程序test.cpp
- 2.仓库同时包含FFmpeg、SDL、openAL的库文件：include文件夹，lib文件夹及out/bulid/x64-Debug/路径下的.dll文件
# 使用说明
- 1.项目编译成功后，在包含exe文件的路径下运行cmd，输入"ffplayer_sync.exe"并将想要播放的文件的路径作为启动参数输入，即可进行播放
- 2.播放窗口可被选中，进行拉伸（画面会同比例拉伸），全屏播放；按空格键可进行暂停/继续播放，按“+”、“-”可调整音量，按“1”、“2”则可分别进行快进10、30秒。
