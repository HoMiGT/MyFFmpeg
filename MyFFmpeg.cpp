#include "MyFFmpeg.h"

#include <cstring>
#include <random>
#include <cmath>
#include <thread>
#include <shared_mutex>

std::shared_mutex rwLock;

MyFFmpeg::MyFFmpeg(const std::string& video_path,
	double crop_y_rate,
	int open_try_count,
	int packet_of_frame
)
	: m_video_path(video_path)
	, m_crop_y_rate(crop_y_rate)
	, m_open_try_count(open_try_count)
	, m_packet_of_frames(packet_of_frame)
{
	av_dict_set(&m_format_options, "probesize", "10485760", 0); // 设置探测大小 10MB
	av_dict_set(&m_format_options, "analyzeduration", "6000000", 0); // 设置分析时长 2s
	av_dict_set(&m_format_options, "rtsp_transport", "tcp", 0);
	av_dict_set(&m_format_options, "max_delay", "250", 0);
	av_dict_set(&m_format_options, "max_analyze_duration", "6000000", 0);  // 设置最大分析时长 2s
	av_dict_set(&m_format_options, "stimeout", "6000000", 0); // 设置超时时间 2s
	av_dict_set(&m_format_options,"timetout","6000000",0);
};


/// <summary>
/// 波动式随机等待
/// </summary>
/// <param name="attempt"></param>
/// <returns></returns>
std::chrono::milliseconds MyFFmpeg::sleep(int attempt)
{
	static std::array<int, 10> arr_wait = { 3,2,1,2,3,1,3,1,3,1 }; // 20
	static std::default_random_engine engine(std::random_device{}());
	static std::uniform_real_distribution<double> distribution(0.0, 1.0);
	int delay = arr_wait[attempt % 10] * 100 + distribution(engine) * 100; // 计算等待时间
	return std::chrono::milliseconds(delay);
}

void MyFFmpeg::destruction() {
	avformat_network_deinit();
	std::cout<<"clean_up-1" << std::endl;
	if (m_bgr_buffer) av_free(m_bgr_buffer);
	m_bgr_buffer = nullptr;
	std::cout<<"clean_up-2" << std::endl;
	if (m_frame_bgr) av_frame_free(&m_frame_bgr);
	m_frame_bgr = nullptr;
	std::cout<<"clean_up-3" << std::endl;
	if (m_frame) av_frame_free(&m_frame);
	m_frame = nullptr;
	std::cout<<"clean_up-4" << std::endl;
	if (m_packet) av_packet_free(&m_packet);
	m_packet = nullptr;
	std::cout<<"clean_up-5" << std::endl;
	if (m_codec_ctx) avcodec_free_context(&m_codec_ctx);
	m_codec_ctx = nullptr;
	std::cout<<"clean_up-6" << std::endl;
	if (m_format_ctx && !m_is_closed_input) avformat_close_input(&m_format_ctx);
	std::cout<<"clean_up-7" << std::endl;
	if (m_format_ctx) avformat_free_context(m_format_ctx);
	m_format_ctx = nullptr;
	std::cout<<"clean_up-8" << std::endl;
	if (m_format_options && !m_is_free_options) av_dict_free(&m_format_options);
	m_format_options = nullptr;
	std::cout<<"clean_up-9" << std::endl;
}

int MyFFmpeg::initialize() noexcept {
	avformat_network_init();
	m_format_ctx = avformat_alloc_context();
	if (!m_format_ctx) PKRT(INIT_ERR_ALLOC_FORMAT);
	m_input_format = av_find_input_format("flv");
	if (!m_input_format) PKRT(INIT_ERR_ALLOC_INPUT_FORMAT);
	std::chrono::milliseconds delay;
	m_open_start_time = static_cast<double>(cv::getTickCount()); 
	do {
		m_is_closed_input = false;
		m_is_free_options = false;
		m_format_ctx->interrupt_callback.callback = this->my_interrupt_callback;
		m_format_ctx->interrupt_callback.opaque = this;
		m_ret = avformat_open_input(&m_format_ctx, m_video_path.c_str(), m_input_format, &m_format_options);
		if (!m_ret) break;
		if(m_format_ctx){
			m_format_ctx->interrupt_callback.callback = nullptr;
			m_format_ctx->interrupt_callback.opaque = nullptr;
			avformat_close_input(&m_format_ctx);
			avformat_free_context(m_format_ctx);
			m_is_closed_input = true;
		}
		m_format_ctx = nullptr;
		if(m_format_options) {
			av_dict_free(&m_format_options);
			m_is_free_options = true;
		} 
		m_format_options = nullptr;

		m_format_ctx = avformat_alloc_context();
		av_dict_set(&m_format_options, "probesize", "10485760", 0); // 设置探测大小 10MB
		av_dict_set(&m_format_options, "analyzeduration", "3000000", 0); // 设置分析时长 3s
		av_dict_set(&m_format_options, "rtsp_transport", "tcp", 0);
		av_dict_set(&m_format_options, "max_delay", "250", 0);
		av_dict_set(&m_format_options, "max_analyze_duration", "3000000", 0);  // 设置最大分析时长 3s
		av_dict_set(&m_format_options, "stimeout", "3000000", 0); // 设置超时时间 3s
		av_dict_set(&m_format_options,"timetout","3000000",0);

		delay = sleep(m_open_try_index);
		std::this_thread::sleep_for(delay);
		m_open_try_index++;
	} while (m_open_try_index < m_open_try_count);
	if (m_ret) PKRT(m_ret); 
	m_open_flag = true;
	int try_index = 0;
	do {
		m_ret = avformat_find_stream_info(m_format_ctx, nullptr);
		if (m_ret >= 0) break;
		try_index++;
	} while (try_index < 2);
	if (m_ret < 0) PKRT(m_ret);
	try_index = 0;
	// av_dump_format(m_format_ctx, -1, m_video_path.c_str(), 0);
	do {
		m_ret = av_find_best_stream(m_format_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
		if (m_ret >= 0) break;
		try_index++;
	} while (try_index < 2);
	if (m_ret < 0) PKRT(m_ret);
	m_stream_index = m_ret;
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

	
	m_packet = av_packet_alloc();
	if (!m_packet) PKRT(INIT_ERR_ALLOC_PACKET);
	m_frame = av_frame_alloc();
	m_frame_bgr = av_frame_alloc();
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

int MyFFmpeg::video_info(py::array_t<int> arr){
	try{
		py::buffer_info info = arr.request();
		int* ptr = static_cast<int*>(info.ptr);
		ptr[0] = m_crop_width;
		ptr[1] = m_crop_height;
	}catch(...){
		PKRT(OTHER_ERROR_EOTHER);
	}
	PKRT(MYFS_SUCCESS);
}

void MyFFmpeg::video_crop(uint8_t* arr, unsigned long step,int index){
	auto video_width = m_codec_ctx->width;
	auto video_height = m_codec_ctx->height;
	auto src_format = static_cast<AVPixelFormat>(m_codec_ctx->sw_pix_fmt);
	auto m_sws_context = sws_getContext(video_width, video_height, src_format, video_width, video_height, AV_PIX_FMT_BGR24, SWS_BILINEAR, nullptr, nullptr, nullptr);
	cv::Mat img(cv::Size(video_width, video_height), CV_8UC3, m_bgr_buffer);	
	av_image_fill_arrays(m_frame_bgr->data, m_frame_bgr->linesize, img.data, AV_PIX_FMT_BGR24, video_width, video_height, 1);
	sws_scale(m_sws_context, m_frame->data, m_frame->linesize, 0, video_height, m_frame_bgr->data, m_frame_bgr->linesize);
	cv::Mat crop = img(cv::Rect(m_crop_x, m_crop_y, m_crop_width, m_crop_height));
	memcpy(arr+step*index, crop.data, step ); 
}

int MyFFmpeg::video_frames(py::array_t<uint8_t> arr,int arr_len){
	std::unique_lock<std::shared_mutex> lock(rwLock);
	if (m_is_suspend){ // 暂停 要对其恢复
		av_read_play(m_format_ctx);
		m_is_suspend = false;
	}
	py::buffer_info info = arr.request();
	auto ptr = static_cast<uint8_t*>(info.ptr);
	auto step = m_crop_width * m_crop_height * 3 * sizeof(uint8_t);
	int fact_arr_index = 0;
	for(fact_arr_index =0; fact_arr_index <arr_len;){
		m_ret = av_read_frame(m_format_ctx, m_packet);
		if (m_ret < 0) return fact_arr_index;
		if (m_packet->stream_index == m_stream_index) {
			m_ret = avcodec_send_packet(m_codec_ctx, m_packet);
			if (m_ret < 0) return fact_arr_index;
			while (avcodec_receive_frame(m_codec_ctx, m_frame) >= 0) {
				if(fact_arr_index>=arr_len) return fact_arr_index;
				video_crop(ptr,step,fact_arr_index);
				fact_arr_index++;
                av_frame_unref(m_frame);
			}
		}
        av_packet_unref(m_packet);
	}
	if(!m_is_suspend){
		av_read_pause(m_format_ctx);
		m_is_suspend = true;
	}
	return fact_arr_index;
}


int MyFFmpeg::my_interrupt_callback(void *opaque){
	MyFFmpeg *p = (MyFFmpeg *)opaque;
	if(p){
		if(!p->m_open_flag && (((double)cv::getTickCount() - p->m_open_start_time) / cv::getTickFrequency()) > 2){
			return 1;
		} 
	}
	return 0;
}


//PYBIND11_MODULE(MyFFmpeg, m) {
//	py::class_<MyFFmpeg>(m, "MyFFmpeg")
//		.def(py::init<const std::string&, const double, const int, const int>())
//		.def("initialize", &MyFFmpeg::initialize)
//		.def("video_info", &MyFFmpeg::video_info)
//		.def("video_frames", &MyFFmpeg::video_frames)
//		.def("destruction",&MyFFmpeg::destruction);
//}


int main(){
    MyFFmpeg f("./30_success.flv");
    auto ret = f.initialize();
    std::cout<<"ret:" << ret <<std::endl;
    py::array_t<uint8_t> array({2});
    auto ptr = array.mutable_data();
    ptr[0] = 0;
    ptr[1] = 0;
    ret = f.video_info(array);
    std::cout<<"video_info ret:"<< ret <<", width:" << ptr[0] << ", height:" << ptr[1] << std::endl;

    std::vector<ssize_t> shape = {30,ptr[1],ptr[0],3};
    py::array_t<uint8_t> frames(shape);
    ret = f.video_frames(frames,30);
    std::cout<<"ret: " << ret << std::endl;


    return 0;
}