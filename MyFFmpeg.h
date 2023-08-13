#pragma once

#include <string>
#include <map>
#include <iostream>
#include <chrono>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/error.h>
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

namespace py = pybind11;

#define MYERRCODE(e)  ((-e))
#define INBUF_SIZE 4096

// 包装结果 pack result
#define PKRT(k)  return ( StateConvert.count(static_cast<int>(k)) ? StateConvert.at(static_cast<int>(k)) : 3999 )

/// ffmpeg version 5.1.2
enum MyFFmpegState {
	MYFS_DEFAULT = -1000,  //  不匹配时的 默认状态

	MYFS_SUCCESS = 0,  // 成功

	INIT_ERR_ALLOC_FORMAT = MYERRCODE(900), // 初始化时 分配 AVFormatContext 失败
	INIT_ERR_ALLOC_INPUT_FORMAT = MYERRCODE(901), // 初始化时 分配 AVInputFormat 失败
	INIT_ERR_FIND_DECODER = MYERRCODE(902), // 初始化时 查找解码器失败
	INIT_ERR_ALLOC_CODEC_CONTEXT = MYERRCODE(903), // 初始化时 分配 AVCodecContext 失败
	INIT_ERR_ALLOC_PACKET = MYERRCODE(904), // 初始化时 分配 AVPacket 失败
	INIT_ERR_ALLOC_FRAME = MYERRCODE(905), // 初始化时 分配 AVFrame 失败
	INIT_ERR_ALLOC_BGR_BUFFER = MYERRCODE(906), // 初始化时 分配 BGRBuffer 失败

	HTTP_BAD_REQUEST = AVERROR_HTTP_BAD_REQUEST, // 400,  -808465656 请求无效
	HTTP_UNAUTHORIZED = AVERROR_HTTP_UNAUTHORIZED, // 401, -825242872 未授权
	HTTP_FORBIDDEN = AVERROR_HTTP_FORBIDDEN, // 403, -858797304 禁止访问
	HTTP_NOT_FOUND = AVERROR_HTTP_NOT_FOUND, // 404, -875574520 未找到
	HTTP_OTHER_4XX = AVERROR_HTTP_OTHER_4XX, // 4XX, -1482175736 其他错误
	HTTP_SERVER_ERROR = AVERROR_HTTP_SERVER_ERROR, // 5XX, -1482175992 服务器错误

	AV_BSF_NOT_FOUND = AVERROR_BSF_NOT_FOUND, // -1179861752 没有找到对应的bitstream filter
	AV_BUG = AVERROR_BUG, // -558323010 内部错误，也可以理解为bug
	AV_BUFFER_TOO_SMALL = AVERROR_BUFFER_TOO_SMALL, // -1397118274 缓冲区太小
	AV_DECODER_NOT_FOUND = AVERROR_DECODER_NOT_FOUND, // -1128613112 没有找到对应的解码器
	AV_DEMUXER_NOT_FOUND = AVERROR_DEMUXER_NOT_FOUND, // -1296385272 没有找到对应的解封装器
	AV_ENCODER_NOT_FOUND = AVERROR_ENCODER_NOT_FOUND, // -1129203192 没有找到对应的编码器
	AV_EOF = AVERROR_EOF, // -541478725 已经到达文件的结尾
	AV_EXIT = AVERROR_EXIT, // -1414092869 立即退出，不需要重启
	AV_EXTERNAL = AVERROR_EXTERNAL, // -542398533 外部错误
	AV_FILTER_NOT_FOUND = AVERROR_FILTER_NOT_FOUND, // -1279870712 没有找到对应的filter
	AV_INVALIDDATA = AVERROR_INVALIDDATA, // -1094995529 无效的数据
	AV_MUXER_NOT_FOUND = AVERROR_MUXER_NOT_FOUND, // -1481985528 没有找到对应的封装器
	AV_OPTION_NOT_FOUND = AVERROR_OPTION_NOT_FOUND, // -1414549496 没有找到对应的选项
	AV_PATCHWELCOME = AVERROR_PATCHWELCOME, // -1163346256 欢迎提交补丁 
	AV_PROTOCOL_NOT_FOUND = AVERROR_PROTOCOL_NOT_FOUND, // -1330794744 没有找到对应的协议
	AV_STREAM_NOT_FOUND = AVERROR_STREAM_NOT_FOUND, // -1381258232 没有找到对应的流
	AV_BUG2 = AVERROR_BUG2, // -541545794 内部错误，也可以理解为bug
	AV_UNKNOWN = AVERROR_UNKNOWN, // -1313558101 未知错误
	AV_EXPERIMENTAL = AVERROR_EXPERIMENTAL, // -733130664 请求的功能被标记为实验性质，如果你真的想使用它，可以设置strict_std_compliance标志
	AV_INPUT_CHANGED = AVERROR_INPUT_CHANGED, // -1668179713 输入数据发生改变，需要重新配置
	AV_OUTPUT_CHANGED = AVERROR_OUTPUT_CHANGED, // -1668179714 输出数据发生改变，需要重新配置

    OTHER_ERROR_EINVAL2 = MYERRCODE(2), // -2 无效参数
	OTHER_ERROR_ESRCH = MYERRCODE(ESRCH), // -3 线程不存在
	OTHER_ERROR_EINTR = MYERRCODE(EINTR), // -4 被信号中断
	OTHER_ERROR_EIO = MYERRCODE(EIO), // -5 输入 / 输出错误
	OTHER_ERROR_ENXIO = MYERRCODE(ENXIO), // -6 没有这样的设备或地址
	OTHER_ERROR_E2BIG = MYERRCODE(E2BIG), // -7 参数列表太长
	OTHER_ERROR_ENOEXEC = MYERRCODE(ENOEXEC), // -8 执行格式错误
	OTHER_ERROR_EBADF = MYERRCODE(EBADF), // -9 文件描述符无效
	OTHER_ERROR_ECHILD = MYERRCODE(ECHILD), // -10 没有子进程
	OTHER_ERROR_EAGAIN = MYERRCODE(EAGAIN), // -11 资源暂时不可用
	OTHER_ERROR_ENOMEM = MYERRCODE(ENOMEM), // -12 没有足够的内存
	OTHER_ERROR_EACCES = MYERRCODE(EACCES), // -13 权限被拒绝
	OTHER_ERROR_EFAULT = MYERRCODE(EFAULT), // -14 错误的地址
	OTHER_ERROR_EBUSY = MYERRCODE(EBUSY), // -16 设备或资源忙
	OTHER_ERROR_EEXIST = MYERRCODE(EEXIST), // -17 文件已存在
	OTHER_ERROR_EXDEV = MYERRCODE(EXDEV), // -18 跨设备链接
	OTHER_ERROR_ENODEV = MYERRCODE(ENODEV), // -19 没有这样的设备
	OTHER_ERROR_ENOTDIR = MYERRCODE(ENOTDIR), // -20 不是目录
	OTHER_ERROR_EISDIR = MYERRCODE(EISDIR), // -21 是目录
	OTHER_ERROR_ENFILE = MYERRCODE(ENFILE), // -23 系统打开文件的数量已达到最大值
	OTHER_ERROR_EMFILE = MYERRCODE(EMFILE), // -24 打开文件的数量已达到最大值
	OTHER_ERROR_ENOTTY = MYERRCODE(ENOTTY), // -25 不是一个TTY
	OTHER_ERROR_EFBIG = MYERRCODE(EFBIG), // -27 文件太大
	OTHER_ERROR_ENOSPC = MYERRCODE(ENOSPC), // -28 没有足够的空间
	OTHER_ERROR_ESPIPE = MYERRCODE(ESPIPE), // -29 无效的寻址
	OTHER_ERROR_EROFS = MYERRCODE(EROFS), // -30 只读文件系统
	OTHER_ERROR_EMLINK = MYERRCODE(EMLINK), // -31 连接太多
	OTHER_ERROR_EPIPE = MYERRCODE(EPIPE), // -32 管道错误
	OTHER_ERROR_EDOM = MYERRCODE(EDOM), // -33 数学参数超出范围
	OTHER_ERROR_ERANGE = MYERRCODE(ERANGE), // -34 数值超出范围
	OTHER_ERROR_EDEADLK = MYERRCODE(EDEADLK), // -36 资源死锁避免
	OTHER_ERROR_ENAMETOOLONG = MYERRCODE(ENAMETOOLONG), // -38 文件名太长
	OTHER_ERROR_ENOLCK = MYERRCODE(ENOLCK), // -39 没有可用的锁
	OTHER_ERROR_ENOSYS = MYERRCODE(ENOSYS), // -40 功能不支持
	OTHER_ERROR_ENOTEMPTY = MYERRCODE(ENOTEMPTY), // -41 目录不为空
	OTHER_ERROR_EILSEQ = MYERRCODE(EILSEQ), // -42 无效的字节序列
	OTHER_ERROR_STRUNCATE = MYERRCODE(80), // -80 字符串截断
	OTHER_ERROR_EADDRINUSE = MYERRCODE(EADDRINUSE), // -100 地址已经在使用中
	OTHER_ERROR_EADDRNOTAVAIL = MYERRCODE(EADDRNOTAVAIL), // -101 地址不可用
	OTHER_ERROR_EAFNOSUPPORT = MYERRCODE(EAFNOSUPPORT), // -102 地址族不支持
	OTHER_ERROR_EALREADY = MYERRCODE(EALREADY), // -103 连接已经在进行中
	OTHER_ERROR_EBADMSG = MYERRCODE(EBADMSG), // -104 错误的消息
	OTHER_ERROR_ECANCELED = MYERRCODE(ECANCELED), // -105 操作被取消
	OTHER_ERROR_ECONNABORTED = MYERRCODE(ECONNABORTED), // -106 连接被中止
	OTHER_ERROR_ECONNREFUSED = MYERRCODE(ECONNREFUSED), // -107 连接被拒绝
	OTHER_ERROR_ECONNRESET = MYERRCODE(ECONNRESET), // -108 连接被重置
	OTHER_ERROR_EDESTADDRREQ = MYERRCODE(EDESTADDRREQ), // -109 目标地址需要
	OTHER_ERROR_EHOSTUNREACH = MYERRCODE(EHOSTUNREACH), // -110 主机不可达
	OTHER_ERROR_EIDRM = MYERRCODE(EIDRM), // -111 标识被删除
	OTHER_ERROR_EINPROGRESS = MYERRCODE(EINPROGRESS), // -112 操作正在进行中
	OTHER_ERROR_EISCONN = MYERRCODE(EISCONN), // -113 连接已经建立
	OTHER_ERROR_ELOOP = MYERRCODE(ELOOP), // -114 太多的符号链接
	OTHER_ERROR_EMSGSIZE = MYERRCODE(EMSGSIZE), // -115 消息太长
	OTHER_ERROR_ENETDOWN = MYERRCODE(ENETDOWN), // -116 网络关闭
	OTHER_ERROR_ENETRESET = MYERRCODE(ENETRESET), // -117 网络重置
	OTHER_ERROR_ENETUNREACH = MYERRCODE(ENETUNREACH), // -118 网络不可达
	OTHER_ERROR_ENOBUFS = MYERRCODE(ENOBUFS), // -119 没有缓冲区空间
	OTHER_ERROR_ENODATA = MYERRCODE(ENODATA), // -120 没有数据可用
	OTHER_ERROR_ENOLINK = MYERRCODE(ENOLINK), // -121 链接已经断开
	OTHER_ERROR_ENOMSG = MYERRCODE(ENOMSG), // -122 没有消息
	OTHER_ERROR_ENOPROTOOPT = MYERRCODE(ENOPROTOOPT), // -123 协议不可用
	OTHER_ERROR_ENOSR = MYERRCODE(ENOSR), // -124 没有数据
	OTHER_ERROR_ENOSTR = MYERRCODE(ENOSTR), // -125 没有数据
	OTHER_ERROR_ENOTCONN = MYERRCODE(ENOTCONN), // -126 没有连接
	OTHER_ERROR_ENOTRECOVERABLE = MYERRCODE(ENOTRECOVERABLE), // -127 状态不可恢复
	OTHER_ERROR_ENOTSOCK = MYERRCODE(ENOTSOCK), // -128 不是一个套接字
	OTHER_ERROR_ENOTSUP = MYERRCODE(ENOTSUP), // -129 操作不支持
	OTHER_ERROR_EOPNOTSUPP = MYERRCODE(EOPNOTSUPP), // -130 操作不支持
	OTHER_ERROR_EOTHER = MYERRCODE(131), // -131 其他错误
	OTHER_ERROR_EOVERFLOW = MYERRCODE(EOVERFLOW), // -132 值太大
	OTHER_ERROR_EOWNERDEAD = MYERRCODE(EOWNERDEAD), // -133 拥有者死亡
	OTHER_ERROR_EPROTO = MYERRCODE(EPROTO), // -134 协议错误
	OTHER_ERROR_EPROTONOSUPPORT = MYERRCODE(EPROTONOSUPPORT), // -135 协议不支持
	OTHER_ERROR_EPROTOTYPE = MYERRCODE(EPROTOTYPE), // -136 协议类型错误
	OTHER_ERROR_ETIME = MYERRCODE(ETIME), // -137 计时器超时
	OTHER_ERROR_ETIMEDOUT = MYERRCODE(ETIMEDOUT), // -138 连接超时
	OTHER_ERROR_ETXTBSY = MYERRCODE(ETXTBSY), // -139 文本忙
	OTHER_ERROR_EWOULDBLOCK = MYERRCODE(EWOULDBLOCK), // -140 操作将阻塞
	OTHER_ERROR_CONTINUE = MYERRCODE(1001), // -1001 继续读帧
	OTHER_ERROR_EINVAL = MYERRCODE(EINVAL), // -22 无效的参数
};



static const std::map<int, int> StateConvert = {
	{static_cast<int>(MYFS_SUCCESS),3100}, // 0  成功
	{static_cast<int>(MYFS_DEFAULT), 3101}, //  -1000  不匹配时的 默认状态
	{static_cast<int>(INIT_ERR_ALLOC_FORMAT),3102}, // -900  初始化时 分配 AVFormatContext 失败
	{static_cast<int>(INIT_ERR_ALLOC_INPUT_FORMAT),3103}, // -901 初始化时 分配 AVInputFormat 失败
	{static_cast<int>(INIT_ERR_FIND_DECODER),3104}, // -902 初始化时 查找解码器失败
	{static_cast<int>(INIT_ERR_ALLOC_CODEC_CONTEXT),3105}, // -903 初始化时 分配 AVCodecContext 失败
	{static_cast<int>(INIT_ERR_ALLOC_PACKET),3106}, // -904 初始化时 分配 AVPacket 失败
	{static_cast<int>(INIT_ERR_ALLOC_FRAME),3107}, // -905 初始化时 分配 AVFrame 失败
	{static_cast<int>(INIT_ERR_ALLOC_BGR_BUFFER),3108}, // -906 初始化时 分配 BGRBuffer 失败
	{static_cast<int>(HTTP_BAD_REQUEST),3109}, // 400,  -808465656 请求无效
	{static_cast<int>(HTTP_UNAUTHORIZED),3110}, // 401, -825242872 未授权
	{static_cast<int>(HTTP_FORBIDDEN),3111}, // 403, -858797304 禁止访问
	{static_cast<int>(HTTP_NOT_FOUND),3112}, // 404, -875574520 未找到
	{static_cast<int>(HTTP_OTHER_4XX),3113}, // 4XX, -1482175736 其他错误
	{static_cast<int>(HTTP_SERVER_ERROR),3114}, // 5XX, -1482175992 服务器错误
	{static_cast<int>(AV_BSF_NOT_FOUND),3115}, // -1179861752 没有找到对应的bitstream filter
	{static_cast<int>(AV_BUG),3116}, // -558323010 内部错误，也可以理解为bug
	{static_cast<int>(AV_BUFFER_TOO_SMALL),3117}, // -1397118274 缓冲区太小
	{static_cast<int>(AV_DECODER_NOT_FOUND),3118}, // -1128613112 没有找到对应的解码器
	{static_cast<int>(AV_DEMUXER_NOT_FOUND),3119}, // -1296385272 没有找到对应的解封装器
	{static_cast<int>(AV_ENCODER_NOT_FOUND),3120}, // -1129203192 没有找到对应的编码器
	{static_cast<int>(AV_EOF),3121}, // -541478725 已经到达文件的结尾
	{static_cast<int>(AV_EXIT),3122}, // -1414092869 立即退出，不需要重启
	{static_cast<int>(AV_EXTERNAL),3123}, // -542398533 外部错误
	{static_cast<int>(AV_FILTER_NOT_FOUND),3124}, // -1279870712 没有找到对应的filter
	{static_cast<int>(AV_INVALIDDATA),3125}, // -1094995529 无效的数据
	{static_cast<int>(AV_MUXER_NOT_FOUND),3126}, // -1279628576 没有找到对应的封装器
	{static_cast<int>(AV_OPTION_NOT_FOUND),3127}, // -1414549496 没有找到对应的选项
	{static_cast<int>(AV_PATCHWELCOME),3128}, // -558323011 欢迎提交补丁
	{static_cast<int>(AV_PROTOCOL_NOT_FOUND),3129}, // -1279482416 没有找到对应的协议
	{static_cast<int>(AV_STREAM_NOT_FOUND),3130}, // -1381258232 没有找到对应的流
	{static_cast<int>(AV_BUG2),3131}, // -541545794 内部错误，也可以理解为bug
	{static_cast<int>(AV_UNKNOWN),3132}, // -1313558101 未知错误
	{static_cast<int>(AV_EXPERIMENTAL),3133}, // -1313558100 实验性质的
	{static_cast<int>(AV_INPUT_CHANGED),3134}, // -1381258231 输入改变
	{static_cast<int>(AV_OUTPUT_CHANGED),3135}, // -1381258230 输出改变
	{static_cast<int>(OTHER_ERROR_ESRCH),3136}, // -3 线程不存在
	{static_cast<int>(OTHER_ERROR_EINTR),3137}, // -4 被信号中断
	{static_cast<int>(OTHER_ERROR_EIO),3138}, // -5 输入 / 输出错误
	{static_cast<int>(OTHER_ERROR_ENXIO),3139}, // -6 没有这样的设备或地址
	{static_cast<int>(OTHER_ERROR_E2BIG),3140}, // -7 参数列表太长
	{static_cast<int>(OTHER_ERROR_ENOEXEC),3141}, // -8 执行格式错误
	{static_cast<int>(OTHER_ERROR_EBADF),3142}, // -9 文件描述符无效
	{static_cast<int>(OTHER_ERROR_ECHILD),3143}, // -10 没有子进程
	{static_cast<int>(OTHER_ERROR_EAGAIN),3144}, // -11 资源暂时不可用
	{static_cast<int>(OTHER_ERROR_ENOMEM),3145}, // -12 内存不足
	{static_cast<int>(OTHER_ERROR_EACCES),3146}, // -13 权限不足
	{static_cast<int>(OTHER_ERROR_EFAULT),3147}, // -14 地址错误
	{static_cast<int>(OTHER_ERROR_EBUSY),3148}, // -16 设备或资源忙
	{static_cast<int>(OTHER_ERROR_EEXIST),3149}, // -17 文件已存在
	{static_cast<int>(OTHER_ERROR_EXDEV),3150}, // -18 跨设备链接
	{static_cast<int>(OTHER_ERROR_ENODEV),3151}, // -19 没有这样的设备
	{static_cast<int>(OTHER_ERROR_ENOTDIR),3152}, // -20 不是目录
	{static_cast<int>(OTHER_ERROR_EISDIR),3153}, // -21 是目录
	{static_cast<int>(OTHER_ERROR_ENFILE),3154}, // -23 系统打开文件的数量已达到最大值
	{static_cast<int>(OTHER_ERROR_EMFILE),3155}, // -24 打开文件的数量已达到最大值
	{static_cast<int>(OTHER_ERROR_ENOTTY),3156}, // -25 不是一个适当的IO设备
	{static_cast<int>(OTHER_ERROR_ETXTBSY),3157}, // -26 文本文件忙
	{static_cast<int>(OTHER_ERROR_EFBIG),3158}, // -27 文件太大
	{static_cast<int>(OTHER_ERROR_ENOSPC),3159}, // -28 没有空间
	{static_cast<int>(OTHER_ERROR_ESPIPE),3160}, // -29 无效的seek
	{static_cast<int>(OTHER_ERROR_EROFS),3161}, // -30 只读文件系统
	{static_cast<int>(OTHER_ERROR_EMLINK),3162}, // -31 太多的链接
	{static_cast<int>(OTHER_ERROR_EPIPE),3163}, // -32 管道错误
	{static_cast<int>(OTHER_ERROR_EDOM),3164}, // -33 数学参数超出范围
	{static_cast<int>(OTHER_ERROR_ERANGE),3165}, // -34 结果太大
	{static_cast<int>(OTHER_ERROR_EDEADLK),3166}, // -36 资源死锁避免
	{static_cast<int>(OTHER_ERROR_ENAMETOOLONG),3167}, // -38 文件名太长
	{static_cast<int>(OTHER_ERROR_ENOLCK),3168}, // -39 没有可用的锁
	{static_cast<int>(OTHER_ERROR_ENOSYS),3169}, // -40 功能不支持
	{static_cast<int>(OTHER_ERROR_ENOTEMPTY),3170}, // -41 目录不为空
	{static_cast<int>(OTHER_ERROR_EILSEQ),3171}, // -42 无效的字节序列
	{static_cast<int>(OTHER_ERROR_STRUNCATE),3172}, // -80 字符串截断
	{static_cast<int>(OTHER_ERROR_EADDRINUSE),3173}, // -100 地址已经在使用中
	{static_cast<int>(OTHER_ERROR_EADDRNOTAVAIL),3174}, // -101 地址不可用
	{static_cast<int>(OTHER_ERROR_EAFNOSUPPORT),3175}, // -102 地址族不支持
	{static_cast<int>(OTHER_ERROR_EALREADY),3176}, // -103 连接已经在进行中
	{static_cast<int>(OTHER_ERROR_EBADMSG),3177}, // -104 错误的消息
	{static_cast<int>(OTHER_ERROR_ECANCELED),3178}, // -105 操作被取消
	{static_cast<int>(OTHER_ERROR_ECONNABORTED),3179}, // -106 连接被终止
	{static_cast<int>(OTHER_ERROR_ECONNREFUSED),3180}, // -107 连接被拒绝
	{static_cast<int>(OTHER_ERROR_ECONNRESET),3181}, // -108 连接被重置
	{static_cast<int>(OTHER_ERROR_EDESTADDRREQ),3182}, // -109 目标地址是必须的
	{static_cast<int>(OTHER_ERROR_EHOSTUNREACH),3183}, // -110 主机不可达
	{static_cast<int>(OTHER_ERROR_EIDRM),3184}, // -111 标识被删除
	{static_cast<int>(OTHER_ERROR_EINPROGRESS),3185}, // -112 操作正在进行中
	{static_cast<int>(OTHER_ERROR_EISCONN),3186}, // -113 连接已经建立
	{static_cast<int>(OTHER_ERROR_ELOOP),3187}, // -114 太多的符号链接
	{static_cast<int>(OTHER_ERROR_EMSGSIZE),3188}, // -115 消息太长
	{static_cast<int>(OTHER_ERROR_ENETDOWN),3189}, // -116 网络关闭
	{static_cast<int>(OTHER_ERROR_ENETRESET),3190}, // -117 网络重置
	{static_cast<int>(OTHER_ERROR_ENETUNREACH),3191}, // -118 网络不可达
	{static_cast<int>(OTHER_ERROR_ENOBUFS),3192}, // -119 没有可用的缓冲区
	{static_cast<int>(OTHER_ERROR_ENODATA),3193}, // -120 没有数据可用
	{static_cast<int>(OTHER_ERROR_ENOLINK),3194}, // -121 链接已经断开
	{static_cast<int>(OTHER_ERROR_ENOMSG),3195}, // -122 没有消息
	{static_cast<int>(OTHER_ERROR_ENOPROTOOPT),3196}, // -123 协议不可用
	{static_cast<int>(OTHER_ERROR_ENOSR),3197}, // -124 没有流资源
	{static_cast<int>(OTHER_ERROR_ENOSTR),3198}, // -125 设备不是一个流
	{static_cast<int>(OTHER_ERROR_ENOTCONN),3199}, // -126 没有连接
	{ static_cast<int>(OTHER_ERROR_ENOTRECOVERABLE),3200}, // -127 状态不可恢复
	{ static_cast<int>(OTHER_ERROR_ENOTSOCK),3201}, // -128 不是一个套接字
	{ static_cast<int>(OTHER_ERROR_ENOTSUP),3202}, // -129 功能不支持
	{ static_cast<int>(OTHER_ERROR_EOPNOTSUPP),3203}, // -130 操作不支持
	{ static_cast<int>(OTHER_ERROR_EOTHER),3204}, // -131 其他错误
	{ static_cast<int>(OTHER_ERROR_EOVERFLOW),3205}, // -132 值太大
	{ static_cast<int>(OTHER_ERROR_EOWNERDEAD),3206}, // -133 拥有者死亡
	{ static_cast<int>(OTHER_ERROR_EPROTO),3207}, // -134 协议错误
	{ static_cast<int>(OTHER_ERROR_EPROTONOSUPPORT),3208}, // -135 协议不支持
	{ static_cast<int>(OTHER_ERROR_EPROTOTYPE),3209}, // -136 协议类型错误
	{ static_cast<int>(OTHER_ERROR_ETIME),3210}, // -137 计时器超时
	{ static_cast<int>(OTHER_ERROR_ETIMEDOUT),3211}, // -138 连接超时
	{ static_cast<int>(OTHER_ERROR_ETXTBSY),3212}, // -139 文本忙
	{ static_cast<int>(OTHER_ERROR_EWOULDBLOCK),3213}, // -140 操作将阻塞
	{static_cast<int>(OTHER_ERROR_CONTINUE),3214}, // -1001 继续读帧
	{static_cast<int>(OTHER_ERROR_EINVAL),3215}, // -22 无效参数
    {static_cast<int>(OTHER_ERROR_EINVAL2),3215} //
};

class MyFFmpeg {
public:
	MyFFmpeg(const std::string video_path,const std::string video_size,int timeout,double crop_y_rate,int open_try_count,const std::string pixel_format);
	~MyFFmpeg() =default;

	int initialize() noexcept; // 初始化
	int video_info(py::array_t<int> arr); // 获取视频的信息
	int video_frames(py::array_t<uint8_t> arr,int arr_len); // 获取帧

//	int video_info(int arr[2]); // 获取视频的信息
//	int video_frames(uint8_t* arr,int arr_len); // 获取帧

	void destruction(); // 清理内存
	static int my_interrupt_callback(void *opaque); // 定时检测打断

private:
	double m_crop_y_rate{ 0.5 };  // 默认高截取的比率
	int m_open_try_count{ 0 };  // 打开尝试次数
	int m_open_try_index{ 0 };  // 打开尝试索引
    std::string m_video_size; // 视频大小
	int m_timeout{0};  // 超时断开连接 
    std::string m_pixel_format; // 视频格式
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

	
	void video_crop(uint8_t* arr,unsigned long step,int index); // 视频截取
	std::chrono::milliseconds sleep(int index);

};


