#include <deque>
#include <opencv2/imgproc/imgproc.hpp>

#include "VideoStabilization.h"


class VideoStabilizationImpl
{
public:
    virtual bool    feed(cv::Mat& input){return false;}
    virtual cv::Mat fetch(){return cv::Mat();}
};


class StabilizationXYThetaKalman : public VideoStabilizationImpl
{
    struct StabilizationFrame
    {
        cv::Mat img,imgGray,imgWarped;
        cv::Mat H;
    };

public:
    virtual bool feed(cv::Mat& input,cv::Mat& output)
    {
        // This function compute the transformation H by compare current frame against last
        if(input.empty()) return false;
        StabilizationFrame frame;
        frame.img=input;
        if(frame.img.channels()==3) cv::cvtColor(frame.img,frame.imgGray,CV_BGR2GRAY);

    }

    virtual cv::Mat fetch()
    {
        // This function actually do the kalman and warp
        return cv::Mat();
    }

    std::deque<StabilizationFrame> frames;
};

VideoStabilization::VideoStabilization(int approach)
    :impl(new VideoStabilizationImpl)
{
    if(approach==XYThetaKalman) impl=SPtr<VideoStabilizationImpl>(new StabilizationXYThetaKalman());
}

bool VideoStabilization::feed(cv::Mat& input,cv::Mat& output)
{
    bool ret=impl->feed(input);
    output=impl->fetch();
    return ret;
}

bool    VideoStabilization::feed(cv::Mat& input)
{
    return impl->feed(input);
}

cv::Mat VideoStabilization::fetch()
{
    return impl->fetch();
}
