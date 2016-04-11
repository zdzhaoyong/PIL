1. Dependency
	Opencv2 are used for image display and could be disabled by comment the 'HAS_OPENCV' in the file 'src/Makefile'. This software can be compiled with or without opencv. Since 'shared_ptr' in namespace std::tr1 are used in the base library, at least c++11 is needed.

2. Compile
	do 'make' in the top directory. Once you don't want to use opencv, please comment the two lines in 'src/Makefile'.

3. Run
	While './uavTransfer' can be used to lauch the application, some configuration are needed to setup default operation and paraments. More infomation about configuration can be obtained in the file 'Default.cfg'.
    The data struct "TrackedImage" are transfered between computers on TCP/IP socket. Three demos have been shown in this project.

3.1. Direct transfer
#Start the server
./uavTransfer Act=DirectServer TestTransfer.FolderPath='Folder to hold images'

#Start the client
./uavTransfer Act=DirectClient

3.2. Transfer by SocketTransfer&DataStream
#Start the server
./uavTransfer Act=TestSocketTransfer TestTransfer.NodeName=Master TestTransfer.FolderPath='Folder to hold images'
#Start the client
./uavTransfer Act=TestSocketTransfer TestTransfer.NodeName=Client

3.3. Transfer by InternetTransfer&DataStream
#Start the server
./uavTransfer Act=TestTransfer TestTransfer.NodeName=Master TestTransfer.FolderPath='Folder to hold images'
#Start the client
./uavTransfer Act=TestTransfer TestTransfer.NodeName=Client
#Multiple clients are supported and you can start the second client as follows:
./uavTransfer Act=TestTransfer TestTransfer.NodeName=Client1 Client1.port=30001

4. Multiple computer configuration
    If you are going to use this procedure to transfer TrackedImages between different computers, some extra configuration about ip and port of Server is needed. It can be seted either in the file "Default.cfg" or arguments directly.
Master.ip=127.0.0.1 #Default ip is this computer
Master.port=30000   #Default port is 30000

