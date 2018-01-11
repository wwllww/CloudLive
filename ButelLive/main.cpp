#include "Login.h"
#include <QApplication>
#include <QFont>
#include <QTranslator>
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QFont font;
    font.setFamily("微软雅黑");
    a.setFont(font);
    QTranslator *translator = new QTranslator;
    translator->load("ButelLive_zh.qm");
    a.installTranslator(translator);
    LoginUI w;
    w.show();
    return a.exec();
}
