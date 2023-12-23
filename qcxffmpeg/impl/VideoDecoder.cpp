#include "VideoDecoder.h"
#include <QDebug>
#include <QImage>
#include <thread>

void VideoDecoder::stopplay()
{
    isStop = true;
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
					int bytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, pVideoCodecCtx->width, pVideoCodecCtx->height, 1);
					buffer_rgb = (uint8_t*)av_malloc(bytes);
					av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize, buffer_rgb, AV_PIX_FMT_RGB24, pVideoCodecCtx->width, pVideoCodecCtx->height, 1);
					AVPixelFormat pixFormat;
					switch (pVideoCodecCtx->pix_fmt) {
					case AV_PIX_FMT_YUVJ420P:
						pixFormat = AV_PIX_FMT_YUV420P;
						break;
					case AV_PIX_FMT_YUVJ422P:
						pixFormat = AV_PIX_FMT_YUV422P;
						break;
					case AV_PIX_FMT_YUVJ444P:
						pixFormat = AV_PIX_FMT_YUV444P;
						break;
					case AV_PIX_FMT_YUVJ440P:
						pixFormat = AV_PIX_FMT_YUV440P;
						break;
					default:
						pixFormat = pVideoCodecCtx->pix_fmt;
						break;
					}
					img_convert_ctx = sws_getContext(pVideoCodecCtx->width, pVideoCodecCtx->height, pixFormat,
						pVideoCodecCtx->width, pVideoCodecCtx->height, AV_PIX_FMT_RGB24, SWS_FAST_BILINEAR, NULL, NULL, NULL);
					if (img_convert_ctx == NULL)
					{
						return;
					}
					sws_scale(img_convert_ctx, pFrame->data, pFrame->linesize, 0, pVideoCodecCtx->height, pFrameRGB->data, pFrameRGB->linesize);

					QImage img(buffer_rgb, pVideoCodecCtx->width, pVideoCodecCtx->height, QImage::Format_RGB888, [](void* data) {
						av_free(data);
						}, buffer_rgb);
					emit videoFrameDataReady(strUrl, img);

					sws_freeContext(img_convert_ctx);
					//av_free(buffer_rgb);
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

	connect(decoder, &VideoDecoder::videoFrameDataReady, this, &VideoDecoderController::onVideoFrameDataReady);
	connect(decoder, &VideoDecoder::videoFrameDataFinish, this, &VideoDecoderController::videoFrameDataFinish);
    auto t = std::thread(&VideoDecoder::startPlay, decoder, serverAddress);
    t.detach();
	m_decoders[serverAddress] = decoder;
}

void VideoDecoderController::stopThread()
{
	for (auto item : m_decoders)
	{
		item.second->stopplay();
	}
}

void VideoDecoderController::onVideoFrameDataReady(QString url, QImage data)
{
    emit videoFrameDataReady(url, data);
}

void VideoDecoderController::videoFrameDataFinish(QString url)
{
	m_decoders.erase(url);
}
