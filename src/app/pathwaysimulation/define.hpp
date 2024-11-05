#ifndef __DEFINE_H__20241105
#define __DEFINE_H__20241105

#include <QtMath>
#include <vector>

struct Vector3d
{
    double x{0.0}; // ����
    double y{0.0}; // γ��
    double z{0.0}; // �߶�

    Vector3d() = default;

    Vector3d(double a)
        : Vector3d(a, 0)
    {}

    Vector3d(double a, double b)
        : Vector3d(a, b, 0)
    {}
    Vector3d(double a, double b, double c)
        : x(a)
        , y(b)
        , z(c)
    {}
};

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

inline Vector3d calculatePosition(const Vector3d & center, double radius_km, double angle)
{
    double earth_radius_km = 6371.0; // ����뾶��ǧ��

    double delta_lat = (radius_km / earth_radius_km) * std::cos(angle);
    double delta_lon = (radius_km / (earth_radius_km * std::cos(center.y * M_PI / 180))) * std::sin(angle);

    return {
        center.x + delta_lon * 180 / M_PI,
        center.y + delta_lat * 180 / M_PI,
        0.0 // �߶���Ϊ0
    };
}

// ����һ��ģ��ɻ��Ĺ켣�б�
inline std::vector<Vector3d> generateFlightPath(const Vector3d & topLeft, const Vector3d & bottomRight, int duration_sec)
{
    std::vector<Vector3d> path;

    // �������ĵ�Ͱ뾶
    Vector3d center = {
        (topLeft.x + bottomRight.x) / 2,
        (topLeft.y + bottomRight.y) / 2,
        0.0};
    double radius_km = std::min(
                           std::abs(topLeft.y - bottomRight.y),
                           std::abs(topLeft.x - bottomRight.x)) *
                       111 / 2.0; // �򻯰뾶���㣬���̱ߵ�һ����Ϊ�뾶��1���Լ����111km

    double speed_km_s      = 1000.0 / 3600.0; // �ɻ��ٶȣ�km/s
    double circumference   = 2 * M_PI * radius_km;
    double time_per_circle = circumference / speed_km_s;
    double angle_per_sec   = 2 * M_PI / time_per_circle; // ÿ�����еĽǶ�

    // ����ÿ���·����
    for (int t = 0; t < duration_sec; ++t)
    {
        double angle = angle_per_sec * t; // ���㵱ǰ�Ƕ�
        path.push_back(calculatePosition(center, radius_km, angle));
    }

    return path;
}

// Ӧ���ƶ�ƽ���˲���ƽ���켣
inline std::vector<Vector3d> applySmoothingFilter(const std::vector<Vector3d> & path, int window_size)
{
    std::vector<Vector3d> smoothed_path;

    int half_window = window_size / 2;
    int path_size   = path.size();

    for (int i = 0; i < path_size; ++i)
    {
        double sum_x = 0.0, sum_y = 0.0, sum_z = 0.0;
        int    count = 0;

        // ȡ��ǰ����Χ�ĵ㣬����ƽ��ֵ
        for (int j = -half_window; j <= half_window; ++j)
        {
            int index = i + j;
            if (index >= 0 && index < path_size)
            { // ȷ��������Ч
                sum_x += path[index].x;
                sum_y += path[index].y;
                ++count;
            }
        }

        sum_z = path[i].z;
        smoothed_path.push_back({sum_x / count, sum_y / count, sum_z});
    }

    return smoothed_path;
}

const double EARTH_RADIUS = 6371000.0; // ����뾶����
const double GRAVITY      = 9.81;      // �������ٶ�

// ���������ľ��루�ף�
inline double calculateDistance(const Vector3d & a, const Vector3d & b)
{
    double dx = (b.x - a.x) * EARTH_RADIUS * M_PI / 180.0 * cos((a.y + b.y) * M_PI / 360.0);
    double dy = (b.y - a.y) * EARTH_RADIUS * M_PI / 180.0;
    double dz = b.z - a.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

// ���㷽λ�ǣ�����
inline double calculateHeading(const Vector3d & a, const Vector3d & b)
{
    double dx = b.x - a.x;
    double dy = b.y - a.y;
    return std::atan2(dy, dx) * 180.0 / M_PI;
}

// ���㸩����
inline double calculatePitch(const Vector3d & a, const Vector3d & b)
{
    double dz                 = b.z - a.z;
    double horizontalDistance = std::sqrt(std::pow(b.x - a.x, 2) + std::pow(b.y - a.y, 2))*111000;

    double pitch = std::atan2(dz, horizontalDistance) * 180.0 / M_PI;
    return pitch;
}

// ��������
inline double calculateRoll(double speed, double radius)
{
    return std::atan(speed * speed / (radius * GRAVITY)) * 180.0 / M_PI;
}

// ����PlatSate����
inline std::vector<PlatSate> generateFlightStates(const std::vector<Vector3d> & flightList, qint64 startTime)
{
    std::vector<PlatSate> stateList;

    if (flightList.size() < 2)
        return stateList; // ȷ�����㹻�ĵ�

    qint64 flightTime = startTime;
    for (size_t i = 1; i < flightList.size(); ++i)
    {
        const Vector3d & prev = flightList[i - 1];
        const Vector3d & curr = flightList[i];

        // �����ٶ�
        double distance = calculateDistance(prev, curr);
        double speed    = distance; // ���1�룬�ٶ�=����

        // ���㺽���
        double heading = calculateHeading(prev, curr);

        // ���㸩����
        double pitch = calculatePitch(prev, curr);

        // ����ת��뾶��ͨ�����������������㣩
        double roll = 0.0;
        if (i > 1)
        {
            const Vector3d & prev2  = flightList[i - 2];
            double           a_to_b = calculateDistance(prev2, prev);
            double           b_to_c = distance;
            double           a_to_c = calculateDistance(prev2, curr);

            // �������Ҷ������ת��뾶
            double cosTheta = (a_to_b * a_to_b + b_to_c * b_to_c - a_to_c * a_to_c) / (2 * a_to_b * b_to_c);
            if (cosTheta < 1.0)
            {
                double turnRadius = a_to_b / (2 * std::sqrt(1 - cosTheta * cosTheta));
                roll              = calculateRoll(speed, turnRadius);
            }
        }

        // ���PlatSate
        PlatSate state;
        state.flightTime = flightTime;
        state.longitude  = curr.x;
        state.latitude   = curr.y;
        state.altitude   = curr.z;
        state.heading    = heading;
        state.pitch      = pitch;
        state.roll       = roll;
        state.speed      = speed;

        // ���ӵ�״̬�б�
        stateList.push_back(state);

        // ���ӷ���ʱ��
        flightTime += 1000;
    }

    return stateList;
}

#endif
