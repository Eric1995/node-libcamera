
// #include <cstdint>
#include <fcntl.h>
#include <iostream>
#include <linux/videodev2.h>
#include <string.h>
// #include <string>
#include <sys/ioctl.h>
#include <sys/mman.h>
// #include <sys/stat.h>
#include <thread>
#include <unistd.h>

struct buffer
{
    void *start;
    int length;
    struct v4l2_buffer inner;
    struct v4l2_plane plane;
};

void map(int fd, uint32_t type, struct buffer *buffer, bool dma)
{
    struct v4l2_buffer *inner = &buffer->inner;

    memset(inner, 0, sizeof(*inner));
    inner->type = type;
    if (dma)
    {
        inner->memory = V4L2_MEMORY_DMABUF;
    }
    else
    {
        inner->memory = V4L2_MEMORY_MMAP;
    }

    inner->index = 0;
    inner->length = 1;
    inner->m.planes = &buffer->plane;
    ioctl(fd, VIDIOC_QUERYBUF, inner);
    // std::cout << "map plane length after query buffer: " << ((&(buffer->plane))[0]).length << std::endl;
    buffer->length = inner->m.planes[0].length;
    buffer->start = mmap(NULL, buffer->length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, inner->m.planes[0].m.mem_offset);
    if (buffer->start == (void *)-1)
    {
        std::cout << "mmap type: " << type << std::endl;
        printf("mmap error : %d-%s. \r\n", errno, strerror(errno));
    }
}

struct buffer output;
struct buffer capture;

int width = 1920;
int height = 1080;

int main()
{
    int fd = open("/dev/video11", O_RDWR);
    FILE *file = fopen("./video.h264", "w");
    // 设置码率
    v4l2_control ctrl = {};
    int ret;
    ctrl.id = V4L2_CID_MPEG_VIDEO_BITRATE;
    ctrl.value = 25 * 1000 * 1000;
    ret = ioctl(fd, VIDIOC_S_CTRL, &ctrl);
    if (ret < 0)
    {
        printf("1%s \n", strerror(errno));
    }
    ctrl.id = V4L2_CID_MPEG_VIDEO_H264_LEVEL;
    ctrl.value = 13;
    ret = ioctl(fd, VIDIOC_S_CTRL, &ctrl);
    if (ret < 0)
    {
        printf("2%s \n", strerror(errno));
    }

    struct v4l2_format fmt;
    fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    ret = ioctl(fd, VIDIOC_G_FMT, &fmt);
    if (ret < 0)
    {
        printf("3%s \n", strerror(errno));
    }
    fmt.fmt.pix_mp.width = width;
    fmt.fmt.pix_mp.height = height;
    fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_YUV420;
    fmt.fmt.pix_mp.field = V4L2_FIELD_ANY;
    fmt.fmt.pix_mp.num_planes = 1;
    fmt.fmt.pix_mp.plane_fmt[0].bytesperline = width;
    ret = ioctl(fd, VIDIOC_S_FMT, &fmt);
    if (ret < 0)
    {
        printf("4%s \n", strerror(errno));
    }

    fmt = {};
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    ret = ioctl(fd, VIDIOC_G_FMT, &fmt);
    if (ret < 0)
    {
        printf("5%s \n", strerror(errno));
    }
    fmt.fmt.pix_mp.width = width;
    fmt.fmt.pix_mp.height = height;
    fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_H264;
    fmt.fmt.pix_mp.field = V4L2_FIELD_ANY;
    fmt.fmt.pix_mp.colorspace = V4L2_COLORSPACE_DEFAULT;
    fmt.fmt.pix_mp.num_planes = 1;
    fmt.fmt.pix_mp.plane_fmt[0].bytesperline = 0;
    fmt.fmt.pix_mp.plane_fmt[0].sizeimage = 1024 << 10;
    ret = ioctl(fd, VIDIOC_S_FMT, &fmt);
    if (ret < 0)
    {
        printf("6%s \n", strerror(errno));
    }
    // struct v4l2_streamparm params;
    // memset(&params, 0, sizeof(params));
    // params.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    // params.parm.output.timeperframe.numerator = 1;
    // params.parm.output.timeperframe.denominator = 30;
    // ioctl(fd, VIDIOC_S_PARM, &params);

    struct v4l2_requestbuffers buf;
    buf.count = 1;
    // struct buffer output;
    buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    buf.memory = V4L2_MEMORY_MMAP;
    ret = ioctl(fd, VIDIOC_REQBUFS, &buf);
    if (ret < 0)
    {
        printf("7%s \n", strerror(errno));
    }
    map(fd, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE, &output, false);

    // struct buffer capture;
    // buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    // buf.memory = V4L2_MEMORY_MMAP;
    // ret = ioctl(fd, VIDIOC_REQBUFS, &buf);
    // if (ret < 0)
    // {
    //     printf("8%s \n", strerror(errno));
    // }
    // map(fd, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, &capture, false);
    // std::cout << "capture inner: " << capture.inner.type << std::endl;
    // ret = ioctl(fd, VIDIOC_QBUF, capture.inner);
    // if (ret < 0)
    // {
    //     printf("9%s \n", strerror(errno));
    // }

    v4l2_plane planes[1];
    v4l2_buffer buffer = {};
    buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    buffer.memory = V4L2_MEMORY_MMAP;
    buffer.index = 0;
    buffer.length = 1;
    buffer.m.planes = planes;
    ret = ioctl(fd, VIDIOC_QUERYBUF, &buffer);
    if (ret < 0)
    {
        printf("9%s \n", strerror(errno));
    }

    // buffers_[i].mem = mmap(0, buffer.m.planes[0].length, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, buffer.m.planes[0].m.mem_offset);

    int type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    ret = ioctl(fd, VIDIOC_STREAMON, &type);
    if (ret < 0)
    {
        printf("10%s \n", strerror(errno));
    }

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    ret = ioctl(fd, VIDIOC_STREAMON, &type);
    if (ret < 0)
    {
        printf("11%s \n", strerror(errno));
    }
    std::cout << "ret: " << ret << std::endl;
    // std::thread tt([fd, file]() {
    //     std::cout << "new thread" << std::endl;
    //     while (true)
    //     {
    //         usleep(4 * 1000);
    //         struct v4l2_buffer buf;
    //         struct v4l2_plane out_planes;
    //         buf.memory = V4L2_MEMORY_MMAP;
    //         buf.length = 1;
    //         memset(&out_planes, 0, sizeof(out_planes));
    //         buf.m.planes = &out_planes;
    //         // 将output buffer出列
    //         buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    //         std::cout << "dequeue OUTPUT~~~~" << std::endl;
    //         int ret = ioctl(fd, VIDIOC_DQBUF, &buf);
    //         if (ret < 0)
    //         {
    //             printf("12%s \n", strerror(errno));
    //         }
    //         std::cout << "ret 1: " << ret << std::endl;
    //         // 将capture buffer出列
    //         buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    //         buf.memory = V4L2_MEMORY_MMAP;
    //         buf.length = 1;
    //         memset(&out_planes, 0, sizeof(out_planes));
    //         buf.m.planes = &out_planes;
    //         ret = ioctl(fd, VIDIOC_DQBUF, &buf);
    //         if (ret < 0)
    //         {
    //             printf("13%s \n", strerror(errno));
    //         }
    //         // std::cout << "ret 2: " << ret2 << std::endl;
    //         // 提取capture buffer里的编码数据，即H264数据
    //         int encoded_len = buf.m.planes[0].bytesused;
    //         if (encoded_len > 0)
    //         {

    //             std::cout << "get h264 frame data, size: " << encoded_len << std::endl;
    //             std::cout << "sizeof uint8: " << sizeof(uint8_t) << std::endl;
    //             size_t ret = fwrite(capture.start, sizeof(uint8_t), encoded_len, file);
    //             if (ret < 0)
    //             {
    //                 printf("13%s \n", strerror(errno));
    //             }
    //         }
    //         else
    //         {
    //             std::cout << "no frame data" << std::endl;
    //         }
    //         // 将capture buffer入列
    //         ioctl(fd, VIDIOC_QBUF, capture.inner);
    //     }
    // });

    for (int i = 0; i < 25; i++)
    {

        usleep(30 * 1000);
        std::string path = "/dev/shm/yuv/" + std::to_string(i) + ".yuv";
        std::cout << path << std::endl;
        int _fd = open(path.c_str(), O_RDWR);
        void *ptr = mmap(NULL, width * height * 1.5, PROT_READ | PROT_WRITE, MAP_SHARED, _fd, 0);
        memcpy(output.start, ptr, width * height * 1.5);
        ret = ioctl(fd, VIDIOC_QBUF, output.inner);
        if (ret < 0)
        {
            // printf("14%s \n", strerror(errno));
        }
        // std::cout << "_fd: " << _fd << std::endl;
        // v4l2_buffer input_buf = {};
        // v4l2_plane planes[VIDEO_MAX_PLANES] = {};
        // input_buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
        // input_buf.index = 0;
        // input_buf.field = V4L2_FIELD_NONE;
        // input_buf.memory = V4L2_MEMORY_MMAP;
        // input_buf.length = 1;
        // input_buf.m.planes = planes;
        // input_buf.m.planes[0].m.fd = _fd;
        // input_buf.m.planes[0].bytesused = 640 * 480 * 1.5;
        // input_buf.m.planes[0].length = 640 * 480 * 1.5;
        // ioctl(fd, VIDIOC_QBUF, &input_buf);
    }
    // tt.join();
    return 0;
}