﻿#include "VideoDecoder.h"
#include <QDebug>
#include <QImage>
#include <thread>
#include <QVideoFrame>
void VideoDecoder::stopplay()
{
    isStop = true;
}
int VideoDecoder::width() {
	return m_width;
}
int VideoDecoder::height() {
		return m_height;
	}
int VideoDecoder::format() {
	return (int)QVideoFrame::Format_YUV420P;
	}
void VideoDecoder::receiveFrame() {
	
	}
void VideoDecoder::startPlay(const QString& strUrl)
{
	auto url = strUrl.toStdString();
	unsigned int    i;
	int             ret;
	int             video_st_index = -1;
	int             audio_st_index = -1;
	AVFormatContext* ifmt_ctx = NULL;
	AVPacket        pkt;
	AVStream* st = NULL;
	AVDictionary* optionsDict = NULL;
	av_dict_set(&optionsDict, "rtsp_transport", "tcp", 0);                //����tcp����	,,��������������Щrtsp���ͻῨ��
	av_dict_set(&optionsDict, "stimeout", "2000000", 0);                  //���û������stimeout
	av_dict_set(&optionsDict, "buffer_size", "10*1024*1024", 0);

	av_init_packet(&pkt);                                                       // initialize packet.
	pkt.data = NULL;
	pkt.size = 0;
	bool bStart = false;
	AVStream* pVst;
	uint8_t* buffer_rgb = NULL;
	AVCodecContext* pVideoCodecCtx = NULL;
	AVCodecParameters* pVideoCodecPar = NULL;
	AVFrame* pFrame = av_frame_alloc();
	AVFrame* pFrameRGB = av_frame_alloc();
	SwsContext* img_convert_ctx = NULL;
	AVCodec* pVideoCodec = NULL;
	uint8_t *m_buffer=nullptr;
	QVideoFrame::PixelFormat pixFormat;
	bool bFirstFrame = true;
	if ((ret = avformat_open_input(&ifmt_ctx, url.c_str(), 0, &optionsDict)) < 0) {            // Open the input file for reading.
		goto EXIT;
	}

	if ((ret = avformat_find_stream_info(ifmt_ctx, NULL)) < 0) {                // Get information on the input file (number of streams etc.).
		goto EXIT;
	}

	for (i = 0; i < ifmt_ctx->nb_streams; i++) {                                // dump information
		av_dump_format(ifmt_ctx, i, url.c_str(), 0);
	}

	for (i = 0; i < ifmt_ctx->nb_streams; i++) {                                // find video stream index
		st = ifmt_ctx->streams[i];
		switch (st->codecpar->codec_type) {
		case AVMEDIA_TYPE_AUDIO: audio_st_index = i; break;
		case AVMEDIA_TYPE_VIDEO: video_st_index = i; break;
		default: break;
		}
	}
	if (-1 == video_st_index) {
		goto EXIT;
	}
	
	while (!isStop)
	{
		do {
			ret = av_read_frame(ifmt_ctx, &pkt);                                // read frames
			if (ret < 0)
			{
				avformat_close_input(&ifmt_ctx);
				while (avformat_open_input(&ifmt_ctx, url.c_str(), 0, &optionsDict))
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(100));
				}
				ret = AVERROR(EAGAIN);
				continue;
			}
			//decode stream
			if (!bStart)
			{
				pVst = ifmt_ctx->streams[video_st_index];

				pVideoCodecPar = pVst->codecpar;

				pVideoCodecCtx = avcodec_alloc_context3(pVideoCodec);
				avcodec_parameters_to_context(pVideoCodecCtx, pVideoCodecPar);

				pVideoCodec = avcodec_find_decoder(pVideoCodecCtx->codec_id);
				if (pVideoCodec == NULL)
					return;

				if (avcodec_open2(pVideoCodecCtx, pVideoCodec, NULL) < 0)
					return;
				bStart = true;
			}

			if (pkt.stream_index == video_st_index)
			{
				int av_result = avcodec_send_packet(pVideoCodecCtx, &pkt);
				ret = avcodec_receive_frame(pVideoCodecCtx, pFrame);
				if (ret)
				{
					if (ret != AVERROR(EAGAIN))
					{
						avcodec_free_context(&pVideoCodecCtx);
						av_packet_unref(&pkt);
						bStart = false;
						ret = AVERROR(EAGAIN);
					}
					continue;
				}

				if (av_result < 0)
				{
					return;
				}
				
				if (ret == 0)
				{
					m_width = pVideoCodecCtx->width;
					m_height = pVideoCodecCtx->height;
					
					if(bFirstFrame && m_width>0)
					{
						pixFormat = QVideoFrame::Format_YUV420P;
						switch (pVideoCodecCtx->pix_fmt) {
							case AV_PIX_FMT_YUVJ420P:
								pixFormat = QVideoFrame::Format_YUV420P;
								break;
							case AV_PIX_FMT_YUVJ422P:
								pixFormat = QVideoFrame::Format_YUV422P;
								break;
							case AV_PIX_FMT_YUVJ444P:
								pixFormat = QVideoFrame::Format_YUV444;
								break;
							default:
								pixFormat = QVideoFrame::Format_YUV420P;
								break;
						}
						emit videoFrameInfo(m_width,m_height,(int)pixFormat);
						bFirstFrame = false;
					}
					int32_t bitLen=1;
					int yLen = m_width * m_height*bitLen;
					int uLen = yLen / 4;
					int allLen = yLen * 3 / 2;
					if(m_buffer==nullptr)
						m_buffer=new uint8_t[m_width*m_height*3/2];
					for (int i = 0; i < m_height; i++) {
						memcpy(m_buffer + i * m_width, pFrame->data[0] + i * pFrame->linesize[0], m_width);
					}
					for (int i = 0; i < m_height / 2; i++) {
						memcpy(m_buffer + yLen+i * m_width / 2,pFrame->data[1] + i * pFrame->linesize[1], m_width / 2);
					}
					for (int i = 0; i < m_height / 2; i++) {
						memcpy(m_buffer + yLen+ uLen+ i * m_width / 2, pFrame->data[2] + i * pFrame->linesize[2], m_width / 2);
					}
					int m_bufLen= allLen;
					QVideoFrame f(m_bufLen, QSize(pVideoCodecCtx->width, pVideoCodecCtx->height), pVideoCodecCtx->width, QVideoFrame::Format_YUV420P);
					if (f.map(QAbstractVideoBuffer::WriteOnly)) {
						memcpy(f.bits(), m_buffer,m_bufLen);
						f.setStartTime(0);
						f.unmap();
						emit newFrameAvailable(f);
					}
					
				}
			}

		} while (ret == AVERROR(EAGAIN));

		if (ret < 0) {
			goto EXIT;
		}

		if (pkt.stream_index == video_st_index) {                               // video frame
		}
		else if (pkt.stream_index == audio_st_index) {                         // audio frame
		}
		else {
		}

		av_packet_unref(&pkt);
	}

EXIT:

	if (NULL != ifmt_ctx) {
		avformat_close_input(&ifmt_ctx);
		ifmt_ctx = NULL;
	}
	delete m_buffer;
	m_buffer = nullptr;
	emit videoFrameDataFinish(strUrl);
	return;
}

//--------------------------------------------------------------------------------//
VideoDecoderController::VideoDecoderController(QObject* parent) : QObject(parent)
{
}

VideoDecoderController::~VideoDecoderController()
{
	for (auto item : m_decoders)
	{
		item.second->stopplay();
	}
}

void VideoDecoderController::startThread(const QString& serverAddress)
{
	if (m_decoders.find(serverAddress) != m_decoders.end())
	{
		return;
	}
	VideoDecoder* decoder = new VideoDecoder;
	m_decoders[serverAddress] = decoder;
	connect(decoder, &VideoDecoder::videoFrameInfo, this, &VideoDecoderController::onVideoFrameInfo);
	connect(decoder, &VideoDecoder::videoFrameDataFinish, this, &VideoDecoderController::videoFrameDataFinish);
    auto t = std::thread(&VideoDecoder::startPlay, decoder, serverAddress);
    t.detach();

}

void VideoDecoderController::stopThread()
{
	for (auto item : m_decoders)
	{
		item.second->stopplay();
	}
}

void VideoDecoderController::onVideoFrameInfo(int width,int height,int format)
{
    emit videoFrameInfo(width, height,format);
}

void VideoDecoderController::videoFrameDataFinish(QString url)
{
	m_decoders.erase(url);
}
