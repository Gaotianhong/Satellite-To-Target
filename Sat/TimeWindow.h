#ifndef TimeWindow_h
#define TimeWindow_h

#include "read_file.h"

bool isPolygon(const Point& tmp, vector<Point>& Polygon) { //判断一个点在不在多边形内部
    size_t vertNumber = Polygon.size();
    bool inside = false;
    for (size_t i = 1; i <= vertNumber; i++) { //奇内偶外
        const Point& P1 = Polygon[i - 1];
        const Point& P2 = Polygon[i % vertNumber];
        if ((P2.y <= tmp.y && tmp.y < P1.y) || (P1.y <= tmp.y && tmp.y < P2.y)) {
            double t = (tmp.x - P2.x)*(P1.y - P2.y) - (P1.x - P2.x)*(tmp.y - P2.y);
            if (P1.y < P2.y) t = -t;
            if (t < 0) inside = !inside;
        }
    }
    return inside;
}

vector<vector<int>> jiaoji(vector<vector<int>>& tmp1, vector<vector<int>>& tmp2) {
    vector<vector<int>> res;
    size_t n1 = tmp1.size(), n2 = tmp2.size();
    if (n1 ==0 || n2 == 0) return res;
    int i = 0, j = 0; // 两个数组的指针
    while (i < n1 && j < n2) {
        int begin = max(tmp1[i][0], tmp2[j][0]);
        int end = min(tmp1[i][1], tmp2[j][1]);
        if (begin <= end) res.push_back({begin, end});
        // 按照判断去遍历下一个
        tmp1[i][1] < tmp2[j][1] ? ++i : ++j;
    }
    return res;
}//求区间的交集用于计算多重时间窗口

struct Window {
    string city; //城市
    long number; //卫星编号
    vector<vector<int>> tW; //时间覆盖窗口，tW[i][j]代表某个卫星对某个城市有i个时间覆盖窗口，tW[i][0]开始，tw[i][1]结束
    Window(long _number, string _city, vector<vector<int>> _tW) { //构造函数
        number = _number;
        city = _city;
        tW =  _tW;
    }
};

void SatTimeWindow() { //计算星座对每个点目标的时间窗口
    ofstream outfile("./Test/timeWindow.txt"); //将结果输入到文件
    auto tar_it = Target.begin(); //这边是遍历第一个数据，可根据需求改动
    vector<Window> window; //时间覆盖窗口，这个是用来计算二重时间窗口的
    outfile << "Target" + to_string(tar_it - Target.begin() + 1) << endl;
    for (auto _tar = tar_it->begin(); _tar != tar_it->end(); _tar++) { //每一个目标数据集对应的城市
        vector<int> MAXDuration; //目标时间间隔的最大值
        vector<int> TOTALDuration; //总的时间间隔
        outfile << _tar->first << "可以观测到的时间窗口" << endl; //城市名
        for (auto itofSat = Satellite.begin(); itofSat != Satellite.end(); itofSat++) { //9个卫星
            bool aSign = true;
            vector<int> timeWindow; //时间窗口
            vector<int> timeInerval; //时间间隔
            vector<vector<int>> multiTimeWindow; //多重时间窗口
            outfile << "对于Satellite" + to_string(itofSat - Satellite.begin()) << endl;
            for (auto Sat = itofSat->begin(); Sat != itofSat->end(); Sat++) { //86401
                if (isPolygon({_tar->second[0], _tar->second[1]}, Sat->second)) {
                    if (aSign == true) { //计算恰好能够对目标提供服务的开始时间
                        aSign = false;
                        timeWindow.push_back(Sat->first); //记录
                        outfile << "[" << output_date(Sat->first) << ",";
                    }
                }
                else { //卫星恰好不能对目标提供服务的时间，前闭后开区间
                    if (aSign == false) {
                        aSign = true;
                        timeWindow.push_back(Sat->first);
                        timeInerval.push_back(timeWindow[1] - timeWindow[0]);
                        multiTimeWindow.push_back(timeWindow); //多重时间窗口
                        outfile << output_date(Sat->first) << ") " << timeWindow[1] - timeWindow[0] << "s" << endl;
                        timeWindow.clear(); //清空
                    }
                }
            }
            if(timeInerval.empty()) continue; //时间间隔不存在
            Window tmp(itofSat - Satellite.begin(), _tar->first, multiTimeWindow);
            window.push_back(tmp); //用于计算二重时间覆盖窗口
            int maxDuration = 0; //求时间间隔的最大值
            for (auto dur = timeInerval.begin(); dur != timeInerval.end(); dur++) {
                if (maxDuration < *dur) {
                    maxDuration = *dur;
                }
            }
            MAXDuration.push_back(maxDuration);
            for (auto it : timeInerval) TOTALDuration.push_back(it);
            multiTimeWindow.clear();
            timeInerval.clear();
        }
        vector<vector<int>> caljiaoji; //交集
        for (auto it1 = window.begin(); it1 != window.end(); it1++) {
            for (auto it2 = it1 + 1; it2 != window.end(); it2++) {
                caljiaoji = jiaoji(it1->tW, it2->tW);
                if (!caljiaoji.empty()) {
                    outfile << "Satellite" + to_string(it1->number) + "和Satllite" + to_string(it2->number) + "观测到的二重覆盖时间窗口：";
                    for (auto it : caljiaoji)
                        outfile << "[" << output_date(it[0]) << "," << output_date(it[1]) << ")" << endl;
                }
            }
        }
        double totalSum = 0;
        for (auto t = TOTALDuration.begin(); t != TOTALDuration.end(); t++) {
            totalSum = totalSum + *t;
        }
        int maxTimeInterval = 0;
        for (auto dur = MAXDuration.begin(); dur != MAXDuration.end(); dur++) {
            if (maxTimeInterval < *dur) {
                maxTimeInterval = *dur;
            }
        }
        outfile << "目标时间间隔的最大值：" << maxTimeInterval << "s，平均值：" << totalSum / TOTALDuration.size() << "s" << endl;
        window.clear(); //清空
    }
    outfile << endl;
    outfile.close();
}

#endif
