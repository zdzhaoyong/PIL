#include <slam/Sim3Solver.h>
#include <base/Svar/Svar.h>
#include <base/types/SE3.h>

#include <fstream>

using namespace std;


struct PoseData
{
    double timestamp;
    pi::SE3d    se3;

    inline friend std::istream& operator >>(std::istream& is,PoseData& rhs)
    {
        double x,y,z,rx,ry,rz,w;
        is>>rhs.timestamp>>x>>y>>z>>rx>>ry>>rz>>w;
        rhs.se3=pi::SE3d(x,y,z,rx,ry,rz,w);
        return is;
    }

    inline friend std::ostream& operator <<(std::ostream& os,const PoseData& rhs)
    {
        os<<rhs.timestamp<<" "<<rhs.se3<<"\n";
        return os;
    }
};

typedef std::vector<PoseData> Trajectory;

Trajectory loadFromFile(string filename)
{
    Trajectory trajectory;
    ifstream ifs(filename.c_str());
    if(!ifs.is_open())
    {
        cout<<"Can't open file "<<filename<<endl;
        return trajectory;
    }

    string line;
    while(getline(ifs,line))
    {
        PoseData dt;
        stringstream str(line);
        str>>dt;
        trajectory.push_back(dt);
    }

    cout<<"Got trajectory length:"<<trajectory.size()<<endl;
    return trajectory;
}

std::vector<Trajectory> loadFromFile(string filename,int num)
{
    std::vector<Trajectory> trajectory;
    ifstream ifs(filename.c_str());
    if(!ifs.is_open())
    {
        cout<<"Can't open file "<<filename<<endl;
        return trajectory;
    }
    else
    {
        cout<<"Loading "<<filename<<endl;
    }
    trajectory.resize(num);
    string line;
    while(getline(ifs,line))
    {
        stringstream str(line);
        for(int i=0;i<num;i++)
        {
            PoseData dt;
            str>>dt;
            trajectory[i].push_back(dt);
        }
    }

    cout<<"Got trajectory length:"<<trajectory[0].size()<<endl;
    return trajectory;
}

int save2File(Trajectory& traj,string filename)
{
    ofstream ofs(filename.c_str());
    if(!ofs.is_open())
    {
        cout<<"Can't open file "<<filename<<endl;
        return -1;
    }
    for(int i=0;i<traj.size();i++)
    {
        ofs<<traj[i];
    }
    return 0;
}

int transformTraj(Trajectory& traj,const pi::SE3d& trans)
{
    for(int i=0,iend=traj.size();i<iend;i++)
    {
        PoseData& data=traj[i];
        data.se3=data.se3*trans;
    }
}

bool valid(const Trajectory& ground,const Trajectory& test)
{
    return ground.size()==test.size()&&ground[0].timestamp-test[0].timestamp<0.01;
}

pi::SE3d findTrans(const Trajectory& ground,const Trajectory& test)
{
    pi::SIM3d result;
    if(!valid(ground,test))
    {
        cerr<<"Not valid!\n";
        return result.get_se3();
    }
    Sim3Solver solver;
    vector<pi::Point3d> groundPoints,testPoints;
    groundPoints.reserve(ground.size());
    testPoints.reserve(ground.size());
    for(int i=0,iend=ground.size();i<iend;i++)
    {
        groundPoints.push_back(ground[i].se3.get_translation());
        testPoints.push_back(test[i].se3.get_translation());
    }
    solver.getSim3Fast(testPoints,groundPoints,result);
    return result.get_se3();
}

double computeATE(const Trajectory& ground,const Trajectory& test)
{
    if(!valid(ground,test))
    {
        cerr<<"Not valid!\n";
        return -1;
    }
    double sum=0;
    for(int i=0,iend=ground.size();i<iend;i++)
    {
        pi::Point3d e=ground[i].se3.get_translation()-test[i].se3.get_translation();
        sum+=e.dot(e);
    }
    return sqrt(sum/ground.size());
}

int main(int argc,char** argv)
{
    svar.ParseMain(argc,argv);
    Trajectory Ground,Traj;
    if(0)
    {
        string GroundTrueFile=svar.GetString("GroundTrueFile","groundtrue.txt");
        string TrajectoryFile=svar.GetString("TrajectoryFile","Trajectory.txt");
        Ground=loadFromFile(GroundTrueFile);
        Traj  =loadFromFile(TrajectoryFile);
    }
    else
    {
        std::vector<Trajectory> trajs=loadFromFile(svar.GetString("AssociateFile","TrajAsso.txt"),2);
        if(trajs.size()!=2) return -1;
        Ground=trajs[0];
        Traj  =trajs[1];
    }
    if(!valid(Ground,Traj))
    {
        cout<<"Size1:"<<Ground.size()<<",Size2:"<<Traj.size()<<endl;
        if(Ground.size()&&Traj.size())
            cout<<"Ground0:"<<Ground[0]<<endl<<"Traj0:"<<Traj[0];
        return -2;
    }
    cout<<"ATE before transform:"<<computeATE(Ground,Traj)<<endl;

    pi::SE3d trans=findTrans(Ground,Traj);
    transformTraj(Traj,trans.inverse());
    cout<<"ATE after transform:"<<computeATE(Ground,Traj)<<endl;
    cout<<"Transfrom:"<<trans<<endl;
    return 0;
}

