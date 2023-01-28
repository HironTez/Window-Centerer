windres "./versiondetails.rc" -O coff -o "./build/versiondetails.res"
windres "./iconinfo.rc" -O coff -o "./build/iconinfo.res"
g++ -mwindows -g "./Window Centerer.cpp" -o "./build/Window Centerer.exe" "./build/versiondetails.res" "./build/iconinfo.res"
