#ifndef WORKER_H
#define WORKER_H

#include <QObject>

class Worker : public QObject
{
    Q_OBJECT
public:
    explicit Worker(QObject *parent = 0);

signals:
    void finished();

public slots:
    void process();
};

#endif // WORKER_H
