#include <iostream>
#include <unistd.h>
#include <base/base.h>
#include <base/system/file_path/file_path.h>
#include <base/debug/debug_config.h>


#include "TrackedImage.h"
#include "InternetTransfer.h"
#include "gps_utils.h"

//#define HAS_OPENCV

#ifdef HAS_OPENCV
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#endif

using namespace std;
using namespace pi;

int TestReadWrite()
{
    // Load first
    string file2read=svar.GetString("TestReadWrite.File2Read","Data/TestReadWrite/00009.png");
    string extname=pi::path_extname(file2read);
    TrackedImage trackedImage;
    cout<<"ExtName="<<extname<<endl;
    if(extname==".png")
    {
        //Read from png
#ifdef HAS_OPENCV
        cv::Mat img=cv::imread(file2read);
        if(img.empty())
        {
            MSG_ERROR("Failed to load from file %s",file2read.c_str());
            return -1;
        }
        cv::imshow("img",img);
        cv::waitKey(-1);
        trackedImage=TrackedImage(img.cols,img.rows,img.channels());
        memcpy(trackedImage.img_data,img.data,img.cols*img.rows*img.channels());
#else
        MSG_ERROR("This need OpenCV support!\n");
        return -1;
#endif
    }
    else
    {
        //Read from raw data
        if(trackedImage.loadFromFile(file2read))
        {
#ifdef HAS_OPENCV
            cv::Mat img(trackedImage.img_h,trackedImage.img_w,CV_8UC3,trackedImage.img_data);

            if(img.empty())
            {
                MSG_ERROR("Failed to load from file %s",file2read.c_str());
                return -1;
            }
            cv::imshow("img",img);
            cv::waitKey(-1);
#else
            MSG_INFO("Loaded success!\n");
#endif
        }
        else
        {
            MSG_ERROR("Fail to load from file %s",file2read.c_str());
            return -1;
        }
    }

    // Write then
    return trackedImage.save2file(svar.GetString("TestReadWrite.File2Write","Data/TestReadWrite/result.bmp"));
}

int TestDataStream()
{
    // Load first
    string file2read=svar.GetString("TestReadWrite.File2Read","Data/TestReadWrite/00009.png");
    string extname=pi::path_extname(file2read);
    TrackedImage trackedImage;
    cout<<"ExtName="<<extname<<endl;
    if(extname==".png")
    {
        //Read from png
#ifdef HAS_OPENCV
        cv::Mat img=cv::imread(file2read);
        if(img.empty())
        {
            MSG_ERROR("Failed to load from file %s",file2read.c_str());
            return -1;
        }
        cv::imshow("img",img);
        cv::waitKey(-1);
        trackedImage=TrackedImage(img.cols,img.rows,img.channels());
        memcpy(trackedImage.img_data,img.data,img.cols*img.rows*img.channels());
#else
        MSG_ERROR("This need OpenCV support!\n");
        return -1;
#endif
    }
    else
    {
        //Read from raw data
        if(trackedImage.loadFromFile(file2read))
        {
#ifdef HAS_OPENCV
            cv::Mat img(trackedImage.img_h,trackedImage.img_w,CV_8UC3,trackedImage.img_data);

            if(img.empty())
            {
                MSG_ERROR("Failed to load from file %s",file2read.c_str());
                return -1;
            }
            cv::imshow("img",img);
            cv::waitKey(-1);
#else
            MSG_INFO("Loaded success!\n");
#endif
        }
        else
        {
            MSG_ERROR("Fail to load from file %s",file2read.c_str());
            return -1;
        }
    }

    pi::RDataStream ds;
    trackedImage.toStream(ds);

    TrackedImage backImage;
    backImage.fromStream(ds);

#ifdef HAS_OPENCV
    cv::Mat img2(backImage.img_h,backImage.img_w,CV_8UC3,backImage.img_data);

    if(img2.empty())
    {
        MSG_ERROR("Failed to load from file %s",file2read.c_str());
        return -1;
    }
    cv::imshow("back_img",img2);
    cv::waitKey(-1);
#endif

    return 0;
}

/** This demo shows how to use internet transfer tracked images,
 Usage:
 @code ./uav_transfer @endcode , start the server
 @code ./uavTransfer TestTransfer.NodeName=Client @endcode,
 Any node name except "Master" can be used to start a client and muti clients are supported.
**/
int TestTransfer()
{
    string nodeName=svar.GetString("TestTransfer.NodeName","Master");
    double rateFrequency=svar.GetDouble("TestTransfer.fps",10);
    InternetTransfer<TrackedImage> imgTransfer;

    if( imgTransfer.begin(nodeName) != 0 ) {
        return -1;
    }
    if(imgTransfer.isMaster())
    {

        //load tracked images from a folder
        string folderName=svar.GetString("TestTransfer.FolderPath",".");
        StringArray dl,tmp;
        pi::path_lsdir(folderName,tmp);
//        while(1) sleep(1);
        for(int i=0;i<tmp.size();i++)
            if(pi::path_extname(tmp[i])==".bmp")
            {
                dl.push_back(tmp[i]);
            }
        if(!dl.size()) return -1;

        //waiting clients
        pi::Rate rate(rateFrequency);
        while(imgTransfer.getNodeMap()->size()<2)
        {
            imgTransfer.getNodeMap()->print();
            sleep(1);
        }

        //send tracked images
        string filename;
        for(int i=0;i<dl.size();i++)
        {
            filename=folderName+"/"+dl[i];
            SPtr<TrackedImage> ele(new TrackedImage);
            ele->loadFromFile(filename);
            imgTransfer.send(ele);
            rate.sleep();
        }
    }
    else
    {
        //receive tracked images
        pi::Rate rate(rateFrequency);
        while(1)
        {
            if(imgTransfer.size())
            {
                SPtr<TrackedImage> ele=imgTransfer.pop();
                MSG_INFO("Loaded timestamp %f success!\n",ele->timestamp);

#ifdef HAS_OPENCV
                cv::Mat img(ele->img_h,ele->img_w,CV_8UC3,ele->img_data);

                if(img.empty())
                {
                    MSG_ERROR("Image is empty!");
                    return -1;
                }
                cv::imshow("img",img);
                cv::waitKey(10);
#endif
            }
            rate.sleep();
        }
    }

    return 0;
}

int TestSocketTransfer()
{
    string nodeName=svar.GetString("TestTransfer.NodeName","Master");
    double rateFrequency=svar.GetDouble("TestTransfer.fps",10);
    SocketTransfer<TrackedImage> imgTransfer;

    if( imgTransfer.begin(nodeName) != 0 ) {
        return -1;
    }

    if(imgTransfer.isMaster())
    {

        //load tracked images from a folder
        string folderName=svar.GetString("TestTransfer.FolderPath",".");
        StringArray dl,tmp;
        pi::path_lsdir(folderName,tmp);
//        while(1) sleep(1);
        for(int i=0;i<tmp.size();i++)
            if(pi::path_extname(tmp[i])==".bmp")
            {
                dl.push_back(tmp[i]);
            }
        if(!dl.size()) return -1;

        //send tracked images
        pi::Rate rate(rateFrequency);
        string filename;
        for(int i=0;i<dl.size();)
        {
            while(imgTransfer.size()>5) usleep(10000);
            filename=folderName+"/"+dl[i];
            SPtr<TrackedImage> ele(new TrackedImage);
            ele->loadFromFile(filename);
            imgTransfer.send(ele);
            rate.sleep();
            i++;
        }
    }
    else
    {
        //receive tracked images
        pi::Rate rate(rateFrequency);
        while(1)
        {
            if(imgTransfer.size())
            {
                SPtr<TrackedImage> ele=imgTransfer.pop();
                MSG_INFO("Loaded timestamp %f success!\n",ele->timestamp);

#ifdef HAS_OPENCV
                cv::Mat img(ele->img_h,ele->img_w,CV_8UC3,ele->img_data);

                if(img.empty())
                {
                    MSG_ERROR("Image is empty!");
                    return -1;
                }
                cv::imshow("img",img);
                cv::waitKey(10);
#endif
            }
            rate.sleep();
        }
    }

    return 0;
}

int TestMessagePassing()
{
    RMessagePassing     mp;
    RMessage            *msg, msg_send;
    int                 msgType = 1, msgID = 0;
    StringArray         nl;


    string              my_name;

    int                 i;
    ru64                t0, t1, dt;

    int                 ret;

    my_name=svar.GetString("Node.name","Master");
    if( mp.begin(my_name) != 0 ) {
        return -1;
    }

    t0 = tm_get_millis();

    while(1) {
        // receive a message & show it
        msg = mp.recvMsg();
        if( msg != NULL ) {
            string      n1, n2;
            DateTime   t0, t1;
            double      dt;

            t1.setCurrentDateTime();
            msg->data.rewind();
            msg->data.read(n1);
            msg->data.read(n1);
            t0.fromStream(msg->data);
            dt = t1.diffTime(t0);

            printf("\nrecved message, msg.size = %d, dt = %f\n   ",
                   msg->data.size(), dt);
            msg->print();

            delete msg;
        }

        // send message to other nodes
        t1 = tm_get_millis();
        dt = t1 - t0;
        if( dt > 60 ) {
            mp.getNodeMap()->getNodeList(nl);

            printf("\n");
            mp.getNodeMap()->print();
            printf("\n");


            for(i=0; i<nl.size(); i++) {
                if( nl[i] != my_name ) {
                    msg_send.msgType = msgType;
                    msg_send.msgID   = msgID++;

                    DateTime tm;
                    tm.setCurrentDateTime();

                    msg_send.data.clear();
                    msg_send.data.write(my_name);
                    msg_send.data.write(nl[i]);
                    tm.toStream(msg_send.data);

                    printf("send message: %s -> %s, msg_size = %d\n",
                           my_name.c_str(), nl[i].c_str(), msg_send.data.size());

                    mp.sendMsg(nl[i], &msg_send);
                }
            }

            t0 = t1;
        }
        usleep(1000);
    }
}

int DirectServer()
{
    int ret;
    RSocket socket;
    if(0!=socket.startServer(svar.GetInt("Master.port",30000)))
    {
        MSG_ERROR("Failed to start Server.");
    }

    while(1)
    {
        pi::RSocket new_socket;


        if( 0 != socket.accept(new_socket) ) {
            dbg_pe("server.accept failed!");
            continue;
        }
        //load tracked images from a folder
        string folderName=svar.GetString("TestTransfer.FolderPath",".");
        double rateFrequency=svar.GetDouble("TestTransfer.fps",10);
        StringArray dl,tmp;
        pi::path_lsdir(folderName,tmp);
        //        while(1) sleep(1);
        for(int i=0;i<tmp.size();i++)
            if(pi::path_extname(tmp[i])==".bmp")
            {
                dl.push_back(tmp[i]);
            }
        if(!dl.size())
        {
            MSG_ERROR("No image found in %s",folderName.c_str());
            return -1;
        }

        //send tracked images
        string filename;
        pi::Rate rate(rateFrequency);
        int headSize=sizeof(TrackedImage)-sizeof(u_char*);
        for(int i=0;i<dl.size();i++)
        {
            filename=folderName+"/"+dl[i];
            TrackedImage ele;
            if(ele.loadFromFile(filename))
            {
                MSG_INFO("Sending timestamp %f",ele.timestamp);
                ret=new_socket.send((u_char*)&ele,headSize);
                if(ret==headSize)
                    ret=new_socket.send(ele.img_data,ele.img_c*ele.img_h*ele.img_w);
                if( ret < 0 ) {
                    dbg_pe("Connection lost!");
                    break;
                } else if( ret < ele.img_c*ele.img_h*ele.img_w ) {
                    dbg_pw("Send data not correct!");
                    continue;
                }
            }
            rate.sleep();
        }
    }
    return 0;
}

int DirectClient()
{
    RSocket socket;
    if(0!=socket.startClient(svar.GetString("Master.ip","127.0.0.1"),
                             svar.GetInt("Master.port",30000)))
    {
        MSG_ERROR("Failed to start client.");
    }

    double rateFrequency=svar.GetDouble("TestTransfer.fps",10);
    pi::Rate rate(rateFrequency);

    int headSize=sizeof(TrackedImage)-sizeof(u_char*);
    while(1)
    {
        TrackedImage ele;
        int ret=socket.recv_until((u_char*)&ele,headSize);
        int imgSize=ele.img_c*ele.img_h*ele.img_w;
        ele.img_data=new u_char[imgSize];

        if(ret==headSize)
            ret=socket.recv_until(ele.img_data,imgSize);

        if( ret < 0 ) {
            dbg_pe("Connection lost!");
            break;
        } else if( ret != imgSize ) {
            dbg_pw("Received data not correct!");
            break;
        }

        MSG_INFO("Loaded timestamp %f success!\n",ele.timestamp);

#ifdef HAS_OPENCV
        cv::Mat img(ele.img_h,ele.img_w,CV_8UC3,ele.img_data);

        if(img.empty())
        {
            MSG_ERROR("Image is empty!");
            return -1;
        }
        cv::imshow("img",img);
        cv::waitKey(10);
#endif
        rate.sleep();
    }
}


#ifdef HAS_OPENCV
cv::Mat Img2Quadr(std::vector<cv::Point2f>& pts,const cv::Mat &src)
{
    timer.enter("Img2Quadr");
    float xmin=pts[0].x,xmax=pts[0].x,ymin=pts[0].y,ymax=pts[0].y;
    for(int i=1;i<4;i++)
    {
        if(pts[i].x<xmin) xmin=pts[i].x;
        if(pts[i].x>xmax) xmax=pts[i].x;
        if(pts[i].y<ymin) ymin=pts[i].y;
        if(pts[i].y>ymax) ymax=pts[i].y;
    }
    int w=xmax-xmin,h=ymax-ymin;
    cv::Mat dst;
    std::vector<cv::Point2f> corners;
    corners.push_back(cv::Point2f(0,0));
    corners.push_back(cv::Point2f(src.cols-1,0));
    corners.push_back(cv::Point2f(0,src.rows-1));
    corners.push_back(cv::Point2f(src.cols-1,src.rows-1));

    bool showPoints=svar.GetInt("ShowPoints",1);
    bool applyDiff=svar.GetInt("ApplyDiff",1);
    cv::Point2f diff;
    if(svar.GetInt("AutoDiff",0))
    {
        diff=cv::Point2f(xmin,ymin);
        dst=cv::Mat::zeros(h,w,src.type());
    }
    else
    {
        static cv::Point2f lastDiff(xmin,ymin);
        static cv::Mat result=cv::Mat::zeros(h,w,src.type());
        static bool firstInsert=true;
        diff=lastDiff;
        if(firstInsert)
        {
            firstInsert=false;
        }
        else
        {
            cerr<<"Last:"<<lastDiff<<endl;
            cerr<<"MIN:"<<",["<<xmin<<","<<ymin<<"],"<<"MAX["<<xmax<<","<<ymax<<"]\n";
            if(xmin<diff.x) diff.x=xmin;
            if(ymin<diff.y) diff.y=ymin;
            if(ymax<lastDiff.y+result.rows) ymax=lastDiff.y+result.rows;
            if(xmax<lastDiff.x+result.cols) xmax=lastDiff.x+result.cols;
            if(ymax-diff.y!=result.rows||xmax-diff.x!=result.cols)
            {
                cv::Rect rect(lastDiff.x-diff.x,lastDiff.y-diff.y,result.cols,result.rows);
                cv::Mat newResult=cv::Mat::zeros(ymax-diff.y,xmax-diff.x,result.type());
                result.clone().convertTo(newResult(rect), newResult.type(), 1, 0);
                cerr<<"ROI:"<<rect<<"ResultSize:["<<newResult.cols<<newResult.rows<<"]"<<endl;
                result=newResult;
            }
            cerr<<"NEW AREA:"<<diff<<",["<<xmax<<","<<ymax<<"]\n";
            lastDiff=diff;
        }
        dst=result;
    }

    if(applyDiff)
    {
        for(int i=0;i<pts.size();i++)
        {
            pts[i]=pts[i]-diff;
            if(showPoints)
                cout<<"PT"<<i<<":"<<pts[i]<<"\n";
        }
    }

    cv::Mat transmtx = cv::getPerspectiveTransform(corners, pts);
    cv::warpPerspective(src, dst, transmtx, dst.size(),cv::INTER_LINEAR,cv::BORDER_TRANSPARENT);
    timer.leave("Img2Quadr");
    return dst;
}

cv::Point2f toPoint2f(const pi::Point3d& pt)
{
    static GPS_Utils* gps=NULL;
    if(!gps) gps=new GPS_Utils(pt);

    pi::Point3d pt3d=gps->Pos2XYZ(pt);
    return cv::Point2f(pt3d.x,-pt3d.y);
}
#endif

int Quadr_Test()
{
    //load tracked images from a folder
    string folderName=svar.GetString("TestTransfer.FolderPath",".");
    string folder2Save=svar.GetString("Quadr_Test.Folder2Save","");
    double rateFrequency=svar.GetDouble("TestTransfer.fps",10);
    StringArray dl,tmp;
    pi::path_lsdir(folderName,tmp);
    for(int i=0;i<tmp.size();i++)
        if(pi::path_extname(tmp[i])==".bmp")
        {
            dl.push_back(tmp[i]);
        }
    if(!dl.size())
    {
        MSG_ERROR("No image found in %s",folderName.c_str());
        return -1;
    }

    //send tracked images
    string filename;
    pi::Rate rate(rateFrequency);
    float factor=svar.GetDouble("GPS.Factor",1.0);
#ifdef HAS_OPENCV
    cv::Mat result;
#endif
    for(int i=0;i<dl.size();i++)
    {
        filename=folderName+"/"+dl[i];
        TrackedImage ele;
        if(ele.loadFromFile(filename))
        {
            MSG_INFO("Sending timestamp %f",ele.timestamp);

#ifdef HAS_OPENCV
            cv::Mat img(ele.img_h,ele.img_w,CV_8UC3,ele.img_data);

            if(img.empty())
            {
                MSG_ERROR("Image is empty!");
                return -1;
            }
            cv::Mat src;
            cv::cvtColor(img,src,CV_BGR2BGRA);

            vector<cv::Point2f> pts;
            pts.push_back(toPoint2f(ele.lt)*factor);
            pts.push_back(toPoint2f(ele.rt)*factor);
            pts.push_back(toPoint2f(ele.lb)*factor);
            pts.push_back(toPoint2f(ele.rb)*factor);
            result=Img2Quadr(pts,src);

            cv::imshow("img",src);
            cv::imshow("result",result);

            // save result to file
            if(folder2Save.size()&&svar.GetInt("AutoDiff",0))
            {
                cv::imwrite(folder2Save+"/result_"+dl[i]+".png",result);
            }

            uchar key=cv::waitKey(0);
            if(key==27) break;
#endif
        }
        rate.sleep();
    }
#ifdef HAS_OPENCV
    if(!svar.GetInt("AutoDiff",0))
    {
        cv::imwrite(folder2Save+"/result.png",result);
    }

    cv::waitKey(0);
#endif

}


int echoHelp()
{
    cerr<<"No such action! Please use parament 'Act' to choose an operation.\n";
    return 0;
}

int main(int argc,char** argv)
{
    svar.ParseMain(argc,argv);
    string act=svar.GetString("Act","DefaultAct");

    if(act=="DefaultAct")    return echoHelp();
    if(act=="TestReadWrite") return TestReadWrite();
    if(act=="TestDataStream") return TestDataStream();
    if(act=="TestMessagePassing") return TestMessagePassing();

    if(act=="TestTransfer")  return TestTransfer();
    if(act=="TestSocketTransfer")  return TestSocketTransfer();
    if(act=="DirectServer")    return DirectServer();
    if(act=="DirectClient")    return DirectClient();
    if(act=="Quadr_Test") return Quadr_Test();

    return 0;
}
