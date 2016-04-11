#include "TrackedImage.h"
#include <fstream>
#include <iostream>
#include <base/utils/utils_str.h>

using namespace pi;
using namespace std;

static unsigned char header[] =
{
    66,77,54,16,14,0,0,0,0,0,54,0,0,0,40,0,0,0,128,2,0,0,224,1,0,0,
    1,0,24,0,0,0,0,0,0,16,14,0,18,11,0,0,18,11,0,0,0,0,0,0,0,0,0,0
};//BMP header

bool saveImage(std::string file_name,int w,int h,int c,u_char* data)
{
    if(w*h*c<=0) return false;
    header[18] = w%256;
    header[19] = w/256;
    header[22] = h%256;
    header[23] = h/256;
    FILE* file = fopen(file_name.c_str(),"wb");
    fwrite(header,54,1,file);
    fwrite(data,w*h*c,1,file);
    fclose(file);
    return true;
}

bool loadImage(std::string file_name,int& w,int& h,int& c,u_char** data)
{
    FILE* file = fopen(file_name.c_str(),"rb");
    if (file!=NULL)
    {
        fread(header,54,1,file);
        w=header[19]*256+header[18];
        h=header[23]*256+header[22];
        c=3;
        size_t size=w*h*c;
        *data=new u_char[size];
        if(!*data) return false;
        if(fread(*data,size,1,file)< 1)
            return false;
        fclose(file);
        return true;
    }
    return false;
}

TrackedImage::TrackedImage(int w,int h,int c):img_c(c),img_w(w),img_h(h)
{
    int size=w*h*c;
    if(size>0)
    {
        img_data=(u_char*)malloc(size);//(ru8*)new pi::Array_<char,w*h*c>();
    }
    else
        img_data=NULL;
}

TrackedImage::TrackedImage(TrackedImage& tr):
    timestamp(tr.timestamp),lt(tr.lt),rt(tr.rt),lb(tr.lb),rb(tr.rb),
    CameraPose(tr.CameraPose),img_w(tr.img_w),img_h(tr.img_h),img_c(tr.img_c)
{
    const size_t size=img_w*img_h*img_c;
    img_data=new u_char[size];
    memcpy(img_data,tr.img_data,size);
}

int TrackedImage::toStream(pi::RDataStream &ds)
{
    // check image is empty
    if( !img_data ) return -1;

    // clear datastream
    ds.clear();

    // set magic number & version number
    ds.setHeader(0x83F8, 1);

    ds.write((u_char*)&timestamp,sizeof(TrackedImage)-sizeof(u_char*));
    ds.write(img_data,img_w*img_h*img_c);

    return 0;
}

int TrackedImage::fromStream(pi::RDataStream &ds)
{
    ru32        d_magic, d_ver;

    // rewind to begining
    ds.rewind();

    // get magic & version number
    ds.getHeader(d_magic, d_ver);

    if( d_magic != 0x83F8 ) {
        dbg_pe("Input data magic number error! %x\n", d_magic);
        return -1;
    }

    if(0 != ds.read((ru8*)&timestamp,sizeof(TrackedImage)-sizeof(u_char*))) return -2;
    int length=img_w*img_h*img_c;
    if(length<=0) return -3;

    img_data=(u_char*)malloc(length);
    if( 0 != ds.read(img_data,length) )      return -4;

    return 0;
}

int TrackedImage::addToFile   (std::fstream& fst)
{
    fst.write((char*)this,sizeof(TrackedImage)-sizeof(u_char*));
    fst.write((char*)img_data,img_w*img_h*img_c);
    return 0;
}

int TrackedImage::takeFromFile(std::fstream& fst)
{
    fst.read((char*)this,sizeof(TrackedImage)-sizeof(u_char*));
    fst.read((char*)img_data,img_w*img_h*img_c);
    return 0;
}

bool TrackedImage::save2file(const std::string& filename)
{
    string imgFileName=filename;
    string infoFileName=filename+".info";

    // save info
    ofstream info_file(infoFileName.c_str());
    if(!info_file.is_open()) return false;
    info_file<<pi::dtos(timestamp)<<" "<<lt<<" "
            <<rt<<" "<<lb<<" "
           <<rb<<" "<<CameraPose<<endl;

    // save image
    return saveImage(imgFileName,img_w,img_h,img_c,img_data);
}

bool TrackedImage::loadFromFile(const std::string& filename)
{
    string imgFileName=filename;
    string infoFileName=filename+".info";

    // save info
    ifstream info_file(infoFileName.c_str());
    if(!info_file.is_open()) return false;
    info_file>>timestamp>>lt
            >>rt>>lb
           >>rb>>CameraPose;

    // save image
    return loadImage(imgFileName,img_w,img_h,img_c,&img_data);
}
