#include "MyFFmpeg.h"

#include <cstring>
#include <random>
#include <cmath>

#define INBUF_SIZE 4096
// 包装结果 pack result
#define PKRT(k)  return ( StateConvert.count(static_cast<int>(k)) ? StateConvert.at(static_cast<int>(k)) : 3999 )

inline void set_options(AVDictionary*& format_options){
//    av_dict_set(&format_options, "pixel_format", "yuvj420p", 0);
    av_dict_set(&format_options, "probesize", "4096", 0); // 设置探测大小 10MB
    av_dict_set(&format_options,"analyzeduration","1000000",0); // 设置探测时长 1s
//    av_dict_set(&format_options, "rtsp_transport", "tcp", 0);
    av_dict_set(&format_options, "max_delay", "300", 0);
    av_dict_set(&format_options,"stimeout","2000000",0); // 设置超时时间 2s
    av_dict_set(&format_options,"timeout","2000000",0); // 设置超时时间 2s
}

MyFFmpeg::MyFFmpeg(const std::string video_path,int timeout,double crop_y_rate,int open_try_count)
	: m_video_path(video_path)
    , m_timeout(timeout)
	, m_crop_y_rate(crop_y_rate)
	, m_open_try_count(open_try_count)
{}

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
        m_is_destruction = true;
    }catch (std::exception& e){
        av_log(nullptr,AV_LOG_ERROR,"destruction error:%s\n",e.what());
    }
}

int MyFFmpeg::initialize() noexcept {
    try{
        avformat_network_init();
        m_input_format = av_find_input_format("flv");
        if (!m_input_format) PKRT(INIT_ERR_ALLOC_INPUT_FORMAT);

        auto open_try_index = 0;
        for(;;){
//            av_log(nullptr,AV_LOG_INFO,"open_try_index:%d\n",m_open_try_index);
            if (++open_try_index >= m_open_try_count){
                PKRT(AV_STREAM_NOT_FOUND);
            }
            m_is_closed_input = false;
            m_is_free_options = false;
            set_options(m_format_options);

            m_format_ctx = avformat_alloc_context();
            m_open_start_time = static_cast<double>(cv::getTickCount());
            m_format_ctx->interrupt_callback.callback = this->my_interrupt_callback;
            m_format_ctx->interrupt_callback.opaque = this;
            m_ret = avformat_open_input(&m_format_ctx, m_video_path.c_str(), m_input_format, &m_format_options);
            if (!m_ret) {
                m_stream_index = -1;
                m_ret = avformat_find_stream_info(m_format_ctx, nullptr);
                if (m_ret>=0){
                    m_stream_index = av_find_best_stream(m_format_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
                    if (m_stream_index >= 0) break;
                }

            };
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
        }
        if (m_ret) PKRT(m_ret);
        if (m_stream_index == -1) PKRT(INIT_ERR_FIND_DECODER);
        m_open_flag = true;
        
        // av_dump_format(m_format_ctx, -1, m_video_path.c_str(), 0);

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
        av_log(nullptr,AV_LOG_ERROR,"initialize error:%s\n",e.what());
        return 3999;
    }
}

int MyFFmpeg::info(py::array_t<int> arr){
	try{
        auto ptr = arr.mutable_data();
		ptr[0] = m_crop_width;
		ptr[1] = m_crop_height;
	}catch(...){
		PKRT(OTHER_ERROR_EOTHER);
	}
	PKRT(MYFS_SUCCESS);
}

int MyFFmpeg::info_c(int ptr[2]){
    try{
        ptr[0] = m_crop_width;
        ptr[1] = m_crop_height;
    }catch(...){
        PKRT(OTHER_ERROR_EOTHER);
    }
    PKRT(MYFS_SUCCESS);
}

void MyFFmpeg::crop(uint8_t* arr, unsigned long step,int index){
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
        m_sws_ctx = nullptr;
    }catch (std::exception& e){
        if(m_sws_ctx) sws_freeContext(m_sws_ctx);
        m_sws_ctx= nullptr;
        av_log(nullptr,AV_LOG_ERROR,"video_crop error: %s\n",e.what());
    }
}

int MyFFmpeg::packet(py::array_t<uint8_t> arr,int arr_len){
    try{
        if (m_is_suspend){ // 暂停 要对其恢复
            av_read_play(m_format_ctx);
            m_is_suspend = false;
        }
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
                    crop(ptr,step,fact_arr_index);
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
        av_log(nullptr,AV_LOG_ERROR,"video_frames error:%s\n",e.what());
        return 0;
    }
}

int MyFFmpeg::packet_c(uint8_t* ptr,int arr_len){
    try{
        if (m_is_suspend){ // 暂停 要对其恢复
            av_read_play(m_format_ctx);
            m_is_suspend = false;
        }
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
                    crop(ptr,step,fact_arr_index);
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
        av_log(nullptr,AV_LOG_ERROR,"video_frames error:%s\n",e.what());
        return 0;
    }
}


int MyFFmpeg::my_interrupt_callback(void *opaque){
	MyFFmpeg *p = (MyFFmpeg *)opaque;
	if(p){
	    auto duration = ((double)cv::getTickCount() - p->m_open_start_time)*1000.0 / cv::getTickFrequency();
		if(!p->m_open_flag && duration > (double)p->m_timeout){
//            av_log(nullptr,AV_LOG_INFO,"open_index:%d, duration:%f, timeout:%f\n",p->m_open_try_index,duration,p->m_timeout);
			return 1;
		} 
	}
	return 0;
}

MyFFmpeg::~MyFFmpeg() {
    if(m_is_destruction){
        return;
    }
    destruction();
}

PYBIND11_MODULE(MyFFmpeg, m) {
	py::class_<MyFFmpeg>(m, "MyFFmpeg")
		.def(py::init<const std::string,const int, const double, const int>(),
		        py::arg("video_path"),
                py::arg("timeout") = 2000,
                py::arg("crop_height_rate")=0.5,
                py::arg("open_try_count") = 5,
                py::return_value_policy::reference)
		.def("initialize", &MyFFmpeg::initialize,py::return_value_policy::copy)
		.def("info", &MyFFmpeg::info,py::return_value_policy::copy)
		.def("packet", &MyFFmpeg::packet,py::return_value_policy::copy)
		.def("destruction",&MyFFmpeg::destruction,py::return_value_policy::copy);
}


int main(){
    MyFFmpeg f("/home/wpwl/Projects/MyFFmpeg/3.flv",2000,0.5,5);
    auto ret = f.initialize();
    std::cout<<"ret:" << ret <<std::endl;

    auto img_width = 540;
    auto img_height = static_cast<int>(960*0.5);

    auto one_frame_size = img_width*img_height*3*sizeof(uint8_t);

    int count = 30;
    uint8_t* frames = new uint8_t[count*one_frame_size];
//    py::buffer_info buf_info(
//            array,                               /* 指向数据的指针 */
//            sizeof(uint8_t),                      /* 单个元素的大小 */
//            py::format_descriptor<uint8_t>::format(), /* 数据的格式描述符 */
//            1,                                    /* 数组的维数 */
//            { 30*one_frame_size },                /* 数组的形状 */
//            { sizeof(uint8_t) }                   /* 在每个维度中步进的字节数 */
//    );
//    py::array_t<uint8_t> frames(buf_info);

    int index =0 ;

    while (true){
        ret = f.packet_c(frames,count);
        std::cout<<"ret: " << ret << std::endl;
        cv::Mat mat;
//        auto buffer = frames.mutable_data();
//        uint8_t *ptr = static_cast<uint8_t*>(buffer);
        for(int i=0;i<ret;i++){
            cv::Mat img(img_height,img_width,CV_8UC3,frames+one_frame_size*i);
            cv::imwrite(std::to_string(index)+".png",img);
            index++;
        }

        if(ret<=0)break;
    }

//	f.destruction();
    delete[] frames;

    return 0;
}
