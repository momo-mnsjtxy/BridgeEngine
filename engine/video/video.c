#include "platform/platform.h"
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "video/video.h"
#include "bapi_internal.h"

#define VIDEO_AUDIO_BYTES_PER_SAMPLE ((int)sizeof(float) * 2)

struct bapi_video_internal {
	AVFormatContext* format_ctx;
	AVCodecContext* codec_ctx;
	AVStream* video_stream;
	struct SwsContext* sws_ctx;
	struct SwrContext* swr_ctx;
	AVCodecContext* audio_ctx;
	AVFrame* frame;
	AVFrame* frame_rgb;
	AVPacket* packet;
	uint8_t* buffer;
	int buffer_size;

	plat_texture_t texture;
	plat_audio_stream_t audio_stream;
	AVFrame* audio_frame;
	uint8_t* audio_buffer;
	int audio_buffer_size;

	int video_stream_idx;
	int audio_stream_idx;
	int source_width;
	int source_height;
	enum AVPixelFormat source_pix_fmt;
	int width;
	int height;
	double fps;
	double time_base;

	int playing;
	int paused;
	int loop;
	float volume;
	double current_time;
	double duration;
	int demux_eof;

	char* filepath;
	uint32_t last_update;
};

static bapi_video_t g_current_video = NULL;

static void reset_audio_playback(bapi_video_t video)
{
	const plat_interface_t* plat = plat_get();
	if (video->audio_stream) {
		plat->clear_audio_stream(video->audio_stream);
	}
	if (video->audio_ctx) {
		avcodec_flush_buffers(video->audio_ctx);
	}
	if (video->swr_ctx) {
		swr_close(video->swr_ctx);
		swr_init(video->swr_ctx);
	}
}

static void reset_video_playback_state(bapi_video_t video)
{
	video->demux_eof = 0;
	avcodec_flush_buffers(video->codec_ctx);
	if (video->audio_ctx) {
		avcodec_flush_buffers(video->audio_ctx);
	}
	reset_audio_playback(video);
}

static int ensure_video_output(bapi_video_t video, int width, int height)
{
	if (width <= 0 || height <= 0) {
		return -1;
	}

	const plat_interface_t* plat = plat_get();

	if (video->texture != NULL && video->buffer != NULL && video->width == width && video->height == height) {
		return 0;
	}

	int buffer_size = av_image_get_buffer_size(AV_PIX_FMT_BGRA, width, height, 1);
	if (buffer_size < 0) {
		return -1;
	}

	uint8_t* buffer = av_malloc((size_t)buffer_size);
	if (buffer == NULL) {
		return -1;
	}

	int fill_result = av_image_fill_arrays(
		video->frame_rgb->data,
		video->frame_rgb->linesize,
		buffer,
		AV_PIX_FMT_BGRA,
		width,
		height,
		1
	);
	if (fill_result < 0) {
		av_free(buffer);
		return -1;
	}

	plat_texture_t texture = video->texture;
	if (texture == NULL || video->width != width || video->height != height) {
		texture = plat->create_texture(bapi_internal_renderer,
					       PLAT_PIXELFORMAT_ARGB8888,
					       PLAT_TEXTUREACCESS_STREAMING,
					       width, height);
		if (!texture) {
			av_free(buffer);
			return -1;
		}
	}

	if (texture != video->texture && video->texture != NULL) {
		plat->destroy_texture(video->texture);
	}
	if (video->buffer != NULL) {
		av_free(video->buffer);
	}

	video->buffer = buffer;
	video->buffer_size = buffer_size;
	video->texture = texture;
	video->width = width;
	video->height = height;
	video->frame_rgb->format = AV_PIX_FMT_BGRA;
	video->frame_rgb->width = width;
	video->frame_rgb->height = height;

	return 0;
}

static int ensure_sws_context(bapi_video_t video, const AVFrame* frame)
{
	enum AVPixelFormat frame_format = (enum AVPixelFormat)frame->format;

	if (frame_format == AV_PIX_FMT_NONE || frame->width <= 0 || frame->height <= 0) {
		return -1;
	}

	if (video->sws_ctx != NULL &&
	    video->source_width == frame->width &&
	    video->source_height == frame->height &&
	    video->source_pix_fmt == frame_format) {
		return 0;
	}

	if (video->sws_ctx != NULL) {
		sws_freeContext(video->sws_ctx);
		video->sws_ctx = NULL;
	}

	video->sws_ctx = sws_getContext(
		frame->width,
		frame->height,
		frame_format,
		frame->width,
		frame->height,
		AV_PIX_FMT_BGRA,
		SWS_BILINEAR,
		NULL,
		NULL,
		NULL
	);
	if (video->sws_ctx == NULL) {
		return -1;
	}

	video->source_width = frame->width;
	video->source_height = frame->height;
	video->source_pix_fmt = frame_format;
	return 0;
}

static int present_video_frame(bapi_video_t video, AVFrame* frame)
{
	const plat_interface_t* plat = plat_get();

	if (ensure_video_output(video, frame->width, frame->height) < 0) {
		printf("[VIDEO] Error: Failed to prepare output surface\n");
		return -1;
	}

	if (ensure_sws_context(video, frame) < 0) {
		printf("[VIDEO] Error: Failed to create sws context\n");
		return -1;
	}

	int scaled_height = sws_scale(
		video->sws_ctx,
		(const uint8_t* const*)frame->data,
		frame->linesize,
		0,
		frame->height,
		video->frame_rgb->data,
		video->frame_rgb->linesize
	);
	if (scaled_height <= 0) {
		printf("[VIDEO] Error: Failed to scale video frame\n");
		return -1;
	}

	int64_t timestamp = frame->best_effort_timestamp;
	if (timestamp == AV_NOPTS_VALUE) {
		timestamp = frame->pts;
	}
	if (timestamp != AV_NOPTS_VALUE) {
		video->current_time = timestamp * video->time_base;
	}

	if (plat->update_texture(video->texture, video->frame_rgb->data[0], video->frame_rgb->linesize[0]) != 0) {
		printf("[VIDEO] Error: Failed to update texture\n");
		return -1;
	}

	return 0;
}

static int queue_audio_frame(bapi_video_t video, AVFrame* frame)
{
	const plat_interface_t* plat = plat_get();

	int out_samples = av_rescale_rnd(
		swr_get_delay(video->swr_ctx, video->audio_ctx->sample_rate) + frame->nb_samples,
		44100,
		video->audio_ctx->sample_rate,
		AV_ROUND_UP
	);
	int required_size = av_samples_get_buffer_size(NULL, 2, out_samples, AV_SAMPLE_FMT_FLT, 0);
	if (required_size < 0) {
		printf("[VIDEO] Failed to compute audio buffer size\n");
		return -1;
	}

	if (required_size > video->audio_buffer_size) {
		uint8_t* resized = av_realloc(video->audio_buffer, required_size);
		if (resized == NULL) {
			printf("[VIDEO] Failed to allocate audio buffer\n");
			return -1;
		}
		video->audio_buffer = resized;
		video->audio_buffer_size = required_size;
	}

	uint8_t* output[] = {video->audio_buffer, NULL};
	int converted_samples = swr_convert(video->swr_ctx, output, out_samples, (const uint8_t* const*)frame->extended_data, frame->nb_samples);
	if (converted_samples < 0) {
		printf("[VIDEO] Failed to resample audio frame\n");
		return -1;
	}

	int output_size = converted_samples * VIDEO_AUDIO_BYTES_PER_SAMPLE;
	if (output_size <= 0) {
		return 0;
	}

	if (video->volume < 0.99f) {
		float* samples = (float*)video->audio_buffer;
		int sample_count = output_size / (int)sizeof(float);
		for (int i = 0; i < sample_count; ++i) {
			samples[i] *= video->volume;
		}
	}

	if (plat->put_audio_stream_data(video->audio_stream, video->audio_buffer, output_size) != 0) {
		printf("[VIDEO] Failed to queue audio data\n");
		return -1;
	}

	if (plat->flush_audio_stream(video->audio_stream) != 0) {
		printf("[VIDEO] Failed to flush audio stream\n");
		return -1;
	}

	return 0;
}

static int decode_audio_packet(bapi_video_t video)
{
	int ret = avcodec_send_packet(video->audio_ctx, video->packet);
	if (ret < 0) {
		return ret;
	}

	while ((ret = avcodec_receive_frame(video->audio_ctx, video->audio_frame)) >= 0) {
		if (queue_audio_frame(video, video->audio_frame) < 0) {
			av_frame_unref(video->audio_frame);
			return -1;
		}
		av_frame_unref(video->audio_frame);
	}

	return ret == AVERROR(EAGAIN) || ret == AVERROR_EOF ? 0 : ret;
}

static int init_audio_decoder(bapi_video_t video)
{
	const plat_interface_t* plat = plat_get();

	if (video->audio_stream_idx < 0) {
		return 0;
	}

	const AVCodec* audio_codec = avcodec_find_decoder(video->format_ctx->streams[video->audio_stream_idx]->codecpar->codec_id);
	if (!audio_codec) {
		printf("[VIDEO] Audio codec not found\n");
		return -1;
	}

	video->audio_ctx = avcodec_alloc_context3(audio_codec);
	if (!video->audio_ctx) {
		return -1;
	}

	if (avcodec_parameters_to_context(video->audio_ctx, video->format_ctx->streams[video->audio_stream_idx]->codecpar) < 0) {
		avcodec_free_context(&video->audio_ctx);
		return -1;
	}

	if (avcodec_open2(video->audio_ctx, audio_codec, NULL) < 0) {
		avcodec_free_context(&video->audio_ctx);
		return -1;
	}

	AVChannelLayout input_layout = {0};
	AVChannelLayout output_layout = AV_CHANNEL_LAYOUT_STEREO;

	if (av_channel_layout_copy(&input_layout, &video->audio_ctx->ch_layout) < 0 || input_layout.nb_channels <= 0) {
		av_channel_layout_uninit(&input_layout);
		av_channel_layout_default(&input_layout, 2);
	}

	if (swr_alloc_set_opts2(
		    &video->swr_ctx,
		    &output_layout,
		    AV_SAMPLE_FMT_FLT,
		    44100,
		    &input_layout,
		    video->audio_ctx->sample_fmt,
		    video->audio_ctx->sample_rate,
		    0,
		    NULL) < 0) {
		av_channel_layout_uninit(&input_layout);
		avcodec_free_context(&video->audio_ctx);
		return -1;
	}

	av_channel_layout_uninit(&input_layout);

	if (!video->swr_ctx || swr_init(video->swr_ctx) < 0) {
		swr_free(&video->swr_ctx);
		avcodec_free_context(&video->audio_ctx);
		return -1;
	}

	video->audio_stream = plat->open_audio_device_stream(PLAT_AUDIO_F32, 2, 44100);
	if (!video->audio_stream) {
		swr_free(&video->swr_ctx);
		avcodec_free_context(&video->audio_ctx);
		printf("[VIDEO] Failed to open audio device stream\n");
		return -1;
	}

	video->audio_frame = av_frame_alloc();
	if (!video->audio_frame) {
		plat->destroy_audio_stream(video->audio_stream);
		video->audio_stream = NULL;
		swr_free(&video->swr_ctx);
		avcodec_free_context(&video->audio_ctx);
		return -1;
	}

	plat->resume_audio_stream_device(video->audio_stream);

	return 0;
}

int bapi_video_init(void)
{
	printf("[VIDEO] Video subsystem initialized (FFmpeg version)\n");
	return 0;
}

void bapi_video_cleanup(void)
{
	g_current_video = NULL;
	printf("[VIDEO] Video subsystem cleaned up\n");
}

bapi_video_t bapi_video_load(const char* filepath)
{
	if (filepath == NULL) {
		printf("[VIDEO] Error: filepath is NULL\n");
		return NULL;
	}

	if (bapi_internal_renderer == NULL) {
		printf("[VIDEO] Error: renderer not initialized\n");
		return NULL;
	}

	bapi_video_t video = malloc(sizeof(struct bapi_video_internal));
	if (video == NULL) {
		printf("[VIDEO] Error: Failed to allocate video struct\n");
		return NULL;
	}
	memset(video, 0, sizeof(struct bapi_video_internal));

	video->filepath = strdup(filepath);
	video->volume = 1.0f;
	video->loop = 0;
	video->playing = 0;
	video->paused = 0;
	video->video_stream_idx = -1;
	video->audio_stream_idx = -1;
	video->source_pix_fmt = AV_PIX_FMT_NONE;

	if (avformat_open_input(&video->format_ctx, filepath, NULL, NULL) != 0) {
		printf("[VIDEO] Error: Cannot open video file %s\n", filepath);
		free(video->filepath);
		free(video);
		return NULL;
	}

	if (avformat_find_stream_info(video->format_ctx, NULL) < 0) {
		printf("[VIDEO] Error: Cannot find stream info\n");
		avformat_close_input(&video->format_ctx);
		free(video->filepath);
		free(video);
		return NULL;
	}

	for (unsigned int i = 0; i < video->format_ctx->nb_streams; i++) {
		if (video->format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && video->video_stream_idx < 0) {
			video->video_stream_idx = i;
		}
		if (video->format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && video->audio_stream_idx < 0) {
			video->audio_stream_idx = i;
		}
	}

	if (video->video_stream_idx < 0) {
		printf("[VIDEO] Error: No video stream found\n");
		avformat_close_input(&video->format_ctx);
		free(video->filepath);
		free(video);
		return NULL;
	}

	video->video_stream = video->format_ctx->streams[video->video_stream_idx];
	const AVCodec* codec = avcodec_find_decoder(video->video_stream->codecpar->codec_id);
	if (!codec) {
		printf("[VIDEO] Error: Codec not found\n");
		avformat_close_input(&video->format_ctx);
		free(video->filepath);
		free(video);
		return NULL;
	}

	video->codec_ctx = avcodec_alloc_context3(codec);
	if (!video->codec_ctx) {
		printf("[VIDEO] Error: Failed to allocate codec context\n");
		avformat_close_input(&video->format_ctx);
		free(video->filepath);
		free(video);
		return NULL;
	}

	if (avcodec_parameters_to_context(video->codec_ctx, video->video_stream->codecpar) < 0) {
		printf("[VIDEO] Error: Failed to copy codec parameters\n");
		avcodec_free_context(&video->codec_ctx);
		avformat_close_input(&video->format_ctx);
		free(video->filepath);
		free(video);
		return NULL;
	}

	if (avcodec_open2(video->codec_ctx, codec, NULL) < 0) {
		printf("[VIDEO] Error: Failed to open codec\n");
		avcodec_free_context(&video->codec_ctx);
		avformat_close_input(&video->format_ctx);
		free(video->filepath);
		free(video);
		return NULL;
	}

	video->time_base = av_q2d(video->video_stream->time_base);
	video->duration = (double)video->format_ctx->duration / AV_TIME_BASE;

	AVRational frame_rate = av_guess_frame_rate(video->format_ctx, video->video_stream, NULL);
	if (frame_rate.num > 0 && frame_rate.den > 0) {
		video->fps = av_q2d(frame_rate);
	} else {
		video->fps = 30.0;
	}

	video->frame = av_frame_alloc();
	video->frame_rgb = av_frame_alloc();
	if (!video->frame || !video->frame_rgb) {
		printf("[VIDEO] Error: Failed to allocate frames\n");
		goto cleanup_error;
	}

	video->packet = av_packet_alloc();
	if (!video->packet) {
		printf("[VIDEO] Error: Failed to allocate packet\n");
		goto cleanup_error;
	}

	if (init_audio_decoder(video) < 0) {
		printf("[VIDEO] Warning: audio track disabled for %s\n", filepath);
	}

	printf("[VIDEO] Loaded: %s (%dx%d @ %.2f fps, duration: %.2fs)\n",
	       filepath, video->width, video->height, video->fps, video->duration);

	return video;

cleanup_error:
	if (video->sws_ctx) sws_freeContext(video->sws_ctx);
	if (video->buffer) av_free(video->buffer);
	if (video->frame_rgb) av_frame_free(&video->frame_rgb);
	if (video->frame) av_frame_free(&video->frame);
	if (video->codec_ctx) avcodec_free_context(&video->codec_ctx);
	if (video->format_ctx) avformat_close_input(&video->format_ctx);
	if (video->filepath) free(video->filepath);
	free(video);
	return NULL;
}

void bapi_video_free(bapi_video_t video)
{
	if (video == NULL) {
		return;
	}

	const plat_interface_t* plat = plat_get();

	if (g_current_video == video) {
		g_current_video = NULL;
	}

	if (video->audio_stream) {
		plat->destroy_audio_stream(video->audio_stream);
	}
	if (video->audio_frame) {
		av_frame_free(&video->audio_frame);
	}
	if (video->swr_ctx) {
		swr_free(&video->swr_ctx);
	}
	if (video->packet) {
		av_packet_free(&video->packet);
	}
	if (video->texture) {
		plat->destroy_texture(video->texture);
	}
	if (video->sws_ctx) {
		sws_freeContext(video->sws_ctx);
	}
	if (video->buffer) {
		av_free(video->buffer);
	}
	if (video->frame_rgb) {
		av_frame_free(&video->frame_rgb);
	}
	if (video->frame) {
		av_frame_free(&video->frame);
	}
	if (video->codec_ctx) {
		avcodec_free_context(&video->codec_ctx);
	}
	if (video->format_ctx) {
		avformat_close_input(&video->format_ctx);
	}
	if (video->filepath) {
		free(video->filepath);
	}
	if (video->audio_ctx) {
		avcodec_free_context(&video->audio_ctx);
	}
	if (video->audio_buffer) {
		av_free(video->audio_buffer);
	}

	free(video);
}

static int decode_video_frame(bapi_video_t video)
{
	while (1) {
		int ret = avcodec_receive_frame(video->codec_ctx, video->frame);
		if (ret == 0) {
			int present_result = present_video_frame(video, video->frame);
			av_frame_unref(video->frame);
			return present_result;
		}
		if (ret != AVERROR(EAGAIN)) {
			return ret == AVERROR_EOF ? -1 : ret;
		}

		if (!video->demux_eof) {
			ret = av_read_frame(video->format_ctx, video->packet);
			if (ret < 0) {
				video->demux_eof = 1;
				avcodec_send_packet(video->codec_ctx, NULL);
				if (video->audio_ctx) {
					avcodec_send_packet(video->audio_ctx, NULL);
				}
				continue;
			}

			if (video->packet->stream_index == video->video_stream_idx) {
				ret = avcodec_send_packet(video->codec_ctx, video->packet);
				av_packet_unref(video->packet);
				if (ret < 0 && ret != AVERROR(EAGAIN)) {
					return ret;
				}
				continue;
			}

			if (video->audio_ctx != NULL && video->packet->stream_index == video->audio_stream_idx) {
				ret = decode_audio_packet(video);
				av_packet_unref(video->packet);
				if (ret < 0) {
					return ret;
				}
				continue;
			}

			av_packet_unref(video->packet);
			continue;
		}

		return -1;
	}
}

int bapi_video_play(bapi_video_t video)
{
	if (video == NULL) {
		return 1;
	}

	const plat_interface_t* plat = plat_get();

	if (!video->playing) {
		av_seek_frame(video->format_ctx, video->video_stream_idx, 0, AVSEEK_FLAG_BACKWARD);
		reset_video_playback_state(video);
		video->current_time = 0;
	}

	video->playing = 1;
	video->paused = 0;
	video->last_update = plat->get_ticks();
	g_current_video = video;

	printf("[VIDEO] Playing: %s\n", video->filepath);
	return 0;
}

void bapi_video_pause(bapi_video_t video)
{
	if (video == NULL) {
		return;
	}

	video->paused = !video->paused;
	printf("[VIDEO] %s: %s\n", video->paused ? "Paused" : "Resumed", video->filepath);
}

void bapi_video_stop(bapi_video_t video)
{
	if (video == NULL) {
		return;
	}

	const plat_interface_t* plat = plat_get();

	video->playing = 0;
	video->paused = 0;
	video->current_time = 0;

	if (video->audio_stream) {
		plat->clear_audio_stream(video->audio_stream);
	}
	if (video->audio_ctx) {
		avcodec_flush_buffers(video->audio_ctx);
	}
	video->demux_eof = 0;

	if (g_current_video == video) {
		g_current_video = NULL;
	}

	printf("[VIDEO] Stopped: %s\n", video->filepath);
}

void bapi_video_render(bapi_video_t video, int x, int y, int w, int h)
{
	if (video == NULL || video->texture == NULL || bapi_internal_renderer == NULL) {
		return;
	}

	const plat_interface_t* plat = plat_get();
	plat->render_texture(bapi_internal_renderer, video->texture, (float)x, (float)y, (float)w, (float)h);
}

void bapi_video_render_fit(bapi_video_t video, int area_x, int area_y, int area_w, int area_h)
{
	if (video == NULL || video->texture == NULL || bapi_internal_renderer == NULL) {
		return;
	}

	const plat_interface_t* plat = plat_get();

	float video_aspect = (float)video->width / (float)video->height;
	float area_aspect = (float)area_w / (float)area_h;

	int render_w, render_h, render_x, render_y;

	if (video_aspect > area_aspect) {
		render_w = area_w;
		render_h = (int)(area_w / video_aspect);
		render_x = area_x;
		render_y = area_y + (area_h - render_h) / 2;
	} else {
		render_h = area_h;
		render_w = (int)(area_h * video_aspect);
		render_x = area_x + (area_w - render_w) / 2;
		render_y = area_y;
	}

	plat->render_texture(bapi_internal_renderer, video->texture,
			     (float)render_x, (float)render_y, (float)render_w, (float)render_h);
}

void bapi_video_render_center(bapi_video_t video, int window_w, int window_h)
{
	if (video == NULL || video->texture == NULL || bapi_internal_renderer == NULL) {
		return;
	}

	const plat_interface_t* plat = plat_get();

	float video_aspect = (float)video->width / (float)video->height;
	float window_aspect = (float)window_w / (float)window_h;

	int render_w, render_h, render_x, render_y;

	if (video_aspect > window_aspect) {
		render_w = window_w;
		render_h = (int)(window_w / video_aspect);
		render_x = 0;
		render_y = (window_h - render_h) / 2;
	} else {
		render_h = window_h;
		render_w = (int)(window_h * video_aspect);
		render_x = (window_w - render_w) / 2;
		render_y = 0;
	}

	plat->render_texture(bapi_internal_renderer, video->texture,
			     (float)render_x, (float)render_y, (float)render_w, (float)render_h);
}

void bapi_video_set_loop(bapi_video_t video, int loop)
{
	if (video != NULL) {
		video->loop = loop;
	}
}

void bapi_video_set_volume(bapi_video_t video, float volume)
{
	if (video != NULL) {
		if (volume < 0.0f) {
			video->volume = 0.0f;
		} else if (volume > 1.0f) {
			video->volume = 1.0f;
		} else {
			video->volume = volume;
		}
	}
}

int bapi_video_is_playing(bapi_video_t video)
{
	if (video == NULL) {
		return 0;
	}
	return video->playing && !video->paused;
}

void bapi_video_get_size(bapi_video_t video, int* w, int* h)
{
	if (video == NULL) {
		if (w != NULL) *w = 0;
		if (h != NULL) *h = 0;
		return;
	}

	if (w != NULL) *w = video->width;
	if (h != NULL) *h = video->height;
}

void bapi_video_update(void)
{
	if (g_current_video == NULL || !g_current_video->playing || g_current_video->paused) {
		return;
	}

	const plat_interface_t* plat = plat_get();
	bapi_video_t video = g_current_video;

	uint32_t now = plat->get_ticks();
	uint32_t frame_delay = (uint32_t)(1000.0 / video->fps);

	if (now - video->last_update < frame_delay) {
		return;
	}

	video->last_update = now;

	if (decode_video_frame(video) < 0) {
		if (video->loop) {
			av_seek_frame(video->format_ctx, video->video_stream_idx, 0, AVSEEK_FLAG_BACKWARD);
			reset_video_playback_state(video);
			video->current_time = 0;
			decode_video_frame(video);
		} else {
			video->playing = 0;
			g_current_video = NULL;
			printf("[VIDEO] Playback finished: %s\n", video->filepath);
		}
	}
}
