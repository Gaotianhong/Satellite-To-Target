import matplotlib.pyplot as plt
import numpy as np
import plotly as py
import plotly.figure_factory as ff

#读取文件存入字典构造甘特图
def read_file_gannt():
    df = []
    obj = []
    file = open("Data/Gannt/TW_gannt.txt")
    i = 0
    for line in file.readlines():
        line = line.strip('\n') #去掉换行符
        obj.append(line)
        i += 1
        if (i == 4):
            mydict = dict(Task=obj[1]+obj[0], Start=obj[2], Finish=obj[3], Resource='TimeWindow') #构造df
            # mydict = dict(Task=obj[0], Start=obj[2], Finish=obj[3], Resource='TimeWindow') #构造df
            df.append(mydict)
            obj.clear()
            i = 0
    obj.clear()
    file.close()

    j = 0
    file1 = open("Data/Gannt/TW_gannt1.txt")
    for line1 in file1.readlines():
        line1 = line1.strip('\n') #去掉换行符
        obj.append(line1)
        j += 1
        if (j == 4):
            mydict = dict(Task=obj[1]+obj[0], Start=obj[2], Finish=obj[3], Resource='doubleTimeWindow') #构造df
            # mydict = dict(Task=obj[0], Start=obj[2], Finish=obj[3], Resource='doubleTimeWindow') #构造df
            df.append(mydict)
            obj.clear()
            j = 0
    file1.close()
    obj.clear()

    print("[1]Dispatch [2]DataStore")
    selection = eval(input("Please input your choice:"))
    number = eval(input("请选择目标数据集："))

    if (selection == 1):
        print("[1]贪心算法 [2]整数规划算法 [3]启发式算法")
        choice1 = eval(input("请输入："))
        if (choice1 == 1):
            fileDispatch = open("Data/Dispatch/GreedyGannt" + str(number) + ".txt")
        if (choice1 == 2):
            fileDispatch = open("Data/Dispatch/IntegerGannt" + str(number) + ".txt")
        if (choice1 == 3):
            fileDispatch = open("Data/Dispatch/HeuristicGannt" + str(number) + ".txt")
        k = 0
        for line in fileDispatch.readlines():
            line = line.strip('\n')  # 去掉换行符
            obj.append(line)
            k += 1
            if (k == 4):
                mydict = dict(Task=obj[1]+obj[0], Start=obj[2], Finish=obj[3], Resource='Observe')  # 构造df
                # mydict = dict(Task=obj[0], Start=obj[2], Finish=obj[3], Resource='Observe')  # 构造df
                df.append(mydict)
                obj.clear()
                k = 0
        fileDispatch.close()
        obj.clear()

    if (selection == 2):
        print("[1]动态规划 [2]启发式算法")
        choice2 = eval(input("请输入："))
        if (choice2 == 1):
            fileDataStore = open("Data/DataStore/dpGannt" + str(number) + ".txt")
        if (choice2 == 2):
            fileDataStore = open("Data/DataStore/heurGannt" + str(number) + ".txt")
        k = 0
        for line in fileDataStore.readlines():
            line = line.strip('\n')  # 去掉换行符
            obj.append(line)
            k += 1
            if (k == 4):
                mydict = dict(Task=obj[1]+obj[0], Start=obj[2], Finish=obj[3], Resource='Observe')  # 构造df
                # mydict = dict(Task=obj[0], Start=obj[2], Finish=obj[3], Resource='Observe')  # 构造df
                df.append(mydict)
                obj.clear()
                k = 0
        fileDataStore.close()
        obj.clear()

    return df #返回列表绘制甘特图

# 绘制甘特图
pyplt = py.offline.plot

df = read_file_gannt()
colors = {'TimeWindow': 'rgb(0, 0, 255)',
          'doubleTimeWindow': 'rgb(255, 255, 0)',
          'Observe': 'rgb(255, 0, 0'}
fig = ff.create_gantt(df, colors=colors, index_col='Resource', show_colorbar=True,
                      showgrid_x=True, showgrid_y=True, group_tasks=True)
pyplt(fig, filename='gannt.html')


# 读取文件统计每个目标时间间隔的最大值和平均值
def read_file_interval():
    objlist = []
    obj = []
    file = open("Data/Gannt/TW_interval.txt")
    i = 0
    for line in file.readlines():
        line = line.strip('\n') #去掉换行符
        if (i != 0):
            line = eval(line)
        obj.append(line)
        i += 1
        if (i == 3):
            objlist.append(obj[0])
            max_v.append(obj[1])
            average_v.append(obj[2])
            obj.clear()
            i = 0
    #将列表转换为元组
    aTarget = tuple(objlist)
    return aTarget

# 读取文件统计每个目标时间间隔的最大值和平均值
# 这两行代码解决 plt 中文显示的问题
plt.rcParams['font.sans-serif'] = ['Arial Unicode MS']
plt.rcParams['axes.unicode_minus'] = False

# 读入统计数据
max_v = []
average_v = []
Target = read_file_interval()

bar_width = 0.2  # 条形宽度
max_value = np.arange(len(Target))  # 时间窗口的最大值
average_value = max_value + bar_width  # 时间窗口的平均值

# 使用两次 bar 函数画出两组条形图
plt.bar(max_value, height=max_v, width=bar_width, color='b')
plt.bar(average_value, height=average_v, width=bar_width, color='r')
plt.xticks(max_value + bar_width / 2, Target)  # max_value + average_value/2 为横坐标轴刻度的位置
plt.ylabel('时间间隙')  # 纵坐标轴标题
plt.title('星座对每个点目标的时间间隙')  # 图形标题

plt.show()

