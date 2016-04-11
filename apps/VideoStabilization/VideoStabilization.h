#ifndef VIDEOSTABILIZATION_H
#define VIDEOSTABILIZATION_H

#include <tr1/memory>
#include <opencv2/core/core.hpp>

#define SPtr std::tr1::shared_ptr

class VideoStabilizationImpl;


class VideoStabilization
{
public:
    enum {NoStabilization=0,XYThetaKalman=1};

    VideoStabilization(int approach=XYThetaKalman);

    bool    feed(cv::Mat& input);
    cv::Mat fetch();
    bool    feed(cv::Mat& input,cv::Mat& output);// implement both feed and fetch

private:
    SPtr<VideoStabilizationImpl> impl;
};

#endif // VIDEOSTABILIZATION_H
