#ifndef __DEFINE_H__20241105
#define __DEFINE_H__20241105

#include <QtMath>
#include <vector>

struct Vector3d
{
    double x{0.0}; // 经度
    double y{0.0}; // 纬度
    double z{0.0}; // 高度

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

inline Vector3d calculatePosition(const Vector3d & center, double radius_km, double angle)
{
    double earth_radius_km = 6371.0; // 地球半径，千米

    double delta_lat = (radius_km / earth_radius_km) * std::cos(angle);
    double delta_lon = (radius_km / (earth_radius_km * std::cos(center.y * M_PI / 180))) * std::sin(angle);

    return {
        center.x + delta_lon * 180 / M_PI,
        center.y + delta_lat * 180 / M_PI,
        0.0 // 高度设为0
    };
}

// 生成一个模拟飞机的轨迹列表
inline std::vector<Vector3d> generateFlightPath(const Vector3d & topLeft, const Vector3d & bottomRight, int duration_sec)
{
    std::vector<Vector3d> path;

    // 计算中心点和半径
    Vector3d center = {
        (topLeft.x + bottomRight.x) / 2,
        (topLeft.y + bottomRight.y) / 2,
        0.0};
    double radius_km = std::min(
                           std::abs(topLeft.y - bottomRight.y),
                           std::abs(topLeft.x - bottomRight.x)) *
                       111 / 2.0; // 简化半径计算，将短边的一半作为半径，1°大约等于111km

    double speed_km_s      = 1000.0 / 3600.0; // 飞机速度，km/s
    double circumference   = 2 * M_PI * radius_km;
    double time_per_circle = circumference / speed_km_s;
    double angle_per_sec   = 2 * M_PI / time_per_circle; // 每秒绕行的角度

    // 生成每秒的路径点
    for (int t = 0; t < duration_sec; ++t)
    {
        double angle = angle_per_sec * t; // 计算当前角度
        path.push_back(calculatePosition(center, radius_km, angle));
    }

    return path;
}

// 应用移动平均滤波，平滑轨迹
inline std::vector<Vector3d> applySmoothingFilter(const std::vector<Vector3d> & path, int window_size)
{
    std::vector<Vector3d> smoothed_path;

    int half_window = window_size / 2;
    int path_size   = path.size();

    for (int i = 0; i < path_size; ++i)
    {
        double sum_x = 0.0, sum_y = 0.0, sum_z = 0.0;
        int    count = 0;

        // 取当前点周围的点，计算平均值
        for (int j = -half_window; j <= half_window; ++j)
        {
            int index = i + j;
            if (index >= 0 && index < path_size)
            { // 确保索引有效
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

const double EARTH_RADIUS = 6371000.0; // 地球半径，米
const double GRAVITY      = 9.81;      // 重力加速度

// 计算两点间的距离（米）
inline double calculateDistance(const Vector3d & a, const Vector3d & b)
{
    double dx = (b.x - a.x) * EARTH_RADIUS * M_PI / 180.0 * cos((a.y + b.y) * M_PI / 360.0);
    double dy = (b.y - a.y) * EARTH_RADIUS * M_PI / 180.0;
    double dz = b.z - a.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

// 计算方位角（航向）
inline double calculateHeading(const Vector3d & a, const Vector3d & b)
{
    double dx = b.x - a.x;
    double dy = b.y - a.y;
    return std::atan2(dy, dx) * 180.0 / M_PI;
}

// 计算俯仰角
inline double calculatePitch(const Vector3d & a, const Vector3d & b)
{
    double dz                 = b.z - a.z;
    double horizontalDistance = std::sqrt(std::pow(b.x - a.x, 2) + std::pow(b.y - a.y, 2))*111000;

    double pitch = std::atan2(dz, horizontalDistance) * 180.0 / M_PI;
    return pitch;
}

// 计算横滚角
inline double calculateRoll(double speed, double radius)
{
    return std::atan(speed * speed / (radius * GRAVITY)) * 180.0 / M_PI;
}

// 生成PlatSate向量
inline std::vector<PlatSate> generateFlightStates(const std::vector<Vector3d> & flightList, qint64 startTime)
{
    std::vector<PlatSate> stateList;

    if (flightList.size() < 2)
        return stateList; // 确保有足够的点

    qint64 flightTime = startTime;
    for (size_t i = 1; i < flightList.size(); ++i)
    {
        const Vector3d & prev = flightList[i - 1];
        const Vector3d & curr = flightList[i];

        // 计算速度
        double distance = calculateDistance(prev, curr);
        double speed    = distance; // 间隔1秒，速度=距离

        // 计算航向角
        double heading = calculateHeading(prev, curr);

        // 计算俯仰角
        double pitch = calculatePitch(prev, curr);

        // 计算转弯半径（通过相邻三个点来估算）
        double roll = 0.0;
        if (i > 1)
        {
            const Vector3d & prev2  = flightList[i - 2];
            double           a_to_b = calculateDistance(prev2, prev);
            double           b_to_c = distance;
            double           a_to_c = calculateDistance(prev2, curr);

            // 根据余弦定理计算转弯半径
            double cosTheta = (a_to_b * a_to_b + b_to_c * b_to_c - a_to_c * a_to_c) / (2 * a_to_b * b_to_c);
            if (cosTheta < 1.0)
            {
                double turnRadius = a_to_b / (2 * std::sqrt(1 - cosTheta * cosTheta));
                roll              = calculateRoll(speed, turnRadius);
            }
        }

        // 填充PlatSate
        PlatSate state;
        state.flightTime = flightTime;
        state.longitude  = curr.x;
        state.latitude   = curr.y;
        state.altitude   = curr.z;
        state.heading    = heading;
        state.pitch      = pitch;
        state.roll       = roll;
        state.speed      = speed;

        // 增加到状态列表
        stateList.push_back(state);

        // 增加飞行时间
        flightTime += 1000;
    }

    return stateList;
}

#endif
