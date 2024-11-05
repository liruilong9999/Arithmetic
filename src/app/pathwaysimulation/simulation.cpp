#include <QDateTime>

#include "simulation.h"

enum class PlatType
{
    J10Type = 0, // ��10
    J11Type,     // ��11
    J16Type,     // ��16
    YJJType,     // Ԥ����
};

enum class SimuPos
{
    XinJiang = 0, // �½�
    NanHai        // �Ϻ�
};

struct PlatSate
{
    qint64 flightTime; // ����ʱ��
    double longitude;  // ����
    double latitude;   // γ��
    double altitude;   // �߶�
    double heading;    // ����
    double pitch;      // ������
    double roll;       // �����
    double speed;      // �ٶ�
};

struct PlatInfo
{
    int                   platId{0};
    PlatType              platType{PlatType::YJJType};
    std::vector<PlatSate> flightDatas;  // 1800-7200�����ݣ�30����-2Сʱ��
    qint64                bizTime;      // ����ʱ��
    std::vector<Vector3d> wayPointList; // ·�����б�
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
    // �����ܹ����ٸ���������
    int simulationCount = int(m_minute * 60.0 / m_step);

    PlatInfo platInfo;
    platInfo.platId;   // todo
    platInfo.platType; // todo
    platInfo.bizTime = QDateTime::currentMSecsSinceEpoch();

    // ���ɹ켣���б�

    // �켣���б���

    qint64 currentTime = m_startTime;
    // ���Ҫʵʱ�����ݣ�����ĸ�ΪQTimer��������ʱ����Ҫ��
    for (int i = 0; i < simulationCount; i++)
    {
        PlatSate platState;
        // todo
        // ���������ɶ�
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

	// �������ֳ� 6*6��С����


    // �����Ԥ����������Ȧ�����������
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
