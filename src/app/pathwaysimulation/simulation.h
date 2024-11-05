#ifndef SIMULATION_H__
#define SIMULATION_H__

#include <QObject>
#include <QPointF>

#include <vector>

struct Vector3d
{
    double x{0.0};
    double y{0.0};
    double z{0.0};

    Vector3d() = default;

    Vector3d(int a)
        : Vector3d(a, 0)
    {}

    Vector3d(int a, int b)
        : Vector3d(a, b, 0)
    {}
    Vector3d(int a, int b, int c)
        : x(a)
        , y(b)
        , z(c)
    {}
};

struct SimulationPrivate;
struct PlatInfo;
class Simulation : public QObject
{
    Q_OBJECT
public:
    Simulation(QObject * parent = nullptr);
    ~Simulation();

    // ��ʼ����
    void startSimulation();

    // ���ɹ켣���б�
    void generatePathWay(PlatInfo platInfo, int simulationCount);

    // �켣���б���
    void bevellingPathWay();

    void save();

private:
    SimulationPrivate * m_pDatas; // �������ݼ�¼

    double m_step{1.0};  // ���沽��(��)
    double m_minute{30}; // ����ʱ��(����)

    qint64                m_startTime{1704042061000}; // ���濪ʼʱ�䣨Ĭ��2024��1��1��1ʱ1��1�룩
    std::vector<Vector3d> m_rangeXinJiang{Vector3d(92.382523, 28.185758, 10000), Vector3d(93.651440, 27.413159, 10000)};
    std::vector<Vector3d> m_rangeNanHai{Vector3d(111.186868, 19.333742, 10000), Vector3d(114.592629, 14.394031, 10000)};
};

#endif
