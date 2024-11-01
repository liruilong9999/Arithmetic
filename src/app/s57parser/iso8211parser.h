#ifndef ISO8211Parser_h__20241028
#define ISO8211Parser_h__20241028

#include <QString>
#include <QObject>

class ISO8211Parser : public QObject
{
public:
    ISO8211Parser(QObject * parent = nullptr);
    ~ISO8211Parser();

    void initDataList();

    void loadS57Data(QString s57path);
    void loadS57Datas(QStringList s57path);

private:
    QStringList m_s57paths;
};

#endif