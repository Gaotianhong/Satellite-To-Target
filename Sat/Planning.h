#ifndef Planning_h
#define Planning_h

#include "TimeWindow.h"

map<string, vector<vector<int>>> target_tW1; //存放某城市可以被观测到的时间窗口
struct cityInfo { //用于贪心算法和启发式算法
    string city; //城市名
    bool visited; //用于标记该城市是否被访问过
    double start; //开始时间
    double need_time; //覆盖时间
    double profit; //观测收益
    double per_profit; //单位时间的收益
    cityInfo(double _start, double _time, double _profit, string _city) {
        start = _start;
        need_time = _time;
        profit = _profit;
        city = _city;
        visited = true; //******
    }
    cityInfo(string _city, double _time, double _profit, double _per_profit):city(_city), visited(false), need_time(_time), profit(_profit), per_profit(_per_profit){} //构造函数 ******
    friend bool operator<(cityInfo c1, cityInfo c2) {
        return c1.per_profit > c2.per_profit; //按单位时间的收益由大到小进行排序
    }
};
vector<cityInfo> tanxinRes; //贪心法
//贪心策略：优先观测单位时间收益大的城市；
//对于每一个卫星，优先观测单位时间收益大的城市，观测完成后，该城市设置一标志代表已观测，对于该卫星的时间片[0,86401)去除该时间窗口
void Greedy() { //贪心法设计星座对地面目标调度规划
    ofstream outfile("./Test/TanxinRes.txt"); //将结果输入到文件
    if (!outfile) outfile.close();
    bool pushData = true;
    double totalProfit = 0.0; //总收益值
    int total_selectCityNum = 0; //选择的城市数量
    for (auto sat_it = Satellite.begin(); sat_it != Satellite.begin() + 9; sat_it++) { //9个卫星，可指定观测的卫星
        outfile << "Satellite" + to_string(sat_it - Satellite.begin()) << endl;
        auto tar_it = Target.begin() + 9; //选择一个指定的数据集
        for (auto _tar = tar_it->begin(); _tar != tar_it->end(); _tar++) { //对应的城市
            if (pushData) {
                tanxinRes.push_back({_tar->first, _tar->second[2], _tar->second[3], _tar->second[3] / _tar->second[2]}); //每个目标单位时间的收益
            }
            bool flag = true; //用于标志能观测到的起止时间
            vector<int> time_window; //时间窗口
            vector<vector<int>> multi_tW; //多个时间窗口
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
                        multi_tW.push_back(time_window); //多重时间窗口
                        flag = true;
                        time_window.clear(); //清空
                    }
                }
            }
            if (!multi_tW.empty())
                target_tW1[_tar->first] = multi_tW;
            multi_tW.clear(); //清空
        }
        pushData = false;
        int selectCityNum = 0;
        double _satProfit = 0.0;
        bool sign[86401]; //该时间片是否存在
        for (int k = 0; k < 86401; k++) sign[k] = true;
        //计算每颗卫星应选择观测的城市
        //若多颗卫星在不同时间段都能观测到这一个城市，优先选择第一个能观测到该城市的时间段，并从头开始选择时间片
        sort(tanxinRes.begin(), tanxinRes.end()); //******按单位时间收益值由大到小排序******
        for (auto p = tanxinRes.begin(); p != tanxinRes.end(); p++) { //按单位时间收益由大到小选择
            if (p->visited != true) { //该城市未被观测过
                bool choose_tW = false;
                vector<vector<int>> temp = target_tW1[p->city]; //该城市对应的观测窗口
                double cover_time = p->need_time;
                for (auto it : temp) {
                    if (it.empty() || cover_time > (double)it[1] - it[0]) continue; //该城市没有卫星能够覆盖
                    for (int i = it[0]; i <= it[0] + cover_time; i++) {
                        if (sign[i] == false) break; //该时间片已经不能用了
                        if (i == it[0] + cover_time && sign[i] == true) {
                            for (int j = it[0]; j <= it[0] + cover_time; j++) sign[j] = false; //用掉该卫星的时间片
                            choose_tW = true;
                            selectCityNum++; //选择城市数量
                            p->visited = true;
                            _satProfit += p->profit; //单颗卫星观测收益
                            outfile << "观测" << p->city << "获得收益" << p->profit << endl;
                        }
                    }
                    if (choose_tW) break; //已经确定要观测的时间片
                }
            }
        }
        target_tW1.clear(); //遍历完一颗卫星之后清空
        totalProfit += _satProfit; //总收益
        total_selectCityNum += selectCityNum; //总数量
        outfile << "选择的城市数量：" << selectCityNum << " 总收益：" << _satProfit << endl;
    }
    outfile <<"选择的城市总数量："<< total_selectCityNum << " 总收益：" << totalProfit << endl;
    outfile.close();
}

struct Integral { //用于对同一颗卫星所能观测到的时间窗口的开始时间进行排序
    string city;
    double need_time; //所需观测时间
    double profit; //收益
    bool visited; //标记城市是否被访问
    int start;
    int finish;
    vector<int> beg_end; //beg_end[0]代表开始时间，beg_end[1]代表结束时间
    Integral(string _city):city(_city){visited = false;}
    Integral(string _city, double _need_time, double _profit, vector<int> _beg_end):city(_city), need_time(_need_time), profit(_profit), beg_end(_beg_end) {
        visited = false;
        start = beg_end[0];
        finish = beg_end[1];
    }
    friend bool operator<(Integral c1, Integral c2) { //按观测时间先后排序
        return c1.start < c2.start;
    }
};
vector<Integral> IInfo; //存入结构体
map<string, bool> IsVisited; //全局判断某城市是否被访问过
//对每一颗卫星，考虑时间片的安放问题，安排尽可能多的城市进行观测，减少碎片化时间片的数量；最好的情况是观察完这一个目标，紧接着可以观察另一个目标，尽可能使时间片变得连续；同时在时间选择方面，下一个时间片开始之前，优先选择时间较短的时间片
void Integer() { //整数规划法设计星座对地面目标调度规划
    ofstream outfile("./Test/IntegerRes.txt"); //将结果输入到文件
    if (!outfile) outfile.close();
    bool pushData = true;
    double totalProfit = 0.0; //总收益值
    int total_selectCityNum = 0; //选择的城市数量
    for (auto sat_it = Satellite.begin(); sat_it != Satellite.begin() + 9; sat_it++) { //9个卫星
        outfile << "Satellite" + to_string(sat_it - Satellite.begin()) << endl;
        auto tar_it = Target.begin() + 9; //选择一个指定的数据集
        for (auto _tar = tar_it->begin(); _tar != tar_it->end(); _tar++) { //对应的城市
            if (pushData) {
                IsVisited[_tar->first] = false; //初始化
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
                        IInfo.push_back({_tar->first, _tar->second[2], _tar->second[3], time_window});
                        flag = true;
                        time_window.clear(); //清空
                    }
                }
            }
        }
        pushData = false;
        sort(IInfo.begin(), IInfo.end()); //排序后同样转换为时间片选择问题
        int selectCityNum = 0;
        double _satProfit = 0.0;
        bool sign[86401]; //该时间片是否存在
        for (int k = 0; k < 86401; k++) sign[k] = true;
        //计算每颗卫星应选择观测的城市
        for (auto p = IInfo.begin(); p != IInfo.end() - 1; p++) { //按时间片先后顺序进行选择
            auto q = p + 1;
            if (IsVisited[p->city] && IsVisited[q->city]) continue; //两个城市都被访问过
            else if (IsVisited[p->city] != true && IsVisited[q->city] != true) { //两个城市均没有该城市没有被访问过 两个为一组进行对比，优先选择访问时间较短的
                auto pq = (p->need_time < q->need_time) ? p:q;
                if (pq->need_time < (double)pq->finish - pq->start) {
                    for (int i = pq->start; i <= pq->start + pq->need_time; i++) {
                        if (sign[i] == false) break; //该时间片已经用完
                        if (i == pq->start + p->need_time && sign[i] == true) {
                            for (int j = pq->start; j <= pq->start + pq->need_time; j++) sign[j] = false; //用掉该卫星的时间片
                            selectCityNum++; //选择城市数量
                            IsVisited[pq->city] = true;
                            _satProfit += pq->profit; //单颗卫星观测收益
                            outfile << "观测" << pq->city << "获得收益" << pq->profit << endl;
                        }
                    }
                }
            }
            else {
                //有一个城市被访问过，选择另外一个城市
                vector<Integral>::iterator pq;
                if (IsVisited[p->city] != true) pq = p; //访问p
                if (IsVisited[q->city] != true) pq = q; //访问q
                if (pq->need_time < (double)pq->finish - pq->start) {
                    for (int i = pq->start; i <= pq->start + pq->need_time; i++) {
                        if (sign[i] == false) break; //该时间片已经用完
                        if (i == pq->start + p->need_time && sign[i] == true) {
                            for (int j = pq->start; j <= pq->start + pq->need_time; j++) sign[j] = false; //用掉该卫星的时间片
                            selectCityNum++; //选择城市数量
                            IsVisited[pq->city] = true;
                            _satProfit += pq->profit; //单颗卫星观测收益
                            outfile << "观测" << pq->city << "获得收益" << pq->profit << endl;
                        }
                    }
                }
            }
        }
        totalProfit += _satProfit; //总收益
        total_selectCityNum += selectCityNum; //总数量
        IInfo.clear(); //遍历完一颗卫星之后清空
        outfile << "选择的城市数量：" << selectCityNum << " 总收益：" << _satProfit << endl;
    }
    outfile <<"选择的城市总数量："<< total_selectCityNum << " 总收益：" << totalProfit << endl;
    outfile.close();
}

bool cmp(cityInfo c1, cityInfo c2) { //按单位时间收益由小到大排序
    return c1.per_profit < c2.per_profit;
}
vector<cityInfo> heuristic; //******按单位时间收益值由小到大排序******
vector<cityInfo> heurRes; //存放每颗卫星选择的城市
//通过插入或删除新的任务，然后按照从低到高的优先级反复插入新的任务
//直接插入新的动态任务或通过重复删除插入新的动态任务，所有新任务首先按照优先级从低到高插入到等待队列中
void Heuristic() { //启发式算法设计星座对地面目标调度规划
    ofstream outfile("./Test/HeurRes.txt"); //将结果输入到文件
    if (!outfile) outfile.close();
    bool pushData = true;
    double totalProfit = 0.0; //总收益值
    int total_selectCityNum = 0; //选择的城市数量
    for (auto sat_it = Satellite.begin(); sat_it != Satellite.begin() + 9; sat_it++) { //9个卫星
        outfile << "Satellite" + to_string(sat_it - Satellite.begin()) << endl;
        auto tar_it = Target.begin() + 9; //选择一个指定的数据集
        for (auto _tar = tar_it->begin(); _tar != tar_it->end(); _tar++) { //对应的城市
            if (pushData) {
                heuristic.push_back({_tar->first, _tar->second[2], _tar->second[3], _tar->second[3] / _tar->second[2]}); //每个目标单位时间的收益
            }
            bool flag = true; //用于标志能观测到的起止时间
            vector<int> time_window; //时间窗口
            vector<vector<int>> multi_tW; //多个时间窗口
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
                        multi_tW.push_back(time_window); //多重时间窗口
                        flag = true;
                        time_window.clear(); //清空
                    }
                }
            }
            if (!multi_tW.empty())
                target_tW1[_tar->first] = multi_tW;
            multi_tW.clear(); //清空
        }
        pushData = false;
        bool sign[86401]; //该时间片是否存在
        for (int k = 0; k < 86401; k++) sign[k] = true;
        sort(heuristic.begin(), heuristic.end(), cmp); //******按单位时间收益由小到大排序******
        //计算每颗卫星应选择观测的城市
        for (auto &p : heuristic) {
            if (p.visited != true) { //该城市未被观测过，在时间片的选择上，从优先级由小到大选择，在一定范围内，如果高优先级的城市的时间片已经被占用，则删除优先级较低的城市的时间片，赋予给高优先级
                //删除的选择：遍历所有已经选择的城市，删除一个单位时间收益最小的，并且删除之后，空余出来的时间片能赋予给该城市
                bool choose_tW = false;
                vector<vector<int>> _tW = target_tW1[p.city]; //该城市对应的观测窗口
                for (auto it : _tW) {
                    if (it.empty() || p.need_time > (double)it[1] - it[0]) continue; //该城市没有卫星能够覆盖
                    for (int i = it[0]; i <= it[0] + p.need_time; i++) {
                        if (sign[i] == false && !heurRes.empty()) { //时间片冲突 删除策略
                            for (auto &its : heurRes) {
                                if (its.visited != true) continue;
                                for (int j = its.start; j <= its.start + its.need_time; j++) {
                                    sign[j] =true; //先删除，查看删除之后是否能多出来的时间片分配给优先级高的
                                }
                                bool IsSatisfy = true;
                                for (int k = i; k < it[0] + p.need_time; k++) {
                                    if (sign[k] == false) IsSatisfy = false;
                                }
                                if (!IsSatisfy) { //不能满足条件
                                    for (int j = its.start; j <= its.start + its.need_time; j++) {
                                        sign[j] = false; //取消删除 遍历下一个
                                    }
                                }
                                else { //将该时间片分配给优先级高的
                                    its.visited = false; //取消访问
                                    //在heuristic里面找到该城市，设置它的标志p->visited为false
                                    for (auto &tmp : heuristic) {
                                        if (tmp.city == its.city) {
                                            tmp.visited = false;
                                            break;
                                        }
                                    }
                                    heurRes.push_back({(double)it[0], p.need_time, p.profit, p.city});
                                    choose_tW = true;
                                    p.visited = true;
                                    break;
                                }
                            }
                            if (choose_tW) break;
                        }
                        else if (i == it[0] + p.need_time &&  sign[i] == true) { //该时间片可以用，直接赋予时间片
                            for (int j = it[0]; j <= it[0] + p.need_time; j++) sign[j] = false; //需要记录一下该城市的时间片
                            heurRes.push_back({(double)it[0], p.need_time, p.profit, p.city});
                            choose_tW = true;
                            p.visited = true;
                        }
                    }
                    if (choose_tW) break;
                }
            }
        }
        target_tW1.clear(); //遍历完一颗卫星之后清空
        int selectCityNum = 0;
        double _satProfit = 0.0;
        for (auto it : heurRes) { //求和
            if (it.visited) { //被访问过的才进行求和
                outfile << "观测" << it.city << "获得收益" << it.profit << endl;
                selectCityNum++;
                _satProfit += it.profit;
            }
        }
        heurRes.clear();
        totalProfit += _satProfit; //总收益
        total_selectCityNum += selectCityNum; //总数量
        outfile << "选择的城市数量：" << selectCityNum << " 总收益：" << _satProfit << endl;
    }
    outfile <<"选择的城市总数量："<< total_selectCityNum << " 总收益：" << totalProfit << endl;
    outfile.close();
}

#endif
