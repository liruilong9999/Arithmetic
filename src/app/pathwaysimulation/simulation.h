#ifndef SIMULATION_H__
#define SIMULATION_H__

#include <QObject>
#include <QPointF>

#include <vector>
#include "define.hpp"

struct SimulationPrivate;
struct PlatInfo;
class Simulation : public QObject
{
    Q_OBJECT
public:
    Simulation(QObject * parent = nullptr);
    ~Simulation();

    // 开始仿真
    void startSimulation(int id,int type);

    // 生成轨迹点列表
    void generatePathWay(PlatInfo & platInfo);

    // 轨迹点列表倒角
    void bevellingPathWay(PlatInfo & platInfo);

    PlatSate calPosState(Vector3d & currPos, Vector3d & nextPos);

    void save();

private:
    SimulationPrivate * m_pDatas; // 仿真数据记录

    double m_step{1.0};  // 仿真步长(秒)
    double m_minute{30}; // 仿真时间(分钟)

    qint64                m_startTime{1704042061000}; // 仿真开始时间（默认2024年1月1日1时1分1秒）
    std::vector<Vector3d> m_rangeXinJiang{Vector3d(92.382523, 28.185758, 10000), Vector3d(93.651440, 27.413159, 10000)};
    std::vector<Vector3d> m_rangeNanHai{Vector3d(111.186868, 19.333742, 10000), Vector3d(114.592629, 14.394031, 10000)};
};

#endif
