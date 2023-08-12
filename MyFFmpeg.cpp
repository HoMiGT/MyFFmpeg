#include "MyFFmpeg.h"

#include <cstring>
#include <random>
#include <cmath>
#include <thread>
#include <shared_mutex>

std::shared_mutex rwLock;

inline void set_options(AVDictionary* format_options,const std::string& video_size,const std::string& pixel_format){
    av_dict_set(&format_options, "video_size", video_size.c_str(), 0);
    av_dict_set(&format_options, "pixel_format", pixel_format.c_str(), 0);
    av_dict_set(&format_options, "probesize", "10485760", 0); // 设置探测大小 10MB
    av_dict_set(&format_options, "analyzeduration", "3000000", 0); // 设置分析时长 2s
    av_dict_set(&format_options, "rtsp_transport", "tcp", 0);
    av_dict_set(&format_options, "max_delay", "250", 0);
    av_dict_set(&format_options, "max_analyze_duration", "3000000", 0);  // 设置最大分析时长 2s
    av_dict_set(&format_options, "stimeout", "3000000", 0); // 设置超时时间 2s
    av_dict_set(&format_options,"timetout","3000000",0);
}

MyFFmpeg::MyFFmpeg(const std::string video_path,const std::string video_size,double crop_y_rate,int open_try_count,const std::string pixel_format)
	: m_video_path(video_path)
    , m_video_size(video_size)
	, m_crop_y_rate(crop_y_rate)
	, m_open_try_count(open_try_count)
    , m_pixel_format(pixel_format)
{
    av_log(nullptr,AV_LOG_INFO,"video_size:%s",video_size.c_str());
    set_options(m_format_options,m_video_size,m_pixel_format);
};


/// <summary>
/// 波动式随机等待
/// </summary>
/// <param name="attempt"></param>
/// <returns></returns>
std::chrono::milliseconds MyFFmpeg::sleep(int attempt)
{
	static std::array<int, 10> arr_wait = { 1,1,1,1,1,1,3,1,3,1 }; // 20
	static std::default_random_engine engine(std::random_device{}());
	static std::uniform_real_distribution<double> distribution(0.0, 1.0);
	int delay = arr_wait[attempt % 10] * 1000 + distribution(engine) * 100; // 计算等待时间
	return std::chrono::milliseconds(delay);
}

void MyFFmpeg::destruction() {
    try{
        if (m_codec_ctx) avcodec_free_context(&m_codec_ctx);
        m_codec_ctx = nullptr;

        if (m_format_ctx && !m_is_closed_input) avformat_close_input(&m_format_ctx);

        if (m_packet) av_packet_free(&m_packet);
        m_packet = nullptr;

        if (m_frame_bgr) av_frame_free(&m_frame_bgr);
        m_frame_bgr = nullptr;
        if (m_frame) av_frame_free(&m_frame);
        m_frame = nullptr;

        if (m_bgr_buffer) av_free(m_bgr_buffer);
        m_bgr_buffer = nullptr;

        if (m_format_ctx) avformat_free_context(m_format_ctx);
        m_format_ctx = nullptr;

        if (m_format_options && !m_is_free_options) av_dict_free(&m_format_options);
        m_format_options = nullptr;

        avformat_network_deinit();
    }catch (std::exception& e){
        std::cout<<"destruction error:"<< e.what() <<std::endl;
    }
}

int MyFFmpeg::initialize() noexcept {
    try{
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
            std::cout<<" a 1" << std::endl;
            m_ret = avformat_open_input(&m_format_ctx, m_video_path.c_str(), m_input_format, &m_format_options);
            std::cout<<" a 2" << std::endl;
            if (!m_ret) {
                std::cout<<" a 3" << std::endl;
                m_ret = avformat_find_stream_info(m_format_ctx, nullptr);
                std::cout<<" a 4" << std::endl;
                if(!m_ret) break;
            };

            if(m_format_ctx){
                m_format_ctx->interrupt_callback.callback = nullptr;
                m_format_ctx->interrupt_callback.opaque = nullptr;
                avformat_close_input(&m_format_ctx);
                avformat_free_context(m_format_ctx);
                m_is_closed_input = true;
            }
            std::cout<<" a 5" << std::endl;
            m_format_ctx = nullptr;
            if(m_format_options) {
                av_dict_free(&m_format_options);
                m_is_free_options = true;
            }
            m_format_options = nullptr;

            m_format_ctx = avformat_alloc_context();
            set_options(m_format_options,m_video_size,m_pixel_format);

            delay = sleep(m_open_try_index);
            std::this_thread::sleep_for(delay);
            m_open_try_index++;
        } while (m_open_try_index < m_open_try_count);
        std::cout<<" a 6" << std::endl;
        if (m_ret) PKRT(m_ret);
        m_open_flag = true;
//        int try_index = 0;
//        do {
//            std::cout<<" b 1" << std::endl;
//            m_ret = avformat_find_stream_info(m_format_ctx, nullptr);
//            std::cout<<" b 2" << std::endl;
//            if (m_ret >= 0) break;
//            try_index++;
//        } while (try_index < 2);
//        if (m_ret < 0) PKRT(m_ret);
         av_dump_format(m_format_ctx, -1, m_video_path.c_str(), 0);
//        do {
//            std::cout<<" c 1" << std::endl;
//            m_ret = av_find_best_stream(m_format_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
//            std::cout<<" c 2" << std::endl;
//            if (m_ret >= 0) break;
//            try_index++;
//        } while (try_index < 2);
//        if (m_ret < 0) PKRT(m_ret);

        for (int i = 0; i < m_format_ctx->nb_streams; i++)
            if (m_format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                m_stream_index = i;
                break;
            }
        if (m_stream_index==-1) PKRT(INIT_ERR_FIND_DECODER);

        m_codec = avcodec_find_decoder(m_format_ctx->streams[m_stream_index]->codecpar->codec_id);
        if (!m_codec) PKRT(INIT_ERR_FIND_DECODER);
        m_codec_ctx = avcodec_alloc_context3(m_codec);
        if (!m_codec_ctx) PKRT(INIT_ERR_ALLOC_CODEC_CONTEXT);
        m_ret = avcodec_parameters_to_context(m_codec_ctx, m_format_ctx->streams[m_stream_index]->codecpar);
        if (m_ret < 0) PKRT(m_ret);
        m_ret = avcodec_open2(m_codec_ctx, m_codec, nullptr);
        if (m_ret) PKRT(m_ret);

        m_frame = av_frame_alloc();
        m_frame_bgr = av_frame_alloc();
        if (!m_frame || !m_frame_bgr) PKRT(INIT_ERR_ALLOC_FRAME);

        m_packet = av_packet_alloc();
        if (!m_packet) PKRT(INIT_ERR_ALLOC_PACKET);

        int one_frame_bytes = av_image_get_buffer_size(AV_PIX_FMT_BGR24, m_codec_ctx->width, m_codec_ctx->height, 1);
        if (!one_frame_bytes) PKRT(INIT_ERR_ALLOC_BGR_BUFFER);
        m_bgr_buffer = static_cast<uint8_t*>(av_malloc(one_frame_bytes+INBUF_SIZE));
        memset(m_bgr_buffer,0,one_frame_bytes+INBUF_SIZE);
        if (!m_bgr_buffer) PKRT(INIT_ERR_ALLOC_BGR_BUFFER);

        m_crop_width = m_codec_ctx->width;
        m_crop_height = static_cast<int>(floor(m_codec_ctx->height * (m_crop_y_rate >= 1 ? 1 : m_crop_y_rate)));
        m_crop_y = static_cast<int>(floor((m_codec_ctx->height - m_crop_height) / 2));
        m_crop_x = 0;
        PKRT(MYFS_SUCCESS);
    }catch (std::exception& e){
        std::cout<<"initialize error:" << e.what() << std::endl;
        return 3999;
    }
}

int MyFFmpeg::video_info(py::array_t<int> arr){
	try{
//		py::buffer_info info = arr.request();
//		int* ptr = static_cast<int*>(info.ptr);
        auto ptr = arr.mutable_data();
		ptr[0] = m_crop_width;
		ptr[1] = m_crop_height;
	}catch(...){
		PKRT(OTHER_ERROR_EOTHER);
	}
	PKRT(MYFS_SUCCESS);
}

//int MyFFmpeg::video_info(int ptr[2]){
//    try{
//        ptr[0] = m_crop_width;
//        ptr[1] = m_crop_height;
//    }catch(...){
//        PKRT(OTHER_ERROR_EOTHER);
//    }
//    PKRT(MYFS_SUCCESS);
//}

void MyFFmpeg::video_crop(uint8_t* arr, unsigned long step,int index){
    try{
        auto video_width = m_codec_ctx->width;
        auto video_height = m_codec_ctx->height;
        auto src_format = static_cast<AVPixelFormat>(m_codec_ctx->sw_pix_fmt);
        m_sws_ctx = sws_getContext(video_width, video_height, src_format, video_width, video_height, AV_PIX_FMT_BGR24, SWS_BILINEAR, nullptr, nullptr, nullptr);
        av_image_fill_arrays(m_frame_bgr->data, m_frame_bgr->linesize, m_bgr_buffer, AV_PIX_FMT_BGR24, video_width, video_height, 1);
        sws_scale(m_sws_ctx, m_frame->data, m_frame->linesize, 0, video_height, m_frame_bgr->data, m_frame_bgr->linesize);
        cv::Mat img(cv::Size(video_width, video_height), CV_8UC3, m_bgr_buffer);
        cv::Mat crop = img(cv::Rect(m_crop_x, m_crop_y, m_crop_width, m_crop_height));
        memcpy(arr+step*index, crop.data, step);
        sws_freeContext(m_sws_ctx);
    }catch (std::exception& e){
        std::cout<<"video_crop error: " << e.what() << std::endl;
    }
}

int MyFFmpeg::video_frames(py::array_t<uint8_t> arr,int arr_len){
//int MyFFmpeg::video_frames(uint8_t* ptr,int arr_len){
    try{
        std::unique_lock<std::shared_mutex> lock(rwLock);
        if (m_is_suspend){ // 暂停 要对其恢复
            av_read_play(m_format_ctx);
            m_is_suspend = false;
        }
//        py::buffer_info info = arr.request();
//        auto ptr = static_cast<uint8_t*>(info.ptr);
        auto ptr = arr.mutable_data();
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
                    av_frame_unref(m_frame_bgr);
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
    }catch (std::exception& e){
        std::cout<<"video_frames error:" << e.what() << std::endl;
        return 0;
    }
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

//: m_video_path(video_path)
//, m_crop_y_rate(crop_y_rate)
//, m_open_try_count(open_try_count)
//, m_video_size(video_size)
//, m_pixel_format(pixel_format)

PYBIND11_MODULE(MyFFmpeg, m) {
	py::class_<MyFFmpeg>(m, "MyFFmpeg")
		.def(py::init<const std::string, const std::string, const double, const int,const std::string>(),
		        py::arg("video_path"),
                py::arg("video_size") ="540x960",
                py::arg("crop_height_rate")=0.5,
                py::arg("open_try_count") = 5,
                py::arg("pixel_format")="yuv420p",
                py::return_value_policy::reference)
		.def("initialize", &MyFFmpeg::initialize,py::return_value_policy::copy)
		.def("video_info", &MyFFmpeg::video_info,py::return_value_policy::copy)
		.def("video_frames", &MyFFmpeg::video_frames,py::return_value_policy::copy)
		.def("destruction",&MyFFmpeg::destruction,py::return_value_policy::copy);
}


//int main(){
//    MyFFmpeg f("/home/wpwl/Projects/MyFFmpeg/30_success.flv");
//    auto ret = f.initialize();
//    std::cout<<"ret:" << ret <<std::endl;
////    py::array_t<uint8_t> array({2});
////    auto ptr = array.mutable_data();
//    int ptr[2] = {0,0};
//    ret = f.video_info(ptr);
//    std::cout<<"video_info ret:"<< ret <<", width:" << ptr[0] << ", height:" << ptr[1] << std::endl;
//
////    std::vector<ssize_t> shape = {30,ptr[1],ptr[0],3};
////    py::array_t<uint8_t> frames(shape);
//    auto one_frame_size = ptr[0]*ptr[1]*3*sizeof(uint8_t);
//    uint8_t* frames = new uint8_t[30*one_frame_size];
//    int index =0 ;
//
//    while (true){
//        ret = f.video_frames(frames,30);
//        std::cout<<"ret: " << ret << std::endl;
//        cv::Mat mat;
//        for(int i=0;i<ret;i++){
//            cv::Mat img(ptr[1],ptr[0],CV_8UC3,frames+one_frame_size*i);
//            cv::imwrite(std::to_string(index)+".png",img);
//            index++;
//        }
//
//        if(ret<=0)break;
//    }
//
////    ret = f.video_frames(frames,30);
////    std::cout<<"ret: " << ret << std::endl;
////    cv::Mat mat;
////    for(int i=0;i<ret;i++){
////        cv::Mat img(ptr[1],ptr[0],CV_8UC3,frames+one_frame_size*i);
////        cv::imwrite(std::to_string(i)+".png",img);
////        index++;
////    }
//
////    if(ret<=0)break;
//
//	f.destruction();
//
//    delete[] frames;
//
////	uint8_t* free_array = array.mutable_data();
////	delete[] free_array;
////
////	uint8_t* free_frames = frames.mutable_data();
////	delete[] free_frames;
//
//
//
//    return 0;
//}