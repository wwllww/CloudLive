#ifndef BASEAPI_H
#define BASEAPI_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>

class BaseApi : public QObject
{
    Q_OBJECT
public:
    explicit BaseApi(QObject *parent = 0);
    void get(const QString url);
    void post(const QString url, const QByteArray &data);

protected:
    virtual void requestFinished(QNetworkReply *reply, const QByteArray data, const int statusCode) = 0;

public slots:
    void serviceRequestFinished(QNetworkReply *reply);

private:
    QNetworkRequest httpRequest;
    QNetworkAccessManager networkAccessManager;

public slots:
};

#endif // BASEAPI_H
