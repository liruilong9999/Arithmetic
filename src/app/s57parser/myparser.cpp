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

    // Step 1: ��ȡDDRͷ����
    if (file.read(reinterpret_cast<char *>(&ddrData.header), sizeof(ddrData.header)) != sizeof(ddrData.header))
    {
        qWarning() << "Failed to read DDR header!";
        file.close();
        return ddrData;
    }

    // Step 2: ��ȡDDRĿ����
    // ����Ŀ�������ݳ���
    int fieldLen = 4 + QString(ddrData.header.lenSize).toInt() + QString(ddrData.header.posSize).toInt();
    while (!file.atEnd())
    {
        DDRPurpose purpose;

        // ʹ��QByteArray���洢Ŀ��������
        QByteArray purposeData; // ��ʼ��Ϊָ����С�����Ϊ��

        // ���ļ���ȡ����
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

        purpose.purposeData = purposeData;   // �� QByteArray ��ֵ�� purpose.purposeData
        ddrData.purposes.push_back(purpose); // ��Ŀ����������ӵ� ddrData.purposes
    }

    // Step 3: ��ȡDDR�ֶ���
    while (!file.atEnd())
    {
        DDRField   field;
        QByteArray data;

        while (!file.atEnd())
        {
            char byte;
            file.getChar(&byte);
            data.append(byte);

            // �ֶ���������ʶ���ж� // "��" -> ASCII 31
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

    // ���� DDRPurpose ����
    out << "DDR Purposes:\n";
    for (const auto & purpose : ddrData.purposes)
    {
        // out << purpose.purposeData.toHex(' ') << "\n"; // ��16���Ƹ�ʽ����
    }

    // ���� DDRField ����
    out << "\nDDR Fields:\n";
    for (const auto & field : ddrData.fields)
    {
        // out << field.fieldData.toHex(' ') << "\n"; // ��16���Ƹ�ʽ����
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
