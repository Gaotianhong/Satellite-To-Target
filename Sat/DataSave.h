#ifndef DataSave_h
#define DataSave_h

#include "TimeWindow.h"

struct SatMemory {
    int num; //卫星编号
    int memory; //容量
    SatMemory(int _num, int _memory):num(_num), memory(_memory){}
};
vector<SatMemory> Sat; //卫星信息
void InputSatMemory() {
    cout << "请输入需要选择的卫星编号及存储容量，输入-1代表结束：";
    int _num = 0, _memory = 0;
    while (_num != -1) { //-1代表输入结束
        cin >> _num >> _memory;
        if (_num != -1) Sat.push_back({_num, _memory});
    }
} //选择仿真周期，观测目标文件，可选卫星资源

struct dpIntegral { //用于对同一颗卫星所能观测到的时间窗口的开始时间进行排序
    string city;
    int need_time; //所需观测时间
    double profit; //收益
    bool visited; //标记城市是否被访问
    int start;
    int finish;
    vector<int> beg_end; //beg_end[0]代表开始时间，beg_end[1]代表结束时间
    dpIntegral(){}
    dpIntegral(string _city):city(_city){visited = false;}
    dpIntegral(string _city, vector<int> _timeWindow, double _needTime, double _profit):city(_city), need_time(_needTime), profit(_profit) {
        start = _timeWindow[0];
        finish = _timeWindow[1];
    }
    friend bool operator<(dpIntegral c1, dpIntegral c2) { //按观测时间先后排序
        return c1.start < c2.start;
    }
};
map<string, bool> Visited; //全局判断某城市是否被访问过
//星载存储器，1MB/增加，增加约束条件
//我采用动态规划的思想是，仿照背包问题的模型，对每颗卫星所能够观测到的城市数n和总容量C构建一个二维数组，有如下关系式：
//P[i,c] = max{P[i-1,c-vi] + pi, P[i-1,c]} 表示从第1个城市到第i个城市进行选择，卫星剩余容量为C的最优解，将选择的结果保存下来，再采用整数规划法的思想进行选择
int DP[2000][2000], Rec[2000][2000];
void DataSaveOfDP() { //动态规划算法解决数据存储问题
    ofstream outfile("./Test/dpTest.txt"); //将结果输入到文件
    vector<dpIntegral> dpIInfo;
    bool pushData = true;
    double totalProfit = 0.0; //总收益值
    double total_selectCityNum = 0.0;
    for (auto itOfSat : Sat) { //选择卫星
        auto sat_it = Satellite.begin() + itOfSat.num;
        outfile << "Satellite" + to_string(sat_it - Satellite.begin()) << endl;
        auto tar_it = Target.begin() + 9; //选择一个指定的数据集
        for (auto _tar = tar_it->begin(); _tar != tar_it->end(); _tar++) { //对应的城市
            if (pushData) {
                Visited[_tar->first] = false;
            }
            bool flag = true; //用于标志能观测到的起止时间
            vector<int> time_window; //时间窗口
            for (auto _sat = sat_it->begin(); _sat != sat_it->end(); _sat++) { //86401s
                if (isPolygon({_tar->second[0], _tar->second[1]}, _sat->second)) {
                    if (flag == true) { //计算恰好能够对目标提供服务的开始时间
                        time_window.push_back(_sat->first); //开始时间
                        flag = false;
                    }
                }
                else { //卫星恰好不能对目标提供服务的时间
                    if (flag == false) {
                        time_window.push_back(_sat->first); //结束时间
                        dpIInfo.push_back({_tar->first, time_window, _tar->second[2], _tar->second[3]});
                        flag = true;
                        time_window.clear(); //清空
                    }
                }
            }
        }
        vector<dpIntegral> DPRes; //用于存放动态规划选择城市的结果
        DPRes = dpIInfo;
        pushData = false;
        //首先在不考虑时间片冲突的情况下，利用动态规划方法选择城市
//        size_t n = dpIInfo.size(); //某颗卫星所能观测到的城市数量
//        dpIntegral tempdpIInfo[n+1]; //统一计量单位
//        for (size_t i = 0; i < n; i++) {
//            tempdpIInfo[i+1].profit = dpIInfo[i].profit;
//            tempdpIInfo[i+1].need_time = dpIInfo[i].need_time;
//        }
//        int C = itOfSat.memory; //该颗卫星的容量
////        int DP[n+1][C+1], Rec[n+1][C+1]; //在函数外面开二维数组，防止溢出
//        for (int i = 0; i <= C; i++) DP[0][i] = 0;
//        for (int i = 0; i <= n; i++) DP[i][0] = 0; //初始化
        //求解表格 选择城市
//        for (int i = 1; i <= n; i++) {
//            for (int c = 1; c <= C; c++) {
//                if (tempdpIInfo[i].need_time <= c && tempdpIInfo[i].profit + DP[i-1][c-tempdpIInfo[i].need_time] > DP[i-1][c]) {
//                    DP[i][c] = tempdpIInfo[i].profit + DP[i-1][c-tempdpIInfo[i].need_time];
//                    Rec[i][c] = 1;
//                }
//                else {
//                    DP[i][c] = DP[i-1][c];
//                    Rec[i][c] = 0;
//                }
//            }
//        }
//        int K = C;
//        for (size_t i = n; i >= 1; i--) { //倒序判断是否选择该城市
//            if (Rec[i][K] == 1) {
//                DPRes.push_back(dpIInfo[i-1]); //记录动态规划的结果
//                K = K - dpIInfo[i-1].need_time;
//            }
//        }
        sort(DPRes.begin(), DPRes.end());
        int selectCityNum = 0;
        double _satProfit = 0.0;
        bool sign[86401]; //该时间片是否存在
        for (int k = 0; k < 86401; k++) sign[k] = true;
        //计算每颗卫星应选择观测的城市
        for (auto p = DPRes.begin(); p != DPRes.end() - 1; p++) { //按时间片先后顺序进行选择
            auto q = p + 1;
            if (Visited[p->city] && Visited[q->city]) continue; //两个城市都被访问过
            else if (Visited[p->city] != true && Visited[q->city] != true) { //两个城市均没有该城市没有被访问过 两个为一组进行对比，优先选择访问时间较短的
                auto pq = (p->need_time < q->need_time) ? p:q;
                if (pq->need_time < (double)pq->finish - pq->start) {
                    for (int i = pq->start; i <= pq->start + pq->need_time; i++) {
                        if (sign[i] == false) break; //该时间片已经用完
                        if (i == pq->start + p->need_time && sign[i] == true) {
                            for (int j = pq->start; j <= pq->start + pq->need_time; j++) sign[j] = false; //用掉该卫星的时间片
                            selectCityNum++; //选择城市数量
                            Visited[pq->city] = true;
                            _satProfit += pq->profit; //单颗卫星观测收益
                            outfile << "观测" << pq->city << "获得收益" << pq->profit << endl;
                        }
                    }
                }
            }
            else {
                //有一个城市被访问过，选择另外一个城市
                if (Visited[p->city] != true) { //访问p
                    if (p->need_time < (double)p->finish - p->start) {
                        for (int i = p->start; i <= p->start + p->need_time; i++) {
                            if (sign[i] == false) break; //该时间片已经用完
                            if (i == p->start + p->need_time && sign[i] == true) {
                                for (int j = p->start; j <= p->start + p->need_time; j++)
                                    sign[j] = false; //用掉该卫星的时间片
                                selectCityNum++; //选择城市数量
                                Visited[p->city] = true;
                                _satProfit += p->profit; //单颗卫星观测收益
                                outfile << "观测" << p->city << "获得收益" << p->profit << endl;
                            }
                        }
                    }
                }
                if (Visited[q->city] != true) {//访问q
                    if (q->need_time < (double)q->finish - q->start) {
                        for (int i = q->start; i <= q->start + q->need_time; i++) {
                            if (sign[i] == false) break; //该时间片已经用完
                            if (i == q->start + q->need_time && sign[i] == true) {
                                for (int j = q->start; j <= q->start + q->need_time; j++)
                                    sign[j] = false; //用掉该卫星的时间片
                                selectCityNum++; //选择城市数量
                                Visited[q->city] = true;
                                _satProfit += q->profit; //单颗卫星观测收益
                                outfile << "观测" << q->city << "获得收益" << q->profit << endl;
                            }
                        }
                    }
                }
            }
        }
        totalProfit += _satProfit; //总收益
        total_selectCityNum += selectCityNum; //总数量
        dpIInfo.clear();
        DPRes.clear(); //遍历完一颗卫星之后清空
        outfile << "选择的城市数量：" << selectCityNum << " 总收益：" << _satProfit << endl;
    }
    outfile <<"选择的城市总数量："<< total_selectCityNum << " 总收益：" << totalProfit << endl;
    outfile.close();
}

#endif
