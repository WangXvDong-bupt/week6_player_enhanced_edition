#include<iostream>
#include<stack>
#include<vector>
#include<cstdio>
#include<string>
#include<future>
#include<chrono>

#include "VideoPlayer.h"
#include "AudioPlayer.h"

using std::cout;
using std::endl;
using std::string;



uint64_t* pts_audio = (uint64_t*)av_malloc(sizeof(uint64_t));

int audioThread(void* opaque) {

	//AudioPlayer audio;
	//audio.openFile(filepath, AVMEDIA_TYPE_AUDIO);
	//audio.audioSetting();
	//audio.setAudioSDL();
	AudioPlayer audio = *((AudioPlayer*)opaque);
	audio.playByOpenAL(pts_audio);
	audio.Player_Quit();
	return 0;
}

int videoThread(void* opaque) {
	//VideoPlayer video;
	//video.openFile(filepath, AVMEDIA_TYPE_VIDEO);
	//video.showInfo();
	//video.setWindow();
	VideoPlayer video = *((VideoPlayer*)opaque);
	video.play(pts_audio);
	video.Player_Quit();
	return 0;
}


int main(int argc, char* argv[])
{
	cout << "hello, little player." << endl;
	if (argc != 2) {
		cout << "input error:" << endl;
		cout << "arg[1] should be the media file." << endl;
	}
	else {
		char* filepath = argv[1];
		cout << "play file:" << filepath << endl;
		avformat_network_init();
		if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
			printf("Could not initialize SDL - %s\n", SDL_GetError());
		}
		AudioPlayer audio;
		audio.openFile(filepath, AVMEDIA_TYPE_AUDIO);
		audio.audioSetting();

		//SDL_Thread* video_tid = SDL_CreateThread(videoThread, NULL, &video);

		std::thread audio_tid{ audioThread, &audio };
		//SDL_Thread* audio_tid = SDL_CreateThread(audioThread, NULL, &audio);

		VideoPlayer video;
		video.openFile(filepath, AVMEDIA_TYPE_VIDEO);
		video.showInfo(filepath);
		video.setWindow();
		video.play(pts_audio);
		video.Player_Quit();
		//std::thread video_tid{ videoThread, &video };
		//video.play(std::ref(fu1));
		//video.Player_Quit();
		cout << "FrameRate:" << video.getFrameRate() << endl;

		audio_tid.join();
		//video_tid.join();
	}

	//std::promise<uint64_t> pr1;
	//std::future<uint64_t> fu1 = pr1.get_future();
	
	return 0;
}