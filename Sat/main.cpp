#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <queue>
#include <map>
#include <algorithm>
#include <numeric>
using namespace std;

#include "read_file.h"
#include "TimeWindow.h"
#include "Planning.h"
#include "DataSave.h"


int main() {
    clock_t start, finish;

    start = clock();
    
    cout << "读取文件中，请稍后......" << endl;
    read_file1();
    read_file2();
    
    cout << "正在计算时间窗口，请稍后......" << endl;
//    SatTimeWindow(); //计算时间窗口的
    
    //************第二问************
    Greedy(); //贪心算法
    Integer(); //整数规划
    Heuristic(); //启发式
    
    //************第三问************
//    InputSatMemory(); //输入卫星编号和容量
    DataSaveOfDP(); //动态规划
    
    finish = clock();

    cout << "usr time:" << double(finish - start) / CLOCKS_PER_SEC << "s" << endl;
}
//0 1000 1 1000 2 1000 3 1000 4 1000 5 1000 6 1000 7 1000 8 1000 -1 0

