#pragma once

#ifndef VIDEOPLAYER_H
#define VIDEOPLAYER_H
//Refresh Event
#define SFM_REFRESH_EVENT  (SDL_USEREVENT + 1)
#define SFM_BREAK_EVENT  (SDL_USEREVENT + 2)
#include "Player.h"
#include "AudioPlayer.h"

class VideoPlayer :public Player
{
public:
	VideoPlayer();
	~VideoPlayer();
private:
	struct SwsContext* img_convert_ctx;
	SDL_Renderer* sdlRenderer;
	SDL_Texture* sdlTexture;
	SDL_Thread* video_tid;
	SDL_Event event;
	SDL_Window* screen;

	double increase = 0;
	uint64_t vTs = 0;
	bool seek_req = false;
	bool speed_req = false;
	double speed_idx = 1;

public:
	void showInfo(char* filepath);
	int setWindow();
	//virtual int play(std::future<uint64_t> &f);
	int play(uint64_t* pts_audio);
	static int sfp_refresh_thread(int timeInterval, bool& exitRefresh, bool& faster, bool& slower, bool& pause, bool& speed_req, double& speed_idx);

	double getFrameRate() const;
};
#endif
