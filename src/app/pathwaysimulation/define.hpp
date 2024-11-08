#ifndef __DEFINE_H__20241105
#define __DEFINE_H__20241105

#include <QtMath>
#include <vector>
#include <random>
#include <algorithm>

const double EARTH_RADIUS = 6371000.0; // ����뾶����
const double GRAVITY      = 9.81;      // �������ٶ�

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

// ���������ľ��루�ף�
inline double calculateDistance(const Vector3d & a, const Vector3d & b)
{
    double dx = (b.x - a.x) * EARTH_RADIUS * M_PI / 180.0 * cos((a.y + b.y) * M_PI / 360.0);
    double dy = (b.y - a.y) * EARTH_RADIUS * M_PI / 180.0;
    double dz = b.z - a.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

// ����ģ��ɻ�·���ĺ���
// ���Բ�ֵ����·����
inline std::vector<Vector3d> generateInterpolatedPath(const std::vector<Vector3d> & waypoints, double speed_km_s)
{
    std::vector<Vector3d> path;
    double                speed_m_s = speed_km_s * 1000.0 / 3600.0; // ���ٶ�ת��Ϊ m/s

    for (size_t i = 1; i < waypoints.size(); ++i)
    {
        Vector3d start = waypoints[i - 1];
        Vector3d end   = waypoints[i];

        double segment_distance = calculateDistance(start, end);
        int    segment_steps    = static_cast<int>(segment_distance / speed_m_s); // �ö�·�����貽��

        // �ڸö�·���ϲ�ֵ����·����
        for (int step = 0; step <= segment_steps; ++step)
        {
            double   t            = static_cast<double>(step) / segment_steps; // ��һ����ֵ����
            Vector3d interpolated = {
                start.x + t * (end.x - start.x),
                start.y + t * (end.y - start.y),
                start.z + t * (end.z - start.z)};
            path.push_back(interpolated);
        }
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
    double horizontalDistance = std::sqrt(std::pow(b.x - a.x, 2) + std::pow(b.y - a.y, 2)) * 111000;

    double pitch = std::atan2(dz, horizontalDistance) * 180.0 / M_PI;
    return pitch;
}

// ��������
inline double calculateRoll(double speed, double radius)
{
    if (radius == 0 || radius > 10000)
    {
        return 0;
    }
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
                if (1 - cosTheta * cosTheta > 0)
                {
                    double fenmu      = 2 * std::sqrt(1 - cosTheta * cosTheta);
                    double turnRadius = a_to_b / fenmu;
                    roll              = calculateRoll(speed, turnRadius);
                }
                else
                {
                    roll = 0;
                }
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

// �ڸ�����Χ�����������
inline std::vector<Vector3d> generateRandomPoints(const Vector3d topLeft, const Vector3d bottomRight, int num_points)
{
    std::vector<Vector3d> randomPoints;
    std::random_device    rd;
    std::mt19937          gen(rd());

    // ��γ�ȷ�Χ
    std::uniform_real_distribution<> lon_dist(topLeft.x, bottomRight.x);
    std::uniform_real_distribution<> lat_dist(bottomRight.y, topLeft.y);

    for (int i = 0; i < num_points; ++i)
    {
        double lon = lon_dist(gen);
        double lat = lat_dist(gen);
        randomPoints.emplace_back(lon, lat, 10000.0); // �߶ȹ̶�Ϊ10000
    }

    return randomPoints;
}

// �������ڼ�������Ĳ��
inline double cross(const Vector3d & o, const Vector3d & a, const Vector3d & b)
{
    return (a.x - o.x) * (b.y - o.y) - (a.y - o.y) * (b.x - o.x);
}

// ͹���㷨�����㰴˳������
inline std::vector<Vector3d> convexHull(std::vector<Vector3d> & points)
{
    std::vector<Vector3d> hull;

    // ����������
    std::sort(points.begin(), points.end(), [](const Vector3d & a, const Vector3d & b) {
        return a.x < b.x || (a.x == b.x && a.y < b.y);
    });

    // ��͹��
    for (const auto & p : points)
    {
        while (hull.size() >= 2 && cross(hull[hull.size() - 2], hull.back(), p) <= 0)
        {
            hull.pop_back();
        }
        hull.push_back(p);
    }

    // ��͹��
    size_t lower_size = hull.size() + 1;
    for (size_t i = points.size(); i > 0; --i)
    {
        const auto & p = points[i - 1];
        while (hull.size() >= lower_size && cross(hull[hull.size() - 2], hull.back(), p) <= 0)
        {
            hull.pop_back();
        }
        hull.push_back(p);
    }

    hull.pop_back(); // ȥ�����һ���㣬��Ϊ���͵�һ�������ظ���
    return hull;
}

#endif
