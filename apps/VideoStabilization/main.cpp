#include <iostream>
#include <opencv2/highgui/highgui.hpp>
#include <base/Svar/Svar.h>
#include <base/time/Global_Timer.h>

#include "VideoStabilization.h"


using namespace std;
using namespace cv;

int main(int argc,char** argv)
{
    svar.ParseMain(argc,argv);
    VideoCapture video(svar.GetString("VideoFile",""));
    if(!video.isOpened())
    {
        cerr<<"Can't open video!\n";return -1;
    }
    VideoStabilization stable;

    cv::Mat output;
    while(true)
    {
        cv::Mat img;
        video>>img;
        if(img.empty()) break;

        pi::timer.enter("VideoStabilization::feed");
        bool bOK=stable.feed(img);
        pi::timer.leave("VideoStabilization::feed");


        pi::timer.enter("VideoStabilization::fetch");
        output=stable.fetch();
        pi::timer.leave("VideoStabilization::fetch");

        cv::imshow("img",img);
        if(!output.empty())
            cv::imshow("output",output);

        uchar key=cv::waitKey(30);
        if(key==27) break;
    }
    return 0;
}
