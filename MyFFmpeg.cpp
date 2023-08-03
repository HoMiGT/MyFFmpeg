#include "MyFFmpeg.h"

#include <cmath>

void MyFFmpeg::crop(int index) noexcept {
	auto src_format = static_cast<AVPixelFormat>(m_frame->format);
	auto m_sws_context = sws_getContext(m_frame->width, m_frame->height, src_format, m_frame->width, m_frame->height, AV_PIX_FMT_BGR24, SWS_BILINEAR, nullptr, nullptr, nullptr);
	av_image_fill_arrays(m_frame_bgr->data, m_frame_bgr->linesize, m_bgr_buffer, AV_PIX_FMT_BGR24, m_frame->width, m_frame->height, 1);
	sws_scale(m_sws_context, m_frame->data, m_frame->linesize, 0, m_frame->height, m_frame_bgr->data, m_frame_bgr->linesize);
	cv::Mat img(m_frame->height, m_frame->width, CV_8UC3, m_bgr_buffer, m_frame_bgr->linesize[0]);
	cv::Mat crop = img(cv::Rect(m_crop_x, m_crop_y, m_crop_width, m_crop_height));
	//cv::imwrite("test" + std::to_string(index) + ".png", crop);
}

void MyFFmpeg::clean_up() {
	if (m_bgr_buffer) {
		av_free(m_bgr_buffer);
	}
	m_bgr_buffer = nullptr;

	if (m_frame_bgr) {
		av_frame_free(&m_frame_bgr);
	}
	m_frame_bgr = nullptr;

	if (m_frame) {
		av_frame_free(&m_frame);
	}
	m_frame = nullptr;

	if (m_packet) {
		av_packet_free(&m_packet);
	}
	m_packet = nullptr;

	if (m_codec_ctx) {
		avcodec_free_context(&m_codec_ctx);
	}
	m_codec_ctx = nullptr;

	if (m_format_ctx) {
		avformat_close_input(&m_format_ctx);
		avformat_free_context(m_format_ctx);
	}
	m_format_ctx = nullptr;

	if (m_format_options) {
		av_dict_free(&m_format_options);
	}
	m_format_options = nullptr;

}

int MyFFmpeg::initialize() noexcept {
	m_format_ctx = avformat_alloc_context();
	if (!m_format_ctx) PKRT(INIT_ERR_ALLOC_FORMAT);
	m_input_format = av_find_input_format("flv");
	if (!m_input_format) PKRT(INIT_ERR_ALLOC_INPUT_FORMAT);
	m_ret = avformat_open_input(&m_format_ctx, m_video_path.c_str(), m_input_format, &m_format_options);
	if (m_ret) PKRT(m_ret);
	m_ret = avformat_find_stream_info(m_format_ctx, nullptr);
	if (m_ret < 0) PKRT(m_ret);
	m_ret = av_find_best_stream(m_format_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
	if (m_ret < 0) PKRT(m_ret);
	m_stream_index = m_ret;
	av_dump_format(m_format_ctx, -1, m_video_path.c_str(), 0);
	m_codec = avcodec_find_decoder(m_format_ctx->streams[m_stream_index]->codecpar->codec_id);
	if (!m_codec) PKRT(INIT_ERR_FIND_DECODER);
	m_codec_ctx = avcodec_alloc_context3(m_codec);
	if (!m_codec_ctx) PKRT(INIT_ERR_ALLOC_CODEC_CONTEXT);
	m_ret = avcodec_parameters_to_context(m_codec_ctx, m_format_ctx->streams[m_stream_index]->codecpar);
	if (m_ret < 0) PKRT(m_ret);
	m_ret = avcodec_open2(m_codec_ctx, m_codec, nullptr);
	if (m_ret) PKRT(m_ret);
	m_packet = av_packet_alloc();
	if (!m_packet) PKRT(INIT_ERR_ALLOC_PACKET);
	m_frame = av_frame_alloc();
	m_frame_bgr = av_frame_alloc();
	if (!m_frame || !m_frame_bgr) PKRT(INIT_ERR_ALLOC_FRAME);
	int one_frame_bytes = av_image_get_buffer_size(AV_PIX_FMT_BGR24, m_codec_ctx->width, m_codec_ctx->height, 1);
	m_bgr_buffer = static_cast<uint8_t*>(av_malloc(one_frame_bytes));
	if (!m_bgr_buffer) PKRT(INIT_ERR_ALLOC_BGR_BUFFER);
	m_crop_width = m_codec_ctx->width;
	m_crop_height = static_cast<int>(floor(m_codec_ctx->height * (m_crop_y_rate >= 1 ? 1 : m_crop_y_rate)));
	m_crop_y = static_cast<int>(floor((m_codec_ctx->height - m_crop_height) / 2));
	m_crop_x = 0;
	PKRT(MYFS_SUCCESS);
}

int MyFFmpeg::frame(int index) {
	m_ret = av_read_frame(m_format_ctx, m_packet);
	if (m_ret < 0 && m_ret != AV_EOF) PKRT(m_ret);
	if (m_packet->stream_index == m_stream_index) {
		m_ret = avcodec_send_packet(m_codec_ctx, m_packet);
		if (m_ret < 0) PKRT(m_ret);
		m_ret = avcodec_receive_frame(m_codec_ctx, m_frame);
		if (m_ret == OTHER_ERROR_EAGAIN || m_ret == AV_EOF) PKRT(m_ret);
		crop(index);
		av_frame_unref(m_frame);
		av_packet_unref(m_packet);
		PKRT(MYFS_SUCCESS);
	}
	PKRT(OTHER_ERROR_CONTINUE);
}


