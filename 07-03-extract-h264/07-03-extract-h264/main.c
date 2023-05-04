#include <stdio.h>
#include <libavutil/log.h>
#include <libavformat/avio.h>
#include <libavformat/avformat.h>


static char err_buff[128]={0};
static char* av_get_err(int errnum){
    av_strerror(errnum,err_buff,128);
    return err_buff;
}

/*
AvCodecContext->extradata[]中为nalu长度
*   codec_extradata:
*   1, 64, 0, 1f, ff, e1, [0, 18], 67, 64, 0, 1f, ac, c8, 60, 78, 1b, 7e,
*   78, 40, 0, 0, fa, 40, 0, 3a, 98, 3, c6, c, 66, 80,
*   1, [0, 5],68, e9, 78, bc, b0, 0,
*/

//ffmpeg -i 2018.mp4 -codec copy -bsf:h264_mp4toannexb -f h264 tmp.h264
//ffmpeg 从mp4上提取H264的nalu h
int main(int argc, char **argv)
{

    AVFormatContext *ifmt_ctx=NULL;
    int videoIndex=-1;
    AVPacket *pkt=NULL;
    int ret=-1;
    int file_end=0;//判斷是都讀取結束

    if(argc<3){
        printf("error could not alllcate context\n.");
         // avformat_close_input(&ifmt_ctx);
        return -1;

    }


    FILE *outfp=fopen(argv[2],"wb");
    printf("in:%s out:%s",argv[1],argv[2]);
// 分解解复用器的内存，使用avformat_close_input 释放

    ifmt_ctx=avformat_alloc_context();


    if(!ifmt_ctx){
        printf("error could not allocate context.\n");
        return -1;

    }
    ret=avformat_open_input(&ifmt_ctx,argv[1],NULL,NULL);
    if(ret!=0){
        printf("error avformat_open_input:%s\n",av_get_err(ret));
        return -1;

    }

    ret=avformat_find_stream_info(ifmt_ctx,NULL);
    if(ret<0){
        printf("error avformat_find_stream_info :%s\n",av_get_err(ret));
          avformat_close_input(&ifmt_ctx);
        return -1;
    }
    //查找出哪個m码流是video/audio/subtitles

    videoIndex=-1;
    videoIndex=av_find_best_stream(ifmt_ctx,AVMEDIA_TYPE_VIDEO,-1,-1,NULL,0);
    if(videoIndex==-1){
        printf("Did not find a video stream.\n");
        avformat_close_input(&ifmt_ctx);
        return -1;
    }

    //分配数据包
    pkt=av_packet_alloc();

    //
    av_init_packet(pkt);

    //获取相应的比特流过滤器
    //FLV/MP4/MKV中，h264需要h264_mp4toannexb 处理，添加SPS和PPS等信息
    //FLV 封装时，可以把多个NALU放在VIDEO TAG中，结构中4B NALU长度+NALU1+4B

    const AVBitStreamFilter *bsFilter=av_bsf_get_by_name("h264_mp4toannexb");

    //初始化过滤器上下文
    AVBSFContext *bsf_ctx=NULL;

    av_bsf_alloc(bsFilter,&bsf_ctx);
    // 添加解码器属性
    avcodec_parameters_copy(bsf_ctx->par_in,ifmt_ctx->streams[videoIndex]->codecpar);
    av_bsf_init(bsf_ctx);
    file_end=0;
    while(0==file_end){
        if(ret=av_read_frame(ifmt_ctx,pkt)<0){
            file_end=1;
            printf("read file end:ret:%d\n,",ret);

        }
        if(ret==0&&pkt->stream_index==videoIndex)
        {
#if 0
            int input_size=pkt->size;
            int out_pkt_count=0;
            if(av_bsf_send_packet(bsf_ctx,pkt)!=0){
                av_packet_unref(pkt);//你不用了酒吧资源释放掉
                continue;
            }
            av_packet_unref(pkt);//释放资源
            while(av_bsf_receive_packet(bsf_ctx,pkt)==0){
                out_pkt_count++;
                //printf
                size_t size=fwrite(pkt->data,1,pkt->size,outfp);
                if(size!=pkt->size){
                    printf("fwrite failed->write:%u\n",size,pkt->size);
                }
                av_packet_unref(pkt);
            }

            if(out_pkt_count>=2){
                printf("cur_pkt(size:%d) only get 1 out pkt:%u",input_size,out_pkt_count);
            }



#else
            size_t size=fwrite(pkt->data,1,pkt->size,outfp);
            if(size!=pkt->size){
                printf("fwrite failed->write:%u,pkt->size:%u\n",size,pkt->size);
            }
            av_packet_unref(pkt);
#endif

        }
        else{
            if(ret==0){
                av_packet_unref(pkt);// 释放内存
            }
        }


    }



    if(outfp)
        fclose(outfp);
    if(bsf_ctx){

        av_bsf_free(&bsf_ctx);
    }

    if(pkt)

    {
        av_packet_free(&pkt);
    }

    if(ifmt_ctx){
        avformat_close_input(&ifmt_ctx);
    }

    printf("finish\n");





    return 0;
}
