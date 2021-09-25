#ifndef read_file_h
#define read_file_h

struct Point {
    double x; //经度
    double y; //纬度
};

//后续求解所有问题都基于这两个数据结构
vector<map<int, vector<Point>>> Satellite; //卫星
vector<map<string, vector<double>>> Target; //目标

void read_file1() { //读取文件
    int num1 = 86400, num2 = 21;
    Point point; //经纬度
    for (int k = 0; k < 9; k++) { //读取SatelliteInfo
        string temp; // 日期
        map<int, vector<Point>> SatInfo;
        ifstream infile("./SatelliteInfo/SatCoverInfo_" + to_string(k) + ".txt");
        if (!infile) infile.close();
        for (int i = 0; i <= num1; i++) {
            vector<Point> longlat;
            getline(infile, temp, '\r');
            for (int j = 0; j < num2; j++) {
                infile >> point.x >> point.y; //读取经度、纬度
                longlat.push_back(point);
            }
            SatInfo[i] = longlat;
            infile.get(); //跳过换行符
        }
        Satellite.push_back(SatInfo); //9个卫星
        infile.close();
    }
}

void read_file2() { //读取文件
    int num = 4;
    for (int k = 1; k <= 10; k++) { //读取TargetInfo
        string city; //城市
        map<string, vector<double>> target;
        ifstream infile("./TargetInfo/target" + to_string(k) + ".txt");
        if (!infile) infile.close();
        while (!infile.eof()) {
            infile >> city;
            vector<double> TarInfo;
            if (infile.fail()) break; //判断读到文件最后一行
            for (int i = 0; i < num; i++) {
                double temp;
                infile >> temp;
                if (i == 0 && temp < 0) {
                    temp = temp + 360; //经度小于0，加360 与卫星经纬度统一
                }
                TarInfo.push_back(temp); //读取每个城市对应的信息
            }
            target[city] = TarInfo;
        }
        Target.push_back(target); //10个城市文件
        infile.close();
    }
}

string output_date(int time) { //将时间转换为日期
    string date = "2022/1/1 ";
    if (time == 86400) {
        date = "2022/1/2 0:00:00";
    }
    else {
        int hour = time / 3600 % 24, minute = time / 60 % 60, second = time % 60; //构造时分秒
        date.append(to_string(hour) + ":");
        if (minute < 10) date.append("0" + to_string(minute) + ":");
        else date.append(to_string(minute) + ":");
        if (second < 10) date.append("0" + to_string(second));
        else date.append(to_string(second));
    }
    return date;
}

#endif
