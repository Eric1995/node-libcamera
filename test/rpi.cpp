
// #include <cstdint>
#include <fcntl.h>
#include <iostream>
#include <linux/videodev2.h>
#include <string.h>
// #include <string>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/poll.h>
// #include <sys/stat.h>
#include <thread>
#include <unistd.h>

FILE *file = fopen("./video.h264", "w");

int width = 1920;
int height = 1080;

int main()
{
    const char device_name[] = "/dev/video11";
    int fd_ = open(device_name, O_RDWR, 0);
    if (fd_ < 0)
        throw std::runtime_error("failed to open V4L2 H264 encoder");
    v4l2_control ctrl = {};
    int ret;
    ctrl.id = V4L2_CID_MPEG_VIDEO_BITRATE;
    ctrl.value = 20 * 1000 * 1000;
    ret = ioctl(fd_, VIDIOC_S_CTRL, &ctrl);
    if (ret < 0)
    {
        printf("1%s \n", strerror(errno));
    }
    struct v4l2_ext_controls ecs1;
    struct v4l2_ext_control ec1;
    memset(&ecs1, 0, sizeof(ecs1));
    memset(&ec1, 0, sizeof(ec1));
    ec1.id = V4L2_CID_MPEG_VIDEO_BITRATE_MODE;
    ec1.value = V4L2_MPEG_VIDEO_BITRATE_MODE_CBR;
    ec1.size = 0;
    ecs1.controls = &ec1;
    ecs1.count = 1;
    ecs1.ctrl_class = V4L2_CTRL_CLASS_MPEG;
    ioctl(fd_, VIDIOC_S_EXT_CTRLS, &ecs1);

    ctrl.id = V4L2_CID_MPEG_VIDEO_H264_LEVEL;
    ctrl.value = 13;
    ret = ioctl(fd_, VIDIOC_S_CTRL, &ctrl);

    ctrl.id = V4L2_CID_MPEG_VIDEO_H264_PROFILE;
    ctrl.value = 4;
    ret = ioctl(fd_, VIDIOC_S_CTRL, &ctrl);

    if (ret < 0)
    {
        printf("2%s \n", strerror(errno));
    }
    v4l2_format fmt = {};
    fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    fmt.fmt.pix_mp.width = width;
    fmt.fmt.pix_mp.height = height;
    // We assume YUV420 here, but it would be nice if we could do something
    // like info.pixel_format.toV4L2Fourcc();
    fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_YUV420;
    fmt.fmt.pix_mp.plane_fmt[0].bytesperline = width;
    fmt.fmt.pix_mp.field = V4L2_FIELD_ANY;
    fmt.fmt.pix_mp.colorspace = V4L2_COLORSPACE_DEFAULT;
    fmt.fmt.pix_mp.num_planes = 1;
    if (ioctl(fd_, VIDIOC_S_FMT, &fmt) < 0)
        throw std::runtime_error("failed to set output format");

    fmt = {};
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    fmt.fmt.pix_mp.width = width;
    fmt.fmt.pix_mp.height = height;
    fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_H264;
    fmt.fmt.pix_mp.field = V4L2_FIELD_ANY;
    fmt.fmt.pix_mp.colorspace = V4L2_COLORSPACE_DEFAULT;
    fmt.fmt.pix_mp.num_planes = 1;
    fmt.fmt.pix_mp.plane_fmt[0].bytesperline = 0;
    fmt.fmt.pix_mp.plane_fmt[0].sizeimage = 512 << 10;
    if (ioctl(fd_, VIDIOC_S_FMT, &fmt) < 0)
        throw std::runtime_error("failed to set capture format");

    struct v4l2_streamparm parm = {};
    parm.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    parm.parm.output.timeperframe.numerator = 90000.0 / 20.0;
    parm.parm.output.timeperframe.denominator = 90000;
    if (ioctl(fd_, VIDIOC_S_PARM, &parm) < 0)
        throw std::runtime_error("failed to set streamparm");
    v4l2_requestbuffers reqbufs = {};
    reqbufs.count = 1;
    reqbufs.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    reqbufs.memory = V4L2_MEMORY_MMAP;
    if (ioctl(fd_, VIDIOC_REQBUFS, &reqbufs) < 0)
        throw std::runtime_error("request for output buffers failed");
    reqbufs = {};
    reqbufs.count = 1;
    reqbufs.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    reqbufs.memory = V4L2_MEMORY_MMAP;
    if (ioctl(fd_, VIDIOC_REQBUFS, &reqbufs) < 0)
        throw std::runtime_error("request for capture buffers failed");
    v4l2_plane planes[1];
    v4l2_buffer buffer = {};
    buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    buffer.memory = V4L2_MEMORY_MMAP;
    buffer.index = 0;
    buffer.length = 1;
    buffer.m.planes = planes;
    if (ioctl(fd_, VIDIOC_QUERYBUF, &buffer) < 0)
        throw std::runtime_error("failed to capture query buffer " + std::to_string(0));
    void *capture_map = mmap(0, buffer.m.planes[0].length, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, buffer.m.planes[0].m.mem_offset);
    if (capture_map == MAP_FAILED)
        throw std::runtime_error("failed to mmap capture buffer " + std::to_string(0));
    if (ioctl(fd_, VIDIOC_QBUF, &buffer) < 0)
        throw std::runtime_error("failed to queue capture buffer " + std::to_string(0));
    /////////////////////////////////////////////////////////////////////////------------------------
    v4l2_plane planes_out[1];
    v4l2_buffer buffer_out = {};
    buffer_out.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    buffer_out.memory = V4L2_MEMORY_MMAP;
    buffer_out.index = 0;
    buffer_out.length = 1;
    buffer_out.m.planes = planes_out;
    if (ioctl(fd_, VIDIOC_QUERYBUF, &buffer_out) < 0)
        throw std::runtime_error("failed to output query buffer " + std::to_string(0));
    void *out_map = mmap(0, buffer_out.m.planes[0].length, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, buffer_out.m.planes[0].m.mem_offset);
    if (out_map == MAP_FAILED)
        throw std::runtime_error("failed to mmap output buffer " + std::to_string(0));
    // START -------------------------------------------------------------------------------
    v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    if (ioctl(fd_, VIDIOC_STREAMON, &type) < 0)
        throw std::runtime_error("failed to start output streaming");
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    if (ioctl(fd_, VIDIOC_STREAMON, &type) < 0)
        throw std::runtime_error("failed to start capture streaming");
    // throw std::runtime_error("failed to mmap capture buffer " + std::to_string(0));

    std::thread tt([fd_, &capture_map, &buffer, &buffer_out]() {
        std::cout << "new thread" << std::endl;
        while (true)
        {
            pollfd p = {fd_, POLLIN, 0};
            // std::cout << "poll" << std::endl;
            int ret = poll(&p, 1, 200);
            if (ret == -1)
            {
                if (errno == EINTR)
                    continue;
                throw std::runtime_error("unexpected errno " + std::to_string(errno) + " from poll");
            }
            if (p.revents & POLLIN)
            {
                std::cout << "p.revents" << std::endl;
                // usleep(4 * 1000);
                struct v4l2_buffer buf;
                struct v4l2_plane out_planes;
                buf.memory = V4L2_MEMORY_MMAP;
                buf.length = 1;
                memset(&out_planes, 0, sizeof(out_planes));
                buf.m.planes = &out_planes;
                // 将output buffer出列
                buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
                std::cout << "dequeue OUTPUT~~~~" << std::endl;
                ret = ioctl(fd_, VIDIOC_DQBUF, &buffer_out);
                if (ret < 0)
                {
                    printf("12%s \n", strerror(errno));
                }
                // std::cout << "ret 1: " << ret << std::endl;
                // 将capture buffer出列
                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
                buf.memory = V4L2_MEMORY_MMAP;
                buf.length = 1;
                memset(&out_planes, 0, sizeof(out_planes));
                buf.m.planes = &out_planes;
                ret = ioctl(fd_, VIDIOC_DQBUF, &buf);
                if (ret < 0)
                {
                    printf("13%s \n", strerror(errno));
                }
                // std::cout << "ret 2: " << ret2 << std::endl;
                // 提取capture buffer里的编码数据，即H264数据
                int encoded_len = buf.m.planes[0].bytesused;
                if (encoded_len > 0)
                {

                    std::cout << "get h264 frame data, size: " << encoded_len << std::endl;
                    size_t ret = fwrite(capture_map, sizeof(uint8_t), encoded_len, file);
                    if (ret < 0)
                    {
                        printf("13%s \n", strerror(errno));
                    }
                }
                else
                {
                    std::cout << "no frame data" << std::endl;
                }
                // 将capture buffer入列
                ioctl(fd_, VIDIOC_QBUF, buffer);
                std::cout << "VIDIOC_QBUF CAPTURE" << std::endl;
            }
        }
    });
    for (int i = 0; i < 44; i++)
    {

        usleep(50 * 1000);
        std::string path = "/dev/shm/yuv/" + std::to_string(i) + ".yuv";
        std::cout << path << std::endl;
        int _fd = open(path.c_str(), O_RDWR);
        void *ptr = mmap(NULL, width * height * 1.5, PROT_READ | PROT_WRITE, MAP_SHARED, _fd, 0);
        memcpy(out_map, ptr, width * height * 1.5);
        // usleep(500 * 1000);
        // v4l2_buffer buffer_out = {};
        // buffer_out.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
        // buffer_out.memory = V4L2_MEMORY_MMAP;
        // buffer_out.index = 0;
        // buffer_out.length = 1;
        // buffer_out.m.planes = planes_out;
        std::cout << "try to queue output buffer" << std::endl;
        if (ioctl(fd_, VIDIOC_QBUF, &buffer_out) < 0)
        {
            printf("%s \n", strerror(errno));
            throw std::runtime_error("failed to queue input to codec");
        }
        else
        {
            std::cout << "VIDIOC_QBUF succeed" << std::endl;
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