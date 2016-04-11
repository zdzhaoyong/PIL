#ifndef TRACKEDIMAGE_H
#define TRACKEDIMAGE_H

#include <deque>
#include <base/types/types.h>
#include <base/Svar/DataStream.h>

// Warning: c should be 3, FIXME: the .bmp image are upsidedown
bool saveImage(std::string file_name,int w,int h,int c,u_char* data);
bool loadImage(std::string file_name,int& w,int& h,int& c,u_char** data);

class TrackedImage
{
public:
    TrackedImage(int w=0,int h=0,int c=1);
    TrackedImage(TrackedImage& tr);
    ~TrackedImage(){delete[] img_data;}

    // DataStream interface
    int toStream(pi::RDataStream &ds);
    int fromStream(pi::RDataStream &ds);

    // file steam
    int addToFile   (std::fstream& fst);
    int takeFromFile(std::fstream& fst);

    // interface to a image file and a info file
    bool save2file(const std::string& filename);
    bool loadFromFile(const std::string& filename);

    double  timestamp;       // videoframe timestamp

    pi::Point3d lt,rt,lb,rb; // 4 points to fix the texture
    pi::Point3d CameraPose;  // the center of camera

    //image data
    int   img_w,img_h,img_c; // Image width, height, channels
    u_char* img_data;          // Pointer to the image data
};



#endif // TRACKEDIMAGE_H
