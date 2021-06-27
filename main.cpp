#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <queue>
#include <map>
#include <algorithm>
#include <numeric>
using namespace std;

int fun_selection() { //功能选择函数
    cout << "请输入需要求解的问题：" << endl;
    cout << "[1] 时间窗口计算" << endl;
    cout << "[2] 卫星调度规划问题" << endl;
    cout << "[3] 数据存储问题" << endl;
    cout << "[0] 退出" << endl;
    cout << "请选择（输入数字）：";
    int selection;
    cin >> selection;
    return selection;
}

struct LL {
    double longitude; //经度
    double latitude; //纬度
};
struct coverWindow {
    long sat; //卫星编号
    string city; //城市
    vector<vector<int>> tW; //时间覆盖窗口，tW[i][j]代表某个卫星对某个城市有i个时间覆盖窗口，tW[i][0]开始，tw[i][1]结束
    coverWindow(long _sat, string _city, vector<vector<int>> _tW):sat(_sat), city(_city), tW(_tW){} //构造函数
};

map<int, vector<LL>> SatCoverInfo;
vector<map<int, vector<LL>>> SatelliteInfo; //卫星

map<string, vector<double>> target;
vector<map<string, vector<double>>> TargetInfo; //目标

vector<int> TransTime = {30, 30, 30, 35, 35, 35, 25, 25, 25}; //卫星的转换时长约束

void read_file() { //读取文件
    cout << "loading data..." << endl;
    string date; // 日期
    LL ll; //经纬度
    vector<LL> llInfo;
    for (int k = 0; k < 9; k++) { //读取SatelliteInfo
        ifstream infileSat("./SatelliteInfo/SatCoverInfo_" + to_string(k) + ".txt");
        if (!infileSat) infileSat.close();
        for (int i = 0; i <= 86400; i++) {
            getline(infileSat, date, '\r');
            for (int j = 0; j < 21; j++) {
                infileSat >> ll.longitude >> ll.latitude; //读取经度、纬度
                llInfo.push_back(ll);
            }
            SatCoverInfo[i] = llInfo;
            llInfo.clear(); //清空
            infileSat.get(); //跳过换行符
        }
        SatelliteInfo.push_back(SatCoverInfo); //9个卫星
        SatCoverInfo.clear(); //清空
        infileSat.close();
    }

    string city; //城市
    vector<double> tarInfo;
    for (int k = 1; k <= 10; k++) { //读取TargetInfo
        ifstream infileTar("./TargetInfo/target" + to_string(k) + ".txt");
        if (!infileTar) infileTar.close();
        while (!infileTar.eof()) {
            infileTar >> city;
            if (infileTar.fail()) break; //判断读到文件最后一行
            for (int i = 0; i < 4; i++) {
                double temp;
                infileTar >> temp;
                if (i == 0 && temp < 0) temp += 360; //经度小于0，加360 与卫星经纬度统一
                tarInfo.push_back(temp); //读取每个城市对应的信息
            }
            target[city] = tarInfo;
            tarInfo.clear(); //清空
        }
        TargetInfo.push_back(target); //10个城市文件
        target.clear(); //清空
        infileTar.close();
    }
    cout << "reading is OK!" << endl;
}

string time_to_date(int time) { //将时间转换为日期
    if (time == 86400) return "2022-01-02 0:00:00";
    string date = "2022-01-01 ";
    int hour = time / 3600 % 24, minute = time / 60 % 60, second = time % 60; //构造时分秒
    date.append(to_string(hour) + ":");
    if (minute < 10) date.append("0" + to_string(minute) + ":");
    else date.append(to_string(minute) + ":");
    if (second < 10) date.append("0" + to_string(second));
    else date.append(to_string(second));
    return date;
}

//求两个区间的交集
vector<vector<int>> Intersection(vector<vector<int>>& time1, vector<vector<int>>& time2) {
    vector<vector<int>> res;
    size_t n1 = time1.size(), n2 = time2.size();
    if (n1 ==0 || n2 == 0) return res;
    int i = 0, j = 0; // 两个数组的指针
    while (i < n1 && j < n2) {
        int start = max(time1[i][0], time2[j][0]);
        int end = min(time1[i][1], time2[j][1]);
        // 注意这里是小于等于，因为[5,5]也是有效的区间，虽然只有一个元素
        if (start <= end) res.push_back({start, end});
        // 按照判断去遍历下一个
        time1[i][1] < time2[j][1] ? ++i : ++j;
    }
    return res;
}

//若要检测的点在多变形的一条边上，射线法判断的结果是不确定的
bool isPointInsidePoly(const LL& ll, vector<LL>& polyVertices) { //判断一个点在不在多边形内部 奇内偶外
    size_t vertCount = polyVertices.size();
    if (vertCount < 2) return false; //多边形顶点数量小于2返回false
    bool inside = false;
    for (size_t i = 1; i <= vertCount; i++) { //增加longitude，固定latitude，计算它穿过多少条边（乔丹曲线定理）
        const LL& A = polyVertices[i - 1];
        const LL& B = polyVertices[i % vertCount];
        if ((B.latitude <= ll.latitude && ll.latitude < A.latitude) || (A.latitude <= ll.latitude && ll.latitude < B.latitude)) { //纬度
            double t = (ll.longitude - B.longitude)*(A.latitude - B.latitude) - (A.longitude - B.longitude)*(ll.latitude - B.latitude);
            if (A.latitude < B.latitude) t = -t;
            if (t < 0) inside = !inside;
        }
    }
    return inside;
}

void calTimeWindow() { //计算星座对每个点目标的时间窗口
    ofstream outfile("/Users/gaotianhong/Desktop/Python/卫星对地覆盖计算及任务规划/Data/TimeWindow.txt"); //将结果输入到文件
    if (!outfile) outfile.close();
    for (auto tar_it = TargetInfo.begin(); tar_it != TargetInfo.end(); tar_it++) { //遍历10个目标数据集
//        auto tar_it = TargetInfo.begin();
        vector<coverWindow> cW; //时间覆盖窗口
        vector<int> max_duration; //星座的时间间隔的最大值
        vector<int> total_duration; //总的时间间隔
        outfile << "target" + to_string(tar_it - TargetInfo.begin() + 1) << endl;
        for (auto _tar = tar_it->begin(); _tar != tar_it->end(); _tar++) { //每一个目标数据集对应的城市
            outfile << _tar->first << "：" << endl; //城市名
            for (auto sat_it = SatelliteInfo.begin(); sat_it != SatelliteInfo.end(); sat_it++) { //9个卫星
                bool flag = true;
                vector<int> time_window; //时间窗口
                vector<vector<int>> multi_tW; //多重时间窗口
                vector<int> duration; //时间间隔
                outfile << "SatCoverInfo_" + to_string(sat_it - SatelliteInfo.begin()) << endl;
                for (auto _sat = sat_it->begin(); _sat != sat_it->end(); _sat++) { //86401
                    if (isPointInsidePoly({_tar->second[0], _tar->second[1]}, _sat->second)) {
                        if (flag == true) { //计算恰好能够对目标提供服务的开始时间
                            time_window.push_back(_sat->first); //记录
                            outfile << time_to_date(_sat->first) << "  ";
                            flag = false;
                        }
                    }
                    else { //卫星恰好不能对目标提供服务的时间，前闭后开区间
                        if (flag == false) {
                            time_window.push_back(_sat->first);
                            int temp = time_window[1] - time_window[0]; //时间间隔
                            multi_tW.push_back(time_window); //多重时间窗口
                            outfile << time_to_date(_sat->first) << " " << temp << "s\n";
                            duration.push_back(temp);
                            time_window.clear(); //清空
                            flag = true;
                        }
                    }
                }
                if (!duration.empty()) { //时间窗口存在
                    //用于计算二重时间覆盖窗口
                    coverWindow tmp(sat_it - SatelliteInfo.begin(), _tar->first, multi_tW);
                    cW.push_back(tmp);
                    int max_period = *max_element(duration.begin(), duration.end());
                    max_duration.push_back(max_period);
                    for (auto it : duration) total_duration.push_back(it);
                    double sum = accumulate(duration.begin(), duration.end(), 0.0);
                    double average_period = sum / duration.size();
                    outfile << "时间间隙的最大值：" << max_period << "s，平均值：" << average_period << "s\n";
                    multi_tW.clear();
                    duration.clear();
                }
            }
            vector<vector<int>> res; //交集
            outfile << "二重覆盖时间窗口：" << endl;
            for (auto it1 = cW.begin(); it1 != cW.end(); it1++) {
                for (auto it2 = it1 + 1; it2 != cW.end(); it2++) {
                    res = Intersection(it1->tW, it2->tW);
                    if (!res.empty()) {
                        outfile << "Sat" + to_string(it1->sat) + "&Sat" + to_string(it2->sat) + ":";
                        for (auto it : res)
                            outfile << time_to_date(it[0]) << "  " << time_to_date(it[1]) << endl;
                    }
                }
            }
            double total_sum =  accumulate(total_duration.begin(), total_duration.end(), 0.0);
            outfile << "星座对点目标时间间隔的最大值：" << *max_element(max_duration.begin(), max_duration.end()) << "s，平均值：" << total_sum / total_duration.size() << "s\n";
            max_duration.clear();
            total_duration.clear();
            cW.clear(); //清空
        }
        outfile << endl;
    }
    outfile.close();
}

void calTimeWindowForGannt() {
    ofstream outfile("/Users/gaotianhong/Desktop/Python/卫星对地覆盖计算及任务规划/Data/Gannt/TW_gannt.txt"); //将结果输入到文件
    ofstream outfile1("/Users/gaotianhong/Desktop/Python/卫星对地覆盖计算及任务规划/Data/Gannt/TW_gannt1.txt"); //将结果输入到文件
    if (!outfile) outfile.close();
//    for (auto tar_it = TargetInfo.begin(); tar_it != TargetInfo.end(); tar_it++) { //遍历10个目标数据集
        auto tar_it = TargetInfo.begin();
        vector<coverWindow> cW; //时间覆盖窗口
        for (auto _tar = tar_it->begin(); _tar != tar_it->end(); _tar++) { //每一个目标数据集对应的城市
            for (auto sat_it = SatelliteInfo.begin(); sat_it != SatelliteInfo.end(); sat_it++) { //9个卫星
                size_t num = sat_it - SatelliteInfo.begin();
                bool flag = true;
                vector<int> time_window; //时间窗口
                vector<vector<int>> multi_tW; //多重时间窗口
                vector<int> duration; //时间间隔
                for (auto _sat = sat_it->begin(); _sat != sat_it->end(); _sat++) { //86401
                    if (isPointInsidePoly({_tar->second[0], _tar->second[1]}, _sat->second)) {
                        if (flag == true) { //计算恰好能够对目标提供服务的开始时间
                            outfile << _tar->first << endl; //城市名
                            outfile << "Sat" << num << endl;
                            time_window.push_back(_sat->first); //记录
                            outfile << time_to_date(_sat->first) << endl;
                            flag = false;
                        }
                    }
                    else { //卫星恰好不能对目标提供服务的时间，前闭后开区间
                        if (flag == false) {
                            time_window.push_back(_sat->first);
                            int temp = time_window[1] - time_window[0]; //时间间隔
                            multi_tW.push_back(time_window); //多重时间窗口
                            outfile << time_to_date(_sat->first) << endl;
                            duration.push_back(temp);
                            time_window.clear(); //清空
                            flag = true;
                        }
                    }
                }
                if (!duration.empty()) { //时间窗口存在
                    //用于计算二重时间覆盖窗口
                    coverWindow tmp(sat_it - SatelliteInfo.begin(), _tar->first, multi_tW);
                    cW.push_back(tmp);
                    multi_tW.clear();
                    duration.clear();
                }
            }
            vector<vector<int>> res; //交集
            for (auto it1 = cW.begin(); it1 != cW.end(); it1++) {
                for (auto it2 = it1 + 1; it2 != cW.end(); it2++) {
                    res = Intersection(it1->tW, it2->tW);
                    if (!res.empty()) {
                        for (auto it : res) {
                            outfile1 << _tar->first << "\nSat" << it1->sat << endl;
                            outfile1 << time_to_date(it[0]) << "\n" << time_to_date(it[1]) << endl;
                            outfile1 << _tar->first << "\nSat" << it2->sat << endl;
                            outfile1 << time_to_date(it[0]) << "\n" << time_to_date(it[1]) << endl;
                        }
                    }
                }
            }
            cW.clear(); //清空
        }
        outfile << endl;
//    }
    outfile.close();
}

void calTimeWindowForInterval() {
    ofstream outfile("/Users/gaotianhong/Desktop/Python/卫星对地覆盖计算及任务规划/Data/Gannt/TW_interval.txt"); //将结果输入到文件
    if (!outfile) outfile.close();
//    for (auto tar_it = TargetInfo.begin(); tar_it != TargetInfo.end(); tar_it++) { //遍历10个目标数据集
        auto tar_it = TargetInfo.begin();
        vector<coverWindow> cW; //时间覆盖窗口
        for (auto _tar = tar_it->begin(); _tar != tar_it->end(); _tar++) { //每一个目标数据集对应的城市
            outfile << _tar->first << endl; //城市名
            vector<double> total_duration; //总时间间隔
            double max_duration = 0.0, average_duration = 0.0; //时间间隔的最大值和平均值
            for (auto sat_it = SatelliteInfo.begin(); sat_it != SatelliteInfo.end(); sat_it++) { //9个卫星
                bool flag = true;
                vector<int> time_window; //时间窗口
                vector<vector<int>> multi_tW; //多重时间窗口
                vector<int> duration; //时间间隔
//                outfile << "SatCoverInfo_" + to_string(sat_it - SatelliteInfo.begin()) << endl;
                for (auto _sat = sat_it->begin(); _sat != sat_it->end(); _sat++) { //86401
                    if (isPointInsidePoly({_tar->second[0], _tar->second[1]}, _sat->second)) {
                        if (flag == true) { //计算恰好能够对目标提供服务的开始时间
                            time_window.push_back(_sat->first); //记录
                            flag = false;
                        }
                    }
                    else { //卫星恰好不能对目标提供服务的时间，前闭后开区间
                        if (flag == false) {
                            time_window.push_back(_sat->first);
                            int temp = time_window[1] - time_window[0]; //时间间隔
                            multi_tW.push_back(time_window); //多重时间窗口
//                            outfile << temp << endl;
                            duration.push_back(temp);
                            time_window.clear(); //清空
                            flag = true;
                        }
                    }
                }
                if (!duration.empty()) { //时间窗口存在
                    //用于计算二重时间覆盖窗口
                    coverWindow tmp(sat_it - SatelliteInfo.begin(), _tar->first, multi_tW);
                    cW.push_back(tmp);
                    for (auto it : duration) total_duration.push_back(it); //用于计算总时间间隔
                    int max_period = *max_element(duration.begin(), duration.end());
                    if (max_period > max_duration) max_duration = max_period; //统计最大时间间隔
//                    double sum = accumulate(duration.begin(), duration.end(), 0.0);
//                    double average_period = sum / duration.size();
//                    outfile << "时间间隙的最大值：" << max_period << "s，平均值：" << average_period << "s\n";
                    multi_tW.clear();
                    duration.clear();
                }
            }
            average_duration = accumulate(total_duration.begin(), total_duration.end(), 0.0) / total_duration.size(); //平均时间间隔
            outfile << max_duration << endl;
            outfile << average_duration << endl;
            total_duration.clear();
            cW.clear(); //清空
        }
        outfile << endl;
//    }
    outfile.close();
}

struct Output { //输出类
    int sat; //卫星编号
    string city;
    int start, finish;
    double profit; //收益
    Output(int _sat, string _city, int _start, int _finish, double _profit):sat(_sat), city(_city), start(_start), finish(_finish), profit(_profit){}
};
vector<Output> output, outputRes;

int startTime, finishTime; //仿真周期
int targetNum; //观测目标文件
vector<int> satNum; //卫星编号
void chooseTestData_2() {
    cout << "请输入仿真周期：";
    cin >> startTime >> finishTime;
    cout << "请输入观测目标文件：";
    cin >> targetNum;
    targetNum -= 1;
    cout << "请输入需要选择的卫星资源（-1代表输入结束）：";
    int num = 0;
    while (num != -1) { //-1代表输入结束
        cin >> num;
        if (num != -1) satNum.push_back(num);
    }
} //选择仿真周期，观测目标文件，可选卫星资源

struct GreedyInfo { //城市信息
    int sat; //卫星编号
    string city; //城市名
    int start, finish; //开始时间，结束时间
    double needTime, profit; //需要观测时间，收益
    GreedyInfo(int _sat, string _city, vector<int> timeWindow, double _needTime, double _profit) {
        sat = _sat;
        city = _city;
        start = timeWindow[0]; finish = timeWindow[1];
        needTime = _needTime; profit = _profit;
    }
    friend bool operator<(GreedyInfo a, GreedyInfo b) {
        return a.start < b.start; //先来先服务
    }
};
bool cmp(GreedyInfo a, GreedyInfo b) {
    return a.profit / a.needTime > b.profit / b.needTime; //按单位时间收益由大到小排序
}
bool cmp_n(GreedyInfo a, GreedyInfo b) {
    return a.needTime < b.needTime; //按观测时间由小到大排序
}
//单颗卫星收益最大，每一颗卫星记录它所能观测到的城市的时间窗口，每一个城市只能在第一次观测到才可以得分
//贪心策略：1.优先观测单位时间收益大的城市；2.先来先服务
//对于每一个卫星，优先观测单位时间收益大的城市，观测完成后，该城市设置一标志代表已观测，对于该卫星的时间片[0,86401)去除该时间窗口
//所有卫星能够观测到的城市一块考虑，有两种贪心策略，第一种是优先观测单位时间收益大的城市，第二种策略是按每颗卫星可以观测到的时间先后顺序一次排序，优先观测所能观测到的并且不会冲突的城市。还有一种策略是按所需时间的大小进行排序，优先观测时间少的，因为这种策略效果不太好，就放弃了。
//在时间片的选择上，对每个城市所能观测到的时间窗口，两层循环，尽量去观测。
void GreedyDispatch() { //贪心法设计星座对地面目标调度规划
    clock_t start, finish;
    start = clock();
    ofstream outfile("/Users/gaotianhong/Desktop/Python/卫星对地覆盖计算及任务规划/Data/Dispatch/GreedyTest" + to_string(targetNum + 1) + ".txt"); //将结果输入到文件
    ofstream outfile1("/Users/gaotianhong/Desktop/Python/卫星对地覆盖计算及任务规划/Data/Dispatch/GreedyGannt" + to_string(targetNum + 1) + ".txt"); //用于绘制甘特图
    vector<GreedyInfo> satTW; //卫星观测城市对应的时间窗口
    map<string, bool> visit; //城市是否观测过
    vector<bool> v;
    map<int, vector<bool>> sign; //卫星对应的时间片
    for (int i = 0; i < 86401; i++) v.push_back(true);
    for (auto sat_num : satNum) { //用于计算每一颗卫星观测每一个城市的时间窗口
        auto sat_it = SatelliteInfo.begin() + sat_num;
        auto tar_it = TargetInfo.begin() + targetNum; //选择一个指定的数据集
        for (auto _tar = tar_it->begin(); _tar != tar_it->end(); _tar++) { //对应的城市
            visit[_tar->first] = false; //初始化
            bool flag = true; //用于标志能观测到的起止时间
            vector<int> time_window; //时间窗口
            for (auto _sat = sat_it->begin(); _sat != sat_it->end(); _sat++) { //86401s
                if (_sat->first < startTime || _sat->first > finishTime) continue;
                if (isPointInsidePoly({_tar->second[0], _tar->second[1]}, _sat->second)) {
                    if (flag == true) { //计算恰好能够对目标提供服务的开始时间
                        time_window.push_back(_sat->first); //开始时间
                        flag = false;
                    }
                }
                else { //卫星恰好不能对目标提供服务的时间
                    if (flag == false) {
                        time_window.push_back(_sat->first); //结束时间
                        satTW.push_back({sat_num, _tar->first, time_window, _tar->second[2], _tar->second[3]});
                        flag = true;
                        time_window.clear(); //清空
                    }
                }
            }
        }
    }
    double maxProfit = 0.0;
    for (int k = 0; k < 3; k++) { //两种策略进行比较，选择收益大的策略输出
        double totalProfit = 0.0;
        if (k == 0) sort(satTW.begin(), satTW.end()); //先来先服务
        if (k == 1) sort(satTW.begin(), satTW.end(), cmp); //单位时间收益由大到小进行排序
        if (k == 2) sort(satTW.begin(), satTW.end(), cmp_n); //按观测时间由小到大进行排序
        for (auto sat_num : satNum) sign[sat_num] = v; //初始化时间窗口
        for (auto _sat = satTW.begin(); _sat != satTW.end(); _sat++) { //每一颗卫星所能观测到的城市
            if (visit[_sat->city]) continue; //该城市已经访问过，遍历下一个城市
            for (int i = _sat->start; i <= _sat->finish - _sat->needTime - TransTime[_sat->sat]; i++) {
                int tmp = i + _sat->needTime + TransTime[_sat->sat];
                for (int j = i; j <= tmp; j++) {
                    if (sign[_sat->sat][j] == false) break; //该时间片不能用了
                    if (j == tmp && sign[_sat->sat][j] && visit[_sat->city] != true) {
                        for (int k = i; k <= tmp; k++) sign[_sat->sat][k] = false; //用掉该时间片
                        visit[_sat->city] = true;
                        totalProfit += _sat->profit; //总收益
                        output.push_back({_sat->sat, _sat->city, i, j - TransTime[_sat->sat], _sat->profit});
                    }
                }
                if (visit[_sat->city]) break;
            }
        }
//        cout << "选择城市数量：" << output.size() << " 总收益：" << totalProfit  << endl;
        for (auto &it : visit) it.second = false;
        if (maxProfit < totalProfit) { //选择收益最大的进行输出
            maxProfit = totalProfit;
            outputRes = output;
        }
        output.clear();
    }
    for (auto res : outputRes) {
        outfile << "Sat" <<  res.sat << "观测" << res.city << " 获得收益：" << res.profit << " 时间窗口：" << time_to_date(res.start) << " " << time_to_date(res.finish) <<endl;
        outfile1 << res.city << "\nSat" << res.sat << "\n" << time_to_date(res.start) << "\n" << time_to_date(res.finish) <<  endl;
    }
    outfile << "选择的城市总数量：" << outputRes.size() << " 获得总收益：" << maxProfit << endl;
    outputRes.clear();
    outfile.close();
    outfile1.close();
    finish = clock();
    cout << "usr time:" << double(finish - start) / CLOCKS_PER_SEC << "s" << endl;
}

struct IntegerInfo { //用于对同一颗卫星所能观测到的时间窗口的开始时间进行排序
    string city;
    int start, finish; //时间窗口的开始时间，结束时间
    int end; //实际完成时间
    double needTime, profit; //所需观测时间，收益
    IntegerInfo(string _city, double _needTime, double _profit, vector<int> _timeWindow):city(_city), needTime(_needTime), profit(_profit) {
        start = _timeWindow[0];
        finish = _timeWindow[1];
    }
    IntegerInfo(string _city, int _start, int _finish, int _end, double _needTime, double _profit) {
        city = _city;
        start = _start;
        finish = _finish;
        end = _end;
        needTime = _needTime;
        profit = _profit;
    }
    friend bool operator<(IntegerInfo c1, IntegerInfo c2) { //按观测时间先后排序
        return c1.start < c2.start;
    }
};
//该问题表述为非固定整数线性规划模型，其中约束是从仔细分析可行时间间隔之间的相互依赖关系导出的
//增加约束，对每一颗卫星，考虑时间片的安放问题，安排尽可能多的城市进行观测，减少碎片化时间片的数量；最好的情况是观察完这一个目标，紧接着可以观察另一个目标，尽可能使时间片变得连续；同时在时间选择方面，下一个时间片开始之前，优先选择时间较短的时间片。
//对每一颗卫星单独进行考虑，对每颗卫星所能观测到的城市，按时间片的先后顺序进行排序，首先将能观测到的所有城市赋予时间窗口的开始的时间片，然后增加约束，查看时间片的冲突情况。依次遍历每一个城市，如果第一个城市和第二个城市的时间片发生冲突，则移动第二个城市的时间片，直至刚好不发生冲突，直到遍历到最后一个城市为止。
void IntegerPlanning() { //整数规划法设计星座对地面目标调度规划
    clock_t start, finish;
    start = clock();
    ofstream outfile("/Users/gaotianhong/Desktop/Python/卫星对地覆盖计算及任务规划/Data/Dispatch/IntegerTest" + to_string(targetNum + 1) + ".txt"); //将结果输入到文件
    ofstream outfile1("/Users/gaotianhong/Desktop/Python/卫星对地覆盖计算及任务规划/Data/Dispatch/IntegerGannt" + to_string(targetNum + 1) + ".txt"); //将结果输入到文件
    bool initData = true;
    map<string, bool> IsOutput; //去除每颗卫星重复访问过的城市
    map<string, bool> visit; //判断某城市是否被访问过
    vector<IntegerInfo> IInfo; //城市信息
    int totalCityNum = 0; //选择的城市数量
    double totalProfit = 0.0; //总收益值
    for (auto sat_num : satNum) { //选择卫星
        auto sat_it = SatelliteInfo.begin() + sat_num;
        outfile << "Sat" + to_string(sat_it - SatelliteInfo.begin()) << endl;
        auto tar_it = TargetInfo.begin() + targetNum; //选择一个指定的数据集
        for (auto _tar = tar_it->begin(); _tar != tar_it->end(); _tar++) { //对应的城市
            if (initData) {
                visit[_tar->first] = false; //初始化
                IsOutput[_tar->first] = false; //初始化
            }
            bool flag = true; //用于标志能观测到的起止时间
            vector<int> time_window; //时间窗口
            for (auto _sat = sat_it->begin(); _sat != sat_it->end(); _sat++) { //86401s
                if (_sat->first < startTime || _sat->first > finishTime) continue;
                if (isPointInsidePoly({_tar->second[0], _tar->second[1]}, _sat->second)) {
                    if (flag == true) { //计算恰好能够对目标提供服务的开始时间
                        time_window.push_back(_sat->first); //开始时间
                        flag = false;
                    }
                }
                else { //卫星恰好不能对目标提供服务的时间
                    if (flag == false) {
                        time_window.push_back(_sat->first); //结束时间
                        flag = true;
                        IInfo.push_back({_tar->first, _tar->second[2], _tar->second[3], time_window});
                        time_window.clear(); //清空
                    }
                }
            }
        }
        initData = false;
        sort(IInfo.begin(), IInfo.end()); //排序后同样转换为时间片选择问题，使得时间片的冲突最小
        vector<IntegerInfo> cityTW; //记录城市所使用的时间片
        //计算每颗卫星应选择观测的城市，首先全部安排在第一个时间片，如果前一个与后一个时间片发生冲突，则进行移动时间片
        for (auto p = IInfo.begin(); p != IInfo.end(); p++) {
            int tmp = p->start + p->needTime + TransTime[sat_num]; //tmp为实际完成时间
            visit[p->city] = true;
            if (!IsOutput[p->city]) { //避免重复观测某个城市
                cityTW.push_back({p->city, p->start, p->finish, tmp, p->needTime, p->profit});
            }
        }
        sort(cityTW.begin(), cityTW.end()); //时间片先后顺序进行选择
        for (auto it1 = cityTW.begin(); it1 != cityTW.end(); it1++) {
            vector<IntegerInfo>::iterator it = it1; //迭代器
            while (!visit[it->city]) { //该城市无法访问，选择前一个城市进行判断
                it--;
            }
            auto it2 = it1 + 1;
            if (it2 == cityTW.end()) break;
            if (it->end >= it2->start) { //时间片冲突，移动第二个时间片
                int tmp1 = it->end + 1 + it2->needTime + TransTime[sat_num];
                if (tmp1 <= it2->finish) { //可移动
                    it2->start = it->end + 1;
                    it2->end = tmp1;
                }
                else { //无法移动该时间片
                    visit[it2->city] = false; //该城市无法访问
                }
            }
        }
        int selectCityNum = 0;
        double _satProfit = 0.0;
        for (auto res : cityTW) {
            if (!visit[res.city]) continue; //该城市未能成功观测
            IsOutput[res.city] = true;
            selectCityNum++;
            _satProfit += res.profit;
            outfile << "观测" << res.city << " 获得收益：" << res.profit << " 时间窗口：" << time_to_date(res.start) << " " << time_to_date(res.end - TransTime[sat_num]) <<endl;
            outfile1 << res.city << "\n" << time_to_date(res.start) << "\n" << time_to_date(res.end - TransTime[sat_num]) << endl;
        }
        totalProfit += _satProfit; //总收益
        totalCityNum += selectCityNum; //总数量
        cityTW.clear();
        IInfo.clear(); //遍历完一颗卫星之后清空
        outfile << "选择的城市数量：" << selectCityNum << " 总收益：" << _satProfit << endl;
    }
    outfile <<"选择的城市总数量："<< totalCityNum << " 总收益：" << totalProfit << endl;
    outfile.close();
    outfile1.close();
    finish = clock();
    cout << "usr time:" << double(finish - start) / CLOCKS_PER_SEC << "s" << endl;
}

struct HeurInfo {
    int sat; //卫星编号
    string city; //城市名
    int start, finish;
    double needTime, profit;
    HeurInfo(int _sat, string _city, vector<int> timeWindow, double _needTime, double _profit) {
        sat = _sat;
        city = _city;
        start = timeWindow[0]; finish = timeWindow[1];
        needTime = _needTime; profit = _profit;
    }
    friend bool operator<(HeurInfo a, HeurInfo b) { //按单位时间收益由小到大排序
        return a.profit / a.needTime > b.profit / b.needTime;
    }
};
bool cmp1(HeurInfo a, HeurInfo b) {
    return a.profit / a.needTime < b.profit / b.needTime;
}
//通过插入或删除新的任务，然后按照从低到高的优先级反复插入新的任务
//直接插入新的动态任务或通过重复删除插入新的动态任务，称为IDI算法。所有新任务首先按照优先级从低到高插入到等待队列中。请注意，新任务是按照优先级从低到高排序的
//启发式算法我是根据论文中的IDI算法进行一些适当的改进，将其与粒子群算法进行结合。IDI算法的基本思想是，我将所有卫星能够观测到的城市一块考虑，按照单位时间收益由小到大的顺序进行排序。依次安排能够观测到的城市，如果下一个城市无法安排，则以一定的概率删除优先级最低的城市，对于这一个算法来说，就是单位时间收益最小的城市，其中删除城市的概率是动态变化的，随着迭代的进行，优先级较高的城市删除的概率会逐渐变小。将这整个过程进行迭代，保存每一次迭代过程的结果，最后选择总收益最好的一次结果进行输出，类似于粒子群算法的思想。
void HeuristicScheduling() { //启发式算法设计星座对地面目标调度规划
    clock_t start, finish;
    start = clock();
    ofstream outfile("/Users/gaotianhong/Desktop/Python/卫星对地覆盖计算及任务规划/Data/Dispatch/HeuristicTest" + to_string(targetNum + 1) + ".txt"); //将结果输入到文件
    ofstream outfile1("/Users/gaotianhong/Desktop/Python/卫星对地覆盖计算及任务规划/Data/Dispatch/HeuristicGannt" + to_string(targetNum + 1) + ".txt"); //将结果输入到文件
    vector<HeurInfo> satTW;
    priority_queue<HeurInfo, vector<HeurInfo>> pq; //优先级队列用于删除策略
    map<string, bool> visit; //用于记录某个城市是否被观测过，便于删除策略
    vector<bool> v;
    map<int, vector<bool>> sign; //卫星对应的时间片
    for (int i = 0; i < 86401; i++) v.push_back(true);
    for (auto sat_num : satNum) { //用于计算每一颗卫星观测每一个城市的时间窗口
        auto sat_it = SatelliteInfo.begin() + sat_num;
        auto tar_it = TargetInfo.begin() + targetNum; //选择一个指定的数据集
        for (auto _tar = tar_it->begin(); _tar != tar_it->end(); _tar++) { //对应的城市
            bool flag = true; //用于标志能观测到的起止时间
            vector<int> time_window; //时间窗口
            for (auto _sat = sat_it->begin(); _sat != sat_it->end(); _sat++) { //86401s
                if (_sat->first < startTime || _sat->first > finishTime) continue;
                if (isPointInsidePoly({_tar->second[0], _tar->second[1]}, _sat->second)) {
                    if (flag == true) { //计算恰好能够对目标提供服务的开始时间
                        time_window.push_back(_sat->first); //开始时间
                        flag = false;
                    }
                }
                else { //卫星恰好不能对目标提供服务的时间
                    if (flag == false) {
                        time_window.push_back(_sat->first); //结束时间
                        satTW.push_back({sat_num, _tar->first, time_window, _tar->second[2], _tar->second[3]});
                        flag = true;
                        time_window.clear(); //清空
                    }
                }
            }
        }
    }
    double maxProfit = 0.0;
    sort(satTW.begin(), satTW.end(), cmp1); //单位时间收益由小到大排序
    int times = 30;
    if (targetNum == 1) times = 1000;
    if (targetNum == 10) times = 3;
    for (int c = 0; c < times; c++) { //迭代次数
        int cnt = 0;
        for (auto _sat : satTW) visit[_sat.city] = false; //迭代开始之前，初始化
        for (auto sat_num : satNum) sign[sat_num] = v; //初始化时间窗口
        for (auto _sat : satTW) { //每一颗卫星所能观测到的城市
            cnt++;
            if (visit[_sat.city]) continue; //该城市已经访问过，遍历一下个城市
            for (int i = _sat.start; i <= _sat.finish - _sat.needTime - TransTime[_sat.sat]; i++) {
                int tmp = i + _sat.needTime + TransTime[_sat.sat];
                for (int j = i; j <= tmp; j++) {
                    if (sign[_sat.sat][j] == false) { //该时间片不能用了
                        double r = rand() % 101; //选择一定的概率删除某个城市
                        double pro = 50 + (double)cnt / satTW.size() * 50.0;
                        if (r > pro) break; //pro动态变化，随着迭代的进行，单位收益较大的时间片删除的概率较小
                        bool arranged = false;
                        auto delCity = pq.top(); //预删除
                        pq.pop();
                        for (int k = delCity.start; k <= delCity.finish; k++)
                            sign[_sat.sat][k] = true;
                        visit[delCity.city] = false;
                        for (int m = i; m <= i + _sat.needTime + TransTime[_sat.sat]; m++) {
                            if (sign[_sat.sat][m] == false) break;
                            if (m == i + _sat.needTime + TransTime[_sat.sat] && sign[_sat.sat][m] && !visit[_sat.city]) {
                                for (int k = i; k <= m; k++) {
                                    sign[_sat.sat][k] = false; //分配时间片
                                }
                                arranged = true;
                                visit[_sat.city] = true;
                                output.push_back({_sat.sat, _sat.city, i, m - TransTime[_sat.sat], _sat.profit});
                                pq.push({_sat.sat, _sat.city, {i, m}, _sat.needTime, _sat.profit});
                            }
                        }
                        if (!arranged) { //取消删除
                            for (int k = delCity.start; k <= delCity.finish; k++) {
                                sign[_sat.sat][k] = false;
                            }
                            visit[delCity.city] = true;
                            pq.push(delCity);
                        }
                    }
                    if (j == tmp && sign[_sat.sat][j] && visit[_sat.city] != true) {
                        for (int k = i; k <= tmp; k++) sign[_sat.sat][k] = false; //用掉该时间片
                        visit[_sat.city] = true;
                        vector<int> tw = {i, tmp};
                        output.push_back({_sat.sat, _sat.city, i, j - TransTime[_sat.sat], _sat.profit});
                        pq.push({_sat.sat, _sat.city, tw, _sat.needTime, _sat.profit});
                    }
                }
                if (visit[_sat.city]) break;
            }
        }
        double totalProfit = 0.0;
        for (auto it : output) {
            if (!visit[it.city]) continue; //该城市访问标记位为false
            totalProfit += it.profit;
        }
        if (maxProfit < totalProfit) {
            maxProfit = totalProfit;
            outputRes = output;
        }
        output.clear();
    }
    int n = 0; //城市数量
    for (auto res : outputRes) {
        if (!visit[res.city]) continue;
        n++;
        outfile << "Sat" <<  res.sat << "观测" << res.city << " 获得收益：" << res.profit << " 时间窗口：" << time_to_date(res.start) << " " << time_to_date(res.finish) <<endl;
        outfile1 << res.city << "\n" << time_to_date(res.start) << "\n" << time_to_date(res.finish) << endl;
    }
    outfile <<"选择的城市总数量："<< n << " 总收益：" << maxProfit << endl;
    outputRes.clear();
    outfile.close();
    outfile1.close();
    finish = clock();
    cout << "usr time:" << double(finish - start) / CLOCKS_PER_SEC << "s" << endl;
}


struct SatMemory {
    int num; //卫星编号
    int memory; //容量
    SatMemory(int _num, int _memory):num(_num), memory(_memory){}
};
vector<SatMemory> Sat; //卫星信息
void chooseTestData_3() {
    cout << "请输入仿真周期：";
    cin >> startTime >> finishTime;
    cout << "请输入观测目标文件：";
    cin >> targetNum;
    targetNum -= 1;
    cout << "请输入需要选择的卫星编号及存储容量（-1代表输入结束）：";
    int _num = 0, _memory = 0;
    while (_num != -1) { //-1代表输入结束
        cin >> _num >> _memory;
        if (_num != -1) Sat.push_back({_num, _memory});
    }
} //选择仿真周期，观测目标文件，可选卫星资源

struct DataStoreInfo {
    string city;
    int start, finish, needTime; //开始时间，结束时间，需要观测的时间
    int end; //实际完成时间
    double profit; //收益
    DataStoreInfo() = default;
    DataStoreInfo(string _city, vector<int> _timeWindow, double _needTime, double _profit):city(_city), needTime(_needTime), profit(_profit) {
        start = _timeWindow[0];
        finish = _timeWindow[1];
    }
    DataStoreInfo(string _city, int _start, int _finish, int _end, double _needTime, double _profit) {
        city = _city;
        start = _start;
        finish = _finish;
        end = _end;
        needTime = _needTime;
        profit = _profit;
    }
    friend bool operator<(DataStoreInfo a, DataStoreInfo b) {
        return a.start < b.start;
    }
};
//星载存储器，1MB/增加，增加约束条件
//动态规划（非线性规划算法），约束条件：星座容量限制，选择时间窗口，使得存储器的观测不会超过其容量，星座对目标观测得分最大
//1.对单颗卫星来说，尽可能使单颗卫星达到最大存储容量，从而让每一颗卫星都能达到最大存储容量（动态规划）2.单星占用率尽可能平均（贪心）
//我采用动态规划的思想是，仿照背包问题的模型，对每颗卫星所能够观测到的城市数n和总容量C构建一个二维数组，有如下关系式：
//P[i,c] = max{P[i-1,c-vi] + pi, P[i-1,c]} 表示从第1个城市到第i个城市进行选择，卫星剩余容量为C的最优解，将选择的结果保存下来，再采用整数规划法的思想进行选择，首先将每个城市所能观测到的时间窗口按开始时间的先后顺序进行排序，全部安排，再根据时间片的冲突情况进行移动时间片，直至遍历到最后一个城市，即选择尽可能多的城市进行观测。
int P[1500][9001], Rec[1500][9001]; //避免二维数组的溢出
void dpDataStore() { //动态规划算法解决数据存储问题
    clock_t start, finish;
    start = clock();
    ofstream outfile("/Users/gaotianhong/Desktop/Python/卫星对地覆盖计算及任务规划/Data/DataStore/dpTest" + to_string(targetNum + 1) + ".txt"); //将结果输入到文件
    ofstream outfile1("/Users/gaotianhong/Desktop/Python/卫星对地覆盖计算及任务规划/Data/DataStore/dpGannt" + to_string(targetNum + 1) + ".txt"); //将结果输入到文件
    map<string, bool> visit; //判断某城市是否被访问过
    map<string, bool> IsPrint; //避免重复城市
    vector<DataStoreInfo> DSInfo;
    bool initData = true;
    int totalCityNum = 0; //选择的城市数量
    double totalProfit = 0.0; //总收益值
    for (auto sat : Sat) { //选择卫星
        auto sat_it = SatelliteInfo.begin() + sat.num;
        outfile << "Sat" + to_string(sat_it - SatelliteInfo.begin()) << endl;
        auto tar_it = TargetInfo.begin() + targetNum; //选择一个指定的数据集
        for (auto _tar = tar_it->begin(); _tar != tar_it->end(); _tar++) { //对应的城市
            if (initData) {
                visit[_tar->first] = false; //初始化城市
                IsPrint[_tar->first] = false;
            }
            bool flag = true; //用于标志能观测到的起止时间
            vector<int> time_window; //时间窗口
            for (auto _sat = sat_it->begin(); _sat != sat_it->end(); _sat++) { //86401s
                if (_sat->first < startTime || _sat->first > finishTime) continue;
                if (isPointInsidePoly({_tar->second[0], _tar->second[1]}, _sat->second)) {
                    if (flag == true) { //计算恰好能够对目标提供服务的开始时间
                        time_window.push_back(_sat->first); //开始时间
                        flag = false;
                    }
                }
                else { //卫星恰好不能对目标提供服务的时间
                    if (flag == false) {
                        time_window.push_back(_sat->first); //结束时间
                        DSInfo.push_back({_tar->first, time_window, _tar->second[2], _tar->second[3]});
                        flag = true;
                        time_window.clear(); //清空
                    }
                }
            }
        }
        vector<DataStoreInfo> DSRes; //用于存放动态规划选择城市的结果
        initData = false;
        //首先在不考虑时间片冲突的情况下，利用动态规划方法选择城市
        //P[i,c] = max{P[i-1,c-vi] + pi, P[i-1,c]} 表示从第1个城市到第i个城市进行选择，卫星剩余容量为c的最优解
        size_t n = DSInfo.size(); //某颗卫星所能观测到的城市数量
        DataStoreInfo dsInfo[n+1]; //统一计量单位
        for (size_t i = 0; i < n; i++) {
            dsInfo[i+1].profit = DSInfo[i].profit;
            dsInfo[i+1].needTime = DSInfo[i].needTime;
        }
        int C = sat.memory; //该颗卫星的容量
//        int P[n+1][C+1], Rec[n+1][C+1]; //Rec[i][c]用于追踪最优解 二维数组过大会产生溢出情况
        for (int i = 0; i <= C; i++) P[0][i] = 0;
        for (int i = 0; i <= n; i++) P[i][0] = 0; //初始化
        //求解表格 选择城市
        for (int i = 1; i <= n; i++) {
            for (int c = 1; c <= C; c++) {
                if (dsInfo[i].needTime <= c && dsInfo[i].profit + P[i-1][c-dsInfo[i].needTime] > P[i-1][c]) {
                    //选择该城市
                    P[i][c] = dsInfo[i].profit + P[i-1][c-dsInfo[i].needTime];
                    Rec[i][c] = 1;
                }
                else {
                    P[i][c] = P[i-1][c];
                    Rec[i][c] = 0;
                }
            }
        }
        int K = C;
        for (size_t i = n; i >= 1; i--) { //倒序判断是否选择该城市
            if (Rec[i][K] == 1) {
                DSRes.push_back(DSInfo[i-1]); //记录动态规划的结果
                K = K - DSInfo[i-1].needTime;
            }
        }
        sort(DSRes.begin(), DSRes.end());
        vector<DataStoreInfo> cityTW; //记录城市所使用的时间片
        for (auto p = DSRes.begin(); p != DSRes.end(); p++) {
            int tmp = p->start + p->needTime + TransTime[sat.num]; //tmp为实际完成时间
            visit[p->city] = true;
            if (!IsPrint[p->city]) {
                cityTW.push_back({p->city, p->start, p->finish, tmp, (double)p->needTime, p->profit});
            }
        }
        sort(cityTW.begin(), cityTW.end()); //时间片先后顺序进行选择
        for (auto it1 = cityTW.begin(); it1 != cityTW.end(); it1++) {
            vector<DataStoreInfo>::iterator it = it1; //迭代器
            while (!visit[it->city]) { //该城市无法访问，选择前一个城市进行判断
                it--;
            }
            auto it2 = it1 + 1;
            if (it2 == cityTW.end()) break;
            if (it->end >= it2->start) { //时间片冲突，移动第二个时间片
                int tmp1 = it->end + 1 + it2->needTime + TransTime[sat.num];
                if (tmp1 <= it2->finish) { //可移动
                    it2->start = it->end + 1;
                    it2->end = tmp1;
                }
                else { //无法移动该时间片
                    visit[it2->city] = false; //该城市无法访问
                }
            }
        }
        int selectCityNum = 0, capacity = 0;
        double _satProfit = 0.0;
        for (auto res : cityTW) {
            if (!visit[res.city]) continue; //该城市未能成功观测
            IsPrint[res.city] = true;
            selectCityNum++;
            _satProfit += res.profit;
            capacity += res.needTime;
            outfile << "观测" << res.city << " 获得收益：" << res.profit << " 时间窗口：" << time_to_date(res.start) << " " << time_to_date(res.end - TransTime[sat.num]) <<endl;
            outfile1 << res.city << "\nSat" << sat.num << "\n" << time_to_date(res.start) << "\n" << time_to_date(res.end - TransTime[sat.num]) << endl;
        }
        totalProfit += _satProfit; //总收益
        totalCityNum += selectCityNum; //总数量
        DSInfo.clear();
        DSRes.clear(); //动态规划的结果
        outfile << "Sat" + to_string(sat_it - SatelliteInfo.begin()) << "消耗容量：" << capacity << endl;
        outfile << "选择的城市数量：" << selectCityNum << " 总收益：" << _satProfit << endl;
    }
    outfile <<"选择的城市总数量："<< totalCityNum << " 总收益：" << totalProfit << endl;
    outfile.close();
    outfile1.close();
    finish = clock();
    cout << "usr time:" << double(finish - start) / CLOCKS_PER_SEC << "s" << endl;
}

bool cmp2(DataStoreInfo a, DataStoreInfo b) { //按照单位时间收益由大到小排序
    return a.profit / a.needTime > b.profit / b.needTime;
}
//使得单星占用率尽可能平均，在不超过卫星的存储容量的情况下，贪心算法：选择观测时间短的，单位时间收益大的，可参考第二问的第三种算法
//借鉴了贪心算法和智能优化算法的思想，每颗卫星单独考虑，在不超过卫星容量的情况下（如果该选择导致当前容量超过了卫星的容量则选择下一个城市进行判断），按每个城市单位时间收益由大到小进行排序，同样的设置一个选择的概率，优先级大的城市，对于该算法来说，就是单位时间收益大的城市，选择的概率较高，这个概率随着迭代的进行，选择城市的概率逐渐变小。对于这个过程，迭代多次，保存每次迭代过程的中间结果，比较每一次的总收益值，选择收益值最大的一次结果进行输出。
void heuristicDataStore() { //启发式算法
    clock_t start, finish;
    start = clock();
    ofstream outfile("/Users/gaotianhong/Desktop/Python/卫星对地覆盖计算及任务规划/Data/DataStore/heurTest" + to_string(targetNum + 1) + ".txt"); //将结果输入到文件
    ofstream outfile1("/Users/gaotianhong/Desktop/Python/卫星对地覆盖计算及任务规划/Data/DataStore/heurGannt" + to_string(targetNum + 1) + ".txt"); //将结果输入到文件
    map<string, bool> visit;
    map<string, bool> visited; //每颗卫星已经选定的观测城市
    vector<DataStoreInfo> DSInfo;
    bool initData = false;
    int totalCityNum = 0;
    double totalProfit = 0.0;
    for (auto sat : Sat) { //选择卫星，每颗卫星单独考虑
        auto sat_it = SatelliteInfo.begin() + sat.num;
        outfile << "Sat" + to_string(sat_it - SatelliteInfo.begin()) << endl;
        auto tar_it = TargetInfo.begin() + targetNum; //选择一个指定的数据集
        for (auto _tar = tar_it->begin(); _tar != tar_it->end(); _tar++) { //对应的城市
            if (initData) {
                visit[_tar->first] = false; //初始化未访问过
                visited[_tar->first] = false;
            }
            bool flag = true; //用于标志能观测到的起止时间
            vector<int> time_window; //时间窗口
            for (auto _sat = sat_it->begin(); _sat != sat_it->end(); _sat++) { //86401s
                if (_sat->first < startTime || _sat->first > finishTime) continue;
                if (isPointInsidePoly({_tar->second[0], _tar->second[1]}, _sat->second)) {
                    if (flag == true) { //计算恰好能够对目标提供服务的开始时间
                        time_window.push_back(_sat->first); //开始时间
                        flag = false;
                    }
                }
                else { //卫星恰好不能对目标提供服务的时间
                    if (flag == false) {
                        time_window.push_back(_sat->first); //结束时间
                        DSInfo.push_back({_tar->first, time_window, _tar->second[2], _tar->second[3]});
                        flag = true;
                        time_window.clear(); //清空
                    }
                }
            }
        }
        sort(DSInfo.begin(), DSInfo.end(), cmp2);
        vector<DataStoreInfo> MAXDSRes;
        double maxProfit = 0.0;
        int times = 200;
        if (targetNum == 1) times = 3000;
        if (targetNum == 10) times = 10;
        for (int c = 0; c < times; c++) {
            bool sign[86401];
            int curMemory = 0; //该卫星的当前消耗容量
            double Profit = 0.0;
            for (int k = 0; k < 86401; k++) sign[k] = true; //该卫星对应的时间片
            vector<DataStoreInfo> DSRes; //输出结果
            int cnt = 0;
            for (auto p : DSInfo) { //遍历所观测的城市
                if (visit[p.city]) continue; //该城市已经访问过
                cnt++;
                double r = rand() % 101;
                double pro = 100 - (double)cnt / DSInfo.size() * 50.0;
                if (r > pro) continue; //pro动态变化，随着迭代的进行，单位收益较大的时间片删除的概率较小
                curMemory += p.needTime;
                if (curMemory > sat.memory) { //超过其容量，无法观测
                    curMemory -= p.needTime;
                    continue;
                }
                for (int i = p.start; i <= p.finish - p.needTime - TransTime[sat.num]; i++) {
                    int tmp = i + p.needTime + TransTime[sat.num];
                    for (int j = i; j <= tmp; j++) {
                        if (sign[j] == false) break; //该时间片不能用了
                        if (j == tmp && sign[j] && visit[p.city] != true) {
                            for (int k = i; k <= tmp; k++) sign[k] = false; //用掉该时间片
                            visit[p.city] = true;
                            DSRes.push_back({p.city, i, p.finish, j, (double)p.needTime, p.profit});
                        }
                    }
                    if (visit[p.city]) break;
                }
                if (!visit[p.city]) curMemory -= p.needTime; //该城市未安排
            }
            for (auto its : DSRes) {
                Profit += its.profit;
            }
            if (maxProfit < Profit) {
                maxProfit = Profit;
                MAXDSRes = DSRes;
            }
            DSRes.clear();
            for (auto it = visit.begin(); it != visit.end(); it++) { //恢复，以便进行下一次迭代
                if (it->second == true && visited[it->first] != true)
                    it->second = false;
            }
        }
        int memory = 0;
        double _satProfit = 0.0;
        for (auto res : MAXDSRes) {
            _satProfit += res.profit;
            memory += res.needTime;
            visit[res.city] = true;
            visited[res.city] = true; //全局访问的城市
            outfile << "观测" << res.city << " 获得收益：" << res.profit << " 时间窗口：" << time_to_date(res.start) << " " << time_to_date(res.end - TransTime[sat.num]) <<endl;
            outfile1 << res.city << "\n" << time_to_date(res.start) << "\n" << time_to_date(res.end - TransTime[sat.num]) << endl;
        }
        totalCityNum += MAXDSRes.size();
        totalProfit += _satProfit; //总收益
        DSInfo.clear();
        outfile << "Sat" + to_string(sat_it - SatelliteInfo.begin()) << "消耗容量：" << memory << endl;
        outfile << "选择的城市数量：" << MAXDSRes.size() << " 总收益：" << _satProfit << endl;
        MAXDSRes.clear();
    }
    outfile <<"选择的城市总数量："<< totalCityNum << " 总收益：" << totalProfit << endl;
    outfile.close();
    outfile1.close();
    finish = clock();
    cout << "usr time:" << double(finish - start) / CLOCKS_PER_SEC << "s" << endl;
}


void TimeWindow() {
    cout << "Calculating...Please wait for a moment!" << endl;
//    calTimeWindow(); //计算时间窗口
    calTimeWindowForGannt();
//    calTimeWindowForInterval();
}
void Dispatch() {
    chooseTestData_2();
    bool quit = false;
    while (!quit) {
        int choice;
        cout << "请选择所需要的算法：[1]贪心法 [2]整数规划法 [3]启发式算法 [0]退出" << endl;
        cout << "请选择（输入数字）：";
        cin >> choice;
        if (choice != 0) cout << "Calculating...Please wait for a moment!" << endl;
        switch (choice) {
            case 1: {GreedyDispatch(); break;} //贪心法设计星座对地面目标调度规划算法
            case 2: {IntegerPlanning(); break;}//整数规划法设计星座对地面目标调度规划
            case 3: {HeuristicScheduling(); break;}//启发式算法设计星座对地面目标调度规划
            case 0: {satNum.clear(); return;}
            default: {cout << "输入出现问题，请重新输入!\n\n"; break;}
        }
        cout << endl;
    }
}
void DataStore() {
    chooseTestData_3();
    bool quit = false;
    while (!quit) {
        int choice;
        cout << "请选择所需要的算法：[1]动态规划 [2]启发式算法 [0]退出" << endl;
        cout << "请选择（输入数字）：";
        cin >> choice;
        if (choice != 0) cout << "Calculating...Please wait for a moment!" << endl;
        switch (choice) {
            case 1: {dpDataStore(); break;} //动态规划
            case 2: {heuristicDataStore(); break;} //启发式算法
            case 0: {Sat.clear(); return;}
            default: {cout << "输入出现问题，请重新输入!\n\n"; break;}
        }
        cout << endl;
    }
}

int main() {
//    cout << "Hello World!" << endl;
    srand(unsigned(time(NULL)));
    clock_t start, finish;

    start = clock();
    read_file(); //读取文件
    
    bool quit = false;
    while(!quit) {
        int choice = fun_selection();
        switch (choice) {
            case 1: {TimeWindow(); break;} //计算时间窗口
            case 2: {Dispatch(); break;} //卫星调度规划
            case 3: {DataStore(); break;} //数据存储
            case 0: {quit = true; break;}
            default: {cout << "输入出现问题，请重新输入!\n\n"; break;}
        }
        cout << endl;
    }
    
    finish = clock();

    cout << "usr time:" << double(finish - start) / CLOCKS_PER_SEC << "s" << endl;
}
//Test1:39600 45000 Test2:0 21600 Test3:39600 54000 Test4:57600 68400 Test5:0 43200
//Test6:28800 43200 Test7:10800 54000 Test8:28800 43200 Test9:54000 72000 Test10:0 86400

//0 2000 6 1500 -1 0 ***1***
//0 1000 1 800 2 1100 -1 0 ***2***
//0 500 1 700 3 800 4 1000 5 1000 -1 0 ***3***
//3 800 4 900 5 600 6 1200 7 500 8 1000 -1 0 ***4***
//0 2500 1 2500 2 2500 3 800 4 800 5 800 6 600 7 600 8 600 -1 0***5***
//0 2500 1 2500 2 1500 6 1500 7 1000 8 1000 -1 0***6***
//3 9000 5 6000 -1 0 ***7***
//0 2200 1 2200 2 2200 3 2200 4 2200 5 2200 -1 0 ***8***
//3 5000 4 2500 5 4000 6 3000 7 2500 8 3000 -1 0 ***9***
//4 5000 5 4000 6 6000 7 6000 8 6000 -1 0 ***10***

