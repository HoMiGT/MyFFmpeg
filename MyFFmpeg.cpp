#include "MyFFmpeg.h"

#include <memory>
#include <random>
#include <cmath>
#include <thread>

MyFFmpeg::MyFFmpeg(const std::string& video_path,
	double crop_y_rate,
	int open_try_count,
	int packet_of_frame,
	bool print_av_format
)
	: m_video_path(video_path)
	, m_crop_y_rate(crop_y_rate)
	, m_open_try_count(open_try_count)
	, m_packet_of_frames(packet_of_frame)
	, m_print_av_format(print_av_format)
{
	av_dict_set_int(&m_format_options, "probesize", 10485760, 0); // 设置探测大小 10MB
	av_dict_set_int(&m_format_options, "analyzeduration", 2000000, 0); // 设置分析时长 2s
	av_dict_set(&m_format_options, "rtsp_transport", "tcp", 0);
	av_dict_set(&m_format_options, "max_delay", "250", 0);
	av_dict_set(&m_format_options, "max_analyze_duration", "2000000", 0);  // 设置最大分析时长 2s
	av_dict_set(&m_format_options, "stimeout", "2000000", 0); // 设置超时时间 2s
};

MyFFmpeg::~MyFFmpeg() {
	clean_up();
}


void MyFFmpeg::crop() noexcept {
	auto src_format = static_cast<AVPixelFormat>(m_frame->format);
	auto m_sws_context = sws_getContext(m_frame->width, m_frame->height, src_format, m_frame->width, m_frame->height, AV_PIX_FMT_BGR24, SWS_BILINEAR, nullptr, nullptr, nullptr);
	av_image_fill_arrays(m_frame_bgr->data, m_frame_bgr->linesize, m_bgr_buffer, AV_PIX_FMT_BGR24, m_frame->width, m_frame->height, 1);
	sws_scale(m_sws_context, m_frame->data, m_frame->linesize, 0, m_frame->height, m_frame_bgr->data, m_frame_bgr->linesize);
	cv::Mat img(m_frame->height, m_frame->width, CV_8UC3, m_bgr_buffer, m_frame_bgr->linesize[0]);
	cv::Mat crop = img(cv::Rect(m_crop_x, m_crop_y, m_crop_width, m_crop_height));
	m_frames.emplace_back(std::move(crop));
}

/// <summary>
/// 波动式随机等待
/// </summary>
/// <param name="attempt"></param>
/// <returns></returns>
std::chrono::milliseconds MyFFmpeg::sleep(int attempt)
{
	static std::array<int, 10> arr_wait = { 3,1,5,1,5,1,3,1,3,1 }; // 24
	static std::default_random_engine engine(std::random_device{}());
	static std::uniform_real_distribution<double> distribution(0.0, 1.0);
	int delay = arr_wait[attempt % 10] * 100 + distribution(engine) * 100; // 计算等待时间
	return std::chrono::milliseconds(delay);
}
void MyFFmpeg::clean_up() {
	avformat_open_input(&m_format_ctx, m_video_path.c_str(), m_input_format, &m_format_options);
	avformat_network_deinit();
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
		avformat_free_context(m_format_ctx);
	}
	m_format_ctx = nullptr;

	if (m_format_options) {
		av_dict_free(&m_format_options);
	}
	m_format_options = nullptr;

}

int MyFFmpeg::initialize() noexcept {
	avformat_network_init();
	m_format_ctx = avformat_alloc_context();
	if (!m_format_ctx) PKRT(INIT_ERR_ALLOC_FORMAT);
	m_input_format = av_find_input_format("flv");
	if (!m_input_format) PKRT(INIT_ERR_ALLOC_INPUT_FORMAT);
	std::chrono::milliseconds delay;
	do {
		m_ret = avformat_open_input(&m_format_ctx, m_video_path.c_str(), m_input_format, &m_format_options);
		if (!m_ret) break;
		delay = sleep(m_open_try_index);
		std::cout << "open input delay: " << delay.count() << std::endl;
		std::this_thread::sleep_for(delay);
		m_open_try_index++;
	} while (m_open_try_index < m_open_try_count);
	if (m_ret) PKRT(m_ret);
	int try_index = 0;
	do {
		m_ret = avformat_find_stream_info(m_format_ctx, nullptr);
		if (m_ret >= 0) break;
		try_index++;
	} while (try_index < 2);
	if (m_ret < 0) PKRT(m_ret);
	try_index = 0;
	do {
		m_ret = av_find_best_stream(m_format_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
		if (m_ret >= 0) break;
		try_index++;
	} while (try_index < 2);
	if (m_ret < 0) PKRT(m_ret);
	m_stream_index = m_ret;
	if (m_print_av_format) av_dump_format(m_format_ctx, -1, m_video_path.c_str(), 0);
	try_index = 0;
	do {
		m_codec = avcodec_find_decoder(m_format_ctx->streams[m_stream_index]->codecpar->codec_id);
		if (m_codec) break;
		try_index++;
	} while (try_index < 2);
	if (!m_codec) PKRT(INIT_ERR_FIND_DECODER);
	try_index = 0;
	do {
		m_codec_ctx = avcodec_alloc_context3(m_codec);
		if (m_codec_ctx) break;
		try_index++;
	} while (try_index < 2);
	if (!m_codec_ctx) PKRT(INIT_ERR_ALLOC_CODEC_CONTEXT);
	m_ret = avcodec_parameters_to_context(m_codec_ctx, m_format_ctx->streams[m_stream_index]->codecpar);
	if (m_ret < 0) PKRT(m_ret);
	try_index = 0;
	do {
		m_ret = avcodec_open2(m_codec_ctx, m_codec, nullptr);
		if (!m_ret)break;
		try_index++;
	} while (try_index < 2);
	if (m_ret) PKRT(m_ret);
	try_index = 0;
	do {
		m_packet = av_packet_alloc();
		if (m_packet)break;
		try_index++;
	} while (try_index < 2);
	if (!m_packet) PKRT(INIT_ERR_ALLOC_PACKET);
	try_index = 0;
	do {
		m_frame = av_frame_alloc();
		if (m_frame)break;
		try_index++;
	} while (try_index < 2);
	try_index = 0;
	do {
		m_frame_bgr = av_frame_alloc();
		if (m_frame_bgr)break;
		try_index++;
	} while (try_index < 2);
	if (!m_frame || !m_frame_bgr) PKRT(INIT_ERR_ALLOC_FRAME);
	int one_frame_bytes = av_image_get_buffer_size(AV_PIX_FMT_BGR24, m_codec_ctx->width, m_codec_ctx->height, 1);
	if (!one_frame_bytes) PKRT(INIT_ERR_ALLOC_BGR_BUFFER);
	try_index = 0;
	do {
		m_bgr_buffer = static_cast<uint8_t*>(av_malloc(one_frame_bytes));
		if (m_bgr_buffer) break;
		try_index++;
	} while (try_index < 2);
	if (!m_bgr_buffer) PKRT(INIT_ERR_ALLOC_BGR_BUFFER);
	m_crop_width = m_codec_ctx->width;
	m_crop_height = static_cast<int>(floor(m_codec_ctx->height * (m_crop_y_rate >= 1 ? 1 : m_crop_y_rate)));
	m_crop_y = static_cast<int>(floor((m_codec_ctx->height - m_crop_height) / 2));
	m_crop_x = 0;
	PKRT(MYFS_SUCCESS);
}

int MyFFmpeg::decode() {
	if (m_is_suspend){ // 暂停 要对其恢复
		av_read_play(m_format_ctx);
		m_is_suspend = false;
	}
	int index = 0;
	m_frames.clear();
	for (;;) {
		m_ret = av_read_frame(m_format_ctx, m_packet);
		if (m_ret < 0 && m_ret != AV_EOF) PKRT(m_ret);
		if (m_packet->stream_index == m_stream_index) {
			m_ret = avcodec_send_packet(m_codec_ctx, m_packet);
			if (m_ret < 0) PKRT(m_ret);
			while (avcodec_receive_frame(m_codec_ctx, m_frame) >= 0) {
				crop();
				av_frame_unref(m_frame);
			}
			if (m_ret == OTHER_ERROR_EAGAIN || m_ret == AV_EOF) PKRT(m_ret);
			av_packet_unref(m_packet);
			if (++index >= m_packet_of_frames){
				av_read_pause(m_format_ctx);
				m_is_suspend = true;
				PKRT(MYFS_SUCCESS);	
			} 
		}
	}
}

py::list MyFFmpeg::frames() {

	py::list py_mats;
	for (auto& mat : m_frames) {
		py::array_t<unsigned char> numpy_array({ mat.rows, mat.cols, mat.channels()}, mat.data);
		py_mats.append(numpy_array);
	}
	return py_mats;
}

int MyFFmpeg::close(){
	if (m_format_ctx){
		avformat_close_input(&m_format_ctx);
		PKRT(MYFS_SUCCESS);
	}
	PKRT(MYFS_SUCCESS);
}

PYBIND11_MODULE(MyFFmpeg, m) {
	py::class_<MyFFmpeg>(m, "MyFFmpeg")
		.def(py::init<const std::string&, const double, const int, const int, const int>())
		.def("initialize", &MyFFmpeg::initialize)
		.def("decode", &MyFFmpeg::decode)
		.def("frames", &MyFFmpeg::frames, py::return_value_policy::move)
		.def("close",&MyFFmpeg::close);
}

