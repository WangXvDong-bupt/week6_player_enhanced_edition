#pragma once

#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H
#include "Player.h"
#include "OpenAL/include/alc.h"
#include "OpenAL/include/al.h"
#include "SoundTouch/include/SoundTouch.h"
#include "SoundTouch/include/STTypes.h"
#include <chrono>
#include <thread>
#include <string>
#include <iomanip>
#include <future>

class AudioPlayer :public Player
{
public:
	AudioPlayer();
	~AudioPlayer();
private:
	SwrContext* swrCtx;

	//�ز�������ѡ��-----------------------------------------------------------start

	//����Ĳ�����ʽ
	enum AVSampleFormat in_sample_fmt;
	//����Ĳ�����ʽ 16bit PCM
	enum AVSampleFormat out_sample_fmt;
	//����Ĳ�����
	int in_sample_rate;
	//����Ĳ�����
	int out_sample_rate;
	//�������������
	uint64_t in_ch_layout;
	//�������������
	uint64_t out_ch_layout;
	//�ز�������ѡ��-----------------------------------------------------------end

	//SDL_AudioSpec
	SDL_AudioSpec wanted_spec;
	//��ȡ�������������
	int out_channel_nb;

	bool fast_forward_10 = false;
	bool fast_forward_30 = false;
	bool seek_req = false;
	double increase = 0;
	bool speed_req = false;
	double speed_idx = 1;

	soundtouch::SoundTouch s_touch;
	int speed_count = 0;

public:
	void audioSetting();
	int setAudioSDL();
	int play();
	int playByOpenAL(uint64_t* pts_audio);

	static int sfp_control_thread(bool& exitRefresh, bool& pause, float& volumn, bool& volumnChange, 
		bool& fast_foeward_10, bool& fast_foeward_30, bool& seek_req, bool& speed_req, double& speed_idx);

	uint64_t ptsAudio = 0;
	uint64_t ptsAudioFromSpeed = 0;
	uint64_t getPts();

private:
	static void fill_audio(void* udata, Uint8* stream, int len);
	int feedAudioData(ALuint uiSource, ALuint alBufferId);
	int allocDataBuf(uint8_t** outData, int inputSamples);
	int feedAudioData_forSpeed(ALuint uiSource, ALuint alBufferId, bool isRevSample);
};


#endif

