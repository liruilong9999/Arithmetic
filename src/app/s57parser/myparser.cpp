#include <QFile>
#include <QDebug>

#include "define.h"

#include "myparser.h"

DDRData readDDRData(const QString & filePath)
{
    DDRData ddrData;
    QFile   file(filePath);

    if (!file.open(QIODevice::ReadOnly))
    {
        qWarning() << "Unable to open file!";
        return ddrData;
    }

    // Step 1: 读取DDR头标区
    if (file.read(reinterpret_cast<char *>(&ddrData.header), sizeof(ddrData.header)) != sizeof(ddrData.header))
    {
        qWarning() << "Failed to read DDR header!";
        file.close();
        return ddrData;
    }

    // Step 2: 读取DDR目次区
    // 计算目次区数据长度
    int fieldLen = 4 + QString(ddrData.header.lenSize).toInt() + QString(ddrData.header.posSize).toInt();
    while (!file.atEnd())
    {
        DDRPurpose purpose;

        // 使用QByteArray来存储目次区数据
        QByteArray purposeData; // 初始化为指定大小并填充为零

        // 从文件读取数据
        int  i       = 0;
        bool isBreak = false;
        while (!file.atEnd() && i < fieldLen)
        {
            i++;
            char byte;
            file.getChar(&byte);
            if (byte == 31)
            {
                isBreak = true;
                break;
            }
            purposeData.append(byte);
        }

        if (isBreak)
        {
            break;
        }

        purpose.purposeData = purposeData;   // 将 QByteArray 赋值给 purpose.purposeData
        ddrData.purposes.push_back(purpose); // 将目的区数据添加到 ddrData.purposes
    }

    // Step 3: 读取DDR字段区
    while (!file.atEnd())
    {
        DDRField   field;
        QByteArray data;

        while (!file.atEnd())
        {
            char byte;
            file.getChar(&byte);
            data.append(byte);

            // 字段区结束标识符判断 // "▽" -> ASCII 31
            if (byte == 31)
            {
                char nextSix[1];
                file.read(nextSix, 1);
                break;
            }
        }

        field.fieldData = data;
        ddrData.fields.push_back(field);
    }

    file.close();
    return ddrData;
}

void saveDDRDataToFile(const DDRData & ddrData, const QString & outputPath)
{
    QFile outFile(outputPath);
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qWarning() << "Unable to open output file!";
        return;
    }

    QTextStream out(&outFile);

    // 保存 DDRPurpose 数据
    out << "DDR Purposes:\n";
    for (const auto & purpose : ddrData.purposes)
    {
        // out << purpose.purposeData.toHex(' ') << "\n"; // 以16进制格式保存
    }

    // 保存 DDRField 数据
    out << "\nDDR Fields:\n";
    for (const auto & field : ddrData.fields)
    {
        // out << field.fieldData.toHex(' ') << "\n"; // 以16进制格式保存
    }

    outFile.close();
}

void run()
{
    QString filePath = "./data/s57test/C110407A.000";
    QString savePath = "./data/s57test/test.txt";
    DDRData ddrData  = readDDRData(filePath);
    // saveDDRDataToFile(ddrData, savePath);
}
