#include <QDateTime>
#include <QDebug>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTextStream>

#include "simulation.h"

struct PlatInfo
{
    int                   platId{0};
    PlatType              platType{PlatType::YJJType};
    std::vector<PlatSate> flightDatas;  // 1800-7200条数据（30分钟-2小时）
    qint64                bizTime;      // 数据时间
    std::vector<Vector3d> wayPointList; // 路径点列表
};

struct SimulationPrivate
{
    std::vector<PlatInfo> platVec;
    SimuPos               pos{0};
};

Simulation::Simulation(QObject * parent /*= nullptr*/)
    : QObject(parent)
    , m_pDatas(new SimulationPrivate)
{
}

Simulation::~Simulation()
{
    if (m_pDatas)
    {
        delete m_pDatas;
        m_pDatas = nullptr;
    }
}

void Simulation::startSimulation(int id, int type)
{
    std::vector<Vector3d> wayPointVec;
    // vec 赋值
    std::vector<Vector3d> range;
    if (m_pDatas->pos == SimuPos::XinJiang)
    {
        range = m_rangeXinJiang;
    }
    else if (m_pDatas->pos == SimuPos::NanHai)
    {
        range = m_rangeNanHai;
    }
    if (id == 1001)
    {
        wayPointVec =
            {{19.94052473346618, 110.48831657503017},
             {20.074631671475437, 110.83512746555998},
             {20.084014872281795, 110.99925607630455},
             {20.093397511268105, 111.23189054196858},
             {20.047819432516675, 111.55586614752524},
             {19.91368964840344, 111.85700646810872},
             {19.550972379736507, 112.01257184698837},
             {19.295236225057447, 111.97261009828534},
             {19.106542689145247, 111.7713741494594},
             {18.995921991591718, 111.43027208017287},
             {18.96488100121892, 111.00639210161087},
             {19.110588382068322, 110.72095103944643},
             {19.41238640507693, 110.60249299864817},
             {19.68406427303759, 110.74378632441959},
             {19.807646585544184, 110.83655466962303},
             {19.948574364211428, 110.9435950679347},
             {19.988816388433246, 111.15482145393639},
             {19.995522393640353, 111.47308823824977},
             {19.94454959713706, 111.68716903487311},
             {19.89356033982839, 111.80990869160385},
             {19.78347497029542, 112.04682477320034},
             {19.608793501264515, 112.13531150247131},
             {19.338335722347683, 112.10248578032241}};
    }

    {
        wayPointVec = generateRandomPoints(range.front(), range.back(), 15);
    }

    if (m_step < 1.0)
    {
        m_step = 1.0;
    }
    if (m_minute < 30.0)
    {
        m_minute = 30.0;
    }
    // 计算总共多少个仿真周期
    int simulationCount = int(m_minute * 60.0 / m_step);

    PlatInfo platInfo;
    platInfo.platId = id;
    platInfo.platType;
    platInfo.bizTime      = QDateTime::currentMSecsSinceEpoch();
    platInfo.wayPointList = wayPointVec;
    // 生成轨迹点列表
    generatePathWay(platInfo);
    // 轨迹点列表倒角
    bevellingPathWay(platInfo);
    qint64 currentTime = m_startTime;

    // 如果要实时发数据，下面的改为QTimer，这里暂时不做要求
    platInfo.flightDatas = generateFlightStates(platInfo.wayPointList, currentTime);
    m_pDatas->platVec.push_back(platInfo);
}

void Simulation::generatePathWay(PlatInfo & platInfo)
{
    std::vector<Vector3d> range;
    if (m_pDatas->pos == SimuPos::XinJiang)
    {
        range = m_rangeXinJiang;
    }
    else if (m_pDatas->pos == SimuPos::NanHai)
    {
        range = m_rangeNanHai;
    }
    else
    {
        // no thing need todo
    }

    if (range.size() != 2)
    {
        return;
    }
    std::vector<Vector3d> wayPointList;
    // 如果是预警机，就绕圈，否则随机走
    if (platInfo.platType == PlatType::YJJType)
    {
        wayPointList = generateInterpolatedPath(platInfo.wayPointList, 1200);
    }
    else
    {
        wayPointList = generateInterpolatedPath(platInfo.wayPointList, 1200);
    }

    // 生成高度
    int size = wayPointList.size();
    if (size < 6)
    {
        for (auto & pos : wayPointList)
        {
            pos.z = 10000.0;
        }
    }
    else
    {
        int    step       = size / 4;              // 时间上的间距
        double heightStep = 1000.0 / (step * 1.0); // 空间上的间距

        double height = 10000.0;
        for (int i = 0; i < size; i++)
        {
            if (i < step) // 降低
            {
                height = height - heightStep;
            }
            else if (i >= step && i < step * 2) // 升高
            {
                height = height + heightStep;
            }
            else if (i >= step * 2 && i < step * 3) // 降低
            {
                height = height - heightStep;
            }
            else if (i >= step * 3 && i < step * 4) // 升高
            {
                height = height + heightStep;
            }
            wayPointList[i].z = height;
        }
    }

    platInfo.wayPointList = wayPointList;
}

void Simulation::bevellingPathWay(PlatInfo & platInfo)
{
    int window_size = 15; // 滑动窗口大小

    std::vector<Vector3d> pathList = platInfo.wayPointList;
    platInfo.wayPointList          = applySmoothingFilter(pathList, window_size);
}

PlatSate Simulation::calPosState(Vector3d & currPos, Vector3d & nextPos)
{
    PlatSate platState;

    return platState;
}

void Simulation::save()
{
    if (!m_pDatas)
    {
        return;
    }

    QString outDir;
    if (m_pDatas->pos == SimuPos::XinJiang)
    {
        outDir = QCoreApplication::applicationDirPath() + "/data/XinJiang.csv";
    }
    else if (m_pDatas->pos == SimuPos::NanHai)
    {
        outDir = QCoreApplication::applicationDirPath() + "/data/Hainan.csv";
    }
    QStringList header = {"平台编号", "数据时间", "飞行时间", "经度", "纬度", "高度", "航向", "俯仰角", "翻滚角", "速度"};
    QFile       file(outDir);
    if (!file.open(QIODevice::WriteOnly))
    {
        return;
    }
    QTextStream stream(&file);
    stream << header.join(",") + "\n";

    for (PlatInfo platInfo : m_pDatas->platVec)
    {
        for (PlatSate platState : platInfo.flightDatas)
        {
            QString str =
                QString::number(platInfo.platId) + "," + QString::number(platInfo.bizTime) + "," + QString::number(platState.flightTime) + "," + QString::number(platState.longitude) + "," + QString::number(platState.latitude) + "," + QString::number(platState.altitude) + "," + QString::number(platState.heading) + "," + QString::number(platState.pitch) + "," + QString::number(platState.roll) + "," + QString::number(platState.speed) + "\n";
            stream << str;
        }
    }
}
