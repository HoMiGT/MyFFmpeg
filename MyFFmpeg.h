#pragma once

#include <string>
#include <map>
#include <iostream>
#include <chrono>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
}
#include <opencv2/opencv.hpp>
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>
#include "MyFFmpegState.h"

namespace py = pybind11;

class MyFFmpeg {
public:
    MyFFmpeg(const std::string video_path,int width,int height,int timeout,double crop_y_rate,int open_try_count);
	~MyFFmpeg();

	int initialize() noexcept; // 初始化
    int info(py::array_t<int> arr); // 获取视频的信息
    int info_c(int arr[]); // 获取视频的信息
	int packet(py::array_t<uint8_t> arr,int arr_len); // 获取帧
    int packet_c(uint8_t* arr,int arr_len); // 获取帧
	void destruction(); // 清理内存

	static int my_interrupt_callback(void *opaque); // 定时检测打断

private:
    int m_width;
    int m_height;
	double m_crop_y_rate{ 0.5 };  // 默认高截取的比率
	int m_open_try_count{ 0 };  // 打开尝试次数
	int m_open_try_index{ 0 };  // 打开尝试索引
	int m_timeout{0};  // 超时断开连接
	int m_crop_x{ 0 };  // 截取图片的x坐标
	int m_crop_y{ 0 };  // 截图图片的y坐标
	int m_crop_width{ 0 };  // 截取图片的宽度
	int m_crop_height{ 0 };  // 截取图片的高度
	int m_ret{ MYFS_DEFAULT };  // 默认结果状态
	int m_stream_index{ -1 };  // 默认视频缩影
	bool m_is_suspend {false}; // 是否暂停视频读取
	bool m_is_closed_input{false}; // 是否关闭了 input
	bool m_is_free_options{false}; // 是否关闭了 options
	bool m_open_flag {false}; // 打开标识 用来判断回调函数是否计算时间超时
	double m_open_start_time{ 0 }; // 打开视频的开始时间 
	const std::string m_video_path;  // 视频路径
	AVDictionary* m_format_options{ nullptr };  // 选项 av_dict_free
	AVFormatContext* m_format_ctx{ nullptr };  // 格式上下文 avformat_close_input  avformat_free_context
	const AVInputFormat* m_input_format{ nullptr };  // 输入格式  静态无需手动删除 
	const AVCodec* m_codec{ nullptr }; // 编解码器  静态无需手动删除
	AVCodecContext* m_codec_ctx{ nullptr };  // 编解码器上下文 avcodec_free_context
	AVPacket* m_packet{ nullptr };  // 数据包 av_packet_free
	AVFrame* m_frame{ nullptr };  // 帧 av_frame_free
	AVFrame* m_frame_bgr{ nullptr };  // bgr帧 av_frame_free
    SwsContext* m_sws_ctx {nullptr};
	uint8_t* m_bgr_buffer{nullptr}; // bgr缓冲区 av_free
	bool m_is_destruction{ false }; // 是否已经销毁

	void crop(uint8_t* arr,unsigned long step,int index); // 视频截取

};


