#include <QDateTime>

#include "simulation.h"

enum class PlatType
{
    J10Type = 0, // 歼10
    J11Type,     // 歼11
    J16Type,     // 歼16
    YJJType,     // 预警机
};

enum class SimuPos
{
    XinJiang = 0, // 新疆
    NanHai        // 南海
};

struct PlatSate
{
    qint64 flightTime; // 飞行时间
    double longitude;  // 经度
    double latitude;   // 纬度
    double altitude;   // 高度
    double heading;    // 航向
    double pitch;      // 俯仰角
    double roll;       // 横滚角
    double speed;      // 速度
};

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

void Simulation::startSimulation()
{
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
    platInfo.platId;   // todo
    platInfo.platType; // todo
    platInfo.bizTime = QDateTime::currentMSecsSinceEpoch();

    // 生成轨迹点列表

    // 轨迹点列表倒角

    qint64 currentTime = m_startTime;
    // 如果要实时发数据，下面的改为QTimer，这里暂时不做要求
    for (int i = 0; i < simulationCount; i++)
    {
        PlatSate platState;
        // todo
        // 推演六自由度
        platInfo.flightDatas.push_back(platState);
    }
    m_pDatas->platVec.push_back(platInfo);
}

void Simulation::generatePathWay(PlatInfo platInfo, int simulationCount)
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

	// 将区域拆分成 6*6个小区域


    // 如果是预警机，就绕圈，否则随机走
    if (platInfo.platType == PlatType::YJJType)
    {

    }
	else
	{
	
	}
}

void Simulation::save()
{
    if (!m_pDatas)
    {
        return;
    }
}
