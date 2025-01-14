#ifndef HISTORYUPDATELISTWIG_H
#define HISTORYUPDATELISTWIG_H

#include <QFrame>
#include <QLabel>
#include <QTextEdit>
#include <QPainter>
#include <QBoxLayout>
#include <QMouseEvent>
#include <QDebug>

class HistoryUpdateListWig : public QFrame
{
    Q_OBJECT
public:
    HistoryUpdateListWig();
    ~HistoryUpdateListWig();
    void setAttribute(const QString &mname, const QString &mstatue, const QString &mtime, const QString &mdescription, const int &myid);//赋值
    QSize getTrueSize();//获取真实大小
    void selectStyle();//选中样式
    void clearStyleSheet();//取消选中样式
    int id = 0;
protected:
    void mousePressEvent(QMouseEvent * event);
private:
    QHBoxLayout *hl1 = nullptr;
    QHBoxLayout *hl2 = nullptr;
    QVBoxLayout *vl1 = nullptr;
    QLabel *debName = nullptr;//app名字&版本号
    QLabel *debStatue = nullptr;//更新状态
    QString debDescription = "";//描述
    //int code = 0 ;//编码
    void initUI();//初始化UI
    void setDescription();//赋值事件
    QFont font;
};

#endif // HISTORYUPDATELISTWIG_H
