
#include "AudioPlayer.h"
#include <iostream>

#include "OpenAL/include/alc.h"
#include "OpenAL/include/al.h"

#include <chrono>
#include <thread>
#include <string>
#include <iomanip>

#define RATE 48000
#define NUMBUFFERS (12)
#define SERVICE_UPDATE_PERIOD (20)

using std::cout;
using std::endl;
using std::string;
using std::stringstream;
AudioPlayer::AudioPlayer()
{

}


AudioPlayer::~AudioPlayer()
{
	SDL_CloseAudio();//关闭音频设备 
	swr_free(&swrCtx);
}


void AudioPlayer::audioSetting()
{
	//重采样设置选项-----------------------------------------------------------start
	//输入的采样格式
	in_sample_fmt = pCodeCtx->sample_fmt;
	//输出的采样格式 16bit PCM
	out_sample_fmt = AV_SAMPLE_FMT_S16;
	//输入的采样率
	in_sample_rate = pCodeCtx->sample_rate;
	//输出的采样率
	out_sample_rate = pCodeCtx->sample_rate;
	//输入的声道布局
	in_ch_layout = pCodeCtx->channel_layout;
	if (in_ch_layout <= 0)
	{
		in_ch_layout = av_get_default_channel_layout(pCodeCtx->channels);
	}
	//输出的声道布局
	out_ch_layout = AV_CH_LAYOUT_STEREO;			//与SDL播放不同

	//frame->16bit 44100 PCM 统一音频采样格式与采样率
	swrCtx = swr_alloc();
	swr_alloc_set_opts(swrCtx, out_ch_layout, out_sample_fmt, out_sample_rate, in_ch_layout, in_sample_fmt,
		in_sample_rate, 0, NULL);
	swr_init(swrCtx);
	//重采样设置选项-----------------------------------------------------------end

	//获取输出的声道个数
	out_channel_nb = av_get_channel_layout_nb_channels(out_ch_layout);

	s_touch.setSampleRate(out_sample_rate); // 设置变速时的采样率
	s_touch.setChannels(out_channel_nb); // 设置变速时的通道数
}

int AudioPlayer::setAudioSDL()
{
	//获取输出的声道个数
	out_channel_nb = av_get_channel_layout_nb_channels(out_ch_layout);
	//SDL_AudioSpec
	wanted_spec.freq = out_sample_rate;
	wanted_spec.format = AUDIO_S16SYS;
	wanted_spec.channels = out_channel_nb;
	wanted_spec.silence = 0;
	wanted_spec.samples = pCodeCtx->frame_size;
	wanted_spec.callback = fill_audio;//回调函数
	wanted_spec.userdata = pCodeCtx;
	if (SDL_OpenAudio(&wanted_spec, NULL) < 0) {
		printf("can't open audio.\n");
		return -1;
	}
}

static Uint8* audio_chunk;
//设置音频数据长度
static Uint32 audio_len;
static Uint8* audio_pos;

void  AudioPlayer::fill_audio(void* udata, Uint8* stream, int len) {
	//SDL 2.0
	SDL_memset(stream, 0, len);
	if (audio_len == 0)		//有数据才播放
		return;
	len = (len > audio_len ? audio_len : len);

	SDL_MixAudio(stream, audio_pos, len, SDL_MIX_MAXVOLUME);
	audio_pos += len;
	audio_len -= len;
}

int AudioPlayer::allocDataBuf(uint8_t** outData, int inputSamples) {
	int bytePerOutSample = -1;
	switch (out_sample_fmt) {
	case AV_SAMPLE_FMT_U8:
		bytePerOutSample = 1;
		break;
	case AV_SAMPLE_FMT_S16P:
	case AV_SAMPLE_FMT_S16:
		bytePerOutSample = 2;
		break;
	case AV_SAMPLE_FMT_S32:
	case AV_SAMPLE_FMT_S32P:
	case AV_SAMPLE_FMT_FLT:
	case AV_SAMPLE_FMT_FLTP:
		bytePerOutSample = 4;
		break;
	case AV_SAMPLE_FMT_DBL:
	case AV_SAMPLE_FMT_DBLP:
	case AV_SAMPLE_FMT_S64:
	case AV_SAMPLE_FMT_S64P:
		bytePerOutSample = 8;
		break;
	default:
		bytePerOutSample = 2;
		break;
	}

	int guessOutSamplesPerChannel =
		av_rescale_rnd(inputSamples, out_sample_rate, in_sample_rate, AV_ROUND_UP);
	int guessOutSize = guessOutSamplesPerChannel * out_sample_rate * bytePerOutSample;

	std::cout << "GuessOutSamplesPerChannel: " << guessOutSamplesPerChannel << std::endl;
	std::cout << "GuessOutSize: " << guessOutSize << std::endl;

	guessOutSize *= 1.2;  // just make sure.

	//*outData = (uint8_t*)av_malloc(sizeof(uint8_t) * guessOutSize);
	*outData = (uint8_t*)av_malloc(2 * out_sample_rate);

	// av_samples_alloc(&outData, NULL, outChannels, guessOutSamplesPerChannel,
	// AV_SAMPLE_FMT_S16, 0);
	return guessOutSize;
}

int AudioPlayer::sfp_control_thread(bool& exitControl, bool& pause, float& volumn, bool& volumnChange, 
	bool& fast_foeward_10, bool& fast_foeward_30, bool& seek_req, bool& speed_req, double& speed_idx) {
	SDL_Event event;
	const Uint8* state = SDL_GetKeyboardState(NULL);
	bool key_space_down = false;
	bool key_plus_down = false;
	bool key_minus_down = false;
	while (!exitControl)
	{

		if (state[SDL_SCANCODE_SPACE] && !key_space_down) {
			key_space_down = true;
			pause = pause ? false : true;
		}
		else if (!state[SDL_SCANCODE_SPACE] && key_space_down) {
			key_space_down = false;
		}

		if (state[SDL_SCANCODE_KP_PLUS] && !key_plus_down) {
			key_plus_down = true;
			if (volumn < 5) {
				volumn += 0.1;
			}
		}
		else if (!state[SDL_SCANCODE_KP_PLUS] && key_plus_down) {
			key_plus_down = false;
		}

		if (state[SDL_SCANCODE_KP_MINUS] && !key_minus_down) {
			key_minus_down = true;
			if (volumn > 0) {
				volumn -= 0.1;
			}
		}
		else if (!state[SDL_SCANCODE_KP_MINUS] && key_minus_down) {
			key_minus_down = false;
		}
		volumnChange = (key_plus_down || key_minus_down);
		
		if (state[SDL_SCANCODE_KP_1] && !fast_foeward_10) {
			fast_foeward_10 = true;
			seek_req = true;
		}
		else if (!state[SDL_SCANCODE_KP_1] && fast_foeward_10) {
			fast_foeward_10 = false;
		}

		if (state[SDL_SCANCODE_KP_2] && !fast_foeward_30) {
			fast_foeward_30 = true;
			seek_req = true;
		}
		else if (!state[SDL_SCANCODE_KP_2] && fast_foeward_30) {
			fast_foeward_30 = false;
		}

		if (state[SDL_SCANCODE_KP_4] && !speed_req) {
			speed_req = true;
			speed_idx = 0.5;
		}

		if (state[SDL_SCANCODE_KP_5] && !speed_req) {
			speed_req = true;
			speed_idx = 0.75;
		}

		if (state[SDL_SCANCODE_KP_6] && !speed_req) {
			speed_req = true;
			speed_idx = 1;
		}

		if (state[SDL_SCANCODE_KP_7] && !speed_req) {
			speed_req = true;
			speed_idx = 1.5;
		}

		if (state[SDL_SCANCODE_KP_8] && !speed_req) {
			speed_req = true;
			speed_idx = 2;
		}

		//更新键盘状态
		state = SDL_GetKeyboardState(NULL);

	}
	return 0;
}

unsigned long ulFormat = 0;
bool fileGotToEnd = false;
int outSamples = 0;
int outDataSize = 0;
int AudioPlayer::feedAudioData_forSpeed(ALuint uiSource, ALuint alBufferId, bool isRevSample) {
	int ret = -1;

	uint8_t* outBuffer = (uint8_t*)av_malloc(2 * out_sample_rate);		//
	static int outBufferSize = 0;
	AVPacket* packet = (AVPacket*)av_malloc(sizeof(AVPacket));
	static AVFrame* aFrame = av_frame_alloc();
	soundtouch::SAMPLETYPE* touchBuffer = (soundtouch::SAMPLETYPE*)av_malloc(4 * out_sample_rate);

	while (true) {
		while (!fileGotToEnd) {

			if(seek_req) {
				AVRational av_get_time_base_q{ 1,1000000 };
				increase = fast_forward_10 ? 10.0 : (fast_forward_30 ? 30.0 : 0);
				//double targetFrame = av_rescale_q((double)ptsAudio / (double)1000 + increase, av_get_time_base_q, pFormatCtx->streams[index]->time_base);
				double targetFrame = ((double)ptsAudio / (double)1000 + increase) / av_q2d(pFormatCtx->streams[index]->time_base) / 1000000;
				av_seek_frame(pFormatCtx, index, targetFrame * AV_TIME_BASE, AVSEEK_FLAG_BACKWARD);
				//avcodec_flush_buffers(pCodeCtx);
				seek_req = false;
				continue;
			}

			if (av_read_frame(pFormatCtx, packet) >= 0) {

				if (packet->stream_index != index) {
					continue;
				}

				ret = avcodec_send_packet(pCodeCtx, packet);
				if (ret == 0) {
					av_packet_unref(packet);
					break;
				}
			}
			else {
				fileGotToEnd = true;
				if (pCodeCtx != nullptr) avcodec_send_packet(pCodeCtx, nullptr);
			}
		}

		ret = -1;
		ret = avcodec_receive_frame(pCodeCtx, aFrame);
		av_packet_unref(packet);
		if (ret == 0) {
			ret = 2;
			//ptsAudio = av_q2d(pFormatCtx->streams[index]->time_base) * packet->pts * 1000;
			ptsAudio = av_q2d(pFormatCtx->streams[index]->time_base) * aFrame->pts * 1000;
			cout << "ptsAudio:" << ptsAudio << "  time_base:" << av_q2d(pFormatCtx->streams[index]->time_base)
				<< "  aFrame->pts" << aFrame->pts << endl;
			break;
		}
		else if (ret == AVERROR_EOF) {
			cout << "no more output frames." << endl;
			ret = 0;
			break;
		}
		else if (ret == AVERROR(EAGAIN)) {
			continue;
		}
		else continue;
	}

	if (ret == 2) {
		if (outBuffer == nullptr) {
			outBufferSize = allocDataBuf(&outBuffer, aFrame->nb_samples);
		}
		else {
			memset(outBuffer, 0, outBufferSize);
		}

		

		outSamples = swr_convert(swrCtx, &outBuffer, 2 * out_sample_rate,
			(const uint8_t**)aFrame->data, aFrame->nb_samples);
		if (outSamples <= 0) {
			//throw std::runtime_error("error: outSamples=" + outSamples);
		}

		outDataSize = av_samples_get_buffer_size(NULL, out_channel_nb, outSamples, out_sample_fmt, 1);

		if (outDataSize <= 0) {
			//throw std::runtime_error("error: outDataSize=" + outDataSize);
		}

		

		if (aFrame->channels == 1) {
			ulFormat = AL_FORMAT_MONO16;
		}
		else {
			ulFormat = AL_FORMAT_STEREO16;
		}

		if (speed_req) {
			s_touch.setTempo(speed_idx);
			ptsAudioFromSpeed = ptsAudio;			//记录开始倍速播放时的pts
			cout << "ptsAudioFromSpeed:" << ptsAudioFromSpeed << "   speed_idx:" << speed_idx << endl;
			speed_req = false;
		}
		else if (speed_idx == 1) {
			ptsAudioFromSpeed = ptsAudio; 
		}
		for (int i = 0; i < outDataSize / 2 + 1; i++) {
			touchBuffer[i] = (outBuffer[i * 2] | (outBuffer[i * 2 + 1] << 8));
		}
		s_touch.putSamples(touchBuffer, outSamples);

		if (isRevSample) {
			int nSamples = 0;
			int touchDateSize;

			nSamples = s_touch.receiveSamples(touchBuffer, outSamples);
			touchDateSize = nSamples * out_channel_nb * av_get_bytes_per_sample(out_sample_fmt);
			alBufferData(alBufferId, ulFormat, touchBuffer, touchDateSize, out_sample_rate);
			alSourceQueueBuffers(uiSource, 1, &alBufferId);
			//ptsAudio = ptsAudioFromSpeed + (ptsAudio - ptsAudioFromSpeed) * speed_idx;			//更新倍速下的pts
			//cout << "ptsAudioForSpeed:" << ptsAudio << endl;
		}
		
	}
	return 0;
}

int AudioPlayer::feedAudioData(ALuint uiSource, ALuint alBufferId) {
	bool isRecSamples;
	if (speed_idx < 1) {
		if (speed_count >= (1 / (1 - speed_idx))) {
			speed_count = 1;
			int nSamples = 0;
			int touchDateSize;
			soundtouch::SAMPLETYPE* touchBuffer = (soundtouch::SAMPLETYPE*)av_malloc(4 * out_sample_rate);
			nSamples = s_touch.receiveSamples(touchBuffer, outSamples);
			touchDateSize = nSamples * out_channel_nb * av_get_bytes_per_sample(out_sample_fmt);
			alBufferData(alBufferId, ulFormat, touchBuffer, touchDateSize, out_sample_rate);
			alSourceQueueBuffers(uiSource, 1, &alBufferId);
		}
		else {
			speed_count += 1;
			feedAudioData_forSpeed(uiSource, alBufferId, true);
		}
	}
	else if (speed_idx > 1) {
		if (speed_count >= (1 / (speed_idx - 1) + 1)) {
			speed_count = 1;
			feedAudioData_forSpeed(uiSource, alBufferId, false);
			feedAudioData(uiSource, alBufferId);
		}
		else {
			speed_count += 1;
			feedAudioData_forSpeed(uiSource, alBufferId, true);
		}
	}
	else {
		feedAudioData_forSpeed(uiSource, alBufferId, true);
	}
	/*else if(speed_idx > 1) {
		if (speed_count >= (1 / (speed_idx - 1) + 1)) {

		}
	}*/
	return 0;
}


uint64_t AudioPlayer::getPts() {
	return ptsAudio;
}


int AudioPlayer::playByOpenAL(uint64_t* pts_audio)
{
	//--------初始化openAL--------
	ALCdevice* pDevice = alcOpenDevice(NULL);
	ALCcontext* pContext = alcCreateContext(pDevice, NULL);
	alcMakeContextCurrent(pContext);
	if (alcGetError(pDevice) != ALC_NO_ERROR)
		return AL_FALSE;
	ALuint source;
	alGenSources(1, &source);
	if (alGetError() != AL_NO_ERROR)
	{
		printf("Couldn't generate audio source\n");
		return -1;
	}
	ALfloat SourcePos[] = { 0.0,0.0,0.0 };
	ALfloat SourceVel[] = { 0.0,0.0,0.0 };
	ALfloat ListenerPos[] = { 0.0,0.0 };
	ALfloat ListenerVel[] = { 0.0,0.0,0.0 };
	ALfloat ListenerOri[] = { 0.0,0.0,-1.0,0.0,1.0,0.0 };
	alSourcef(source, AL_PITCH, 1.0);
	alSourcef(source, AL_GAIN, 1.0);
	alSourcefv(source, AL_POSITION, SourcePos);
	alSourcefv(source, AL_VELOCITY, SourceVel);
	alSourcef(source, AL_REFERENCE_DISTANCE, 50.0f);
	alSourcei(source, AL_LOOPING, AL_FALSE);
	//--------初始化openAL--------
	alDistanceModel(AL_LINEAR_DISTANCE_CLAMPED);
	alListener3f(AL_POSITION, 0, 0, 0);

	ALuint alBufferArray[NUMBUFFERS];

	alGenBuffers(NUMBUFFERS, alBufferArray);


	//启动播放和音量控制线程
	bool exitControl = false;
	bool pause = false;
	bool volumnChange = false;
	float volumn = 1.0;
	
	std::thread controlThread{ sfp_control_thread, std::ref(exitControl), std::ref(pause), std::ref(volumn), std::ref(volumnChange),
		std::ref(fast_forward_10), std::ref(fast_forward_30), std::ref(seek_req), std::ref(speed_req), std::ref(speed_idx) };



	// feed audio buffer first time.
	for (int i = 0; i < NUMBUFFERS; i++) {
		feedAudioData(source, alBufferArray[i]);
		//p.set_value(std::ref(ptsAudio));
		*pts_audio = ptsAudio;
	}

	// Start playing source
	alSourcePlay(source);


	ALint iTotalBuffersProcessed = 0;
	ALint iBuffersProcessed;
	ALint iState;
	ALuint bufferId;
	ALint iQueuedBuffers;
	while (true) {
		std::this_thread::sleep_for(std::chrono::milliseconds(SERVICE_UPDATE_PERIOD));

		// Request the number of OpenAL Buffers have been processed (played) on
		// the Source
		iBuffersProcessed = 0;
		alGetSourcei(source, AL_BUFFERS_PROCESSED, &iBuffersProcessed);
		iTotalBuffersProcessed += iBuffersProcessed;

		// Keep a running count of number of buffers processed (for logging
		// purposes only)
		iTotalBuffersProcessed += iBuffersProcessed;
		// cout << "Total Buffers Processed: " << iTotalBuffersProcessed << endl;
		// cout << "Processed: " << iBuffersProcessed << endl;

		// For each processed buffer, remove it from the Source Queue, read next
		// chunk of audio data from disk, fill buffer with new data, and add it
		// to the Source Queue
		while (iBuffersProcessed > 0) {
			bufferId = 0;
			alSourceUnqueueBuffers(source, 1, &bufferId);
			feedAudioData(source, bufferId);
			//p.set_value(std::ref(ptsAudio));
			*pts_audio = ptsAudio;
			iBuffersProcessed -= 1;
		}

		//改变音量
		if (volumnChange) {
			alSourcef(source, AL_GAIN, volumn);
			cout << "volumnChange" << endl;
		}

		/*if (speed_req) {
			//alSourcef(source, AL_PITCH, speed_idx);		//改变音调
			speed_req = false;
		}*/


		// Check the status of the Source.  If it is not playing, then playback
		// was completed, or the Source was starved of audio data, and needs to
		// be restarted.
		alGetSourcei(source, AL_SOURCE_STATE, &iState);

		if (pause) {
			alSourcePause(source);
		}
		else if (iState != AL_PLAYING) {
			// If there are Buffers in the Source Queue then the Source was
			// starved of audio data, so needs to be restarted (because there is
			// more audio data to play)
			alGetSourcei(source, AL_BUFFERS_QUEUED, &iQueuedBuffers);
			if (iQueuedBuffers) {
				alSourcePlay(source);
			}
			else {
				// Finished playing
				break;
			}
		}
	}


	return 0;
}



int AudioPlayer::play()
{
	return 0;
}
/*int AudioPlayer::play()
{
	//缂栫爜鏁版嵁
	AVPacket* packet = (AVPacket*)av_malloc(sizeof(AVPacket));
	//av_init_packet(packet);		//鍒濆鍖?
	//瑙ｅ帇缂╂暟鎹?
	AVFrame* frame = av_frame_alloc();
	//瀛樺偍pcm鏁版嵁
	uint8_t* out_buffer = (uint8_t*)av_malloc(2 * RATE);

	int ret, got_frame, framecount = 0;
	//涓�甯т竴甯ц鍙栧帇缂╃殑闊抽鏁版嵁AVPacket

	while (av_read_frame(pFormatCtx, packet) >= 0) {
		if (packet->stream_index == index) {
				//瑙ｇ爜AVPacket->AVFrame
			ret = avcodec_decode_audio4(pCodeCtx, frame, &got_frame, packet);

			//ret = avcodec_send_packet(pCodeCtx, packet);
			//if (ret != 0) { printf("%s/n", "error"); }
			//got_frame = avcodec_receive_frame(pCodeCtx, frame);			//output=0銆媠uccess, a frame was returned
			//while ((got_frame = avcodec_receive_frame(pCodeCtx, frame)) == 0) {
			//		//璇诲彇鍒颁竴甯ч煶棰戞垨鑰呰棰?
			//		//澶勭悊瑙ｇ爜鍚庨煶瑙嗛 frame
			//		swr_convert(swrCtx, &out_buffer, 2 * 44100, (const uint8_t**)frame->data, frame->nb_samples);
			//}


			if (ret < 0) {
				 printf("%s", "瑙ｇ爜瀹屾垚");
			}

			//闈?锛屾鍦ㄨВ鐮?
			int out_buffer_size;
			if (got_frame) {
				//printf("瑙ｇ爜%d甯?, framecount++);
				swr_convert(swrCtx, &out_buffer, 2 * 44100, (const uint8_t**)frame->data, frame->nb_samples);
				//鑾峰彇sample鐨剆ize
				out_buffer_size = av_samples_get_buffer_size(NULL, out_channel_nb, frame->nb_samples,
					out_sample_fmt, 1);
				//璁剧疆闊抽鏁版嵁缂撳啿,PCM鏁版嵁
				audio_chunk = (Uint8*)out_buffer;
				//璁剧疆闊抽鏁版嵁闀垮害
				audio_len = out_buffer_size;
				audio_pos = audio_chunk;
				//鍥炴斁闊抽鏁版嵁
				SDL_PauseAudio(0);
				while (audio_len > 0)//绛夊緟鐩村埌闊抽鏁版嵁鎾斁瀹屾瘯!
					SDL_Delay(10);
				packet->data += ret;
				packet->size -= ret;
			}
		}
		//av_packet_unref(packet);
	}

	av_free(out_buffer);
	av_frame_free(&frame);
	//av_free_packet(packet);
	av_packet_unref(packet);
	SDL_CloseAudio();//鍏抽棴闊抽璁惧
	swr_free(&swrCtx);
	return 0;
}*/
