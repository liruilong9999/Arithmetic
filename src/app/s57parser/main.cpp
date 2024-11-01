#include <QApplication>
#include <QMenuBar>
#include <iostream>

#include <gdal.h>
#include <gdal_priv.h>
#include <ogrsf_frmts.h>
#include <cpl_conv.h>

#include <iostream>
#include <string>

#include <cstring>

// 画线段的函数
inline void DrawLine(int x1, int y1, int x2, int y2, GDALDataset * poOutDataset)
{
    int width = 4096, height = 4096;
    // 这里使用简单的 Bresenham 算法绘制线段
    int dx  = abs(x2 - x1);
    int dy  = abs(y2 - y1);
    int sx  = (x1 < x2) ? 1 : -1;
    int sy  = (y1 < y2) ? 1 : -1;
    int err = dx - dy;

    while (true)
    {
        // 边界检查
        if (x1 >= 0 && x1 < width && y1 >= 0 && y1 < height)
        {
            unsigned char red = 231, green = 113, blue = 72; // 线条颜色为绿色
            poOutDataset->GetRasterBand(1)->RasterIO(GF_Write, x1, y1, 1, 1, &red, 1, 1, GDT_Byte, 0, 0);
            poOutDataset->GetRasterBand(2)->RasterIO(GF_Write, x1, y1, 1, 1, &green, 1, 1, GDT_Byte, 0, 0);
            poOutDataset->GetRasterBand(3)->RasterIO(GF_Write, x1, y1, 1, 1, &blue, 1, 1, GDT_Byte, 0, 0);
        }

        if (x1 == x2 && y1 == y2)
            break;
        int err2 = err * 2;
        if (err2 > -dy)
        {
            err -= dy;
            x1 += sx;
        }
        if (err2 < dx)
        {
            err += dx;
            y1 += sy;
        }
    }
}

void RasterizeS57ToGeoTIFF(const std::string & s57Path, const std::string & outputPath)
{
    GDALAllRegister();

    // 打开 S-57 数据集
    GDALDataset * poDataset = (GDALDataset *)GDALOpenEx(s57Path.c_str(), GDAL_OF_VECTOR, nullptr, nullptr, nullptr);
    if (poDataset == nullptr)
    {
        std::cerr << "Failed to load S57 data." << std::endl;
        return;
    }

    int width = 4096, height = 4096;
    // 确定 S-57 数据的经纬度范围
    double minX = 121.48; // 例：替换为实际最小经度
    double maxX = 126.71; // 例：替换为实际最大经度
    double minY = 36.494; // 例：替换为实际最小纬度
    double maxY = 39.62;  // 例：替换为实际最大纬度

    double pixelWidth  = (maxX - minX) / width;  // 计算每个像素的宽度
    double pixelHeight = (maxY - minY) / height; // 计算每个像素的高度

    // 设置地理变换参数
    double adfGeoTransform[6] = {minX, pixelWidth, 0, maxY, 0, -pixelHeight};

    GDALDataset * poOutDataset = GetGDALDriverManager()->GetDriverByName("GTiff")->Create(outputPath.c_str(), width, height, 3, GDT_Byte, nullptr);

    poOutDataset->SetGeoTransform(adfGeoTransform);

    OGRSpatialReference oSRS;
    oSRS.SetWellKnownGeogCS("WGS84");
    char * pszWKT = nullptr;
    oSRS.exportToWkt(&pszWKT);
    poOutDataset->SetProjection(pszWKT);
    CPLFree(pszWKT);

    // 遍历 S-57 数据集中的图层
    for (int i = 0; i < poDataset->GetLayerCount(); i++)
    {
        OGRLayer *   poLayer = poDataset->GetLayer(i);
        OGRFeature * poFeature;
        poLayer->ResetReading();

        while ((poFeature = poLayer->GetNextFeature()) != nullptr)
        {
            OGRGeometry * poGeometry = poFeature->GetGeometryRef();
            if (poGeometry != nullptr && wkbFlatten(poGeometry->getGeometryType()) == wkbPoint)
            {
                OGRPoint * poPoint = (OGRPoint *)poGeometry;
                int        x       = static_cast<int>((poPoint->getX() - adfGeoTransform[0]) / adfGeoTransform[1]);
                int        y       = static_cast<int>((poPoint->getY() - adfGeoTransform[3]) / adfGeoTransform[5]);

                // 输出调试信息
                std::cout << "Longitude: " << poPoint->getX() << ", Latitude: " << poPoint->getY() << std::endl;
                std::cout << "Calculated Pixel Coordinates: (" << x << ", " << y << ")" << std::endl;

                // 边界检查
                if (x >= 0 && x < width && y >= 0 && y < height)
                {
                    // 设置 RGB 颜色（例如红色）
                    unsigned char red = 255, green = 0, blue = 0;

                    for (int dx = -1; dx <= 1; dx++)
                    {
                        for (int dy = -1; dy <= 1; dy++)
                        {
                            poOutDataset->GetRasterBand(1)->RasterIO(GF_Write, x + dx, y + dy, 1, 1, &red, 1, 1, GDT_Byte, 0, 0);
                            poOutDataset->GetRasterBand(2)->RasterIO(GF_Write, x + dx, y + dy, 1, 1, &green, 1, 1, GDT_Byte, 0, 0);
                            poOutDataset->GetRasterBand(3)->RasterIO(GF_Write, x + dx, y + dy, 1, 1, &blue, 1, 1, GDT_Byte, 0, 0);
                        }
                    }
                }
                else
                {
                    std::cerr << "Pixel coordinates out of bounds: (" << x << ", " << y << ")" << std::endl;
                }
            }
            else if (poGeometry != nullptr && wkbFlatten(poGeometry->getGeometryType()) == wkbLineString)
            {
                OGRLineString * poLineString = (OGRLineString *)poGeometry;
                for (int j = 0; j < poLineString->getNumPoints() - 1; j++)
                {
                    double x1 = poLineString->getX(j);
                    double y1 = poLineString->getY(j);
                    double x2 = poLineString->getX(j + 1);
                    double y2 = poLineString->getY(j + 1);

                    // 将经纬度转换为像素坐标
                    int px1 = static_cast<int>((x1 - adfGeoTransform[0]) / adfGeoTransform[1]);
                    int py1 = static_cast<int>((y1 - adfGeoTransform[3]) / adfGeoTransform[5]);
                    int px2 = static_cast<int>((x2 - adfGeoTransform[0]) / adfGeoTransform[1]);
                    int py2 = static_cast<int>((y2 - adfGeoTransform[3]) / adfGeoTransform[5]);

                    // 绘制线段
                    DrawLine(px1, py1, px2, py2, poOutDataset);
                }
            }
            else if (poGeometry != nullptr && wkbFlatten(poGeometry->getGeometryType()) == wkbPolygon)
            {
                OGRPolygon *    poPolygon = (OGRPolygon *)poGeometry;
                OGRLinearRing * poRing    = poPolygon->getExteriorRing();

                for (int j = 0; j < poRing->getNumPoints() - 1; j++)
                {
                    double x1 = poRing->getX(j);
                    double y1 = poRing->getY(j);
                    double x2 = poRing->getX(j + 1);
                    double y2 = poRing->getY(j + 1);

                    // 将经纬度转换为像素坐标
                    int px1 = static_cast<int>((x1 - adfGeoTransform[0]) / adfGeoTransform[1]);
                    int py1 = static_cast<int>((y1 - adfGeoTransform[3]) / adfGeoTransform[5]);
                    int px2 = static_cast<int>((x2 - adfGeoTransform[0]) / adfGeoTransform[1]);
                    int py2 = static_cast<int>((y2 - adfGeoTransform[3]) / adfGeoTransform[5]);

                    // 绘制边界
                    DrawLine(px1, py1, px2, py2, poOutDataset);
                }

                // 可以填充多边形，这里省略具体的填充逻辑
            }
            else
            {
            }

            OGRFeature::DestroyFeature(poFeature);
        }
    }

    // 关闭数据集
    GDALClose(poOutDataset);
    GDALClose(poDataset);
}
int main(int argc, char * argv[])
{
    QApplication a(argc, argv);

    std::string s57Path    = "./data/s57testdata/C1100103.000";
    std::string outputPath = "./data/s57testdata/C1100103.png";

    RasterizeS57ToGeoTIFF(s57Path, outputPath);

    int ret = a.exec();
    return ret;
}